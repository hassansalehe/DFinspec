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

#ifndef _DETECTOR_CONFLICTREPORT_HPP_
#define _DETECTOR_CONFLICTREPORT_HPP_

// includes and definitions
#include "defs.hpp"
#include "action.hpp" // defines Action class

// This struct keeps the line information of the
// address with determinism conflict
class Conflict {
 public:
  ADDRESS  addr;
  Action   action1;
  Action   action2;

  Conflict(const Action& curMemAction, const Action& prevMemAction) {
    action1 = curMemAction;
    action2 = prevMemAction;
    addr    = curMemAction.addr;
  }

  bool operator<(const Conflict &RHS) const {
    return addr < RHS.addr || action1.taskId < action2.taskId;
  }
}; // end Conflict


// For storing names of conflicting tasks
// and the addresses they conflict at
class Report {
 public:
  std::string         task1Name;
  std::string         task2Name;
  std::set<Conflict>  buggyAccesses;
}; // end Report

#endif // end conflictReport.h
