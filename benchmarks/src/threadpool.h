#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

/* maximum number of threads in the pool */
#define MAX_THREADS				200
#define DEFAULT_NUM_THREADS		1

extern int          numthreads;                /* number of threads in the program run */
extern int          threads_go;
extern pthread_t   *pool;                      /* the pool of threads */
extern bool         dataflow_execution;


void CreateThreadPool(int numthreads);
void *GetCurrentTask();
void BlockThreads();
void ReleaseThreads();
void DispatchTasks();
void ScheduleTask(task_t *task);
void Terminate();


#endif

