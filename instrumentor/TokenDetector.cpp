/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015,2016 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Instrumentation pass for identifying token functions in the runtime.

#include "Excludes.h"

using namespace llvm;

namespace {
  /// TokenDetectorr: instrument the code in module to find in/out tokens
  struct TokenDetector : public FunctionPass {

    static char ID;  // Instrumentation Pass identification, replacement for typeid.

    TokenDetector() : FunctionPass(ID) {}
    StringRef getPassName() const override{ return "TokenDetector"; }

    bool doInitialization(Module &M) override {
      INS::InitializeSignatures();
    }

    bool runOnFunction(Function &F) override {

      if(INS::isPassTokenFunc(F.getName())) {
	return instrumentTokens(F); // the function with tokens
      }

      if(INS::isRuntimeInitializer(F.getName())) {
	return insertVoidFunction(F, "INS_Init");
      }

      if(INS::isRuntimeTerminator(F.getName())) {
	return insertVoidFunction(F, "INS_Fini");
      }
      // function not modified
      return false;
    }

    /**
     * This method given a function name, constructs an insertable callback function
     * and inserts at the beginning of the application function F.
     * @param F the function object from the application
     * @param functionName the name of the function to be inserted
     * @return boolean, true if the function was inserted
     */
    bool insertVoidFunction(Function &F, StringRef functionName) {

      // create finction
      Module &M = *F.getParent();
      IRBuilder<> IRB(M.getContext());
      Function * INS_func = checkSanitizerInterfaceFunction(
	M.getOrInsertFunction(functionName, IRB.getVoidTy(), IRB.getVoidTy(), nullptr));

      // insert function
      IRBuilder<> entryPoint(F.getEntryBlock().getFirstNonPHI());
      entryPoint.CreateCall(INS_func);
      return true;
    }

    bool instrumentTokens(Function &F) {

      bool Modified = false;
      // Create callback to get the tokens
      Module &M = *F.getParent();
      IRBuilder<> fIRB(M.getContext());
      Type * IntTy = M.getDataLayout().getIntPtrType(M.getContext());

      Function * INS_RegOutToken = checkSanitizerInterfaceFunction(
	M.getOrInsertFunction("INS_RegOutToken", fIRB.getVoidTy(),
		fIRB.getInt8PtrTy(), fIRB.getInt8PtrTy(), IntTy, nullptr));

      //loop through the function body to find memcpy calls
      for (auto &BB : F) {
	for (auto &Inst : BB) {

//	  Inst.dump();

	  if (isa<MemIntrinsic>(Inst)) {
	    CallInst *M = dyn_cast<MemIntrinsic>(&Inst);
	    Function *calledF = M->getCalledFunction();

//	    if(calledF)
//	      errs() << "  MEM CALL: " << INS::demangleName(calledF->getName()) << "\n";

	    IRBuilder<> IRB(&Inst);

	    if (MemTransferInst *M = dyn_cast<MemTransferInst>(&Inst)) {

	      if(isa<MemCpyInst>(M)) { // memcpy(newtoken->value, tokendata, token_size);
		// for accessing tokens
		IRB.CreateCall(INS_RegOutToken,
		    {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()), // newtoken
		    IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),  // tokendata
		    IRB.CreateIntCast(M->getArgOperand(2), IntTy, false)});       // tokensize
	      }
	      Modified = true;
	    }
	  }
	}
      }
      return Modified;

    }


    bool doFinalization(Module &M) override {
      INS::ClearSignatures();
    }

  };
}  // namespace

char TokenDetector::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerTokenDetector(const PassManagerBuilder &,
			legacy::PassManagerBase &PM) {
  PM.add(new TokenDetector());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
		registerTokenDetector);

