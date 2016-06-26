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


// This file contains common type defs

#ifndef DEFS_H_ADFinspec
#define DEFS_H_ADFinspec

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

using namespace std;

using Void = void;
typedef        void        VOID;
typedef      void *        ADDRESS;
typedef    ofstream        FILEPTR;
typedef    long int        INTEGER;
typedef    long int        VALUE;
typedef const char*        STRING;
typedef vector<int>        INTVECTOR;
typedef    set<int>        INTSET;
typedef unordered_set<int> UNORD_INTSET;
typedef        bool        BOOL;
typedef vector<string>     STRVECTOR;
using uint    =  unsigned int;
using ulong   =  unsigned long;
using lint    =  long int;
using address =  void *;

// for conflict task pairs
typedef pair<string,string>     STRPAIR;
typedef set<pair<int,int>>      INT_PAIRS;
typedef map<STRPAIR, INT_PAIRS> CONFLICT_PAIRS;

#endif
