#ifndef INTERNAL_HPP_
#define INTERNAL_HPP_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <pthread.h>
#include <cerrno>
#include <cassert>
#include <vector>
#include <atomic>
#include <sys/time.h>
//CUZZ4ADF
#include <queue>        // std::priority_queue
#include <algorithm>	// std::find
////////////

#define USE_SPIN_LOCK
#define USE_TEST_THREAD


#ifdef USE_SPIN_LOCK
  #define RUNTIME_LOCK_TYPE         pthread_spinlock_t
  #define RUNTIME_LOCK_INIT(a,b)    pthread_spin_init(a,b)
  #define RUNTIME_LOCK_DESTROY(a)   pthread_spin_destroy(a)
  #define RUNTIME_LOCK_ACQUIRE(a)   pthread_spin_lock(a)
  #define RUNTIME_LOCK_RELEASE(a)   pthread_spin_unlock(a)
#else
  #define RUNTIME_LOCK_TYPE         pthread_mutex_t
  #define RUNTIME_LOCK_INIT(a,b)    pthread_mutex_init(a,b)
  #define RUNTIME_LOCK_DESTROY(a)   pthread_mutex_destroy(a)
  #define RUNTIME_LOCK_ACQUIRE(a)   pthread_mutex_lock(a)
  #define RUNTIME_LOCK_RELEASE(a)   pthread_mutex_unlock(a)
#endif


#include "adf.h"

#include "common.h"
#include "debug.h"
#include "buffer.h"
#include "taskqueue.h"
#include "worksteal.h"
#include "threadpool.h"

//#include "cuzz4adf.h"
#include "testscheduler.h"

#include <unistd.h>





#endif /* INTERNAL_HPP_ */
