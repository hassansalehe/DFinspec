#ifndef __BUFFER_H__
#define __BUFFER_H__


using namespace std;

#define DEFAULT_BUFFER_CAPACITY 5000

/* forward declarations */
struct task_queue_s;
struct task_s;


typedef struct token_buffer_s {
	token_buffer_s       *next;
	int                   capacity;
	int                   size;
	RUNTIME_LOCK_TYPE     buffer_lock;
	token_t              *head;
	token_t              *tail;
	struct task_queue_s  *wait_queue;
} token_buffer_t;


typedef struct token_buffer_list_s {
	token_buffer_t   *buff;
	int               num_buffers;   /* number of buffers in the list */
	int               numtasks;      /* number of task instances that share this buffer list */
} token_buffer_list_t;


typedef struct map_buffer_list_s {
	struct map_buffer_list_s   *next;
	token_buffer_t             *buff;
} map_buffer_list_t;

/* Each task has one to one correspondence to buffer list, but there can be a number of task instances of the same task.
 * Therefore, when each instance is created we increase the numtasks value, and decrease it when an instance is destroyed.
 * When the value reaches 0, then we destroy the buffer list with associated buffers. */

#define USE_MAP
/* Buffer map - based on the token address */
#ifdef USE_MAP
  #include <map>
  typedef std::map<void *, map_buffer_list_t *> adf_map_t;
#else
  #include <unordered_map>
  typedef std::unordered_map<void *, map_buffer_list_t *> adf_map_t;
#endif

extern adf_map_t adf_map;

/* ==== Token functions ==== */

token_t *CreateToken(void *newvalue, int size);
token_t *CopyToken(token_t *src);
void FreeToken(token_t *token);
void FreeTokenList(token_t *list);


/* ==== Token buffer functions ==== */

token_buffer_t *CreateTokenBuffer(int cap);

/* These functions assume that the buffer is locked
   Since one thread could try to put the token into the buffer
   at the same time the other thread was trying to get a token,
   the first thread might put the token into the buffer
   while the other thread enqueues the task into the buffer waiting queue.
   Therefore we have to check the queue and put the token into the buffer
   in a single atomic action. */
void PutToken(token_buffer_t *buff, token_t *token);
token_t *GetToken(token_buffer_t *buff);

void PassToken2Buffer (token_buffer_t *buff, token_t *token);
bool ConsumeTokens(struct task_s *task);

/* This function only gives approximate buffer size
   for scheduling purposes. The buffer doesn't have to be locked. */
int GetBufferSize(token_buffer_t *buff);
void InsertBuffer(token_buffer_list_t *list, token_buffer_t *newbuff);
void DestroyBuffer(token_buffer_t *buff);
void FreeBufferList(token_buffer_list_t *list, bool destroy);


/* ==== Buffer map functions ==== */

/* Checks if the addr exists in the map.
 * If so, it gets the buffer list reference and use it to add a new buffer to the list.
 * if not, it creates a new buffer list and adds given buffer.
 * The buffer list is created dynamically and only once. */
void MapInsert(void *addr, token_buffer_t *buff);

/* Then Get function returns reference to the buffer list */
map_buffer_list_t *MapGetBufferList(void *addr);
void MapDestroy();


int CountBlockedTasks();

#endif
