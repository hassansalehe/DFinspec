#ifndef __COMMON_H__
#define __COMMON_H__


#define NANOSEC(x)   (((double) (x)) / nanoticks)
#define MICROSEC(x)  (((double) (x)) / nanoticks) / 1000.0
#define MILISEC(x)   (((double) (x)) / nanoticks) / 1000000.0
#define TO_SEC(x)    (((double) (x)) / nanoticks) / 1000000000.0

extern double    nanoticks;

void set_alarm(int seconds);
int  check_alarm();

/* time should be in microseconds */
void Delay(long d_time);
void InitDelay();

void PrintError(const char *format, ...);
void PrintMsg(const char *format, ...);


#endif
