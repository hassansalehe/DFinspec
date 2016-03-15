/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015, 2016 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Defines the Validator class. It determines whether the bugs detected are real.

#ifndef _VALIDATOR_HPP_
#define _VALIDATOR_HPP_

// includes and definitions
#include "defs.h"

enum OPERATION {
  ALLOCA,
  BITCAST,
  CALL,
  GETELEMENTPTR,
  STORE,
  LOAD,
  RET,
  MUL,
  ADD,
  SUB,
  SHL,
};

typedef struct Instruction {
  INTEGER lineNo;
  string destination;
  string type;
  OPERATION oper;
  string operand1;
  string operand2;

  // raw
  string raw;
} Instruction;

class BugValidator {


  public:
    BugValidator(char * IRlogName);
    void validate(CONFLICT_PAIRS & tasksAndLines);

  private:
    map<string, vector<Instruction>> Tasks;
    string trim(string sentence);
    STRVECTOR splitInstruction(string stmt);
    Instruction makeInstruction(string stmt);
    bool involveSimpleOperations(string task1, INTEGER line1);
    bool isSafe(vector<Instruction> & trace, INTEGER loc, string operand);
};


#endif // end validator.h
