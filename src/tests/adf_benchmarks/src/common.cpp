#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <stdint.h>
#include <cstring>
#include <csignal>
#include <unistd.h>

#include "common.h"


/*
###########################################################################################
	Support for timed delays
###########################################################################################
*/


double    nanoticks;    /* CPU ticks per nanosecond */
uint64_t  d_clock;      /* Number of clocks for 1000 long integer additions */
double    d_nanosec;    /* Time in nanoseconds for 1000 long integer additions */


/*
================
   rdtsc
================
*/
inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    /*
     * cpuid will serialize the following rdtsc with respect to all other
     * instructions the processor may be handling.
     */
    __asm__ __volatile__ (
            "xorl %%eax, %%eax\n"
            "cpuid\n"
            "rdtsc\n"
            : "=a" (lo), "=d" (hi)
            :
            : "%ebx", "%ecx");
    return (uint64_t)hi << 32 | lo;
}

// static inline void pin_thread(int n) {
//     cpu_set_t mask;
//     CPU_ZERO(&mask);
//     CPU_SET(n, &mask);
//     sched_setaffinity(0, sizeof(mask), &mask);
// }


double CPUTicksPerNanosecond()
{
    FILE    *cpuinfo = NULL;
    int      pos;
    char     line [100] = {0};
    char     strvalue[9];
    double   ticks = -1.0;

    cpuinfo = fopen ("/proc/cpuinfo" , "r");
    if (cpuinfo == NULL) perror ("Error opening file");
    else
    {
        while ( ! feof (cpuinfo) )
        {
            if ( fgets (line , sizeof(line) , cpuinfo) != NULL ) {
                if (strstr(line, "MHz") != NULL) {
                    pos = strlen(line) - 9;
                    memcpy(strvalue, (void *) &line[pos], 9);
                    ticks = atof(strvalue)/1000.0;
                }
            }
        }
        fclose (cpuinfo);
    }
    return ticks;
}

void MeasureLoop()
{
    long i;
    d_clock = rdtsc();
    for (i=0; i<10000; i++) { asm volatile(""); }
    d_clock = rdtsc() - d_clock;
    d_nanosec = NANOSEC(d_clock);
}

/* time should be in microseconds */
void Delay(long d_time)
{
    long i;
    long count = 10000.0 * ((1000.0 * d_time) / d_nanosec );
    for (i=0; i<count; i++) { asm volatile(""); }
}

void InitDelay()
{
    /* pin the thread to core 1 to measure the loop time */
//     cpu_set_t mask;
//     CPU_ZERO(&mask);
//     CPU_SET(1, &mask);
//     sched_setaffinity(0, sizeof(mask), &mask);

    nanoticks = CPUTicksPerNanosecond();
    MeasureLoop();

    /* unpin the thread */
//     CPU_ZERO(&mask);
//     sched_setaffinity(0, sizeof(mask), &mask);
}


/*
###########################################################################################
	timeval helper functions
###########################################################################################
*/
/* Use only for sequential processes (not multithreaded)
 * Never mix with sleep() function in the same process  */

int alarm_flag;

/* The signal handler just clears the flag and sets default action for SIGALRM */
static void alarm_handler(int signo)
{
	alarm_flag = 0;
	signal(SIGALRM, SIG_DFL);
}

void set_alarm(int seconds)
{
	/* set alarm flag */
	alarm_flag = 1;

	/* cancel pending alarm request, if any */
	alarm(0);

	/* Establish a handler for SIGALRM signals.  */
	signal(SIGALRM, alarm_handler);

	/* Set an alarm to go off after a given number of seconds.  */
	alarm (seconds);
}

int check_alarm()
{
	return alarm_flag;
}

/*
###########################################################################################
	Debug printing
###########################################################################################
*/

void PrintError(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	fprintf(stderr, format, args);
	va_end (args);
}

void PrintMsg(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	printf(format, args);
	va_end (args);
}
