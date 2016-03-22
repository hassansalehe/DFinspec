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

// callbacks for instrumentation

#include <iostream>
#include <pthread.h>
#include <map>

using namespace std;

extern "C" {

  // to initialize the logger
  void INS_Init();

  // to finalize and book-keep the logger
  void INS_Fini();

  // callbacks at creation of task
  void AdfCreateTask(void **intokens, void* fn);

  // callbacks for tokens
  void INS_RegInToken(void * tokenAddr, unsigned long size);
  void INS_RegOutToken(void * bufferAddr, void * tokenAddr, unsigned long size);

  // callbacks for memory access, race detection
  void INS_AdfMemRead8(void *addr);
  void INS_AdfMemRead4(void *addr);
  void INS_AdfMemRead1(void *addr);
  void INS_AdfMemWrite8(void *addr, long int value, int lineNo, void * funcName);
  void INS_AdfMemWrite4(void *addr, long int value, int lineNo, void * funcName);
  void INS_AdfMemWrite1(void *addr, long int value, int lineNo, void * funcName);

  void INS_AdfMemWriteFloat(void * addr, float value, int lineNo, void * funcName);
  void INS_AdfMemWriteDouble(void * addr, double value, int lineNo, void * funcName);

  // task begin and end callbacks
  void INS_TaskStartFunc(void* addr);
  void INS_TaskFinishFunc(void* addr);

  void toolVptrUpdate(void *addr, void * value);
  void toolVptrLoad(void *addr, void * value);

  typedef struct Thread2Task {
    unsigned int threadID = 0;
    unsigned int taskID = 0;
    bool active = false;
  } Thread2Task;

  map<unsigned int, Thread2Task> thr2TaskMap;
};

