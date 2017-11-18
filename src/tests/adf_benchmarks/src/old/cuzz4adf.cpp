#include <cmath>
#include <atomic>

#include "internal.h"


/*
	cuzz4adf.cpp
*/


typedef struct thread_param {
	int ID;
} thread_param_t;


int     testnumthreads = 0;            /* number of threads in the program run */

int     test_thread_stop = 0;
int     test_threads_go = 0;
long    test_ready_task_count;
bool    test_dataflow_execution = false;
pthread_t    test_thread = 0;

int     p_change_num = 0;
long    test_max_tasks = 10000;

int     curr_num = 0;


vector<int> given_priorities;
std::priority_queue<task_t*, std::vector<task_t*>, MyComparator>    p_queue;

void TestWait4Start();



bool ExistsInVector(int in)
{
	if(std::find(given_priorities.begin(), given_priorities.end(), in) != given_priorities.end())
		return true;
		
	return false;
}

int RandomPriority()
{
	return (rand() % test_max_tasks) + p_change_num;
}

void NextPriority(task_t* in)
{
	int rand_num = RandomPriority();
	
	while(ExistsInVector(rand_num))
		rand_num = RandomPriority();
	
	in->priority = rand_num;
	given_priorities.push_back(rand_num);
}


//TO-DO: initialize the priority queue
void CreatePriorityQueue()
{

}

//TO-DO: destroy the priority queue
void DestroyPriorityQueue()
{

}


/*
####################################################################################
	Thread private data
####################################################################################
*/

pthread_key_t  testThreadID;            /* thread ID */
pthread_key_t  test_current_task;        /* reference to the task currently executed by a thread */


int TestGetThreadID()
{
	int *tID = (int *) pthread_getspecific(testThreadID);
	if (tID == NULL) return -1;
	if (*tID < 0) return -1;
	return *tID;
}


void *TestGetCurrentTask()
{
	return pthread_getspecific(test_current_task);
}


void TestSetCurrentTask(void *currtask)
{
	pthread_setspecific(test_current_task, currtask);
}


/*
####################################################################################
	Thread state functions
####################################################################################
*/

void TestReportIdleThread() {
	if (p_queue.empty()) {
		DEBUGPRINT(DEBUG_THREAD, "No ready task!\n");
		test_dataflow_execution = false;
		TaskRegionDone();
	}
}

void TestDescheduleThread(int idle_cycle) {
	TRACE_EVENT(5006,56)
	usleep(1);
	TRACE_EVENT(5006,0)
}


/*
####################################################################################
	Scheduling
####################################################################################
*/


/*
====================================================================================
	GetNextTask
====================================================================================
*/
task_t* TestGetNextTask(int myID) {
	
	task_t *my_next_task = NULL;
	//TO-DO: get top of priority queue
	if(!p_queue.empty())
	{
		my_next_task = p_queue.top();
		p_queue.pop();
	}

	return my_next_task;
}

/* Initial task allocation to worker threads */
void TestDispatchTasks() {
	printf("TestDispatchTasks started!\n");
	long numtasks = task_list.size();
	long i;

	for (i = 0; i < numtasks; i++) {
		if (task_list[i]->status == ready) {
			
			//TO-DO: push to the priority queue
			TestScheduleTask(task_list[i]);
		}
	}

}

/* Called by worker thread for dynamic task creation */
void TestScheduleTask(task_t *task) {
	//printf("TestScheduleTask started!\n");
	if (task->status != ready) {
		task->status = ready;
		test_ready_task_count++;
	}
	
	//TO-DO: push to priority queues
	p_queue.push(task);
}


/*
####################################################################################
	Threading support
####################################################################################
*/

/*
====================================================================================
	ThreadRun
====================================================================================
*/
void *TestThreadRun(void *param)
{
	int		myID;
	int     idle_cycle = 0;
	task_t	*task;

	STM_INIT_THREAD

	/* Set thread ID */
	myID = ((thread_param_t *)param)->ID;
	free(param);
	pthread_setspecific(testThreadID, &myID);

	DEBUGPRINT(DEBUG_THREAD, "Thread %d started\n", myID);

	TestWait4Start();   /* wait until the main thread finishes initialization */

	TRACE_EVENT(5001,51)

	STAT_START_TIMER(thread_run, myID);

	for (;;) {

		if (test_thread_stop != 0) {
			TRACE_EVENT(5005,0)
			DEBUGPRINT(DEBUG_THREAD, "Thread %d exiting ...\n", myID);
			break;
		}

		TRACE_EVENT(5002,52)
		task = NULL;
		task = TestGetNextTask(myID);
		TRACE_EVENT(5002,0)

		/* execute the task	 */
		if (task != NULL) {
			DEBUGPRINT(DEBUG_THREAD, "Thread %d : Dequeued task %d\n", myID, task->taskID);
			/* set the current task reference */

			if (idle_cycle > 0)
				TRACE_EVENT(5005,0)
			idle_cycle = 0;


			TRACE_EVENT(5003,53)

			TestSetCurrentTask((void *) task);
			task->status = running;

			/* run the task as long as there are input tokens ready */
			DEBUGPRINT(DEBUG_THREAD, "Task %d started running!\n", task->taskID);
			RunTask(task);
			DEBUGPRINT(DEBUG_THREAD, "Task %d finished running!\n", task->taskID);			
			
			TRACE_EVENT(5003,0)
			TRACE_EVENT(5004,54)

			if (!task->stop) {
				if (ConsumeTokens(task)) {
					task->status = ready;
					TestScheduleTask(task);

				}
				else
					test_ready_task_count--;
			}
			else {
				task->status = stopped;
				test_ready_task_count--;
			}

			task = NULL;

			TRACE_EVENT(5004,0)
		}
		else {
			if(idle_cycle == 0) {
				DEBUGPRINT(DEBUG_THREAD, "Thread idle!\n");
				TestReportIdleThread();
				TRACE_EVENT(5005,55)
			}
//			TestReportIdleThread();
//			DescheduleThread(idle_cycle);
			idle_cycle++;
		}

	}

	STAT_STOP_TIMER(thread_run, myID);

	GET_STM_STAT(myID)
	STM_EXIT_THREAD

	DEBUGPRINT(DEBUG_THREAD, "Thread %d done\n", myID);
	TRACE_EVENT(5001,0)
	return NULL;
}

/*
====================================================================================
	CreateTestThread
====================================================================================
*/
void TestCreateThread()
{
	int status;

	DEBUGPRINT(DEBUG_THREAD, "Creating testing thread ...\n");
	
	testnumthreads = 1;
	
	test_ready_task_count = 0;
	
	/* create threBad private keys */
	pthread_key_create(&testThreadID, NULL);
	pthread_key_create(&test_current_task, NULL);

	thread_param_t *arg = (thread_param_t *) malloc(sizeof(thread_param_t));
	arg->ID = 0;
	status = pthread_create(&test_thread, NULL, TestThreadRun, (void *) arg);
	
	if (status == EINVAL) {
		PrintError(0, "\n\npthread_create - incorrect argument!\n");
		return;
	}
	if (status == EAGAIN) {
		PrintError(0, "\n\npthread_create - not enough resources to create a thread!\n");
		return;
	}
	if (status == EBUSY) {
		PrintError(0, "\n\npthread_create - thread creation not allowed at this time!\n");
		return;
	}
	
	//TO-DO: Initialize priority queue
	
	//CreatePriorityQueue();
}

/*
####################################################################################
	Thread control
####################################################################################
*/

/*
====================================================================================
	Wait4Start
====================================================================================
*/
void TestWait4Start()
{
	pthread_mutex_lock(&glock);
	pthread_mutex_unlock(&glock);
}

/*
====================================================================================
	BlockThread
====================================================================================
*/
void TestBlockThread()
{
	pthread_mutex_lock(&glock);
}

/*
====================================================================================
	ReleaseThread
====================================================================================
*/
void TestReleaseThreads()
{
	test_threads_go = 1;
	test_dataflow_execution = true;
	pthread_mutex_unlock(&glock);
}

void StopThread()
{
	test_thread_stop = 1;
	
	pthread_join(test_thread, NULL);
	pthread_key_delete(testThreadID);
}

/*
====================================================================================
	Terminate
====================================================================================
*/
void TestTerminate()
{
	DEBUGPRINT(DEBUG_THREAD, "\n####### Terminate ########\n\n");

	/* signal threads to stop working */
	StopThread();

	MapDestroy();	/* this should probably go to TaskRegionDone() */
	DestroyTaskList();
	
	//TO-DO: Destroy priority queue
	//DestroyPriorityQueue();
}
