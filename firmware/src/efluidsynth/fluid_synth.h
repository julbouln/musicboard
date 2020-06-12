#ifndef _FLUID_SYNTH_H
#define _FLUID_SYNTH_H

#include "fluid_list.h"
#include "fluid_rev.h"
#include "fluid_voice.h"
#include "fluid_chorus.h"
#include "fluid_sys.h"
#include "fluid_types.h"

/***************************************************************
 *
 *                         DEFINES
 */
#define FLUID_NUM_PROGRAMS      128
#define DRUM_INST_BANK		128

#ifdef FLUID_SAMPLE_FORMAT_FLOAT
#define FLUID_SAMPLE_FORMAT     FLUID_SAMPLE_FLOAT
#else
#define FLUID_SAMPLE_FORMAT     FLUID_SAMPLE_DOUBLE
#endif


/***************************************************************
 *
 *                         ENUM
 */
enum fluid_loop {
  FLUID_UNLOOPED = 0,
  FLUID_LOOP_DURING_RELEASE = 1,
  FLUID_NOTUSED = 2,
  FLUID_LOOP_UNTIL_RELEASE = 3
};

enum fluid_synth_status
{
  FLUID_SYNTH_CLEAN,
  FLUID_SYNTH_PLAYING,
  FLUID_SYNTH_QUIET,
  FLUID_SYNTH_STOPPED
};

typedef struct _fluid_bank_offset_t fluid_bank_offset_t;

struct _fluid_bank_offset_t {
	int sfont_id;
	int offset;
};

/*
 * fluid_synth_t
 */

struct _fluid_synth_t
{
  /* fluid_settings_old_t settings_old;  the old synthesizer settings */
  fluid_settings_t* settings;         /** the synthesizer settings */
  int polyphony;                     /** maximum polyphony */
  char with_reverb;                  /** Should the synth use the built-in reverb unit? */
  char with_chorus;                  /** Should the synth use the built-in chorus unit? */
  char verbose;                      /** Turn verbose mode on? */
  char dump;                         /** Dump events to stdout to hook up a user interface? */
  fluid_real_t sample_rate;                /** The sample rate */
  int midi_channels;                 /** the number of MIDI channels (>= 16) */
  int audio_channels;                /** the number of audio channels (1 channel=left+right) */
  int audio_groups;                  /** the number of (stereo) 'sub'groups from the synth.
					 Typically equal to audio_channels. */
  int effects_channels;              /** the number of effects channels (= 2) */
  uint32_t state;                /** the synthesizer state */
  uint32_t ticks;                /** the number of audio samples since the start */
  uint32_t start;                /** the start in msec, as returned by system clock */

  fluid_list_t *loaders;              /** the soundfont loaders */
  fluid_list_t* sfont;                /** the loaded soundfont */
  unsigned int sfont_id;
  fluid_list_t* bank_offsets;       /** the offsets of the soundfont banks */

  fluid_real_t gain;                        /** master gain */
  fluid_channel_t** channel;          /** the channels */
  int num_channels;                   /** the number of channels */
  int nvoice;                         /** the length of the synthesis process array */
  fluid_voice_t** voice;              /** the synthesis processes */
  uint32_t noteid;                /** the id is incremented for every new note. it's used for noteoff's  */
  uint32_t storeid;
  int nbuf;                           /** How many audio buffers are used? (depends on nr of audio channels / groups)*/

  fluid_buf_t** left_buf;
  fluid_buf_t** right_buf;
  fluid_buf_t** fx_left_buf;
  fluid_buf_t** fx_right_buf;

  fluid_revmodel_t* reverb;
  fluid_chorus_t* chorus;
  int cur;                           /** the current sample in the audio buffers to be output */

  char outbuf[256];                  /** buffer for message output */
  double cpu_load;

  fluid_tuning_t*** tuning;           /** 128 banks of 128 programs for the tunings */
  fluid_tuning_t* cur_tuning;         /** current tuning in the iteration */

  fluid_midi_router_t* midi_router;     /* The midi router. Could be done nicer. */
  fluid_mutex_t busy;                   /* Indicates, whether the audio thread is currently running.
					 * Note: This simple scheme does -not- provide 100 % protection against
					 * thread problems, for example from MIDI thread and shell thread
					 */
};

int fluid_synth_set_reverb_preset(fluid_synth_t* synth, int num);

int fluid_synth_one_block(fluid_synth_t* synth, int do_not_mix_fx_to_out);

fluid_preset_t* fluid_synth_get_preset(fluid_synth_t* synth,
				     unsigned int sfontnum,
				     unsigned int banknum,
				     unsigned int prognum);

fluid_preset_t* fluid_synth_find_preset(fluid_synth_t* synth,
				      unsigned int banknum,
				      unsigned int prognum);

int fluid_synth_all_notes_off(fluid_synth_t* synth, int chan);
int fluid_synth_all_sounds_off(fluid_synth_t* synth, int chan);
int fluid_synth_modulate_voices(fluid_synth_t* synth, int chan, int is_cc, int ctrl);
int fluid_synth_modulate_voices_all(fluid_synth_t* synth, int chan);
int fluid_synth_channel_pressure(fluid_synth_t* synth, int chan, int val);
int fluid_synth_damp_voices(fluid_synth_t* synth, int chan);
int fluid_synth_kill_voice(fluid_synth_t* synth, fluid_voice_t * voice);
void fluid_synth_kill_by_exclusive_class(fluid_synth_t* synth, fluid_voice_t* voice);
void fluid_synth_release_voice_on_same_note(fluid_synth_t* synth, int chan, int key);

void fluid_synth_print_voice(fluid_synth_t* synth);

/** This function assures that every MIDI channels has a valid preset
 *  (NULL is okay). This function is called after a SoundFont is
 *  unloaded or reloaded. */
void fluid_synth_update_presets(fluid_synth_t* synth);


int fluid_synth_update_gain(fluid_synth_t* synth, char* name, fluid_real_t value);
int fluid_synth_update_polyphony(fluid_synth_t* synth, char* name, int value);

fluid_bank_offset_t* fluid_synth_get_bank_offset0(fluid_synth_t* synth, int sfont_id);
void fluid_synth_remove_bank_offset(fluid_synth_t* synth, int sfont_id);

/*
 * misc
 */

void fluid_synth_settings(fluid_settings_t* settings);

  /**   Embedded synthesizer
   *  
   *    You create a new synthesizer with new_fluid_synth() and you destroy
   *    if with delete_fluid_synth(). Use the settings structure to specify
   *    the synthesizer characteristics. 
   *
   *    You have to load a SoundFont in order to hear any sound. For that
   *    you use the fluid_synth_sfload() function.
   *
   *    You can use the audio driver functions described below to open
   *    the audio device and create a background audio thread.
   *  
   *    The API for sending MIDI events is probably what you expect:
   *    fluid_synth_noteon(), fluid_synth_noteoff(), ...
   * 
   */


  /** Creates a new synthesizer object. 
   *
   *  Creates a new synthesizer object. As soon as the synthesizer is
   *  created, it will start playing.  
   *
   * \param settings a pointer to a settings structure
   * \return a newly allocated synthesizer or NULL in case of error
   */
fluid_synth_t* new_fluid_synth(fluid_settings_t* settings);


  /** 
   * Deletes the synthesizer previously created with new_fluid_synth.
   *
   * \param synth the synthesizer object
   * \return 0 if no error occured, -1 otherwise 
   */
int delete_fluid_synth(fluid_synth_t* synth);


  /** Get a reference to the settings of the synthesizer.
   *
   * \param synth the synthesizer object
   * \return pointer to the settings
   */
fluid_settings_t* fluid_synth_get_settings(fluid_synth_t* synth);


  /*
   * 
   * MIDI channel messages 
   *
   */

  /** Send a noteon message. Returns 0 if no error occurred, -1 otherwise. */
int fluid_synth_noteon(fluid_synth_t* synth, int chan, int key, int vel);

  /** Send a noteoff message. Returns 0 if no error occurred, -1 otherwise.  */
int fluid_synth_noteoff(fluid_synth_t* synth, int chan, int key);

  /** Send a control change message. Returns 0 if no error occurred, -1 otherwise.  */
int fluid_synth_cc(fluid_synth_t* synth, int chan, int ctrl, int val);

  /** Get a control value. Returns 0 if no error occurred, -1 otherwise.  */
int fluid_synth_get_cc(fluid_synth_t* synth, int chan, int ctrl, int* pval);

  /** Send a pitch bend message. Returns 0 if no error occurred, -1 otherwise.  */
int fluid_synth_pitch_bend(fluid_synth_t* synth, int chan, int val);

  /** Get the pitch bend value. Returns 0 if no error occurred, -1 otherwise. */

int fluid_synth_get_pitch_bend(fluid_synth_t* synth, int chan, int* ppitch_bend);

  /** Set the pitch wheel sensitivity. Returns 0 if no error occurred, -1 otherwise. */
int fluid_synth_pitch_wheel_sens(fluid_synth_t* synth, int chan, int val);

  /** Get the pitch wheel sensitivity. Returns 0 if no error occurred, -1 otherwise. */
int fluid_synth_get_pitch_wheel_sens(fluid_synth_t* synth, int chan, int* pval);

  /** Send a program change message. Returns 0 if no error occurred, -1 otherwise. */
int fluid_synth_program_change(fluid_synth_t* synth, int chan, int program);

  /** Select a bank. Returns 0 if no error occurred, -1 otherwise. */

int fluid_synth_bank_select(fluid_synth_t* synth, int chan, unsigned int bank);

  /** Select a sfont. Returns 0 if no error occurred, -1 otherwise. */

int fluid_synth_sfont_select(fluid_synth_t* synth, int chan, unsigned int sfont_id);

  /** Select a preset for a channel. The preset is specified by the
      SoundFont ID, the bank number, and the preset number. This
      allows any preset to be selected and circumvents preset masking
      due to previously loaded SoundFonts on the SoundFont stack.

      \param synth The synthesizer
      \param chan The channel on which to set the preset
      \param sfont_id The ID of the SoundFont 
      \param bank_num The bank number
      \param preset_num The preset number
      \return 0 if no errors occured, -1 otherwise
  */

int fluid_synth_program_select(fluid_synth_t* synth, int chan, 
            unsigned int sfont_id, 
            unsigned int bank_num, 
            unsigned int preset_num);

  /** Returns the program, bank, and SoundFont number of the preset on
      a given channel. Returns 0 if no error occurred, -1 otherwise. */

int fluid_synth_get_program(fluid_synth_t* synth, int chan, 
         unsigned int* sfont_id, 
         unsigned int* bank_num, 
         unsigned int* preset_num);

  /** Send a bank select and a program change to every channel to
   *  reinitialize the preset of the channel. This function is useful
   *  mainly after a SoundFont has been loaded, unloaded or
   *  reloaded. . Returns 0 if no error occurred, -1 otherwise. */
int fluid_synth_program_reset(fluid_synth_t* synth);

  /** Send a reset. A reset turns all the notes off and resets the
      controller values. */
int fluid_synth_system_reset(fluid_synth_t* synth);


  /*
   * 
   * Low level access 
   *
   */

  /** Create and start voices using a preset. The id passed as
   * argument will be used as the voice group id.  */
int fluid_synth_start(fluid_synth_t* synth, unsigned int id, 
             fluid_preset_t* preset, int audio_chan, 
             int midi_chan, int key, int vel);

  /** Stop the voices in the voice group defined by id. */
int fluid_synth_stop(fluid_synth_t* synth, unsigned int id);

  /** Change the value of a generator of the voices in the voice group
   * defined by id. */
/* int fluid_synth_ctrl(fluid_synth_t* synth, int id,  */
/*            int gen, float value,  */
/*            int absolute, int normalized); */


  /*
   * 
   * SoundFont management 
   *
   */

  /** Loads a SoundFont file and creates a new SoundFont. The newly
      loaded SoundFont will be put on top of the SoundFont
      stack. Presets are searched starting from the SoundFont on the
      top of the stack, working the way down the stack until a preset
      is found.

      \param synth The synthesizer object
      \param filename The file name
      \param reset_presets If non-zero, the presets on the channels will be reset
      \returns The ID of the loaded SoundFont, or -1 in case of error
  */

int fluid_synth_sfload(fluid_synth_t* synth, const char* filename, int reset_presets);

  /** Removes a SoundFont from the stack and deallocates it.

      \param synth The synthesizer object
      \param id The id of the SoundFont
      \param reset_presets If TRUE then presets will be reset for all channels
      \returns 0 if no error, -1 otherwise
  */
int fluid_synth_sfunload(fluid_synth_t* synth, unsigned int id, int reset_presets);

  /** Add a SoundFont. The SoundFont will be put on top of
      the SoundFont stack.

      \param synth The synthesizer object
      \param sfont The SoundFont
      \returns The ID of the loaded SoundFont, or -1 in case of error
  */
int fluid_synth_add_sfont(fluid_synth_t* synth, fluid_sfont_t* sfont);

  /** Remove a SoundFont that was previously added using
   *  fluid_synth_add_sfont(). The synthesizer does not delete the
   *  SoundFont; this is responsability of the caller.

      \param synth The synthesizer object
      \param sfont The SoundFont
  */
void fluid_synth_remove_sfont(fluid_synth_t* synth, fluid_sfont_t* sfont);

  /** Count the number of loaded SoundFonts.

      \param synth The synthesizer object
      \returns The number of loaded SoundFonts 
  */
int fluid_synth_sfcount(fluid_synth_t* synth);

  /** Get a SoundFont. The SoundFont is specified by its index on the
      stack. The top of the stack has index zero. 
    
      \param synth The synthesizer object
      \param num The number of the SoundFont (0 <= num < sfcount)
      \returns A pointer to the SoundFont
  */
fluid_sfont_t* fluid_synth_get_sfont(fluid_synth_t* synth, unsigned int num);

  /** Get a SoundFont. The SoundFont is specified by its ID.
    
      \param synth The synthesizer object
      \param id The id of the sfont
      \returns A pointer to the SoundFont
  */
fluid_sfont_t* fluid_synth_get_sfont_by_id(fluid_synth_t* synth, unsigned int id);


  /** Get the preset of a channel */
fluid_preset_t* fluid_synth_get_channel_preset(fluid_synth_t* synth, int chan);

  /** Offset the bank numbers in a SoundFont. Returns -1 if an error
   * occured (out of memory or negative offset) */ 
int fluid_synth_set_bank_offset(fluid_synth_t* synth, int sfont_id, int offset);

  /** Get the offset of the bank numbers in a SoundFont. */ 
int fluid_synth_get_bank_offset(fluid_synth_t* synth, int sfont_id);



  /*
   * 
   * Reverb 
   *
   */

  /** Set the parameters for the built-in reverb unit */
void fluid_synth_set_reverb(fluid_synth_t* synth, double roomsize, 
           double damping, double width, double level);

  /** Turn on (1) / off (0) the built-in reverb unit */
void fluid_synth_set_reverb_on(fluid_synth_t* synth, int on);


  /** Query the current state of the reverb. */
double fluid_synth_get_reverb_roomsize(fluid_synth_t* synth);
double fluid_synth_get_reverb_damp(fluid_synth_t* synth);
double fluid_synth_get_reverb_level(fluid_synth_t* synth);
double fluid_synth_get_reverb_width(fluid_synth_t* synth);

  /* Those are the default settings for the reverb */
#define FLUID_REVERB_DEFAULT_ROOMSIZE 0.2f
#define FLUID_REVERB_DEFAULT_DAMP 0.0f
#define FLUID_REVERB_DEFAULT_WIDTH 0.5f
#define FLUID_REVERB_DEFAULT_LEVEL 0.9f

  /*
   * 
   * Chorus 
   *
   */

enum fluid_chorus_mod {
  FLUID_CHORUS_MOD_SINE = 0,
  FLUID_CHORUS_MOD_TRIANGLE = 1
};

  /** Set up the chorus. It should be turned on with fluid_synth_set_chorus_on.
   * If faulty parameters are given, all new settings are discarded.
   * Keep in mind, that the needed CPU time is proportional to 'nr'.
   */
void fluid_synth_set_chorus(fluid_synth_t* synth, int nr, double level, 
           double speed, double depth_ms, int type);

  /** Turn on (1) / off (0) the built-in chorus unit */
void fluid_synth_set_chorus_on(fluid_synth_t* synth, int on);

  /** Query the current state of the chorus. */
int fluid_synth_get_chorus_nr(fluid_synth_t* synth);
double fluid_synth_get_chorus_level(fluid_synth_t* synth);
double fluid_synth_get_chorus_speed_Hz(fluid_synth_t* synth);
double fluid_synth_get_chorus_depth_ms(fluid_synth_t* synth);
int fluid_synth_get_chorus_type(fluid_synth_t* synth); /* see fluid_chorus_mod */

  /* Those are the default settings for the chorus. */
#define FLUID_CHORUS_DEFAULT_N 3
#define FLUID_CHORUS_DEFAULT_LEVEL 2.0f
#define FLUID_CHORUS_DEFAULT_SPEED 0.3f
#define FLUID_CHORUS_DEFAULT_DEPTH 8.0f
#define FLUID_CHORUS_DEFAULT_TYPE FLUID_CHORUS_MOD_SINE 

  /*
   * 
   * Audio and MIDI channels 
   *
   */

  /** Returns the number of MIDI channels that the synthesizer uses
      internally */
int fluid_synth_count_midi_channels(fluid_synth_t* synth);

  /** Returns the number of audio channels that the synthesizer uses
      internally */
int fluid_synth_count_audio_channels(fluid_synth_t* synth);

  /** Returns the number of audio groups that the synthesizer uses
      internally. This is usually identical to audio_channels. */
int fluid_synth_count_audio_groups(fluid_synth_t* synth);

  /** Returns the number of effects channels that the synthesizer uses
      internally */
int fluid_synth_count_effects_channels(fluid_synth_t* synth);



  /*
   * 
   * Synthesis parameters 
   *
   */

  /** Set the master gain */
void fluid_synth_set_gain(fluid_synth_t* synth, fluid_real_t gain);

  /** Get the master gain */
fluid_real_t fluid_synth_get_gain(fluid_synth_t* synth);

  /** Set the polyphony limit (FluidSynth >= 1.0.6) */
int fluid_synth_set_polyphony(fluid_synth_t* synth, int polyphony);

  /** Get the polyphony limit (FluidSynth >= 1.0.6) */
int fluid_synth_get_polyphony(fluid_synth_t* synth);

  /** Get the internal buffer size. The internal buffer size if not the
      same thing as the buffer size specified in the
      settings. Internally, the synth *always* uses a specific buffer
      size independent of the buffer size used by the audio driver. The
      internal buffer size is normally 64 samples. The reason why it
      uses an internal buffer size is to allow audio drivers to call the
      synthesizer with a variable buffer length. The internal buffer
      size is useful for client who want to optimize their buffer sizes.
  */
int fluid_synth_get_internal_bufsize(fluid_synth_t* synth);

  /** Set the interpolation method for one channel or all channels (chan = -1) */

int fluid_synth_set_interp_method(fluid_synth_t* synth, int chan, int interp_method);

  /* Flags to choose the interpolation method */
enum fluid_interp {
  /* no interpolation: Fastest, but questionable audio quality */
  FLUID_INTERP_NONE = 0,
  /* Straight-line interpolation: A bit slower, reasonable audio quality */
  FLUID_INTERP_LINEAR = 1,
  /* Fourth-order interpolation: Requires 50 % of the whole DSP processing time, good quality 
   * Default. */
  FLUID_INTERP_DEFAULT = 4,
  FLUID_INTERP_4THORDER = 4,
  FLUID_INTERP_7THORDER = 7,
  FLUID_INTERP_HIGHEST=7
};



#ifndef FLUID_NO_NRPN_EXT
  /*
   * 
   * Generator interface 
   *
   */

  /** Change the value of a generator. This function allows to control
      all synthesis parameters in real-time. The changes are additive,
      i.e. they add up to the existing parameter value. This function is
      similar to sending an NRPN message to the synthesizer. The
      function accepts a float as the value of the parameter. The
      parameter numbers and ranges are described in the SoundFont 2.01
      specification, paragraph 8.1.3, page 48. See also 'fluid_gen_type'.

      \param synth The synthesizer object.
      \param chan The MIDI channel number.
      \param param The parameter number.
      \param value The parameter value.
      \returns Your favorite dish.
  */

int fluid_synth_set_gen(fluid_synth_t* synth, int chan, int param, fluid_real_t value);


  /** Retreive the value of a generator. This function returns the value
      set by a previous call 'fluid_synth_set_gen' or by an NRPN message.

      \param synth The synthesizer object.
      \param chan The MIDI channel number.
      \param param The generator number.
      \returns The value of the generator.  
  */
float fluid_synth_get_gen(fluid_synth_t* synth, int chan, int param);
#endif



  /*
   * 
   * Tuning 
   *
   */

  /** Create a new key-based tuning with given name, number, and
      pitches. The array 'pitches' should have length 128 and contains
      the pitch in cents of every key in cents. However, if 'pitches' is
      NULL, a new tuning is created with the well-tempered scale.
    
      \param synth The synthesizer object
      \param tuning_bank The tuning bank number [0-127]
      \param tuning_prog The tuning program number [0-127]
      \param name The name of the tuning
      \param pitch The array of pitch values. The array length has to be 128.
  */

int fluid_synth_create_key_tuning(fluid_synth_t* synth, int tuning_bank, int tuning_prog, char* name, double* pitch);

  /** Create a new octave-based tuning with given name, number, and
      pitches.  The array 'pitches' should have length 12 and contains
      derivation in cents from the well-tempered scale. For example, if
      pitches[0] equals -33, then the C-keys will be tuned 33 cents
      below the well-tempered C. 

      \param synth The synthesizer object
      \param tuning_bank The tuning bank number [0-127]
      \param tuning_prog The tuning program number [0-127]
      \param name The name of the tuning
      \param pitch The array of pitch derivations. The array length has to be 12.
  */

int fluid_synth_create_octave_tuning(fluid_synth_t* synth, int tuning_bank, int tuning_prog, char* name, double* pitch);

  /** Request a note tuning changes. Both they 'keys' and 'pitches'
      arrays should be of length 'num_pitches'. If 'apply' is non-zero,
      the changes should be applied in real-time, i.e. sounding notes
      will have their pitch updated. 'APPLY' IS CURRENTLY IGNORED. The
      changes will be available for newly triggered notes only. 

      \param synth The synthesizer object
      \param tuning_bank The tuning bank number [0-127]
      \param tuning_prog The tuning program number [0-127]
      \param len The length of the keys and pitch arrays
      \param keys The array of keys values.
      \param pitch The array of pitch values.
      \param apply Flag to indicate whether to changes should be applied in real-time.    
  */

int fluid_synth_tune_notes(fluid_synth_t* synth, int tuning_bank, int tuning_prog,
        int len, int *keys, double* pitch, int apply);

  /** Select a tuning for a channel. 

  \param synth The synthesizer object
  \param chan The channel number [0-max channels]
  \param tuning_bank The tuning bank number [0-127]
  \param tuning_prog The tuning program number [0-127]
  */

int fluid_synth_select_tuning(fluid_synth_t* synth, int chan, int tuning_bank, int tuning_prog);

  /** Set the tuning to the default well-tempered tuning on a channel.

  \param synth The synthesizer object
  \param chan The channel number [0-max channels]
  */
int fluid_synth_reset_tuning(fluid_synth_t* synth, int chan);

  /** Start the iteration throught the list of available tunings.

  \param synth The synthesizer object
  */
void fluid_synth_tuning_iteration_start(fluid_synth_t* synth);


  /** Get the next tuning in the iteration. This functions stores the
      bank and program number of the next tuning in the pointers given as
      arguments.

      \param synth The synthesizer object
      \param bank Pointer to an int to store the bank number
      \param prog Pointer to an int to store the program number
      \returns 1 if there is a next tuning, 0 otherwise
  */

int fluid_synth_tuning_iteration_next(fluid_synth_t* synth, int* bank, int* prog);

  /*
   * 
   * Misc 
   *
   */

  /** Get an estimation of the CPU load due to the audio synthesis.
      Returns a percentage (0-100).

      \param synth The synthesizer object
  */
double fluid_synth_get_cpu_load(fluid_synth_t* synth);

  /** Get a textual representation of the last error */
char* fluid_synth_error(fluid_synth_t* synth);


  /*
   *  
   *    Synthesizer plugin
   *  
   *    
   *    To create a synthesizer plugin, create the synthesizer as
   *    explained above. Once the synthesizer is created you can call
   *    any of the functions below to get the audio. 
   * 
   */

  /** Generate a number of samples. This function expects two signed
   *  16bits buffers (left and right channel) that will be filled with
   *  samples.
   *
   *  \param synth The synthesizer
   *  \param len The number of samples to generate
   *  \param lout The sample buffer for the left channel
   *  \param loff The offset, in samples, in the left buffer where the writing pointer starts
   *  \param lincr The increment, in samples, of the writing pointer in the left buffer 
   *  \param rout The sample buffer for the right channel
   *  \param roff The offset, in samples, in the right buffer where the writing pointer starts
   *  \param rincr The increment, in samples, of the writing pointer in the right buffer 
   *  \returns 0 if no error occured, non-zero otherwise
   */

int fluid_synth_write_s16(fluid_synth_t* synth, int len, 
               void* lout, int loff, int lincr, 
               void* rout, int roff, int rincr);


int fluid_synth_write_s32(fluid_synth_t* synth, int len, 
               void* lout, int loff, int lincr, 
               void* rout, int roff, int rincr);

  /** Generate a number of samples. This function expects two floating
   *  point buffers (left and right channel) that will be filled with
   *  samples.
   *
   *  \param synth The synthesizer
   *  \param len The number of samples to generate
   *  \param lout The sample buffer for the left channel
   *  \param loff The offset, in samples, in the left buffer where the writing pointer starts
   *  \param lincr The increment, in samples, of the writing pointer in the left buffer 
   *  \param rout The sample buffer for the right channel
   *  \param roff The offset, in samples, in the right buffer where the writing pointer starts
   *  \param rincr The increment, in samples, of the writing pointer in the right buffer 
   *  \returns 0 if no error occured, non-zero otherwise
   */

int fluid_synth_write_float(fluid_synth_t* synth, int len, 
           void* lout, int loff, int lincr, 
           void* rout, int roff, int rincr);


  /* Type definition of the synthesizer's audio callback function. */
typedef int (*fluid_audio_callback_t)(fluid_synth_t* synth, int len, 
             void* out1, int loff, int lincr, 
             void* out2, int roff, int rincr);

  /*
   *  Synthesizer's interface to handle SoundFont loaders 
   */


  /** Add a SoundFont loader to the synthesizer. Note that SoundFont
      loader don't necessarily load SoundFonts. They can load any type
      of wavetable data but export a SoundFont interface. */
void fluid_synth_add_sfloader(fluid_synth_t* synth, fluid_sfloader_t* loader);

  /** Allocate a synthesis voice. This function is called by a
      soundfont's preset in response to a noteon event.
      The returned voice comes with default modulators installed (velocity-to-attenuation,
      velocity to filter, ...)
      Note: A single noteon event may create any number of voices, when the preset is layered. 
      Typically 1 (mono) or 2 (stereo).*/
fluid_voice_t* fluid_synth_alloc_voice(fluid_synth_t* synth, fluid_sample_t* sample, 
               int channum, int key, int vel);

  /** Start a synthesis voice. This function is called by a
      soundfont's preset in response to a noteon event after the voice
      has been allocated with fluid_synth_alloc_voice() and
      initialized. 
      Exclusive classes are processed here.*/
void fluid_synth_start_voice(fluid_synth_t* synth, fluid_voice_t* voice);


  /** Write a list of all voices matching ID into buf, but not more than bufsize voices.
   * If ID <0, return all voices. */
void fluid_synth_get_voicelist(fluid_synth_t* synth, 
              fluid_voice_t* buf[], int bufsize, int ID);

#endif  /* _FLUID_SYNTH_H */
