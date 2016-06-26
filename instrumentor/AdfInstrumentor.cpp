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

// Instrumentation pass for memory accesses and other actions.

#include "Excludes.h"

//include for logging IR codes for verification of nondeterministic bugs
#include "IIRlogger.h"

using namespace llvm;

/*
The necesssary steps:
  1. Identify the tasks
  2. Instrument the body of the tasks
  3. A callback to generate id of the task
  4. A callback to register the termination of the task
  5. Identify all incoming tokens
  6. Identify all outgoing tokens
*/

namespace {
// /// AdfSanitizer: instrument the code in module to find memory accesses
 struct AdfSanitizer : public FunctionPass {
   AdfSanitizer() : FunctionPass(ID) {}
   const char *getPassName() const override;
   bool runOnFunction(Function &F) override;
   bool doInitialization(Module &M) override;
   bool doFinalization(Module &M) override {
     INS::ClearSignatures();
   }
   static char ID;  // Instrumentation Pass identification, replacement for typeid.

 private:

   Constant* getLineNumber(Instruction* I) {

     if (auto Loc = I->getDebugLoc()) { // Here I is an LLVM instruction
       IRBuilder<> IRB(I);
       unsigned Line = Loc->getLine();
       //StringRef File = Loc->getFilename();
       // StringRef Dir = Loc->getDirectory();
     //if (MDNode *N = I->getMetadata("dbg")) {
     //  DILocation Loc(N);
     //  unsigned Line = Loc.getLineNumber();
       return ConstantInt::get(Type::getInt32Ty(I->getContext()), Line);
     }
     else {
       Constant* zero = ConstantInt::get(Type::getInt32Ty(I->getContext()), 0);
       return zero;
     }
   }

   void initializeCallbacks(Module &M);
   bool instrumentLoadOrStore(Instruction *I, const DataLayout &DL);
   bool instrumentAtomic(Instruction *I, const DataLayout &DL);
   bool instrumentMemIntrinsic(Instruction *I);
   void chooseInstructionsToInstrument(SmallVectorImpl<Instruction *> &Local,
                                      SmallVectorImpl<Instruction *> &All,
                                      const DataLayout &DL);
   bool addrPointsToConstantData(Value *Addr);
   int getMemoryAccessFuncIndex(Value *Addr, const DataLayout &DL);

   Type *IntptrTy;
   IntegerType *OrdTy;
//   // Callbacks to run-time library are computed in doInitialization.
     Function *INS_TaskStartFunc;
     Function *INS_TaskFinishFunc;

     // register every new instrumented function
     Value *funcNamePtr = NULL;
     Function *INS_TaskStartFunc2;

     // callbacks for registering tokens
     Function *INS_RegInToken;
     //Function *INS_RegOutToken;

     // callback for task creation
     Function *AdfCreateTask;

     // callbacks for instrumenting doubles and floats
     Constant *INS_MemWriteFloat;
     Constant *INS_MemWriteDouble;
//   // Accesses sizes are powers of two: 1, 2, 4, 8, 16.
   static const size_t kNumberOfAccessSizes = 5;
  Function *TsanRead[kNumberOfAccessSizes];
  Function *INS_MemWrite[kNumberOfAccessSizes];
  Function *TsanUnalignedRead[kNumberOfAccessSizes];
  Function *TsanUnalignedWrite[kNumberOfAccessSizes];
  Function *TsanAtomicLoad[kNumberOfAccessSizes];
  Function *TsanAtomicStore[kNumberOfAccessSizes];
  Function *TsanAtomicRMW[AtomicRMWInst::LAST_BINOP + 1][kNumberOfAccessSizes];
  Function *TsanAtomicCAS[kNumberOfAccessSizes];
  Function *TsanAtomicThreadFence;
  Function *TsanAtomicSignalFence;
  Function *toolVptrUpdate;
  Function *TsanVptrLoad;
  Function *MemmoveFn, *MemcpyFn, *MemsetFn, *hassanFn;
//   Function *TsanCtorFunction;
 };
 }  // namespace
//
 char AdfSanitizer::ID = 0;
//
 const char *AdfSanitizer::getPassName() const {
   return "AdfSanitizer";
 }

//
// FunctionPass *llvm::createAdfSanitizerPass() {
//   return new AdfSanitizer();
// }

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerAdfSanitizer(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new AdfSanitizer());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerAdfSanitizer);

 void AdfSanitizer::initializeCallbacks(Module &M) {
   IRBuilder<> IRB(M.getContext());
  // Initialize the callbacks.


  // callbacks for tokens collection
  INS_RegInToken = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "INS_RegInToken", IRB.getVoidTy(), IRB.getInt8PtrTy(),  IRB.getInt8Ty(), nullptr));

  //INS_RegOutToken = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
  //    "INS_RegOutToken", IRB.getVoidTy(), IRB.getInt8PtrTy(),  IRB.getInt8PtrTy(), IntptrTy, nullptr));

  // callback for task creation
 AdfCreateTask = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "AdfCreateTask", IRB.getVoidTy(), IRB.getInt8PtrTy(),  nullptr));


  INS_TaskStartFunc = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "INS_TaskStartFunc", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

  // register every executed function.
  INS_TaskStartFunc2 = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "INS_TaskStartFunc2", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

  INS_TaskFinishFunc = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("INS_TaskFinishFunc", IRB.getVoidTy(), nullptr));
  OrdTy = IRB.getInt32Ty();

  // functions to instrument floats and doubles
  LLVMContext &Ctx = M.getContext();
  INS_MemWriteFloat = M.getOrInsertFunction("INS_AdfMemWriteFloat",
	Type::getVoidTy(Ctx), Type::getFloatPtrTy(Ctx), Type::getFloatTy(Ctx), Type::getInt8Ty(Ctx), NULL);

  INS_MemWriteDouble = M.getOrInsertFunction("INS_AdfMemWriteDouble",
	Type::getVoidTy(Ctx), Type::getDoublePtrTy(Ctx), Type::getDoubleTy(Ctx), Type::getInt8Ty(Ctx), NULL);

  for (size_t i = 0; i < kNumberOfAccessSizes; ++i) {
    const unsigned ByteSize = 1U << i;
    const unsigned BitSize = ByteSize * 8;
    std::string ByteSizeStr = utostr(ByteSize);
    std::string BitSizeStr = utostr(BitSize);
    SmallString<32> ReadName("INS_AdfMemRead" + ByteSizeStr);
    TsanRead[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        ReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    SmallString<32> WriteName("INS_AdfMemWrite" + ByteSizeStr);
    INS_MemWrite[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        WriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr));

    SmallString<64> UnalignedReadName("__tsan_unaligned_read" + ByteSizeStr);
    TsanUnalignedRead[i] =
        checkSanitizerInterfaceFunction(M.getOrInsertFunction(
            UnalignedReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    SmallString<64> UnalignedWriteName("__tsan_unaligned_write" + ByteSizeStr);
    TsanUnalignedWrite[i] =
        checkSanitizerInterfaceFunction(M.getOrInsertFunction(
            UnalignedWriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    Type *Ty = Type::getIntNTy(M.getContext(), BitSize);
    Type *PtrTy = Ty->getPointerTo();
    SmallString<32> AtomicLoadName("__tsan_atomic" + BitSizeStr + "_load");
    TsanAtomicLoad[i] = checkSanitizerInterfaceFunction(
        M.getOrInsertFunction(AtomicLoadName, Ty, PtrTy, OrdTy, nullptr));

    SmallString<32> AtomicStoreName("__tsan_atomic" + BitSizeStr + "_store");
    TsanAtomicStore[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        AtomicStoreName, IRB.getVoidTy(), PtrTy, Ty, OrdTy, nullptr));

    for (int op = AtomicRMWInst::FIRST_BINOP;
        op <= AtomicRMWInst::LAST_BINOP; ++op) {
      TsanAtomicRMW[op][i] = nullptr;
      const char *NamePart = nullptr;
      if (op == AtomicRMWInst::Xchg)
        NamePart = "_exchange";
      else if (op == AtomicRMWInst::Add)
        NamePart = "_fetch_add";
      else if (op == AtomicRMWInst::Sub)
        NamePart = "_fetch_sub";
      else if (op == AtomicRMWInst::And)
        NamePart = "_fetch_and";
      else if (op == AtomicRMWInst::Or)
        NamePart = "_fetch_or";
      else if (op == AtomicRMWInst::Xor)
        NamePart = "_fetch_xor";
      else if (op == AtomicRMWInst::Nand)
        NamePart = "_fetch_nand";
      else
        continue;
      SmallString<32> RMWName("__tsan_atomic" + itostr(BitSize) + NamePart);
      TsanAtomicRMW[op][i] = checkSanitizerInterfaceFunction(
          M.getOrInsertFunction(RMWName, Ty, PtrTy, Ty, OrdTy, nullptr));
    }

    SmallString<32> AtomicCASName("__tsan_atomic" + BitSizeStr +
                                  "_compare_exchange_val");
    TsanAtomicCAS[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        AtomicCASName, Ty, PtrTy, Ty, Ty, OrdTy, OrdTy, nullptr));
  }
  toolVptrUpdate = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("toolVptrUpdate", IRB.getVoidTy(),
                            IRB.getInt8PtrTy(), IRB.getInt8PtrTy(), nullptr));
  TsanVptrLoad = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "__tsan_vptr_read", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));
  TsanAtomicThreadFence = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "__tsan_atomic_thread_fence", IRB.getVoidTy(), OrdTy, nullptr));
  TsanAtomicSignalFence = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "__tsan_atomic_signal_fence", IRB.getVoidTy(), OrdTy, nullptr));

  MemmoveFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memmove", IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt8PtrTy(), IntptrTy, nullptr));
  MemcpyFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memcpy", IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt8PtrTy(), IntptrTy, nullptr));
  MemsetFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memset", IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt32Ty(), IntptrTy, nullptr));

 hassanFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("hassanFun", IRB.getInt8PtrTy(),
                            IRB.getInt32Ty(), nullptr));

 }


 bool AdfSanitizer::doInitialization(Module &M) {
   errs() << "Hi my student "<< M.getName()<< "\n";

   // initialization of task IIR logger.
   IIRlog::Init( M.getName() );

   //for (Function &F : M)
   //  errs() << INS::demangleName(F.getName()) << "\n";
   CallGraph myGraph(M);
   //myGraph.dump();
   INS::InitializeSignatures();

   const DataLayout &DL = M.getDataLayout();
   IntptrTy = DL.getIntPtrType(M.getContext());
//   std::tie(TsanCtorFunction, std::ignore) = createSanitizerCtorAndInitFunctions(
//       M, kTsanModuleCtorName, kTsanInitName, /*InitArgTypes=*/{},
//       /*InitArgs=*/{});
//
//   appendToGlobalCtors(M, TsanCtorFunction, 0);
//
//   return true;
   return false;
 }

static bool isVtableAccess(Instruction *I) {
  if (MDNode *Tag = I->getMetadata(LLVMContext::MD_tbaa))
    return Tag->isTBAAVtableAccess();
  return false;
}

bool AdfSanitizer::addrPointsToConstantData(Value *Addr) {
  // If this is a GEP, just analyze its pointer operand.
  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Addr))
    Addr = GEP->getPointerOperand();

  if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Addr)) {
    if (GV->isConstant()) {
      // Reads from constant globals can not race with any writes.
      //NumOmittedReadsFromConstantGlobals++;
      return true;
    }
  } else if (LoadInst *L = dyn_cast<LoadInst>(Addr)) {
    if (isVtableAccess(L)) {
      // Reads from a vtable pointer can not race with any writes.
      //NumOmittedReadsFromVtable++;
      return true;
    }
  }
  return false;
}

// // Instrumenting some of the accesses may be proven redundant.
// // Currently handled:
// //  - read-before-write (within same BB, no calls between)
// //  - not captured variables
// //
// // We do not handle some of the patterns that should not survive
// // after the classic compiler optimizations.
// // E.g. two reads from the same temp should be eliminated by CSE,
// // two writes should be eliminated by DSE, etc.
// //
// // 'Local' is a vector of insns within the same BB (no calls between).
// // 'All' is a vector of insns that will be instrumented.
void AdfSanitizer::chooseInstructionsToInstrument(
    SmallVectorImpl<Instruction *> &Local, SmallVectorImpl<Instruction *> &All,
    const DataLayout &DL) {
  SmallSet<Value*, 8> WriteTargets;
  // Iterate from the end.
  for (SmallVectorImpl<Instruction*>::reverse_iterator It = Local.rbegin(),
       E = Local.rend(); It != E; ++It) {
    Instruction *I = *It;
    if (StoreInst *Store = dyn_cast<StoreInst>(I)) {
      WriteTargets.insert(Store->getPointerOperand());
    } else {
      LoadInst *Load = cast<LoadInst>(I);
      Value *Addr = Load->getPointerOperand();
      if (WriteTargets.count(Addr)) {
        // We will write to this temp, so no reason to analyze the read.
        //NumOmittedReadsBeforeWrite++;
        continue;
      }
      if (addrPointsToConstantData(Addr)) {
        // Addr points to some constant data -- it can not race with any writes.
        continue;
      }
    }
    Value *Addr = isa<StoreInst>(*I)
        ? cast<StoreInst>(I)->getPointerOperand()
        : cast<LoadInst>(I)->getPointerOperand();
    if (isa<AllocaInst>(GetUnderlyingObject(Addr, DL)) &&
        !PointerMayBeCaptured(Addr, true, true)) {
      // The variable is addressable but not captured, so it cannot be
      // referenced from a different thread and participate in a data race
      // (see llvm/Analysis/CaptureTracking.h for details).
      //NumOmittedNonCaptured++;
      continue;
    }
    All.push_back(I);
  }
  Local.clear();
}

static bool isAtomic(Instruction *I) {
  if (LoadInst *LI = dyn_cast<LoadInst>(I))
    return LI->isAtomic() && LI->getSynchScope() == CrossThread;
  if (StoreInst *SI = dyn_cast<StoreInst>(I))
    return SI->isAtomic() && SI->getSynchScope() == CrossThread;
  if (isa<AtomicRMWInst>(I))
    return true;
  if (isa<AtomicCmpXchgInst>(I))
    return true;
  if (isa<FenceInst>(I))
    return true;
  return false;
}

 bool AdfSanitizer::runOnFunction(Function &F) {
  // This is required to prevent instrumenting call to __tsan_init from within
  // the module constructor.
  if (INS::DontInstrument(F.getName()))
     return false;

  bool Res = false;
  bool HasCalls = false;
  bool isTaskBody = INS::isTaskBodyFunction( F.getName() );

  if(isTaskBody) {
    StringRef name = INS::demangleName(F.getName());
    auto idx = name.find('(');
    if(idx != StringRef::npos)
      name = name.substr(0, idx);

    IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    Value *taskName = IRB.CreateGlobalStringPtr(name, "taskName");
    IRB.CreateCall(INS_TaskStartFunc, {IRB.CreatePointerCast(taskName, IRB.getInt8PtrTy())});

    Res = true;

    /* Log all its body statements for verification of nondererminism bugs */
    IIRlog::LogNewTask( name );
    for (auto &BB : F) {
      for (auto &Inst : BB) {
        unsigned lineNo = 0;
         if(auto Loc = Inst.getDebugLoc())
           lineNo = Loc->getLine();

        IIRlog::LogNewIIRcode( lineNo, Inst);
      }
    }

  }

  // Register task name
  StringRef funcName = INS::demangleName(F.getName());
  IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
  funcNamePtr = IRB.CreateGlobalStringPtr(funcName, "functionName");
  //IRB.CreateCall(INS_TaskStartFunc2, {IRB.CreatePointerCast(funcNamePtr, IRB.getInt8PtrTy())});

//else return false;
   /*
   if ( isTaskBodyFunction(F) ) {
       instrumentReturns(F);
       instrumentOutTokens();

   }
  //SymbolTableList<Argument> attSet =

  auto xx = F.arg_begin();
  for(;xx != F.arg_end();xx++)
  {
     Value *x = xx;
     x->dump();
     //if(isa<IntegerType>(&*xx))
        //errs() << x->getType() << "\n";
  }
  errs() << "ARGUMENT LENGTH " << F.arg_size() << "\n";
//   if (&F == TsanCtorFunction)
//     return false;
//   errs() << F.getName() << "\n";
  */
  initializeCallbacks(*F.getParent());
  SmallVector<Instruction*, 8> RetVec;
  SmallVector<Instruction*, 8> AllLoadsAndStores;
  SmallVector<Instruction*, 8> LocalLoadsAndStores;
  SmallVector<Instruction*, 8> AtomicAccesses;
  SmallVector<Instruction*, 8> MemIntrinCalls;

//   bool SanitizeFunction = F.hasFnAttribute(Attribute::SanitizeThread);
  const DataLayout &DL = F.getParent()->getDataLayout();

//   // Traverse all instructions, collect loads/stores/returns, check for calls.
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      //Inst.dump();
      if (isAtomic(&Inst))
        AtomicAccesses.push_back(&Inst);
      else if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst))
        LocalLoadsAndStores.push_back(&Inst);
      else if (isa<ReturnInst>(Inst))
        RetVec.push_back(&Inst);
      else if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst)) {
        //Inst.dump();
        // hassan
        if(isa<CallInst>(Inst))
        {
          CallInst *M = dyn_cast<CallInst>(&Inst);
          Function *calledF = M->getCalledFunction();
          if(calledF)
          //errs() << "  CALL: " << INS::demangleName(calledF->getName()) << "\n";
          if(!calledF)
            continue;
          /*
          if(isTaskBody && INS::isPassTokenFunc(calledF->getName()))
          {
            IRBuilder<> IRB(&Inst);
            IRB.CreateCall(INS_RegOutToken,
              {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
               IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
               IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

          }
          if(calledF && calledF->arg_begin() != calledF->arg_end() && ! INS::isTaskBodyFunction(calledF->getName())) {
            IRBuilder<> IRB(&Inst);
            if(calledF->arg_size() >= 2 && !INS::isLLVMCall(calledF->getName())) {
              errs() << "A CALL INSTRUCTION TO "<< INS::demangleName(calledF->getName()) << "\n";
              IRB.CreateCall(hassanFn,
                    { IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
                      IRB.CreateIntCast(M->getArgOperand(1), IRB.getInt32Ty(), false)//,
                      // IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)
                    });
            }
          }
          */
        }
        else
        {
          InvokeInst *M = dyn_cast<InvokeInst>(&Inst);
          Function *calledF = M->getCalledFunction();
          //if(calledF)
          //errs() << "  INVOKE: " << INS::demangleName(calledF->getName()) << "\n";
          if(!calledF)
            continue;
          if(INS::isTaskCreationFunc(calledF->getName()))
          {
//             IRBuilder<> IRB(&Inst);
//             IRB.CreateCall(AdfCreateTask,
//               {IRB.CreatePointerCast(M->getArgOperand(2), IRB.getInt8PtrTy()),
//                IRB.CreatePointerCast(M->getArgOperand(3), IRB.getInt8PtrTy())//,
//                //IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)
//               });
              /*
              auto xx = calledF->arg_begin();
              int o=0;
              for(;xx != calledF->arg_end(); xx++)
              {
                   o++;
                   Value *x = xx;
                   //errs() << "ARG: ";
                   //x->dump();
                   if(o==4) {
		     CallInst * CI = dyn_cast<CallInst>(x);
		     if(CI) {
		      Function *fn = dyn_cast<Function>(CI->getCalledFunction()->stripPointerCasts());
		      if(fn) {
			errs() <<"GGG" << x << "\n";
		      }
		     }
                   }

                   //if(isa<IntegerType>(&*xx))
                   //errs() << x->getType()->getName() << "\n";
              }*/
          }
        }

        if (isa<MemIntrinsic>(Inst)) {
          MemIntrinCalls.push_back(&Inst);
          CallInst *M = dyn_cast<MemIntrinsic>(&Inst);
          Function *calledF = M->getCalledFunction();
          //if(calledF)
          //errs() << "  MEM CALL: " << INS::demangleName(calledF->getName()) << "\n";

        }
        HasCalls = true;
        chooseInstructionsToInstrument(LocalLoadsAndStores, AllLoadsAndStores, DL);
      }
    }
    chooseInstructionsToInstrument(LocalLoadsAndStores, AllLoadsAndStores, DL);
  }

  // We have collected all loads and stores.
  // FIXME: many of these accesses do not need to be checked for races
  // (e.g. variables that do not escape, etc).

  // Instrument memory accesses only if we want to report bugs in the function.
  //!HASSAN if (ClInstrumentMemoryAccesses && SanitizeFunction)
    for (auto Inst : AllLoadsAndStores) {
      Res |= instrumentLoadOrStore(Inst, DL);
    }

  // Instrument atomic memory accesses in any case (they can be used to
  // implement synchronization).
  //!HASSAN if (ClInstrumentAtomics)
    for (auto Inst : AtomicAccesses) {
      Res |= instrumentAtomic(Inst, DL);
    }

  //!HASSAN if (ClInstrumentMemIntrinsics && SanitizeFunction)
    for (auto Inst : MemIntrinCalls) {

      MemTransferInst *M = dyn_cast<MemTransferInst>(Inst);
      if (isTaskBody && M) {
	IRBuilder<> IRB(Inst);
	//errs() << "memTransfer HHHHHHHHHHHHHHHHH";
	//Inst->dump();
	// for accessing tokens
	IRB.CreateCall(INS_RegInToken,
	    {//IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
	    IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
	    IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});
      }

      Res |= instrumentMemIntrinsic(Inst);
    }

  // Instrument function entry/exit points if there were instrumented accesses.
  // insert call at the start of the task
  if (isTaskBody && (Res || HasCalls) /*!HASSAN && ClInstrumentFuncEntryExit*/) {
    IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    Value *ReturnAddress = IRB.CreateCall(Intrinsic::getDeclaration(F.getParent(), Intrinsic::returnaddress),
                                         IRB.getInt32(0));
    //IRB.CreateCall(INS_TaskStartFunc, ReturnAddress);
    if(isTaskBody) {  // instrument returns of a task body to track when it finishes.
      for (auto RetInst : RetVec) {
        IRBuilder<> IRBRet(RetInst);
        IRBRet.CreateCall(INS_TaskFinishFunc, {});
      }
    }
    Res = true;
  }
   return Res;
 }

bool AdfSanitizer::instrumentLoadOrStore(Instruction *I, const DataLayout &DL) {
  IRBuilder<> IRB(I);
  bool IsWrite = isa<StoreInst>(*I);
  Value *Addr = IsWrite
      ? cast<StoreInst>(I)->getPointerOperand()
      : cast<LoadInst>(I)->getPointerOperand();

  int Idx = getMemoryAccessFuncIndex(Addr, DL);
 if (Idx < 0)
    return false;
  // HASSAN
  //if (Idx >= 3) I->dump();

  if (IsWrite && isVtableAccess(I)) {
   // DEBUG(dbgs() << "  VPTR : " << *I << "\n");
    Value *StoredValue = cast<StoreInst>(I)->getValueOperand();
    // StoredValue may be a vector type if we are storing several vptrs at once.
    // In this case, just take the first element of the vector since this is
    // enough to find vptr races.
    if (isa<VectorType>(StoredValue->getType()))
      StoredValue = IRB.CreateExtractElement(
          StoredValue, ConstantInt::get(IRB.getInt32Ty(), 0));
    if (StoredValue->getType()->isIntegerTy())
      StoredValue = IRB.CreateIntToPtr(StoredValue, IRB.getInt8PtrTy());
    // Call toolVptrUpdate.
    IRB.CreateCall(toolVptrUpdate,
                   {IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                    IRB.CreatePointerCast(StoredValue, IRB.getInt8PtrTy())});
    //NumInstrumentedVtableWrites++;
    return true;
  }
  if (!IsWrite && isVtableAccess(I)) {
    IRB.CreateCall(TsanVptrLoad,
                   IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()));
    //NumInstrumentedVtableReads++;
    return true;
  }
  const unsigned Alignment = IsWrite ? cast<StoreInst>(I)->getAlignment() : cast<LoadInst>(I)->getAlignment();
  Type *OrigTy = cast<PointerType>(Addr->getType())->getElementType();
  const uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  Value *OnAccessFunc = nullptr;
  if (Alignment == 0 || Alignment >= 8 || (Alignment % (TypeSize / 8)) == 0)
    OnAccessFunc = IsWrite ? INS_MemWrite[Idx] : TsanRead[Idx];
  else
    OnAccessFunc = IsWrite ? TsanUnalignedWrite[Idx] : TsanUnalignedRead[Idx];

  if(IsWrite)
  {

    Value *Val = cast<StoreInst>(I)->getValueOperand();
    Constant* LineNo = getLineNumber(I);
    if(Val->getType()->isFloatTy())
    {
      Value* args[] = {Addr, Val, LineNo, funcNamePtr};
      IRB.CreateCall(INS_MemWriteFloat, args);
    }
    else if(Val->getType()->isDoubleTy())
    {
      Value* args[] = {Addr, Val, LineNo, funcNamePtr};
      IRB.CreateCall(INS_MemWriteDouble, args);
    }
    else if(Val->getType()->isIntegerTy())
    {
#ifdef DEBUG
       unsigned lineNo = 0;
       if(auto Loc = I->getDebugLoc())
          lineNo = Loc->getLine();
       if(lineNo == 1172)
       {
         errs() << "Unknown ";
         Val->getType()->dump();
         I->dump();
         errs() << "Idx " << Idx << " Alignment " << Alignment << "\n";
         LineNo->dump();
         errs() << "Computed:  " << lineNo << "\n";
         //errs() << INS::demangleName(F.getName()) << "\n";
         errs() << "========\n";
       }
#endif
       IRB.CreateCall(OnAccessFunc,
		{IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
		 //IRB.CreatePointerCast(Val, Val->getType())
		 Val, LineNo, funcNamePtr
		});
    }
    else
      IRB.CreateCall(OnAccessFunc,
                 {IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                   //IRB.CreatePointerCast(Val, Val->getType())
                   Val, LineNo, funcNamePtr
                });

  }else
    IRB.CreateCall(OnAccessFunc,
		 {IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy())});
  //if (IsWrite) NumInstrumentedWrites++;
  //else         NumInstrumentedReads++;
  return true;
}

static ConstantInt *createOrdering(IRBuilder<> *IRB, AtomicOrdering ord) {
  uint32_t v = 0;
  switch (ord) {
    case AtomicOrdering::NotAtomic: llvm_unreachable("unexpected atomic ordering!");
    case AtomicOrdering::Unordered:              // Fall-through.
    case AtomicOrdering::Monotonic:              v = 0; break;
    // case AtomicOrdering::Consume:                v = 1; break;  // Not specified yet.
    case AtomicOrdering::Acquire:                v = 2; break;
    case AtomicOrdering::Release:                v = 3; break;
    case AtomicOrdering::AcquireRelease:         v = 4; break;
    case AtomicOrdering::SequentiallyConsistent: v = 5; break;
  }
  return IRB->getInt32(v);
}

// // If a memset intrinsic gets inlined by the code gen, we will miss races on it.
// // So, we either need to ensure the intrinsic is not inlined, or instrument it.
// // We do not instrument memset/memmove/memcpy intrinsics (too complicated),
// // instead we simply replace them with regular function calls, which are then
// // intercepted by the run-time.
// // Since tsan is running after everyone else, the calls should not be
// // replaced back with intrinsics. If that becomes wrong at some point,
// // we will need to call e.g. __tsan_memset to avoid the intrinsics.
bool AdfSanitizer::instrumentMemIntrinsic(Instruction *I) {
  IRBuilder<> IRB(I);
  if (MemSetInst *M = dyn_cast<MemSetInst>(I)) {

    IRB.CreateCall(
        MemsetFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(1), IRB.getInt32Ty(), false),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    I->eraseFromParent();
  } else if (MemTransferInst *M = dyn_cast<MemTransferInst>(I)) {
    IRB.CreateCall(
        isa<MemCpyInst>(M) ? MemcpyFn : MemmoveFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    I->eraseFromParent();
  }
  return false;
}

// // Both llvm and AdfSanitizer atomic operations are based on C++11/C1x
// // standards.  For background see C++11 standard.  A slightly older, publicly
// // available draft of the standard (not entirely up-to-date, but close enough
// // for casual browsing) is available here:
// // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf
// // The following page contains more background information:
// // http://www.hpl.hp.com/personal/Hans_Boehm/c++mm/
//
bool AdfSanitizer::instrumentAtomic(Instruction *I, const DataLayout &DL) {
  IRBuilder<> IRB(I);
  if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
    Value *Addr = LI->getPointerOperand();
    int Idx = getMemoryAccessFuncIndex(Addr, DL);
//    if (Idx < 0)
      return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     Type *Ty = Type::getIntNTy(IRB.getContext(), BitSize);
//     Type *PtrTy = Ty->getPointerTo();
//     Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      createOrdering(&IRB, LI->getOrdering())};
//     CallInst *C = CallInst::Create(TsanAtomicLoad[Idx], Args);
//     ReplaceInstWithInst(I, C);
//
   } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
//     Value *Addr = SI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     Type *Ty = Type::getIntNTy(IRB.getContext(), BitSize);
//     Type *PtrTy = Ty->getPointerTo();
//     Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(SI->getValueOperand(), Ty, false),
//                      createOrdering(&IRB, SI->getOrdering())};
//     CallInst *C = CallInst::Create(TsanAtomicStore[Idx], Args);c
//     ReplaceInstWithInst(I, C);
   } else if (AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(I)) {
//     Value *Addr = RMWI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     Function *F = TsanAtomicRMW[RMWI->getOperation()][Idx];
//     if (!F)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     Type *Ty = Type::getIntNTy(IRB.getContext(), BitSize);
//     Type *PtrTy = Ty->getPointerTo();
//     Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(RMWI->getValOperand(), Ty, false),
//                      createOrdering(&IRB, RMWI->getOrdering())};
//     CallInst *C = CallInst::Create(F, Args);
//     ReplaceInstWithInst(I, C);
   } else if (AtomicCmpXchgInst *CASI = dyn_cast<AtomicCmpXchgInst>(I)) {
//     Value *Addr = CASI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     Type *Ty = Type::getIntNTy(IRB.getContext(), BitSize);
//     Type *PtrTy = Ty->getPointerTo();
//     Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(CASI->getCompareOperand(), Ty, false),
//                      IRB.CreateIntCast(CASI->getNewValOperand(), Ty, false),
//                      createOrdering(&IRB, CASI->getSuccessOrdering()),
//                      createOrdering(&IRB, CASI->getFailureOrdering())};
//     CallInst *C = IRB.CreateCall(TsanAtomicCAS[Idx], Args);
//     Value *Success = IRB.CreateICmpEQ(C, CASI->getCompareOperand());
//
//     Value *Res = IRB.CreateInsertValue(UndefValue::get(CASI->getType()), C, 0);
//     Res = IRB.CreateInsertValue(Res, Success, 1);
//
//     I->replaceAllUsesWith(Res);
//     I->eraseFromParent();
   } else if (FenceInst *FI = dyn_cast<FenceInst>(I)) {
//     Value *Args[] = {createOrdering(&IRB, FI->getOrdering())};
//     Function *F = FI->getSynchScope() == SingleThread ?
//         TsanAtomicSignalFence : TsanAtomicThreadFence;
//     CallInst *C = CallInst::Create(F, Args);
//     ReplaceInstWithInst(I, C);
   }
//   return true;
}

int AdfSanitizer::getMemoryAccessFuncIndex(Value *Addr, const DataLayout &DL) {
  Type *OrigPtrTy = Addr->getType();
  Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
  assert(OrigTy->isSized());
  uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  if (TypeSize != 8  && TypeSize != 16 &&
      TypeSize != 32 && TypeSize != 64 && TypeSize != 128) {
    //NumAccessesWithBadSize++;
    // Ignore all unusual sizes.
    return -1;
  }
  size_t Idx = countTrailingZeros(TypeSize / 8);
  assert(Idx < kNumberOfAccessSizes);
  return Idx;
}
