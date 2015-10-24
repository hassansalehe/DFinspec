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
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Pass.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>
#include <string>
#include <cxxabi.h>

using namespace llvm;
using namespace std;

namespace INS {
   StringRef blackList[] = 
   {
     "Settings::Settings(char*)",
     "Settings::~Settings()",
     "Settings::PrintStringSettings()",
     "genmat(float**)",
     "Configurator::GetContent(float***)",
     "Configurator::Close()",
     "Solver::Solver(Configurator*)",
     "Configurator::WriteSettings()",
     "Solver::InitSolver()",
     "Solver::~Solver()",
     "Solver::allocate_clean_block()"
   };

   StringRef whiteList[] =
   {
     "adf_create_task" 
   };

   bool DontInstrument(StringRef &name) const {
     //errs() << name << "\n";
     int status = -1;
     string name_(name.str());
     char* d =abi::__cxa_demangle(name_.c_str(), nullptr, nullptr, &status);
      if(! status) {
       StringRef ab(d);
       string dname(d);
       if(dname.find("fwd(") != string::npos) {
         errs() << ab << "\n";
         return false;
       }
     }
     return true;
   }
}
