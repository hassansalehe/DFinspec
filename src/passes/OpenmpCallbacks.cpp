#include <omp.h>
#include <ompt.h>
#include <iostream>

#include "Callbacks.h"
#include "Logger.h"
#include "TaskInfo.h"
#include <thread>
#include <cassert>

using namespace std;

// local to a thread(and thus a task)
thread_local TaskInfo taskInfo;

///////////////////////////////////////////
///// Instrumentation functions
//////////////////////////////////////////

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
  taskInfo.threadID = threadID;
  taskInfo.taskID = INS::GenTaskID();
  taskInfo.taskName = (char *)taskName;
  taskInfo.active = true;

#ifdef DEBUG
  cout << "Task_Began, (threadID: "<< taskInfo.threadID << ", taskID : " << taskInfo.taskID <<") name: "<< taskInfo.taskName<< endl;
#endif
  INS::TaskBeginLog(taskInfo);
}

void INS_TaskFinishFunc( void* addr ) {

  uint threadID = (uint)pthread_self();
  assert(taskInfo.threadID == threadID);
  taskInfo.active = false;

//#ifdef DEBUG
  cout << "Task_Ended: (threadID: " << threadID << ") taskID: " << taskInfo.taskID << endl;
//#endif
  INS::TaskEndLog(taskInfo);
}

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

/** Callbacks for tokens */
void INS_RegReceiveToken( address tokenAddr, ulong size )
{
  INTEGER value = getMemoryValue( tokenAddr, size );

  INS::TaskReceiveTokenLog( taskInfo, tokenAddr, value );
  #ifdef DEBUG
    cout << "ReceiveToken: " << value << " addr: " << tokenAddr << endl;
  #endif
}

void INS_RegSendToken( ADDRESS bufLocAddr, ADDRESS tokenAddr, ulong size ) {

  INTEGER value = getMemoryValue( tokenAddr, size );
  INS::TaskSendTokenLog( taskInfo, bufLocAddr, value );
  #ifdef DEBUG
    cout << "SendToken: " << value << " addr: " << bufLocAddr << endl;
  #endif
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

/** Callbacks for load operations  */
inline void INS_AdfMemRead( address addr, ulong size, int lineNo, address funcName  ) {

  //lint value = getMemoryValue( addr, size );
  uint threadID = (uint)pthread_self();

  if( taskInfo.active ) {
    INS::Read( taskInfo, addr, lineNo, (char*)funcName );

  #ifdef DEBUG
    cout << "READ: addr: " << addr << " value: "<< value << " taskID: " << taskInfo.taskID << endl;
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
inline void INS_AdfMemWrite( address addr, lint value, int lineNo, address funcName ) {

  uint threadID = (uint)pthread_self();

  if( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "=WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << taskInfo.taskID << " line number: " << lineNo << endl;
  #endif
  }
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
  #ifdef DEBUG
    printf("store addr %p value %f float\n", addr, value);
  #endif
  if( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << taskInfo.taskID << endl;
  #endif
  }
}

void INS_AdfMemWriteDouble( address addr, double value, int lineNo, address funcName ) {
  uint threadID = (uint)pthread_self();
#ifdef DEBUG
  printf("store addr %p value %f\n", addr, value);
#endif

  if( taskInfo.active ) {
    INS::Write( taskInfo, addr, (lint)value, lineNo, (char*)funcName );
  #ifdef DEBUG
    cout << "WRITE: addr:" << addr << " value " << (lint)value << " taskID: " << taskInfo.taskID << endl;
  #endif
  }
}

//////////////////////////////////////////////////
//// OMPT Callback functions
/////////////////////////////////////////////////
static ompt_get_thread_data_t ompt_get_thread_data;
static ompt_get_unique_id_t ompt_get_unique_id;


#define register_callback_t(name, type)                       \
do{                                                           \
  type f_##name = &on_##name;                                 \
  if (ompt_set_callback(name, (ompt_callback_t)f_##name) ==   \
      ompt_set_never)                                         \
    printf("0: Could not register callback '" #name "'\n");   \
}while(0)

#define register_callback(name) register_callback_t(name, name##_t)

static void
on_ompt_callback_implicit_task(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    unsigned int team_size,
    unsigned int thread_num) {
  switch(endpoint) {
    case ompt_scope_begin:
      INS_TaskBeginFunc(NULL);
      break;
    case ompt_scope_end:
      INS_TaskFinishFunc(NULL);
      break;
  }
}

/// Initialization and Termination callbacks

static int dfinspec_initialize(
  ompt_function_lookup_t lookup,
  ompt_data_t *tool_data) {
  std::cout << "Initializing\n";

  // Register callbacks
  ompt_set_callback_t ompt_set_callback =
     (ompt_set_callback_t) lookup("ompt_set_callback");
  ompt_get_thread_data =
     (ompt_get_thread_data_t) lookup("ompt_get_thread_data");
  ompt_get_unique_id = (ompt_get_unique_id_t) lookup("ompt_get_unique_id");

  register_callback(ompt_callback_implicit_task);

  // Initialize detection runtime
  INS_Init();

  return 1;
}

static void dfinspec_finalize(
  ompt_data_t *tool_data) {
  std::cout << "Finalizing\n";

//  INS_Fini(); // finalize detection runtime
}

ompt_start_tool_result_t* ompt_start_tool(
  unsigned int omp_version,
  const char *runtime_version) {

  static ompt_start_tool_result_t ompt_start_end = {
      &dfinspec_initialize,
      &dfinspec_finalize, 0};
  return &ompt_start_end;
}

