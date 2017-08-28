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

// Defines the Validator class. It determines whether the bugs detected are real.

#ifndef _VALIDATOR_CPP_
#define _VALIDATOR_CPP_


// includes and definitions
#include "validator.h"

VOID BugValidator::parseTasksIR(char * IRlogName)
{
  vector<Instruction> * currentTask = NULL;
  string sttmt; // program statement
  ifstream IRcode(IRlogName); //  open IRlog file

  while( getline( IRcode, sttmt ) )
  {
    if( isEmpty(sttmt) ) continue; // skip empty line
    if( isDebugCall(sttmt) ) continue; // skip debug call

    sttmt = Instruction::trim( sttmt ); // trim spaces
    if ( isValid(sttmt) ) { // check if normal statement

      INTEGER lineNo = getLineNumber( sttmt );

      // skip instruction with line # 0: args to task body
      if (lineNo <= 0) continue;

      sttmt = sttmt.substr(sttmt.find_first_not_of(' ', sttmt.find_first_of(' ')));
      Instruction instr( sttmt );
      instr.lineNo = lineNo;
      currentTask->push_back(instr);
      continue;
    }

    if( isTaskName(sttmt) ) { // else a new task name
      cout << "Task name: " << sttmt << endl;
      Tasks[sttmt] = vector<Instruction>();
      currentTask = &Tasks[sttmt];
      //map<string, map<INTEGER, vector<string>>> Tasks;
      continue;
    }

    // got here :-(, wierd string!
    cout << "Unexpected program statement: " << sttmt << endl;
  }
  IRcode.close();
  cout << "Tasks no: " << Tasks.size() << endl;
}


/**
 * Checks for commutative task operations which have been
 * flagged as conflicts.
 */
VOID BugValidator::validate(CONFLICT_PAIRS & tasksAndLines) {

  // for each task pair
  auto taskPair = tasksAndLines.begin();
  while(taskPair != tasksAndLines.end()) {
     auto it = taskPair++;
     string task1 = it->first.first;
     string task2 = it->first.second;
     cout << task1 << " <--> "<< task2 << endl;// << it->second << endl;

     // iterate over all line pairs
     auto lPair = it->second.begin();
     while(lPair != it->second.end())
     {
        auto temPair = lPair++;

        INTEGER line1 = temPair->first;
        INTEGER line2 = temPair->second;
        cout << "Lines " << line1 << " <--> " << line2 << endl;

        operationSet.clear(); // clear set of commuting operations

        // check if line1 operations commute
        if( involveSimpleOperations( task1, line1 ) &&
	    // check if line2 operations commute
            involveSimpleOperations(  task2, line2)) {
            it->second.erase(temPair);
            cout << "THERE IS SAFETY line1: " << line1 << " line2: " << line2 << endl;
        }
     }
     if(it->second.empty())
       tasksAndLines.erase(it);
  }
}

BOOL BugValidator::involveSimpleOperations(string taskName, INTEGER lineNumber) {

  // get the instructions of a task
  vector<Instruction> &taskBody = Tasks[taskName];
  Instruction instr;
  INTEGER index = -1;

  for(auto i = taskBody.begin(); i != taskBody.end(); i++) {
     if(i->lineNo > lineNumber) break;
     if(i->lineNo == lineNumber) instr  = *i;
     index++;
  }

  cout << "SAFET " << taskName << " " << instr.destination << ", idx: "<< index << endl;

  // expected to be a store
  if(instr.oper == STORE) {
    return isSafe(taskBody, index, instr.operand1);
    //bool r1 = isOnsimpleOperations(lineNumber - 1, istr.destination);
    //bool r2 = isOnsimpleOperations(lineNumber - 1, istr.operand1);
  }
  return false;
}

bool BugValidator::isSafe(const vector<Instruction> & taskBody, INTEGER loc, string operand) {

  if(loc < 0)
      return true;

  Instruction instr = taskBody.at(loc);

  if(instr.oper == ALLOCA && instr.destination == operand)
      return true;

  if( instr.oper == BITCAST ) {
    // destination has been casted from a different address
    if( instr.destination == operand)
      return isSafe(taskBody, loc-1, instr.operand1);
    else
      return isSafe(taskBody, loc-1, operand);
  }

  // used as parameter somewhere and might be a pointer
  if(instr.oper == CALL) {
    if(instr.raw.find(operand) != string::npos)
      return false;
    else
     return isSafe(taskBody, loc-1, operand);
  }

  // ADD or SUB or MUL or DIV
  if(instr.oper == ADD || instr.oper == SUB ||
    instr.oper == MUL || instr.oper == DIV) {
    if(instr.destination == operand) {

      // return immediately is operation can not
      // commute with previous operations
      if(! operationSet.isCommutative(instr.oper ) )
        return false;

      // append the commutative operation
      operationSet.appendOperation( instr.oper );

      bool t1 = isSafe(taskBody, loc-1, instr.operand1);
      bool t2 = isSafe(taskBody, loc-1, instr.operand2);
      return t1 && t2;
    }
    else
      return isSafe(taskBody, loc-1, operand);
  }

  // STORE
  if(instr.oper == STORE) {
    if(instr.destination == operand)
      return isSafe(taskBody, loc-1, instr.operand1);
    return isSafe(taskBody, loc-1, operand);
  }

  // load
  if(instr.oper == LOAD) {
    if(instr.destination == operand)
      return isSafe(taskBody, loc-1, instr.operand1);
  }

  return isSafe(taskBody, loc-1, operand);
  // GETEMEMENTSPTR
}

#endif // end validator.cpp
