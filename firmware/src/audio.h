#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void audio_init(void);
void audio_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize);

#endif