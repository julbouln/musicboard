/***********************************************************************

                  -- USB-DAC for STM32F4 Discovery --

------------------------------------------------------------------------

	Audio stream buffer

	Copyright (c) 2016 Kiyoto.
	Strongly modified and commented by Lefucjusz, 2018
	This file is licensed under the MIT License. See /LICENSE.TXT file.

***********************************************************************/

#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include "usbd_audio.h"

#define AUDIO_BUFFER_SIZE		AUDIO_TOTAL_BUF_SIZE

void audio_buffer_init(void);
void audio_buffer_feed(uint8_t* buf, uint32_t size);
void audio_buffer_fill(uint32_t val, uint32_t size);
void audio_buffer_fill_next_ip(uint32_t val);
uint8_t* audio_buffer_getptr(uint32_t ofs, uint32_t size);
uint32_t audio_buffer_getfeedback(void);

extern volatile uint32_t audio_buffer_srate;
#endif
