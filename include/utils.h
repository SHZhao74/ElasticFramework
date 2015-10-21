#ifndef NEWSCL_UTILS_H_
#define NEWSCL_UTILS_H_
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#ifdef CLDEBUG
#define P_LOCK(a) printf("----------------mutex lock file:%s(%d)\n)", __FILE__, __LINE__);if(pthread_mutex_lock(&(a))!=0){assert(false);}
#define P_UNLOCK(a) printf("----------------mutex unlock file:%s(%d)\n)", __FILE__, __LINE__); pthread_mutex_unlock(&(a))
#else
#define P_LOCK(a) if(pthread_mutex_lock(&(a))!=0){assert(false);}
#define P_UNLOCK(a) pthread_mutex_unlock(&(a))
#endif

    
#endif  // NEWSCL_UTILS_H_
