#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>

void synth_init(void);
void synth_reset(void);
void synth_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize);

uint8_t synth_available(void);
#endif