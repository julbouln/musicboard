/**

   This header contains a bunch of (mostly) system and machine
   dependent functions:

   - timers
   - current time in milliseconds and microseconds
   - debug logging
   - profiling
   - memory locking
   - checking for floating point exceptions

 */

#ifndef _FLUID_SYS_H
#define _FLUID_SYS_H

#include "fluid_types.h"


void fluid_sys_config(void);
void fluid_log_config(void);
void fluid_time_config(void);


/*
 * Utility functions
 */
char *fluid_strtok (char **str, char *delim);


/**

  Additional debugging system, separate from the log system. This
  allows to print selected debug messages of a specific subsystem.

 */

extern unsigned int fluid_debug_flags;

#if DEBUG

enum fluid_debug_level {
  FLUID_DBG_DRIVER = 1
};

int fluid_debug(int level, char * fmt, ...);

#else
#define fluid_debug
#endif


/** fluid_curtime() returns the current time in milliseconds. This time
    should only be used in relative time measurements.  */

/** fluid_utime() returns the time in micro seconds. this time should
    only be used to measure duration (relative times). */


unsigned int fluid_curtime(void);
double fluid_utime(void);

/**
    Timers

 */

/* if the callback function returns 1 the timer will continue; if it
   returns 0 it will stop */
typedef int (*fluid_timer_callback_t)(void* data, unsigned int msec);

typedef struct _fluid_timer_t fluid_timer_t;

fluid_timer_t* new_fluid_timer(int msec, fluid_timer_callback_t callback,
					    void* data, int new_thread, int auto_destroy);

int delete_fluid_timer(fluid_timer_t* timer);
int fluid_timer_join(fluid_timer_t* timer);
int fluid_timer_stop(fluid_timer_t* timer);

/**

    Muteces

*/

#ifdef FLUID_ENABLE_THREAD
#include <pthread.h>
typedef pthread_mutex_t fluid_mutex_t;
#define fluid_mutex_init(_m)      pthread_mutex_init(&(_m), NULL)
#define fluid_mutex_destroy(_m)   pthread_mutex_destroy(&(_m))
#define fluid_mutex_lock(_m)      pthread_mutex_lock(&(_m))
#define fluid_mutex_unlock(_m)    pthread_mutex_unlock(&(_m))
#else
typedef int fluid_mutex_t;
#define fluid_mutex_init(_m)      { (_m) = 0; }
#define fluid_mutex_destroy(_m)
#define fluid_mutex_lock(_m)
#define fluid_mutex_unlock(_m)
#endif

/**
     Threads

*/

typedef struct _fluid_thread_t fluid_thread_t;
typedef void (*fluid_thread_func_t)(void* data);

/** When detached, 'join' does not work and the thread destroys itself
    when finished. */
fluid_thread_t* new_fluid_thread(fluid_thread_func_t func, void* data, int detach);
int delete_fluid_thread(fluid_thread_t* thread);
int fluid_thread_join(fluid_thread_t* thread);


/**

    Profiling
 */


/**
    Profile numbers. List all the pieces of code you want to profile
    here. Be sure to add an entry in the fluid_profile_data table in
    fluid_sys.c
*/
enum {
  FLUID_PROF_WRITE_S16,
  FLUID_PROF_ONE_BLOCK,
  FLUID_PROF_ONE_BLOCK_CLEAR,
  FLUID_PROF_ONE_BLOCK_VOICE,
  FLUID_PROF_ONE_BLOCK_VOICES,
  FLUID_PROF_ONE_BLOCK_REVERB,
  FLUID_PROF_ONE_BLOCK_CHORUS,
  FLUID_PROF_VOICE_NOTE,
  FLUID_PROF_VOICE_RELEASE,
  FLUID_PROF_LAST
};


#if WITH_PROFILING

void fluid_profiling_print(void);


/** Profiling data. Keep track of min/avg/max values to execute a
    piece of code. */
typedef struct _fluid_profile_data_t {
  int num;
  char* description;
  double min, max, total;
  unsigned int count;
} fluid_profile_data_t;

extern fluid_profile_data_t fluid_profile_data[];

/** Macro to obtain a time refence used for the profiling */
#define fluid_profile_ref() fluid_utime()

/** Macro to calculate the min/avg/max. Needs a time refence and a
    profile number. */
#define fluid_profile(_num,_ref) { \
  double _now = fluid_utime(); \
  double _delta = _now - _ref; \
  fluid_profile_data[_num].min = _delta < fluid_profile_data[_num].min ? _delta : fluid_profile_data[_num].min; \
  fluid_profile_data[_num].max = _delta > fluid_profile_data[_num].max ? _delta : fluid_profile_data[_num].max; \
  fluid_profile_data[_num].total += _delta; \
  fluid_profile_data[_num].count++; \
  _ref = _now; \
}


#else

/* No profiling */
#define fluid_profiling_print()
#define fluid_profile_ref()  0
#define fluid_profile(_num,_ref)

#endif



/**

    Memory locking

    Memory locking is used to avoid swapping of the large block of
    sample data.
 */

#if defined(HAVE_SYS_MMAN_H)
#define fluid_mlock(_p,_n)      mlock(_p, _n)
#define fluid_munlock(_p,_n)    munlock(_p,_n)
#else
#define fluid_mlock(_p,_n)      0
#define fluid_munlock(_p,_n)
#endif


#endif /* _FLUID_SYS_H */
