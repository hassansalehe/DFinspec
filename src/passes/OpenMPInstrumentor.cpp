//////////////////////////////////////////////////////////////
//
// OpenMPInstrumentor.cpp
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

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

#include "OpenMPUtil.h"

namespace {

// Function pass to instrument OpenMP .cpp programs
// For detecting output nondeterminism.
struct OpenMPInstrumentorPass : public llvm::FunctionPass {

  static char ID;
  OpenMPInstrumentorPass() : FunctionPass(ID) {}

  llvm::StringRef getPassName() const override {
    return "OpenMPInstrumentorPass";
  }

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  bool runOnFunction(llvm::Function &F) override {
    // print function name
    llvm::errs().write_escaped(F.getName()) << '\n';

    // save the function body
    dfinspec::logFunction(F);
    return false;
  }

}; // end of OpenMPInstrumentorPass
} // end of namespace

char OpenMPInstrumentorPass::ID = 0;

static void registerOpenMPInstrumentorPass(
  const llvm::PassManagerBuilder &,
  llvm::legacy::PassManagerBase &PM) { PM.add(new OpenMPInstrumentorPass()); }

static llvm::RegisterStandardPasses regPass(
   llvm::PassManagerBuilder::EP_EarlyAsPossible,
   registerOpenMPInstrumentorPass);

