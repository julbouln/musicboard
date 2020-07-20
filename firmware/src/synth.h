#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>

void synth_init(void);
void synth_buffer_init(void);
void synth_reset(void);
uint8_t synth_loading();
void synth_set_volume(float vol);
uint8_t synth_available(void);
void synth_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize);

void synth_render(uint32_t bufpos, uint32_t bufsize);
void synth_midi_process(uint8_t *msg, uint32_t len);
#endif