#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>

#define DO_LOCK

#if defined(DO_TM)
	#define TRANSACTION_BEGIN		__tm_atomic {
	#define TRANSACTION_END			}
	#define TM_INIT
	#define TM_CALLABLE			__attribute__((tm_callable))
	#define TM_ONLY				__attribute__((tm_only))
	#define TM_PURE				__attribute__((tm_pure))
	#define TM_UNKNOWN			__attribute__((tm_unknown))
#elif defined (DO_LOCK)
// 	pthread_mutex_t				global_lock;
// 	#define TRANSACTION_BEGIN		pthread_mutex_lock(&global_lock);
// 	#define TRANSACTION_END			pthread_mutex_unlock(&global_lock);
// 	#define TM_INIT				pthread_mutex_init(&global_lock, NULL);
	#define TM_CALLABLE
	#define TM_ONLY
	#define TM_PURE
	#define TM_UNKNOWN
#else
	#define TRANSACTION_BEGIN
	#define TRANSACTION_END
	#define TM_INIT
	#define TM_CALLABLE
	#define TM_ONLY
	#define TM_PURE
	#define TM_UNKNOWN
# endif



TM_CALLABLE
void	* TM_memset(void * s,int c,size_t count);
TM_CALLABLE
void	* TM_memcpy(void * dest,const void *src,size_t count);
TM_CALLABLE
char	* TM_strcpy(char * dest,const char *src);
TM_CALLABLE
char	* TM_strncpy(char * dest,const char *src,size_t count);
TM_CALLABLE
char	* TM_strcat(char * dest, const char * src);
TM_CALLABLE
char	* TM_strncat(char *dest, const char *src, size_t count);
TM_CALLABLE
int		TM_strcmp(const char * cs,const char * ct);
TM_CALLABLE
int		TM_strncmp(const char * cs,const char * ct,size_t count);
TM_CALLABLE
size_t	TM_strlen(const char * s);
TM_CALLABLE
size_t	TM_strnlen(const char * s, size_t count);
TM_CALLABLE
char	* TM_strstr(const char * s1,const char * s2);
TM_CALLABLE
int		TM_strcasecmp(const char *s1, const char *s2);



TM_CALLABLE
int	TM_atoi (char *str);
TM_CALLABLE
float TM_atof (char *str);

TM_PURE
float	TM_sqrt (float x);
TM_PURE
float	TM_cos (float x);
TM_PURE
float	TM_sin (float x);
TM_PURE
float	TM_atan2 (float y, float x);
TM_PURE
int		TM_rand (void);
TM_PURE
float	TM_fabs (float x);
TM_PURE
float	TM_floor (float x);
TM_PURE
float	TM_ceil (float x);

TM_PURE
int TM_tolower(int x);

TM_CALLABLE
int TM_vsprintf(char *buf, const char *fmt, va_list args);
TM_CALLABLE
int TM_sprintf(char *buf, const char *fmt, ...);
