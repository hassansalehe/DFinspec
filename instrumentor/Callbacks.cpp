#include <iostream>
#include "Callbacks.h"
#include "Logger.h"

using namespace std;

long int getMemoryValue(void *addr, unsigned long size)
{
  if( size == sizeof(char) )
     return *(static_cast<char *>(addr));

  if( size == sizeof(short) )
     return *(static_cast<short *>(addr));

  if( size == sizeof(int) )
     return *(static_cast<int *>(addr));

  if( size == sizeof(long) )
     return *(static_cast<long *>(addr));

   if( size == sizeof(long long) )
     return *(static_cast<long long *>(addr));
   return -1;
}

// to initialize the logger
void INS_Init() {
  INS::Init();
}

// to finalize na book-keep the logger
void INS_Fini() {
  INS::Finalize();
}

void INS_TaskStartFunc(void* taskName) {
  Thread2Task t2t;
  t2t.threadID = (unsigned int)pthread_self();
  t2t.taskID = INS::GenTaskID();
  t2t.active = true;
  thr2TaskMap[t2t.threadID] = t2t;
//#ifdef DEBUG
  cout << "Task_Started, (threadID: "<< t2t.threadID << ", taskID : " << thr2TaskMap[t2t.threadID].taskID <<") name: "<< (char *)taskName<< endl;
//#endif
  INS::TaskStartLog(t2t.taskID, (char*)taskName);

}


void INS_TaskFinishFunc(void* addr) {

  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);
  if(t2t != thr2TaskMap.end())
     thr2TaskMap[threadID].active = false;

  INS::TaskEndLog(t2t->second.taskID);
//#ifdef DEBUG
  cout << "Task_Ended: (threadID: " << threadID << ") taskID: " << thr2TaskMap[threadID].taskID << endl;
//#endif
}

/** Callbacks for tokens */
void INS_RegInToken(void * tokenAddr, unsigned long size)
{
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(tokenAddr, size);
    INS::TaskInTokenLog(t2t->second.taskID, tokenAddr, value);
#ifdef DEBUG
    cout << "InToken: " << value << " addr: " << tokenAddr << endl;
#endif
  }
}

void INS_RegOutToken(ADDRESS bufLocAddr, ADDRESS tokenAddr, unsigned long size)
{
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(tokenAddr, size);
    INS::TaskOutTokenLog(t2t->second.taskID, bufLocAddr, value);
#ifdef DEBUG
    cout << "OutToken: " << value << " addr: " << bufLocAddr << endl;
#endif
  }
}


void toolVptrUpdate(void *addr, void * value) {
#ifdef DEBUG
  cout << " VPTR write: addr:" << addr << " value " << (long int)value << endl;
#endif
}

void toolVptrLoad(void *addr, void * value) {
#ifdef DEBUG
  cout << " VPTR read: addr:" << addr << " value " << (long int)value << endl;
#endif
}

/** Callbacks for store operations  */
void INS_AdfMemRead(void *addr, unsigned long size) {

  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(addr, size);
    INS::Read(t2t->second.taskID, addr, value);
#ifdef DEBUG
    cout << "READ: addr: " << addr << " value: "<< value << " taskID: " << t2t->second.taskID << endl;
#endif
  }
}

void INS_AdfMemRead1(void *addr) {
  INS_AdfMemRead(addr, 1);
}

void INS_AdfMemRead4(void *addr) {
  INS_AdfMemRead(addr, 4);
}

void INS_AdfMemRead8(void *addr) {
  INS_AdfMemRead(addr, 8);
}

/** Callbacks for store operations  */
void INS_AdfMemWrite(void *addr, long int value, int lineNo) {

  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INS::Write(t2t->second.taskID, addr, (long int)value, lineNo);
#ifdef DEBUG
    cout << "=WRITE: addr:" << addr << " value " << (long int)value << " taskID: " << t2t->second.taskID << " line number: " << lineNo << endl;
#endif
  }
}

void INS_AdfMemWrite1(void *addr, long int value, int lineNo) {
  INS_AdfMemWrite(addr, value, lineNo);
}

void INS_AdfMemWrite4(void *addr, long int value, int lineNo) {
  INS_AdfMemWrite(addr, value, lineNo);
}

void INS_AdfMemWrite8(void *addr, long int value, int lineNo) {
  INS_AdfMemWrite(addr, value, lineNo);
}

void INS_AdfMemWriteFloat(void * addr, float value, int lineNo) {
#ifdef DEBUG
  printf("store addr %p value %f float\n", addr, value);
#endif
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
#ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (long int)value << " float, taskID: " << t2t->second.taskID << endl;
#endif
    INS::Write(t2t->second.taskID, addr, (long int)value, lineNo);
  }
}

void INS_AdfMemWriteDouble(void * addr, double value, int lineNo) {
#ifdef DEBUG
  printf("store addr %p value %f\n", addr, value);
#endif
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
#ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (long int)value << " double taskID: " << t2t->second.taskID << endl;
#endif
    INS::Write(t2t->second.taskID, addr, (long int)value, lineNo);
  }
}
