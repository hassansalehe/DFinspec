#include "testscheduler.h"

/*
void RunTask(task_t* task)
{
	(*(task->fn))(NULL);
}
*/ 

TaskThread::TaskThread(task_t* _task, pthread_mutex_t* _m_lock, pthread_cond_t* _m_cond)
{
	priority = 0;
	task = _task;
	m_lock = _m_lock;
	m_cond = _m_cond;
	pthread_cond_init(&cond, NULL);
		
	pthread_create(&thread, NULL, thread_func, static_cast<void*>(this));	
}


/*
 * 
 * name: RunTaskThread
 * @param: none
 * @return: none
 * 
 * Function is used for pthread_create. Pthread will wait for the condition
 * variable until it is scheduled by the thread. Than execute the lambda
 * function assigned to the task. There is possiblity that it will be 
 * suspended and re-scheduled later during execution of RunTask. 
 * If it successfully finishes the execution it will change its status 
 * to finished. After finishing it will signal the scheduler thread to 
 * wake and continue execution. 
 * 
 */
void TaskThread::RunTaskThread()
{	
	pthread_mutex_lock(m_lock);
	while(status != scheduled)
	{
		status = waiting;
		pthread_cond_wait(&cond, m_lock);
	}
	pthread_mutex_unlock(m_lock);
	
	status = running;
	RunTask(task);
	
	if (!task->stop) {
		if (ConsumeTokens(task))
		{
			task->status = ready;
			t_scheduler->AddTask(task);
		}
	}
	else 
		task->status = stopped;
	
	status = finished;
	
	pthread_mutex_lock(m_lock);
	if(status == finished)
		pthread_cond_signal(m_cond);
	pthread_mutex_unlock(m_lock);
}

/*
========================
class TestScheduler
========================
*/

TaskScheduler::TaskScheduler(int s_count)
{
	current_thread = NULL;
	pthread_cond_init(&main_cond, NULL);
	pthread_mutex_init(&main_cond_lock, NULL);
	pchange_count = s_count;
	current_step = 0;
	AssignRandomPChange();
}

/*
 * 
 * name: 	AddTask
 * @param 	task_t *
 * @return	none
 * 
 * Function will create a new thread assigned for the given task newtask
 * and will assign a random priority for the thread. Later it will add 
 * the created thread to the threads vector. Also if the task is ready 
 * for execution it will added to the priority queue.
 *  
 */
void TaskScheduler::AddTask(task_t *newtask)
{
	TESTPRINT(printf("TestScheduler::AddTask(tID: %d)\n", newtask->taskID);)
	TaskThread* newThread = new TaskThread(newtask, &main_cond_lock, &main_cond);
	AssignRandomPriority(newThread);
	PushThread(newThread);
	TESTPRINT(printf("Thread p: %d tID: %d created\n", newThread->priority, newThread->task->taskID);)
	if(newtask->status == ready)
		Push2PQueue(newThread);
	/*
	TaskThread* temp = ContainsThread(newtask);
	if(temp == NULL)
	{
		TaskThread* newThread = new TaskThread(newtask, &main_cond_lock, &main_cond);
		//while(newThread->status != waiting)
		//{}
		AssignRandomPriority(newThread);
		PushThread(newThread);
		printf("New thread with %d priority and taskID %d created\n", newThread->priority, newThread->task->taskID);
		if(newtask->status == ready)
			Push2PQueue(newThread);
	}
	else
	{
		AssignRandomPriority(temp);
		if(newtask->status == ready)
			Push2PQueue(temp);
	}*/
}

/*
 * 
 * name: 	PushThread
 * @param 	TaskThread* 
 * @return 	none
 * 
 * Function will push back the input task thread into the threads vector
 * 
 */
void TaskScheduler::PushThread(TaskThread* t)
{
	threads.insert(t);
}

/*
 * 
 * name: 	Push2PQueue
 * @param 	TaskThread* 
 * @return 	none
 * 
 * Function will push back the input task thread to the priority queue
 * to be assigned for execution.
 * 
 */
void TaskScheduler::Push2PQueue(TaskThread* t)
{
	p_queue.push(t);
}


/*
 * 
 * name: 	PopTaskThread
 * @param 	none
 * @return	TaskThread* 
 * 
 * Function will pop and return the top of the priority queue to be scheduled. 
 * If the thread is suspended the task thread will be pushed back to the 
 * priority queue to be scheduled again.
 * 
 */
TaskThread* TaskScheduler::PopTaskThread()
{
	TaskThread * result = NULL;
	
	if(!p_queue.empty())
	{
		result = p_queue.top();
		p_queue.pop();
	}
	
	return result;
}



/*
 * 
 * name: 	AssignRandomPriority
 * @param	TaskThread*
 * @return	none
 * 
 * Function will assign a random priority for the input task thread.
 * It will use MAX_PRIORITY macro (corresponds to n) for the upper bound
 * for the priorities and do a check if the new random priority is given
 * to any previous thread
 * 
 */
void TaskScheduler::AssignRandomPriority(TaskThread* t)
{
	int rand_num = (rand() % MAX_PRIORITY) + NUM_PCHANGE;
	
	while(given_priorities.find(rand_num) != given_priorities.end())
		rand_num = (rand() % MAX_PRIORITY) + NUM_PCHANGE;

	given_priorities.insert(rand_num);
	
	t->priority = rand_num;
}

/*
 * 
 * name: 	AssignRandomPChange
 * @param	none
 * @return	none
 * 
 * Function for assigning random priority change points. It uses the macro
 * MAX_STEP (correponds to k) for choosing on which step to have a 
 * priority change.
 * 
 */
void TaskScheduler::AssignRandomPChange()
{
	int rand_num = 0;
	for(int i = 0; i < pchange_count; i++)
	{
		rand_num = rand()%MAX_STEP;
		while(pchange_steps.find(rand_num) != pchange_steps.end())
			rand_num = rand()%MAX_STEP;
		pchange_steps.insert(rand_num);
	}
	
	TESTPRINT(printf("pchange_step %d \n", rand_num););
}

/*
 * 
 * name: 	SuspendCurrentThread
 * @param	none
 * @return	none
 * 
 * Function for suspending the executing thread and waking scheduler 
 * thread for assigning a new thread for the execution. First, it will
 * assign a new priority number which is between 0-NUM_PCHANGE (corresponds
 * to d) and add it back to the priority queue. Later it will signal
 * the scheduler thread for continuing the execution with another thread.
 * After that it will change the status of the thread to waiting and will
 * suspend by waiting on the condition variable of the thread.
 * 
 */
void TaskScheduler::SuspendCurrentThread()
{
	current_thread->priority = pchange_count--;
	Push2PQueue(current_thread);
	
	pthread_mutex_lock(&main_cond_lock);
	pthread_cond_signal(&main_cond);
	current_thread->status = waiting;
	pthread_cond_wait(&current_thread->cond, &main_cond_lock);
	pthread_mutex_unlock(&main_cond_lock);

}

/*
 * 
 * name: 	RunCurrentThread
 * @param	none
 * @return	none
 * 
 * Function for starting the current thread running. It will change
 * the chosen threads status to scheduled and will signal the current
 * threads condition variable for waking it. Than it will wait on the 
 * scheduler condition variable for waiting until the current thread is
 * finished or suspended.
 * 
 */

void TaskScheduler::RunCurrentThread()
{
	
	pthread_mutex_lock(&main_cond_lock);
	
	current_thread->status = scheduled;
	pthread_cond_signal(&current_thread->cond);
	pthread_cond_wait(&main_cond, &main_cond_lock);

	pthread_mutex_unlock(&main_cond_lock); 

}



/*
 * 
 * name: 	IsExecutionFinished
 * @param	none
 * @return	bool
 * 
 * Function will go over each thread in the threads vector and return 
 * true if all threads are finished else it will return false.
 * 
 */
bool TaskScheduler::IsExecutionFinished()
{
	bool result = true;
	std::set<TaskThread*>::iterator it;
	
	for (it=threads.begin(); it!=threads.end(); ++it)
		if((*it)->status != finished)
			return false;
	
	return result;
}

TaskThread* TaskScheduler::ContainsThread(task_t* t)
{
	TaskThread* result = NULL;
	std::set<TaskThread*>::iterator it;
	
	for (it=threads.begin(); it!=threads.end(); ++it)
		if((*it)->task == t)
			return (*it);
	
	return result;
}

void TaskScheduler::InitPQueue(std::vector<task_t*> t_list)
{
	long numtasks = t_list.size();
	long i;
	
	for (i = 0; i < numtasks; i++) {
		if (t_list[i]->status == ready)
			AddTask(t_list[i]);
	}

}

void PrintVector(std::vector<TaskThread*> in)
{	
	std::vector<TaskThread*>::const_iterator i;
	printf("\n####### Execution Order #######\n");
	for( i = in.begin(); i != in.end(); ++i)
		printf("Thread-p:\t%d,\ttID:\t%d\n",(*i)->priority, (*i)->task->taskID);
	printf("\n###############################\n");
}

void TaskScheduler::Terminate()
{
	TESTPRINT(PrintVector(executed_threads);)
	TESTPRINT(printf("current_step = %d\n", current_step));
	MapDestroy();
	DestroyTaskList();
}


/*
 * 
 * name: 	Start
 * @param	none
 * @return	none
 * 
 * Function will start the scheduler and continue until all threads are
 * finished.
 * 
 */
void TaskScheduler::Start()
{
	TESTPRINT(printf("START()\n");)
	while(!p_queue.empty())
	{
		current_thread = PopTaskThread();
		TESTPRINT(printf("RunCurrentThread() p: %d, tID: %d\n", current_thread->priority, current_thread->task->taskID);)
		RunCurrentThread();	
		executed_threads.push_back(current_thread);
		TESTPRINT(printf("End RunCurrentThread()\n");)
	}
	TESTPRINT(printf("END START()\n");)
}
