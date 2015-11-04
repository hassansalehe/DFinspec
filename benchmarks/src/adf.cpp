/*
###########################################################################################
	ADF API functions
###########################################################################################
*/

#ifdef ADF

#include "internal.h"

pthread_mutex_t glock;
TaskScheduler* t_scheduler;

/*
 * 
 * name: 	CheckSwitchPoint
 * @param	none
 * @return	none
 * 
 * Function for checking if at the current step any priority change point
 * is assigned or not. If it there is a priority change point it will call
 * call SuspendCurrentThread function and will wait the scheduler to 
 * schedule a new thread. If there is no priority change point it will
 * continue execution.
 * 
 */
void CheckSwitchPoint(TaskScheduler* ts)
{  
	TESTPRINT(printf("TestScheduler::CheckSwitchPoint\n");)
	bool is_pchange = ts->pchange_steps.find(ts->current_step++) != ts->pchange_steps.end();
	
	if(is_pchange)
	{
		TESTPRINT(printf("TestScheduler::CheckSwitchPoint::is_pchange\n");)
		ts->SuspendCurrentThread();
	}

}

void InitRuntime (int nthreads)
{
	TRACE_INIT
	TRACE_EVENT(4001,41)
	TRACE_EVENT(4005,45)

	InitDelay();
	StartClock();
	numthreads = nthreads;
	TaskRegionInit();
	STAT_INIT(nthreads)

	TRACE_EVENT(4005,0)
	TRACE_EVENT(4006,46)

	STM_INIT
	STM_INIT_THREAD
	STM_STAT_INIT

	TRACE_EVENT(4006,0)
	TRACE_EVENT(4007,47)
	
#ifdef USE_TEST_THREAD
	//Create the scheduler and block the threads 
	//For testing scheduler
	//TestBlockThread();		
	//TestCreateThread();
	//srand (time(NULL));
	printf("INIT TASKSCHEDULER\n");
	t_scheduler = new TaskScheduler();
#else
	BlockThreads();   /* acquire the lock that will block all threads
							until the main thread creates all tasks */
	CreateThreadPool(numthreads);
#endif

	TRACE_EVENT(4007,0)
	TRACE_EVENT(4001,0)
	TRACE_EVENT(4002,42)
}


void TerminateRuntime()
{
#ifdef USE_TEST_THREAD
	t_scheduler->Terminate();
#else
	Terminate();
#endif
	STM_EXIT_THREAD
	STM_EXIT

	StopClock();
	PRINT_GLOBAL_STM_STAT
	PRINT_TIME_PER_THREAD
	PRINT_GLOBAL_TIME
	PRINT_GLOBAL_STAT
	PrintTime();
	STAT_CLEAN

	TRACE_EVENT(4004,0)
	TRACE_FINALIZE
}


void StartDataflow()
{

 #ifdef USE_TEST_THREAD
	//Go over tasks and add any ready thread to the priority queue
	//Start scheduler
	//For testing scheduler
	//TestDispatchTasks();
	//TestReleaseThreads();
	printf("StartDataflow()\n");
	//ReleaseThreads();
	t_scheduler->InitPQueue(task_list);
	t_scheduler->Start();
#else
	DispatchTasks();
	ReleaseThreads();
#endif

	TRACE_EVENT(4002,0)
	TRACE_EVENT(4003,43)
}


void TaskWait()
{
#ifndef USE_TEST_THREAD
	TaskRegionWait();
#endif

	TRACE_EVENT(4003,0)
	TRACE_EVENT(4004,44)
}


void StopTask()
{
	task_t *task = NULL;
#ifdef USE_TEST_THREAD
	task = t_scheduler->current_thread->task;
#else
	task = (task_t *) GetCurrentTask();
#endif
	task->stop = true;
}


void PassToken (void *addr, void *tokendata, size_t token_size)
{
	map_buffer_list_t *bufflist = NULL;

	TRACE_EVENT(7020,720)
	TRACE_EVENT(7021,721)
	STAT_START_TIMER(pass_tokens, GetThreadID());

	bufflist = MapGetBufferList(addr);

	TRACE_EVENT(7021,0)
	TRACE_EVENT(7022,722)

	if (bufflist == NULL) {
		printf("PassToken : output token not mapped\taddr(token) = %p\n", addr);
		exit(1);
	}

	while (bufflist != NULL) {
		TRACE_EVENT(7023,723)
		/* create a new token object */
		token_t *newtoken = (token_t *) malloc(sizeof(token_t));
		newtoken->next_token = NULL;
		newtoken->value = malloc(token_size);
		newtoken->size = token_size;

		/* copy token value */
		memcpy(newtoken->value, tokendata, token_size);

		/* pass a copy of the token to the buffer */
		PassToken2Buffer(bufflist->buff, newtoken);
		
		/* next buffer */
		bufflist = bufflist->next;
		TRACE_EVENT(7023,0)
	}


	STAT_STOP_TIMER(pass_tokens, GetThreadID());

	TRACE_EVENT(7022,0)
	TRACE_EVENT(7020,0)
}

/*
###########################################################################################
	Non ADF (OpenMP) API functions
###########################################################################################
*/

#elif defined(OPENMP)

#include "adf.h"
#include "common.h"

omp_lock_t      glock;

void InitRuntime (int nthreads)
{
	InitDelay();
	StartClock();
	STAT_INIT(nthreads)
	STM_INIT
	STM_STAT_INIT
}


void TerminateRuntime()
{
	STM_EXIT
	StopClock();
	PRINT_GLOBAL_STM_STAT
	PRINT_TIME_PER_THREAD
	PRINT_GLOBAL_TIME
	PRINT_GLOBAL_STAT
	PrintTime();
	STAT_CLEAN
}

/*
###########################################################################################
	Support for sequential applications
###########################################################################################
*/

#else /* sequential */

#include "adf.h"
#include "common.h"

void InitRuntime (int nthreads)
{
	TRACE_INIT
	InitDelay();
	StartClock();
	STAT_INIT(1)
}


void TerminateRuntime()
{
	StopClock();
	PRINT_GLOBAL_TIME
	PRINT_GLOBAL_STAT
	PrintTime();
	STAT_CLEAN
	TRACE_FINALIZE
}

#endif


