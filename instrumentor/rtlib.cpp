#include <iostream>
using namespace std;

extern "C" {

// callbacks at creation of task
void AdfCreateTask(void **intokens);

// callbacks for tokens
void regInToken(void * tokenAddr, unsigned long size);
void regOutToken(void * bufferAddr, void * tokenAddr, unsigned long size);

// callbacks for memory access, race detection
void AdfMemRead8(void *addr);
void AdfMemRead4(void *addr);
void AdfMemRead1(void *addr);
void AdfMemWrite8(void *addr, void * value);
void AdfMemWrite4(void *addr, void * value);
void AdfMemWrite1(void *addr, void * value);

// task begin and end callbacks
void taskStartFunc(void* addr);
void taskFinishFunc(void* addr);

void toolVptrUpdate(void *addr, void * value);
void toolVptrLoad(void *addr, void * value);

void hassanFun(void * addr, int value);
};

void AdfCreateTask(void**intokens) {
   if(intokens)
     cout << "IN-TOKEN: " << *intokens << endl;
}

void regInToken(void * tokenAddr, unsigned long size)
{
   long int token = -1;
   if( size == sizeof(int) )
     token = *(static_cast<int *>(tokenAddr));
   cout << "In  Token: " << token << " addr: " << tokenAddr << endl;
}

void regOutToken(void * bufferAddr, void * tokenAddr, unsigned long size)
{
   long int token = -1;
   if( size == sizeof(int))
     token = *(static_cast<int *>(tokenAddr));
   cout << "Out Token: " << token << " addr: " << (static_cast<int *>(bufferAddr)) << endl;
}


void hassanFun(void * addr, int value) {
  cout << "HASSAN FUNC addr: " << addr << " value_from_addr: "<< *(static_cast<int *>(addr))<< " value: "<< value << endl;
}

void taskStartFunc(void* addr) {
  cout << "Task started, (return address : " << addr <<")"<< endl;
}


void taskFinishFunc(void* addr) {
  cout << "Task Ended: (" << addr << ")"<< endl;
}

void toolVptrUpdate(void *addr, void * value) {
  cout << " VPTR write: addr:" << addr << " value " << (long int)value << endl;
}

void toolVptrLoad(void *addr, void * value) {
  cout << " VPTR read: addr:" << addr << " value " << (long int)value << endl;
}

void AdfMemRead1(void *addr) {
    cout << "read:  addr1: "<< addr<< endl;
}

void AdfMemRead4(void *addr) {
    cout << "read:  addr4: "<< addr<< endl;
}

void AdfMemRead8(void *addr) {
    cout << "read:  addr8: "<< addr<< endl;
}

void AdfMemWrite4(void *addr, void * value) {
    cout << "write: addr4:" << addr << " value " << (long int)value << endl;
}

void AdfMemWrite8(void *addr, void * value) {
    cout << "write: addr8:" << addr << " value " << (long int)value << endl;
}

void AdfMemWrite1(void *addr, void * value) {
    cout << "write: addr1:" << addr << " value " << (long int)value << endl;
}

