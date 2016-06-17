/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// this files redifines data types for redability and WORE (Write-Once-Reuse-Everywhere)
// Defines the ADF Checker class

#ifndef _CHECKER_HPP_
#define _CHECKER_HPP_

// includes and definitions
#include "defs.h"

using namespace std;

// a bag to hold the tasks that happened-before
typedef struct SerialBag {
  int outBufferCount;
  UNORD_INTSET HB;  // unordered int set

  SerialBag(): outBufferCount(0){}
} SerialBag;

// for constructing happans-before between tasks
typedef struct Task {
  string name;     // name of the task
  UNORD_INTSET inEdges;  // incoming data streams
  UNORD_INTSET outEdges; // outgoing data streams
} Task;

// represents last write,
// used for non-determinism checking
typedef struct Write {

  INTEGER wrtTaskId;
  VALUE value;
  VALUE lineNo;
  string funcName;
  Write(INTEGER tskId, VALUE val, VALUE ln, string fname):
    wrtTaskId(tskId),
    value(val),
    lineNo(ln),
    funcName(fname) {}
} Write;


// This struct keeps the line information of the
// address with determinism conflict
typedef struct Conflict {
  ADDRESS addr;
  VALUE lineNo1;
  VALUE lineNo2;

  string funcName1;
  string funcName2;
  Conflict(ADDRESS _addr, VALUE _ln1, string fname1, VALUE _ln2, string fname2) {
    addr = _addr;
    lineNo1 = _ln1;
    lineNo2 = _ln2;

    funcName1 = fname1;
    funcName2 = fname2;
  }
  bool operator<(const Conflict &RHS) const {
    return addr < RHS.addr;
  }
} Conflict;

typedef struct Report {
  string task1Name;

  string  task2Name;
  set<Conflict> addresses;
} Report;

typedef SerialBag * SerialBagPtr;

class Checker {
  public:
  VOID addTaskNode(string & logLine);
  VOID saveWrite(INTEGER taskId, ADDRESS addr, VALUE value, VALUE lineNo, string& funcName);
  VOID processLogLines(string & line);

  // a pair of conflicting task body with a set of line numbers
  CONFLICT_PAIRS & getConflictingPairs();

  VOID reportConflicts();
  VOID printHBGraph();
  VOID printHBGraphJS();  // for printing dependency graph in JS format
  VOID testing();
  ~Checker();

  private:
    VOID saveNondeterminismReport(INTEGER taskId, ADDRESS addr, VALUE lineNo, string& funcName, Write* write);
    VOID checkDetOnPreviousTasks(INTEGER taskId, ADDRESS addr, VALUE value, VALUE lineNo, string & funcName);
    unordered_map <INTEGER, SerialBagPtr> serial_bags; // hold bags of tasks
    unordered_map<INTEGER, Task> graph;  // in and out edges
    unordered_map<ADDRESS, vector<Write>> writes; // for writes
    map<pair<INTEGER, INTEGER>, Report> conflictTable;
    CONFLICT_PAIRS conflictTasksAndLines;
};


#endif // end checker.h
