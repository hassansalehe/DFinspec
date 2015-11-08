#include <iostream>
#include "Callbacks.h"
#include "Logger.h"

using namespace std;

long int getTokenValue(void *addr, unsigned long size)
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


void AdfCreateTask(void**intokens, void* fn) {
//   if(intokens)
//     cout << "IN-TOKEN: " << *intokens << " Function: "<< fn<<  endl;
}

void INS_RegInToken(void * tokenAddr, unsigned long size)
{
   INTEGER taskID = 0;
   INTEGER value = getTokenValue(tokenAddr, size);
   STRING taskName = "A";
   INS::TaskStartLog(taskID, tokenAddr, value, taskName);
   cout << "InToken: " << value << " addr: " << tokenAddr << endl;
}

void INS_RegOutToken(ADDRESS bufLocAddr, ADDRESS tokenAddr, unsigned long size)
{
  INTEGER taskID = 0;
  INTEGER value = getTokenValue(tokenAddr, size);
  INS::TaskEndLog(taskID, bufLocAddr, value);
  cout << "OutToken: " << value << " addr: " << bufLocAddr << endl;
}


void hassanFun(void * addr, int value) {
  cout << "HASSAN FUNC addr: " << addr << " value_from_addr: "<< *(static_cast<int *>(addr))<< " value: "<< value << endl;
}

void INS_TaskStartFunc(void* addr) {
  cout << "Task_Started, (return address : " << addr <<")"<< endl;
}


void INS_TaskFinishFunc(void* addr) {
  cout << "Task_Ended: (" << addr << ")"<< endl;
}

void toolVptrUpdate(void *addr, void * value) {
  cout << " VPTR write: addr:" << addr << " value " << (long int)value << endl;
}

void toolVptrLoad(void *addr, void * value) {
  cout << " VPTR read: addr:" << addr << " value " << (long int)value << endl;
}

void INS_AdfMemRead1(void *addr) {

  INTEGER taskID = 0;
  INTEGER value = getTokenValue(addr, 1);
  INS::Read(taskID, addr, value);

  cout << "read:  addr1: "<< addr<< endl;
}

void INS_AdfMemRead4(void *addr) {
  cout << "read:  addr4: "<< addr<< endl;

  INTEGER taskID = 1;
  INTEGER value = getTokenValue(addr, 4);
  INS::Read(taskID, addr, value);
    cout << "read:  addr4: "<< addr<< " value: " << value << endl;
}

void INS_AdfMemRead8(void *addr) {

  INTEGER taskID = 0;
  INTEGER value = getTokenValue(addr, 8);
  INS::Read(taskID, addr, value);

  cout << "read:  addr8: "<< addr << " value: " << value << endl;
}

void INS_AdfMemWrite1(void *addr, void * value) {

  INTEGER taskID = 0;
  INTEGER value_ = getTokenValue(addr, 1);
  cout << "write: addr1:" << addr << " value " << (long int)value << endl;
  INS::Write(taskID, addr, (long int)value);
}

void INS_AdfMemWrite4(void *addr, void * value) {

  INTEGER taskID = 0;
  cout << "write: addr4:" << addr << " value " << (long int)value << endl;
  INS::Write(taskID, addr, (long int)value);
}

void INS_AdfMemWrite8(void *addr, void * value) {

  INTEGER taskID = 0;
  INTEGER value_ = getTokenValue(addr, 8);
  cout << "write: addr8:" << addr << " value " << (long int)value << endl;
  INS::Write(taskID, addr, (long int)value);
}

