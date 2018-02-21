#include <cmath>
#include <atomic>

//#include "adf.h"
#include "internal.h"



typedef struct thread_param {
	int ID;
} thread_param_t;


int          numthreads = 0;            /* number of threads in the program run */
atomic<int> *thread_stop;               /* semaphores for thread exit */
int          threads_go = 0;
pthread_t   *pool = NULL;               /* the pool of threads */

int          deque_logsize = 10;         /* log size of the deque array */
int          bind_queue_logsize = 8;    /* log size of the deque array */

atomic<long> ready_task_count;
bool         dataflow_execution = false;



void Wait4Start();

/*
####################################################################################
	Thread private data
####################################################################################
*/

pthread_key_t  threadID;            /* thread ID */
pthread_key_t  current_task;        /* reference to the task currently executed by a thread */


int GetThreadID()
{
	int *tID = (int *) pthread_getspecific(threadID);
	if (tID == NULL) return -1;
	if (*tID < 0 || *tID >= numthreads) return -1;
	return *tID;
}


void *GetCurrentTask()
{
	return pthread_getspecific(current_task);
}


void SetCurrentTask(void *currtask)
{
	pthread_setspecific(current_task, currtask);
}




/*
####################################################################################
	Helper functions for debugging
####################################################################################
*/

void PrintDequeStates() {
	int i;
	long t, b;
	for (i=0; i<numthreads; i++) {
		t = deque_list[i].top;
		b = deque_list[i].bottom;
		printf("deque_%d = (%ld, %ld)\n", i, t, b);
	}
}

void PrintTaskStates() {
	printf("\n\n");
	printf("task list size: %d\n", (int) task_list.size());
	for (int i = 0; i< (int) task_list.size(); i++) {
		switch (task_list[i]->status) {
		case waiting:
			printf("task %d: waiting\n", i);
			break;
		case queued:
			printf("task %d: queued\n", i);
			break;
		case running:
			printf("task %d: running\n", i);
			break;
		case ready:
			printf("task %d: ready\n", i);
			break;
		case stopped:
			printf("task %d: stopped\n", i);
			break;
		default:
			printf("task status error!\n");
			break;
		}
	}
}

void PrintBlockedTaskCount() {
	printf("\n\nblocked tasks: %d\n\n", CountBlockedTasks());
}


/*
####################################################################################
	Thread state functions
####################################################################################
*/

void ReportIdleThread() {
	if (ready_task_count.load() == 0) {
		dataflow_execution = false;
		TaskRegionDone();
	}
}

void DescheduleThread(int idle_cycle) {
	TRACE_EVENT(5006,56)
	usleep(1);
	TRACE_EVENT(5006,0)
}


/*
####################################################################################
	Scheduling
####################################################################################
*/

/* This is a trivial implementation that tries
 * to steal from each queue in the ascending order */
task_t* AscendingSteal(int thief) {
	task_t *next_task = NULL;
	int i;

	for ( i=0; i < numthreads; i++) {
		if (i == thief) continue; /* don't steal form our deque */
		if (steal(&deque_list[i], &next_task))
			break;
	}

	return next_task;
}


/* Random task stealing */
task_t* RandomSteal(int thief) {
	task_t *next_task = NULL;
	int cnt = 0, rnd;

	while (cnt < numthreads) {
		rnd = rand()%numthreads;
		if (rnd != thief) {
			if (steal(&deque_list[rnd], &next_task))
				break;
		}
		cnt++;
	}

	return next_task;
}

/* Neighbour task stealing */
task_t* NeighbourSteal(int thief) {
	task_t *next_task = NULL;
	int i, victim;

	for (i=1; i<= numthreads/2 ; i++) {
		victim = (numthreads + thief-i) % numthreads;
		if (victim != thief) {
			if (steal(&deque_list[victim], &next_task))
				break;
		}
		next_task = NULL;
		victim = (thief+i) % numthreads;
		if (victim != thief) {
			if (steal(&deque_list[victim], &next_task))
				break;
		}
		next_task = NULL;
	}

	return next_task;
}


/* This should implement scheduling algorithm */
task_t* GetNextTask(int myID) {
	task_t *my_next_task = NULL;

	if (BindQueuePop_lock(&bind_queue_list[myID], &my_next_task)) {
		return my_next_task;
	}

	if (GetReservedTask(myID, &my_next_task)) {
		return my_next_task;
	}

	/* try to deque task from our own deque */
	if (popBottom(&deque_list[myID], &my_next_task)) {
		assert(my_next_task != NULL);
		return my_next_task;
	}

	/* now try to steal the task from other queues */
	//my_next_task = AscendingSteal(myID);
	my_next_task = NeighbourSteal(myID);
	//my_next_task = RandomSteal(myID);

	return my_next_task;
}


/* Initial task allocation to worker threads */
void DispatchTasks() {
	long numtasks = task_list.size();
	long i, cnt = 0;

	for (i = 0; i < numtasks; i++) {
		if (task_list[i]->status == ready) {
			if (task_list[i]->thread_bind_id != -1) {
				BindQueuePush_lock(&bind_queue_list[task_list[i]->thread_bind_id], task_list[i]);
				ready_task_count ++;
			}
			else {
				pushBottom(&deque_list[cnt % numthreads], task_list[i]);
				cnt++;
			}
		}
	}

	ready_task_count += cnt;
}

/* Called by worker thread for dynamic task creation */
void ScheduleTask(task_t *task) {
	if (task->status != ready) {
		task->status = ready;
		ready_task_count++;
	}
	if (task->thread_bind_id != -1)
		BindQueuePush_lock(&bind_queue_list[task->thread_bind_id], task);
	else {
		if (!ReserveTask(task))
			pushBottom(&deque_list[GetThreadID()], task);
	}
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
void *ThreadRun(void *param)
{
	int		myID;
	int     idle_cycle = 0;
	task_t	*task;

	STM_INIT_THREAD

	/* Set thread ID */
	myID = ((thread_param_t *)param)->ID;
	free(param);
	pthread_setspecific(threadID, &myID);

	DEBUGPRINT(DEBUG_THREAD, "Thread %d started\n", myID);

	Wait4Start();   /* wait until the main thread finishes initialization */

	TRACE_EVENT(5001,51)

	STAT_START_TIMER(thread_run, myID);

	for (;;) {

		if (thread_stop[myID].load() != 0) {
			TRACE_EVENT(5005,0)
			DEBUGPRINT(DEBUG_THREAD, "Thread %d exiting ...\n", myID);
			break;
		}

		TRACE_EVENT(5002,52)
		task = NULL;
		task = GetNextTask(myID);
		TRACE_EVENT(5002,0)

		/* execute the task	 */
		if (task != NULL) {
			DEBUGPRINT(DEBUG_THREAD, "Thread %d : Dequeued task %d\n", myID, task->taskID);
			/* set the current task reference */

			if (idle_cycle > 0)
				TRACE_EVENT(5005,0)
			idle_cycle = 0;


			TRACE_EVENT(5003,53)

			SetCurrentTask((void *) task);
			task->status = running;

			/* run the task as long as there are input tokens ready */
			RunTask(task);

			TRACE_EVENT(5003,0)
			TRACE_EVENT(5004,54)

			if (!task->stop) {
				if (ConsumeTokens(task)) {
					task->status = ready;
					ScheduleTask(task);
//					if (task->thread_bind_id != -1)
//						BindQueuePush_lock(&bind_queue_list[task->thread_bind_id], task);
//					else
//						pushBottom(&deque_list[myID], task);
				}
				else
					ready_task_count--;
			}
			else {
				task->status = stopped;
				ready_task_count--;
			}

			task = NULL;

			TRACE_EVENT(5004,0)
		}
		else {
			if(idle_cycle == 0) {
				ReportIdleThread();
				TRACE_EVENT(5005,55)
			}
//			ReportIdleThread();
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
	CreateThreadPool
====================================================================================
*/
void CreateThreadPool(int Nthreads)
{
	int i, status;

	numthreads = Nthreads;

	DEBUGPRINT(DEBUG_THREAD, "Creating a thread pool ...\n");

	/* sanity check the numthreads argument */
	if ((numthreads <= 0) || (numthreads > MAX_THREADS)) {
		PrintError(0, "\n\nCreateThreadPool: numthreads argument wrong : %d!\n", numthreads);
		return;
	}

	/* Reset thread exit flags */
	thread_stop = new atomic<int>[numthreads];
	for (i=0; i<numthreads; i++)
		thread_stop[i] = 0;

	ready_task_count = 0;

	TRACE_EVENT(4008,48)

	/* create the thread pool */
	pool = (pthread_t *) malloc(numthreads * sizeof(pthread_t));
	if (pool == NULL) {
		PrintError(0, "\n\nOut of memory creating a new threadpool!\n");
		return;
	}

	/* create thread private keys */
	pthread_key_create(&threadID, NULL);
	pthread_key_create(&current_task, NULL);

	/* create threads */
	for (i=0 ; i<numthreads; i++) {
		thread_param_t *arg = (thread_param_t *) malloc(sizeof(thread_param_t));
		arg->ID = i;
		status = pthread_create(&pool[i], NULL, ThreadRun, (void *) arg);
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
	}

	TRACE_EVENT(4008,0)
	TRACE_EVENT(4009,49)

	/* create worksteal deques */
	CreateDequeList(numthreads, deque_logsize);
	CreateBindQueueList(numthreads, bind_queue_logsize);

	TRACE_EVENT(4009,0)

}


/*
####################################################################################
	Thread control
####################################################################################
*/

void Wait4Start()
{
	pthread_mutex_lock(&glock);
	pthread_mutex_unlock(&glock);
}


void BlockThreads()
{
	pthread_mutex_lock(&glock);
}


void ReleaseThreads()
{
	threads_go = 1;
	dataflow_execution = true;
	pthread_mutex_unlock(&glock);
}

void StopThreads()
{
	int i=0;
	for(i=0; i<numthreads; i++)
		thread_stop[i] = 1;

	/* wait for threads to finish */
	for (i=0; i<numthreads; i++)
		pthread_join(pool[i], NULL);

	pthread_key_delete(threadID);
	free(pool);
	delete [] thread_stop;
	thread_stop = NULL;
	pool = NULL;
}


/*
====================================================================================
	Terminate
====================================================================================
*/
void Terminate()
{
	DEBUGPRINT(DEBUG_THREAD, "\n####### Terminate ########\n\n");

	/* signal threads to stop working */
	StopThreads();

	MapDestroy();	/* this should probably go to TaskRegionDone() */
	DestroyTaskList();
	DestroyDequeList(numthreads);
	DestroyBindQueueList(numthreads);
}

