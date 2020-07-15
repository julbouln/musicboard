/***********************************************************************

                  -- USB-DAC for STM32F4 Discovery --

------------------------------------------------------------------------

	Audio stream buffer

	Copyright (c) 2016 Kiyoto.
	Strongly modified and commented by Lefucjusz, 2018
	This file is licensed under the MIT License. See /LICENSE.TXT file.

***********************************************************************/

#include "audio_buffer.h"

//Buffer - data stores packets, index shows where the next data packet can be stored
struct
{
	uint8_t data[AUDIO_BUFFER_SIZE];
	uint32_t index;
} audio_buffer __attribute__ ((aligned(128)));

volatile uint32_t audio_buffer_srate = USBD_AUDIO_FREQ; //Sampling rate

//Counters used to manage the number of bytes that have flown into/out of the buffer
volatile int32_t audio_buffer_count = 0; //Update the value during both in and out data flow
volatile int32_t audio_buffer_count_fixed = 0; //Update only when the data flow out of the buffer

//Init buffer
void audio_buffer_init(void)
{
	audio_buffer.index = 0;
	audio_buffer_count = 0;
	audio_buffer_count_fixed = 0;
}

//Function preparing the incoming packets and feeding the buffer during playback
void audio_buffer_feed(uint8_t *buf, uint32_t size)
{
	int16_t *in_data, *out_data; //Pointers used during the packet preparation
	uint32_t i = audio_buffer.index; //Store the index of the beginning of free space in buffer
	const uint32_t data_size = size; //Store the size (in bytes) of the currently processed packet

	in_data = (void*)buf; //Treat the processed packets as stream of 32-bit samples
	out_data = (void*)(&(audio_buffer.data[audio_buffer.index])); //Treat the buffer in the same way

	do //Process and store in buffer every 32-bit sample from packet...
	{
		//Sample should be send in upper halfword - lower halfword order, so swap the halfwords
		*out_data = *in_data;
		//Move incoming sample pointer to next sample
		in_data++;
		//Move the circular buffer index four positions
		//because buffer cells are uint8_t and sample is uint32_t - 32/8 = 4
		i += 2;
		//If we are on the end of matrix emulating the circular buffer
		//go to the beginning - close the circle
		i = (i >= AUDIO_BUFFER_SIZE) ? 0 : i;
		//Obtain the address of next sample to be processed
		out_data = (void*)(&(audio_buffer.data[i]));
	} while (size -= 2); //...until you run out of samples
	//(subtract 4 because size is in bytes (uint8_t) and sample has 4 bytes (uint32_t)

	//Update the buffer free space index
	audio_buffer.index = (audio_buffer.index + data_size) % AUDIO_BUFFER_SIZE;
	//Buffer now stores packet_size bytes more
	audio_buffer_count += data_size;
}

//Fill the size/4 samples in buffer with given value
void audio_buffer_fill(uint32_t val, uint32_t size)
{
	uint32_t i = audio_buffer.index; //Store the index of the beginning of free space in buffer
	const uint32_t data_size = size; //Store the size of the data to be stored in buffer
	
	do //For every sample..
	{
		//Store the given value in buffer
		*((int16_t*)(&(audio_buffer.data[i]))) = val;
		//Move the circular buffer index four positions
		//because buffer cells are uint8_t and sample is uint32_t - 32/8 = 4
		i += 2;
		//If we are on the end of matrix emulating the circular buffer
		//go to the beginning - close the circle
		i = (i >= AUDIO_BUFFER_SIZE) ? 0 : i;
	} while (size -= 2);//...until you run out of samples
	//(subtract 4 because size is in bytes (uint8_t) and sample has 4 bytes (uint32_t)
	
	//Update the buffer free space index
	audio_buffer.index = (audio_buffer.index + data_size) % AUDIO_BUFFER_SIZE;
	//Buffer now stores data_size bytes more
	audio_buffer_count += data_size;
}

//Function used to from buffer, returns the pointer to buffer of specified offset
//Also updates the value used to calculate feedback, so has to be called every transfer completion event
uint8_t* audio_buffer_getptr(uint32_t ofs, uint32_t size)
{
	if (ofs < AUDIO_BUFFER_SIZE) //If offset is in the buffer range...
	{
		//Decrease the number of bytes stored in buffer by the size of the packet to be sent
		audio_buffer_count -= size;
		//Handle buffer overflow
		while (audio_buffer_count >= (int32_t)AUDIO_BUFFER_SIZE)
		{
			audio_buffer_count -= AUDIO_BUFFER_SIZE;
		}
		//Handle buffer underflow
		while (audio_buffer_count < 0)
		{
			audio_buffer_count += AUDIO_BUFFER_SIZE;
		}
		//Store the value of the packet to be send - used later to calculate feedback value
		audio_buffer_count_fixed = audio_buffer_count;
		
		//Return the pointer to the beginning of packet in buffer
		return &(audio_buffer.data[ofs]);
	}
	return (void*)0;//...otherwise return NULL
}

//Returns the calculated feedback value in 10.14 standard (for USB2.0 fullspeed audio endpoint)
/*

Ff is expressed in number of samples per (micro)frame for one channel. The Ff value consists of:
  - an integer part that represents the (integer) number of samples per (micro)frame and, 
  - a fractional part that represents the "fraction" of a sample that would be needed to match the sampling frequency Fs to a resolution of 1 Hz or better.

31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14             13 12 11 10 9 8 7 6 5 4 3 2 1 0
 0  0  0  0  0  0  0  0 Nbr of samples per frame for one channel  Fraction of a sample    0 0 0 0

*/
uint32_t audio_buffer_getfeedback(void)
{
#if 0
	int32_t diff;
	diff = ((int32_t)(AUDIO_BUFFER_SIZE / 2) - audio_buffer_count_fixed); //Calculate the byte difference
//	diff /= 4; //Convert it to the number of samples (2 channels * 4 bytes per sample = 8)
	diff <<= 14; //Align the integer part to fit 10.14 standard
	diff /= (AUDIO_BUFFER_SIZE / 2);	//??? Convert to values around 1ms
	//Obtain the desirable Fs value and return it in 10.14 format
//	return diff + (((audio_buffer_srate << 10) / 1000) << 4);
	return (48 << 14) + diff;
#endif
	int32_t diff;
	diff = ((int32_t)(AUDIO_BUFFER_SIZE / 2) - audio_buffer_count_fixed); //Calculate the byte difference

	if(diff <= AUDIO_BUFFER_SIZE/4) {
		diff = -(1 << 14);
	} else if(diff >= AUDIO_BUFFER_SIZE*3/4) {
		diff = (1 << 14);
	} else {
		diff = 0;
	}
	return (48 << 14) + diff;
}
