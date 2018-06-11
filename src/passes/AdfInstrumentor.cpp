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

// Instrumentation pass for memory accesses and other actions.

#include "Excludes.hpp"
#include "IIRlogger.hpp"

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
// /// DFinspec: instrument the code in module to find memory accesses
 struct DFinspec : public llvm::FunctionPass {
   DFinspec() : llvm::FunctionPass(ID) {}
   llvm::StringRef getPassName() const override;
   bool runOnFunction(llvm::Function &F) override;
   bool doInitialization(llvm::Module &M) override;
   bool doFinalization(llvm::Module &M) override {
     INS::ClearSignatures();
   }

   // Instrumentation Pass identification, replacement for typeid.
   static char ID;

 private:

   llvm::Constant *getLineNumber(llvm::Instruction *I) {

     if (auto Loc = I->getDebugLoc()) { // Here I is an LLVM instruction
       llvm::IRBuilder<> IRB(I);
       unsigned Line = Loc->getLine();
       //llvm::StringRef File = Loc->getFilename();
       // llvm::StringRef Dir = Loc->getDirectory();
     //if (MDNode *N = I->getMetadata("dbg")) {
     //  DILocation Loc(N);
     //  unsigned Line = Loc.getLineNumber();
       return llvm::ConstantInt::get(
           llvm::Type::getInt32Ty(I->getContext()), Line);
     }
     else {
       llvm::Constant* zero = llvm::ConstantInt::get(
           llvm::Type::getInt32Ty(I->getContext()), 0);
       return zero;
     }
   }

   void initializeCallbacks(llvm::Module &M);
   bool instrumentLoadOrStore(llvm::Instruction *I, const llvm::DataLayout &DL);
   bool instrumentAtomic(llvm::Instruction *I, const llvm::DataLayout &DL);
   bool instrumentMemIntrinsic(llvm::Instruction *I);
   void chooseInstructionsToInstrument(
       llvm::SmallVectorImpl<llvm::Instruction *> &Local,
       llvm::SmallVectorImpl<llvm::Instruction *> &All,
       const llvm::DataLayout &DL);
   bool addrPointsToConstantData(llvm::Value *Addr);
   int getMemoryAccessFuncIndex(llvm::Value *Addr, const llvm::DataLayout &DL);

   llvm::Type *IntptrTy;
   llvm::IntegerType *OrdTy;
//   // Callbacks to run-time library are computed in doInitialization.
     llvm::Function *INS_TaskBeginFunc;
     llvm::Function *INS_TaskFinishFunc;

     // register every new instrumented function
     llvm::Value *funcNamePtr = NULL;
     llvm::Function *INS_TaskBeginFunc2;

     // callbacks for registering tokens
     llvm::Function *INS_RegReceiveToken;
     //llvm::Function *INS_RegSendToken;

     // callback for task creation
     llvm::Function *AdfCreateTask;

     // callbacks for instrumenting doubles and floats
     llvm::Constant *INS_MemWriteFloat;
     llvm::Constant *INS_MemWriteDouble;
//   // Accesses sizes are powers of two: 1, 2, 4, 8, 16.
   static const size_t kNumberOfAccessSizes = 5;
  llvm::Function *TsanRead[kNumberOfAccessSizes];
  llvm::Function *INS_MemWrite[kNumberOfAccessSizes];
  llvm::Function *TsanUnalignedRead[kNumberOfAccessSizes];
  llvm::Function *TsanUnalignedWrite[kNumberOfAccessSizes];
  llvm::Function *TsanAtomicLoad[kNumberOfAccessSizes];
  llvm::Function *TsanAtomicStore[kNumberOfAccessSizes];
  llvm::Function *TsanAtomicRMW[llvm::AtomicRMWInst::LAST_BINOP + 1][kNumberOfAccessSizes];
  llvm::Function *TsanAtomicCAS[kNumberOfAccessSizes];
  llvm::Function *TsanAtomicThreadFence;
  llvm::Function *TsanAtomicSignalFence;
  llvm::Function *toolVptrUpdate;
  llvm::Function *TsanVptrLoad;
  llvm::Function *MemmoveFn, *MemcpyFn, *MemsetFn, *hassanFn;
//   llvm::Function *TsanCtorFunction;
 };
 }  // namespace
//
 char DFinspec::ID = 0;
//
 llvm::StringRef DFinspec::getPassName() const {
   return "DFinspec";
 }

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerDFinspec(const llvm::PassManagerBuilder &,
                             llvm::legacy::PassManagerBase &PM) {
  PM.add(new DFinspec());
}
static llvm::RegisterStandardPasses
  RegisterMyPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                 registerDFinspec);

 void DFinspec::initializeCallbacks(llvm::Module &M) {
   llvm::IRBuilder<> IRB(M.getContext());
  // Initialize the callbacks.


  // callbacks for tokens collection
  INS_RegReceiveToken = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction(
          "INS_RegReceiveToken",
          IRB.getVoidTy(), IRB.getInt8PtrTy(),
          IRB.getInt8Ty(), nullptr));

  // callback for task creation
 AdfCreateTask = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "AdfCreateTask", IRB.getVoidTy(), IRB.getInt8PtrTy(),  nullptr));


  INS_TaskBeginFunc = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "INS_TaskBeginFunc", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

  // register every executed function.
  INS_TaskBeginFunc2 = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "INS_TaskBeginFunc2", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

  INS_TaskFinishFunc = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("INS_TaskFinishFunc", IRB.getVoidTy(), nullptr));
  OrdTy = IRB.getInt32Ty();

  // functions to instrument floats and doubles
  llvm::LLVMContext &Ctx = M.getContext();
  INS_MemWriteFloat = M.getOrInsertFunction("INS_AdfMemWriteFloat",
	    llvm::Type::getVoidTy(Ctx), llvm::Type::getFloatPtrTy(Ctx),
      llvm::Type::getFloatTy(Ctx), llvm::Type::getInt8Ty(Ctx), nullptr);

  INS_MemWriteDouble = M.getOrInsertFunction("INS_AdfMemWriteDouble",
	    llvm::Type::getVoidTy(Ctx), llvm::Type::getDoublePtrTy(Ctx),
      llvm::Type::getDoubleTy(Ctx), llvm::Type::getInt8Ty(Ctx), nullptr);

  for (size_t i = 0; i < kNumberOfAccessSizes; ++i) {
    const unsigned ByteSize = 1U << i;
    const unsigned BitSize = ByteSize * 8;
    std::string ByteSizeStr = llvm::utostr(ByteSize);
    std::string BitSizeStr = llvm::utostr(BitSize);
    llvm::SmallString<32> ReadName("INS_AdfMemRead" + ByteSizeStr);
    TsanRead[i] = llvm::checkSanitizerInterfaceFunction(
        M.getOrInsertFunction(
            ReadName, IRB.getVoidTy(),
            IRB.getInt8PtrTy(), IRB.getInt64Ty(),
            IRB.getInt8PtrTy(), nullptr));

    llvm::SmallString<32> WriteName("INS_AdfMemWrite" + ByteSizeStr);
    INS_MemWrite[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        WriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(),
        IRB.getInt64Ty(), IRB.getInt8Ty(), nullptr));

    llvm::SmallString<64> UnalignedReadName("__tsan_unaligned_read" + ByteSizeStr);
    TsanUnalignedRead[i] =
        checkSanitizerInterfaceFunction(M.getOrInsertFunction(
            UnalignedReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    llvm::SmallString<64> UnalignedWriteName("__tsan_unaligned_write" + ByteSizeStr);
    TsanUnalignedWrite[i] =
        checkSanitizerInterfaceFunction(M.getOrInsertFunction(
            UnalignedWriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    llvm::Type *Ty = llvm::Type::getIntNTy(M.getContext(), BitSize);
    llvm::Type *PtrTy = Ty->getPointerTo();
    llvm::SmallString<32> AtomicLoadName("__tsan_atomic" + BitSizeStr + "_load");
    TsanAtomicLoad[i] = checkSanitizerInterfaceFunction(
        M.getOrInsertFunction(AtomicLoadName, Ty, PtrTy, OrdTy, nullptr));

    llvm::SmallString<32> AtomicStoreName("__tsan_atomic" + BitSizeStr + "_store");
    TsanAtomicStore[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        AtomicStoreName, IRB.getVoidTy(), PtrTy, Ty, OrdTy, nullptr));

    for (int op = llvm::AtomicRMWInst::FIRST_BINOP;
        op <= llvm::AtomicRMWInst::LAST_BINOP; ++op) {
      TsanAtomicRMW[op][i] = nullptr;
      const char *NamePart = nullptr;
      if (op == llvm::AtomicRMWInst::Xchg)
        NamePart = "_exchange";
      else if (op == llvm::AtomicRMWInst::Add)
        NamePart = "_fetch_add";
      else if (op == llvm::AtomicRMWInst::Sub)
        NamePart = "_fetch_sub";
      else if (op == llvm::AtomicRMWInst::And)
        NamePart = "_fetch_and";
      else if (op == llvm::AtomicRMWInst::Or)
        NamePart = "_fetch_or";
      else if (op == llvm::AtomicRMWInst::Xor)
        NamePart = "_fetch_xor";
      else if (op == llvm::AtomicRMWInst::Nand)
        NamePart = "_fetch_nand";
      else
        continue;
      llvm::SmallString<32> RMWName(
          "__tsan_atomic" + llvm::itostr(BitSize) + NamePart);
      TsanAtomicRMW[op][i] = checkSanitizerInterfaceFunction(
          M.getOrInsertFunction(RMWName, Ty, PtrTy, Ty, OrdTy, nullptr));
    }

    llvm::SmallString<32> AtomicCASName("__tsan_atomic" + BitSizeStr +
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

 bool DFinspec::doInitialization(llvm::Module &M) {

   // initialization of task IIR logger.
   IIRlog::Init( M.getName() );

   llvm::CallGraph myGraph(M);
   INS::InitializeSignatures();
   const llvm::DataLayout &DL = M.getDataLayout();
   IntptrTy = DL.getIntPtrType(M.getContext());
   return false;
 }

static bool isVtableAccess(llvm::Instruction *I) {
  if (llvm::MDNode *Tag = I->getMetadata(llvm::LLVMContext::MD_tbaa)) {
    return Tag->isTBAAVtableAccess();
  }
  return false;
}

bool DFinspec::addrPointsToConstantData(llvm::Value *Addr) {
  // If this is a GEP, just analyze its pointer operand.
  if (llvm::GetElementPtrInst *GEP =
      llvm::dyn_cast<llvm::GetElementPtrInst>(Addr)) {
    Addr = GEP->getPointerOperand();
  }

  if (llvm::GlobalVariable *GV =
      llvm::dyn_cast<llvm::GlobalVariable>(Addr)) {
    if (GV->isConstant()) {
      // Reads from constant globals can not race with any writes.
      //NumOmittedReadsFromConstantGlobals++;
      return true;
    }
  } else if (llvm::LoadInst *L = llvm::dyn_cast<llvm::LoadInst>(Addr)) {
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
void DFinspec::chooseInstructionsToInstrument(
    llvm::SmallVectorImpl<llvm::Instruction *> &Local,
    llvm::SmallVectorImpl<llvm::Instruction *> &All,
    const llvm::DataLayout &DL) {
  llvm::SmallSet<llvm::Value*, 8> WriteTargets;
  // Iterate from the end.
  for (llvm::SmallVectorImpl<llvm::Instruction*>::reverse_iterator It =
         Local.rbegin(),
       E = Local.rend(); It != E; ++It) {
    llvm::Instruction *I = *It;
    if (llvm::StoreInst *Store = llvm::dyn_cast<llvm::StoreInst>(I)) {
      WriteTargets.insert(Store->getPointerOperand());
    } else {
      llvm::LoadInst *Load = llvm::cast<llvm::LoadInst>(I);
      llvm::Value *Addr = Load->getPointerOperand();
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
    llvm::Value *Addr = llvm::isa<llvm::StoreInst>(*I)
        ? llvm::cast<llvm::StoreInst>(I)->getPointerOperand()
        : llvm::cast<llvm::LoadInst>(I)->getPointerOperand();
    if (llvm::isa<llvm::AllocaInst>(GetUnderlyingObject(Addr, DL)) &&
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

static bool isAtomic(llvm::Instruction *I) {
  if (llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(I))
    return LI->isAtomic() && LI->getSyncScopeID() != llvm::SyncScope::SingleThread;
  if (llvm::StoreInst *SI = llvm::dyn_cast<llvm::StoreInst>(I))
    return SI->isAtomic() && SI->getSyncScopeID() != llvm::SyncScope::SingleThread;
  if (llvm::isa<llvm::AtomicRMWInst>(I))
    return true;
  if (llvm::isa<llvm::AtomicCmpXchgInst>(I))
    return true;
  if (llvm::isa<llvm::FenceInst>(I))
    return true;
  return false;
}

 bool DFinspec::runOnFunction(llvm::Function &F) {
  // This is required to prevent instrumenting call to __tsan_init from within
  // the module constructor.
  if (INS::DontInstrument(F.getName()))
     return false;

  bool Res = false;
  bool HasCalls = false;
  bool isTaskBody = INS::isTaskBodyFunction( F.getName() );

  if (isTaskBody) {
    llvm::StringRef name = INS::demangleName(F.getName());
    auto idx = name.find('(');
    if (idx != llvm::StringRef::npos)
      name = name.substr(0, idx);

    llvm::IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    llvm::Value *taskName = IRB.CreateGlobalStringPtr(name, "taskName");
    IRB.CreateCall(INS_TaskBeginFunc,
        {IRB.CreatePointerCast(taskName, IRB.getInt8PtrTy())});

    Res = true;

    /* Log all its body statements for verification of nondererminism bugs */
    IIRlog::LogNewTask( name );
    for (auto &BB : F) {
      for (auto &Inst : BB) {
        unsigned lineNo = 0;
         if (auto Loc = Inst.getDebugLoc()) {
           lineNo = Loc->getLine();
         }

        IIRlog::LogNewIIRcode( lineNo, Inst);
      }
    }

  }

  // Register task name
  llvm::StringRef funcName = INS::demangleName(F.getName());
  llvm::IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
  funcNamePtr = IRB.CreateGlobalStringPtr(funcName, "functionName");

  initializeCallbacks(*F.getParent());
  llvm::SmallVector<llvm::Instruction*, 8> RetVec;
  llvm::SmallVector<llvm::Instruction*, 8> AllLoadsAndStores;
  llvm::SmallVector<llvm::Instruction*, 8> LocalLoadsAndStores;
  llvm::SmallVector<llvm::Instruction*, 8> AtomicAccesses;
  llvm::SmallVector<llvm::Instruction*, 8> MemIntrinCalls;

//   bool SanitizeFunction = F.hasFnAttribute(Attribute::SanitizeThread);
  const llvm::DataLayout &DL = F.getParent()->getDataLayout();

//   // Traverse all instructions, collect loads/stores/returns, check for calls.
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      //Inst.dump();
      if (isAtomic(&Inst))
        AtomicAccesses.push_back(&Inst);
      else if (llvm::isa<llvm::LoadInst>(Inst) || llvm::isa<llvm::StoreInst>(Inst))
        LocalLoadsAndStores.push_back(&Inst);
      else if (llvm::isa<llvm::ReturnInst>(Inst))
        RetVec.push_back(&Inst);
      else if (llvm::isa<llvm::CallInst>(Inst) || llvm::isa<llvm::InvokeInst>(Inst)) {
        //Inst.dump();
        // hassan
        if (llvm::isa<llvm::CallInst>(Inst)) {
          llvm::CallInst *M = llvm::dyn_cast<llvm::CallInst>(&Inst);
          llvm::Function *calledF = M->getCalledFunction();
          if (calledF) {
          //errs() << "  CALL: "
          //       << INS::demangleName(calledF->getName()) << "\n";
          }
          if (!calledF) {
            continue;
          }
        } else { // its invoke instruction
          llvm::InvokeInst *M = llvm::dyn_cast<llvm::InvokeInst>(&Inst);
          llvm::Function *calledF = M->getCalledFunction();
          if (!calledF) {
            continue;
          }
        }

        if (llvm::isa<llvm::MemIntrinsic>(Inst)) {
          MemIntrinCalls.push_back(&Inst);
          llvm::CallInst *M = llvm::dyn_cast<llvm::MemIntrinsic>(&Inst);
          llvm::Function *calledF = M->getCalledFunction();
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

      llvm::MemTransferInst *M = llvm::dyn_cast<llvm::MemTransferInst>(Inst);
      if (isTaskBody && M) {
        llvm::IRBuilder<> IRB(Inst);

        // for accessing tokens
        IRB.CreateCall(INS_RegReceiveToken,
          {//IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
	          IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
	          IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});
      }

      Res |= instrumentMemIntrinsic(Inst);
    }

  // Instrument function entry/exit points if there were
  // instrumented accesses. insert call at the beginning of the task
  if (isTaskBody && (Res || HasCalls)
      /*!HASSAN && ClInstrumentFuncEntryExit*/) {
    llvm::IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    llvm::Value *ReturnAddress = IRB.CreateCall(
        llvm::Intrinsic::getDeclaration(F.getParent(),
                                  llvm::Intrinsic::returnaddress),
                                  IRB.getInt32(0));
    //IRB.CreateCall(INS_TaskBeginFunc, ReturnAddress);
    if (isTaskBody) {
      // instrument returns of a task body to track when it finishes.
      for (auto RetInst : RetVec) {
        llvm::IRBuilder<> IRBRet(RetInst);
        IRBRet.CreateCall(INS_TaskFinishFunc, {});
      }
    }
    Res = true;
  }
   return Res;
 }

bool DFinspec::instrumentLoadOrStore(
    llvm::Instruction *I,
    const llvm::DataLayout &DL) {
  llvm::IRBuilder<> IRB(I);
  bool IsWrite = llvm::isa<llvm::StoreInst>(*I);
  llvm::Value *Addr = IsWrite
      ? llvm::cast<llvm::StoreInst>(I)->getPointerOperand()
      : llvm::cast<llvm::LoadInst>(I)->getPointerOperand();

  int Idx = getMemoryAccessFuncIndex(Addr, DL);
 if (Idx < 0)
    return false;
  // HASSAN
  //if (Idx >= 3) I->dump();

  if (IsWrite && isVtableAccess(I)) {
   // DEBUG(dbgs() << "  VPTR : " << *I << "\n");
    llvm::Value *StoredValue = llvm::cast<llvm::StoreInst>(I)->getValueOperand();
    // StoredValue may be a vector type if we are storing several vptrs at once.
    // In this case, just take the first element of the vector since this is
    // enough to find vptr races.
    if (llvm::isa<llvm::VectorType>(StoredValue->getType()))
      StoredValue = IRB.CreateExtractElement(
          StoredValue, llvm::ConstantInt::get(IRB.getInt32Ty(), 0));
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
  const unsigned Alignment =
      IsWrite
       ? llvm::cast<llvm::StoreInst>(I)->getAlignment()
       : llvm::cast<llvm::LoadInst>(I)->getAlignment();
  llvm::Type *OrigTy = llvm::cast<llvm::PointerType>(
      Addr->getType())->getElementType();
  const uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  llvm::Value *OnAccessFunc = nullptr;
  if (Alignment == 0 || Alignment >= 8 ||
      (Alignment % (TypeSize / 8)) == 0) {
    OnAccessFunc = IsWrite ? INS_MemWrite[Idx] : TsanRead[Idx];
  } else {
    OnAccessFunc = IsWrite
                    ? TsanUnalignedWrite[Idx]
                    : TsanUnalignedRead[Idx];
  }
  llvm::Constant* LineNo = getLineNumber(I);

  if (IsWrite) {
    llvm::Value *Val = llvm::cast<llvm::StoreInst>(I)->getValueOperand();
    llvm::Value* args[] = {Addr, Val, LineNo, funcNamePtr};
    if ( Val->getType()->isFloatTy() ) {
      IRB.CreateCall( INS_MemWriteFloat, args );
    } else if ( Val->getType()->isDoubleTy() ) {
      IRB.CreateCall( INS_MemWriteDouble, args );
    } else if ( Val->getType()->isIntegerTy() ) {
       IRB.CreateCall(OnAccessFunc,
           {   IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
		           //IRB.CreatePointerCast(Val, Val->getType())
		           Val, LineNo, funcNamePtr
            });
    } else {
      IRB.CreateCall(OnAccessFunc,
          {     IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()),
                //IRB.CreatePointerCast(Val, Val->getType())
                Val, LineNo, funcNamePtr
          });
    } // end IsWrite
  } else { // this is read action
    IRB.CreateCall(OnAccessFunc, {Addr, LineNo, funcNamePtr});
  }
  //if (IsWrite) NumInstrumentedWrites++;
  //else         NumInstrumentedReads++;
  return true;
}

static llvm::ConstantInt *createOrdering(
    llvm::IRBuilder<> *IRB,
    llvm::AtomicOrdering ord) {
  uint32_t v = 0;
  switch (ord) {
    case llvm::AtomicOrdering::NotAtomic:
        llvm_unreachable("unexpected atomic ordering!");
    case llvm::AtomicOrdering::Unordered:              // Fall-through.
    case llvm::AtomicOrdering::Monotonic:              v = 0; break;
    // case llvm::AtomicOrdering::Consume:  v = 1; break;  // Not specified yet.
    case llvm::AtomicOrdering::Acquire:                v = 2; break;
    case llvm::AtomicOrdering::Release:                v = 3; break;
    case llvm::AtomicOrdering::AcquireRelease:         v = 4; break;
    case llvm::AtomicOrdering::SequentiallyConsistent: v = 5; break;
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
bool DFinspec::instrumentMemIntrinsic(llvm::Instruction *I) {
  llvm::IRBuilder<> IRB(I);
  if (llvm::MemSetInst *M = llvm::dyn_cast<llvm::MemSetInst>(I)) {

    IRB.CreateCall(
        MemsetFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(1), IRB.getInt32Ty(), false),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    I->eraseFromParent();
  } else if (llvm::MemTransferInst *M =
      llvm::dyn_cast<llvm::MemTransferInst>(I)) {
    IRB.CreateCall(
        llvm::isa<llvm::MemCpyInst>(M) ? MemcpyFn : MemmoveFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    I->eraseFromParent();
  }
  return false;
}

// // Both llvm and DFinspec atomic operations are based on C++11/C1x
// // standards.  For background see C++11 standard.  A slightly older, publicly
// // available draft of the standard (not entirely up-to-date, but close enough
// // for casual browsing) is available here:
// // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3242.pdf
// // The following page contains more background information:
// // http://www.hpl.hp.com/personal/Hans_Boehm/c++mm/
//
bool DFinspec::instrumentAtomic(
    llvm::Instruction *I,
    const llvm::DataLayout &DL) {
  llvm::IRBuilder<> IRB(I);
  if (llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(I)) {
    llvm::Value *Addr = LI->getPointerOperand();
    int Idx = getMemoryAccessFuncIndex(Addr, DL);
//    if (Idx < 0)
      return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     llvm::Type *Ty = llvm::Type::getIntNTy(IRB.getContext(), BitSize);
//     llvm::Type *PtrTy = Ty->getPointerTo();
//     llvm::Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      createOrdering(&IRB, LI->getOrdering())};
//     CallInst *C = CallInst::Create(TsanAtomicLoad[Idx], Args);
//     ReplaceInstWithInst(I, C);
//
   } else if (llvm::StoreInst *SI = llvm::dyn_cast<llvm::StoreInst>(I)) {
//     llvm::Value *Addr = SI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     llvm::Type *Ty = llvm::Type::getIntNTy(IRB.getContext(), BitSize);
//     llvm::Type *PtrTy = Ty->getPointerTo();
//     llvm::Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(SI->getValueOperand(), Ty, false),
//                      createOrdering(&IRB, SI->getOrdering())};
//     CallInst *C = CallInst::Create(TsanAtomicStore[Idx], Args);c
//     ReplaceInstWithInst(I, C);
   } else if (llvm::AtomicRMWInst *RMWI =
       llvm::dyn_cast<llvm::AtomicRMWInst>(I)) {
//     llvm::Value *Addr = RMWI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     Function *F = TsanAtomicRMW[RMWI->getOperation()][Idx];
//     if (!F)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     llvm::Type *Ty = llvm::Type::getIntNTy(IRB.getContext(), BitSize);
//     llvm::Type *PtrTy = Ty->getPointerTo();
//     llvm::Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(RMWI->getValOperand(), Ty, false),
//                      createOrdering(&IRB, RMWI->getOrdering())};
//     CallInst *C = CallInst::Create(F, Args);
//     ReplaceInstWithInst(I, C);
   } else if (llvm::AtomicCmpXchgInst *CASI =
       llvm::dyn_cast<llvm::AtomicCmpXchgInst>(I)) {
//     llvm::Value *Addr = CASI->getPointerOperand();
//     int Idx = getMemoryAccessFuncIndex(Addr, DL);
//     if (Idx < 0)
//       return false;
//     const unsigned ByteSize = 1U << Idx;
//     const unsigned BitSize = ByteSize * 8;
//     llvm::Type *Ty = llvm::Type::getIntNTy(IRB.getContext(), BitSize);
//     llvm::Type *PtrTy = Ty->getPointerTo();
//     llvm::Value *Args[] = {IRB.CreatePointerCast(Addr, PtrTy),
//                      IRB.CreateIntCast(CASI->getCompareOperand(), Ty, false),
//                      IRB.CreateIntCast(CASI->getNewValOperand(), Ty, false),
//                      createOrdering(&IRB, CASI->getSuccessOrdering()),
//                      createOrdering(&IRB, CASI->getFailureOrdering())};
//     CallInst *C = IRB.CreateCall(TsanAtomicCAS[Idx], Args);
//     llvm::Value *Success = IRB.CreateICmpEQ(C, CASI->getCompareOperand());
//
//     llvm::Value *Res = IRB.CreateInsertValue(UndefValue::get(CASI->getType()), C, 0);
//     Res = IRB.CreateInsertValue(Res, Success, 1);
//
//     I->replaceAllUsesWith(Res);
//     I->eraseFromParent();
   } else if (llvm::FenceInst *FI = llvm::dyn_cast<llvm::FenceInst>(I)) {
//     llvm::Value *Args[] = {createOrdering(&IRB, FI->getOrdering())};
//     Function *F = FI->getSynchScope() == SingleThread ?
//         TsanAtomicSignalFence : TsanAtomicThreadFence;
//     CallInst *C = CallInst::Create(F, Args);
//     ReplaceInstWithInst(I, C);
   }
//   return true;
}

int DFinspec::getMemoryAccessFuncIndex(
    llvm::Value *Addr,
    const llvm::DataLayout &DL) {
  llvm::Type *OrigPtrTy = Addr->getType();
  llvm::Type *OrigTy = llvm::cast<llvm::PointerType>(OrigPtrTy)->getElementType();
  assert(OrigTy->isSized());
  uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  if (TypeSize != 8  && TypeSize != 16 &&
      TypeSize != 32 && TypeSize != 64 && TypeSize != 128) {
    //NumAccessesWithBadSize++;
    // Ignore all unusual sizes.
    return -1;
  }
  size_t Idx = llvm::countTrailingZeros(TypeSize / 8);
  assert(Idx < kNumberOfAccessSizes);
  return Idx;
}
