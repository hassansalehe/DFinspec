#ifndef __ADF_H__
#define __ADF_H__

#include "statistics.h"
#include "adf_tm.h"


/*
###########################################################################################
	TRACING
###########################################################################################
*/


#ifdef DO_TRACE
  #include "extrae_user_events.h"
  #define TRACE_INIT         Extrae_init();
  #define TRACE_FINALIZE     Extrae_fini();
  #define TRACE_EVENT(a,b)   Extrae_event(a,b);
#else
  #define TRACE_INIT
  #define TRACE_FINALIZE
  #define TRACE_EVENT(a,b)
#endif


/* ADF - EXTRAE event IDs mapping
 * ==================================
 *
 *  type  state  event_name
 *
 * 4xxx   main thread operations
 * ----------------------------------
 *   4001  41  runtime init
 *   4002  42  task creation
 *   4003  43  dataflow execution
 *   4004  44  runtime termination
 *   4005  45  misc init
 *   4006  46  STM init
 *   4007  47  threadpool create
 *   4008  48  create threads
 *   4009  49  create queues
 *
 * 5xxx    thread related
 * ----------------------------------
 *   5001  51  total thread run
 *   5002  52  thread get task
 *   5003  53  thread task run
 *   5004  54  thread schedule task
 *   5005  55  thread idle time
 *   5006  56  thread sleep cycle
 *
 * 6xxx    task related
 * ----------------------------------
 *   6001  61  create task
 *   6002  62  create buffer list
 *   6003  63  create & insert buffer
 *   6004  64  map buffer
 *   6005  65  create instances
 *
 * 7xxx    token passing
 * ----------------------------------
 *   7001  701  PassToken2Buffer
 *   7002  702  handle token
 *   7003  703  buffer lock acquire
 *   7004  704  DequeTask and PutToken
 *   7005  705  blocked-task handling
 *   7010  710  ConsumeTokens
 *   7011  711  single buffer check
 *   7012  712  buffer lock acquire
 *   7013  713  GetToken or EnqueTask
 *   7020  720  PassToken
 *   7021  721  MapGetBufferList
 *   7022  722  pass all token copies
 *   7023  723  pass single token copy
 *
 * 8xxx    worksteel deque ops
 * ----------------------------------
 *   8001  81  pushBottom
 *   8002  82  popBottom
 *   8003  83  steel
 *   8004  84  get reserved task
 */


void InitRuntime (int nthreads);
void TerminateRuntime();


/*
###########################################################################################
	ADF API functions
###########################################################################################
*/

#ifdef ADF

#include <stdarg.h>
#include <functional>

class TaskScheduler;

enum task_args_t {
	PIN = 1
};

typedef struct token_s {
	struct token_s  *next_token;
	void            *value;
	int              size;
} token_t;

extern pthread_mutex_t glock;
/*Test Scheduler*/
extern TaskScheduler* t_scheduler;
void CheckSwitchPoint(TaskScheduler* ts);
/***************/


int  GetThreadID();

void StartDataflow();
void TaskWait();
void StopTask();
void PassToken (void *addr, void *tokendata, size_t token_size);
void CreateTask ( int num_instances, int numtokens, void *tokens[],
				  std::function <void (token_t *)> fn, int num_args, va_list vl);


static inline void adf_init(int nthreads)
{
	InitRuntime(nthreads);
}

static inline void adf_start()
{
	StartDataflow();
}

static inline void  adf_taskwait()
{
	TaskWait();
}

static inline void adf_terminate()
{
	TerminateRuntime();
}

static inline void adf_create_task( int num_instances, int numtokens, void *tokens[],
									std::function <void (token_t *)> fn, int num_args = 0, ...)
{
	va_list vl;
	va_start(vl, num_args);
	CreateTask(num_instances, numtokens, tokens, fn, num_args, vl);
	va_end(vl);
}

static inline void adf_pass_token(void *addr, void *token, size_t token_size)
{
	/* each thread keeps a private reference to the current task */
	PassToken(addr, token, token_size);
}

static inline void adf_task_stop()
{
	StopTask();
}



/*
###########################################################################################
	Non ADF (OpenMP) API functions
###########################################################################################
*/

/* Use for non adf test applications i.e. OpenMP and Cilk
 * Support for TM, statistics and delay */

#elif defined(OPENMP)

#include <omp.h>

extern omp_lock_t glock;

static inline void NonADF_init(int nthreads)
{
	InitRuntime (nthreads);
}

static inline void NonADF_terminate()
{
	TerminateRuntime();
}

static inline void STM_thread_init()
{
	STM_INIT_THREAD
}

static inline void STM_thread_exit(int myID)
{
	GET_STM_STAT(myID)
	STM_EXIT_THREAD
}


#else

/*
###########################################################################################
	Support for sequential applications
###########################################################################################
*/

static inline void sequential_init()
{
	InitRuntime(1);
}

static inline void sequential_terminate()
{
	TerminateRuntime();
}

#endif


#endif /* __ADF_H__ */
