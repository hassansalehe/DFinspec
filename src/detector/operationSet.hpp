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

// Defines the OperationSet class to keep the sequence of operations
// on a variable to determine if the operations commute.

#ifndef _DETECTOR_OPERATIONSET_HPP_
#define _DETECTOR_OPERATIONSET_HPP_

// includes and definitions
#include "defs.hpp"

class OperationSet {
  public:
    void clear() { operations.clear(); }

    void appendOperation(OPERATION op) {
      operations.insert( op );
    }

    /**
     * Checks if operation "op" commutes with
     * previous operations which manipulate a
     * shared memory location.
     */
    bool isCommutative(OPERATION op) {

     // compare with other operation
     for (auto i = operations.begin(); i != operations.end(); i++) {
      switch(op) {
        case ADD:
        case SUB:
          if (*i != ADD && *i != SUB) {
            return false;
          }
          break;
        case MUL:
        case DIV:
          if (*i != MUL && *i != DIV) {
            return false;
          }
          break;
        default:
          return false;
        }
      }

      // compatible with all operations in the set
      return true;
    }

  private:
  std::set<OPERATION> operations;

}; // end class

#endif // end operationSet.h
