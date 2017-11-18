#ifndef __CUZZ4ADF_H__
#define __CUZZ4ADF_H__

/*
	cuzz4adf.h
*/

struct MyComparator {
  bool operator() (task_t* arg1, task_t* arg2) {
    return *arg1 < *arg2; //calls your operator
  }
};


extern std::priority_queue<task_t*, std::vector<task_t*>, MyComparator>    p_queue;
extern int                              p_change_num;

extern int          testnumthreads;                /* number of threads in the program run */
extern int          test_threads_go;
extern pthread_t    test_thread;                      /* the pool of threads */
extern bool         test_dataflow_execution;
extern long         test_max_tasks;

void NextPriority(task_t* in);
void TestCreateThread();
void *TestGetCurrentTask();
void TestBlockThread();
void TestReleaseThreads();
void TestDispatchTasks();
void TestScheduleTask(task_t *task);
void TestTerminate();

///////////////////////////////////////////////////////////////

#endif
