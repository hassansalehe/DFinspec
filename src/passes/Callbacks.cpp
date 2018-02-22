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

// callbacks for instrumentation

#include "Callbacks.hpp"
#include "Logger.hpp"
#include "TaskInfo.hpp"
#include <thread>
#include <cassert>

// local to a thread(and thus a task)
thread_local TaskInfo taskInfo;

lint getMemoryValue( address addr, ulong size ) {
  if ( size == sizeof(char) ) {
     return *(static_cast<char *>(addr));
  }

  if ( size == sizeof(short) ) {
     return *(static_cast<short *>(addr));
  }

  if ( size == sizeof(int) ) {
     return *(static_cast<int *>(addr));
  }

  if ( size == sizeof(float) ) {
    return *(static_cast<float *>(addr));
  }

  if ( size == sizeof(double) ) {
    return *(static_cast<double *>(addr));
  }

  if ( size == sizeof(long) ) {
     return *(static_cast<long *>(addr));
  }

  if ( size == sizeof(long long) ) {
     return *(static_cast<long long *>(addr));
  }

  // else, get the best value possible.
  return *(static_cast<long long *>(addr));
}

// to initialize the logger
void INS_Init() {
  INS::Init();
}

// to finalize na book-keep the logger
void INS_Fini() {
  INS::Finalize();
}

void INS_TaskBeginFunc( void *taskName ) {

  auto threadID     = static_cast<uint>( pthread_self() );
  taskInfo.threadID = threadID;
  taskInfo.taskID   = INS::GenTaskID();
  taskInfo.taskName = (char *)taskName;
  taskInfo.active   = true;

#ifdef DEBUG
  std::cout << "Task_Began, (threadID: "
            << taskInfo.threadID << ", taskID : "
            << taskInfo.taskID   << ") name: "
            << taskInfo.taskName << std::endl;
#endif
  INS::TaskBeginLog(taskInfo);
}


void INS_TaskFinishFunc( void *addr ) {

  uint threadID = (uint)pthread_self();
  assert (taskInfo.threadID == threadID);
  taskInfo.active = false;

#ifdef DEBUG
  std::cout << "Task_Ended: (threadID: " << threadID
            << ") taskID: " << taskInfo.taskID << std::endl;
#endif
  INS::TaskEndLog(taskInfo);
}

/** Callbacks for tokens */
void INS_RegReceiveToken( address tokenAddr, ulong size ) {
  INTEGER value = getMemoryValue( tokenAddr, size );

  INS::TaskReceiveTokenLog( taskInfo, tokenAddr, value );
  #ifdef DEBUG
    std::cout << "ReceiveToken: " << value
              << " addr: " << tokenAddr << std::endl;
  #endif
}

void INS_RegSendToken( ADDRESS bufLocAddr,
    ADDRESS tokenAddr, ulong size) {

  INTEGER value = getMemoryValue( tokenAddr, size );
  INS::TaskSendTokenLog( taskInfo, bufLocAddr, value );
  #ifdef DEBUG
    std::cout << "SendToken: " << value
              << " addr: " << bufLocAddr << std::endl;
  #endif
}


void toolVptrUpdate( address addr, address value ) {
#ifdef DEBUG
  std::cout << " VPTR write: addr:" << addr
            << " value " << (lint)value << std::endl;
#endif
}

void toolVptrLoad( address addr, address value ) {
#ifdef DEBUG
  std::cout << " VPTR read: addr:" << addr
            << " value " << (lint)value << std::endl;
#endif
}

/** Callbacks for store operations  */
inline void INS_AdfMemRead(
    address addr, ulong size, int lineNo, address funcName  ) {

  //lint value = getMemoryValue( addr, size );
  uint threadID = (uint)pthread_self();
  if ( taskInfo.active ) {
    INS::Read( taskInfo, addr, lineNo, (char*)funcName );

  #ifdef DEBUG
    std::cout << "READ: addr: " << addr << " value: "
              << value << " taskID: "
              << taskInfo.taskID << std::endl;
  #endif
  }
}

void INS_AdfMemRead1( address addr, int lineNo, address funcName  ) {
  INS_AdfMemRead( addr, 1, lineNo, funcName );
}

void INS_AdfMemRead4( address addr, int lineNo, address funcName  ) {
  INS_AdfMemRead( addr, 4, lineNo, funcName );
}

void INS_AdfMemRead8( address addr, int lineNo, address funcName  ) {
  INS_AdfMemRead( addr, 8, lineNo, funcName );
}

/** Callbacks for store operations  */
inline void INS_AdfMemWrite(
    address addr, lint value, int lineNo, address funcName ) {

  uint threadID = (uint)pthread_self();

  if ( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    std::cout << "=WRITE: addr:" << addr << " value "
              << (lint)value << " taskID: " << taskInfo.taskID
              << " line number: " << lineNo << std::endl;
  #endif
  }
}

void INS_AdfMemWrite1(
    address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite(addr, value, lineNo, funcName);
}

void INS_AdfMemWrite4(
    address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite(addr, value, lineNo, funcName);
}

void INS_AdfMemWrite8(
    address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite( addr, value, lineNo, funcName );
}

void INS_AdfMemWriteFloat(
    address addr, float value, int lineNo, address funcName ) {

  uint threadID = (uint)pthread_self();
  #ifdef DEBUG
    printf("store addr %p value %f float\n", addr, value);
  #endif
  if ( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    std::cout << "WRITE: addr:" << addr << " value "
              << (lint)value << " taskID: "
              << taskInfo.taskID << std::endl;
  #endif
  }
}

void INS_AdfMemWriteDouble(
    address addr, double value, int lineNo, address funcName ) {
  uint threadID = (uint)pthread_self();
#ifdef DEBUG
  printf("store addr %p value %f\n", addr, value);
#endif

  if ( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char *)funcName );
  #ifdef DEBUG
    std::cout << "WRITE: addr:" << addr << " value "
              << (lint)value << " taskID: "
              << taskInfo.taskID << std::endl;
  #endif
  }
}
