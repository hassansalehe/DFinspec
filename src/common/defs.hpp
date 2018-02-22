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


// This file contains common type defs

#ifndef _COMMON_DEFS_HPP_
#define _COMMON_DEFS_HPP_

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
#include <ctime>
#include <chrono>
#include <regex>
#include <sstream>

#include <mutex>          // std::mutex
//#include<//pthread.h>
//#include <thread>       // std::thread

typedef    bool                      BOOL;
typedef    void                      VOID;
typedef    void *                    ADDRESS;
typedef    long int                  INTEGER;
typedef    long int                  VALUE;
typedef    const char*               STRING;
typedef    std::ofstream             FILEPTR;
typedef    std::set<int>             INTSET;
typedef    std::vector<int>          INTVECTOR;
typedef    std::unordered_set<int>   UNORD_INTSET;
typedef    std::vector<std::string>  STRVECTOR;

// for conflict task pairs
typedef    std::pair<std::string,std::string>   STRPAIR;
typedef    std::set<std::pair<int,int>>         INT_PAIRS;
typedef    std::map<STRPAIR, INT_PAIRS>         CONFLICT_PAIRS;

using  Void     =   void;
using  uint     =   unsigned int;
using  ulong    =   unsigned long;
using  lint     =   long int;
using  address  =   void *;

enum OPERATION {
  ALLOCA,
  BITCAST,
  CALL,
  GETELEMENTPTR,
  STORE,
  LOAD,
  RET,
  ADD,
  SUB,
  MUL,
  DIV,
  SHL,
};

static std::string OperRepresentation(OPERATION op) {

  switch(op) {
    case ALLOCA: return "ALLOCA";
    case BITCAST: return "BITCAST";
    case CALL: return "CALL";
    case GETELEMENTPTR: return "GETELEMENTPTR";
    case STORE: return "STORE";
    case LOAD: return "LOAD";
    case RET: return "RET";
    case ADD: return "ADD";
    case SUB: return "SUB";
    case MUL: return "MUL";
    case DIV: return "DIV";
    case SHL: return "SHL";
    default:
      return "UNKNOWN";
  }
};

#endif
