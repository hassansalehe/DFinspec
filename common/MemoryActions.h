/////////////////////////////////////////////////////////////////
//  Finspec: a lightweight non-determinism checking
//          tool for shared memory Dataflow applications
//
//    Copyright (c) 2015 - 2017 Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////
//
// This class holds the first action to a memory memory
// location and the last write action.
// Every new write action replaces the previous write.

#ifndef MEMORY_ACTIONS_H
#define MEMORY_ACTIONS_H

#include "action.h"

class MemoryActions {
  public:
    bool isEmpty;
    bool isThereLastWrite;

    Action first;
    Action lastWrite;


    int taskId;
    ADDRESS addr;     // destination address

    // Default constructor
    MemoryActions() {
      isEmpty = true;
    }

    // Constructor which accepts an action
    MemoryActions(Action & action) {

      isEmpty = true;
      isThereLastWrite = false;
      storeAction( action );
    }

    // Stores action if
    // (a) is first action of task, or
    // (b) is last write action
    void storeAction(Action & action) {
       if (isEmpty ) {
         first = action;
         isEmpty = false;
         taskId = action.taskId;
         addr = action.addr;
       }

       if ( action.isWrite ) {
         lastWrite = action;
         isThereLastWrite = true;
       }
    }

    void printActions(ostringstream & os) {
      if( isThereLastWrite ) {
        // means we have both first and last actions
        first.printActionNN( os );
        os << " :: ";
        lastWrite.printAction( os );
      }
      else
        if(! isEmpty ) first.printAction( os );
    }

};

#endif // MemoryActions.h
