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

#ifndef _VALIDATOR_CPP_
#define _VALIDATOR_CPP_


// includes and definitions
#include "validator.h"

string BugValidator::trim(string sentence) {
    size_t start = sentence.find_first_not_of(' ');
    size_t end = sentence.find_last_not_of(' ');

    string st = sentence.substr(start, (end -start)+1);
    return st;
}

BugValidator::BugValidator(char * IRlogName)
{

  vector<Instruction> * currentTask = NULL;

  string sttmt; // program statement
  ifstream IRcode(IRlogName); //  open IRlog file
  while( getline( IRcode, sttmt ) )
  {
     // skip if line is empty
     if(sttmt.find_first_not_of(' ') == string::npos)
       continue;

     // if is a debug call
     if (sttmt.find("llvm.dbg.declare") != string::npos)
       continue;

     sttmt = trim(sttmt); // trim spaces

     // check if normal statement
     if ( regex_search (sttmt, regex("^[0-9]+: ") ))
     {
       // do something with
       // get line number
       smatch result;
       regex_search(sttmt, result, regex("^[0-9]+") );

       if(result.size() <= 0 || result.size()> 1 )
       {
         // a line should have only one line number
         cerr << "Incorrect number of lines: " << IRlogName << endl;
         exit(EXIT_FAILURE);
       }

       INTEGER lineNo = stoul(result[0]);
        sttmt = sttmt.substr(sttmt.find_first_not_of(' ', sttmt.find_first_of(' ')));
       Instruction instr = makeInstruction(sttmt);
       instr.lineNo = lineNo;
       currentTask->push_back(instr);
       continue;
     }

     // else a new task name
     if(sttmt.find_first_of(' ') == string::npos)
     {
       //
       cout << "Task name: " << sttmt << endl;
       Tasks[sttmt] = vector<Instruction>();
       currentTask = &Tasks[sttmt];
       //map<string, map<INTEGER, vector<string>>> Tasks;
       continue;
     }

     // wierd string
     cout << "Wierd program statement: " << sttmt << endl;
  }
  IRcode.close();
  cout << "Tasks no: " << Tasks.size();
}


VOID BugValidator::validate(CONFLICT_PAIRS & tasksAndLines) {

  // for each task pair
  auto taskPair = tasksAndLines.begin();
  while(taskPair != tasksAndLines.end())
  {
     auto it = taskPair++;
     string task1 = it->first.first;
     string task2 = it->first.second;
     cout << task1 << " "<< task2 << endl;// << it->second << endl;

     // iterate over all line pairs
     auto lPair = it->second.begin();
     while(lPair != it->second.end())
     {
        auto temPair = lPair++;

        INTEGER line1 = temPair->first;
        INTEGER line2 = temPair->second;
        cout << "Lines " << line1 << " " << line2 << endl;

        // check if line1 operations commute
        if( involveSimpleOperations( task1, line1 ) &&
	    // check if line2 operations commute
            involveSimpleOperations(  task2, line2)) {
            it->second.erase(temPair);
            cout << "THERE IS SAFETY" << endl;
        }
     }
     if(it->second.empty())
       tasksAndLines.erase(it);
  }
}

vector<string> BugValidator::splitInstruction(string stmt) {

  // split statements
  vector<string> segments;
  stringstream ss( stmt ); // make string stream.
  string tok;

  while(getline(ss, tok, ',')) {
    segments.push_back(tok);
  }

  vector<string> segments2;
  // make individual words
  for(auto stmt : segments) {
    string trimed = trim(stmt);
    stringstream sg2(trimed);

    while( getline(sg2, tok, ' ') ) {
       segments2.push_back(tok);
    }
  }
  return segments2;
}

BOOL BugValidator::involveSimpleOperations(
    string taskName,
    INTEGER lineNumber) {

  // get the instructions of a task
  vector<Instruction> taskBody = Tasks[taskName];
  Instruction instr;
  INTEGER index = -1;

  for(auto i = taskBody.begin(); i != taskBody.end(); i++)
  {
     if(i->lineNo > lineNumber)
          break;

     if(i->lineNo == lineNumber)
      instr  = *i;

     index++;
  }

    cout << "SAFET " << taskName << " " << instr.destination << "idx "<< index << endl;
  // expected to be a store
  if(instr.oper == STORE) {

     return isSafe(taskBody, index, instr.operand1);
      cout << "A store " << instr.destination << endl;

      //bool r1 = isOnsimpleOperations(lineNumber - 1, istr.destination);
      //bool r2 = isOnsimpleOperations(lineNumber - 1, istr.operand1);
  }
  return false;
}

bool BugValidator::isSafe(vector<Instruction> & trace, INTEGER loc, string operand) {
    if(loc < 0)
        return true;

    Instruction instr = trace.at(loc);

    if(instr.oper == ALLOCA && instr.destination == operand)
        return true;

    //
    if( instr.oper == BITCAST ) {
      if( instr.destination == operand)
        return isSafe(trace, loc-1, instr.operand1);
      else
          return isSafe(trace, loc-1, operand);
    }

    // used as parameter somewhere and might be a pointer
    if(instr.oper == CALL) {
        if(instr.raw.find(operand) != string::npos)
          return false;
        else
            return isSafe(trace, loc-1, operand);
    }
    // MUL
    if(instr.oper == MUL) {
       if(instr.destination == operand)
          return false;
       else
          return isSafe(trace, loc-1, operand);
    }

    // ADD
    if(instr.oper == ADD || instr.oper == SUB) {
       if(instr.destination == operand) {
          bool t1 = isSafe(trace, loc-1, instr.operand1);
          bool t2 = isSafe(trace, loc-1, instr.operand2);
          return t1 && t2;
      }
   }

   // STORE
   if(instr.oper == STORE) {
      if(instr.destination == operand)
         return isSafe(trace, loc-1, instr.operand1);
      return isSafe(trace, loc-1, operand);
   }

   // load
   if(instr.oper == LOAD) {
      if(instr.destination == operand)
         return isSafe(trace, loc-1, instr.operand1);
   }

   return isSafe(trace, loc-1, operand);
    // GETEMEMENTSPTR

}
//bool isOnsimpleOperations(lineNumber - 1, istr.operand1) {

//}

Instruction BugValidator::makeInstruction(string stmt) {
  // is assignment
  Instruction instr;
  instr.raw = trim( stmt );

  vector<string> contents = splitInstruction(stmt);

  if(contents[0] == "store") {
    instr.oper = STORE;
    instr.destination = contents[4];
    instr.operand1 = contents[2];
    instr.operand2 = contents[2];
    instr.type = contents[1];
  }
  else if(contents[2] == "load") {
    instr.oper = LOAD;
    instr.destination = contents[0];
    instr.operand1 = contents[5];
    instr.type = contents[3];
  }
  else if (regex_search(contents[2], regex("[fidb]add")) ||
      regex_search(contents[2], regex("[fidb]mul")) ||
      regex_search(contents[2], regex("[fidb]sub")) ) {

    instr.destination = contents[0];
    instr.type = contents[3];
    instr.operand1 = contents[4];
    instr.operand2 = contents[5];
    if (regex_search(contents[2], regex("[fidb]add")))
        instr.oper = ADD;
    if (regex_search(contents[2], regex("[fidb]mul")))
        instr.oper = MUL;
    // ...

  }
  else if(contents[2] == "add" ||
          contents[2] == "sub" ||
          contents[2] == "mul" ||
          contents[2] == "shl") {
     instr.destination = contents[0];
     instr.oper = contents[2] == "add" ? ADD :
     (instr.oper = contents[2] == "sub"? SUB :
     (instr.oper = contents[2] == "mul"? MUL : SHL));
     // <result> = add nuw nsw <ty> <op1>, <op2>  ; yields {ty}:result
     if(contents[3] == "nuw" && contents[4] == "nsw") {
        instr.type = contents[5];
        instr.operand1 = contents[6];
        instr.operand2 = contents[7];
     }
     else if(contents[3] == "nuw" || contents[3] == "nsw") {
        // <result> = add nuw <ty> <op1>, <op2>      ; yields {ty}:result
        // <result> = add nsw <ty> <op1>, <op2>      ; yields {ty}:result
        instr.type = contents[4];
        instr.operand1 = contents[5];
        instr.operand2 = contents[6];
     }
     else {
        // <result> = add <ty> <op1>, <op2>          ; yields {ty}:result
        instr.type = contents[3];
        instr.operand1 = contents[4];
        instr.operand2 = contents[5];
     }
  }
  else if(contents[2] == "alloca") {
    instr.destination = contents[0];
    instr.oper = ALLOCA;
    instr.type = contents[3];
  }
  else if(contents[2] == "bitcast") {
    instr.destination = contents[0];
    instr.oper = BITCAST;
    instr.operand1 = contents[4];
    instr.operand2 = contents[4];
  }
  else if(contents[0] == "call") {
    instr.oper = CALL;
  }
  /*
  // find operation
  if(regex_search(segments[0], regex("store ")) {
     instr.oper = STORE;
     string tmp = ;
     stringstream a(trim(segments[0]));
     tmp = ""'
     (getline(ss, tok, ','); //store
  }
  if(regex_search(segments[0], regex("load "))
     instr.oper = LOAD;

  if(regex_search(segments[0], regex("call "))
     instr.oper = CALL;

  if(regex_search(segments[0], regex("alloca "))
     instr.oper = ALLOCA;

  if(regex_search(segments[0], regex("bitcast "))
     instr.oper = BITCAST;

  if(regex_search(segments[0], regex("[fidb]add "))
     instr.oper = ADD;

  if(regex_search(segments[0], regex("[fidb]mull "))
     instr.oper = MULT;

  */
  return instr;
}

#endif // end validator.cpp
