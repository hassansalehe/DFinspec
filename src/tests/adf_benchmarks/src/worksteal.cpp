//#include "adf.h"
#include "internal.h"

worksteal_deque_t *deque_list;
bind_queue_t *bind_queue_list;

/*
====================================================================================
    Circular Array
====================================================================================
*/

circular_array_t *CreateCircularArray(int logsize) {
    circular_array_t *newarray = (circular_array_t*) malloc(sizeof(circular_array_t));
    newarray->log_size = logsize;
    newarray->segment = (task_t **) malloc((1<<logsize) * sizeof(task_t *));
    return newarray;
}

void DestroyCircularArray(circular_array_t *array) {
    free(array->segment);
    free(array);
}

static inline long GetSize(circular_array_t *array) {
    return 1<<array->log_size;
}

static inline task_t *GetTaskFromArray(circular_array_t *array, long index) {
    return array->segment[index % GetSize(array)];
}

static inline void PutTaskIntoArray(circular_array_t *array, long index, task_t *object) {
    array->segment[index % GetSize(array)] = object;
}

void GrowCircularArray(circular_array_t *array, long bottom, long top) {
    array->log_size++;
    task_t **new_segment = (task_t **) malloc((1<<array->log_size) * sizeof(task_t *));
    for (long i=top; i<bottom; i++) {
        new_segment[i] = GetTaskFromArray(array, i);
    }
    free(array->segment);
    array->segment = new_segment;
    new_segment = NULL;
    free(new_segment);
}

//void GrowCircularArray(circular_array_t *array, long bottom, long top) {
//	long i;
//	printf("GrowCircularArray\n");
//	for (i=0; i < 1<<array->log_size; i++) {
//		printf("%ld\t%p\n", i, GetTaskFromArray(array, i));
//	}
//
//	array->log_size++;
//	task_t **new_segment = (task_t **) malloc((1<<array->log_size) * sizeof(task_t *));
//	for (long i=top; i<bottom; i++) {
//		new_segment[i] = GetTaskFromArray(array, i);
//	}
//	free(array->segment);
//	array->segment = new_segment;
//	new_segment = NULL;
//	free(new_segment);
//
//	printf("\nafter the grow\n");
//	for (i=0; i < 1<<array->log_size; i++) {
//		printf("%ld\t%p\n", i, GetTaskFromArray(array, i));
//	}
//}

//void GrowCircularArray(circular_array_t *array, long bottom, long top) {
//	long i;
//	printf("GrowCircularArray\n");
//	for (i=0; i < 1<<array->log_size; i++) {
//		printf("%ld\t%p\n", i, GetTaskFromArray(array, i));
//	}
//
//	long newsize;
//    array->log_size++;
//    newsize = 1<<array->log_size;
//    task_t **new_segment = (task_t **) malloc(newsize * sizeof(task_t *));
//    for (long i=top; i<bottom; i++) {
//        new_segment[i % newsize] = GetTaskFromArray(array, i);
//    }
//    array->segment = new_segment;
//    free(array->segment);
//    new_segment = NULL;
//    free(new_segment);
//
//	printf("\nafter the grow\n");
//	for (i=0; i < 1<<array->log_size; i++) {
//		printf("%ld\t%p\n", i, GetTaskFromArray(array, i));
//	}
//}




/*
====================================================================================
    Reserve Task scheduler optimization support
====================================================================================
*/

/* A thread reserves a task when it enables it by producing the token
 * that satisfies the task dependency. Only one task per thread can be reserved. */

/* This function fails if the thread has reserved task. */
bool ReserveTask(task_t *task) {
#ifndef RESERVE_TASK
	return false;
#else
	bool success = true;
	if (deque_list[GetThreadID()].reserved_task == NULL)
		deque_list[GetThreadID()].reserved_task = task;
	else
		success = false;
	return success;
#endif
}

bool GetReservedTask(int myID, task_t **task) {
#ifndef RESERVE_TASK
	return false;
#else
	TRACE_EVENT(8004,84)
	*task =  deque_list[myID].reserved_task;
	deque_list[myID].reserved_task = NULL;
	TRACE_EVENT(8004,0)
	return (*task != NULL);
#endif
}



/*
====================================================================================
    WorkSteal Deque - Atomic
====================================================================================
*/


void InitDeque_atomic(worksteal_deque_atomic_t *deque, int logsize) {
	deque->reserved_task = NULL;
    deque->top = 0;
    deque->bottom = 0;
    deque->active_array = CreateCircularArray(logsize);
}

void DestroyDeque_atomic(worksteal_deque_atomic_t *deque) {
    DestroyCircularArray(deque->active_array );
    //free(deque);
}

/*
public void pushBottom(Object o) {
	long b = this.bottom;
	long t = this.top;
	CircularArray a = this.activeArray;
	long size = b - t;
	if (size >= a.size()-1) {
		a = a.grow(b, t);
		this.activeArray = a;
	}
	a.put(b, o);
	bottom = b+1;
}
*/

void pushBottom_atomic(worksteal_deque_atomic_t *deque, task_t *task) {
	TRACE_EVENT(8001,81)
    long b = deque->bottom;
    long t = deque->top.load(memory_order_relaxed);
    long size = b - t;
    circular_array_t *a = deque->active_array;

    if (size >= GetSize(a)-1) {
        GrowCircularArray(a, b, t);
    }

//    task->status = queued;
//    task->deque = (int) (deque - (worksteal_deque_atomic_t *) deque_list);
//    task->deque_pos = b;

    PutTaskIntoArray(a, b, task);
    asm volatile ("":::"memory");
    deque->bottom = b+1;
    TRACE_EVENT(8001,0)
}

/*
public Object popBottom() {
	long b = this.bottom;
	CircularArray a = this.activeArray;
	b = b - 1;
	this.bottom = b;
	long t = this.top;
	long size = b - t;
	if (size < 0) {
		bottom = t;
		return Empty;
	}
	Object o = a.get(b);
	if (size > 0)
		return o;
	if (! casTop(t, t+1))
		o = Empty;
	this.bottom = t+1;
	return o;
}
*/


bool popBottom_atomic(worksteal_deque_atomic_t *deque, task_t **task) {
	TRACE_EVENT(8002,82)
	long b = deque->bottom;
    circular_array_t *a = deque->active_array;
    b = b -1;
    deque->bottom = b;
    asm volatile ("":::"memory");
    long t = deque->top.load(memory_order_relaxed);
    long size = b - t;

    /* deque is empty */
    if (size < 0) {
        deque->bottom = t;
        *task = NULL;
        TRACE_EVENT(8002,0)
        return false;
    }

    *task = GetTaskFromArray(a, b);

    /* there are more than a single element in deque */
    if (size > 0) {
    	TRACE_EVENT(8002,0)
        return true;
    }

    /* this is the only element in deque.
     * if CAS fails somebody stole the task from us*/
    if (! deque->top.compare_exchange_strong(t, t+1)) {
        *task = NULL;
        deque->bottom = t; /* bottom = new_top */
        TRACE_EVENT(8002,0)
        return false;
    }
    deque->bottom = t + 1; /* bottom = new_top */

    TRACE_EVENT(8002,0)

    return true;
}


/*
public Object steal() {
	long t = this.top;
	long b = this.bottom;
	CircularArray a = this.activeArray;
	long size = b - t;
	if (size <= 0) return Empty;
	Object o = a.get(t);
	if (! casTop(t, t+1))
		return Abort;
	return o;
}
*/


bool steal_atomic(worksteal_deque_atomic_t *deque,task_t **task) {
	TRACE_EVENT(8003,83)
    long top = deque->top.load(memory_order_relaxed);
    asm volatile ("":::"memory");
    long bot = deque->bottom;
    long size = bot - top;

    circular_array_t *a = deque->active_array;

    if (size <= 0) {
    	*task = NULL;
    	TRACE_EVENT(8003,0)
    	return false;
    }

    *task = GetTaskFromArray(a, top);

    if (! deque->top.compare_exchange_strong(top, top+1)) {
    	*task = NULL;
    	TRACE_EVENT(8003,0)
        return false;
    }

    TRACE_EVENT(8003,0)

    return true;
}


/*
====================================================================================
    WorkSteal Deque - Lock
====================================================================================
*/



void InitDeque_lock(worksteal_deque_lock_t *deque, int logsize) {
    deque->top = 0;
    deque->bottom = 0;
    deque->active_array = CreateCircularArray(logsize);
    pthread_mutex_init(&deque->dlock, NULL);
}

void DestroyDeque_lock(worksteal_deque_lock_t *deque) {
	pthread_mutex_destroy(&deque->dlock);
    DestroyCircularArray(deque->active_array );
    //free(deque);
}

void pushBottom_lock(worksteal_deque_lock_t *deque, task_t *task) {
	pthread_mutex_lock(&deque->dlock);

    long size = deque->bottom - deque->top;;

    if (size >= GetSize(deque->active_array)-1) {
        GrowCircularArray(deque->active_array, deque->bottom, deque->top);
    }

    task->status = queued;

    PutTaskIntoArray(deque->active_array, deque->bottom, task);
    deque->bottom++;

    pthread_mutex_unlock(&deque->dlock);
}


bool popBottom_lock(worksteal_deque_lock_t *deque, task_t **task) {
	pthread_mutex_lock(&deque->dlock);

    long size;
    bool result = false;

    size = deque->bottom - deque->top;

    if (size > 0) {
		deque->bottom--;
		*task = GetTaskFromArray(deque->active_array, deque->bottom);
		result = true;
    }

	pthread_mutex_unlock(&deque->dlock);

	return result;
}

bool steal_lock(worksteal_deque_lock_t *deque,task_t **task) {
	pthread_mutex_lock(&deque->dlock);

	bool result = false;
    long size = deque->bottom - deque->top;

    if (size > 0) {
    	*task = GetTaskFromArray(deque->active_array, deque->top);
    	deque->top++;
    	result = true;
    }

	pthread_mutex_unlock(&deque->dlock);

	return result;
}



/*
====================================================================================
    WorkSteal Deque - Implementation choice
====================================================================================
*/

#ifdef DEQUE_LOCK

void InitDeque(worksteal_deque_t *deque, int logsize) {
	InitDeque_lock(deque, logsize);
}
void DestroyDeque(worksteal_deque_t *deque) {
	DestroyDeque_lock(deque);
}
void pushBottom(worksteal_deque_t *deque, task_t *task) {
	pushBottom_lock(deque, task);
}
bool popBottom(worksteal_deque_t *deque, task_t **task) {
	return popBottom_lock(deque, task);
}
bool steal(worksteal_deque_t *deque,task_t **task) {
	return steal_lock(deque, task);
}

#else /* atomic */

void InitDeque(worksteal_deque_t *deque, int logsize) {
	InitDeque_atomic(deque, logsize);
}
void DestroyDeque(worksteal_deque_t *deque) {
	DestroyDeque_atomic(deque);
}
void pushBottom(worksteal_deque_t *deque, task_t *task) {
	pushBottom_atomic(deque, task);
}
bool popBottom(worksteal_deque_t *deque, task_t **task) {
	return popBottom_atomic(deque, task);
}
bool steal(worksteal_deque_t *deque,task_t **task) {
	return steal_atomic(deque, task);
}

#endif /* atomic */




/*
====================================================================================
    Deque List
====================================================================================
*/

void CreateDequeList(int numthreads, int logsize) {
	deque_list = (worksteal_deque_t *) malloc(numthreads * sizeof(worksteal_deque_t));
	for (int i = 0; i < numthreads; i++)
		InitDeque(&deque_list[i], logsize);
}

void DestroyDequeList(int numthreads) {
	for (int i = 0; i < numthreads; i++)
		DestroyDeque(&deque_list[i]);
	free(deque_list);
}



/*
====================================================================================
    Bind Task List
====================================================================================
*/



void InitBindQueue_lock(bind_queue_lock_t *bqueue, int logsize) {
    bqueue->top = 0;
    bqueue->bottom = 0;
    bqueue->active_array = CreateCircularArray(logsize);
    pthread_mutex_init(&bqueue->queue_lock, NULL);
}

void DestroyBindQueue_lock(bind_queue_lock_t *bqueue) {
	pthread_mutex_destroy(&bqueue->queue_lock);
    DestroyCircularArray(bqueue->active_array );
}

void BindQueuePush_lock(bind_queue_lock_t *bqueue, task_t *task) {
	pthread_mutex_lock(&bqueue->queue_lock);

    long size = bqueue->bottom - bqueue->top;;

    if (size >= GetSize(bqueue->active_array)-1) {
        GrowCircularArray(bqueue->active_array, bqueue->bottom, bqueue->top);
    }

    task->status = queued;

    PutTaskIntoArray(bqueue->active_array, bqueue->bottom, task);
    bqueue->bottom++;

    pthread_mutex_unlock(&bqueue->queue_lock);
}


bool BindQueuePop_lock(bind_queue_lock_t *bqueue, task_t **task) {
	pthread_mutex_lock(&bqueue->queue_lock);

    long size;
    bool result = false;

    size = bqueue->bottom - bqueue->top;

    if (size > 0) {
    	*task = GetTaskFromArray(bqueue->active_array, bqueue->top);
    	bqueue->top++;
		result = true;
    }

	pthread_mutex_unlock(&bqueue->queue_lock);

	return result;
}

bool IsBindQueueEmpty(bind_queue_lock_t *bqueue) {
	pthread_mutex_lock(&bqueue->queue_lock);

    bool result = false;

    if (bqueue->top == bqueue->bottom)
    	result = true;

	pthread_mutex_unlock(&bqueue->queue_lock);

	return result;
}


void CreateBindQueueList(int numthreads, int logsize) {
	bind_queue_list = (bind_queue_t *) malloc(numthreads * sizeof(bind_queue_t));
	for (int i = 0; i < numthreads; i++)
		InitBindQueue_lock(&bind_queue_list[i], logsize);
}

void DestroyBindQueueList(int numthreads) {
	for (int i = 0; i < numthreads; i++)
		DestroyBindQueue_lock(&bind_queue_list[i]);
	free(bind_queue_list);
}



















