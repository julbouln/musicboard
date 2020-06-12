#ifndef _FLUID_REV_H
#define _FLUID_REV_H

#include "fluid_types.h"

typedef struct _fluid_revmodel_t fluid_revmodel_t;

/*
 * reverb
 */
fluid_revmodel_t* new_fluid_revmodel(void);
void delete_fluid_revmodel(fluid_revmodel_t* rev);

void fluid_revmodel_processmix(fluid_revmodel_t* rev, fluid_buf_t *in,
			      fluid_buf_t *left_out, fluid_buf_t *right_out);

void fluid_revmodel_reset(fluid_revmodel_t* rev);

void fluid_revmodel_setroomsize(fluid_revmodel_t* rev, fluid_real_t value);
void fluid_revmodel_setdamp(fluid_revmodel_t* rev, fluid_real_t value);
void fluid_revmodel_setlevel(fluid_revmodel_t* rev, fluid_real_t value);
void fluid_revmodel_setwidth(fluid_revmodel_t* rev, fluid_real_t value);
void fluid_revmodel_setmode(fluid_revmodel_t* rev, fluid_real_t value);

fluid_real_t fluid_revmodel_getroomsize(fluid_revmodel_t* rev);
fluid_real_t fluid_revmodel_getdamp(fluid_revmodel_t* rev);
fluid_real_t fluid_revmodel_getlevel(fluid_revmodel_t* rev);
fluid_real_t fluid_revmodel_getwidth(fluid_revmodel_t* rev);

/*
 * reverb preset
 */
typedef struct _fluid_revmodel_presets_t {
  char* name;
  fluid_real_t roomsize;
  fluid_real_t damp;
  fluid_real_t width;
  fluid_real_t level;
} fluid_revmodel_presets_t;

#endif /* _FLUID_REV_H */
