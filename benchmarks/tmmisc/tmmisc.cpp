/*
QuakeTM
Transactional implementation of the original Quake game
Copyright (C) 2008 Barcelona Supercomputing Center
Author: Vladimir Gajinov
*/

#include "tmmisc.h"


/*
===============================
Function from ctype.c
===============================
*/

TM_PURE
int TM_tolower(int x)
{
	return tolower(x);
}


/*
===============================
Function from string.c
===============================
*/

TM_CALLABLE
char * TM_strcpy(char * dest,const char *src)
{
  char *tmp = dest;

  while ((*dest++ = *src++) != '\0')
    /* nothing */;
  return tmp;
}

TM_CALLABLE
char * TM_strncpy(char * dest,const char *src,size_t count)
{
	char *tmp = dest;

	while (count-- && (*dest++ = *src++) != '\0')
		/* nothing */;

	return tmp;
}

TM_CALLABLE
char * TM_strcat(char * dest, const char * src)
{
  char *tmp = dest;

  while (*dest)
    dest++;
  while ((*dest++ = *src++) != '\0')
    ;

  return tmp;
}

TM_CALLABLE
char * TM_strncat(char *dest, const char *src, size_t count)
{
  char *tmp = dest;

  if (count) {
    while (*dest)
      dest++;
    while ((*dest++ = *src++)) {
      if (--count == 0) {
        *dest = '\0';
        break;
      }
    }
  }

  return tmp;
}

TM_CALLABLE
int TM_strcmp(const char * cs,const char * ct)
{
	register signed char __res;

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}

	return __res;
}

TM_CALLABLE
int TM_strncmp(const char * cs,const char * ct,size_t count)
{
  register signed char __res = 0;

  while (count) {
    if ((__res = *cs - *ct++) != 0 || !*cs++)
      break;
    count--;
  }

  return __res;
}

TM_CALLABLE
size_t TM_strlen(const char * s)
{
  const char *sc;

  for (sc = s; *sc != '\0'; ++sc)
    /* nothing */;
  return sc - s;
}

TM_CALLABLE
size_t TM_strnlen(const char * s, size_t count)
{
  const char *sc;

  for (sc = s; count-- && *sc != '\0'; ++sc)
    /* nothing */;
  return sc - s;
}

TM_CALLABLE
void * TM_memset(void * s,int c,size_t count)
{
  char *xs = (char *) s;

  while (count--)
    *xs++ = c;

  return s;
}

TM_CALLABLE
void * TM_memcpy(void * dest,const void *src,size_t count)
{
  char *tmp = (char *) dest, *s = (char *) src;

  while (count--)
    *tmp++ = *s++;

  return dest;
}

TM_CALLABLE
int TM_memcmp(const void * cs,const void * ct,size_t count)
{
  unsigned char *su1, *su2;
  int res = 0;

  for( su1 = (unsigned char *) cs, su2 = (unsigned char *) ct; 0 < count; ++su1, ++su2, count--)
    if ((res = *su1 - *su2) != 0)
      break;
  return res;
}

TM_CALLABLE
char * TM_strstr(const char * s1,const char * s2)
{
  int l1, l2;

  l2 = TM_strlen(s2);
  if (!l2)
    return (char *) s1;
  l1 = TM_strlen(s1);
  while (l1 >= l2) {
    l1--;
    if (!TM_memcmp(s1,s2,l2))
      return (char *) s1;
    s1++;
  }
  return NULL;
}

TM_CALLABLE
int TM_strcasecmp(const char *s1, const char *s2)
{
	while ((*s1 != '\0') &&
		   (TM_tolower(*(unsigned char *) s1) == TM_tolower(*(unsigned char *) s2)))
	{
		s1++;
		s2++;
	}

	return TM_tolower(*(unsigned char *) s1) - TM_tolower(*(unsigned char *) s2);
}



/*
===============================
Function from math.c
===============================
*/


TM_PURE
float TM_sqrt (float x)
{
	return sqrt(x);
}

TM_PURE
int TM_rand (void)
{
	return rand();
}

TM_PURE
float TM_cos (float x)
{
	return cos(x);
}

TM_PURE
float TM_sin (float x)
{
	return sin(x);
}

TM_PURE
float TM_floor (float x)
{
	return floor(x);
}

TM_PURE
float TM_ceil (float x)
{
	return ceil(x);
}

TM_PURE
float TM_atan2 (float y, float x)
{
	return atan2(y, x);
}

TM_PURE
float TM_fabs (float x)
{
	return fabs(x);
}



// /*
// ===============================
// Function from omp.h
// ===============================
// */
//
// TM_PURE
// int ThreadNumber(void)
// {
//   return omp_get_thread_num();
// }



