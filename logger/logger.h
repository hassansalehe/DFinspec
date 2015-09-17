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

#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

typedef        void  VOID;
typedef      void *  ADDRESS;
typedef    ofstream  FILEPTR;
typedef    long int  INTEGER;
typedef const char*  STRING;
typedef    set<int>  INTSET;
typedef vector<int>  INTVECTOR;


class INS {

  public:
    static VOID Init();
    static INTEGER GenTaskID();
    static VOID TaskStartLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value, STRING taskName);
    static VOID TransactionBegin(INTEGER taskID);
    static VOID TransactionEnd(INTEGER taskID);
    static VOID TaskEndLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value);
    static VOID Read(INTEGER taskID, ADDRESS addr, INTEGER value);
    static VOID Read2(INTEGER taskID, ADDRESS addr, int row, int column);
    static VOID Write2(INTEGER taskID, ADDRESS addr, int row, int column);
    static VOID Write(INTEGER taskID, ADDRESS addr, INTEGER value);
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
    static map<pair<ADDRESS,INTEGER>, INTEGER> idMap;

    // keeping the happens-before relation between tasks
    static unordered_map<INTEGER, INTSET> HB;

    // keeping track of the last writer to a memory location
    static unordered_map<ADDRESS, INTEGER> lastWriter;

    // keeping track of the last reader from a memory location
    static unordered_map<ADDRESS, INTEGER> lastReader;
};

#endif
