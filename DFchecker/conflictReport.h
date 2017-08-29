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

//
//

#ifndef _CONFLICT_HPP_
#define _CONFLICT_HPP_

// includes and definitions
#include "defs.h"

#include "action.h" // defines Action class

using namespace std;

// This struct keeps the line information of the
// address with determinism conflict
class Conflict {
 public:
  ADDRESS addr;
  VALUE lineNo1;
  VALUE lineNo2;

  string taskName1;
  string taskName2;

  bool isWrite1;
  bool isWrite2;
  Conflict(ADDRESS _addr, VALUE ln1, string tname1, VALUE ln2, string tname2, bool actype1, bool actype2):
    addr(_addr), lineNo1(ln1), lineNo2(ln2), taskName1(tname1), taskName2(tname2),
    isWrite1(actype1), isWrite2(actype2){}

  Conflict(const Action& curWrite, const Action& prevWrite) {
    lineNo1 = prevWrite.lineNo;
    taskName1 = prevWrite.funcName;
    isWrite1 = prevWrite.isWrite;

    addr = curWrite.addr;
    lineNo2 = curWrite.lineNo;
    taskName2 = curWrite.funcName;
    isWrite2 = curWrite.isWrite;
  }

  bool operator<(const Conflict &RHS) const {
    return addr < RHS.addr;
  }
}; // end Conflict


// For storing names of conflicting tasks
// and the addresses they conflict at
class Report {
 public:
  string task1Name;
  string  task2Name;
  set<Conflict> addresses;
}; // end Report

#endif // end conflictReport.h
