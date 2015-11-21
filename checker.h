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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <vector>
#include <set>
#include <map>

#include "defs.h"

using namespace std;

// a bag to hold the tasks that happened-before
typedef struct SerialBag {
  int outBufferCount;
  INTSET HB;

  SerialBag(): outBufferCount(0){}
} SerialBag;

// for constructing happans-before between tasks
typedef struct Task {
  string name;     // name of the task
  INTSET inEdges;  // incoming data streams
  INTSET outEdges; // outgoing data streams
} Task;

// represents last write,
// used for non-determinism checking
typedef struct Write {

  INTEGER wrtTaskId;
  VALUE value;
  VALUE lineNo;
  Write(INTEGER tskId, VALUE val, VALUE ln):wrtTaskId(tskId), value(val), lineNo(ln) {}
} Write;


// This struct keeps the line information of the
// address with determinism conflict
typedef struct Conflict {
  ADDRESS addr;
  VALUE lineNo1;
  VALUE lineNo2;
  Conflict(ADDRESS _addr, VALUE _ln1, VALUE _ln2) {
    addr = _addr;
    lineNo1 = _ln1;
    lineNo2 = _ln2;
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
  VOID saveWrite(INTEGER taskId, ADDRESS addr, VALUE value, VALUE lineNo);
  VOID processLogLines(string & line);
  VOID reportConflicts();
  VOID printHBGraph();
  VOID testing();
  ~Checker();

  private:
    VOID checkDetOnPreviousTasks(INTEGER taskId, ADDRESS addr, VALUE value, VALUE lineNo);
    unordered_map <INTEGER, SerialBagPtr> serial_bags; // hold bags of tasks
    unordered_map<INTEGER, Task> graph;  // in and out edges
    unordered_map<ADDRESS, vector<Write>> writes; // for writes
    map<pair<INTEGER, INTEGER>, Report> conflictTable;
};


#endif // end checker.h
