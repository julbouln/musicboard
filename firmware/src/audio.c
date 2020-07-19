#include "audio.h"
#include "audio_buffer.h"

void audio_init() {
}

void audio_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize) {
  int16_t *in = (int16_t *)audio_buffer_getptr(bufpos, bufsize);
  int16_t *out = (int16_t *)&buf[0] + bufpos / 2;
  
  int blkCnt = (bufsize / 2) >> 2;
  while (blkCnt--) {
    *out++ += *in++ >> 1;
    *out++ += *in++ >> 1;
    *out++ += *in++ >> 1;
    *out++ += *in++ >> 1;
  }
  
}
