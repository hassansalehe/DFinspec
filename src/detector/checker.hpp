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

// this files redifines data types for redability and
// WORE (Write-Once-Reuse-Everywhere)
// Defines the ADF Checker class

#ifndef _DETECTOR_CHECKER_HPP_
#define _DETECTOR_CHECKER_HPP_

// includes and definitions
#include "defs.hpp"
#include "action.hpp"         // defines Action class
#include "conflictReport.hpp" // defines Conflict and Report structs
#include "sigManager.hpp"     // for managing function names
#include "MemoryActions.hpp"
#include "validator.hpp"
#include <list>

// a bag to hold the tasks that happened-before
typedef struct SerialBag {
  int             outBufferCount;
  UNORD_INTSET    HB;

  SerialBag(): outBufferCount(0){}
} SerialBag;

// for constructing happans-before between tasks
typedef struct Task {
  std::string     name;       // name of the task
  UNORD_INTSET    inEdges;    // incoming data streams
  UNORD_INTSET    outEdges;   // outgoing data streams
} Task;

typedef SerialBag *SerialBagPtr;

class Checker {
  public:
  VOID addTaskNode(std::string &logLine);
  VOID saveTaskActions(const MemoryActions &taskActions);
  VOID processLogLines(std::string &line);

  // a pair of conflicting task body with a set of line numbers
  VOID checkCommutativeOperations( BugValidator &validator );

  VOID reportConflicts();
  VOID printHBGraph();
  VOID printHBGraphJS();  // for printing dependency graph in JS format
  VOID testing();
  ~Checker();

  private:
    /** Constructs action object from the log file */
    VOID constructMemoryAction(std::stringstream &ssin,
                               std::string &opType,
                               Action &action);

    VOID saveNondeterminismReport(const Action &curWrite,
                                  const Action &write);

    // hold bags of tasks
    std::unordered_map <INTEGER, SerialBagPtr>   serial_bags;
    std::unordered_map<INTEGER, Task>            graph; // in&out edges
    //// for writes
    std::unordered_map<ADDRESS,
        std::list<MemoryActions>>                writes;
    std::map<std::pair<STRING, STRING>, Report>  conflictTable;
    CONFLICT_PAIRS                               conflictTasksAndLines;
    // For holding function signatures.
    SigManager                                   signatureManager;
};

#endif // end checker.h
