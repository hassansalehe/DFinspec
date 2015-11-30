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

// implements logging functionalities

#ifndef LOGGER_CPP
#define LOGGER_CPP

#include "Logger.h"

#include <mutex>  // std::mutex
//#include <thread>         // std::thread
//#include<//pthread.h>

std::mutex guardLock;

////pthread_mutex_t g_lock;

// static attributes redefined
FILEPTR INS::logger;
FILEPTR INS::HBlogger;
INTEGER INS::taskIDSeed = 0;
unordered_map<INTEGER, STRING> INS::funcNames;
map<pair<ADDRESS,INTEGER>, INTEGER> INS::idMap;
unordered_map<INTEGER, INTSET> INS::HB;
unordered_map<ADDRESS, INTEGER> INS::lastWriter;
unordered_map<ADDRESS, INTEGER> INS::lastReader;

// open file for logging.
VOID INS::Init() {

  //pthread_mutex_init(&g_lock, NULL);
  // reset attributes used
  idMap.clear();
  HB.clear();
  lastReader.clear();
  lastWriter.clear();

  taskIDSeed = 0;

  // get current time to suffix log files
  time_t currentTime;
  time(&currentTime);
  cout << "TIME " << currentTime << endl;

  if(! logger.is_open())
    logger.open("Tracelog-" + to_string(currentTime) + ".txt",  ofstream::out | ofstream::trunc);
  if(! HBlogger.is_open())
    HBlogger.open("HBlog-" + to_string(currentTime) + ".txt",  ofstream::out | ofstream::trunc);
  if(! logger.is_open() || ! HBlogger.is_open())
  {
    cerr << "Could not open log file" << endl;
    cerr << "Exiting ..." << endl;
    exit(EXIT_FAILURE);
  }

}


// Generates a unique ID for each new task.
INTEGER INS::GenTaskID()
{
    guardLock.lock();
    //pthread_mutex_lock(&g_lock);
    INTEGER taskID = taskIDSeed++;

    guardLock.unlock();
    //pthread_mutex_unlock(&g_lock);

    return taskID;
}


// close file used in logging
VOID INS::Finalize() {

  guardLock.lock();
    //pthread_mutex_lock(&g_lock);

  idMap.clear();
  HB.clear();
  lastReader.clear();
  lastWriter.clear();

  if(logger.is_open())
    logger.close();

  if(HBlogger.is_open())
    HBlogger.close();

  guardLock.unlock();
    //pthread_mutex_unlock(&g_lock);
}

VOID INS::TransactionBegin(INTEGER taskID)
{
  guardLock.lock();
  logger << taskID << " " << funcNames[taskID] << " TM ST " << endl;
  guardLock.unlock();
}

VOID INS::TransactionEnd(INTEGER taskID)
{
  guardLock.lock();
  logger << taskID << " "<< funcNames[taskID] << " TM ED" << endl;
  guardLock.unlock();
}

// called when a task starts execution. retrieves parent task id
VOID INS::TaskStartLog(INTEGER taskID, STRING taskName)
{
  guardLock.lock();
    //pthread_mutex_lock(&g_lock);
  funcNames[taskID] = taskName;
  logger << taskID << " " << funcNames[taskID] << " ST " << endl;

  guardLock.unlock();
}

// called when a task starts execution. retrieves parent task id
VOID INS::TaskInTokenLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value)
{
  guardLock.lock();

  INTEGER parentID = -1;

 if(bufLocAddr) { // dependent through a streaming buffer
    parentID = idMap[make_pair(bufLocAddr, value)];

    logger << taskID << " " << funcNames[taskID] << " IT " << parentID << endl;
    HBlogger << taskID << " " << parentID << endl;

    // there is a happens before between taskID and parentID:
    //parentID ---happens-before---> taskID
    if(HB.find(taskID) == HB.end())
      HB[taskID] = INTSET();
    HB[taskID].insert(parentID);

    // take the happens-before of the parentID
    if(HB.find(parentID) != HB.end())
      HB[taskID].insert(HB[parentID].begin(), HB[parentID].end());

    //for(INTSET::iterator pID = HB[taskID].begin(); pID != HB[taskID].end(); pID++)
    //  logger << *pID << " ";
    //logger << endl;
  }

  guardLock.unlock();
   //pthread_mutex_unlock(&g_lock);
}


// called before the task terminates.
VOID INS::TaskEndLog(INTEGER taskID) {

  guardLock.lock();
    //pthread_mutex_lock(&g_lock);
  logger << taskID << " " << funcNames[taskID] << " ED" << endl;

  guardLock.unlock();
    //pthread_mutex_unlock(&g_lock);
}


// stores the buffer address of the token and the value stored in
// the buffer for the succeeding task
VOID INS::TaskOutTokenLog(INTEGER taskID, ADDRESS bufLocAddr, INTEGER value) {

  guardLock.lock();
    //pthread_mutex_lock(&g_lock);

  idMap[make_pair(bufLocAddr, value)] = taskID;
  logger << taskID << " " << funcNames[taskID] << " OT" << endl;

  guardLock.unlock();
    //pthread_mutex_unlock(&g_lock);
}


// provides the address of memory a task reads from
VOID INS::Read(INTEGER taskID, ADDRESS addr, INTEGER value)
{
  guardLock.lock();

  logger << taskID << " " << funcNames[taskID] << " RD "<< addr << " " << value << endl;
  guardLock.unlock();
    //pthread_mutex_unlock(&g_lock);
}


// checks a race at write operation
VOID INS::Write(INTEGER taskID, ADDRESS addr, INTEGER value, INTEGER lineNo)
{
  guardLock.lock();
  logger << taskID << " " << funcNames[taskID] << " WR "<< addr << " " << value << " " << lineNo << endl;
  guardLock.unlock();
}

#endif

