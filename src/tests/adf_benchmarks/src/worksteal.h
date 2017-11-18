#ifndef __WORKSTEAL_H__
#define __WORKSTEAL_H__

typedef struct circular_array_s {
    int      log_size;
    task_t **segment;
} circular_array_t;

typedef struct worksteal_deque_atomic_s {
	task_t           *reserved_task;
    volatile long     bottom;
    atomic<long>      top;
    circular_array_t *active_array;
} worksteal_deque_atomic_t;

typedef struct worksteal_deque_lock_s {
    long             bottom;
    long             top;
    circular_array_t *active_array;
    pthread_mutex_t  dlock;
} worksteal_deque_lock_t;

typedef struct bind_queue_lock_s {
    long             bottom;
    long             top;
    circular_array_t *active_array;
    pthread_mutex_t  queue_lock;
} bind_queue_lock_t;

//#define DEQUE_LOCK


#ifdef DEQUE_LOCK
	typedef worksteal_deque_lock_t worksteal_deque_t;
#else /* atomic */
	typedef worksteal_deque_atomic_t worksteal_deque_t;
#endif /* atomic */

extern worksteal_deque_t *deque_list;

void InitDeque(worksteal_deque_t *deque, int logsize);
void DestroyDeque(worksteal_deque_t *deque);
void pushBottom(worksteal_deque_t *deque, task_t *task);
bool popBottom(worksteal_deque_t *deque, task_t **task);
bool steal(worksteal_deque_t *deque,task_t **task);

/* support for task reserving scheduler optimization */
bool ReserveTask(task_t *task);
bool GetReservedTask(int myID, task_t **task);


void CreateDequeList(int numthreads, int logsize);
void DestroyDequeList(int numthreads);



/* Bind Queue Support */
typedef bind_queue_lock_t bind_queue_t;
extern bind_queue_t *bind_queue_list;

void InitBindQueue_lock(bind_queue_lock_t *bqueue, int logsize);
void DestroyBindQueue_lock(bind_queue_lock_t *bqueue);
void BindQueuePush_lock(bind_queue_lock_t *bqueue, task_t *task);
bool BindQueuePop_lock(bind_queue_lock_t *bqueue, task_t **task);
bool IsBindQueueEmpty(bind_queue_lock_t *bqueue);

void CreateBindQueueList(int numthreads, int logsize);
void DestroyBindQueueList(int numthreads);


#endif

