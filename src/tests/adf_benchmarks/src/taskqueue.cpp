#include "internal.h"
#include <vector>

task_region_t            task_region;
std::vector<task_t*>     task_list;

void TaskRegionDone();

/*
====================================================================================
	TaskListInsert
====================================================================================
*/
void TaskListInsert(task_t *task)
{
	assert (task != NULL);
	task_list.push_back(task);
}

void DestroyTaskList() {
	std::vector<task_t*>::iterator it;
	for (it=task_list.begin(); it != task_list.end(); it++) {
		DestroyTask(*it);
	}
	task_list.clear();
}



/*
====================================================================================
	InitQueue
====================================================================================
*/
void InitQueue(task_queue_t *queue)
{
	queue->qhead = NULL;
	queue->qtail = NULL;
	queue->empty = true;
	queue->size = 0;
}

/*
====================================================================================
	GetQueueSize
====================================================================================
*/
int GetQueueSize(task_queue_t *queue)
{
	return queue->size;
}

/*
====================================================================================
	QueueEmpty
====================================================================================
*/
bool QueueEmpty(task_queue_t *queue)
{
	return queue->empty;
}

/*
====================================================================================
	EnqueueTask
====================================================================================
*/
int EnqueueTask(task_queue_t *queue, task_t *task)
{
	task->status = waiting;

	/* validate the task and the queueu */
	if (task == NULL || queue == NULL){
		errno = ENOMEM;
		exit(1);
	}

	/* add a task at the end of the queue */
	if (queue->empty == true) {
		queue->empty = false;
		queue->qhead = task;
		queue->qtail = task;
		task->next_task = task->prev_task = NULL;
	}
	else {
		queue->qtail->next_task = task;
		queue->qtail = task;
		task->prev_task = queue->qtail;
		task->next_task = NULL;
	}

	queue->size++;

	return 0;
}


/*
====================================================================================
	DequeueTask

	The function can return a NULL task pointer!
====================================================================================
*/
int DequeueTask(task_queue_t *queue, task_t **task)
{
	/* validate the queue */
	if (queue == NULL) {
		errno = ENOMEM;
		return 1;
	}

	/* get the task from the head of the queue */
	*task = queue->qhead;
	if ((*task) != NULL) {
		if (queue->qhead == queue->qtail) {
			queue->qhead = queue->qtail = NULL;
			queue->empty = true;
		}
		else
			queue->qhead = queue->qhead->next_task;

		(*task)->next_task = NULL;
		(*task)->prev_task = NULL;
		queue->size--;
	}

	return 0;
}


/*
====================================================================================
	FreeQueue
====================================================================================
*/
void FreeQueue(task_queue_t *queue)
{
	queue->qhead = queue->qtail = NULL;
	free(queue);
}


/*
====================================================================================
	CreateTask
====================================================================================
*/
void HandleExtraArguments(task_t *task, int num_args, va_list vl) {
	int i;
	enum task_args_t arg_type;

	/* set taks parameters to default values */
	task->thread_bind_id = -1;

	for (i = 0; i < num_args/2; i++) {
		arg_type = (enum task_args_t) va_arg(vl, int);
		switch (arg_type) {
		case PIN:
			task->thread_bind_id = va_arg(vl, int);
			break;
		}
	}

}


/*
====================================================================================
	CreateTask
====================================================================================
*/
void CreateTask (int num_instances, int numtokens, void *tokens[], std::function <void (token_t *)> fn, int num_args, va_list vl)
{
	TRACE_EVENT(6001,61)
	STAT_START_TIMER(create_task, 0);

	int i;
	static int ID = 0;
	token_buffer_t *newbuff = NULL;
	token_buffer_list_t *bufflist = (token_buffer_list_t *) malloc(sizeof(token_buffer_list_t));
	task_t *newtask;

	/* initialize buffer list */
	bufflist->buff = NULL;
	bufflist->num_buffers = 0;
	bufflist->numtasks = num_instances;

	/* create a list of input buffers - one for each input token */
	TRACE_EVENT(6002,62)
	for (i=0; i<numtokens; i++) {
		TRACE_EVENT(6003,63)
		newbuff = CreateTokenBuffer(-1);
		InsertBuffer(bufflist, newbuff);

		TRACE_EVENT(6003,0)
		TRACE_EVENT(6004,64)
		/* add new entry into the buffer map for a token address and associated buffer */
		MapInsert(tokens[i], newbuff);
		TRACE_EVENT(6004,0)
	}

	TRACE_EVENT(6002,0)
	TRACE_EVENT(6005,65)

	/* create task instances */
	for (i=0; i<num_instances; i++) {
		newtask = (task_t *) malloc(sizeof(task_t));
		newtask->prev_task = newtask->next_task = NULL;
		newtask->token_list = newtask->token_list_tail = NULL;
		newtask->buffer_list = NULL;
		newtask->current_buffer = NULL;

		newtask->taskID = ++ID;
		newtask->fn = new std::function <void (token_t *)> (fn);
		newtask->stop = false;
		newtask->numtokens = numtokens;
		newtask->buffer_list = bufflist;
		newtask->current_buffer = bufflist->buff;

		HandleExtraArguments(newtask, num_args, vl);

		DEBUGPRINT(DEBUG_TASK, "Created task %d\n", newtask->taskID);

		if (newtask->current_buffer != NULL) {
			/* For dynamic task creation we need to lock the buffer */
			EnqueueTask( newtask->current_buffer->wait_queue, newtask);
		}
		else {
			newtask->status = ready;
			if (dataflow_execution)
			#ifdef USE_TEST_THREAD
				t_scheduler->AddTask(newtask);
			#else
				ScheduleTask(newtask);
			#endif
		}

		TaskListInsert(newtask);
	}

	STAT_STOP_TIMER(create_task, 0);
	TRACE_EVENT(6005,0)
	TRACE_EVENT(6001,0)
}

/*
====================================================================================
	RunTask
====================================================================================
*/
void RunTask(task_t *task)
{
	STAT_START_TIMER(run_task, GetThreadID());
	
	/* Run an outlined procedure for the task - fn */
	(*(task->fn))(task->token_list);
	/* free used tokens */
	if (task->token_list != NULL) {
		FreeTokenList(task->token_list);
		task->token_list = task->token_list_tail = NULL;
	}
	STAT_STOP_TIMER(run_task, GetThreadID());
}

/*
====================================================================================
	DestroyTask
====================================================================================
*/
void DestroyTask(task_t *task)
{
	DEBUGPRINT(DEBUG_TASK, "Task %d terminated!\n", task->taskID);

	delete(task->fn);
	task->fn = NULL;
	FreeTokenList(task->token_list);
	task->token_list = NULL;
	task->token_list_tail = NULL;
	task->buffer_list->numtasks--;
	/* Destroy buffers only when there are no tasks using this buffer list (numtasks = 0) */
	if (task->buffer_list->numtasks == 0)
		FreeBufferList(task->buffer_list, true);
	task->buffer_list = NULL;
	task->current_buffer = NULL;
	task->next_task = task->prev_task = NULL;
	free(task);
	task = NULL;
}



/*
####################################################################################
	Task Region Implementation
####################################################################################
*/



/*
====================================================================================
	TaskRegionInit
====================================================================================
*/
void TaskRegionInit()
{
	pthread_cond_init(&task_region.region_cond, NULL);
	pthread_mutex_init(&task_region.region_cond_lock, NULL);
}


/*
====================================================================================
	TaskRegionWait
====================================================================================
*/
//extern bool CheckIdleCount();

void TaskRegionWait()
{
	int status;

	//if (CheckIdleCount()) return;

	/* lock the queue */
	status = pthread_mutex_lock(&task_region.region_cond_lock);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionWait : Thread %d Error locking the queue lock!\n", GetThreadID());
		exit(1);
	}

	/* wait for all the task from the task region to finish */
	DEBUGPRINT(DEBUG_TASK, "TaskRegionWait : waiting ...\n");

	/* this condition is used to avoid missing the signal */
	status = pthread_cond_wait(&task_region.region_cond, &task_region.region_cond_lock);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionWait : Error on conditional wait!\n");
		exit(1);
	}

	DEBUGPRINT(DEBUG_TASK, "TaskRegionWait : continuing ...\n");

	/* release the lock */
	status = pthread_mutex_unlock(&task_region.region_cond_lock);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionWait : Error unlocking the queue lock!\n");
		exit(1);
	}
}

/*
====================================================================================
	TaskRegionDone
====================================================================================
*/
/* CAUTION: This function will be called when the thread pool is created
 * What happens with the signal that is sent, and how does this influence the execution */
void TaskRegionDone()
{
	int status;

	/* lock the queue */
	status = pthread_mutex_lock(&task_region.region_cond_lock);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionDone : Error locking the queue lock!\n");
		exit(1);
	}

	status = pthread_cond_signal(&task_region.region_cond);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionDone : Error on conditional signal!\n");
		exit(1);
	}

	DEBUGPRINT(DEBUG_TASK, "TaskRegionDone : signal ...\n");

	/* release the lock */
	status = pthread_mutex_unlock(&task_region.region_cond_lock);
	if (status != 0) {
		DEBUGPRINT(DEBUG_TASK, "TaskRegionDone : Error unlocking the queue lock!\n");
		exit(1);
	}
}




