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
#include "MemoryActions.h"

using namespace std;

typedef struct TaskInfo {
  uint threadID = 0;
  uint taskID = 0;
  bool active = false;

  char * taskName;

  // stores pointers of signatures of functions executed by task
  // for faster acces
  unordered_map<STRING, INTEGER> functions;

  // stores memory actions performed by task.
  unordered_map<address, MemoryActions> memoryLocations;

  // improve performance by buffering actions and write only once.
  ostringstream actionBuffer;

  /**
   * Stores the action info as performed by task.
   * The rules for storing this information are
   * explained in MemoryActions.h
   */
  void saveMemoryAction(Action & action) {

    auto loc = memoryLocations.find(action.addr);
    if(loc == memoryLocations.end()) {

      MemoryActions memActions( action );
      memoryLocations[action.addr] = memActions;
    }
    else {
      loc->second.storeAction( action );
    }

  }

  /**
   * Prints to ostringstream all memory access actions
   * recorded.
   */
  void printMemoryActions() {
    for(auto it = memoryLocations.begin(); it != memoryLocations.end(); ++it)
      it->second.printActions( actionBuffer );
  }

  /** HELPER FUNCTIONS */

  /**
   * returns ID if function registered before, otherwise 0.
   */
   INTEGER getFunctionId( STRING funcName ) {
    auto fd = functions.find( funcName );
    if( fd == functions.end() ) return 0;

    return fd->second;
   }

   /**
    * Registers function for faster access.
    */
   void registerFunction(STRING funcName, INTEGER funcId ) {
     functions[funcName] = funcId;
   }

} TaskInfo;

// holder of task identification information
static unordered_map<uint, TaskInfo> taskInfos;

#endif // TaskInfo.h
