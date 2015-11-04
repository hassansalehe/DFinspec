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
   static char ID;  // Instrumentation Pass identification, replacement for typeid.
 
 private:
   void initializeCallbacks(Module &M);
   bool instrumentLoadOrStore(Instruction *I, const DataLayout &DL);
   bool instrumentAtomic(Instruction *I, const DataLayout &DL);
   bool instrumentMemIntrinsic(Instruction *I);
   void chooseInstructionsToInstrument(SmallVectorImpl<Instruction *> &Local,
                                      SmallVectorImpl<Instruction *> &All,
                                      const DataLayout &DL);
   bool addrPointsToConstantData(Value *Addr);
   int getMemoryAccessFuncIndex(Value *Addr, const DataLayout &DL);
   void getInTokens(Function & F);
 
   Type *IntptrTy;
   IntegerType *OrdTy;
//   // Callbacks to run-time library are computed in doInitialization.
     Function *taskStartFunc;
     Function *taskFinishFunc;

     // callbacks for registering tokens
     Function *regInToken;
     Function *regOutToken;

//   // Accesses sizes are powers of two: 1, 2, 4, 8, 16.
   static const size_t kNumberOfAccessSizes = 5;
  Function *TsanRead[kNumberOfAccessSizes];
  Function *TsanWrite[kNumberOfAccessSizes];
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
regInToken = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "regInToken", IRB.getVoidTy(), IRB.getInt8PtrTy(),  IRB.getInt8Ty(), nullptr));

regOutToken = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "regOutToken", IRB.getVoidTy(), IRB.getInt8PtrTy(),  IRB.getInt8PtrTy(), IntptrTy, nullptr));


  taskStartFunc = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      "taskStartFunc", IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));
  taskFinishFunc = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("taskFinishFunc", IRB.getVoidTy(), nullptr));
  OrdTy = IRB.getInt32Ty();
  for (size_t i = 0; i < kNumberOfAccessSizes; ++i) {
    const unsigned ByteSize = 1U << i;
    const unsigned BitSize = ByteSize * 8;
    std::string ByteSizeStr = utostr(ByteSize);
    std::string BitSizeStr = utostr(BitSize);
    SmallString<32> ReadName("AdfMemRead" + ByteSizeStr);
    TsanRead[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        ReadName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

    SmallString<32> WriteName("AdfMemWrite" + ByteSizeStr);
    TsanWrite[i] = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
        WriteName, IRB.getVoidTy(), IRB.getInt8PtrTy(), nullptr));

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
   //for (Function &F : M)
   //  errs() << INS::demangleName(F.getName()) << "\n";
   CallGraph myGraph(M);
   //myGraph.dump();
   errs() << "Hi my student\n";

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

 void AdfSanitizer::getInTokens(Function & F) {

 }

 bool AdfSanitizer::runOnFunction(Function &F) {
  // This is required to prevent instrumenting call to __tsan_init from within
  // the module constructor.
  if (INS::DontInstrument(F.getName()))
     return false;

  bool isTaskBody = INS::isTaskBodyFunction( F.getName() );
  
  if(isTaskBody) { 
     errs() << "BODY: "<< INS::demangleName(F.getName()) << "\n";
     errs() << "PARENT: " << F.getParent()->getName() << "\n";
     getInTokens(F);
  }else
     errs() << "START: "<< INS::demangleName(F.getName()) << "\n";
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
  bool Res = false;
  bool HasCalls = false;
//   bool SanitizeFunction = F.hasFnAttribute(Attribute::SanitizeThread);
  const DataLayout &DL = F.getParent()->getDataLayout();

//   // Traverse all instructions, collect loads/stores/returns, check for calls.
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      Inst.dump();
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
          errs() << "  CALL: " << INS::demangleName(calledF->getName()) << "\n";
          if(!calledF)
            continue;
          if(isTaskBody && INS::isPassTokenFunc(calledF->getName()))
          {
            IRBuilder<> IRB(&Inst);
            IRB.CreateCall(regOutToken,
              {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
               IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
               IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

          } 
          /*
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
          if(calledF)
          errs() << "  INVOKE: " << INS::demangleName(calledF->getName()) << "\n";
          if(!calledF)
            continue;
          if(isTaskBody && INS::isPassTokenFunc(calledF->getName()))
          {
            IRBuilder<> IRB(&Inst);
            IRB.CreateCall(regOutToken,
              {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
               IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
               IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

          }
        }

        if (isa<MemIntrinsic>(Inst)) {
          MemIntrinCalls.push_back(&Inst);
          CallInst *M = dyn_cast<MemIntrinsic>(&Inst);
          Function *calledF = M->getCalledFunction();
          if(calledF)
          errs() << "  MEM CALL: " << INS::demangleName(calledF->getName()) << "\n";

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
      Res |= instrumentMemIntrinsic(Inst);
    }

  // Instrument function entry/exit points if there were instrumented accesses.
  // insert call at the start of the task
  if (isTaskBody && (Res || HasCalls) /*!HASSAN && ClInstrumentFuncEntryExit*/) {
    IRBuilder<> IRB(F.getEntryBlock().getFirstNonPHI());
    Value *ReturnAddress = IRB.CreateCall(Intrinsic::getDeclaration(F.getParent(), Intrinsic::returnaddress),
                                         IRB.getInt32(0));
    IRB.CreateCall(taskStartFunc, ReturnAddress);
    if(isTaskBody) {  // instrument returns of a task body to track when it finishes.
      for (auto RetInst : RetVec) {
        IRBuilder<> IRBRet(RetInst);
        IRBRet.CreateCall(taskFinishFunc, {});
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
  if (Idx >= 3) I->dump();
  
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
  const unsigned Alignment = IsWrite
      ? cast<StoreInst>(I)->getAlignment()
      : cast<LoadInst>(I)->getAlignment();
  Type *OrigTy = cast<PointerType>(Addr->getType())->getElementType();
  const uint32_t TypeSize = DL.getTypeStoreSizeInBits(OrigTy);
  Value *OnAccessFunc = nullptr;
  if (Alignment == 0 || Alignment >= 8 || (Alignment % (TypeSize / 8)) == 0)
    OnAccessFunc = IsWrite ? TsanWrite[Idx] : TsanRead[Idx];
  else
    OnAccessFunc = IsWrite ? TsanUnalignedWrite[Idx] : TsanUnalignedRead[Idx];
  IRB.CreateCall(OnAccessFunc, IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()));
  //if (IsWrite) NumInstrumentedWrites++;
  //else         NumInstrumentedReads++;
  return true;
}

static ConstantInt *createOrdering(IRBuilder<> *IRB, AtomicOrdering ord) {
  uint32_t v = 0;
  switch (ord) {
    case NotAtomic: llvm_unreachable("unexpected atomic ordering!");
    case Unordered:              // Fall-through.
    case Monotonic:              v = 0; break;
    // case Consume:                v = 1; break;  // Not specified yet.
    case Acquire:                v = 2; break;
    case Release:                v = 3; break;
    case AcquireRelease:         v = 4; break;
    case SequentiallyConsistent: v = 5; break;
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
    errs() << "memset HHHHHHHHHHHHHHHHHOi";
    I->dump();
    IRB.CreateCall(
        MemsetFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(1), IRB.getInt32Ty(), false),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});
    // for accessing tokens 
   // IRB.CreateCall(regInToken,
   //     {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
   //      IRB.CreateIntCast(M->getArgOperand(1), IRB.getInt32Ty(), false),
   //      IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    I->eraseFromParent();
  } else if (MemTransferInst *M = dyn_cast<MemTransferInst>(I)) {
    errs() << "memTransfer HHHHHHHHHHHHHHHHH";
    I->dump();
    IRB.CreateCall(
        isa<MemCpyInst>(M) ? MemcpyFn : MemmoveFn,
        {IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
         IRB.CreatePointerCast(M->getArgOperand(1), IRB.getInt8PtrTy()),
         IRB.CreateIntCast(M->getArgOperand(2), IntptrTy, false)});

    // for accessing tokens 
    IRB.CreateCall(regInToken,
        {//IRB.CreatePointerCast(M->getArgOperand(0), IRB.getInt8PtrTy()),
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
