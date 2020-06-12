#ifndef _FLUID_GEN_H
#define _FLUID_GEN_H

#include "fluid_types.h"
#include "fluid_list.h"

typedef struct _fluid_gen_info_t {
  char num;   /* Generator number */
  char init;    /* Does the generator need to be initialized (cfr. fluid_voice_init()) */
  char nrpn_scale;  /* The scale to convert from NRPN (cfr. fluid_gen_map_nrpn()) */
  fluid_real_t min;   /* The minimum value */
  fluid_real_t max;   /* The maximum value */
  fluid_real_t def;   /* The default value (cfr. fluid_gen_set_default_values()) */
} fluid_gen_info_t;

/**
 * Generator (effect) numbers (Soundfont 2.01 specifications section 8.1.3)
 */
enum fluid_gen_type {
  GEN_STARTADDROFS,   /**< Sample start address offset (0-32767) */
  GEN_ENDADDROFS,   /**< Sample end address offset (-32767-0) */
  GEN_STARTLOOPADDROFS,   /**< Sample loop start address offset (-32767-32767) */
  GEN_ENDLOOPADDROFS,   /**< Sample loop end address offset (-32767-32767) */
  GEN_STARTADDRCOARSEOFS, /**< Sample start address coarse offset (X 32768) */
  GEN_MODLFOTOPITCH,    /**< Modulation LFO to pitch */
  GEN_VIBLFOTOPITCH,    /**< Vibrato LFO to pitch */
  GEN_MODENVTOPITCH,    /**< Modulation envelope to pitch */
  GEN_FILTERFC,     /**< Filter cutoff */
  GEN_FILTERQ,      /**< Filter Q */
  GEN_MODLFOTOFILTERFC,   /**< Modulation LFO to filter cutoff */
  GEN_MODENVTOFILTERFC,   /**< Modulation envelope to filter cutoff */
  GEN_ENDADDRCOARSEOFS,   /**< Sample end address coarse offset (X 32768) */
  GEN_MODLFOTOVOL,    /**< Modulation LFO to volume */
  GEN_UNUSED1,      /**< Unused */
  GEN_CHORUSSEND,   /**< Chorus send amount */
  GEN_REVERBSEND,   /**< Reverb send amount */
  GEN_PAN,      /**< Stereo panning */
  GEN_UNUSED2,      /**< Unused */
  GEN_UNUSED3,      /**< Unused */
  GEN_UNUSED4,      /**< Unused */
  GEN_MODLFODELAY,    /**< Modulation LFO delay */
  GEN_MODLFOFREQ,   /**< Modulation LFO frequency */
  GEN_VIBLFODELAY,    /**< Vibrato LFO delay */
  GEN_VIBLFOFREQ,   /**< Vibrato LFO frequency */
  GEN_MODENVDELAY,    /**< Modulation envelope delay */
  GEN_MODENVATTACK,   /**< Modulation envelope attack */
  GEN_MODENVHOLD,   /**< Modulation envelope hold */
  GEN_MODENVDECAY,    /**< Modulation envelope decay */
  GEN_MODENVSUSTAIN,    /**< Modulation envelope sustain */
  GEN_MODENVRELEASE,    /**< Modulation envelope release */
  GEN_KEYTOMODENVHOLD,    /**< Key to modulation envelope hold */
  GEN_KEYTOMODENVDECAY,   /**< Key to modulation envelope decay */
  GEN_VOLENVDELAY,    /**< Volume envelope delay */
  GEN_VOLENVATTACK,   /**< Volume envelope attack */
  GEN_VOLENVHOLD,   /**< Volume envelope hold */
  GEN_VOLENVDECAY,    /**< Volume envelope decay */
  GEN_VOLENVSUSTAIN,    /**< Volume envelope sustain */
  GEN_VOLENVRELEASE,    /**< Volume envelope release */
  GEN_KEYTOVOLENVHOLD,    /**< Key to volume envelope hold */
  GEN_KEYTOVOLENVDECAY,   /**< Key to volume envelope decay */
  GEN_INSTRUMENT,   /**< Instrument ID (shouldn't be set by user) */
  GEN_RESERVED1,    /**< Reserved */
  GEN_KEYRANGE,     /**< MIDI note range */
  GEN_VELRANGE,     /**< MIDI velocity range */
  GEN_STARTLOOPADDRCOARSEOFS, /**< Sample start loop address coarse offset (X 32768) */
  GEN_KEYNUM,     /**< Fixed MIDI note number */
  GEN_VELOCITY,     /**< Fixed MIDI velocity value */
  GEN_ATTENUATION,    /**< Initial volume attenuation */
  GEN_RESERVED2,    /**< Reserved */
  GEN_ENDLOOPADDRCOARSEOFS, /**< Sample end loop address coarse offset (X 32768) */
  GEN_COARSETUNE,   /**< Coarse tuning */
  GEN_FINETUNE,     /**< Fine tuning */
  GEN_SAMPLEID,     /**< Sample ID (shouldn't be set by user) */
  GEN_SAMPLEMODE,   /**< Sample mode flags */
  GEN_RESERVED3,    /**< Reserved */
  GEN_SCALETUNE,    /**< Scale tuning */
  GEN_EXCLUSIVECLASS,   /**< Exclusive class number */
  GEN_OVERRIDEROOTKEY,    /**< Sample root note override */

  /* the initial pitch is not a "standard" generator. It is not
   * mentioned in the list of generator in the SF2 specifications. It
   * is used, however, as the destination for the default pitch wheel
   * modulator. */
  GEN_PITCH,      /**< Pitch (NOTE: Not a real SoundFont generator) */
  GEN_LAST      /**< Value defines the count of generators (#fluid_gen_type) */
};

/**
 * SoundFont generator structure.
 */
typedef struct _fluid_gen_t
{
  uint8_t num;
  uint8_t flags; /**< Is the generator set or not (#fluid_gen_flags) */
  fluid_real_t val;          /**< The nominal value */
  fluid_real_t mod;          /**< Change by modulators */
#ifndef FLUID_NO_NRPN_EXT
  fluid_real_t nrpn;         /**< Change by NRPN messages */
#endif
} fluid_gen_t;

/**
 * Enum value for 'flags' field of #_fluid_gen_t (not really flags).
 */
enum fluid_gen_flags
{
  GEN_UNUSED,   /**< Generator value is not set */
  GEN_SET,    /**< Generator value is set */
  GEN_ABS_NRPN    /**< DOCME */
};


fluid_gen_t * fluid_gen_create(uint8_t num);
fluid_gen_t *fluid_gen_get(fluid_list_t *gen_list, uint8_t num);
fluid_real_t fluid_gen_get_default_value(uint8_t num);
void fluid_gen_delete(fluid_gen_t *gen);

int fluid_gen_set_default_values(fluid_gen_t* gen);
int fluid_gen_init(fluid_gen_t* gen, fluid_channel_t* channel);

fluid_real_t fluid_gen_scale(int gen, fluid_real_t value);
#ifndef FLUID_NO_NRPN_EXT
fluid_real_t fluid_gen_scale_nrpn(int gen, int nrpn);
#endif
#define fluid_gen_set_mod(_gen, _val)  { (_gen)->mod = (fluid_real_t) (_val); }
#define fluid_gen_set_nrpn(_gen, _val) { (_gen)->nrpn = (fluid_real_t) (_val); }

#endif /* _FLUID_GEN_H */
