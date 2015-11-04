#include "internal.h"


adf_map_t adf_map;

/*
####################################################################################
	Token Functions
####################################################################################
*/

/*
====================================================================================
	CreateToken
====================================================================================
*/
token_t *CreateToken(void *newvalue, int size)
{
	token_t *token = (token_t *) malloc(sizeof(token_t));
	token->next_token = NULL;
	token->size = (size_t) size;
	// TODO : Check how this should be done
	//token->value = newvalue;
	// or
	token->value = malloc((size_t) size);
	memcpy(token->value, newvalue, (size_t) size);

	return token;
}

/*
====================================================================================
	CopyToken
====================================================================================
*/
token_t *CopyToken(token_t *src)
{
	return CreateToken(src->value, (int) src->size);
}

/*
====================================================================================
	FreeToken
====================================================================================
*/
void FreeToken(token_t *token)
{
	token->next_token = NULL;
	free(token->value);
	token->value = NULL;
	free(token);
}

/*
====================================================================================
	FreeTokenList
====================================================================================
*/
void FreeTokenList(token_t *list)
{
	token_t *tmp = NULL;
	while (list != NULL) {
		tmp = list;
		list = list->next_token;
		FreeToken(tmp);
	}
}


/*
####################################################################################
	Buffer Functions
####################################################################################
*/

/*
====================================================================================
	CreateTokenBuffer
====================================================================================
*/
token_buffer_t *CreateTokenBuffer(int cap)
{
	int ret;
	token_buffer_t *buff = (token_buffer_t *) malloc(sizeof(token_buffer_t));

	buff->next = NULL;
	if (cap <= 0)
		buff->capacity = DEFAULT_BUFFER_CAPACITY;
	else
		buff->capacity = cap;
	buff->size = 0;
	buff->head = buff->tail = NULL;
	ret = RUNTIME_LOCK_INIT(&buff->buffer_lock, 0);
	if (ret) {
		PrintError(0, "\n\npthread_mutex_init() in CreateTokenBuffer() failed!\n");
		return NULL;
	}

	buff->wait_queue = (task_queue_t *) malloc(sizeof(task_queue_t));
	InitQueue(buff->wait_queue);

	DEBUGPRINT(DEBUG_TASK, "Creating queue at address %p\n", buff->wait_queue);

	return buff;
}

/*
====================================================================================
	PutToken
====================================================================================
*/
void PutToken(token_buffer_t *buff, token_t *token)
{
	assert((buff != NULL) && (token != NULL));

	if (buff->size == buff->capacity)
	{
		PrintError(0, "Buffer overflow in PutToken()!");
		return;
	}

	buff->size++;

	/* make sure the token is not linked anywhere */
	token->next_token = NULL;
	if (buff->head == NULL)
		buff->head = buff->tail = token;
	else {
		buff->tail->next_token = token;
		buff->tail = token;
	}
}

/*
====================================================================================
	GetToken
====================================================================================
*/
token_t *GetToken(token_buffer_t *buff)
{
	assert(buff != NULL);

	if (buff->head == NULL)
		return NULL;


	token_t *token = buff->head;
	if (buff->head != buff->tail)
		buff->head = buff->head->next_token;
	else
		buff->head = buff->tail = NULL;	/* this is the only token in the buffer */

	token->next_token = NULL;
	buff->size--;

	return token;
}

/*
====================================================================================
	PassToken2Buffer
====================================================================================

    First check if a task is blocked in the waiting queue for this token.
    If that is the case, pass the token directly to the task
    and check if the task can be enabled (all input tokens are consumed).

	When a token is produced, a thread checks
	associated waiting queue. If the task is found
	the thread passes the token to the task and
	moves it to a different waiting queue, or to
	a ready queue if all input data dependencies are
	satisfied for the task
*/
void PassToken2Buffer (token_buffer_t *buff, token_t *newtoken)
{
	/* TODO We have to lock the whole buffer so a queueu lock in this case might not be needed.
	 * Here, I am locking the buffer only until I put the token into the buffer
	 * or if I find a task to pass the token to.
	 * I am not sure if this matches the tokens correctly. */
	task_t   *task;

	TRACE_EVENT(7001,701)
	TRACE_EVENT(7002,702)
	TRACE_EVENT(7003,703)

	RUNTIME_LOCK_ACQUIRE(&buff->buffer_lock);
	TRACE_EVENT(7003,0)
	TRACE_EVENT(7004,704)
	DequeueTask(buff->wait_queue, &task);
	/* if there are no waiting tasks put the token into the buffer */
	
	if (task == NULL)
		PutToken(buff, newtoken);
	RUNTIME_LOCK_RELEASE(&buff->buffer_lock);

	TRACE_EVENT(7004,0)
	TRACE_EVENT(7002,0)
	TRACE_EVENT(7005,705)
	
	if (task != NULL) {
		/* We have found a waiting task.
		 * Pass it a token immediately - don't put it into the buffer.
		 * Put the token at the end of the token list */
		if (task->token_list != NULL) {
			task->token_list_tail->next_token = newtoken;
			task->token_list_tail = newtoken;
		}
		else
			/* this is the first token in the list */
			task->token_list = task->token_list_tail = newtoken;

		/* Check if the remaining input dependencies for the task are already satisfied */
		/* If the task becomes enabled, try to reserve it. If this fails enqueue it to the task queue */
		task->current_buffer = task->current_buffer->next;
		if (ConsumeTokens(task))
		#ifdef USE_TEST_THREAD
		{
			task->status = ready;
			t_scheduler->AddTask(task);
		}
		#else
			ScheduleTask(task);
		#endif
			
	}

	TRACE_EVENT(7005,0)
	TRACE_EVENT(7001,0)
}


/*
====================================================================================
	ConsumeTokens
====================================================================================
*/
/* Check if the input dependencies for the task are already satisfied */
bool ConsumeTokens(task_t *task)
{
	bool            enabled = true;
	token_t        *tmptoken = NULL;
	token_buffer_t *currbuff = NULL;

	TRACE_EVENT(7010,710)
	STAT_START_TIMER(consume_tokens, GetThreadID());

	currbuff = task->current_buffer;
	while (currbuff != NULL && enabled) {

		TRACE_EVENT(7011,711)
		TRACE_EVENT(7012,712)

		/* lock the buffer along with its waiting task queue */
		RUNTIME_LOCK_TYPE *lockref =  &task->current_buffer->buffer_lock;
		RUNTIME_LOCK_ACQUIRE(lockref);

		TRACE_EVENT(7012,0)
		TRACE_EVENT(7013,713)

		/* If there are tokens in the buffer get the first one and move to the next buffer;
		 * otherwise put the task into the buffer waiting queue */
		tmptoken = GetToken(task->current_buffer);
		if (tmptoken != NULL) {
			if (task->token_list == NULL)
				task->token_list = task->token_list_tail = tmptoken;
			else {
				task->token_list_tail->next_token = tmptoken;
				task->token_list_tail = tmptoken;
			}
			task->current_buffer = task->current_buffer->next;
			currbuff = task->current_buffer;
		}
		else {
			/* We haven't found the token in the buffer so
			 * we enqueue the task into the buffer queue */
			EnqueueTask(task->current_buffer->wait_queue, task);
			enabled = false;
		}

		RUNTIME_LOCK_RELEASE(lockref);

		TRACE_EVENT(7013,0)
		TRACE_EVENT(7011,0)
	}

	/* If all the task tokens are acquired the task is enabled */
	if (enabled == true)
		task->current_buffer = task->buffer_list->buff;

	STAT_STOP_TIMER(consume_tokens, GetThreadID());
	TRACE_EVENT(7010,0)

	return enabled;
}


/*
====================================================================================
	GetBufferSize
====================================================================================
*/
int GetBufferSize(token_buffer_t *buff)
{
	return buff->size;
}


/*
====================================================================================
	InsertBuffer
====================================================================================
*/
void InsertBuffer(token_buffer_list_t *list, token_buffer_t *newbuff)
{
	token_buffer_t *tmp;

	if (list->buff == NULL)
		list->buff = newbuff;
	else {
		tmp = list->buff;
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = newbuff;
	}
	list->num_buffers++;
}


/*
====================================================================================
	DestroyBuffer
====================================================================================
*/
void DestroyBuffer(token_buffer_t *buff)
{
	token_t *tmp;

	FreeQueue(buff->wait_queue);

	while (buff->head != NULL) {
		tmp = buff->head;
		buff->head = buff->head->next_token;
		FreeToken(tmp);
	}

	buff->next = NULL;
	buff->head = buff->tail = NULL;
	RUNTIME_LOCK_DESTROY(&buff->buffer_lock);
	free(buff);
}


/*
====================================================================================
	FreeBufferList
====================================================================================
*/
void FreeBufferList(token_buffer_list_t *list, bool destroy)
{
	token_buffer_t *tmp = NULL, *curr = NULL;

	/* if destroy == false don't touch buffers, only the reference to the list. */
	if (destroy == true) {
		curr = list->buff;
		while (curr != NULL) {
			tmp = curr;
			curr = curr->next;
			tmp->next = NULL;
			DestroyBuffer(tmp);
		}
	}

	list->buff = NULL;
	free(list);

}


/*
####################################################################################
	Buffer Map Functions
####################################################################################
*/

/* TODO I need to add MapRemove(*buff)
 * The reason is the following: when the task instance is destroyed, the corresponding
 * buffer list is also freed, along with associated buffers. If some later task creates
 * a token for some of these buffers, it will find the reference to the destroyed buffers
 * in this map. So we need to remove references to the destoyed buffers immidiately.
 * One way to do this is to save the address of the token when the buffer is created
 * by adding a new field 'void *token_addr' to the token_buffer_t structer.
 */

/*
====================================================================================
	MapInsert
====================================================================================
*/
void MapInsert(void *addr, token_buffer_t *buff)
{
	map_buffer_list_t *bufflist = NULL;

	if (adf_map.count(addr) > 0) { /* add a buffer to an existing buffer list */
		bufflist = (map_buffer_list_t *) adf_map.find(addr)->second;
		while (bufflist->next != NULL) bufflist = bufflist->next;
		bufflist->next = (map_buffer_list_t *) malloc(sizeof(token_buffer_list_t));
		bufflist = bufflist->next;
		bufflist->next = NULL;
		bufflist->buff = buff;
	}
	else { /* create a new buffer list and insert it into the map */
		bufflist = (map_buffer_list_t *) malloc(sizeof(token_buffer_list_t));
		bufflist->next = NULL;
		bufflist->buff = buff;
		adf_map.insert(std::pair<void *, map_buffer_list_t *> (addr, bufflist));
	}

}


/*
====================================================================================
	MapGetBufferList
====================================================================================
*/
map_buffer_list_t *MapGetBufferList(void *addr)
{
	if (adf_map.count(addr) > 0)
		return adf_map.find(addr)->second;
	else
		return NULL;
}


/*
====================================================================================
	MapDestroy
====================================================================================
*/
void MapDestroy()
{
	map_buffer_list_t *bufflist = NULL, *tmp = NULL;

	adf_map_t::iterator it;
	for (it = adf_map.begin(); it!= adf_map.end(); it++) {
		bufflist = it->second;
		while (bufflist != NULL) {
			tmp = bufflist;
			bufflist = bufflist->next;
			tmp->next = NULL;
			tmp->buff = NULL;
			free(tmp);
		}
	}
	adf_map.clear();
}


int CountBlockedTasks() {
	map_buffer_list_t *bufflist = NULL;
	int cnt = 0;

	adf_map_t::iterator it;
	for (it = adf_map.begin(); it!= adf_map.end(); it++) {
		bufflist = it->second;
		while (bufflist != NULL) {
			cnt += bufflist->buff->wait_queue->size;
			bufflist = bufflist->next;
		}
	}
	return cnt;
}


