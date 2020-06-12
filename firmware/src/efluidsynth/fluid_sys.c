#include "fluid_sys.h"
#include "fluid_log.h"

static char fluid_errbuf[512];  /* buffer for error message */

static fluid_log_function_t fluid_log_function[LAST_LOG_LEVEL];
static void* fluid_log_user_data[LAST_LOG_LEVEL];
static int fluid_log_initialized = 0;

static char* fluid_libname = "fluidsynth";


void fluid_sys_config()
{
  fluid_log_config();
  fluid_time_config();
}

unsigned int fluid_debug_flags = 0;

#if DEBUG
/*
 * fluid_debug
 */
int fluid_debug(int level, char * fmt, ...)
{
  if (fluid_debug_flags & level) {
    fluid_log_function_t fun;
    va_list args;

    va_start (args, fmt);
    vsnprintf(fluid_errbuf, sizeof (fluid_errbuf), fmt, args);
    va_end (args);

    fun = fluid_log_function[FLUID_DBG];
    if (fun != NULL) {
      (*fun)(level, fluid_errbuf, fluid_log_user_data[FLUID_DBG]);
    }
  }
  return 0;
}
#endif

/**
 * Installs a new log function for a specified log level.
 * @param level Log level to install handler for.
 * @param fun Callback function handler to call for logged messages
 * @param data User supplied data pointer to pass to log function
 * @return The previously installed function.
 */
fluid_log_function_t
fluid_set_log_function(int level, fluid_log_function_t fun, void* data)
{
  fluid_log_function_t old = NULL;

  if ((level >= 0) && (level < LAST_LOG_LEVEL)) {
    old = fluid_log_function[level];
    fluid_log_function[level] = fun;
    fluid_log_user_data[level] = data;
  }
  return old;
}

/**
 * Default log function which prints to the stderr.
 * @param level Log level
 * @param message Log message
 * @param data User supplied data (not used)
 */
void
fluid_default_log_function(int level, char* message, void* data)
{
  FILE* out;

  out = stderr;

  if (fluid_log_initialized == 0) {
    fluid_log_config();
  }

  switch (level) {
  case FLUID_PANIC:
    FLUID_FPRINTF(out, "%s: panic: %s\n", fluid_libname, message);
    break;
  case FLUID_ERR:
    FLUID_FPRINTF(out, "%s: error: %s\n", fluid_libname, message);
    break;
  case FLUID_WARN:
    FLUID_FPRINTF(out, "%s: warning: %s\n", fluid_libname, message);
    break;
  case FLUID_INFO:
    FLUID_FPRINTF(out, "%s: %s\n", fluid_libname, message);
    break;
  case FLUID_DBG:
#if DEBUG
    FLUID_FPRINTF(out, "%s: debug: %s\n", fluid_libname, message);
#endif
    break;
  default:
    FLUID_FPRINTF(out, "%s: %s\n", fluid_libname, message);
    break;
  }
  FLUID_FFLUSH(out);
}

/*
 * fluid_init_log
 */
void
fluid_log_config(void)
{
  if (fluid_log_initialized == 0) {

    fluid_log_initialized = 1;

    if (fluid_log_function[FLUID_PANIC] == NULL) {
      fluid_set_log_function(FLUID_PANIC, fluid_default_log_function, NULL);
    }

    if (fluid_log_function[FLUID_ERR] == NULL) {
      fluid_set_log_function(FLUID_ERR, fluid_default_log_function, NULL);
    }

    if (fluid_log_function[FLUID_WARN] == NULL) {
      fluid_set_log_function(FLUID_WARN, fluid_default_log_function, NULL);
    }

    if (fluid_log_function[FLUID_INFO] == NULL) {
      fluid_set_log_function(FLUID_INFO, fluid_default_log_function, NULL);
    }

    if (fluid_log_function[FLUID_DBG] == NULL) {
      fluid_set_log_function(FLUID_DBG, fluid_default_log_function, NULL);
    }
  }
}

/**
 * Print a message to the log.
 * @param level Log level (#fluid_log_level).
 * @param fmt Printf style format string for log message
 * @param ... Arguments for printf 'fmt' message string
 * @return Always returns -1
 */
int
fluid_log(int level, char* fmt, ...)
{
  fluid_log_function_t fun = NULL;

  va_list args;
  va_start (args, fmt);
  vsnprintf(fluid_errbuf, sizeof (fluid_errbuf), fmt, args);
  va_end (args);

  if ((level >= 0) && (level < LAST_LOG_LEVEL)) {
    fun = fluid_log_function[level];
    if (fun != NULL) {
      (*fun)(level, fluid_errbuf, fluid_log_user_data[level]);
    }
  }
  return FLUID_FAILED;
}

/**
 * An improved strtok, still trashes the input string, but is portable and
 * thread safe.  Also skips token chars at beginning of token string and never
 * returns an empty token (will return NULL if source ends in token chars though).
 * NOTE: NOT part of public API
 * @internal
 * @param str Pointer to a string pointer of source to tokenize.  Pointer gets
 *   updated on each invocation to point to beginning of next token.  Note that
 *   token char get's overwritten with a 0 byte.  String pointer is set to NULL
 *   when final token is returned.
 * @param delim String of delimiter chars.
 * @return Pointer to the next token or NULL if no more tokens.
 */
char *fluid_strtok (char **str, char *delim)
{
  char *s, *d, *token;
  char c;

  if (str == NULL || delim == NULL || !*delim)
  {
    FLUID_LOG(FLUID_ERR, "Null pointer");
    return NULL;
  }

  s = *str;
  if (!s) return NULL;	/* str points to a NULL pointer? (tokenize already ended) */

  /* skip delimiter chars at beginning of token */
  do
  {
    c = *s;
    if (!c)	/* end of source string? */
    {
      *str = NULL;
      return NULL;
    }

    for (d = delim; *d; d++)	/* is source char a token char? */
    {
      if (c == *d)	/* token char match? */
      {
	s++;		/* advance to next source char */
	break;
      }
    }
  } while (*d);		/* while token char match */

  token = s;		/* start of token found */

  /* search for next token char or end of source string */
  for (s = s+1; *s; s++)
  {
    c = *s;

    for (d = delim; *d; d++)	/* is source char a token char? */
    {
      if (c == *d)	/* token char match? */
      {
	*s = '\0';	/* overwrite token char with zero byte to terminate token */
	*str = s+1;	/* update str to point to beginning of next token */
	return token;
      }
    }
  }

  /* we get here only if source string ended */
  *str = NULL;
  return token;
}

/*
 * fluid_error
 */
char*
fluid_error()
{
  return fluid_errbuf;
}

#if FLUID_ENABLE_THREAD
#include <pthread.h>
#endif
/*=============================================================*/
/*                                                             */
/*                           POSIX                             */
/*                                                             */
/*=============================================================*/


/***************************************************************
 *
 *               Timer
 */

struct _fluid_timer_t
{
  long msec;
  fluid_timer_callback_t callback;
  void* data;
#if FLUID_ENABLE_THREAD
  pthread_t thread;
#endif
  int cont;
  int auto_destroy;
};

void*
fluid_timer_start(void *data)
{
  int count = 0;
  int cont = 1;
  long start;
  long delay;
  fluid_timer_t* timer;
  timer = (fluid_timer_t*) data;

  /* keep track of the start time for absolute positioning */
  start = fluid_curtime();

  while (cont) {

    /* do whatever we have to do */
    cont = (*timer->callback)(timer->data, fluid_curtime() - start);

    count++;

    /* to avoid incremental time errors, calculate the delay between
       two callbacks bringing in the "absolute" time (count *
       timer->msec) */
    delay = (count * timer->msec) - (fluid_curtime() - start);
    if (delay > 0) {
      usleep(delay * 1000);
    }

    cont &= timer->cont;
  }

#if FLUID_ENABLE_THREAD
  FLUID_LOG(FLUID_DBG, "Timer thread finished");
  if (timer->thread != 0) {
    pthread_exit(NULL);
  }
#endif

  if (timer->auto_destroy) {
    FLUID_FREE(timer);
  }

  return NULL;
}

fluid_timer_t*
new_fluid_timer(int msec, fluid_timer_callback_t callback, void* data,
	       int new_thread, int auto_destroy)
{
  int err;

  fluid_timer_t* timer = FLUID_NEW(fluid_timer_t);
  if (timer == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }
  timer->msec = msec;
  timer->callback = callback;
  timer->data = data;
  timer->cont = 1;
  timer->auto_destroy = auto_destroy;

#if FLUID_ENABLE_THREAD
  timer->thread = 0;

  pthread_attr_t *attr = NULL;
  pthread_attr_t rt_attr;
  int sched = SCHED_FIFO;
  struct sched_param priority;
  err = pthread_attr_init(&rt_attr);
  if (err == 0) {
	  err = pthread_attr_setschedpolicy(&rt_attr, SCHED_FIFO);
	  if (err == 0) {
		  priority.sched_priority = 10;
		  err = pthread_attr_setschedparam(&rt_attr, &priority);
		  if (err == 0) {
			  attr = &rt_attr;
		  }
	  }
  }

  if (new_thread) {
	  err = pthread_create(&timer->thread, attr, fluid_timer_start, (void*) timer);
	  if (err == 0) {
		  FLUID_LOG(FLUID_DBG, "The timer thread was created with real-time priority");
	  } else {
		  /* Create the thread with default attributes */
		  err = pthread_create(&timer->thread, NULL, fluid_timer_start, (void*) timer);
		  if (err != 0) {
			  FLUID_LOG(FLUID_ERR, "Failed to create the timer thread");
			  FLUID_FREE(timer);
			  return NULL;
		  } else {
			  FLUID_LOG(FLUID_DBG, "The timer thread does not have real-time priority");
		  }
	  }
  } else {
    fluid_timer_start((void*) timer);
  }
  #else
    fluid_timer_start((void*) timer);
  #endif
  return timer;
}

int
delete_fluid_timer(fluid_timer_t* timer)
{
  timer->cont = 0;
  fluid_timer_join(timer);
  FLUID_LOG(FLUID_DBG, "Joined player thread");
  FLUID_FREE(timer);
  return FLUID_OK;
}

int
fluid_timer_join(fluid_timer_t* timer)
{
  int err = 0;
#if FLUID_ENABLE_THREAD
  if (timer->thread != 0) {
    err = pthread_join(timer->thread, NULL);
  }
#endif
  FLUID_LOG(FLUID_DBG, "Joined player thread");
  return (err == 0)? FLUID_OK : FLUID_FAILED;
}


/***************************************************************
 *
 *               Time
 */

static double fluid_cpu_frequency = -1.0;

double rdtsc(void);
double fluid_estimate_cpu_frequency(void);

void fluid_time_config(void)
{
  if (fluid_cpu_frequency < 0.0) {
    fluid_cpu_frequency = fluid_estimate_cpu_frequency() / 1000000.0;
    if (fluid_cpu_frequency == 0.0) fluid_cpu_frequency = 1.0;
  }
}

#ifdef FLUID_POSIX

unsigned int fluid_curtime()
{
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000 + now.tv_usec / 1000;
}

#else

unsigned int fluid_curtime()
{
  return 0;
}


#endif

double fluid_utime(void)
{
  return (rdtsc() / fluid_cpu_frequency);
}

#if !defined(__i386__)

double rdtsc(void)
{
  return 0.0;
}

double fluid_estimate_cpu_frequency(void)
{
  return 1.0;
}

#else

double rdtsc(void)
{
  unsigned int a, b;

  __asm__ ("rdtsc" : "=a" (a), "=d" (b));
  return (double)b * (double)0x10000 * (double)0x10000 + a;
}

double fluid_estimate_cpu_frequency(void)
{
  double start, stop;
  unsigned int a0, b0, a1, b1;
  unsigned int before, after;

  before = fluid_curtime();
  __asm__ ("rdtsc" : "=a" (a0), "=d" (b0));

  sleep(1);

  after = fluid_curtime();
  __asm__ ("rdtsc" : "=a" (a1), "=d" (b1));


  start = (double)b0 * (double)0x10000 * (double)0x10000 + a0;
  stop = (double)b1 * (double)0x10000 * (double)0x10000 + a1;

  return 1000 * (stop - start) / (after - before);
}


#endif	// #else    (its POSIX)


/***************************************************************
 *
 *               Profiling (Linux, i586 only)
 *
 */

#if WITH_PROFILING

fluid_profile_data_t fluid_profile_data[] =
{
  { FLUID_PROF_WRITE_S16,        "fluid_synth_write_s16           ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK,        "fluid_synth_one_block           ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK_CLEAR,  "fluid_synth_one_block:clear     ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK_VOICE,  "fluid_synth_one_block:one voice ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK_VOICES, "fluid_synth_one_block:all voices", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK_REVERB, "fluid_synth_one_block:reverb    ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_ONE_BLOCK_CHORUS, "fluid_synth_one_block:chorus    ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_VOICE_NOTE,       "fluid_voice:note                ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_VOICE_RELEASE,    "fluid_voice:release             ", 1e10, 0.0, 0.0, 0},
  { FLUID_PROF_LAST, "last", 1e100, 0.0, 0.0, 0}
};


void fluid_profiling_print(void)
{
  int i;

  printf("fluid_profiling_print\n");

  FLUID_LOG(FLUID_INFO, "Estimated CPU frequency: %.0f MHz", fluid_cpu_frequency);
  FLUID_LOG(FLUID_INFO, "Estimated times: min/avg/max (micro seconds)");

  for (i = 0; i < FLUID_PROF_LAST; i++) {
    if (fluid_profile_data[i].count > 0) {
      FLUID_LOG(FLUID_INFO, "%s: %.3f/%.3f/%.3f",
	       fluid_profile_data[i].description,
	       fluid_profile_data[i].min,
	       fluid_profile_data[i].total / fluid_profile_data[i].count,
	       fluid_profile_data[i].max);
    } else {
      FLUID_LOG(FLUID_DBG, "%s: no profiling available", fluid_profile_data[i].description);
    }
  }
}


#endif /* WITH_PROFILING */



/***************************************************************
 *
 *               Threads
 *
 */

#ifdef FLUID_ENABLE_THREAD

struct _fluid_thread_t {
  pthread_t pthread;
  fluid_thread_func_t func;
  void* data;
  int detached;
};

static void* fluid_thread_start(void *data)
{
  fluid_thread_t* thread = (fluid_thread_t*) data;

  thread->func(thread->data);

  if (thread->detached) {
    FLUID_FREE(thread);
  }

  return NULL;
}

fluid_thread_t* new_fluid_thread(fluid_thread_func_t func, void* data, int detach)
{
  fluid_thread_t* thread;
  pthread_attr_t attr;

  if (func == NULL) {
    FLUID_LOG(FLUID_ERR, "Invalid thread function");
    return NULL;
  }

  thread = FLUID_NEW(fluid_thread_t);
  if (thread == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }

  thread->data = data;
  thread->func = func;
  thread->detached = detach;

  pthread_attr_init(&attr);

  if (detach) {
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  }

  if (pthread_create(&thread->pthread, &attr, fluid_thread_start, thread)) {
    FLUID_LOG(FLUID_ERR, "Failed to create the thread");
    FLUID_FREE(thread);
    return NULL;
  }

  return thread;
}

int delete_fluid_thread(fluid_thread_t* thread)
{
  FLUID_FREE(thread);
  return FLUID_OK;
}

int fluid_thread_join(fluid_thread_t* thread)
{
  int err = 0;

  if (thread->pthread != 0) {
    err = pthread_join(thread->pthread, NULL);
  }
  return (err == 0)? FLUID_OK : FLUID_FAILED;
}
 #else
/* Not implemented */
fluid_thread_t* new_fluid_thread(fluid_thread_func_t func, void* data, int detach) { return NULL; }
int delete_fluid_thread(fluid_thread_t* thread) { return 0; }
int fluid_thread_join(fluid_thread_t* thread) { return 0; }
 #endif

