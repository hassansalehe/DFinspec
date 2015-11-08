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

void INS_TaskStartFunc(void* argAddr) {
  Thread2Task t2t;
  t2t.threadID = (unsigned int)pthread_self();
  t2t.taskID = INS::GenTaskID();
  t2t.active = true;
  thr2TaskMap[t2t.threadID] = t2t;
  cout << "Task_Started, (threadID: "<< t2t.threadID << ", taskID : " << thr2TaskMap[t2t.threadID].taskID <<") addr: "<< argAddr<< endl;
  INS::TaskStartLog(t2t.taskID, NULL, 0, "A");

}


void INS_TaskFinishFunc(void* addr) {

  unsigned int threadID = (unsigned int)pthread_self();
  if(thr2TaskMap.find(threadID) != thr2TaskMap.end())
     thr2TaskMap[threadID].active = false;

  cout << "Task_Ended: (threadID: " << threadID << ")"<< endl;
}

/** Callbacks for tokens */
void INS_RegInToken(void * tokenAddr, unsigned long size)
{
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(tokenAddr, size);
    STRING taskName = "A";
    INS::TaskStartLog(t2t->second.taskID, tokenAddr, value, taskName);
    cout << "InToken: " << value << " addr: " << tokenAddr << endl;
  }
}

void INS_RegOutToken(ADDRESS bufLocAddr, ADDRESS tokenAddr, unsigned long size)
{
  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(tokenAddr, size);
    INS::TaskEndLog(t2t->second.taskID, bufLocAddr, value);
    cout << "OutToken: " << value << " addr: " << bufLocAddr << endl;
  }
}


void toolVptrUpdate(void *addr, void * value) {
  cout << " VPTR write: addr:" << addr << " value " << (long int)value << endl;
}

void toolVptrLoad(void *addr, void * value) {
  cout << " VPTR read: addr:" << addr << " value " << (long int)value << endl;
}

/** Callbacks for store operations  */
void INS_AdfMemRead(void *addr, unsigned long size) {

  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    INTEGER value = getMemoryValue(addr, size);
    cout << "READ: addr: " << addr << " value: "<< value << " taskID: " << t2t->second.taskID << endl;
    INS::Read(t2t->second.taskID, addr, value);
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
void INS_AdfMemWrite(void *addr, void * value) {

  unsigned int threadID = (unsigned int)pthread_self();
  auto t2t = thr2TaskMap.find(threadID);

  if( t2t != thr2TaskMap.end() && t2t->second.active)
  {
    cout << "WRITE: addr:" << addr << " value " << (long int)value << " taskID: " << t2t->second.taskID << endl;
    INS::Write(t2t->second.taskID, addr, (long int)value);
  }
}

void INS_AdfMemWrite1(void *addr, void * value) {
  INS_AdfMemWrite(addr, value);
}

void INS_AdfMemWrite4(void *addr, void * value) {
  INS_AdfMemWrite(addr, value);
}

void INS_AdfMemWrite8(void *addr, void * value) {
  INS_AdfMemWrite(addr, value);
}
