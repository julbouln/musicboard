#ifndef _FLUID_MOD_H
#define _FLUID_MOD_H

#include "fluid_conv.h"
#include "fluid_chan.h"
#include "fluid_types.h"

  /* Modulator-related definitions */

  /* Maximum number of modulators in a voice */
#define FLUID_NUM_MOD           64

  /*
   *  fluid_mod_t
   */
struct _fluid_mod_t
{
  uint8_t dest;
  uint8_t src1;
  uint8_t flags1;
  uint8_t src2;
  uint8_t flags2;
  fluid_real_t amount;
  /* The 'next' field allows to link modulators into a list.  It is
   * not used in fluid_voice.c, there each voice allocates memory for a
   * fixed number of modulators.  Since there may be a huge number of
   * different zones, this is more efficient.
   */
  fluid_mod_t * next;
};


/* Flags telling the polarity of a modulator.  Compare with SF2.01
   section 8.2. Note: The numbers of the bits are different!  (for
   example: in the flags of a SF modulator, the polarity bit is bit
   nr. 9) */
enum fluid_mod_flags
{
  FLUID_MOD_POSITIVE = 0,
  FLUID_MOD_NEGATIVE = 1,
  FLUID_MOD_UNIPOLAR = 0,
  FLUID_MOD_BIPOLAR = 2,
  FLUID_MOD_LINEAR = 0,
  FLUID_MOD_CONCAVE = 4,
  FLUID_MOD_CONVEX = 8,
  FLUID_MOD_SWITCH = 12,
  FLUID_MOD_GC = 0,
  FLUID_MOD_CC = 16
};

/* Flags telling the source of a modulator.  This corresponds to
 * SF2.01 section 8.2.1 */
enum fluid_mod_src
{
  FLUID_MOD_NONE = 0,
  FLUID_MOD_VELOCITY = 2,
  FLUID_MOD_KEY = 3,
  FLUID_MOD_KEYPRESSURE = 10,
  FLUID_MOD_CHANNELPRESSURE = 13,
  FLUID_MOD_PITCHWHEEL = 14,
  FLUID_MOD_PITCHWHEELSENS = 16
};

/* Allocates memory for a new modulator */
fluid_mod_t * fluid_mod_new(void);

/* Frees the modulator */
void fluid_mod_delete(fluid_mod_t * mod);

void fluid_mod_set_source1(fluid_mod_t* mod, int src, int flags); 
void fluid_mod_set_source2(fluid_mod_t* mod, int src, int flags); 
void fluid_mod_set_dest(fluid_mod_t* mod, int dst); 
void fluid_mod_set_amount(fluid_mod_t* mod, double amount); 

int fluid_mod_get_source1(fluid_mod_t* mod);
int fluid_mod_get_flags1(fluid_mod_t* mod);
int fluid_mod_get_source2(fluid_mod_t* mod);
int fluid_mod_get_flags2(fluid_mod_t* mod);
int fluid_mod_get_dest(fluid_mod_t* mod);
double fluid_mod_get_amount(fluid_mod_t* mod);

/* Determines, if two modulators are 'identical' (all parameters
   except the amount match) */
int fluid_mod_test_identity(fluid_mod_t * mod1, fluid_mod_t * mod2);

void fluid_mod_clone(fluid_mod_t* mod, fluid_mod_t* src);
fluid_real_t fluid_mod_get_value(fluid_mod_t* mod, fluid_channel_t* chan, fluid_voice_t* voice);
void fluid_dump_modulator(fluid_mod_t * mod);

#define fluid_mod_has_source(mod,cc,ctrl)  \
( ((((mod)->src1 == ctrl) && (((mod)->flags1 & FLUID_MOD_CC) != 0) && (cc != 0)) \
   || ((((mod)->src1 == ctrl) && (((mod)->flags1 & FLUID_MOD_CC) == 0) && (cc == 0)))) \
|| ((((mod)->src2 == ctrl) && (((mod)->flags2 & FLUID_MOD_CC) != 0) && (cc != 0)) \
    || ((((mod)->src2 == ctrl) && (((mod)->flags2 & FLUID_MOD_CC) == 0) && (cc == 0)))))

#define fluid_mod_has_dest(mod,gen)  ((mod)->dest == gen)

#endif /* _FLUID_MOD_H */
