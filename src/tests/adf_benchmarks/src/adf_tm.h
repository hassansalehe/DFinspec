#ifndef ADF_TM_H_
#define ADF_TM_H_


//#define TINY_STM

/*
###########################################################################################
	Synchronization
###########################################################################################
*/

#if defined(ADF) || defined(OPENMP)


/* ------------------------------- Global lock ------------------------------- */
#ifdef OPENMP
  #define LOCK                          omp_set_lock(&glock);
  #define UNLOCK                        omp_unset_lock(&glock);
  #define LOCK_INIT                     omp_init_lock(&glock);
  #define LOCK_DESTROY                  omp_destroy_lock(&glock);
#else
  #define LOCK                          pthread_mutex_lock(&glock);
  #define UNLOCK                        pthread_mutex_unlock(&glock);
  #define LOCK_INIT                     pthread_mutex_init(&glock, NULL);
  #define LOCK_DESTROY                  pthread_mutex_destroy(&glock);
#endif


#ifndef ADF_STM
  #define TRANSACTION_BEGIN(id,ro)        LOCK
  #define TRANSACTION_END                 UNLOCK
  #define TRANSACTION_COMMIT              UNLOCK
  #define TRANSACTION_ABORT               UNLOCK
  #define TRANSACTION_BLOCK_END
  #define STM_INIT                        LOCK_INIT
  #define STM_EXIT                        LOCK_DESTROY
  #define TMREAD(x)                       x
  #define TMREADLONG(x)                   x
  #define TMREADPTR(x)                    x
  #define TMREADMEM(addr, buff)           buff = addr
  #define TMWRITE(x, value)               x=value
  #define TMWRITELONG(x, value)           x=value
  #define TMWRITEPTR(x, value)            x=value
  #define TMWRITEMEM(addr, buff)          addr = buff
  #define STM_INIT_THREAD
  #define STM_EXIT_THREAD

  #define TRANSACTION_SAFE
  #define TRANSACTION_UNSAFE
  #define TRANSACTION_CALLABLE
  #define TRANSACTION_PURE


/* ------------------------------- Transactional memory ------------------------------- */


#elif defined(TINY_STM)    /* TinySTM */

  extern "C" {
  #include <atomic_ops.h>
  #include "stm.h"
  #include "wrappers.h"
  }

  //#define STM_STATISTICS

  /*
  * Useful macros to work with transactions. Note that, to use nested
  * transactions, one should check the environment returned by
  * stm_get_env() and only call sigsetjmp() if it is not null.
  */
  #define RO                              1
  #define RW                              0
  #define START(id, ro)                   { stm_tx_attr_t _a = {id, ro};             \
                                            sigjmp_buf *_e = stm_start(&_a);         \
                                            int ret = 0;                             \
                                            if (_e != NULL) ret = sigsetjmp(*_e, 0); \
					    {
//                                             if (ret != STM_ABORT_EXPLICIT) {
  #define LOAD(addr)                      stm_load((stm_word_t *)addr)
  #define LOADMEM(addr, buff, size)       stm_load_bytes((uint8_t *) addr, (uint8_t *) buff, (size_t) size)
  #define STORE(addr, value)              stm_store((stm_word_t *)addr, (stm_word_t)value)
  #define STOREMEM(addr, buff, size)      stm_store_bytes((uint8_t *) addr, (uint8_t *) buff, (size_t) size)
  #define COMMIT                          stm_commit();
  #define ABORT                           stm_abort(0);

  #define TRANSACTION_BEGIN(id,ro)        START(id,ro)
  #define TRANSACTION_END                 COMMIT }}
  #define TRANSACTION_COMMIT              COMMIT
  #define TRANSACTION_ABORT               ABORT
  #define TRANSACTION_BLOCK_END           }}
  #define STM_INIT                        stm_init();
  #define STM_EXIT                        stm_exit();
  #define STM_INIT_THREAD                 stm_init_thread();
  #define STM_EXIT_THREAD                 stm_exit_thread();
  #define TMREAD(x)                       LOAD(&x)
  #define TMREADLONG(x)                   stm_load_long(&x)
  #define TMREADPTR(x)                    stm_load_ptr((volatile void **) &x)
  #define TMREADMEM(addr, buff)           LOADMEM(&addr, &buff, sizeof(buff))
  #define TMWRITE(x, value)               STORE(&x, value)
  #define TMWRITELONG(x, value)           stm_store_long(&x, value)
  #define TMWRITEPTR(x, value)            stm_store_ptr((volatile void **) &x, (void *) value)
  #define TMWRITEMEM(addr, buff)          STOREMEM(&addr, &buff, sizeof(buff))

  #define TRANSACTION_SAFE
  #define TRANSACTION_UNSAFE
  #define TRANSACTION_CALLABLE
  #define TRANSACTION_PURE

#else /* GCC_ITM */
  #define TRANSACTION_BEGIN(id,ro)        /*CheckSwitchPoint(t_scheduler);__transaction_atomic*/ {
  #define TRANSACTION_END                 } /*CheckSwitchPoint(t_scheduler)*/;
  #define TRANSACTION_COMMIT
  #define TRANSACTION_ABORT
  #define TRANSACTION_BLOCK_END
  #define STM_INIT
  #define STM_EXIT
  #define TMREAD(x)                       x
  #define TMREADLONG(x)                   x
  #define TMREADPTR(x)                    x
  #define TMREADMEM(addr, buff)           buff = addr
  #define TMWRITE(x, value)               x=value
  #define TMWRITELONG(x, value)           x=value
  #define TMWRITEPTR(x, value)            x=value
  #define TMWRITEMEM(addr, buff)          addr = buff
  #define STM_INIT_THREAD
  #define STM_EXIT_THREAD

  #define TRANSACTION_SAFE                __attribute__((transaction_safe))
  #define TRANSACTION_UNSAFE              __attribute__((transaction_unsafe))
  #define TRANSACTION_CALLABLE            __attribute__((transaction_callable))
  #define TRANSACTION_PURE                __attribute__((transaction_pure))


#endif /* ADF_STM */

/* ------------------------------- Sequential ------------------------------- */
#else

  #define TRANSACTION_BEGIN(id,ro)
  #define TRANSACTION_END
  #define TRANSACTION_COMMIT
  #define TRANSACTION_ABORT
  #define TRANSACTION_BLOCK_END
  #define STM_INIT
  #define STM_EXIT
  #define TMREAD(x)
  #define TMREADLONG(x)
  #define TMREADPTR(x)
  #define TMREADMEM(addr, buff)
  #define TMWRITE(x, value)
  #define TMWRITELONG(x, value)
  #define TMWRITEPTR(x, value)
  #define TMWRITEMEM(addr, buff)
  #define STM_INIT_THREAD
  #define STM_EXIT_THREAD
  #define TRANSACTION_SAFE
  #define TRANSACTION_UNSAFE
  #define TRANSACTION_CALLABLE
  #define TRANSACTION_PURE

#endif /* end sequential */


/*
###########################################################################################
	TinySTM statistics
###########################################################################################
*/

#if defined(ADF_STM)

typedef struct tinystm_statistics_s {
  	unsigned long nb_tx;
  	unsigned long nb_commits;
  	unsigned long nb_aborts;
  	double        abort_rate;
  	unsigned long nb_aborts_1;
  	unsigned long nb_aborts_2;
  	unsigned long nb_aborts_locked_read;
  	unsigned long nb_aborts_locked_write;
  	unsigned long nb_aborts_validate_read;
  	unsigned long nb_aborts_validate_write;
  	unsigned long nb_aborts_validate_commit;
  	unsigned long nb_aborts_invalid_memory;
  	unsigned long nb_aborts_killed;
  	unsigned long locked_reads_ok;
  	unsigned long locked_reads_failed;
  	unsigned long max_retries;
} tinystm_statistics_t;


void InitSTMStats();
void STM_GetStatistics(int threadID);
void PrintThreadSTMStats();
void PrintGlobalSTMStats();
void FreeSTMStat();

#endif

#if defined(STM_STATISTICS)
  #define STM_STAT_INIT              InitSTMStats();
  #define GET_STM_STAT(tID)          STM_GetStatistics(tID);
  #define PRINT_THREAD_STM_STAT      PrintThreadSTMStats();
  #define PRINT_GLOBAL_STM_STAT      PrintGlobalSTMStats();
  #define STM_STAT_CLEAN             FreeSTMStat();
#else
  #define STM_STAT_INIT
  #define GET_STM_STAT(tID)
  #define PRINT_THREAD_STM_STAT
  #define PRINT_GLOBAL_STM_STAT
  #define STM_STAT_CLEAN
#endif


#endif /* ADF_TM_H_ */
