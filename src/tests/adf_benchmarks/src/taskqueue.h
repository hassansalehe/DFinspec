#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__


enum task_status {
	waiting = 0,
	queued = 1,
	running = 2,
	ready = 3,
	stopped = 4,
	finished = 5,	//new status to be used by thread scheduler
	scheduled = 6	//new status to be used by thread scheduler	
};


typedef struct task_s {
	struct task_s        *prev_task;
	struct task_s        *next_task;

	int                   taskID;
	int                   thread_bind_id;    /* to which thread this task instance is bind to (-1 = no binding) */
	task_status           status;
	bool                  stop;              /* this is the flag to signal that until clause is true */

	int                   numtokens;
	token_t              *token_list;
	token_t              *token_list_tail;
	token_buffer_list_t  *buffer_list;
	token_buffer_t       *current_buffer;    /* this is the buffer of the token that we need */
	std::function <void (token_t *)>   *fn;
	
#ifdef USE_TEST_THREAD
	int priority;
	
	bool operator<(const task_s& rhs) const
    {
        return priority < rhs.priority;
    }
#endif

} task_t;

typedef struct task_queue_s {
	task_t             *qhead;
	task_t             *qtail;
	bool                empty;
	int                 size;
} task_queue_t;

typedef struct task_region_s {
	pthread_cond_t    region_cond;
	pthread_mutex_t   region_cond_lock;
} task_region_t;

extern task_region_t            task_region;
extern std::vector<task_t*>     task_list;

void InitQueue(task_queue_t *queue);
int  EnqueueTask(task_queue_t *queue, task_t *task);
int  DequeueTask(task_queue_t *queue, task_t **task);
void FreeQueue(task_queue_t *queue);

void RunTask(task_t *task);
void DestroyTask(task_t *task);
void DestroyTaskList();


void TaskRegionInit();
void TaskRegionWait();
void TaskRegionDone();


#endif


