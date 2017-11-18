/////////////////////////////////////////////////////////////////
//  DFinspec: a lightweight non-determinism checking
//          tool for shared memory DataFlow applications
//
//    Copyright (c) 2015 - 2017 Hassan Salehe Matar
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

#ifndef SRC_PASSES_INCLUDES_ADFSchedulerSignatures_H
#define SRC_PASSES_INCLUDES_ADFSchedulerSignatures_H

#include <vector>
#include <string>

namespace dfinspec {

/// This class which holds ADF runtime signatures necessary to detect
/// important runtime points for starting and terminating nondeterminism
/// detection runtime.
class ADFSchedulerSignatures {
  private:
    int curPos;
    std::vector<std::string> signatures;

  public:
    // Initalize the signatures
    ADFSchedulerSignatures() {
      signatures = {
        "TASK:::operator()(token_s*) const",
        "TERM:TerminateRuntime",
        "INIT:InitRuntime",
        "PASS:PassToken"
      };
      curPos = 0;
    }

    // Set signature line into "holder" argument.
    // Return true if successful, false otherwise.
    bool getline(std::string & holder) {
      if(curPos >= signatures.size())
        return false;
      holder = signatures[curPos++];
      return true;
    }

    bool good() {
      return curPos < signatures.size();
    }

    bool is_open() {
      return good();
    }

    bool close() {
      curPos = 0;
      return true;
    }
}; // end class
} // end namespace

#endif
