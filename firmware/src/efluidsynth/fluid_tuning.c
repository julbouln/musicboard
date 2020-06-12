#include "fluid_tuning.h"
#include "fluid_log.h"

fluid_tuning_t* new_fluid_tuning(char* name, int bank, int prog)
{
  fluid_tuning_t* tuning;
  int i;

  tuning = FLUID_NEW(fluid_tuning_t);
  if (tuning == NULL) {
    FLUID_LOG(FLUID_PANIC, "Out of memory");
    return NULL;
  }

  tuning->name = NULL;

  if (name != NULL)
    tuning->name = FLUID_STRDUP(name);

  tuning->bank = bank;
  tuning->prog = prog;

  for (i = 0; i < 128; i++)
    tuning->pitch[i] = i * 100.0;

  return tuning;
}

void delete_fluid_tuning(fluid_tuning_t* tuning)
{
  if (tuning == NULL)
    return;

  if (tuning->name != NULL)
    FLUID_FREE(tuning->name);

  FLUID_FREE(tuning);
}

void fluid_tuning_set_name(fluid_tuning_t* tuning, char* name)
{
  if (tuning->name != NULL) {
    FLUID_FREE(tuning->name);
    tuning->name = NULL;
  }
  if (name != NULL)
    tuning->name = FLUID_STRDUP(name);
}

char* fluid_tuning_get_name(fluid_tuning_t* tuning)
{
  return tuning->name;
}

void fluid_tuning_set_key(fluid_tuning_t* tuning, int key, double pitch)
{
  tuning->pitch[key] = pitch;
}

void fluid_tuning_set_octave(fluid_tuning_t* tuning, double* pitch_deriv)
{
  int i;

  for (i = 0; i < 128; i++)
    tuning->pitch[i] = i * 100.0 + pitch_deriv[i % 12];
}

void fluid_tuning_set_all(fluid_tuning_t* tuning, double* pitch)
{
  int i;

  for (i = 0; i < 128; i++)
    tuning->pitch[i] = pitch[i];
}

void fluid_tuning_set_pitch(fluid_tuning_t* tuning, int key, double pitch)
{
  if ((key >= 0) && (key < 128))
    tuning->pitch[key] = pitch;
}
