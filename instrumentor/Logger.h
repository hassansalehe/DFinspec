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

/*
This is a logger for all events in an ADF application
*/


#ifndef LOGGER_H
#define LOGGER_H

#include "defs.h"

struct hash_function {
  size_t operator()(const std::pair<ADDRESS,INTEGER> &p) const {
    auto seed = std::hash<ADDRESS>{}(p.first);
    auto tid = std::hash<INTEGER>{}(p.second);
    //seed ^= tid + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    //return seed;
    // presumably addr and tid are 32 bit
    return (seed << 32) + tid;

    }
};

class INS {

  public:
    static VOID Init();
    static INTEGER GenTaskID();
    static VOID TaskStartLog(INTEGER taskID, STRING taskName);
    static VOID TaskInTokenLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value);
    static VOID TransactionBegin(INTEGER taskID);
    static VOID TransactionEnd(INTEGER taskID);

    static VOID TaskEndLog(INTEGER taskID);
    static VOID TaskOutTokenLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value);

    static VOID Read(INTEGER taskID, ADDRESS addr, INTEGER value);
    static VOID Read2(INTEGER taskID, ADDRESS addr, int row, int column);
    static VOID Write2(INTEGER taskID, ADDRESS addr, int row, int column);
    static VOID Write(INTEGER taskID, ADDRESS addr, INTEGER value, INTEGER lineNo, STRING funcName);
    static VOID Finalize();

  private:
    // a function to tell whether there is a happens-before relation between two tasks
    static bool hasHB(INTEGER tID, INTEGER parentID);

    // a strictly increasing value, used as tasks unique id generator
    static INTEGER taskIDSeed;

    // a file pointer for the log file
    static FILEPTR logger;

    // a file pointer for the HB log file
    static FILEPTR HBlogger;

    // storing task names
    static unordered_map<INTEGER, STRING>funcNames;

    // mapping buffer location, value with task id
    static unordered_map<pair<ADDRESS,INTEGER>, INTEGER, hash_function> idMap;

    // keeping the happens-before relation between tasks
    static unordered_map<INTEGER, INTSET> HB;

    // keeping track of the last writer to a memory location
    static unordered_map<ADDRESS, INTEGER> lastWriter;

    // keeping track of the last reader from a memory location
    static unordered_map<ADDRESS, INTEGER> lastReader;

public:
    // global lock to protect metadata
    static std::mutex guardLock;
};

#endif
