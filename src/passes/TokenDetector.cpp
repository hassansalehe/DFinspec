/////////////////////////////////////////////////////////////////
//  DFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    Copyright (c) 2015 - 2018 Hassan Salehe Matar
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Instrumentation pass for identifying token functions in the runtime.

#include "Excludes.hpp"

namespace {

/// TokenDetector: instrument the code in module to find in/out tokens
struct TokenDetector : public llvm::FunctionPass {
  static char ID;  // Pass identification, replacement for typeid.

  TokenDetector() : llvm::FunctionPass(ID) {}

  llvm::StringRef getPassName() const override {
    return "TokenDetector";
  }

  bool doInitialization(llvm::Module &M) override {
    INS::InitializeSignatures();
  }

  bool runOnFunction(llvm::Function &F) override {
    if (INS::isPassTokenFunc(F.getName())) {
      return instrumentTokens(F); // the function with tokens
    }

    if (INS::isRuntimeInitializer(F.getName())) {
      return insertVoidFunction(F, "INS_Init");
    }

    if (INS::isRuntimeTerminator(F.getName())) {
      return insertVoidFunction(F, "INS_Fini");
    }
    // function not modified
    return false;
  }

  /**
   * This method given a function name, constructs an
   * insertable callback function and inserts at the beginning
   * of the application function F.
   * @param F the function object from the application
   * @param functionName the name of the function to be inserted
   * @return boolean, true if the function was inserted
   */
  bool insertVoidFunction(
      llvm::Function &F,
      llvm::StringRef functionName) {

    // create finction
    llvm::Module &M = *F.getParent();
    llvm::IRBuilder<> IRB(M.getContext());
    llvm::Function * INS_func = checkSanitizerInterfaceFunction(
	      M.getOrInsertFunction(functionName,
            IRB.getVoidTy(), IRB.getVoidTy(), nullptr));

    // insert function
    llvm::IRBuilder<> entryPoint(F.getEntryBlock().getFirstNonPHI());
    entryPoint.CreateCall(INS_func);
    return true;
  }

  bool instrumentTokens(llvm::Function &F) {

    bool Modified = false;
    // Create callback to get the tokens
    llvm::Module &M = *F.getParent();
    llvm::IRBuilder<> fIRB(M.getContext());
    llvm::Type * IntTy = M.getDataLayout().getIntPtrType(M.getContext());

    llvm::Function * INS_RegSendToken = checkSanitizerInterfaceFunction(
	      M.getOrInsertFunction("INS_RegSendToken", fIRB.getVoidTy(),
		        fIRB.getInt8PtrTy(), fIRB.getInt8PtrTy(), IntTy, nullptr));

    //loop through the function body to find memcpy calls
    for (auto &BB : F) {
	    for (auto &Inst : BB) {
        if (llvm::isa<llvm::MemIntrinsic>(Inst)) {
          llvm::CallInst *M = llvm::dyn_cast<llvm::MemIntrinsic>(&Inst);
          llvm::Function *calledF = M->getCalledFunction();
          llvm::IRBuilder<> IRB(&Inst);

          if ( llvm::MemTransferInst *M =
              llvm::dyn_cast<llvm::MemTransferInst>(&Inst) ) {
            if ( llvm::isa<llvm::MemCpyInst>(M) ) {
              // memcpy(newtoken->value, tokendata, token_size);
              // for accessing tokens
              IRB.CreateCall(INS_RegSendToken,
                {  IRB.CreatePointerCast(
                       M->getArgOperand(0), IRB.getInt8PtrTy()), // newtoken
		               IRB.CreatePointerCast(
                       M->getArgOperand(1), IRB.getInt8PtrTy()),  // tokendata
		               IRB.CreateIntCast(
                       M->getArgOperand(2), IntTy, false)
                });       // tokensize
            }
            Modified = true;
          }
        }
      }
    }
    return Modified;
  }

  bool doFinalization(llvm::Module &M) override {
    INS::ClearSignatures();
  }
}; // end struct

}  // end namespace

char TokenDetector::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerTokenDetector(
    const llvm::PassManagerBuilder &,
		llvm::legacy::PassManagerBase &PM) {
  PM.add(new TokenDetector());
}

static llvm::RegisterStandardPasses
    RegisterMyPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
		registerTokenDetector);
