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

// callbacks for instrumentation

#include "Callbacks.h"
#include "Logger.h"
#include "TaskInfo.h"

using namespace std;

lint getMemoryValue( address addr, ulong size )
{
  if( size == sizeof(char) )
     return *(static_cast<char *>(addr));

  if( size == sizeof(short) )
     return *(static_cast<short *>(addr));

  if( size == sizeof(int) )
     return *(static_cast<int *>(addr));

  if( size == sizeof(float) )
    return *(static_cast<float *>(addr));

  if( size == sizeof(double) )
    return *(static_cast<double *>(addr));

  if( size == sizeof(long) )
     return *(static_cast<long *>(addr));

   if( size == sizeof(long long) )
     return *(static_cast<long long *>(addr));

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

void INS_TaskBeginFunc( void* taskName ) {

  auto threadID = static_cast<uint>( pthread_self() );
  INS::guardLock.lock(); // protect the task id map

  taskInfos[threadID] = TaskInfo();
  auto& t2t = taskInfos[threadID];
  t2t.threadID = threadID;
  t2t.taskID = INS::GenTaskID();
  t2t.active = true;

#ifdef DEBUG
  cout << "Task_Began, (threadID: "<< t2t.threadID << ", taskID : " << taskInfos[t2t.threadID].taskID <<") name: "<< (char *)taskName<< endl;
#endif
  INS::TaskBeginLog(t2t, (char*)taskName);
  INS::guardLock.unlock(); // release lock
}


void INS_TaskFinishFunc( void* addr ) {

  INS::guardLock.lock(); // protect the task id map

  uint threadID = (uint)pthread_self();
  auto t2t = taskInfos.find(threadID);
  if(t2t != taskInfos.end())
     taskInfos[threadID].active = false;

#ifdef DEBUG
  cout << "Task_Ended: (threadID: " << threadID << ") taskID: " << taskInfos[threadID].taskID << endl;
#endif
  INS::TaskEndLog(t2t->second);
  INS::guardLock.unlock(); // release lock
}

/** Callbacks for tokens */
void INS_RegReceiveToken( address tokenAddr, ulong size )
{
  uint threadID = (uint)pthread_self();
  INTEGER value = getMemoryValue( tokenAddr, size );

  INS::guardLock.lock(); // protect the task id map
  auto t2t = taskInfos.find(threadID);
  if( t2t != taskInfos.end() && t2t->second.active )
    INS::TaskReceiveTokenLog( t2t->second, tokenAddr, value );
  #ifdef DEBUG
    cout << "ReceiveToken: " << value << " addr: " << tokenAddr << endl;
  #endif
  INS::guardLock.unlock(); // release lock
}

void INS_RegSendToken( ADDRESS bufLocAddr, ADDRESS tokenAddr, ulong size ) {

  uint threadID = (uint)pthread_self();
  INTEGER value = getMemoryValue( tokenAddr, size );

  INS::guardLock.lock(); // protect the task id map
  auto t2t = taskInfos.find( threadID );
  if( t2t != taskInfos.end() && t2t->second.active )
    INS::TaskSendTokenLog( t2t->second, bufLocAddr, value );
  #ifdef DEBUG
    cout << "SendToken: " << value << " addr: " << bufLocAddr << endl;
  #endif
  INS::guardLock.unlock(); // release lock
}


void toolVptrUpdate( address addr, address value ) {
#ifdef DEBUG
  cout << " VPTR write: addr:" << addr << " value " << (lint)value << endl;
#endif
}

void toolVptrLoad( address addr, address value ) {
#ifdef DEBUG
  cout << " VPTR read: addr:" << addr << " value " << (lint)value << endl;
#endif
}

/** Callbacks for store operations  */
void INS_AdfMemRead( address addr, ulong size, int lineNo, address funcName  ) {

  lint value = getMemoryValue( addr, size );
  uint threadID = (uint)pthread_self();

  INS::guardLock.lock(); // protect the task id map
  auto t2t = taskInfos.find(threadID);
  if( t2t != taskInfos.end() && t2t->second.active ) {

    TaskInfo & taskInfo = t2t->second;
    INS::Read( taskInfo, addr, value, lineNo, (char*)funcName );

  #ifdef DEBUG
    cout << "READ: addr: " << addr << " value: "<< value << " taskID: " << t2t->second.taskID << endl;
  #endif
  }
  INS::guardLock.unlock(); // release lock
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
void INS_AdfMemWrite( address addr, lint value, int lineNo, address funcName ) {

  INS::guardLock.lock(); // protect the task id map
  uint threadID = (uint)pthread_self();
  auto t2t = taskInfos.find( threadID );

  if( t2t != taskInfos.end() && t2t->second.active ) {
    INS::Write( t2t->second, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "=WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << t2t->second.taskID << " line number: " << lineNo << endl;
  #endif
  }
  INS::guardLock.unlock(); // release lock
}

void INS_AdfMemWrite1( address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite(addr, value, lineNo, funcName);
}

void INS_AdfMemWrite4( address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite(addr, value, lineNo, funcName);
}

void INS_AdfMemWrite8( address addr, lint value, int lineNo, address funcName ) {
  INS_AdfMemWrite( addr, value, lineNo, funcName );
}

void INS_AdfMemWriteFloat( address addr, float value, int lineNo, address funcName ) {

  uint threadID = (uint)pthread_self();
  INS::guardLock.lock(); // protect the task id map
  #ifdef DEBUG
    printf("store addr %p value %f float\n", addr, value);
  #endif
  auto t2t = taskInfos.find( threadID );
  if( t2t != taskInfos.end() && t2t->second.active ) {
    INS::Write( t2t->second, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << t2t->second.taskID << endl;
  #endif
  }
  INS::guardLock.unlock(); // release lock
}

void INS_AdfMemWriteDouble( address addr, double value, int lineNo, address funcName ) {
  uint threadID = (uint)pthread_self();
  INS::guardLock.lock(); // protect the task id map
#ifdef DEBUG
  printf("store addr %p value %f\n", addr, value);
#endif

  auto t2t = taskInfos.find( threadID );
  if( t2t != taskInfos.end() && t2t->second.active )
    INS::Write( t2t->second, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << t2t->second.taskID << endl;
  #endif
  INS::guardLock.unlock(); // release lock
}
