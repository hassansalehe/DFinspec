#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Pass.h"


using namespace llvm;


namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    bool instrument(Value *Ptr, Value *InstVal) 
    {
      errs() << "Instrument (" << *Ptr << ") for (" << *InstVal<< ")\n";
      return false;
    }

    virtual bool runOnFunction(Function &F) {
      
      // 1. Identify some functions may be for exclusion
      errs() << "Function: " << F.getName() << "!\n";
      if(F.getName() != "love") // instrument only a function called love
        return false;

      //2. get the data layout
      const DataLayout &DL = F.getParent()->getDataLayout();

      // 3. get the function name
      string funcName = F.getName();
      
      //..
      // Get the function to call from our runtime library.
      LLVMContext &Ctx = F.getContext();
      Constant *logFunc = F.getParent()->getOrInsertFunction(
        "logop", Type::getVoidTy(Ctx), Type::getInt32Ty(Ctx), NULL
      );

      /*
      for (auto &B : F) {
        for (auto &I : B) {
          if (auto *op = dyn_cast<BinaryOperator>(&I)) {
            // Insert *after* `op`.
            op->dump();
            IRBuilder<> builder(op);
            builder.SetInsertPoint(&B, ++builder.GetInsertPoint());

            // Insert a call to our function.
            Value* args[] = {op, builder.CreatePointerCast(I.getOperand(0), builder.getInt8PtrTy())};
            builder.CreateCall(logFunc, args);

            return true;
          }
        }
      }
      */
      std::vector<Instruction*> WorkList;
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        Instruction *I = &*i;
        if (isa<LoadInst>(I) || isa<StoreInst>(I) || isa<AtomicCmpXchgInst>(I) ||
           isa<AtomicRMWInst>(I))
           WorkList.push_back(I);
      }
  bool MadeChange = false;
  for (std::vector<Instruction*>::iterator i = WorkList.begin(),
       e = WorkList.end(); i != e; ++i) {
    Instruction *Inst = *i;

    if (LoadInst *LI = dyn_cast<LoadInst>(Inst)) {
      MadeChange |= instrument(LI->getPointerOperand(), LI);
    } else if (StoreInst *SI = dyn_cast<StoreInst>(Inst)) {
      MadeChange |=
          instrument(SI->getPointerOperand(), SI->getValueOperand());
    } else if (AtomicCmpXchgInst *AI = dyn_cast<AtomicCmpXchgInst>(Inst)) {
      MadeChange |=
          instrument(AI->getPointerOperand(), AI->getCompareOperand());
    } else if (AtomicRMWInst *AI = dyn_cast<AtomicRMWInst>(Inst)) {
      MadeChange |=
          instrument(AI->getPointerOperand(), AI->getValOperand());
    } else {
      llvm_unreachable("unknown Instruction type");
    }
  }
      return MadeChange;
    }
  };
}

char SkeletonPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new SkeletonPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerSkeletonPass);
