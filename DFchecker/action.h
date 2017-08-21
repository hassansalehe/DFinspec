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

// this header defines the Action class. Action represetions a single
// action of a dataflow program. Example actions are the write, read,
// token receive, token send, task start, and task end

#ifndef _ACTION_HPP_
#define _ACTION_HPP_

// includes and definitions
#include "defs.h"

using namespace std;

class Action {
 public:
  INTEGER tid;      // task id of writter
  ADDRESS addr;     // destination address
  VALUE value;      // value written
  VALUE lineNo;     // source-line number
  string funcName;  // source-function name
  Action(INTEGER tskId, VALUE val, VALUE ln, string fname):
    tid(tskId), value(val), lineNo(ln), funcName(fname) {}

  Action(INTEGER tskId, ADDRESS adr, VALUE val, VALUE ln, string fname):
    tid(tskId),
    addr(adr),
    value(val),
    lineNo(ln),
    funcName(fname) {}

  Action() {}
}; // end Action

#endif // end action.h
