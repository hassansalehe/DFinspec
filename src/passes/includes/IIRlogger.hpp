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

// Include for the instrumentation passes.

#ifndef _PASSES_INCLUDES_IIRLOGGER_HPP_
#define _PASSES_INCLUDES_IIRLOGGER_HPP_

#include "Libs.hpp" // all LLVM includes stored there

namespace IIRlog {

  // the log out stream
  std::ofstream logFile;

  void Init( llvm::StringRef cppName ) {
    llvm::errs() <<  "File name will be " << cppName << "\n";
    logFile.open("" + cppName.str() + ".iir",
                 std::ofstream::out | std::ofstream::trunc);
    if ( !logFile.is_open() ) {
      llvm::errs() << "FILE NO OPEN \n";
    }
  }

  void LogNewTask( llvm::StringRef taskName ) {
     llvm::errs() << taskName.str() << "\n";
     logFile << taskName.str() << std::endl << std::flush;
  }

  void LogNewIIRcode(int lineNo, llvm::Instruction &IIRcode ) {
    //errs() << lineNo << ": " << IIRcode.str() << "\n";
    std::string tempBuf;
    llvm::raw_string_ostream rso(tempBuf);
    IIRcode.print(rso);
    logFile << lineNo << ": " << tempBuf << std::endl;
  }

} // end namespace

#endif
