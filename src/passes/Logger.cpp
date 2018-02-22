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

// implements logging functionalities

#include "Logger.hpp"

// static attributes redefined
std::mutex INS::guardLock;

FILEPTR INS::logger;

FILEPTR INS::HBlogger;

std::ostringstream INS::HBloggerBuffer;

std::atomic<INTEGER> INS::taskIDSeed{ 0 };

std::unordered_map<STRING, INTEGER> INS::funcNames;

INTEGER INS::funcIDSeed = 1;

std::unordered_map<std::pair<ADDRESS,INTEGER>,
    INTEGER, hash_function> INS::idMap;

std::unordered_map<INTEGER, INTSET> INS::HB;

std::unordered_map<ADDRESS, INTEGER> INS::lastWriter;

std::unordered_map<ADDRESS, INTEGER> INS::lastReader;

