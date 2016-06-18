/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015, 2016 - Hassan Salehe Matar & MSRC at Koc University
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

  string funcName1;
  string funcName2;
  Conflict(ADDRESS _addr, VALUE ln1, string fname1, VALUE ln2, string fname2):
    addr(_addr), lineNo1(ln1), lineNo2(ln2), funcName1(fname1), funcName2(fname2){}

  Conflict(const Action& curWrite, const Action& prevWrite) {
    lineNo1 = prevWrite.lineNo;
    funcName1 = prevWrite.funcName;

    addr = curWrite.addr;
    lineNo2 = curWrite.lineNo;
    funcName2 = curWrite.funcName;
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
