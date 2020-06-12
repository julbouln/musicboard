#ifndef _FLUID_CHORUS_H
#define _FLUID_CHORUS_H

#include "fluid_types.h"

typedef struct _fluid_chorus_t fluid_chorus_t;

/*
 * chorus
 */
fluid_chorus_t* new_fluid_chorus(fluid_real_t sample_rate);
void delete_fluid_chorus(fluid_chorus_t* chorus);
void fluid_chorus_processmix(fluid_chorus_t* chorus, fluid_buf_t *in,
			    fluid_buf_t *left_out, fluid_buf_t *right_out);

int fluid_chorus_init(fluid_chorus_t* chorus);
void fluid_chorus_reset(fluid_chorus_t* chorus);

void fluid_chorus_set_nr(fluid_chorus_t* chorus, int nr);
void fluid_chorus_set_level(fluid_chorus_t* chorus, fluid_real_t level);
void fluid_chorus_set_speed_Hz(fluid_chorus_t* chorus, fluid_real_t speed_Hz);
void fluid_chorus_set_depth_ms(fluid_chorus_t* chorus, fluid_real_t depth_ms);
void fluid_chorus_set_type(fluid_chorus_t* chorus, int type);
int fluid_chorus_update(fluid_chorus_t* chorus);
int fluid_chorus_get_nr(fluid_chorus_t* chorus);
fluid_real_t fluid_chorus_get_level(fluid_chorus_t* chorus);
fluid_real_t fluid_chorus_get_speed_Hz(fluid_chorus_t* chorus);
fluid_real_t fluid_chorus_get_depth_ms(fluid_chorus_t* chorus);
int fluid_chorus_get_type(fluid_chorus_t* chorus);

#endif /* _FLUID_CHORUS_H */
