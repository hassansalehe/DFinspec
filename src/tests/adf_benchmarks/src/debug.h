#ifndef __DEBUG_H__
#define __DEBUG_H__

/* internal debug facility */
#define DEBUG_NONE     0
#define DEBUG_THREAD   1
#define DEBUG_TASK     2

#ifndef DEBUGMODS
#define DEBUGMODS DEBUG_NONE
// #define DEBUGMODS DEBUG_THREAD+DEBUG_TASK
#endif

#define DEBUGPRINT(mod, ...) /*\  */
//	if (mod && DEBUGMODS) printf(__VA_ARGS__); HASSAN


#endif
