#ifndef HELIOS_SRC_USR_INCLUDE_PTHREAD_OS_H_
#define HELIOS_SRC_USR_INCLUDE_PTHREAD_OS_H_

#include <stdint.h>
#include <syscall.h>

typedef struct {
	uint32_t id;
	char * stack;
	void * ret_val;
} pthread_t;

typedef unsigned int pthread_attr_t;

extern int pthread_create(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void*), void* arg);
extern void pthread_exit(void * val);
extern int pthread_kill(pthread_t thread, int sig);

extern int clone(uintptr_t, uintptr_t, void*);
extern int gettid();

#endif /* HELIOS_SRC_USR_INCLUDE_PTHREAD_OS_H_ */
