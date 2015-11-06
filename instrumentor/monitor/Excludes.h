#ifndef EXCLUDES_H_P_P
#define EXCLUDES_H_P_P

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

   bool DontInstrument(StringRef name) {
     int status = -1;
     string name_(name.str());
     char* d =abi::__cxa_demangle(name_.c_str(), nullptr, nullptr, &status);
      if(! status) {
       StringRef ab(d);
       string dname(d);
       //errs() << ab << "\n";
       if(dname.find("_call(") != string::npos||dname.find("_create(") != string::npos) {
         return false;
       }
     }
      return false;
     //return true;
   }
   
   StringRef demangleName(StringRef name)
   {
      int status = -1;
      string name_(name.str());
      char* d =abi::__cxa_demangle(name_.c_str(), nullptr, nullptr, &status);
      if(! status) {
        StringRef ab(d);
        return ab;
      }
      return name;
   }

   string Demangle(StringRef name) 
   {
      int status = -1;
      char* d =abi::__cxa_demangle(name.str().c_str(), nullptr, nullptr, &status);
      if(! status) {
        string dname(d);
        return dname;
      }
      return string("");
   }

   bool isTaskBodyFunction(StringRef name) {
    
     int status = -1;
     char* d =abi::__cxa_demangle(name.str().c_str(), nullptr, nullptr, &status);
     if(! status) {
       string dname(d);
       //if(dname.find("std::function<void (token_s*)>::function") != string::npos 
       //            || dname.find("create_task") != string::npos || dname.find("luTask") != string::npos)
       if(dname.find("::operator()(token_s*) const") != string::npos)
         return true;
     }
     return false;
   }

   bool isLLVMCall(StringRef name) {
      if(name.find("llvm") != StringRef::npos)
        return true;
      return false;
     /*int status = -1;
     char* d =abi::__cxa_demangle(name.str().c_str(), nullptr, nullptr, &status);
     if(! status) {
       string dname(d);
       errs() << "What: " << name << "\n";
       errs() << "But how: " << dname << "\n";
       if(dname.find("llvm") != string::npos) {
         errs() << "Da eeeH: " <<  name << "\n";
         return true;  
       }
     }
     else
       errs() << "Failed: " << name << "\n";
     return false;*/
   }

   bool isPassTokenFunc(StringRef name) {

     return name.find("adf_pass_token") !=StringRef::npos;
   }

   bool isTaskCreationFunc(StringRef name) {

     return name.find("adf_create_task") != StringRef::npos;
   }
}

#endif
