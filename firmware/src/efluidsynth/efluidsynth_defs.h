#ifndef EFLUIDSYNTH_DEFS_H
#define EFLUIDSYNTH_DEFS_H

#define FLUID_BUFSIZE       256

#ifdef __arm__
#include "qspi_wrapper.h"


#define	FLUID_LOG(...) 
//fluid_log( __VA_ARGS__)


#define FLUID_MALLOC(_n)             malloc(_n)
#define FLUID_REALLOC(_p,_n)         realloc(_p,_n)
#define FLUID_NEW(_t)                (_t*)malloc(sizeof(_t))
#define FLUID_ARRAY(_t,_n)           (_t*)malloc((_n)*sizeof(_t))
#define FLUID_FREE(_p)               free(_p)

typedef QSPI_FILE* fluid_file;

#define FLUID_FOPEN(_f,_m)       QSPI_fopen(_f,_m)
#define FLUID_FCLOSE(_f)             QSPI_fclose(_f)
#define FLUID_FREAD(_p,_s,_n,_f)     QSPI_fread(_p,_s,_n,_f)
#define FLUID_FSEEK(_f,_n,_set)      QSPI_fseek(_f,_n,_set)
#define FLUID_FTELL(_f)				 QSPI_ftell(_f)
#define FLUID_FEOF(_f)				 QSPI_feof(_f)
#define FLUID_REWIND(_f)			 QSPI_fseek(_f,0,SEEK_SET)
#define FLUID_MMAP(_p,_s,_f)				QSPI_mmap(_p,_s,_f)

#define FLUID_MEMCPY(_dst,_src,_n)   memcpy(_dst,_src,_n)
#define FLUID_MEMSET(_s,_c,_n)       memset(_s,_c,_n)
#define FLUID_STRLEN(_s)             strlen(_s)
#define FLUID_STRCMP(_s,_t)          strcmp(_s,_t)
#define FLUID_STRNCMP(_s,_t,_n)      strncmp(_s,_t,_n)
#define FLUID_STRCPY(_dst,_src)      strcpy(_dst,_src)
#define FLUID_STRCHR(_s,_c)          strchr(_s,_c)

#define FLUID_SIN(_s)				sinf(_s)
#define FLUID_COS(_s)				cosf(_s)
#define FLUID_ABS(_s)				fabsf(_s)
#define FLUID_POW(_b,_e)			powf(_b,_e)
#define FLUID_SQRT(_s)				sqrtf(_s)
#define FLUID_MATH_LOG(_s)			logf(_s)

#else

#define	FLUID_LOG(...) fluid_log( __VA_ARGS__)

typedef FILE*  fluid_file;

#define FLUID_MALLOC(_n)             malloc(_n)
#define FLUID_REALLOC(_p,_n)         realloc(_p,_n)
#define FLUID_NEW(_t)                (_t*)malloc(sizeof(_t))
#define FLUID_ARRAY(_t,_n)           (_t*)malloc((_n)*sizeof(_t))
#define FLUID_FREE(_p)               free(_p)

#define FLUID_FOPEN(_f,_m)       	 fopen(_f,_m)
#define FLUID_FCLOSE(_f)             fclose(_f)
#define FLUID_FREAD(_p,_s,_n,_f)     fread(_p,_s,_n,_f)
#define FLUID_FSEEK(_f,_n,_set)      fseek(_f,_n,_set)
#define FLUID_FTELL(_f)				 ftell(_f)
#define FLUID_FEOF(_f)				 feof(_f)
#define FLUID_REWIND(_f)			 rewind(_f)
#include <sys/mman.h>
#define	FLUID_MMAP(_p,_s,_f)		mmap(0, _s, PROT_READ, MAP_SHARED, fileno(_f), 0)

#define FLUID_MEMCPY(_dst,_src,_n)   memcpy(_dst,_src,_n)
#define FLUID_MEMSET(_s,_c,_n)       memset(_s,_c,_n)
#define FLUID_STRLEN(_s)             strlen(_s)
#define FLUID_STRCMP(_s,_t)          strcmp(_s,_t)
#define FLUID_STRNCMP(_s,_t,_n)      strncmp(_s,_t,_n)
#define FLUID_STRCPY(_dst,_src)      strcpy(_dst,_src)
#define FLUID_STRCHR(_s,_c)          strchr(_s,_c)

#define FLUID_SIN(_s)				sinf(_s)
#define FLUID_COS(_s)				cosf(_s)
#define FLUID_ABS(_s)				fabsf(_s)
#define FLUID_POW(_b,_e)			powf(_b,_e)
#define FLUID_SQRT(_s)				sqrtf(_s)
#define FLUID_MATH_LOG(_s)			logf(_s)
#endif
#endif