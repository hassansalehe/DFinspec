//////////////////////////////////////////////////////////////
//
// Logger.h
//
// Copyright (c) 2017, Hassan Salehe Matar
// All rights reserved.
//
// This file is part of DFinspec. For details, see
// https://github.com/hassansalehe/DFinspec. Please also
// see the LICENSE file for additional BSD notice
//
// Redistribution and use in source and binary forms, with
// or without modification, are permitted provided that
// the following conditions are met:
//
// * Redistributions of source code must retain the above
//   copyright notice, this list of conditions and the
//   following disclaimer.
//
// * Redistributions in binary form must reproduce the
//   above copyright notice, this list of conditions and
//   the following disclaimer in the documentation and/or
//   other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names
//   of its contributors may be used to endorse or promote
//   products derived from this software without specific
//   prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////

#ifndef SRC_INCLUDE_LOGGER_H
#define SRC_INCLUDE_LOGGER_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/DebugInfo.h"

#include <string>
#include <unordered_map>
#include <fstream>

using line_code = std::unordered_map<int, std::string>;

namespace dfinspec {

  // color strings
  const std::string magenta_start = "\x1b[36m";
  const std::string magenta_end = "\x1b[0m";

  // stores the source code which is later printed
  // next to the instruction
  std::unordered_map<std::string, line_code> source_code;

  // Returns corresponding source code expression statement
  // for a given instruction debug info.
  std::string getExprSttmt(llvm::DILocation * Loc) {
    unsigned Line = Loc->getLine();
    llvm::StringRef File = Loc->getFilename();
    llvm::StringRef Dir = Loc->getDirectory();

    auto & sttmts = source_code[File];
    if(sttmts.find(Line) == sttmts.end()) { // read the file
      int id = 1;
      std::ifstream source_file;
      source_file.open(File/*, std::ifstream::in*/);

      // check if file does not exist
      if(!source_file.good()) {
        llvm::errs() << "                                         \n";
        llvm::errs() << " ERROR!                                  \n";
        llvm::errs() << "Error opening source file " <<  File << "\n";
        llvm::errs() << "                                         \n";
        exit ( EXIT_FAILURE );
      }

      if(source_file.is_open()) {

        for (std::string stmt; std::getline(source_file, stmt); )
          sttmts[id++] = stmt;

        source_file.close(); // close file
      }
    }

    return sttmts[Line];
  }

  // Outputs the IR of the function to stdout.
  // Included information is source line number
  // for each instruction
  void logFunction(llvm::Function & F) {

    int prevLineNo = 0;

    for (auto &BB : F) {
      for (auto &Inst : BB) {
        unsigned lineNo = 0;
        auto Loc = Inst.getDebugLoc();
        if(Loc)
          lineNo = Loc->getLine();

        std::string tempBuf;
        llvm::raw_string_ostream rso(tempBuf);
        Inst.print(rso);

        // print original source expression sttmt
        if(prevLineNo != lineNo) {
           std::string stmt = dfinspec::getExprSttmt(Loc);
           llvm::errs() << magenta_start << lineNo << ": " << stmt << magenta_end << "\n";
           prevLineNo = lineNo;
        }

        if(lineNo)
          llvm::errs() << " " << tempBuf << "\n";
        else
          llvm::errs() << lineNo << ": " << tempBuf << "\n";
      }
    }
  }

} // end of namespace

#endif // end define
