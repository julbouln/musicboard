#ifndef _FLUID_TYPES_H
#define _FLUID_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>
	
#include "efluidsynth_defs.h"
#include "fluid_dsp.h"

/***************************************************************
 *
 *         BASIC TYPES
 */

typedef enum {
  FLUID_OK = 0,
  FLUID_FAILED = -1
} fluid_status;

/* Linux & Darwin */
typedef int8_t             sint8;
typedef uint8_t           uint8;
typedef int16_t            sint16;
typedef uint16_t          uint16;
typedef int32_t            sint32;
typedef uint32_t          uint32;
typedef int64_t            sint64;
typedef uint64_t          uint64;

/***************************************************************
 *
 *       FORWARD DECLARATIONS
 */

/***************************************************************
 *
 *                      CONSTANTS
 */

#define FLUID_SPRINTF                sprintf
#define FLUID_FPRINTF                fprintf
#define FLUID_PRINTF                 printf
#define FLUID_FFLUSH(_o)             fflush(_o)
#define FLUID_FLUSH()                FLUID_FFLUSH(stdout)

#ifndef PI
#define PI                          3.141592654
#endif

/***************************************************************
 *
 *                      SYSTEM INTERFACE
 */

#ifdef strdup
#define FLUID_STRDUP(s)              strdup(s)
#else
#define FLUID_STRDUP(s) 		    FLUID_STRCPY(FLUID_MALLOC(FLUID_STRLEN(s) + 1), s)
#endif

#define fluid_clip(_val, _min, _max) \
{ (_val) = ((_val) < (_min))? (_min) : (((_val) > (_max))? (_max) : (_val)); }

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define FLUID_ASSERT(a,b)
#define FLUID_ASSERT_P(a,b)

char* fluid_error(void);

/* Internationalization */
#define _(s) s

#ifdef FLUID_CALC_FORMAT_FLOAT
typedef float fluid_real_t;
#else
typedef double fluid_real_t;
#endif

/* Forward declarations */
typedef struct _fluid_channel_t fluid_channel_t;
typedef struct _fluid_env_data_t fluid_env_data_t;
typedef struct _fluid_adriver_definition_t fluid_adriver_definition_t;
typedef struct _fluid_tuning_t fluid_tuning_t;
typedef struct _fluid_hashtable_t  fluid_hashtable_t;
typedef struct _fluid_client_t fluid_client_t;
typedef struct _fluid_settings_t fluid_settings_t;
typedef struct _fluid_synth_t fluid_synth_t;
typedef struct _fluid_voice_t fluid_voice_t;
typedef struct _fluid_sfloader_t fluid_sfloader_t;
typedef struct _fluid_sfont_t fluid_sfont_t;
typedef struct _fluid_preset_t fluid_preset_t;
typedef struct _fluid_sample_t fluid_sample_t;
typedef struct _fluid_sample_chunk_t fluid_sample_chunk_t;
typedef struct _fluid_mod_t fluid_mod_t;
typedef struct _fluid_audio_driver_t fluid_audio_driver_t;
typedef struct _fluid_player_t fluid_player_t;
typedef struct _fluid_midi_event_t fluid_midi_event_t;
typedef struct _fluid_midi_driver_t fluid_midi_driver_t;
typedef struct _fluid_midi_router_t fluid_midi_router_t;
typedef struct _fluid_midi_router_rule_t fluid_midi_router_rule_t;
typedef struct _fluid_hashtable_t fluid_cmd_handler_t;
typedef struct _fluid_shell_t fluid_shell_t;
typedef struct _fluid_server_t fluid_server_t;
typedef struct _fluid_event_t fluid_event_t;
typedef struct _fluid_sequencer_t fluid_sequencer_t;
typedef struct _fluid_ramsfont_t fluid_ramsfont_t;
typedef struct _fluid_rampreset_t fluid_rampreset_t;

typedef int fluid_istream_t;
typedef int fluid_ostream_t;

#ifdef __cplusplus
}
#endif

#endif /* _FLUIDSYNTH_TYPES_H */
