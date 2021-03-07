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

// Defines a class to hold task identification info

#ifndef _PASSES_INCLUDES_TASKINFO_HPP_
#define _PASSES_INCLUDES_TASKINFO_HPP_

#include "defs.hpp"
#include "MemoryActions.hpp"

typedef struct TaskInfo {
  uint threadID    =  0;
  uint taskID      =  0;
  bool active      =  false;
  char *taskName;

  // stores pointers of signatures of functions executed by task
  // for faster acces
  std::unordered_map<STRING, INTEGER>         functions;

  // stores memory actions performed by task.
  std::unordered_map<address, MemoryActions>  memoryLocations;

  // improve performance by buffering actions and write only once.
  std::ostringstream                          actionBuffer;

  /**
   * Stores the action info as performed by task.
   * The rules for storing this information are
   * explained in MemoryActions.h
   */
  inline void saveMemoryAction(Action &action) {
    MemoryActions &loc = memoryLocations[action.addr];
    loc.storeAction( action );
  }

  inline void saveReadAction(
      ADDRESS &addr,
      INTEGER &lineNo,
      const INTEGER funcID) {
    MemoryActions & loc = memoryLocations[addr];

    if ( loc.hasWrite() ) return;

    Action action( taskID, addr, 0, lineNo, funcID );
    action.isWrite = false;
    loc.storeAction( action );
  }

  inline void saveWriteAction(
      ADDRESS addr,
      INTEGER value,
      INTEGER lineNo,
      INTEGER funcID) {
     MemoryActions & loc = memoryLocations[addr];

     bool isWrite = true;
     loc.storeAction(taskID, addr, value, lineNo, funcID, isWrite);
  }

  /**
   * Prints to ostringstream all memory access actions
   * recorded.
   */
  void printMemoryActions() {
    for (auto& memAction : memoryLocations) {
      memAction.second.printActions( actionBuffer );
    }
  }

  /** HELPER FUNCTIONS */

  /**
   * returns ID if function registered before, otherwise 0.
   */
   inline INTEGER getFunctionId( const STRING funcName ) {
     auto fd = functions.find( funcName );
     if (fd == functions.end()) {
       return 0;
     } else {
       return fd->second;
     }
   }

   /**
    * Registers function for faster access.
    */
   void registerFunction(STRING funcName, INTEGER funcId ) {
     functions[funcName] = funcId;
   }

} TaskInfo;

// holder of task identification information
static std::unordered_map<uint, TaskInfo> taskInfos;

#endif // TaskInfo.h
