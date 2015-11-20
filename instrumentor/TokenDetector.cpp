#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"

#include "Excludes.h"

using namespace llvm;

namespace {
  /// TokenDetectorr: instrument the code in module to find in/out tokens
  struct TokenDetector : public FunctionPass {

    static char ID;  // Instrumentation Pass identification, replacement for typeid.

    TokenDetector() : FunctionPass(ID) {}
    const char *getPassName() const override{ return "TokenDetector";}

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

    bool insertVoidFunction(Function &F, StringRef functionName) {
      errs() << "Function name" << functionName << "\n";
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

