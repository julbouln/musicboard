/*

  More information about micro tuning can be found at:

  http://www.midi.org/about-midi/tuning.htm
  http://www.midi.org/about-midi/tuning-scale.htm
  http://www.midi.org/about-midi/tuning_extens.htm

*/

#ifndef _FLUID_TUNING_H
#define _FLUID_TUNING_H

#include "fluid_types.h"

struct _fluid_tuning_t {
  char* name;
  int bank;
  int prog;
  double pitch[128];  /* the pitch of every key, in cents */
};

fluid_tuning_t* new_fluid_tuning(char* name, int bank, int prog);
void delete_fluid_tuning(fluid_tuning_t* tuning);

void fluid_tuning_set_name(fluid_tuning_t* tuning, char* name);
char* fluid_tuning_get_name(fluid_tuning_t* tuning);

#define fluid_tuning_get_bank(_t) ((_t)->bank)
#define fluid_tuning_get_prog(_t) ((_t)->prog)

void fluid_tuning_set_pitch(fluid_tuning_t* tuning, int key, double pitch);
#define fluid_tuning_get_pitch(_t, _key) ((_t)->pitch[_key])

void fluid_tuning_set_octave(fluid_tuning_t* tuning, double* pitch_deriv);

void fluid_tuning_set_all(fluid_tuning_t* tuning, double* pitch);
#define fluid_tuning_get_all(_t) (&(_t)->pitch[0])

#endif /* _FLUID_TUNING_H */
