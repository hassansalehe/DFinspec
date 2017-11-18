#ifdef ADF
#include "internal.h"
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include "adf.h"
#endif


typedef struct time_stat_s {
	struct timeval thread_run;
	struct timeval dequeue_ready_task;
	struct timeval ready_queue_wait;
	struct timeval run_task;
	struct timeval task_body;
	struct timeval pass_tokens;
	struct timeval finalize_task;
	struct timeval consume_tokens;
	struct timeval enqueue_ready_task;
	struct timeval create_task;

	struct timeval thread_run_start;
	struct timeval dequeue_ready_task_start;
	struct timeval ready_queue_wait_start;
	struct timeval run_task_start;
	struct timeval task_body_start;
	struct timeval pass_tokens_start;
	struct timeval finalize_task_start;
	struct timeval consume_tokens_start;
	struct timeval enqueue_ready_task_start;
	struct timeval create_task_start;

	struct timeval thread_run_stop;
	struct timeval dequeue_ready_task_stop;
	struct timeval ready_queue_wait_stop;
	struct timeval run_task_stop;
	struct timeval task_body_stop;
	struct timeval pass_tokens_stop;
	struct timeval finalize_task_stop;
	struct timeval consume_tokens_stop;
	struct timeval enqueue_ready_task_stop;
	struct timeval create_task_stop;
} time_stat_t;

typedef struct event_stat_s {
	long task_exec;
	long tm_exec;
} event_stat_t;

time_stat_t  *time_stats;
event_stat_t *event_stats;
static int   num_threads;

struct timeval startTime;
struct timeval endTime;


/*
###########################################################################################
	timeval helper functions
###########################################################################################
*/

/*
====================================================================================
	timeval_subtract
====================================================================================
*/
int timeval_subtract(timeval *result, timeval *x, timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/*
====================================================================================
	timeval_add
====================================================================================
*/
void timeval_add(timeval *result, timeval *x, timeval *y)
{
	long usec = x->tv_usec + y->tv_usec;
	if (usec < 1000000) {
		result->tv_sec = x->tv_sec + y->tv_sec;
		result->tv_usec = usec;
	}
	else {
		/* add carry to time in seconds */
		result->tv_sec = x->tv_sec + y->tv_sec + 1;
		result->tv_usec = usec - 1000000;
	}
}

/*
====================================================================================
	get_msec
====================================================================================
*/
double get_msec(timeval t)
{
	double msec = t.tv_sec * 1000.0 + t.tv_usec / 1000.0;
	return msec;
}

/*
====================================================================================
	timeval_max
====================================================================================
*/
void timeval_max(timeval *oldt, timeval *newt)
{
	if ( get_msec(*newt) > get_msec(*oldt) ) {
		oldt->tv_sec = newt->tv_sec;
		oldt->tv_usec = newt->tv_usec;
	}
}


/*
###########################################################################################
	Timing statistics
###########################################################################################
*/

/*
====================================================================================
	InitStat
====================================================================================
*/
void InitStat(int numthreads)
{
	/* the first array object is the global statistics */
	num_threads = numthreads;
	time_stats = (time_stat_t *) calloc( (num_threads + 1), sizeof(time_stat_t));
	event_stats = (event_stat_t *) calloc( (num_threads + 1), sizeof(event_stat_t));

}

/*
====================================================================================
	FreeStat
====================================================================================
*/
void FreeStat()
{
	free(time_stats);
	free(event_stats);
}

/*
====================================================================================
	start_timer
====================================================================================
*/
void start_timer( enum time_event_e event, int threadID)
{
	int statID = threadID + 1;
	switch (event) {
	  case 0:
		  gettimeofday( & time_stats[statID].thread_run_start, NULL);
		  break;
	  case 1:
		  gettimeofday( & time_stats[statID].dequeue_ready_task_start, NULL);
		  break;
	  case 2:
		  gettimeofday( & time_stats[statID].ready_queue_wait_start, NULL);
		  break;
	  case 3:
		  gettimeofday( & time_stats[statID].run_task_start, NULL);
		  break;
	  case 4:
		  gettimeofday( & time_stats[statID].task_body_start, NULL);
		  break;
	  case 5:
		  gettimeofday( & time_stats[statID].pass_tokens_start, NULL);
		  break;
	  case 6:
		  gettimeofday( & time_stats[statID].finalize_task_start, NULL);
		  break;
	  case 7:
		  gettimeofday( & time_stats[statID].consume_tokens_start, NULL);
		  break;
	  case 8:
		  gettimeofday( & time_stats[statID].enqueue_ready_task_start, NULL);
		  break;
	  case 9:
		  gettimeofday( & time_stats[statID].create_task_start, NULL);
		  break;
	  default:
		  printf("Non-existing statistical event ... terminating!\n");
		  exit(1);
	}

}

/*
====================================================================================
	stop_timer
====================================================================================
*/
void stop_timer( enum time_event_e event, int threadID)
{
	int statID = threadID + 1;
	switch (event) {
	  case 0:
		  gettimeofday( & time_stats[statID].thread_run_stop, NULL);
		  timeval_subtract( & time_stats[statID].thread_run_stop,
				    & time_stats[statID].thread_run_stop,
				    & time_stats[statID].thread_run_start);
		  timeval_add( & time_stats[statID].thread_run,
			       & time_stats[statID].thread_run,
			       & time_stats[statID].thread_run_stop );
		  break;
	  case 1:
		  gettimeofday( & time_stats[statID].dequeue_ready_task_stop, NULL);
		  timeval_subtract( & time_stats[statID].dequeue_ready_task_stop,
				    & time_stats[statID].dequeue_ready_task_stop,
				    & time_stats[statID].dequeue_ready_task_start);
		  timeval_add( & time_stats[statID].dequeue_ready_task,
			       & time_stats[statID].dequeue_ready_task,
			       & time_stats[statID].dequeue_ready_task_stop );
		  break;
	  case 2:
		  gettimeofday( & time_stats[statID].ready_queue_wait_stop, NULL);
		  timeval_subtract( & time_stats[statID].ready_queue_wait_stop,
				    & time_stats[statID].ready_queue_wait_stop,
				    & time_stats[statID].ready_queue_wait_start);
		  timeval_add( & time_stats[statID].ready_queue_wait,
			       & time_stats[statID].ready_queue_wait,
			       & time_stats[statID].ready_queue_wait_stop );
		  break;
	  case 3:
		  gettimeofday( & time_stats[statID].run_task_stop, NULL);
		  timeval_subtract( & time_stats[statID].run_task_stop,
				    & time_stats[statID].run_task_stop,
				    & time_stats[statID].run_task_start);
		  timeval_add( & time_stats[statID].run_task,
			       & time_stats[statID].run_task,
			       & time_stats[statID].run_task_stop );
		  break;
	  case 4:
		  gettimeofday( & time_stats[statID].task_body_stop, NULL);
		  timeval_subtract( & time_stats[statID].task_body_stop,
				    & time_stats[statID].task_body_stop,
				    & time_stats[statID].task_body_start);
		  timeval_add( & time_stats[statID].task_body,
			       & time_stats[statID].task_body,
			       & time_stats[statID].task_body_stop );
		  break;
	  case 5:
		  gettimeofday( & time_stats[statID].pass_tokens_stop, NULL);
		  timeval_subtract( & time_stats[statID].pass_tokens_stop,
				    & time_stats[statID].pass_tokens_stop,
				    & time_stats[statID].pass_tokens_start);
		  timeval_add( & time_stats[statID].pass_tokens,
			       & time_stats[statID].pass_tokens,
			       & time_stats[statID].pass_tokens_stop );
		  break;
	  case 6:
		  gettimeofday( & time_stats[statID].finalize_task_stop, NULL);
		  timeval_subtract( & time_stats[statID].finalize_task_stop,
				    & time_stats[statID].finalize_task_stop,
				    & time_stats[statID].finalize_task_start);
		  timeval_add( & time_stats[statID].finalize_task,
			       & time_stats[statID].finalize_task,
			       & time_stats[statID].finalize_task_stop );
		  break;
	  case 7:
		  gettimeofday( & time_stats[statID].consume_tokens_stop, NULL);
		  timeval_subtract( & time_stats[statID].consume_tokens_stop,
				    & time_stats[statID].consume_tokens_stop,
				    & time_stats[statID].consume_tokens_start);
		  timeval_add( & time_stats[statID].consume_tokens,
			       & time_stats[statID].consume_tokens,
			       & time_stats[statID].consume_tokens_stop );
		  break;
	  case 8:
		  gettimeofday( & time_stats[statID].enqueue_ready_task_stop, NULL);
		  timeval_subtract( & time_stats[statID].enqueue_ready_task_stop,
				    & time_stats[statID].enqueue_ready_task_stop,
				    & time_stats[statID].enqueue_ready_task_start);
		  timeval_add( & time_stats[statID].enqueue_ready_task,
			       & time_stats[statID].enqueue_ready_task,
			       & time_stats[statID].enqueue_ready_task_stop );
		  break;
	  case 9:
		  gettimeofday( & time_stats[statID].create_task_stop, NULL);
		  timeval_subtract( & time_stats[statID].create_task_stop,
				    & time_stats[statID].create_task_stop,
				    & time_stats[statID].create_task_start);
		  timeval_add( & time_stats[statID].create_task,
			       & time_stats[statID].create_task,
			       & time_stats[statID].create_task_stop );
		  break;
	  default:
		  printf("Non-existing timing event ... terminating!\n");
		  exit(1);
	}

}

/*
====================================================================================
	PrintTiming
====================================================================================
*/
void PrintTiming(int statID)
{
	if (statID == 0)
		printf("\nGlobal timings per event:\n");
	else
		printf("\nThread %d timings per event:\n", statID - 1);
	printf("------------------------------\n");
	printf("Thread run:          %g\n", get_msec(time_stats[statID].thread_run));
	printf("Dequeue ready task:  %g\n", get_msec(time_stats[statID].dequeue_ready_task));
	printf("Ready queue wait:    %g\n", get_msec(time_stats[statID].ready_queue_wait));
	printf("Task run:            %g\n", get_msec(time_stats[statID].run_task));
	printf("Task body:           %g\n", get_msec(time_stats[statID].task_body));
	printf("Pass tokens:         %g\n", get_msec(time_stats[statID].pass_tokens));
	printf("Finalize task:       %g\n", get_msec(time_stats[statID].finalize_task));
	printf("Consume tokens:      %g\n", get_msec(time_stats[statID].consume_tokens));
	printf("Enqueue ready task:  %g\n", get_msec(time_stats[statID].enqueue_ready_task));
	printf("Create task:         %g\n", get_msec(time_stats[statID].create_task));
}

/*
====================================================================================
	PrintAverageTiming
====================================================================================
*/
void PrintAverageTiming()
{
	printf("\n\nAverage per-thread timings:\n");
	printf("------------------------------\n");
	printf("Thread run:          %g\n",
	      get_msec(time_stats[0].thread_run)/num_threads);
	printf("Dequeue ready task:  %g\n",
	      get_msec(time_stats[0].dequeue_ready_task)/num_threads);
	printf("Ready queue wait:    %g\n",
	      get_msec(time_stats[0].ready_queue_wait)/num_threads);
	printf("Task run:            %g\n",
	      get_msec(time_stats[0].run_task)/num_threads);
	printf("Task body:           %g\n",
	      get_msec(time_stats[0].task_body)/num_threads);
	printf("Pass tokens:         %g\n",
	      get_msec(time_stats[0].pass_tokens)/num_threads);
	printf("Finalize task:       %g\n",
	      get_msec(time_stats[0].finalize_task)/num_threads);
	printf("Consume tokens:      %g\n",
	      get_msec(time_stats[0].consume_tokens)/num_threads);
	printf("Enqueue ready task:  %g\n",
	      get_msec(time_stats[0].enqueue_ready_task)/num_threads);
	printf("Create task:         %g\n",
	      get_msec(time_stats[0].create_task)/num_threads);
}

/*
====================================================================================
	PrintMaxTiming
====================================================================================
*/
void PrintMaxTiming(time_stat_t *mt)
{
	printf("\n\nMaximum per-thread timings:\n");
	printf("------------------------------\n");
	printf("Thread run:          %g\n", get_msec(mt->thread_run));
	printf("Dequeue ready task:  %g\n", get_msec(mt->dequeue_ready_task));
	printf("Ready queue wait:    %g\n", get_msec(mt->ready_queue_wait));
	printf("Task run:            %g\n", get_msec(mt->run_task));
	printf("Task body:           %g\n", get_msec(mt->task_body));
	printf("Pass tokens:         %g\n", get_msec(mt->pass_tokens));
	printf("Finalize task:       %g\n", get_msec(mt->finalize_task));
	printf("Consume tokens:      %g\n", get_msec(mt->consume_tokens));
	printf("Enqueue ready task:  %g\n", get_msec(mt->enqueue_ready_task));
	printf("Create task:         %g\n", get_msec(mt->create_task));
}

/*
====================================================================================
	PrintThreadTiming
====================================================================================
*/
void PrintThreadTiming(int threadID)
{
	PrintTiming(threadID + 1);
}

/*
====================================================================================
	PrintAllThreadTiming
====================================================================================
*/
void PrintAllThreadTiming()
{
	int i;
	for (i=1; i<=num_threads; i++)
		PrintTiming(i);
}

/*
====================================================================================
	PrintGlobalTiming
====================================================================================
*/
void PrintGlobalTiming()
{
	int i;

	/* add thread timing values to a global sum */
	for (i=1; i<=num_threads; i++) {
		timeval_add( &time_stats[0].thread_run,
			     &time_stats[0].thread_run,
			     &time_stats[i].thread_run);

		timeval_add( &time_stats[0].dequeue_ready_task,
			     &time_stats[0].dequeue_ready_task,
			     &time_stats[i].dequeue_ready_task);

		timeval_add( &time_stats[0].ready_queue_wait,
			     &time_stats[0].ready_queue_wait,
			     &time_stats[i].ready_queue_wait);

		timeval_add( &time_stats[0].run_task,
			     &time_stats[0].run_task,
			     &time_stats[i].run_task);

		timeval_add( &time_stats[0].task_body,
			     &time_stats[0].task_body,
			     &time_stats[i].task_body);

		timeval_add( &time_stats[0].pass_tokens,
			     &time_stats[0].pass_tokens,
			     &time_stats[i].pass_tokens);

		timeval_add( &time_stats[0].finalize_task,
			     &time_stats[0].finalize_task,
			     &time_stats[i].finalize_task);

		timeval_add( &time_stats[0].consume_tokens,
			     &time_stats[0].consume_tokens,
			     &time_stats[i].consume_tokens);

		timeval_add( &time_stats[0].enqueue_ready_task,
			     &time_stats[0].enqueue_ready_task,
			     &time_stats[i].enqueue_ready_task);

		timeval_add( &time_stats[0].create_task,
			     &time_stats[0].create_task,
			     &time_stats[i].create_task);

	}
	PrintTiming(0);
	PrintAverageTiming();

	/* get the maximum values for timings */
	time_stat_t max_times;
	memset( &max_times, 0, sizeof(max_times));
	for (i=1; i<=num_threads; i++) {
		timeval_max( &max_times.thread_run, &time_stats[i].thread_run );
		timeval_max( &max_times.dequeue_ready_task, &time_stats[i].dequeue_ready_task );
		timeval_max( &max_times.ready_queue_wait, &time_stats[i].ready_queue_wait );
		timeval_max( &max_times.run_task, &time_stats[i].run_task );
		timeval_max( &max_times.task_body, &time_stats[i].task_body );
		timeval_max( &max_times.pass_tokens, &time_stats[i].pass_tokens );
		timeval_max( &max_times.finalize_task, &time_stats[i].finalize_task );
		timeval_max( &max_times.consume_tokens, &time_stats[i].consume_tokens );
		timeval_max( &max_times.enqueue_ready_task, &time_stats[i].enqueue_ready_task );
		timeval_max( &max_times.create_task, &time_stats[i].create_task );
	}
	PrintMaxTiming(&max_times);
}


/*
###########################################################################################
	ADF execution time
###########################################################################################
*/

/*
====================================================================================
	StartClock
====================================================================================
*/
void StartClock()
{
	gettimeofday(&startTime, NULL);
}

/*
====================================================================================
	StopClock
====================================================================================
*/
void StopClock()
{
	gettimeofday(&endTime, NULL);
}

/*
====================================================================================
	PrintTime
====================================================================================
*/
void PrintTime()
{
	struct timeval runtime;
	double mtime;	/* time in miliseconds */
	timeval_subtract(&runtime, &endTime, &startTime);
	mtime = (runtime.tv_sec * 1000000 + runtime.tv_usec) / 1000.0;
	printf("\nADF running time [s:ms:us]: %ld:%ld:%ld\t= %g ms\n\n",
	       runtime.tv_sec, runtime.tv_usec / 1000, runtime.tv_usec % 1000, mtime );
}


/*
###########################################################################################
	Counting statistics
###########################################################################################
*/

/*
====================================================================================
	count_event
====================================================================================
*/
void count_event( enum stat_event_e event, int threadID)
{
	int statID = threadID + 1;
	switch (event) {
	  case 0:
		  event_stats[statID].task_exec ++;
		  break;
	  case 1:
		  event_stats[statID].tm_exec ++;
		  break;
	  default:
		  printf("Non-existing statistical event ... terminating!\n");
		  exit(1);
	}
}

/*
====================================================================================
	PrintStats
====================================================================================
*/
void PrintStats(int statID)
{
	if (statID == 0)
		printf("\nGlobal statistics per event:\n");
	else
		printf("\nThread %d statistics per event:\n", statID - 1);
	printf("------------------------------\n");
	printf("Transactions:            %ld\n", event_stats[statID].tm_exec);
	printf("Tasks executed:          %ld\n", event_stats[statID].task_exec);
}

/*
====================================================================================
	PrintThreadStats
====================================================================================
*/
void PrintThreadStats(int threadID)
{
	PrintStats(threadID + 1);
}

/*
====================================================================================
	PrintGlobalStats
====================================================================================
*/
void PrintGlobalStats()
{
	int i;
	for (i=1; i<=num_threads; i++) {
		event_stats[0].task_exec = event_stats[0].task_exec + event_stats[i].task_exec;
		event_stats[0].tm_exec = event_stats[0].tm_exec + event_stats[i].tm_exec;
	}
	PrintStats(0);
}






/*
###########################################################################################
	TinySTM statistics
###########################################################################################
*/

#ifdef STM_STATISTICS

tinystm_statistics_t *stm_stat = NULL;


/*
====================================================================================
	InitSTMStats
====================================================================================
*/
void InitSTMStats()
{
	stm_stat = (tinystm_statistics_t *) calloc(num_threads+1, sizeof(tinystm_statistics_t));
}

/*
====================================================================================
	FreeSTMStat
====================================================================================
*/
void FreeSTMStat()
{
	free(stm_stat);
}

/*
====================================================================================
	STM_GetStatistics
====================================================================================
*/
void STM_GetStatistics(int threadID)
{
	tinystm_statistics_t *d = &stm_stat[threadID+1];
	stm_get_stats("nb_commits", &d->nb_commits);
	stm_get_stats("nb_aborts", &d->nb_aborts);
	stm_get_stats("nb_aborts_1", &d->nb_aborts_1);
	stm_get_stats("nb_aborts_2", &d->nb_aborts_2);
	stm_get_stats("nb_aborts_locked_read", &d->nb_aborts_locked_read);
	stm_get_stats("nb_aborts_locked_write", &d->nb_aborts_locked_write);
	stm_get_stats("nb_aborts_validate_read", &d->nb_aborts_validate_read);
	stm_get_stats("nb_aborts_validate_write", &d->nb_aborts_validate_write);
	stm_get_stats("nb_aborts_validate_commit", &d->nb_aborts_validate_commit);
	stm_get_stats("nb_aborts_invalid_memory", &d->nb_aborts_invalid_memory);
	stm_get_stats("nb_aborts_killed", &d->nb_aborts_killed);
	stm_get_stats("locked_reads_ok", &d->locked_reads_ok);
	stm_get_stats("locked_reads_failed", &d->locked_reads_failed);
	stm_get_stats("max_retries", &d->max_retries);
	d->nb_tx = d->nb_commits + d->nb_aborts;
	d->abort_rate = 100.0 * (double) d->nb_aborts / (double) d->nb_tx;
}

/*
====================================================================================
	PrintSTMStat
====================================================================================
*/
void PrintSTMStat(int i)
{
	printf("  #tx         : %lu\n", stm_stat[i].nb_tx);
	printf("  #commits    : %lu\n", stm_stat[i].nb_commits);
	printf("  #aborts     : %lu\n", stm_stat[i].nb_aborts);
	printf("  #abort rate : %.4g\n", stm_stat[i].abort_rate);
	printf("    #lock-r   : %lu\n", stm_stat[i].nb_aborts_locked_read);
	printf("    #lock-w   : %lu\n", stm_stat[i].nb_aborts_locked_write);
	printf("    #val-r    : %lu\n", stm_stat[i].nb_aborts_validate_read);
	printf("    #val-w    : %lu\n", stm_stat[i].nb_aborts_validate_write);
	printf("    #val-c    : %lu\n", stm_stat[i].nb_aborts_validate_commit);
	printf("    #inv-mem  : %lu\n", stm_stat[i].nb_aborts_invalid_memory);
	printf("    #killed   : %lu\n", stm_stat[i].nb_aborts_killed);
	printf("  #aborts>=1  : %lu\n", stm_stat[i].nb_aborts_1);
	printf("  #aborts>=2  : %lu\n", stm_stat[i].nb_aborts_2);
	printf("  #lr-ok      : %lu\n", stm_stat[i].locked_reads_ok);
	printf("  #lr-failed  : %lu\n", stm_stat[i].locked_reads_failed);
	printf("  Max retries : %lu\n", stm_stat[i].max_retries);
}

/*
====================================================================================
	PrintThreadSTMStats
====================================================================================
*/
void PrintThreadSTMStats()
{
	int i;
	for (i = 1; i <= num_threads; i++) {
		printf("\nThread %d STM statistics\n", i);
		printf("------------------------------\n\n");
		PrintSTMStat(i);
	}
}

/*
====================================================================================
	PrintGlobalSTMStats
====================================================================================
*/
void PrintGlobalSTMStats()
{
	int i;
	for (i = 1; i <= num_threads; i++) {
		stm_stat[0].nb_tx += stm_stat[i].nb_tx;
		stm_stat[0].nb_commits += stm_stat[i].nb_commits;
		stm_stat[0].nb_aborts += stm_stat[i].nb_aborts;
		stm_stat[0].nb_aborts_locked_read += stm_stat[i].nb_aborts_locked_read;
		stm_stat[0].nb_aborts_locked_write += stm_stat[i].nb_aborts_locked_write;
		stm_stat[0].nb_aborts_validate_read += stm_stat[i].nb_aborts_validate_read;
		stm_stat[0].nb_aborts_validate_write += stm_stat[i].nb_aborts_validate_write;
		stm_stat[0].nb_aborts_validate_commit += stm_stat[i].nb_aborts_validate_commit;
		stm_stat[0].nb_aborts_invalid_memory += stm_stat[i].nb_aborts_invalid_memory;
		stm_stat[0].nb_aborts_killed += stm_stat[i].nb_aborts_killed;
		stm_stat[0].nb_aborts_1 += stm_stat[i].nb_aborts_1;
		stm_stat[0].nb_aborts_2 += stm_stat[i].nb_aborts_2;
		stm_stat[0].locked_reads_ok += stm_stat[i].locked_reads_ok;
		stm_stat[0].locked_reads_failed += stm_stat[i].locked_reads_failed;
		stm_stat[0].max_retries += stm_stat[i].max_retries;
	}
	stm_stat[0].abort_rate = 100.0 * (double) stm_stat[0].nb_aborts / (double) stm_stat[0].nb_tx;

	printf("\nGlobal STM statistics\n");
	printf("------------------------------\n\n");
	PrintSTMStat(0);
}

#endif



