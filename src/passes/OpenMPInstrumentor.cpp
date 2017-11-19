//////////////////////////////////////////////////////////////
//
// OpenMPInstrumentor.cpp
//
// Copyright (c) 2017, Hassan Salehe Matar
// All rights reserved.
//
// This file is part of DFinspec. For details, see
// https://github.com/hassansalehe/DFinspec. Please also
// see the LICENSE file for additional BSD notice
//
// Redistribution and use in source and binary forms, with
// or without modification, are permitted provided that
// the following conditions are met:
//
// * Redistributions of source code must retain the above
//   copyright notice, this list of conditions and the
//   following disclaimer.
//
// * Redistributions in binary form must reproduce the
//   above copyright notice, this list of conditions and
//   the following disclaimer in the documentation and/or
//   other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names
//   of its contributors may be used to endorse or promote
//   products derived from this software without specific
//   prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "OpenMPUtil.h"

namespace {

// Function pass to instrument OpenMP .cpp programs
// For detecting output nondeterminism.
struct OpenMPInstrumentorPass : public llvm::FunctionPass {

  static char ID;
  OpenMPInstrumentorPass() : FunctionPass(ID) {}

  llvm::StringRef getPassName() const override {
    return "OpenMPInstrumentorPass";
  }

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  llvm::Constant* getLineNumber(llvm::Instruction* I) {

    if (auto Loc = I->getDebugLoc()) {
      llvm::IRBuilder<> IRB(I);
      unsigned Line = Loc->getLine();
      return llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(I->getContext()), Line);
    }
    else {
      llvm::Constant* zero = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(I->getContext()), 0);
      return zero;
    }
  }

  bool runOnFunction(llvm::Function &F) override {
    // print function name
    llvm::errs().write_escaped(F.getName()) << '\n';

    // save the function body
   // dfinspec::logFunction(F);

    // return if main
    if(F.getName() == "main")
     return false;

    const llvm::DataLayout &DL = F.getParent()->getDataLayout();
    initializeCallbacks(*F.getParent());

    // Instrument store instructions
    for (auto &BB : F) {
      for (auto &Inst : BB) {

        if(llvm::StoreInst *instr = llvm::dyn_cast<llvm::StoreInst>(&Inst)) {

          llvm::errs() << Inst << "\n";
          llvm::Value * addr = instr->getPointerOperand();

          llvm::Type *OrigPtrTy = addr->getType();
          llvm:: Type *OrigTy = llvm::cast<llvm::PointerType>(OrigPtrTy)->getElementType();

          assert(OrigTy->isSized());

          uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);

          if (TypeSize != 8  && TypeSize != 16 && TypeSize != 32 && TypeSize != 64 && TypeSize != 128) {
             llvm::errs() << "Unsual size \n";
          }

          llvm::IRBuilder<> IRB(&Inst);
          llvm::Value *value = instr->getValueOperand();
          llvm::Constant *LineNo = getLineNumber(&Inst);
          llvm::StringRef name = F.getName();
          llvm::Value *taskName = IRB.CreateGlobalStringPtr(name, "taskName");

          llvm::Value *args[] = {addr, value, LineNo, taskName};

          if(INS_MemWrite4) {

            IRB.CreateCall(INS_MemWrite4, args);
            //    {IRB.CreatePointerCast(addr, IRB.getInt8PtrTy()), value, 0, taskName});
            //IRB.CreateCall(INS_TaskBeginFunc, {IRB.CreatePointerCast(taskName, IRB.getInt8PtrTy())});

            llvm::errs() << Inst << "\n";
          }
        }
      }
    }

    return true;
  }

  private:
  llvm::Constant * INS_MemWrite1;
  llvm::Constant * INS_MemWrite2;
  llvm::Constant * INS_MemWrite4;
  llvm::Constant * INS_MemWrite8;
  llvm::Constant * INS_MemWrite16;
  llvm::Constant * INS_TaskBeginFunc;

  void initializeCallbacks(llvm::Module & M);

}; // end of OpenMPInstrumentorPass
} // end of namespace

void OpenMPInstrumentorPass::initializeCallbacks(llvm::Module & M) {
  llvm::IRBuilder<> IRB(M.getContext());

  INS_MemWrite1 = M.getOrInsertFunction(
      "INS_AdfMemWrite1", IRB.getVoidTy(), IRB.getInt8PtrTy(),
      IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr);

  INS_MemWrite2 = M.getOrInsertFunction(
      "INS_AdfMemWrite2", IRB.getVoidTy(), IRB.getInt8PtrTy(),
      IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr);

  INS_MemWrite4 = M.getOrInsertFunction(
      "INS_AdfMemWrite4", IRB.getVoidTy(), IRB.getInt8PtrTy(),
      IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr);

  INS_MemWrite8 = M.getOrInsertFunction(
      "INS_AdfMemWrite8", IRB.getVoidTy(), IRB.getInt8PtrTy(),
      IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr);

  INS_MemWrite16 = M.getOrInsertFunction(
      "INS_AdfMemWrite16", IRB.getVoidTy(), IRB.getInt8PtrTy(),
      IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr);

  INS_TaskBeginFunc = M.getOrInsertFunction(
      "INS_TaskBeginFunc", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr);
}

char OpenMPInstrumentorPass::ID = 0;

static void registerOpenMPInstrumentorPass(
  const llvm::PassManagerBuilder &,
  llvm::legacy::PassManagerBase &PM) { PM.add(new OpenMPInstrumentorPass()); }

static llvm::RegisterStandardPasses regPass(
   llvm::PassManagerBuilder::EP_EarlyAsPossible,
   registerOpenMPInstrumentorPass);

