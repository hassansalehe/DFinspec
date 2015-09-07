/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking 
//          tool for ADF applications
//
//    (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
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

using namespace std;

typedef        void        VOID;
typedef      void *        ADDRESS;
typedef    ofstream        FILEPTR;
typedef    long int        INTEGER;
typedef    long int        VALUE;
typedef const char*        STRING;
typedef vector<int>        INTVECTOR;
typedef unordered_set<int> INTSET;


#endif
