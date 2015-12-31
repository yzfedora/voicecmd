#ifndef _DEBUG_H
#define _DEBUG_H

#if defined(__DEBUG__)
#define DEBUG(msg, ...)						\
	do {							\
		fprintf(stderr,  msg "\n", ##__VA_ARGS__);	\
	} while (0)
#else
#define	DEBUG(msg, ...)
#endif

#endif
