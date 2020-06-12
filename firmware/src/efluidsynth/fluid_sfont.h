#ifndef _PRIV_FLUID_SFONT_H
#define _PRIV_FLUID_SFONT_H

  /**
   *
   *   SoundFont plugins
   *
   *    It is possible to add new SoundFont loaders to the
   *    synthesizer. The API uses a couple of "interfaces" (structures
   *    with callback functions): fluid_sfloader_t, fluid_sfont_t, and
   *    fluid_preset_t. 
   *
   *    To add a new SoundFont loader to the synthesizer, call
   *    fluid_synth_add_sfloader() and pass a pointer to an
   *    fluid_sfloader_t structure. The important callback function in
   *    this structure is "load", which should try to load a file and
   *    returns a fluid_sfont_t structure, or NULL if it fails.
   *
   *    The fluid_sfont_t structure contains a callback to obtain the
   *    name of the soundfont. It contains two functions to iterate
   *    though the contained presets, and one function to obtain a
   *    preset corresponding to a bank and preset number. This
   *    function should return an fluid_preset_t structure.
   *
   *    The fluid_preset_t structure contains some functions to obtain
   *    information from the preset (name, bank, number). The most
   *    important callback is the noteon function. The noteon function
   *    should call fluid_synth_alloc_voice() for every sample that has
   *    to be played. fluid_synth_alloc_voice() expects a pointer to a
   *    fluid_sample_t structure and returns a pointer to the opaque
   *    fluid_voice_t structure. To set or increments the values of a
   *    generator, use fluid_voice_gen_{set,incr}. When you are
   *    finished initializing the voice call fluid_voice_start() to
   *    start playing the synthesis voice.
   * */

  enum {
    FLUID_PRESET_SELECTED,
    FLUID_PRESET_UNSELECTED,
    FLUID_SAMPLE_DONE
  };

/*
 * fluid_sfloader_t
 */

struct _fluid_sfloader_t {
  /** Private data */
  void* data;

  /** The free must free the memory allocated for the loader in
   * addition to any private data. It should return 0 if no error
   * occured, non-zero otherwise.*/
  int (*free)(fluid_sfloader_t* loader);

  /** Load a file. Returns NULL if an error occured. */
  fluid_sfont_t* (*load)(fluid_sfloader_t* loader, const char* filename);
};


/*
 * fluid_sfont_t
 */

struct _fluid_sfont_t {
  void* data;
  uint32_t id;

  /** The 'free' callback function should return 0 when it was able to
      free all resources. It should return a non-zero value if some of
      the samples could not be freed because they are still in use. */
  int (*free)(fluid_sfont_t* sfont);

  /** Return the name of the sfont */
  char* (*get_name)(fluid_sfont_t* sfont);

  /** Return the preset with the specified bank and preset number. All
   *  the fields, including the 'sfont' field, should * be filled
   *  in. If the preset cannot be found, the function returns NULL. */
  fluid_preset_t* (*get_preset)(fluid_sfont_t* sfont, unsigned int bank, unsigned int prenum);

  void (*iteration_start)(fluid_sfont_t* sfont);

  /* return 0 when no more presets are available, 1 otherwise */
  int (*iteration_next)(fluid_sfont_t* sfont, fluid_preset_t* preset);
};

#define fluid_sfont_get_id(_sf) ((_sf)->id)


/*
 * fluid_preset_t 
 */

struct _fluid_preset_t {
  void* data;
  fluid_sfont_t* sfont;
  int (*free)(fluid_preset_t* preset);
  char* (*get_name)(fluid_preset_t* preset);
  int (*get_banknum)(fluid_preset_t* preset);
  int (*get_num)(fluid_preset_t* preset);

  /** handle a noteon event. Returns 0 if no error occured. */
  int (*noteon)(fluid_preset_t* preset, fluid_synth_t* synth, int chan, int key, int vel);

  /** Implement this function if the preset needs to be notified about
      preset select and unselect events. */
  int (*notify)(fluid_preset_t* preset, int reason, int chan);
};

typedef short fluid_sampledata;

/*
 * fluid_sample_t
 */

struct _fluid_sample_t
{
#ifndef FLUID_NO_NAMES
  char name[21];
#endif
  uint32_t start;
  uint32_t end;	/* Note: Index of last valid sample point (contrary to SF spec) */
  uint32_t loopstart;
  uint32_t loopend;	/* Note: first point following the loop (superimposed on loopstart) */
  uint32_t samplerate;
  int origpitch;
  int pitchadj;
  int sampletype;
  int valid;
  fluid_sampledata* data;

  /** The amplitude, that will lower the level of the sample's loop to
      the noise floor. Needed for note turnoff optimization, will be
      filled out automatically */
  /* Set this to zero, when submitting a new sample. */
  int amplitude_that_reaches_noise_floor_is_valid; 
  fluid_real_t amplitude_that_reaches_noise_floor;

  /** Count the number of playing voices that use this sample. */
  uint32_t refcount;

  /** Implement this function if the sample or SoundFont needs to be
      notified when the sample is no longer used. */
  int (*notify)(fluid_sample_t* sample, int reason);

  /** Pointer to SoundFont specific data */
  void* userdata;
};


#define fluid_sample_refcount(_sample) ((_sample)->refcount)


/** Sample types */

#define FLUID_SAMPLETYPE_MONO	1
#define FLUID_SAMPLETYPE_RIGHT	2
#define FLUID_SAMPLETYPE_LEFT	4
#define FLUID_SAMPLETYPE_LINKED	8
#define FLUID_SAMPLETYPE_ROM	0x8000

/*
 * Utility macros to access soundfonts, presets, and samples
 */

#define fluid_sfloader_delete(_loader) { if ((_loader) && (_loader)->free) (*(_loader)->free)(_loader); }
#define fluid_sfloader_load(_loader, _filename) (*(_loader)->load)(_loader, _filename)


#define delete_fluid_sfont(_sf)   ( ((_sf) && (_sf)->free)? (*(_sf)->free)(_sf) : 0)
#define fluid_sfont_get_name(_sf) (*(_sf)->get_name)(_sf)
#define fluid_sfont_get_preset(_sf,_bank,_prenum) (*(_sf)->get_preset)(_sf,_bank,_prenum)
#define fluid_sfont_iteration_start(_sf) (*(_sf)->iteration_start)(_sf)
#define fluid_sfont_iteration_next(_sf,_pr) (*(_sf)->iteration_next)(_sf,_pr)
#define fluid_sfont_get_data(_sf) (_sf)->data
#define fluid_sfont_set_data(_sf,_p) { (_sf)->data = (void*) (_p); }


#define delete_fluid_preset(_preset) \
  { if ((_preset) && (_preset)->free) { (*(_preset)->free)(_preset); }}

#define fluid_preset_get_data(_preset) (_preset)->data
#define fluid_preset_set_data(_preset,_p) { (_preset)->data = (void*) (_p); }
#define fluid_preset_get_name(_preset) (*(_preset)->get_name)(_preset)
#define fluid_preset_get_banknum(_preset) (*(_preset)->get_banknum)(_preset)
#define fluid_preset_get_num(_preset) (*(_preset)->get_num)(_preset)

#define fluid_preset_noteon(_preset,_synth,_ch,_key,_vel) \
  (*(_preset)->noteon)(_preset,_synth,_ch,_key,_vel)

#define fluid_preset_notify(_preset,_reason,_chan) \
  { if ((_preset) && (_preset)->notify) { (*(_preset)->notify)(_preset,_reason,_chan); }}


#define fluid_sample_incr_ref(_sample) (_sample)->refcount++;

#define fluid_sample_decr_ref(_sample) \
  (_sample)->refcount--; \
  if (((_sample)->refcount == 0) && ((_sample)->notify)) \
    (*(_sample)->notify)(_sample, FLUID_SAMPLE_DONE);

extern uint64_t fluid_sample_mem;


#endif /* _PRIV_FLUID_SFONT_H */
