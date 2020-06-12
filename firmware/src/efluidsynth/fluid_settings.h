#ifndef _FLUID_SETTINGS_H
#define _FLUID_SETTINGS_H

#include "fluid_types.h"


struct _fluid_settings_t {
  char verbose;
  char dump;
  char reverb;
  char chorus;
  int polyphony;
  int midi_channels;
  float gain;
  int audio_channels;
  int audio_groups;
  int effects_channels;
  float sample_rate;
};

#if 0

/** returns 1 if the option was added, 0 otherwise */
int fluid_settings_add_option(fluid_settings_t* settings, char* name, char* s);

/** returns 1 if the option was added, 0 otherwise */
int fluid_settings_remove_option(fluid_settings_t* settings, char* name, char* s);


typedef int (*fluid_num_update_t)(void* data, char* name, fluid_real_t value);
typedef int (*fluid_str_update_t)(void* data, char* name, char* value);
typedef int (*fluid_int_update_t)(void* data, char* name, int value);

/** returns 0 if the value has been resgister correctly, non-zero
    otherwise */
int fluid_settings_register_str(fluid_settings_t* settings, char* name, char* def, int hints,
			       fluid_str_update_t fun, void* data);

/** returns 0 if the value has been resgister correctly, non-zero
    otherwise */
int fluid_settings_register_num(fluid_settings_t* settings, char* name, fluid_real_t min, fluid_real_t max,
			       fluid_real_t def, int hints, fluid_num_update_t fun, void* data);


/** returns 0 if the value has been resgister correctly, non-zero
    otherwise */
int fluid_settings_register_int(fluid_settings_t* settings, char* name, int min, int max,
			       int def, int hints, fluid_int_update_t fun, void* data);

  /**
   *
   *    Synthesizer settings
   *    
   *     
   *     The create a synthesizer object you will have to specify its
   *     settings. These settings are stored in the structure below. 

   *     void my_synthesizer() 
   *     {
   *       fluid_settings_t* settings;
   *       fluid_synth_t* synth;
   *       fluid_audio_driver_t* adriver;
   *
   *
   *       settings = new_fluid_settings();
   *       fluid_settings_setstr(settings, "audio.driver", "alsa");
   *       // ... change settings ... 
   *       synth = new_fluid_synth(settings);
   *       adriver = new_fluid_audio_driver(settings, synth);
   *
   *       ...
   *
   *     }
   * 
   *
   */

/* Hint FLUID_HINT_BOUNDED_BELOW indicates that the LowerBound field
   of the FLUID_PortRangeHint should be considered meaningful. The
   value in this field should be considered the (inclusive) lower
   bound of the valid range. If FLUID_HINT_SAMPLE_RATE is also
   specified then the value of LowerBound should be multiplied by the
   sample rate. */
#define FLUID_HINT_BOUNDED_BELOW   0x1

/* Hint FLUID_HINT_BOUNDED_ABOVE indicates that the UpperBound field
   of the FLUID_PortRangeHint should be considered meaningful. The
   value in this field should be considered the (inclusive) upper
   bound of the valid range. If FLUID_HINT_SAMPLE_RATE is also
   specified then the value of UpperBound should be multiplied by the
   sample rate. */
#define FLUID_HINT_BOUNDED_ABOVE   0x2

/* Hint FLUID_HINT_TOGGLED indicates that the data item should be
   considered a Boolean toggle. Data less than or equal to zero should
   be considered `off' or `false,' and data above zero should be
   considered `on' or `true.' FLUID_HINT_TOGGLED may not be used in
   conjunction with any other hint except FLUID_HINT_DEFAULT_0 or
   FLUID_HINT_DEFAULT_1. */
#define FLUID_HINT_TOGGLED         0x4

/* Hint FLUID_HINT_SAMPLE_RATE indicates that any bounds specified
   should be interpreted as multiples of the sample rate. For
   instance, a frequency range from 0Hz to the Nyquist frequency (half
   the sample rate) could be requested by this hint in conjunction
   with LowerBound = 0 and UpperBound = 0.5. Hosts that support bounds
   at all must support this hint to retain meaning. */
#define FLUID_HINT_SAMPLE_RATE     0x8

/* Hint FLUID_HINT_LOGARITHMIC indicates that it is likely that the
   user will find it more intuitive to view values using a logarithmic
   scale. This is particularly useful for frequencies and gains. */
#define FLUID_HINT_LOGARITHMIC     0x10

/* Hint FLUID_HINT_INTEGER indicates that a user interface would
   probably wish to provide a stepped control taking only integer
   values. Any bounds set should be slightly wider than the actual
   integer range required to avoid floating point rounding errors. For
   instance, the integer set {0,1,2,3} might be described as [-0.1,
   3.1]. */
#define FLUID_HINT_INTEGER         0x20


#define FLUID_HINT_FILENAME        0x01
#define FLUID_HINT_OPTIONLIST      0x02

enum fluid_types_enum {
  FLUID_NO_TYPE = -1,
  FLUID_NUM_TYPE,
  FLUID_INT_TYPE,
  FLUID_STR_TYPE,
  FLUID_SET_TYPE
};

fluid_settings_t* new_fluid_settings(void);
void delete_fluid_settings(fluid_settings_t* settings);

int fluid_settings_get_type(fluid_settings_t* settings, char* name);


int fluid_settings_get_hints(fluid_settings_t* settings, char* name);

/** Returns whether the setting is changeable in real-time. */
int fluid_settings_is_realtime(fluid_settings_t* settings, char* name);


/** returns 1 if the value has been set, 0 otherwise */

int fluid_settings_setstr(fluid_settings_t* settings, char* name, char* str);

/** 
    Get the value of a string setting. If the value does not exists,
    'str' is set to NULL. Otherwise, 'str' will point to the
    value. The application does not own the returned value. Instead,
    the application should make a copy of the value if it needs it
    later.

   \returns 1 if the value exists, 0 otherwise 
*/

int fluid_settings_getstr(fluid_settings_t* settings, char* name, char** str);

/** Get the default value of a string setting. */

char* fluid_settings_getstr_default(fluid_settings_t* settings, char* name);

/** Get the value of a numeric setting. 

   \returns 1 if the value exists and is equal to 'value', 0
    otherwise 
*/

int fluid_settings_str_equal(fluid_settings_t* settings, char* name, char* value);


/** returns 1 if the value has been set, 0 otherwise */

int fluid_settings_setnum(fluid_settings_t* settings, char* name, fluid_real_t val);

/** returns 1 if the value exists, 0 otherwise */

int fluid_settings_getnum(fluid_settings_t* settings, char* name, fluid_real_t* val);

/** Get the default value of a string setting. */

fluid_real_t fluid_settings_getnum_default(fluid_settings_t* settings, char* name);
  
/** Get the range of values of a numeric settings. */

void fluid_settings_getnum_range(fluid_settings_t* settings, char* name, 
				fluid_real_t* min, fluid_real_t* max);


/** returns 1 if the value has been set, 0 otherwise */

int fluid_settings_setint(fluid_settings_t* settings, char* name, int val);

/** returns 1 if the value exists, 0 otherwise */

int fluid_settings_getint(fluid_settings_t* settings, char* name, int* val);

/** Get the default value of a string setting. */

int fluid_settings_getint_default(fluid_settings_t* settings, char* name);
  
/** Get the range of values of a numeric settings. */

void fluid_settings_getint_range(fluid_settings_t* settings, char* name, 
				int* min, int* max);



typedef void (*fluid_settings_foreach_option_t)(void* data, char* name, char* option);

void fluid_settings_foreach_option(fluid_settings_t* settings, 
				  char* name, void* data, 
				  fluid_settings_foreach_option_t func);


typedef void (*fluid_settings_foreach_t)(void* data, char* s, int type);

void fluid_settings_foreach(fluid_settings_t* settings, void* data, 
			   fluid_settings_foreach_t func);

#endif
#endif /* _FLUID_SETTINGS_H */
