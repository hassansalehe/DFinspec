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

/*
This is a logger for all events in an ADF application
*/

#ifndef LOGGER_H
#define LOGGER_H

#include "defs.h"
#include "TaskInfo.h"

struct hash_function {
  size_t operator()( const std::pair<ADDRESS,INTEGER> &p ) const {
    auto seed = std::hash<ADDRESS>{}(p.first);
    auto tid = std::hash<INTEGER>{}(p.second);
    //seed ^= tid + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    //return seed;
    // presumably addr and tid are 32 bit
    return (seed << 32) + tid;
  }
};

class INS {

  private:
    // a function to tell whether there is a happens-before relation between two tasks
    static bool hasHB( INTEGER tID, INTEGER parentID );

    // a strictly increasing value, used as tasks unique id generator
    static INTEGER taskIDSeed;

    // a file pointer for the log file
    static FILEPTR logger;

    // a file pointer for the HB log file
    static FILEPTR HBlogger;

    // storing task names
    static unordered_map<INTEGER, STRING>taskNames;

    // storing function name pointers
    static unordered_map<STRING, INTEGER>funcNames;
    static INTEGER funcIDSeed;

    // mapping buffer location, value with task id
    static unordered_map<pair<ADDRESS,INTEGER>, INTEGER, hash_function> idMap;

    // keeping the happens-before relation between tasks
    static unordered_map<INTEGER, INTSET> HB;

    // keeping track of the last writer to a memory location
    static unordered_map<ADDRESS, INTEGER> lastWriter;

    // keeping track of the last reader from a memory location
    static unordered_map<ADDRESS, INTEGER> lastReader;

  public:
    // global lock to protect metadata, use this lock
    // when you call any function of this class
    static std::mutex guardLock;

    // open file for logging.
    static inline VOID Init() {

      // reset attributes used
      idMap.clear(); HB.clear();
      lastReader.clear(); lastWriter.clear();

      taskIDSeed = 0;

      // get current time to suffix log files
      time_t currentTime; time(&currentTime);
      struct tm * timeinfo = localtime(&currentTime);

      char buff[40];
      strftime( buff, 40, "%d-%m-%Y_%H.%M.%S", timeinfo );
      string timeStr( buff );

      if(! logger.is_open() )
        logger.open( "Tracelog_" + timeStr + ".txt",  ofstream::out | ofstream::trunc );
      if(! HBlogger.is_open())
        HBlogger.open( "HBlog_" + timeStr + ".txt",  ofstream::out | ofstream::trunc );
      if(! logger.is_open() || ! HBlogger.is_open() ) {
        cerr << "Could not open log file \nExiting ..." << endl;
        exit(EXIT_FAILURE);
      }
    }

    /** Generates a unique ID for each new task. */
    static inline INTEGER GenTaskID() {
      // guardLock.lock();
      INTEGER taskID = taskIDSeed++;
      // guardLock.unlock();
      return taskID;
    }

    /** close file used in logging */
    static inline VOID Finalize() {
      //guardLock.lock();
      idMap.clear(); HB.clear();
      lastReader.clear(); lastWriter.clear();

      if( logger.is_open() ) logger.close();
      if( HBlogger.is_open() ) HBlogger.close();
      //guardLock.unlock();
    }

    static inline VOID TransactionBegin( INTEGER taskID ) {
      // guardLock.lock();
      logger << taskID << " STTM " << taskNames[taskID] << endl;
      // guardLock.unlock();
    }

    static inline VOID TransactionEnd( INTEGER taskID ) {
      // guardLock.lock();
      logger << taskID << " EDTM "<< taskNames[taskID] << endl;
      // guardLock.unlock();
    }

    /** called when a task starts execution. retrieves parent task id */
    static inline VOID TaskStartLog( TaskInfo& task, STRING taskName ) {
      // guardLock.lock();
      auto tid = task.taskID;
      taskNames[tid] = taskName;
      task.actionBuffer << tid << " B " << taskNames[tid] << endl;
      // guardLock.unlock();
    }

    /** called when a task starts execution and retrieves parent task id */
    static inline VOID TaskInTokenLog( TaskInfo & task, ADDRESS bufLocAddr, INTEGER value ) {
      // guardLock.lock();
      INTEGER parentID = -1;
      auto tid = task.taskID;

      // if streaming location address not null and there is a sender of the token
      auto key = make_pair( bufLocAddr, value );
      if(bufLocAddr && idMap.count( key ) ) { // dependent through a streaming buffer
        parentID = idMap[key];

        if(parentID == tid) { // there was a bug where a task could send token to itself
          // guardLock.unlock();
          return; // do nothing, just return
        }

        task.actionBuffer << tid << " C " << taskNames[tid] << " " << parentID << endl;
        HBlogger << tid << " " << parentID << endl;

        // there is a happens before between taskID and parentID:
        //parentID ---happens-before---> taskID
        if(HB.find( tid ) == HB.end())
          HB[tid] = INTSET();
        HB[tid].insert( parentID );

        // take the happens-before of the parentID
        if(HB.find( parentID ) != HB.end())
          HB[tid].insert(HB[parentID].begin(), HB[parentID].end());
      }
      // guardLock.unlock();
    }

    /** called before the task terminates. */
    static inline VOID TaskEndLog( TaskInfo& task ) {
      // guardLock.lock();
      auto tid = task.taskID;
      task.actionBuffer << tid << " E " << taskNames[tid] << endl;
      logger << task.actionBuffer.str(); // print to file
      task.actionBuffer.str(""); // clear buffer
      // guardLock.unlock();
    }

    /**
     * stores the buffer address of the token and the value stored in
     * the buffer for the succeeding task
     */
    static inline VOID TaskOutTokenLog( TaskInfo & task, ADDRESS bufLocAddr, INTEGER value ) {
      // guardLock.lock();
      auto tid = task.taskID;
      auto key = make_pair(bufLocAddr, value );
      idMap[key] = tid;
      task.actionBuffer << tid << " S " << taskNames[tid] << endl;
      logger << task.actionBuffer.str(); // print to file
      task.actionBuffer.str(""); // clear buffer
      // guardLock.unlock();
    }

    /** provides the address of memory a task reads from */
    static inline VOID Read( TaskInfo & task, ADDRESS addr, INTEGER value ) {
      return; // logging reads currently disabled because we don't use reads to check non-determinism
      // guardLock.lock();
      auto tid = task.taskID;
      task.actionBuffer << tid << " R " << addr << " " << value << endl;
      // guardLock.unlock();
    }

    /** stores a write action */
    static inline VOID Write( TaskInfo & task, ADDRESS addr, INTEGER value, INTEGER lineNo, STRING funcName ) {
      // guardLock.lock();
      auto tid = task.taskID;

      int funcID = 0;
      if( funcNames.find(funcName) == funcNames.end() ) { // new function
        funcID = funcIDSeed++;
        funcNames[funcName] = funcID;

        // print to the log file
        task.actionBuffer << funcID << " F " << funcName << endl;
      }

      funcID = funcNames[funcName];
      task.actionBuffer << tid << " W " <<  addr << " " << value << " " << lineNo << " " << funcID << endl;
      // guardLock.unlock();
    }
};
#endif
