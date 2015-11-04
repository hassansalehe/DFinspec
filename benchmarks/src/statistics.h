#ifndef STATISTICS_H_
#define STATISTICS_H_

/*
###########################################################################################
	ADF Statistics
###########################################################################################
*/

//#define GET_TIMING_STAT
#define GET_COUNT_STAT
#if (defined(GET_TIMING_STAT) || defined(GET_COUNT_STAT))
  #define GET_STATISTICS
#endif

#if defined(GET_STATISTICS)
  #define STAT_INIT(nt)                  InitStat(nt);
  #define STAT_CLEAN                     FreeStat();
#else
  #define STAT_INIT(nt)
  #define STAT_CLEAN
#endif

#if defined(GET_TIMING_STAT)
  #define STAT_START_TIMER( event, tID)  start_timer(event, tID);
  #define STAT_STOP_TIMER( event, tID)   stop_timer(event, tID);
  #define PRINT_THREAD_TIME(tID)         PrintThreadTiming(tID);
  #define PRINT_TIME_PER_THREAD          PrintAllThreadTiming();
  #define PRINT_GLOBAL_TIME              PrintGlobalTiming();
#else
  #define STAT_START_TIMER( event, tID)
  #define STAT_STOP_TIMER( event, tID)
  #define PRINT_THREAD_TIME(tID)
  #define PRINT_TIME_PER_THREAD
  #define PRINT_GLOBAL_TIME
#endif

#if defined(GET_COUNT_STAT)
  #define STAT_COUNT_EVENT( event, tID)  count_event(event, tID);
  #define PRINT_THREAD_STAT(tID)         PrintThreadStats(tID);
  #define PRINT_GLOBAL_STAT              PrintGlobalStats();
#else
  #define STAT_COUNT_EVENT( event, tID)
  #define PRINT_THREAD_STAT(tID)
  #define PRINT_GLOBAL_STAT
#endif

enum time_event_e {
	thread_run = 0,
	dequeue_ready_task,
	ready_queue_wait,
	run_task,
	task_body,
	pass_tokens,
	finalize_task,
	consume_tokens,
	enqueue_ready_task,
	create_task
};

enum stat_event_e {
	task_exec = 0,
	tm_exec = 1
};


void start_timer( enum time_event_e event, int threadID);
void stop_timer( enum time_event_e event, int threadID);

void InitStat(int numthreads);
void FreeStat();
void PrintThreadTiming(int threadID);
void PrintAllThreadTiming();
void PrintGlobalTiming();

void StartClock();
void StopClock();
void PrintTime();

void count_event( enum stat_event_e event, int threadID);
void PrintThreadStats(int threadID);
void PrintGlobalStats();



#endif /* STATISTICS_H_ */
