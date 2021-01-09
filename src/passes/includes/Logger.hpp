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

// This is a logger for all events in an ADF application

#ifndef _PASSES_INCLUDES_LOGGER_HPP_
#define _PASSES_INCLUDES_LOGGER_HPP_

#include "TaskInfo.hpp"
#include "defs.hpp"

#include <atomic>
#include <mutex>

struct hash_function {
  size_t operator()( const std::pair<ADDRESS,INTEGER> &p ) const {
    auto seed = std::hash<ADDRESS>{}(p.first);
    auto tid  = std::hash<INTEGER>{}(p.second);
    //seed ^= tid + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    //return seed;
    // presumably addr and tid are 32 bit
    return (seed << 32) + tid;
  }
};

class INS {

  private:
    // a function to tell whether there is a happens-before relation between two tasks
    static inline bool hasHB( INTEGER tID, INTEGER parentID );

    // a strictly increasing value, used as tasks unique id generator
    static std::atomic<INTEGER>                 taskIDSeed;

    // a file pointer for the log file
    static FILEPTR                              logger;

    // a file pointer for the HB log file
    static FILEPTR                              HBlogger;
    static std::ostringstream                   HBloggerBuffer;

    // storing function name pointers
    static std::unordered_map<STRING, INTEGER>  funcNames;
    static INTEGER                              funcIDSeed;

    // mapping buffer location, value with task id
    static std::unordered_map<
        std::pair<ADDRESS,INTEGER>,
        INTEGER, hash_function      >           idMap;

    // keeping the happens-before relation between tasks
    static std::unordered_map<INTEGER, INTSET>  HB;

    // keeping track of the last writer to a memory location
    static std::unordered_map<ADDRESS, INTEGER> lastWriter;

    // keeping track of the last reader from a memory location
    static std::unordered_map<ADDRESS, INTEGER> lastReader;

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
      struct tm *timeinfo = localtime(&currentTime);

      char buff[40];
      strftime( buff, 40, "%d-%m-%Y_%H.%M.%S", timeinfo );
      std::string timeStr( buff );

      if (! logger.is_open() ) {
        logger.open( "Tracelog_" + timeStr + ".txt",
            std::ofstream::out | std::ofstream::trunc );
      }

      if (! HBlogger.is_open()) {
        HBlogger.open( "HBlog_" + timeStr + ".txt",
            std::ofstream::out | std::ofstream::trunc );
      }

      if (! logger.is_open() || ! HBlogger.is_open() ) {
        std::cerr << "Could not open log file \nExiting ...\n";
        exit(EXIT_FAILURE);
      }
    }

    /** Generates a unique ID for each new task. */
    static inline INTEGER GenTaskID() {
      INTEGER taskID = taskIDSeed.fetch_add(1);
      return taskID;
    }

    /** registers the function if not registered yet.
     * Also prints the function to standard output
     */
    static inline INTEGER RegisterFunction(const STRING funcName) {

      guardLock.lock();
      int funcID = 0;
      auto fd = funcNames.find(funcName);
      if ( fd == funcNames.end() ) { // new function
        funcID = funcIDSeed++;
        funcNames[funcName] = funcID;
        // print to the log file
        logger << funcID << " F " << funcName << std::endl;
      } else {
         funcID = fd->second;
      }

      guardLock.unlock();
      return funcID;
    }

    /** close file used in logging */
    static inline VOID Finalize() {
      guardLock.lock();

      // Write HB relations to file
      HBlogger << HBloggerBuffer.str();

      idMap.clear(); HB.clear();
      lastReader.clear(); lastWriter.clear();

      if ( logger.is_open() ) logger.close();
      if ( HBlogger.is_open() ) HBlogger.close();
      guardLock.unlock();
    }

    static inline VOID TransactionBegin( TaskInfo & task ) {
      task.actionBuffer << task.taskID << " BTM "
                        << task.taskName << std::endl;
    }

    static inline VOID TransactionEnd( TaskInfo & task ) {
      task.actionBuffer << task.taskID << " ETM "
                        << task.taskName << std::endl;
    }

    /** called when a task begins execution and retrieves parent task id */
    static inline VOID TaskBeginLog( TaskInfo& task) {
      task.actionBuffer << task.taskID << " B "
                        << task.taskName << std::endl;
    }

    /** called when a task begins execution. retrieves parent task id */
    static inline VOID TaskReceiveTokenLog(
        TaskInfo & task,
        ADDRESS bufLocAddr,
        INTEGER value ) {
      INTEGER parentID = -1;
      auto tid = task.taskID;

      guardLock.lock();
      // if streaming location address not null and
      // there is a sender of the token
      auto key = std::make_pair( bufLocAddr, value );
      if (bufLocAddr && idMap.count( key ) ) {
        // dependent through a streaming buffer
        parentID = idMap[key];

        if (parentID != tid) {
          // there was a bug where a task could send token to itself
          HBloggerBuffer << tid << " " << parentID << std::endl;

          // there is a happens before between taskID and parentID:
          //parentID ---happens-before---> taskID
          if (HB.find( tid ) == HB.end()) {
            HB[tid] = INTSET();
          }
          HB[tid].insert( parentID );

          // take the happens-before of the parentID
          if (HB.find( parentID ) != HB.end()) {
            HB[tid].insert(HB[parentID].begin(), HB[parentID].end());
          }
          task.actionBuffer << tid << " C " << task.taskName << " "
                            << parentID << std::endl;
        }
      }
      guardLock.unlock();
    }

    /** called before the task terminates. */
    static inline VOID TaskEndLog( TaskInfo& task ) {

      task.printMemoryActions();
      task.actionBuffer << task.taskID << " E "
                        << task.taskName << std::endl;

      guardLock.lock(); // protect file descriptor
      logger << task.actionBuffer.str(); // print to file
      guardLock.unlock();

      task.actionBuffer.str(""); // clear buffer
    }

    /**
     * stores the buffer address of the token and the value stored in
     * the buffer for the succeeding task
     */
    static inline VOID TaskSendTokenLog(
        TaskInfo &task,
        ADDRESS bufLocAddr,
        INTEGER value ) {

      auto key = std::make_pair(bufLocAddr, value );
      task.actionBuffer << task.taskID << " S "
                        << task.taskName << std::endl;
      guardLock.lock(); //  protect file descriptor & idMap
      idMap[key] = task.taskID;
      guardLock.unlock();
    }

    /** provides the address of memory a task reads from */
    static inline VOID Read(
        TaskInfo &task,
        ADDRESS addr,
        INTEGER lineNo,
        STRING funcName ) {
      INTEGER funcID = task.getFunctionId( funcName );
      // register function if not registered yet
      if ( funcID == 0 ) {
        funcID = RegisterFunction( funcName );
        task.registerFunction( funcName, funcID );
      }
      task.saveReadAction(addr, lineNo, funcID);
    }

    /** stores a write action */
    static inline VOID Write(
        TaskInfo &task,
        ADDRESS addr,
        INTEGER value,
        INTEGER lineNo,
        STRING funcName ) {

      INTEGER funcID = task.getFunctionId( funcName );
      // register function if not registered yet
      if ( funcID == 0 ) {
        funcID = RegisterFunction( funcName );
        task.registerFunction( funcName, funcID );
      }
      task.saveWriteAction(addr, value, lineNo, funcID);
    }
};
#endif
