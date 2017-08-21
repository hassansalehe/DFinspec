/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    Copyright (c) 2015 - 2017 Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Defines a class to hold task identification info

#ifndef LLVM_TASK_INFO_HPP
#define LLVM_TASK_INFO_HPP

#include "defs.h"

using namespace std;

typedef struct TaskInfo {
  uint threadID = 0;
  uint taskID = 0;
  bool active = false;

  // improve performance by buffering actions and write only once.
  ostringstream actionBuffer;
} TaskInfo;

#endif // TaskInfo.h
