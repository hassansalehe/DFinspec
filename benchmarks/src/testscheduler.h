#ifndef __TESTSCHEDULER_H__
#define __TESTSCHEDULER_H__

#include <iostream>       // std::cout
#include <queue>          // std::priority_queue
#include <set>         	  // std::set
#include <vector>     	  // std::vector
#include <functional>     // std::function
#include <algorithm>      // std::find


#define MAX_PRIORITY 10000		// n: upperbound on # of threads
#define MAX_STEP 4000			// k: upperbound on # of switch points
#define NUM_PCHANGE 2		// d: # of priority change points

//#define TESTPRINT(x) x
#define TESTPRINT(x) 

/*
 * Probabilistic Concurrency Testing (PCT) gives efficient probability 
 * of finding the bug with respect to
 * 		number of threads: n
 * 		number of steps/inst: k ==> for optimization issues this 
 * 									set to the number of sync. points 
 * 		bug depth: d
 * the probabilty of finding a bug at each run is:
 * 		1/n.k^d-1
 * 
 * In the case of ADF, 
 * 		n is the upperbound for the number of tasks 
 * 		k is the upperbound for the number of sync. point (TM_BEGIN, TM_END)
 * 		d is the number of priority change points
 * 
 * Each created task is assigned to a thread with a random priority and 
 * added to the threads set. If at creation time the task is ready it will
 * be added to the priority queue holding the enabled threads/tasks. 
 * At the beginning of the execution scheduler will choose randomly
 * d steps from possible priority change steps.
 * 
 * When execution started the scheduler will get top of the priority queue 
 * and start executing seeing a possible priority changing point(sync.
 * point). It will check if there is a priority change point assigned for
 * the current step. If so it will change the current threads priority 
 * and wake scheduler for assining a new thread to run. If there is no 
 * priority switch assigned it will continue executing till the end.
 * This will continue until all tasks are executed by assigned threads.
 * 		
*/
#include "internal.h"

/*
typedef struct token_s {
	struct token_s  *next_token;
	void            *value;
	int              size;
} token_t;

typedef struct task_s {
		
	int 			taskID;
	token_t			*token_list;
	task_status 	status;
	std::function <void (token_t *)>   *fn;
	
} task_t;

void RunTask(task_t* task);
*/
class TaskThread {
	
public:
	int 				threadID;
	int 				priority;	
	task_t* 			task;
	pthread_t 			thread;
	pthread_cond_t		cond;		//per thread condition variable
	pthread_mutex_t*	m_lock;		//thread scheduler mutex lock
	pthread_cond_t*		m_cond;		//thread scheduler cond. variable
	task_status			status;		//current status of thread
	
	//Static callback function for pthread_create
	static void* thread_func(void *pThisArg)
	{
		//This pointer is passed as an argument
		TaskThread *pThis = static_cast<TaskThread*>(pThisArg);
		pThis->RunTaskThread();
		return NULL;
	}
	
	//comparison operator task threads
	bool operator<(const TaskThread& rhs) const
    {
        return priority < rhs.priority;
    }
    
    TaskThread(task_t* _task, pthread_mutex_t* _m_lock, pthread_cond_t* _m_cond);
    
    //function for running the lambda function assigned to the task
    void RunTaskThread();
	
};

//Struct for comparing pointers to task threads used by std::priority_queue
struct threadptrcomparator {

	bool operator() (TaskThread* arg1, TaskThread* arg2) {
		return *arg1 < *arg2;
  }
};

class TaskScheduler {
	
public:
	
	std::priority_queue<TaskThread*, 
						std::vector<TaskThread*>,
						threadptrcomparator> p_queue;
					

	std::vector<TaskThread*>	executed_threads;
	std::set<TaskThread*> 		threads;						
	TaskThread* 				current_thread;		//current thread to run
	pthread_cond_t				main_cond;			//scheduler cond. variabl
	pthread_mutex_t				main_cond_lock;		//scheduler mutex lock
	int							pchange_count;		//number of priority change points
	std::set<int> 				given_priorities;	//set of given priorities to the threads
	std::set<int>				pchange_steps;		//set of given priority change points
	int 						current_step;		//current step
	
	TaskScheduler(int s_count = NUM_PCHANGE);
	
	void AddTask(task_t *newtask);
	void PushThread(TaskThread* t);
	void Push2PQueue(TaskThread* t);
	TaskThread* PopTaskThread();
	void AssignRandomPriority(TaskThread* t);
	void AssignRandomPChange();
	
	void SuspendCurrentThread();
	void RunCurrentThread();
	
	bool IsExecutionFinished();
	TaskThread* ContainsThread(task_t* t);
	void InitPQueue(std::vector<task_t*> t_list);
	void Terminate();
	
	void Start();
};

#endif
