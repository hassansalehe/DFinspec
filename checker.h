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

using namespace std;

typedef        void        VOID;
typedef      void *        ADDRESS;
typedef    ofstream        FILEPTR;
typedef    long int        INTEGER;
typedef    long int        VALUE;
typedef const char*        STRING;
typedef vector<int>        INTVECTOR;
typedef unordered_set<int> INTSET;

// a bag to hold the tasks that happened-before
typedef struct Sbag {
  int outStr;
  string name;
  INTSET HB;

  Sbag(): outStr(0){}
} Sbag;

// for constructing happans-before between tasks
typedef struct Task {
  INTSET inEdges;
  INTSET outEdges;
} Task;

// represents last write, 
// used for non-determinism checking
typedef struct Write {

  INTEGER wrtTaskId;
  VALUE value;
  Write(INTEGER tskId, VALUE val):wrtTaskId(tskId), value(val) {}
} Write;

typedef Sbag * PSbag;

class Checker {
  public:
  VOID addTaskNode(string & logLine);
  VOID saveWrite(INTEGER taskId, ADDRESS addr, VALUE value);
  VOID processLogLines(string & line);
  VOID testing();

  private:
    unordered_map <INTEGER, PSbag> p_bags; // hold bags of tasks
    unordered_map<INTEGER, Task> graph;  // in and out edges
    unordered_map<ADDRESS, vector<Write>> writes; // for writes
};


#endif // end checker.h
