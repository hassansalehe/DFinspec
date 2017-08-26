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
  bool isWrite;     // true if this action is "write"

  Action(INTEGER tskId, VALUE val, VALUE ln, string fname):
    tid(tskId), value(val), lineNo(ln), funcName(fname) {}

  Action(INTEGER tskId, ADDRESS adr, VALUE val, VALUE ln, string fname):
    tid(tskId),
    addr(adr),
    value(val),
    lineNo(ln),
    funcName(fname) {}

  Action(INTEGER tskId, address adr, lint val, int ln, address fname ):
    tid(tskId),
    addr(adr),
    value(val),
    lineNo(ln) {
      if(!fname)
        funcName = "Unknown";
      else
        funcName = string((char*)fname);
    }

  Action() {}

  /**
   * Generates string representation of the action and stores in "buff"
   */
  void printAction(ostringstream & buff) {
    string type = " RD ";

    if ( isWrite ) type = " WR ";

    buff << tid << type <<  addr << " " << value << " " << lineNo << " " << funcName << endl;

  }

}; // end Action

#endif // end action.h
