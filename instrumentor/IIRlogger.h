/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Include for the instrumentation passes.

#ifndef IIRLOGGER_H_P_P
#define IIRLOGGER_H_P_P

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
#include <fstream>
#include <memory>
#include <string>
#include <cxxabi.h>

using namespace llvm;
using namespace std;

namespace IIRlog {

  // the log out stream
  ofstream logFile;

  void Init( StringRef cppName ) {
    errs() <<  "File name will be " << cppName << "\n";
    logFile.open("" + cppName.str() + ".iir",  ofstream::out | ofstream::trunc);
    if( !logFile.is_open() )
      errs() << "FILE NO OPEN \n";
  }

  void LogNewTask( StringRef taskName ) {

     errs() << taskName.str() << "\n";
     logFile << taskName.str() << endl << flush;
  }

  void LogNewIIRcode(int lineNo, Instruction& IIRcode ) {
    //errs() << lineNo << ": " << IIRcode.str() << "\n";
    string tempBuf;
    raw_string_ostream rso(tempBuf);
    IIRcode.print(rso);
    logFile << lineNo << ": " << tempBuf << endl;
  }

}

#endif
