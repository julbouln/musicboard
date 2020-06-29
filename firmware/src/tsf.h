/* TinySoundFont - v0.8 - SoundFont2 synthesizer - https://github.com/schellingb/TinySoundFont
                                     no warranty implied; use at your own risk

   NOTE: Modified version to reduce memory usage and with ARM optimization.

   Do this:
      #define TSF_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #define TSF_IMPLEMENTATION
   #include "tsf.h"

   [OPTIONAL] #define TSF_NO_STDIO to remove stdio dependency
   [OPTIONAL] #define TSF_MALLOC, TSF_REALLOC, and TSF_FREE to avoid stdlib.h
   [OPTIONAL] #define TSF_MEMCPY, TSF_MEMSET to avoid string.h
   [OPTIONAL] #define TSF_POW, TSF_POWF, TSF_EXPF, TSF_LOG, TSF_TAN, TSF_LOG10, TSF_SQRT to avoid math.h

   NOT YET IMPLEMENTED
     - Better low-pass filter without lowering performance too much
     - Support for modulators

   LICENSE (MIT)

   Copyright (C) 2017, 2018 Bernhard Schelling
   Based on SFZero, Copyright (C) 2012 Steve Folta (https://github.com/stevefolta/SFZero)

   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the Software
   without restriction, including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
   to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef TSF_INCLUDE_TSF_INL
#define TSF_INCLUDE_TSF_INL

#ifdef __cplusplus
extern "C" {
#  define CPP_DEFAULT0 = 0
#else
#  define CPP_DEFAULT0
#endif

//define this if you want the API functions to be static
#ifdef TSF_STATIC
#define TSFDEF static
#else
#define TSFDEF extern
#endif

#include <stdint.h>

// The load functions will return a pointer to a struct tsf which all functions
// thereafter take as the first parameter.
// On error the tsf_load* functions will return NULL most likely due to invalid
// data (or if the file did not exist in tsf_load_filename).
typedef struct tsf tsf;

#ifndef TSF_NO_STDIO
// Directly load a SoundFont from a .sf2 file path
TSFDEF tsf* tsf_load_filename(const char* filename);
#endif

// Load a SoundFont from a block of memory
TSFDEF tsf* tsf_load_memory(const void* buffer, int32_t size);

// Stream structure for the generic loading
struct tsf_stream
{
	// Custom data given to the functions as the first parameter
	void* data;

	// Function pointer will be called to read 'size' bytes into ptr (returns number of read bytes)
	int32_t (*read)(void* data, void* ptr, uint32_t size);

	// Function pointer will be called to skip ahead over 'count' bytes (returns 1 on success, 0 on error)
	int32_t (*skip)(void* data, uint32_t count);

	int32_t (*tell)(void* data);
	int32_t (*seek)(void* data, uint32_t count);
};

// Generic SoundFont loading method using the stream structure above
TSFDEF tsf* tsf_load(struct tsf_stream* stream);

// Free the memory related to this tsf instance
TSFDEF void tsf_close(tsf* f);

// Stop all playing notes immediatly and reset all channel parameters
TSFDEF void tsf_reset(tsf* f);

// Returns the preset index from a bank and preset number, or -1 if it does not exist in the loaded SoundFont
TSFDEF int32_t tsf_get_presetindex(const tsf* f, int32_t bank, int32_t preset_number);

// Returns the number of presets in the loaded SoundFont
TSFDEF int32_t tsf_get_presetcount(const tsf* f);

// Returns the name of a preset index >= 0 and < tsf_get_presetcount()
TSFDEF const char* tsf_get_presetname(const tsf* f, int32_t preset_index);

// Returns the name of a preset by bank and preset number
TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int32_t bank, int32_t preset_number);

// Supported output modes by the render methods
enum TSFOutputMode
{
	// Two channels with single left/right samples one after another
	TSF_STEREO_INTERLEAVED,
	// Two channels with all samples for the left channel first then right
	TSF_STEREO_UNWEAVED,
	// A single channel (stereo instruments are mixed into center)
	TSF_MONO,
};

// Thread safety:
// Your audio output which calls the tsf_render* functions will most likely
// run on a different thread than where the playback tsf_note* functions
// are called. In which case some sort of concurrency control like a
// mutex needs to be used so they are not called at the same time.

// Setup the parameters for the voice render methods
//   outputmode: if mono or stereo and how stereo channel data is ordered
//   samplerate: the number of samples per second (output frequency)
//   global_gain_db: volume gain in decibels (>0 means higher, <0 means lower)
TSFDEF void tsf_set_output(tsf* f, enum TSFOutputMode outputmode, int32_t samplerate, float global_gain_db CPP_DEFAULT0);

// Start playing a note
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
//   bank: instrument bank number (alternative to preset_index)
//   preset_number: preset number (alternative to preset_index)
//   (bank_note_on returns 0 if preset does not exist, otherwise 1)
TSFDEF void tsf_note_on(tsf* f, int32_t preset_index, int32_t key, float vel);
TSFDEF int32_t  tsf_bank_note_on(tsf* f, int32_t bank, int32_t preset_number, int32_t key, float vel);

// Stop playing a note
//   (bank_note_off returns 0 if preset does not exist, otherwise 1)
TSFDEF void tsf_note_off(tsf* f, int32_t preset_index, int32_t key);
TSFDEF int32_t  tsf_bank_note_off(tsf* f, int32_t bank, int32_t preset_number, int32_t key);

// Stop playing all notes (end with sustain and release)
TSFDEF void tsf_note_off_all(tsf* f);

// Returns the number of active voices
TSFDEF int32_t tsf_active_voice_count(tsf* f);

// Render output samples into a buffer
// You can either render as signed 16-bit values (tsf_render_short) or
// as 32-bit float values (tsf_render_float)
//   buffer: target buffer of size samples * output_channels * sizeof(type)
//   samples: number of samples to render
//   flag_mixing: if 0 clear the buffer first, otherwise mix into existing data
TSFDEF void tsf_render_short(tsf* f, int16_t* buffer, int32_t samples, int32_t flag_mixing CPP_DEFAULT0);

// Higher level channel based functions, set up channel parameters
//   channel: channel number
//   preset_index: preset index >= 0 and < tsf_get_presetcount()
//   preset_number: preset number (alternative to preset_index)
//   flag_mididrums: 0 for normal channels, otherwise apply MIDI drum channel rules
//   bank: instrument bank number (alternative to preset_index)
//   pan: stereo panning value from 0.0 (left) to 1.0 (right) (default 0.5 center)
//   volume: linear volume scale factor (default 1.0 full)
//   pitch_wheel: pitch wheel position 0 to 16383 (default 8192 unpitched)
//   pitch_range: range of the pitch wheel in semitones (default 2.0, total +/- 2 semitones)
//   tuning: tuning of all playing voices in semitones (default 0.0, standard (A440) tuning)
//   (set_preset_number and set_bank_preset return 0 if preset does not exist, otherwise 1)
TSFDEF void tsf_channel_set_presetindex(tsf* f, int32_t channel, int32_t preset_index);
TSFDEF int32_t  tsf_channel_set_presetnumber(tsf* f, int32_t channel, int32_t preset_number, int32_t flag_mididrums CPP_DEFAULT0);
TSFDEF void tsf_channel_set_bank(tsf* f, int32_t channel, int32_t bank);
TSFDEF int32_t  tsf_channel_set_bank_preset(tsf* f, int32_t channel, int32_t bank, int32_t preset_number);
TSFDEF void tsf_channel_set_pan(tsf* f, int32_t channel, float pan);
TSFDEF void tsf_channel_set_volume(tsf* f, int32_t channel, float volume);
TSFDEF void tsf_channel_set_pitchwheel(tsf* f, int32_t channel, int32_t pitch_wheel);
TSFDEF void tsf_channel_set_pitchrange(tsf* f, int32_t channel, float pitch_range);
TSFDEF void tsf_channel_set_tuning(tsf* f, int32_t channel, float tuning);

// Start or stop playing notes on a channel (needs channel preset to be set)
//   channel: channel number
//   key: note value between 0 and 127 (60 being middle C)
//   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
TSFDEF void tsf_channel_note_on(tsf* f, int32_t channel, int32_t key, float vel);
TSFDEF void tsf_channel_note_off(tsf* f, int32_t channel, int32_t key);
TSFDEF void tsf_channel_note_off_all(tsf* f, int32_t channel); //end with sustain and release
TSFDEF void tsf_channel_sounds_off_all(tsf* f, int32_t channel); //end immediatly

// Apply a MIDI control change to the channel (not all controllers are supported!)
TSFDEF void tsf_channel_midi_control(tsf* f, int32_t channel, int32_t controller, int32_t control_value);

// Get current values set on the channels
TSFDEF int32_t tsf_channel_get_preset_index(tsf* f, int32_t channel);
TSFDEF int32_t tsf_channel_get_preset_bank(tsf* f, int32_t channel);
TSFDEF int32_t tsf_channel_get_preset_number(tsf* f, int32_t channel);
TSFDEF float tsf_channel_get_pan(tsf* f, int32_t channel);
TSFDEF float tsf_channel_get_volume(tsf* f, int32_t channel);
TSFDEF int32_t tsf_channel_get_pitchwheel(tsf* f, int32_t channel);
TSFDEF float tsf_channel_get_pitchrange(tsf* f, int32_t channel);
TSFDEF float tsf_channel_get_tuning(tsf* f, int32_t channel);

#ifdef __cplusplus
#  undef CPP_DEFAULT0
}
#endif

// end header
// ---------------------------------------------------------------------------------------------------------
#endif //TSF_INCLUDE_TSF_INL

#ifdef TSF_IMPLEMENTATION

// The lower this block size is the more accurate the effects are.
// Increasing the value significantly lowers the CPU usage of the voice rendering.
// If LFO affects the low-pass filter it can be hearable even as low as 8.
#ifndef TSF_RENDER_EFFECTSAMPLEBLOCK
#define TSF_RENDER_EFFECTSAMPLEBLOCK 64
#endif

// Grace release time for quick voice off (avoid clicking noise)
#define TSF_FASTRELEASETIME 0.01f

// Reuse voice < level
#define TSF_REUSE_LEVEL 0.1f

// execute GC every TSF_GC_F render
#define TSF_GC_F 1024

#define TSF_MAX_SAMPLES 2048

//#define TSF_MEM_PROF // quick and dirty memory profile

#ifdef TSF_MEM_PROF
#include <stdlib.h>
#include <malloc.h>
uint64_t cur_mem = 0;
uint64_t max_mem = 0;

void *prof_malloc(uint64_t size) {
	cur_mem += size;
	if (cur_mem > max_mem)
		max_mem = cur_mem;
	printf("MALLOC %ld/%ld\n", cur_mem, max_mem);
	return malloc(size);
}

void *prof_realloc(void *ptr, uint64_t size) {
	if (ptr)
		cur_mem -= malloc_usable_size(ptr);
	cur_mem += size;
	if (cur_mem > max_mem)
		max_mem = cur_mem;
	printf("REALLOC %ld/%ld\n", cur_mem, max_mem);
	return realloc(ptr, size);
}

void prof_free(void *ptr) {
	cur_mem -= malloc_usable_size(ptr);
	printf("FREE %ld/%ld\n", cur_mem, max_mem);
}
#endif

#if !defined(TSF_MALLOC) || !defined(TSF_FREE) || !defined(TSF_REALLOC)
#include <stdlib.h>
#ifdef TSF_MEM_PROF
#define TSF_MALLOC  prof_malloc
#define TSF_FREE    prof_free
#define TSF_REALLOC prof_realloc
#else
#define TSF_MALLOC  malloc
#define TSF_FREE    free
#define TSF_REALLOC realloc
#endif
#endif

#if !defined(TSF_MEMCPY) || !defined(TSF_MEMSET)
#include <string.h>
#define TSF_MEMCPY  memcpy
#define TSF_MEMSET  memset
#endif

#if !defined(TSF_FILE) || !defined(TSF_MMAP)
#include <sys/mman.h>
#define TSF_FILE FILE
#define TSF_FOPEN fopen
#define TSF_FREAD fread
#define TSF_FTELL ftell
#define TSF_FSEEK fseek
#define TSF_MMAP(p,s,f)  mmap(0, s, PROT_READ, MAP_SHARED, fileno(f), 0);
#endif

#if !defined(TSF_POWF) || !defined(TSF_EXPF) || !defined(TSF_LOG) || !defined(TSF_TAN) || !defined(TSF_LOG10) || !defined(TSF_SQRT)
#include <math.h>
#if !defined(__cplusplus) && !defined(NAN) && !defined(powf) && !defined(expf) && !defined(sqrtf)
#define powf (float)pow // deal with old math.h
#define expf (float)exp // files that come without
#define sqrtf (float)sqrt // powf, expf and sqrtf
#endif
#define TSF_POWF    powf
#define TSF_EXPF    expf
#define TSF_LOG     log
#define TSF_TAN     tan
#define TSF_LOG10   log10
#define TSF_SQRTF   sqrtf
#endif

#ifndef TSF_NO_STDIO
#  include <stdio.h>
#endif

#define TSF_TRUE 1
#define TSF_FALSE 0
#define TSF_BOOL char
#define TSF_PI 3.14159265358979323846264338327950288
#define TSF_NULL 0

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ARM_FEATURE_DSP

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __SMULBB(uint32_t op1, uint32_t op2)
{
	uint32_t result;

	__ASM volatile ("smulbb %0, %1, %2" : "=r" (result) : "r" (op1), "r" (op2) );
	return (result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __SMLABB (uint32_t op1, uint32_t op2, uint32_t op3)
{
	uint32_t result;

	__ASM volatile ("smlabb %0, %1, %2, %3" : "=r" (result) : "r" (op1), "r" (op2), "r" (op3) );
	return (result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __SMLABT (uint32_t op1, uint32_t op2, uint32_t op3)
{
	uint32_t result;

	__ASM volatile ("smlabt %0, %1, %2, %3" : "=r" (result) : "r" (op1), "r" (op2), "r" (op3) );
	return (result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __SMLATB (uint32_t op1, uint32_t op2, uint32_t op3)
{
	uint32_t result;

	__ASM volatile ("smlatb %0, %1, %2, %3" : "=r" (result) : "r" (op1), "r" (op2), "r" (op3) );
	return (result);
}

__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __SMLATT (uint32_t op1, uint32_t op2, uint32_t op3)
{
	uint32_t result;

	__ASM volatile ("smlatt %0, %1, %2, %3" : "=r" (result) : "r" (op1), "r" (op2), "r" (op3) );
	return (result);
}

#else

#define __PKHBT(ARG1, ARG2, ARG3) ( (((int32_t)(ARG1) <<    0) & (int32_t)0x0000FFFF) | \
                                      (((int32_t)(ARG2) << ARG3) & (int32_t)0xFFFF0000)  )
#define __PKHTB(ARG1, ARG2, ARG3) ( (((int32_t)(ARG1) <<    0) & (int32_t)0xFFFF0000) | \
                                      (((int32_t)(ARG2) >> ARG3) & (int32_t)0x0000FFFF)  )

static int32_t __SSAT(int32_t x, int32_t y)
{
	if (x > (1 << (y - 1)) - 1) return (1 << (y - 1)) - 1;
	else if (x < -(1 << (y - 1))) return -(1 << (y - 1)) + 1;
	else return (int32_t)x;
}

static inline uint32_t __SMUAD(uint32_t x, uint32_t y)
{
	return ((uint32_t)(((((int32_t)x << 16) >> 16) * (((int32_t)y << 16) >> 16)) +
	                   ((((int32_t)x      ) >> 16) * (((int32_t)y      ) >> 16))   ));
}

static inline uint32_t __SMUSD(uint32_t x, uint32_t y)
{
	return ((uint32_t)(((((int32_t)x << 16) >> 16) * (((int32_t)y << 16) >> 16)) -
	                   ((((int32_t)x      ) >> 16) * (((int32_t)y      ) >> 16))   ));
}

static inline int32_t __SMULBB(int32_t x, int32_t y) {
	return ((int32_t)((int16_t) (x & 0xFFFF)) * (int32_t)((int16_t) (y & 0xFFFF)));
}

static inline int32_t __SMLABB(int32_t x, int32_t y, int32_t z) {
	return ((int32_t)((int16_t) (x & 0xFFFF)) * (int32_t)((int16_t) (y & 0xFFFF)) + z);
}

static inline int32_t __SMLABT(int32_t x, int32_t y, int32_t z) {
	return ((int32_t)((int16_t) (x & 0xFFFF)) * (int32_t)((int16_t) ((y >> 16) & 0xFFFF)) + z);
}

static inline int32_t __SMLATB(int32_t x, int32_t y, int32_t z) {
	return ((int32_t)((int16_t) ((x >> 16) & 0xFFFF)) * (int32_t)((int16_t) (y & 0xFFFF)) + z);
}

static inline int32_t __SMLATT(int32_t x, int32_t y, int32_t z) {
	return ((int32_t)((int16_t) ((x >> 16) & 0xFFFF)) * (int32_t)((int16_t) ((y >> 16) & 0xFFFF))  + z);
}

#endif

#define float_to_fixed(v) (int32_t)((v) * 32768.0f)
#define fixed_to_float(v) (float)((v) / 32768.0f)

#define float_to_fixed64(v) (uint64_t)((v) * 4294967296.0f)
#define fixed64_to_float(v) (float)((v) / 4294967296.0f)

#ifndef TSF_NO_REVERB
#include "reverb.h"
#endif

#ifndef TSF_NO_CHORUS
#include "chorus.h"
#endif

typedef char tsf_fourcc[4];
typedef int8_t tsf_s8;
typedef uint8_t tsf_u8;
typedef uint16_t tsf_u16;
typedef int16_t tsf_s16;
typedef uint32_t tsf_u32;
typedef char tsf_char20[20];

#define TSF_FourCCEquals(value1, value2) (value1[0] == value2[0] && value1[1] == value2[1] && value1[2] == value2[2] && value1[3] == value2[3])

struct tsf_hydra
{
	int32_t phdrNum, pbagNum, pmodNum, pgenNum, instNum, ibagNum, imodNum, igenNum, shdrNum;
	int32_t phdrPos, pbagPos, pmodPos, pgenPos, instPos, ibagPos, imodPos, igenPos, shdrPos;
};

struct tsf
{
	struct tsf_preset* presets;
	struct tsf_stream* stream;
	struct tsf_hydra hydra;
	int16_t* fontSamples;
	uint32_t fontSamplesOffset;
	uint32_t fontSampleCount;
	struct tsf_voice* voices;
	struct tsf_channels* channels;

	int32_t presetNum;
	int32_t voiceNum;
	int32_t voicesMax;
	int32_t outputSampleSize;
	uint32_t voicePlayIndex;

	enum TSFOutputMode outputmode;
	float outSampleRate;
	float globalGainDB;

	uint16_t gc;
#ifndef TSF_NO_REVERB
	reverb_t rev;
#endif
#ifndef TSF_NO_CHORUS
	chorus_t chorus;
#endif

	int32_t buffer[TSF_MAX_SAMPLES * 2];
	int32_t chorusBuffer[TSF_MAX_SAMPLES];
	int32_t reverbBuffer[TSF_MAX_SAMPLES];

};

#ifndef TSF_NO_STDIO
static int32_t tsf_stream_stdio_read(TSF_FILE* f, void* ptr, uint32_t size) { return (int32_t)TSF_FREAD(ptr, 1, size, f); }
static int32_t tsf_stream_stdio_skip(TSF_FILE* f, uint32_t count) { return !TSF_FSEEK(f, count, SEEK_CUR); }
static int32_t tsf_stream_stdio_tell(TSF_FILE* f, uint32_t count) { return TSF_FTELL(f); }
static int32_t tsf_stream_stdio_seek(TSF_FILE* f, uint32_t count) { return TSF_FSEEK(f, count, SEEK_SET); }
TSFDEF tsf* tsf_load_filename(const char* filename)
{
	tsf* res;
	struct tsf_stream *stream = (struct tsf_stream *)TSF_MALLOC(sizeof(struct tsf_stream));
	stream->read = (int32_t(*)(void*, void*, uint32_t))&tsf_stream_stdio_read;
	stream->skip = (int32_t(*)(void*, uint32_t))&tsf_stream_stdio_skip;
	stream->tell = (int32_t(*)(void *))&tsf_stream_stdio_tell;
	stream->seek = (int32_t(*)(void*, uint32_t))&tsf_stream_stdio_seek;

	TSF_FILE* f = TSF_FOPEN(filename, "rb");
	if (!f)
	{
		//if (e) *e = TSF_FILENOTFOUND;
		return TSF_NULL;
	}
	stream->data = f;
	res = tsf_load(stream);
	//fclose(f);
	return res;
}
#endif

enum { TSF_LOOPMODE_NONE, TSF_LOOPMODE_CONTINUOUS, TSF_LOOPMODE_SUSTAIN };

enum { TSF_SEGMENT_NONE, TSF_SEGMENT_DELAY, TSF_SEGMENT_ATTACK, TSF_SEGMENT_HOLD, TSF_SEGMENT_DECAY, TSF_SEGMENT_SUSTAIN, TSF_SEGMENT_RELEASE, TSF_SEGMENT_DONE };

enum
{
	phdrSizeInFile = 38, pbagSizeInFile =  4, pmodSizeInFile = 10,
	pgenSizeInFile =  4, instSizeInFile = 22, ibagSizeInFile =  4,
	imodSizeInFile = 10, igenSizeInFile =  4, shdrSizeInFile = 46
};

union tsf_hydra_genamount { struct { tsf_u8 lo, hi; } range; tsf_s16 shortAmount; tsf_u16 wordAmount; };
struct tsf_hydra_phdr { tsf_char20 presetName; tsf_u16 preset, bank, presetBagNdx; tsf_u32 library, genre, morphology; };
struct tsf_hydra_pbag { tsf_u16 genNdx, modNdx; };
struct tsf_hydra_pmod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_pgen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_inst { tsf_char20 instName; tsf_u16 instBagNdx; };
struct tsf_hydra_ibag { tsf_u16 instGenNdx, instModNdx; };
struct tsf_hydra_imod { tsf_u16 modSrcOper, modDestOper; tsf_s16 modAmount; tsf_u16 modAmtSrcOper, modTransOper; };
struct tsf_hydra_igen { tsf_u16 genOper; union tsf_hydra_genamount genAmount; };
struct tsf_hydra_shdr { tsf_char20 sampleName; tsf_u32 start, end, startLoop, endLoop, sampleRate; tsf_u8 originalPitch; tsf_s8 pitchCorrection; tsf_u16 sampleLink, sampleType; };

#define TSFR(FIELD) stream->read(stream->data, &i->FIELD, sizeof(i->FIELD));
static void tsf_hydra_read_phdr(struct tsf_hydra_phdr* i, struct tsf_stream* stream) { TSFR(presetName) TSFR(preset) TSFR(bank) TSFR(presetBagNdx) TSFR(library) TSFR(genre) TSFR(morphology) }
static void tsf_hydra_read_pbag(struct tsf_hydra_pbag* i, struct tsf_stream* stream) { TSFR(genNdx) TSFR(modNdx) }
static void tsf_hydra_read_pmod(struct tsf_hydra_pmod* i, struct tsf_stream* stream) { TSFR(modSrcOper) TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper) }
static void tsf_hydra_read_pgen(struct tsf_hydra_pgen* i, struct tsf_stream* stream) { TSFR(genOper) TSFR(genAmount) }
static void tsf_hydra_read_inst(struct tsf_hydra_inst* i, struct tsf_stream* stream) { TSFR(instName) TSFR(instBagNdx) }
static void tsf_hydra_read_ibag(struct tsf_hydra_ibag* i, struct tsf_stream* stream) { TSFR(instGenNdx) TSFR(instModNdx) }
static void tsf_hydra_read_imod(struct tsf_hydra_imod* i, struct tsf_stream* stream) { TSFR(modSrcOper) TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper) }
static void tsf_hydra_read_igen(struct tsf_hydra_igen* i, struct tsf_stream* stream) { TSFR(genOper) TSFR(genAmount) }
static void tsf_hydra_read_shdr(struct tsf_hydra_shdr* i, struct tsf_stream* stream) { TSFR(sampleName) TSFR(start) TSFR(end) TSFR(startLoop) TSFR(endLoop) TSFR(sampleRate) TSFR(originalPitch) TSFR(pitchCorrection) TSFR(sampleLink) TSFR(sampleType) }
#undef TSFR

struct tsf_riffchunk { tsf_fourcc id; tsf_u32 size; };
struct tsf_envelope { float delay, attack, hold, decay, sustain, release, keynumToHold, keynumToDecay; };
struct tsf_voice_envelope { float level, slope; int32_t samplesUntilNextSegment; int16_t segment, midiVelocity; struct tsf_envelope parameters; TSF_BOOL segmentIsExponential, isAmpEnv; };
struct tsf_voice_lowpass { float QInv; int32_t a0, a1, b1, b2; int32_t z1, z2; TSF_BOOL active; };
struct tsf_voice_lfo { int32_t samplesUntil; float level, delta; };

struct tsf_region
{
	int16_t loop_mode;
	uint32_t sample_rate;
	uint8_t lokey, hikey, lovel, hivel;
	uint32_t group, offset, end, loop_start, loop_end;
	int16_t transpose, tune, pitch_keycenter, pitch_keytrack;
	float attenuation, pan;
	struct tsf_envelope ampenv, modenv;
	int16_t initialFilterQ, initialFilterFc;
	int16_t modEnvToPitch, modEnvToFilterFc, modLfoToFilterFc, modLfoToVolume;
	float delayModLFO;
	int16_t freqModLFO, modLfoToPitch;
	float delayVibLFO;
	int16_t freqVibLFO, vibLfoToPitch;
};

struct tsf_preset
{
#ifndef TSF_NO_PRESET_NAME
	tsf_char20 presetName;
#endif
	tsf_u16 preset, bank;
	struct tsf_region* regions;
	int32_t regionNum;
	int32_t pphdrIdx;
	TSF_BOOL loaded;
	uint16_t refCount;
};

struct tsf_voice
{
	int32_t playingPreset, playingKey, playingChannel;
	struct tsf_region* region;
	float pitchInputTimecents, pitchOutputFactor;
	uint64_t sourceSamplePosition;
	float  noteGainDB, panFactorLeft, panFactorRight;
	uint32_t playIndex, loopStart, loopEnd;
	uint8_t locked;
	struct tsf_voice_envelope ampenv, modenv;
	struct tsf_voice_lowpass lowpass;
	struct tsf_voice_lfo modlfo, viblfo;
};

struct tsf_channel
{
	uint16_t presetIndex, bank, pitchWheel, midiPan, midiVolume, midiExpression, midiRPN, midiData;
	float reverb, chorus;
	float panOffset, gainDB, pitchRange, tuning;
	int16_t buffer[TSF_RENDER_EFFECTSAMPLEBLOCK];
};

struct tsf_channels
{
	void (*setupVoice)(tsf* f, struct tsf_voice* voice);
	struct tsf_channel* channels;
	int32_t channelNum, activeChannel;
};

static float tsf_timecents2Secsf(float timecents) { return TSF_POWF(2.0f, timecents / 1200.0f); }
static float tsf_cents2Hertz(float cents) { return 8.176f * TSF_POWF(2.0f, cents / 1200.0f); }
static float tsf_decibelsToGain(float db) { return (db > -100.f ? TSF_POWF(10.0f, db * 0.05f) : 0); }
static float tsf_gainToDecibels(float gain) { return (gain <= .00001f ? -100.f : (float)(20.0 * TSF_LOG10(gain))); }

static TSF_BOOL tsf_riffchunk_read(struct tsf_riffchunk* parent, struct tsf_riffchunk* chunk, struct tsf_stream* stream)
{
	TSF_BOOL IsRiff, IsList;
	if (parent && sizeof(tsf_fourcc) + sizeof(tsf_u32) > parent->size) return TSF_FALSE;
	if (!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	if (!stream->read(stream->data, &chunk->size, sizeof(tsf_u32))) return TSF_FALSE;
	if (parent && sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size > parent->size) return TSF_FALSE;
	if (parent) parent->size -= sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size;
	IsRiff = TSF_FourCCEquals(chunk->id, "RIFF"), IsList = TSF_FourCCEquals(chunk->id, "LIST");
	if (IsRiff && parent) return TSF_FALSE; //not allowed
	if (!IsRiff && !IsList) return TSF_TRUE; //custom type without sub type
	if (!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc)) || *chunk->id <= ' ' || *chunk->id >= 'z') return TSF_FALSE;
	chunk->size -= sizeof(tsf_fourcc);
	return TSF_TRUE;
}

static void tsf_region_clear(struct tsf_region* i, TSF_BOOL for_relative)
{
	TSF_MEMSET(i, 0, sizeof(struct tsf_region));
	i->hikey = i->hivel = 127;
	i->pitch_keycenter = 60; // C4
	if (for_relative) return;

	i->pitch_keytrack = 100;

	i->pitch_keycenter = -1;

	// SF2 defaults in timecents.
	i->ampenv.delay = i->ampenv.attack = i->ampenv.hold = i->ampenv.decay = i->ampenv.release = -12000.0f;
	i->modenv.delay = i->modenv.attack = i->modenv.hold = i->modenv.decay = i->modenv.release = -12000.0f;

	i->initialFilterFc = 13500;

	i->delayModLFO = -12000.0f;
	i->delayVibLFO = -12000.0f;
}

static void tsf_region_operator(struct tsf_region* region, tsf_u16 genOper, union tsf_hydra_genamount* amount, struct tsf_region* merge_region)
{
	enum
	{
		_GEN_TYPE_MASK       = 0x0F,
		GEN_FLOAT            = 0x01,
		GEN_INT              = 0x02,
		GEN_UINT_ADD         = 0x03,
		GEN_UINT_ADD15       = 0x04,
		GEN_KEYRANGE         = 0x05,
		GEN_VELRANGE         = 0x06,
		GEN_LOOPMODE         = 0x07,
		GEN_GROUP            = 0x08,
		GEN_KEYCENTER        = 0x09,

		_GEN_LIMIT_MASK      = 0xF0,
		GEN_INT_LIMIT12K     = 0x10, //min -12000, max 12000
		GEN_INT_LIMITFC      = 0x20, //min 1500, max 13500
		GEN_INT_LIMITQ       = 0x30, //min 0, max 960
		GEN_INT_LIMIT960     = 0x40, //min -960, max 960
		GEN_INT_LIMIT16K4500 = 0x50, //min -16000, max 4500
		GEN_FLOAT_LIMIT12K5K = 0x60, //min -12000, max 5000
		GEN_FLOAT_LIMIT12K8K = 0x70, //min -12000, max 8000
		GEN_FLOAT_LIMIT1200  = 0x80, //min -1200, max 1200
		GEN_FLOAT_LIMITPAN   = 0x90, //* .001f, min -.5f, max .5f,
		GEN_FLOAT_LIMITATTN  = 0xA0, //* .1f, min 0, max 144.0
		GEN_FLOAT_MAX1000    = 0xB0, //min 0, max 1000
		GEN_FLOAT_MAX1440    = 0xC0, //min 0, max 1440

		_GEN_MAX = 59,
	};
#define _TSFREGIONOFFSET(TYPE, FIELD) (uint8_t)(((TYPE*)&((struct tsf_region*)0)->FIELD) - (TYPE*)0)
#define _TSFREGIONENVOFFSET(TYPE, ENV, FIELD) (uint8_t)(((TYPE*)&((&(((struct tsf_region*)0)->ENV))->FIELD)) - (TYPE*)0)
	static const struct { uint8_t mode, offset; } genMetas[_GEN_MAX] =
	{
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(uint32_t, offset               ) }, // 0 StartAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(uint32_t, end                  ) }, // 1 EndAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(uint32_t, loop_start           ) }, // 2 StartloopAddrsOffset
		{ GEN_UINT_ADD                     , _TSFREGIONOFFSET(uint32_t, loop_end             ) }, // 3 EndloopAddrsOffset
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(uint32_t, offset               ) }, // 4 StartAddrsCoarseOffset
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int16_t, modLfoToPitch        ) }, // 5 ModLfoToPitch
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int16_t, vibLfoToPitch        ) }, // 6 VibLfoToPitch
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int16_t, modEnvToPitch        ) }, // 7 ModEnvToPitch
		{ GEN_INT   | GEN_INT_LIMITFC      , _TSFREGIONOFFSET(         int16_t, initialFilterFc      ) }, // 8 InitialFilterFc
		{ GEN_INT   | GEN_INT_LIMITQ       , _TSFREGIONOFFSET(         int16_t, initialFilterQ       ) }, // 9 InitialFilterQ
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int16_t, modLfoToFilterFc     ) }, //10 ModLfoToFilterFc
		{ GEN_INT   | GEN_INT_LIMIT12K     , _TSFREGIONOFFSET(         int16_t, modEnvToFilterFc     ) }, //11 ModEnvToFilterFc
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(uint32_t, end                  ) }, //12 EndAddrsCoarseOffset
		{ GEN_INT   | GEN_INT_LIMIT960     , _TSFREGIONOFFSET(         int16_t, modLfoToVolume       ) }, //13 ModLfoToVolume
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //15 ChorusEffectsSend (unsupported)
		{ 0                                , (0                                                  ) }, //16 ReverbEffectsSend (unsupported)
		{ GEN_FLOAT | GEN_FLOAT_LIMITPAN   , _TSFREGIONOFFSET(       float, pan                  ) }, //17 Pan
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ 0                                , (0                                                  ) }, //   Unused
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONOFFSET(       float, delayModLFO          ) }, //21 DelayModLFO
		{ GEN_INT   | GEN_INT_LIMIT16K4500 , _TSFREGIONOFFSET(         int16_t, freqModLFO           ) }, //22 FreqModLFO
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONOFFSET(       float, delayVibLFO          ) }, //23 DelayVibLFO
		{ GEN_INT   | GEN_INT_LIMIT16K4500 , _TSFREGIONOFFSET(         int16_t, freqVibLFO           ) }, //24 FreqVibLFO
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, modenv, delay        ) }, //25 DelayModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, modenv, attack       ) }, //26 AttackModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, modenv, hold         ) }, //27 HoldModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, modenv, decay        ) }, //28 DecayModEnv
		{ GEN_FLOAT | GEN_FLOAT_MAX1000    , _TSFREGIONENVOFFSET(    float, modenv, sustain      ) }, //29 SustainModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, modenv, release      ) }, //30 ReleaseModEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, modenv, keynumToHold ) }, //31 KeynumToModEnvHold
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, modenv, keynumToDecay) }, //32 KeynumToModEnvDecay
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, delay        ) }, //33 DelayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, attack       ) }, //34 AttackVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K5K , _TSFREGIONENVOFFSET(    float, ampenv, hold         ) }, //35 HoldVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, decay        ) }, //36 DecayVolEnv
		{ GEN_FLOAT | GEN_FLOAT_MAX1440    , _TSFREGIONENVOFFSET(    float, ampenv, sustain      ) }, //37 SustainVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT12K8K , _TSFREGIONENVOFFSET(    float, ampenv, release      ) }, //38 ReleaseVolEnv
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToHold ) }, //39 KeynumToVolEnvHold
		{ GEN_FLOAT | GEN_FLOAT_LIMIT1200  , _TSFREGIONENVOFFSET(    float, ampenv, keynumToDecay) }, //40 KeynumToVolEnvDecay
		{ 0                                , (0                                                  ) }, //   Instrument (special)
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_KEYRANGE                     , (0                                                  ) }, //43 KeyRange
		{ GEN_VELRANGE                     , (0                                                  ) }, //44 VelRange
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(uint32_t, loop_start           ) }, //45 StartloopAddrsCoarseOffset
		{ 0                                , (0                                                  ) }, //46 Keynum (special)
		{ 0                                , (0                                                  ) }, //47 Velocity (special)
		{ GEN_FLOAT | GEN_FLOAT_LIMITATTN  , _TSFREGIONOFFSET(       float, attenuation          ) }, //48 InitialAttenuation
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_UINT_ADD15                   , _TSFREGIONOFFSET(uint32_t, loop_end             ) }, //50 EndloopAddrsCoarseOffset
		{ GEN_INT                          , _TSFREGIONOFFSET(         int16_t, transpose            ) }, //51 CoarseTune
		{ GEN_INT                          , _TSFREGIONOFFSET(         int16_t, tune                 ) }, //52 FineTune
		{ 0                                , (0                                                  ) }, //   SampleID (special)
		{ GEN_LOOPMODE                     , _TSFREGIONOFFSET(         int16_t, loop_mode            ) }, //54 SampleModes
		{ 0                                , (0                                                  ) }, //   Reserved
		{ GEN_INT                          , _TSFREGIONOFFSET(         int16_t, pitch_keytrack       ) }, //56 ScaleTuning
		{ GEN_GROUP                        , _TSFREGIONOFFSET(uint32_t, group                ) }, //57 ExclusiveClass
		{ GEN_KEYCENTER                    , _TSFREGIONOFFSET(         int16_t, pitch_keycenter      ) }, //58 OverridingRootKey
	};
#undef _TSFREGIONOFFSET
#undef _TSFREGIONENVOFFSET
	if (amount)
	{
		int32_t offset;
		if (genOper >= _GEN_MAX) return;
		offset = genMetas[genOper].offset;
		switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
		{
		case GEN_FLOAT:      ((       float*)region)[offset]  = amount->shortAmount;     return;
		case GEN_INT:        ((         int16_t*)region)[offset]  = amount->shortAmount;     return;
		case GEN_UINT_ADD:   ((uint32_t*)region)[offset] += amount->shortAmount;     return;
		case GEN_UINT_ADD15: ((uint32_t*)region)[offset] += amount->shortAmount << 15; return;
		case GEN_KEYRANGE:   region->lokey = amount->range.lo; region->hikey = amount->range.hi; return;
		case GEN_VELRANGE:   region->lovel = amount->range.lo; region->hivel = amount->range.hi; return;
		case GEN_LOOPMODE:   region->loop_mode       = ((amount->wordAmount & 3) == 3 ? TSF_LOOPMODE_SUSTAIN : ((amount->wordAmount & 3) == 1 ? TSF_LOOPMODE_CONTINUOUS : TSF_LOOPMODE_NONE)); return;
		case GEN_GROUP:      region->group           = amount->wordAmount;  return;
		case GEN_KEYCENTER:  region->pitch_keycenter = amount->shortAmount; return;
		}
	}
	else //merge regions and clamp values
	{
		for (genOper = 0; genOper != _GEN_MAX; genOper++)
		{
			int32_t offset = genMetas[genOper].offset;
			switch (genMetas[genOper].mode & _GEN_TYPE_MASK)
			{
			case GEN_FLOAT:
			{
				float *val = &((float*)region)[offset], vfactor, vmin, vmax;
				*val += ((float*)merge_region)[offset];
				switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
				{
				case GEN_FLOAT_LIMIT12K5K: vfactor =   1.0f; vmin = -12000.0f; vmax = 5000.0f; break;
				case GEN_FLOAT_LIMIT12K8K: vfactor =   1.0f; vmin = -12000.0f; vmax = 8000.0f; break;
				case GEN_FLOAT_LIMIT1200:  vfactor =   1.0f; vmin =  -1200.0f; vmax = 1200.0f; break;
				case GEN_FLOAT_LIMITPAN:   vfactor = 0.001f; vmin =     -0.5f; vmax =    0.5f; break;
				case GEN_FLOAT_LIMITATTN:  vfactor =   0.1f; vmin =      0.0f; vmax =  144.0f; break;
				case GEN_FLOAT_MAX1000:    vfactor =   1.0f; vmin =      0.0f; vmax = 1000.0f; break;
				case GEN_FLOAT_MAX1440:    vfactor =   1.0f; vmin =      0.0f; vmax = 1440.0f; break;
				default: continue;
				}
				*val *= vfactor;
				if      (*val < vmin) *val = vmin;
				else if (*val > vmax) *val = vmax;
				continue;
			}
			case GEN_INT:
			{
				short *val = &((int16_t*)region)[offset], vmin, vmax;
				*val += ((int16_t*)merge_region)[offset];
				switch (genMetas[genOper].mode & _GEN_LIMIT_MASK)
				{
				case GEN_INT_LIMIT12K:     vmin = -12000; vmax = 12000; break;
				case GEN_INT_LIMITFC:      vmin =   1500; vmax = 13500; break;
				case GEN_INT_LIMITQ:       vmin =      0; vmax =   960; break;
				case GEN_INT_LIMIT960:     vmin =   -960; vmax =   960; break;
				case GEN_INT_LIMIT16K4500: vmin = -16000; vmax =  4500; break;
				default: continue;
				}
				if      (*val < vmin) *val = vmin;
				else if (*val > vmax) *val = vmax;
				continue;
			}
			case GEN_UINT_ADD:
			{
				((uint32_t*)region)[offset] += ((uint32_t*)merge_region)[offset];
				continue;
			}
			}
		}
	}
}

static void tsf_region_envtosecs(struct tsf_envelope* p, TSF_BOOL sustainIsGain)
{
	// EG times need to be converted from timecents to seconds.
	// Pin very short EG segments.  Timecents don't get to zero, and our EG is
	// happier with zero values.
	p->delay   = (p->delay   < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->delay));
	p->attack  = (p->attack  < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->attack));
	p->release = (p->release < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->release));

	// If we have dynamic hold or decay times depending on key number we need
	// to keep the values in timecents so we can calculate it during startNote
	if (!p->keynumToHold)  p->hold  = (p->hold  < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->hold));
	if (!p->keynumToDecay) p->decay = (p->decay < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->decay));

	if (p->sustain < 0.0f) p->sustain = 0.0f;
	else if (sustainIsGain) p->sustain = tsf_decibelsToGain(-p->sustain / 10.0f);
	else p->sustain = 1.0f - (p->sustain / 1000.0f);
}

static int32_t tsf_preset_idx(struct tsf_hydra *hydra, struct tsf_stream *stream, struct tsf_hydra_phdr *pphdr, int32_t pphdrIdx) {
	int32_t idx = 0;
	int32_t otherpphdrIdx;
	for (otherpphdrIdx = 0; otherpphdrIdx < hydra->phdrNum - 1; otherpphdrIdx++)
	{
		struct tsf_hydra_phdr otherphdr;
		stream->seek(stream->data, hydra->phdrPos + otherpphdrIdx * phdrSizeInFile);
		tsf_hydra_read_phdr(&otherphdr, stream);

		if (otherphdr.bank > pphdr->bank) continue;
		else if (otherphdr.bank < pphdr->bank) idx++;
		else if (otherphdr.preset > pphdr->preset) continue;
		else if (otherphdr.preset < pphdr->preset) idx++;
		else if (otherpphdrIdx < pphdrIdx) idx++;
	}
	return idx;
}

static void tsf_preload_presets(tsf* res)
{
	enum { GenInstrument = 41, GenKeyRange = 43, GenVelRange = 44, GenSampleID = 53 };
	struct tsf_stream *stream = res->stream;
	struct tsf_hydra *hydra = &res->hydra;
	// Read each preset.
	int32_t pphdrIdx;
	for (pphdrIdx = 0; pphdrIdx < hydra->phdrNum - 1; pphdrIdx++)
	{
		struct tsf_hydra_phdr pphdr;
		struct tsf_hydra_phdr nextpphdr;
		stream->seek(stream->data, hydra->phdrPos + pphdrIdx * phdrSizeInFile);
		tsf_hydra_read_phdr(&pphdr, stream);

		stream->seek(stream->data, hydra->phdrPos + (pphdrIdx + 1) * phdrSizeInFile);
		tsf_hydra_read_phdr(&nextpphdr, stream);

		int32_t sortedIndex = 0;
		struct tsf_preset* preset;

		sortedIndex = tsf_preset_idx(hydra, stream, &pphdr, pphdrIdx);

		preset = &res->presets[sortedIndex];
#ifndef TSF_NO_PRESET_NAME
		TSF_MEMCPY(preset->presetName, pphdr.presetName, sizeof(preset->presetName));
		preset->presetName[sizeof(preset->presetName) - 1] = '\0'; //should be zero terminated in source file but make sure
#endif
		preset->bank = pphdr.bank;
		preset->preset = pphdr.preset;
		preset->regionNum = 0;
		preset->regions = TSF_NULL;
		preset->pphdrIdx = pphdrIdx;
		preset->loaded = TSF_FALSE;
		preset->refCount = 0;
//		printf("PRESET name:%s bank:%d preset:%d\n",preset->presetName, preset->bank, preset->preset);

		int32_t ppbagIdx;
		//count regions covered by this preset
		for (ppbagIdx = pphdr.presetBagNdx; ppbagIdx < nextpphdr.presetBagNdx; ppbagIdx++)
		{
			struct tsf_hydra_pbag ppbag, nextppbag;

			stream->seek(stream->data, hydra->pbagPos + ppbagIdx * pbagSizeInFile);
			tsf_hydra_read_pbag(&ppbag, stream);
			stream->seek(stream->data, hydra->pbagPos + (ppbagIdx + 1) * pbagSizeInFile);
			tsf_hydra_read_pbag(&nextppbag, stream);

			uint8_t plokey = 0, phikey = 127, plovel = 0, phivel = 127;
			int32_t ppgenIdx;
			for (ppgenIdx = ppbag.genNdx; ppgenIdx < nextppbag.genNdx; ppgenIdx++)
			{
				struct tsf_hydra_pgen ppgen;
				struct tsf_hydra_inst pinst, nextpinst;

				stream->seek(stream->data, hydra->pgenPos + ppgenIdx * pgenSizeInFile);
				tsf_hydra_read_pgen(&ppgen, stream);

				if (ppgen.genOper == GenKeyRange) { plokey = ppgen.genAmount.range.lo; phikey = ppgen.genAmount.range.hi; continue; }
				if (ppgen.genOper == GenVelRange) { plovel = ppgen.genAmount.range.lo; phivel = ppgen.genAmount.range.hi; continue; }
				if (ppgen.genOper != GenInstrument) continue;
				if (ppgen.genAmount.wordAmount >= hydra->instNum) continue;

				stream->seek(stream->data, hydra->instPos + ppgen.genAmount.wordAmount * instSizeInFile);
				tsf_hydra_read_inst(&pinst, stream);

				stream->seek(stream->data, hydra->instPos + (ppgen.genAmount.wordAmount + 1) * instSizeInFile);
				tsf_hydra_read_inst(&nextpinst, stream);

				int32_t pibagIdx;
				for (pibagIdx = pinst.instBagNdx; pibagIdx < nextpinst.instBagNdx; pibagIdx++)
				{
					struct tsf_hydra_ibag pibag, nextpibag;

					stream->seek(stream->data, hydra->ibagPos + pibagIdx * ibagSizeInFile);
					tsf_hydra_read_ibag(&pibag, stream);

					stream->seek(stream->data, hydra->ibagPos + (pibagIdx + 1) * ibagSizeInFile);
					tsf_hydra_read_ibag(&nextpibag, stream);

					uint8_t ilokey = 0, ihikey = 127, ilovel = 0, ihivel = 127;

					//for (pigen = hydra->igens + pibag->instGenNdx, pigenEnd = hydra->igens + pibag[1].instGenNdx; pigen != pigenEnd; pigen++)
					//{
					int32_t pigenIdx;
					for (pigenIdx = pibag.instGenNdx; pigenIdx < nextpibag.instGenNdx; pigenIdx++)
					{
						struct tsf_hydra_igen pigen;

						stream->seek(stream->data, hydra->igenPos + pigenIdx * igenSizeInFile);
						tsf_hydra_read_igen(&pigen, stream);

						if (pigen.genOper == GenKeyRange) { ilokey = pigen.genAmount.range.lo; ihikey = pigen.genAmount.range.hi; continue; }
						if (pigen.genOper == GenVelRange) { ilovel = pigen.genAmount.range.lo; ihivel = pigen.genAmount.range.hi; continue; }
						if (pigen.genOper == GenSampleID && ihikey >= plokey && ilokey <= phikey && ihivel >= plovel && ilovel <= phivel) preset->regionNum++;
					}
				}
			}
		}
	}
}

static void tsf_unload_preset(tsf *res, int32_t idx) {
	struct tsf_preset* preset;
	preset = &res->presets[idx];

	TSF_FREE(preset->regions);
	preset->regions = TSF_NULL;
	preset->loaded = TSF_FALSE;
	preset->refCount = 0;
}

static void tsf_load_preset(tsf* res, int32_t idx)
{
	enum { GenInstrument = 41, GenKeyRange = 43, GenVelRange = 44, GenSampleID = 53 };

	struct tsf_stream *stream = res->stream;
	struct tsf_hydra *hydra = &res->hydra;

	struct tsf_hydra_phdr pphdr;
	struct tsf_hydra_phdr nextpphdr;

	struct tsf_region globalRegion;

	struct tsf_preset* preset;

	int32_t region_index = 0;

	preset = &res->presets[idx];

	stream->seek(stream->data, hydra->phdrPos + preset->pphdrIdx * phdrSizeInFile);
	tsf_hydra_read_phdr(&pphdr, stream);

	stream->seek(stream->data, hydra->phdrPos + (preset->pphdrIdx + 1) * phdrSizeInFile);
	tsf_hydra_read_phdr(&nextpphdr, stream);

#ifdef TSF_MEM_PROF
	printf("MALLOC struct tsf_region (%d) %d * %ld = %ld\n", idx, preset->regionNum, sizeof(struct tsf_region), preset->regionNum * sizeof(struct tsf_region));
#endif
	preset->regions = (struct tsf_region*)TSF_MALLOC(preset->regionNum * sizeof(struct tsf_region));
	tsf_region_clear(&globalRegion, TSF_TRUE);

	// Zones.
	int32_t ppbagIdx;
	for (ppbagIdx = pphdr.presetBagNdx; ppbagIdx < nextpphdr.presetBagNdx; ppbagIdx++)
	{
		struct tsf_hydra_pbag ppbag, nextppbag;

		stream->seek(stream->data, hydra->pbagPos + ppbagIdx * pbagSizeInFile);
		tsf_hydra_read_pbag(&ppbag, stream);
		stream->seek(stream->data, hydra->pbagPos + (ppbagIdx + 1) * pbagSizeInFile);
		tsf_hydra_read_pbag(&nextppbag, stream);

		struct tsf_region presetRegion = globalRegion;
		int32_t hadGenInstrument = 0;

		// Generators.
		int32_t ppgenIdx;
		for (ppgenIdx = ppbag.genNdx; ppgenIdx < nextppbag.genNdx; ppgenIdx++)
		{
			struct tsf_hydra_pgen ppgen;

			stream->seek(stream->data, hydra->pgenPos + ppgenIdx * pgenSizeInFile);
			tsf_hydra_read_pgen(&ppgen, stream);

			// Instrument.
			if (ppgen.genOper == GenInstrument)
			{
				struct tsf_hydra_inst pinst, nextpinst;

				struct tsf_region instRegion;
				tsf_u16 whichInst = ppgen.genAmount.wordAmount;
				if (whichInst >= hydra->instNum) continue;

				tsf_region_clear(&instRegion, TSF_FALSE);

				stream->seek(stream->data, hydra->instPos + whichInst * instSizeInFile);
				tsf_hydra_read_inst(&pinst, stream);

				stream->seek(stream->data, hydra->instPos + (whichInst + 1) * instSizeInFile);
				tsf_hydra_read_inst(&nextpinst, stream);

				int32_t pibagIdx;
				for (pibagIdx = pinst.instBagNdx; pibagIdx < nextpinst.instBagNdx; pibagIdx++)
				{
					struct tsf_hydra_ibag pibag, nextpibag;

					stream->seek(stream->data, hydra->ibagPos + pibagIdx * ibagSizeInFile);
					tsf_hydra_read_ibag(&pibag, stream);

					stream->seek(stream->data, hydra->ibagPos + (pibagIdx + 1) * ibagSizeInFile);
					tsf_hydra_read_ibag(&nextpibag, stream);

					// Generators.
					struct tsf_region zoneRegion = instRegion;
					int32_t hadSampleID = 0;
					int32_t pigenIdx;
					for (pigenIdx = pibag.instGenNdx; pigenIdx < nextpibag.instGenNdx; pigenIdx++)
					{
						struct tsf_hydra_igen pigen;

						stream->seek(stream->data, hydra->igenPos + pigenIdx * igenSizeInFile);
						tsf_hydra_read_igen(&pigen, stream);

						if (pigen.genOper == GenSampleID)
						{
							//struct tsf_hydra_shdr* pshdr;
							struct tsf_hydra_shdr pshdr;

							//preset region key and vel ranges are a filter for the zone regions
							if (zoneRegion.hikey < presetRegion.lokey || zoneRegion.lokey > presetRegion.hikey) continue;
							if (zoneRegion.hivel < presetRegion.lovel || zoneRegion.lovel > presetRegion.hivel) continue;
							if (presetRegion.lokey > zoneRegion.lokey) zoneRegion.lokey = presetRegion.lokey;
							if (presetRegion.hikey < zoneRegion.hikey) zoneRegion.hikey = presetRegion.hikey;
							if (presetRegion.lovel > zoneRegion.lovel) zoneRegion.lovel = presetRegion.lovel;
							if (presetRegion.hivel < zoneRegion.hivel) zoneRegion.hivel = presetRegion.hivel;

							//sum regions
							tsf_region_operator(&zoneRegion, 0, TSF_NULL, &presetRegion);

							// EG times need to be converted from timecents to seconds.
							tsf_region_envtosecs(&zoneRegion.ampenv, TSF_TRUE);
							tsf_region_envtosecs(&zoneRegion.modenv, TSF_FALSE);

							// LFO times need to be converted from timecents to seconds.
							zoneRegion.delayModLFO = (zoneRegion.delayModLFO < -11950.0f ? 0.0f : tsf_timecents2Secsf(zoneRegion.delayModLFO));
							zoneRegion.delayVibLFO = (zoneRegion.delayVibLFO < -11950.0f ? 0.0f : tsf_timecents2Secsf(zoneRegion.delayVibLFO));

							// Fixup sample positions
							//pshdr = &hydra->shdrs[pigen->genAmount.wordAmount];

							stream->seek(stream->data, hydra->shdrPos + pigen.genAmount.wordAmount * shdrSizeInFile);
							tsf_hydra_read_shdr(&pshdr, stream);

							zoneRegion.offset += pshdr.start;
							zoneRegion.end += pshdr.end;
							zoneRegion.loop_start += pshdr.startLoop;
							zoneRegion.loop_end += pshdr.endLoop;
							if (pshdr.endLoop > 0) zoneRegion.loop_end -= 1;
							if (zoneRegion.pitch_keycenter == -1) zoneRegion.pitch_keycenter = pshdr.originalPitch;
							zoneRegion.tune += pshdr.pitchCorrection;
							zoneRegion.sample_rate = pshdr.sampleRate;
							if (zoneRegion.end && zoneRegion.end < res->fontSampleCount) zoneRegion.end++;
							else zoneRegion.end = res->fontSampleCount;

							preset->regions[region_index] = zoneRegion;
							region_index++;
							hadSampleID = 1;
						}
						else tsf_region_operator(&zoneRegion, pigen.genOper, &pigen.genAmount, TSF_NULL);
					}

					// Handle instrument's global zone.
					if (pibagIdx == pinst.instBagNdx && !hadSampleID)
						instRegion = zoneRegion;

					// Modulators (TODO)
					//if (ibag->instModNdx < ibag[1].instModNdx) addUnsupportedOpcode("any modulator");
				}
				hadGenInstrument = 1;
			}
			else tsf_region_operator(&presetRegion, ppgen.genOper, &ppgen.genAmount, TSF_NULL);
		}

		// Modulators (TODO)
		//if (pbag->modNdx < pbag[1].modNdx) addUnsupportedOpcode("any modulator");

		// Handle preset's global zone.
		if (ppbagIdx == pphdr.presetBagNdx && !hadGenInstrument)
			globalRegion = presetRegion;
	}

	preset->loaded = TSF_TRUE;
}

static void tsf_load_samples(int16_t** fontSamples, uint32_t* fontSampleCount, struct tsf_riffchunk *chunkSmpl, struct tsf_stream* stream)
{
	// Read sample data into float format buffer.
	uint32_t samplesLeft, samplesToRead;
	samplesLeft = *fontSampleCount = chunkSmpl->size / sizeof(int16_t);

	uint32_t offset = stream->tell(stream->data);

	*fontSamples = (int16_t *)TSF_MMAP(0, chunkSmpl->size + offset, ((TSF_FILE *)stream->data));
	for (; samplesLeft; samplesLeft -= samplesToRead)
	{
		samplesToRead = (samplesLeft > 1024 ? 1024 : samplesLeft);
		stream->skip(stream->data, samplesToRead * sizeof(int16_t));
	}
}

static void tsf_voice_envelope_nextsegment(struct tsf_voice_envelope* e, int16_t active_segment, float outSampleRate)
{
	switch (active_segment)
	{
	case TSF_SEGMENT_NONE:
		e->samplesUntilNextSegment = (int32_t)(e->parameters.delay * outSampleRate);
		if (e->samplesUntilNextSegment > 0)
		{
			e->segment = TSF_SEGMENT_DELAY;
			e->segmentIsExponential = TSF_FALSE;
			e->level = 0.0;
			e->slope = 0.0;
			return;
		}
	/* fall through */
	case TSF_SEGMENT_DELAY:
		e->samplesUntilNextSegment = (int32_t)(e->parameters.attack * outSampleRate);
		if (e->samplesUntilNextSegment > 0)
		{
			if (!e->isAmpEnv)
			{
				//mod env attack duration scales with velocity (velocity of 1 is full duration, max velocity is 0.125 times duration)
				e->samplesUntilNextSegment = (int32_t)(e->parameters.attack * ((145 - e->midiVelocity) / 144.0f) * outSampleRate);
			}
			e->segment = TSF_SEGMENT_ATTACK;
			e->segmentIsExponential = TSF_FALSE;
			e->level = 0.0f;
			e->slope = 1.0f / e->samplesUntilNextSegment;
			return;
		}
	/* fall through */
	case TSF_SEGMENT_ATTACK:
		e->samplesUntilNextSegment = (int32_t)(e->parameters.hold * outSampleRate);
		if (e->samplesUntilNextSegment > 0)
		{
			e->segment = TSF_SEGMENT_HOLD;
			e->segmentIsExponential = TSF_FALSE;
			e->level = 1.0f;
			e->slope = 0.0f;
			return;
		}
	/* fall through */
	case TSF_SEGMENT_HOLD:
		e->samplesUntilNextSegment = (int32_t)(e->parameters.decay * outSampleRate);
		if (e->samplesUntilNextSegment > 0)
		{
			e->segment = TSF_SEGMENT_DECAY;
			e->level = 1.0f;
			if (e->isAmpEnv)
			{
				// I don't truly understand this; just following what LinuxSampler does.
				float mysterySlope = -9.226f / e->samplesUntilNextSegment;
				e->slope = TSF_EXPF(mysterySlope);
				e->segmentIsExponential = TSF_TRUE;
				if (e->parameters.sustain > 0.0f)
				{
					// Again, this is following LinuxSampler's example, which is similar to
					// SF2-style decay, where "decay" specifies the time it would take to
					// get to zero, not to the sustain level.  The SFZ spec is not that
					// specific about what "decay" means, so perhaps it's really supposed
					// to specify the time to reach the sustain level.
					e->samplesUntilNextSegment = (int32_t)(TSF_LOG(e->parameters.sustain) / mysterySlope);
				}
			}
			else
			{
				e->slope = -1.0f / e->samplesUntilNextSegment;
				e->samplesUntilNextSegment = (int32_t)(e->parameters.decay * (1.0f - e->parameters.sustain) * outSampleRate);
				e->segmentIsExponential = TSF_FALSE;
			}
			return;
		}
	/* fall through */
	case TSF_SEGMENT_DECAY:
		e->segment = TSF_SEGMENT_SUSTAIN;
		e->level = e->parameters.sustain;
		e->slope = 0.0f;
		e->samplesUntilNextSegment = 0x7FFFFFFF;
		e->segmentIsExponential = TSF_FALSE;
		return;
	case TSF_SEGMENT_SUSTAIN:
		e->segment = TSF_SEGMENT_RELEASE;
		e->samplesUntilNextSegment = (int32_t)((e->parameters.release <= 0 ? TSF_FASTRELEASETIME : e->parameters.release) * outSampleRate);
		if (e->isAmpEnv)
		{
			// I don't truly understand this; just following what LinuxSampler does.
			float mysterySlope = -9.226f / e->samplesUntilNextSegment;
			e->slope = TSF_EXPF(mysterySlope);
			e->segmentIsExponential = TSF_TRUE;
		}
		else
		{
			e->slope = -e->level / e->samplesUntilNextSegment;
			e->segmentIsExponential = TSF_FALSE;
		}
		return;
	case TSF_SEGMENT_RELEASE:
	default:
		e->segment = TSF_SEGMENT_DONE;
		e->segmentIsExponential = TSF_FALSE;
		e->level = e->slope = 0.0f;
		e->samplesUntilNextSegment = 0x7FFFFFF;
	}
}

static void tsf_voice_envelope_setup(struct tsf_voice_envelope* e, struct tsf_envelope* new_parameters, int32_t midiNoteNumber, int16_t midiVelocity, TSF_BOOL isAmpEnv, float outSampleRate)
{
	e->parameters = *new_parameters;
	if (e->parameters.keynumToHold)
	{
		e->parameters.hold += e->parameters.keynumToHold * (60.0f - midiNoteNumber);
		e->parameters.hold = (e->parameters.hold < -10000.0f ? 0.0f : tsf_timecents2Secsf(e->parameters.hold));
	}
	if (e->parameters.keynumToDecay)
	{
		e->parameters.decay += e->parameters.keynumToDecay * (60.0f - midiNoteNumber);
		e->parameters.decay = (e->parameters.decay < -10000.0f ? 0.0f : tsf_timecents2Secsf(e->parameters.decay));
	}
	e->midiVelocity = midiVelocity;
	e->isAmpEnv = isAmpEnv;
	tsf_voice_envelope_nextsegment(e, TSF_SEGMENT_NONE, outSampleRate);
}

static void tsf_voice_envelope_process(struct tsf_voice_envelope* e, int32_t numSamples, float outSampleRate)
{
	if (e->slope)
	{
		if (e->segmentIsExponential) e->level *= TSF_POWF(e->slope, (float)numSamples);
		else e->level += (e->slope * numSamples);
	}
	if ((e->samplesUntilNextSegment -= numSamples) <= 0)
		tsf_voice_envelope_nextsegment(e, e->segment, outSampleRate);
}

#ifndef TSF_NO_LOWPASS

static void tsf_voice_lowpass_setup(struct tsf_voice_lowpass* e, float Fc)
{
	// Lowpass filter from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
	float K = TSF_TAN(TSF_PI * Fc), KK = K * K;
	float norm = 1 / (1 + K * e->QInv + KK);
	e->a0 = float_to_fixed(KK * norm);
	e->a1 = float_to_fixed(2 * KK * norm);
	e->b1 = float_to_fixed(2 * (KK - 1) * norm);
	e->b2 = float_to_fixed((1 - K * e->QInv + KK) * norm);

//	printf("LOW-PASS a0:%d a1:%d b1:%d b2:%d\n",e->a0, e->a1, e->b1, e->b2);
}

static int32_t tsf_voice_lowpass_process(struct tsf_voice_lowpass* e, int32_t in)
{
	int32_t in0, in1, in2;
	int32_t out;
	out = (e->a0 * in + e->z1) >> 15;
	e->z1 = (e->a1 * in) + e->z2 - (e->b1 * out);
	e->z2 = (e->a0 * in) - (e->b2 * out);

//	printf("LOW-PASS in:%d out:%d z1:%d z2:%d\n",In,Out,e->z1,e->z2);
	return __SSAT(out, 16);
}

#endif

static void tsf_voice_lfo_setup(struct tsf_voice_lfo* e, float delay, int32_t freqCents, float outSampleRate)
{
	e->samplesUntil = (int32_t)(delay * outSampleRate);
	e->delta = (4.0f * tsf_cents2Hertz((float)freqCents) / outSampleRate);
	e->level = 0;
}

static void tsf_voice_lfo_process(struct tsf_voice_lfo* e, int32_t blockSamples)
{
	if (e->samplesUntil > blockSamples) { e->samplesUntil -= blockSamples; return; }
	e->level += e->delta * blockSamples;
	if      (e->level >  1.0f) { e->delta = -e->delta; e->level =  2.0f - e->level; }
	else if (e->level < -1.0f) { e->delta = -e->delta; e->level = -2.0f - e->level; }
}

static void tsf_voice_kill(struct tsf_voice* v)
{
	v->playingPreset = -1;
}

static void tsf_voice_end(struct tsf_voice* v, float outSampleRate)
{
	tsf_voice_envelope_nextsegment(&v->ampenv, TSF_SEGMENT_SUSTAIN, outSampleRate);
	tsf_voice_envelope_nextsegment(&v->modenv, TSF_SEGMENT_SUSTAIN, outSampleRate);
	if (v->region->loop_mode == TSF_LOOPMODE_SUSTAIN)
	{
		// Continue playing, but stop looping.
		v->loopEnd = v->loopStart;
	}
}

static void tsf_voice_endquick(struct tsf_voice* v, float outSampleRate)
{
	v->ampenv.parameters.release = 0.0f; tsf_voice_envelope_nextsegment(&v->ampenv, TSF_SEGMENT_SUSTAIN, outSampleRate);
	v->modenv.parameters.release = 0.0f; tsf_voice_envelope_nextsegment(&v->modenv, TSF_SEGMENT_SUSTAIN, outSampleRate);
}

static void tsf_voice_calcpitchratio(struct tsf_voice* v, float pitchShift, float outSampleRate)
{
	float note = v->playingKey + v->region->transpose + v->region->tune / 100.0;
	float adjustedPitch = v->region->pitch_keycenter + (note - v->region->pitch_keycenter) * (v->region->pitch_keytrack / 100.0);
	if (pitchShift) adjustedPitch += pitchShift;
	v->pitchInputTimecents = adjustedPitch * 100.0;
	v->pitchOutputFactor = v->region->sample_rate / (tsf_timecents2Secsf(v->region->pitch_keycenter * 100.0) * outSampleRate);
}

static void tsf_voice_render(tsf* f, struct tsf_voice* v, int32_t* outputBuffer, int32_t *chorusBuffer, int32_t *reverbBuffer, int32_t numSamples)
{
	struct tsf_region* region = v->region;
	int16_t* input = f->fontSamplesOffset + f->fontSamples;
	int32_t* output = outputBuffer;

	// Cache some values, to give them at least some chance of ending up in registers.
	TSF_BOOL updateModEnv = (region->modEnvToPitch || region->modEnvToFilterFc);
	TSF_BOOL updateModLFO = (v->modlfo.delta && (region->modLfoToPitch || region->modLfoToFilterFc || region->modLfoToVolume));
	TSF_BOOL updateVibLFO = (v->viblfo.delta && (region->vibLfoToPitch));
	TSF_BOOL isLooping    = (v->loopStart < v->loopEnd);
	uint32_t tmpLoopStart = v->loopStart, tmpLoopEnd = v->loopEnd;
	uint64_t tmpSampleEndDbl = ((uint64_t)region->end) << 32, tmpLoopEndDbl = ((uint64_t)tmpLoopEnd + 1) << 32;
	uint64_t tmpSourceSamplePosition = v->sourceSamplePosition;
	struct tsf_voice_lowpass tmpLowpass = v->lowpass;

	TSF_BOOL dynamicLowpass = (region->modLfoToFilterFc || region->modEnvToFilterFc);
	float tmpSampleRate = f->outSampleRate, tmpInitialFilterFc, tmpModLfoToFilterFc, tmpModEnvToFilterFc;

	TSF_BOOL dynamicPitchRatio = (region->modLfoToPitch || region->modEnvToPitch || region->vibLfoToPitch);
	float pitchRatio;
	float tmpModLfoToPitch, tmpVibLfoToPitch, tmpModEnvToPitch;

	TSF_BOOL dynamicGain = (region->modLfoToVolume != 0);
	float noteGain = 0, tmpModLfoToVolume;

	if (dynamicLowpass) tmpInitialFilterFc = (float)region->initialFilterFc, tmpModLfoToFilterFc = (float)region->modLfoToFilterFc, tmpModEnvToFilterFc = (float)region->modEnvToFilterFc;
	else tmpInitialFilterFc = 0, tmpModLfoToFilterFc = 0, tmpModEnvToFilterFc = 0;

	if (dynamicPitchRatio) pitchRatio = 0, tmpModLfoToPitch = (float)region->modLfoToPitch, tmpVibLfoToPitch = (float)region->vibLfoToPitch, tmpModEnvToPitch = (float)region->modEnvToPitch;
	else pitchRatio = tsf_timecents2Secsf(v->pitchInputTimecents) * v->pitchOutputFactor, tmpModLfoToPitch = 0, tmpVibLfoToPitch = 0, tmpModEnvToPitch = 0;

	if (dynamicGain) tmpModLfoToVolume = (float)region->modLfoToVolume * 0.1f;
	else noteGain = tsf_decibelsToGain(v->noteGainDB), tmpModLfoToVolume = 0;

	struct tsf_channel *chan = &f->channels->channels[v->playingChannel];

	int32_t *fxRevBuf = reverbBuffer;
	int32_t *fxChorusBuf = chorusBuffer;

	while (numSamples)
	{
		float gainMono;
		int32_t gainLeft, gainRight, gainStereo;
		int32_t gainChorus = 0, gainReverb = 0, gainEffect;
		int32_t blockSamples = (numSamples > TSF_RENDER_EFFECTSAMPLEBLOCK ? TSF_RENDER_EFFECTSAMPLEBLOCK : numSamples);
		numSamples -= blockSamples;

#ifndef TSF_NO_LOWPASS
		if (dynamicLowpass)
		{
			float fres = tmpInitialFilterFc + v->modlfo.level * tmpModLfoToFilterFc + v->modenv.level * tmpModEnvToFilterFc;
			float lowpassFc = (fres <= 13500 ? tsf_cents2Hertz(fres) / tmpSampleRate : 1.0f);
			tmpLowpass.active = (lowpassFc < 0.499f);
			if (tmpLowpass.active) tsf_voice_lowpass_setup(&tmpLowpass, lowpassFc);
		}
#endif

		if (dynamicPitchRatio)
			pitchRatio = tsf_timecents2Secsf(v->pitchInputTimecents + (v->modlfo.level * tmpModLfoToPitch + v->viblfo.level * tmpVibLfoToPitch + v->modenv.level * tmpModEnvToPitch)) * v->pitchOutputFactor;

		if (dynamicGain)
			noteGain = tsf_decibelsToGain(v->noteGainDB + (v->modlfo.level * tmpModLfoToVolume));

		gainMono = noteGain * v->ampenv.level * 0.5f; // FIXME x 0.5 needed to avoid saturation with int32_t buf

		// Update EG.
		tsf_voice_envelope_process(&v->ampenv, blockSamples, tmpSampleRate);
		if (updateModEnv) tsf_voice_envelope_process(&v->modenv, blockSamples, tmpSampleRate);

		// Update LFOs.
		if (updateModLFO) tsf_voice_lfo_process(&v->modlfo, blockSamples);
		if (updateVibLFO) tsf_voice_lfo_process(&v->viblfo, blockSamples);

		uint64_t phaseIncr = float_to_fixed64(pitchRatio);
		uint32_t pos, nextPos;
#ifndef TSF_NO_INTERPOLATION
		int32_t alpha;
#endif

		gainLeft = float_to_fixed(gainMono * v->panFactorLeft), gainRight = float_to_fixed(gainMono * v->panFactorRight);
		gainStereo = __PKHBT(gainLeft, gainRight, 16);

#ifndef TSF_NO_CHORUS
		gainChorus = float_to_fixed(gainMono * chan->chorus);
#endif
#ifndef TSF_NO_REVERB
		gainReverb = float_to_fixed(gainMono * chan->reverb);
#endif

		gainEffect = __PKHBT(gainChorus, gainReverb, 16);

		int32_t samplesBuf[TSF_RENDER_EFFECTSAMPLEBLOCK];
		int32_t *buf = samplesBuf;
		int32_t in0, in1, in2, in3;
		int32_t out0, out1, out2, out3;

		int blkCnt, blckRemain;

		blkCnt = blockSamples;

		while (blkCnt-- && tmpSourceSamplePosition < tmpSampleEndDbl)
		{
			pos = (tmpSourceSamplePosition >> 32);
#ifndef TSF_NO_INTERPOLATION
			alpha = (tmpSourceSamplePosition - ((uint64_t)pos << 32)) >> 17;
			nextPos = (isLooping && pos >= tmpLoopEnd ? tmpLoopStart : pos + 1);

			in0 = __PKHBT((32767 - alpha), alpha, 16);
			in1 = __PKHBT(input[pos], input[nextPos], 16);

			out0 = (int32_t)__SMUAD(in0, in1) >> 15;

			*buf++ = __SSAT(out0, 16);
#else
			*buf++ = (int32_t)input[pos];
#endif
			tmpSourceSamplePosition += phaseIncr;

			if (isLooping && tmpSourceSamplePosition >= tmpLoopEndDbl) tmpSourceSamplePosition -= ((uint64_t)(tmpLoopEnd - tmpLoopStart + 1)) << 32;
		}

		blckRemain = blkCnt + 1; // some blocks are skipped

#ifndef TSF_NO_LOWPASS
		if (tmpLowpass.active) {
			buf = samplesBuf;
			blkCnt = (blockSamples - blckRemain) >> 1;

			while (blkCnt--)
			{
				in0 = *buf++;
				in1 = *buf++;

				out0 = tsf_voice_lowpass_process(&tmpLowpass, in0);
				out1 = tsf_voice_lowpass_process(&tmpLowpass, in1);

				buf -= 2;

				*buf++ = __SSAT(out0, 16);
				*buf++ = __SSAT(out1, 16);
			}

			blkCnt = (blockSamples - blckRemain) >> 1;
			if ((blockSamples - blckRemain) - (blkCnt << 1) ) {
				in0 = *buf++;
				out0 = tsf_voice_lowpass_process(&tmpLowpass, in0);
				buf -= 1;
				*buf++ = __SSAT(out0, 16);
			}

		}

#endif

		buf = samplesBuf;
		blkCnt = (blockSamples - blckRemain) >> 1;
		while (blkCnt--)
		{
			in0 = __PKHBT(buf[0], buf[1], 16);
			buf += 2;

			out0 = (int32_t) * output++;
			out1 = (int32_t) * output++;
			out2 = (int32_t) * output++;
			out3 = (int32_t) * output++;

			out0 = __SMLABB(in0, gainStereo, out0);
			out1 = __SMLABT(in0, gainStereo, out1);
			out2 = __SMLATB(in0, gainStereo, out2);
			out3 = __SMLATT(in0, gainStereo, out3);

			output -= 4;

			*output++ = out0;
			*output++ = out1;
			*output++ = out2;
			*output++ = out3;

#ifndef TSF_NO_CHORUS
			out0 = (int32_t) * fxChorusBuf++;
			out1 = (int32_t) * fxChorusBuf++;

			out0 = __SMLABB(in0, gainEffect, out0);
			out1 = __SMLATB(in0, gainEffect, out1);

			fxChorusBuf -= 2;

			*fxChorusBuf++ = out0;
			*fxChorusBuf++ = out1;
#endif

#ifndef TSF_NO_REVERB
			out0 = (int32_t) * fxRevBuf++;
			out1 = (int32_t) * fxRevBuf++;

			out0 = __SMLABT(in0, gainEffect, out0);
			out1 = __SMLATT(in0, gainEffect, out1);

			fxRevBuf -= 2;

			*fxRevBuf++ = out0;
			*fxRevBuf++ = out1;
#endif
		}

		// remaining sample
		blkCnt = (blockSamples - blckRemain) >> 1;
		if ((blockSamples - blckRemain) - (blkCnt << 1) ) {
			in0 = __PKHBT(buf[0], buf[0], 16);
			buf += 1;

			out0 = (int32_t) * output++;
			out1 = (int32_t) * output++;

			out0 = __SMLABB(in0, gainStereo, out0);
			out1 = __SMLABT(in0, gainStereo, out1);

			output -= 2;

			out0 = (int32_t) * fxChorusBuf++;
			out0 = __SMLABB(in0, gainEffect, out0);
			fxChorusBuf -= 1;
			*fxChorusBuf++ = out0;

			out0 = (int32_t) * fxRevBuf++;
			out0 = __SMLABT(in0, gainEffect, out0);
			fxRevBuf -= 1;
			*fxRevBuf++ = out1;
		}

		if (tmpSourceSamplePosition >= tmpSampleEndDbl || v->ampenv.segment == TSF_SEGMENT_DONE)
		{
			tsf_voice_kill(v);
			return;
		}
	}

	v->sourceSamplePosition = tmpSourceSamplePosition;
	if (tmpLowpass.active || dynamicLowpass) v->lowpass = tmpLowpass;
}

#ifndef TSF_NO_REVERB
/*
Colour is a "tilt" EQ similar to that used on old Quad amps, rolling off the treble when turned down and bass when turned up
Size adjusts the size of the "room", and to an extent its shape.
Decay adjusts the feedback trim through the comb filters.
*/
TSFDEF void tsf_reverb_setup(tsf* f, float colour, float size, float decay) {
	if (colour < -6.0f)
		colour = -6.0f;
	if (colour > 6.0f)
		colour = 6.0f;
	if (size < 0.1f)
		size = 0.1f;
	if (size > 1.0f)
		size = 1.0f;

	if (decay < 0.0f)
		decay = 0.0f;
	if (decay > 1.0)
		decay = 1.0f;

	reverb_init(&f->rev);
	reverb_set_colour(&f->rev, colour);
	reverb_set_size(&f->rev, size);
	reverb_set_decay(&f->rev, decay);
}
#endif

#ifndef TSF_NO_CHORUS
TSFDEF void tsf_chorus_setup(tsf* f, float delay, float decay, float speed, float depth) {
	chorus_init(&f->chorus, f->outSampleRate, delay, decay, speed, depth, MOD_TRIANGLE);
}
#endif

TSFDEF tsf* tsf_load(struct tsf_stream* stream)
{
	tsf* res = TSF_NULL;
	struct tsf_riffchunk chunkHead;
	struct tsf_riffchunk chunkList;
	struct tsf_hydra hydra;
	int16_t* fontSamples = TSF_NULL;
	uint32_t offset = 0;
	uint32_t fontSampleCount = 0;

	if (!tsf_riffchunk_read(TSF_NULL, &chunkHead, stream) || !TSF_FourCCEquals(chunkHead.id, "sfbk"))
	{
		//if (e) *e = TSF_INVALID_NOSF2HEADER;
		return res;
	}

	// Read hydra and locate sample data.
	TSF_MEMSET(&hydra, 0, sizeof(hydra));
	while (tsf_riffchunk_read(&chunkHead, &chunkList, stream))
	{
		struct tsf_riffchunk chunk;
		if (TSF_FourCCEquals(chunkList.id, "pdta"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream))
			{
#define HandleChunk(chunkName) (TSF_FourCCEquals(chunk.id, #chunkName) && !(chunk.size % chunkName##SizeInFile)) \
					{ \
						int32_t num = chunk.size / chunkName##SizeInFile; \
						hydra.chunkName##Num = num; \
						hydra.chunkName##Pos = stream->tell(stream->data); \
						stream->skip(stream->data, chunk.size); \
					}

				if HandleChunk(phdr) else if HandleChunk(pbag) else if HandleChunk(pmod)
							else if HandleChunk(pgen) else if HandleChunk(inst) else if HandleChunk(ibag)
										else if HandleChunk(imod) else if HandleChunk(igen) else if HandleChunk(shdr)
													else stream->skip(stream->data, chunk.size);
#undef HandleChunk
			}
		}
		else if (TSF_FourCCEquals(chunkList.id, "sdta"))
		{
			while (tsf_riffchunk_read(&chunkList, &chunk, stream))
			{
				if (TSF_FourCCEquals(chunk.id, "smpl"))
				{
					offset = stream->tell(stream->data);
					tsf_load_samples(&fontSamples, &fontSampleCount, &chunk, stream);
				}
				else stream->skip(stream->data, chunk.size);
			}
		}
		else stream->skip(stream->data, chunkList.size);
	}
	if (!hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum || !hydra.phdrNum)
	{
		//if (e) *e = TSF_INVALID_INCOMPLETE;
	}
//	else if (fontSamples == TSF_NULL)
//	{
	//if (e) *e = TSF_INVALID_NOSAMPLEDATA;
//	}
	else
	{
		res = (tsf*)TSF_MALLOC(sizeof(tsf));
		TSF_MEMSET(res, 0, sizeof(tsf));
		res->presetNum = hydra.phdrNum - 1;
		res->presets = (struct tsf_preset*)TSF_MALLOC(res->presetNum * sizeof(struct tsf_preset));
		res->fontSamples = fontSamples;
		res->fontSamplesOffset = offset / sizeof(int16_t);
		res->fontSampleCount = fontSampleCount;
		res->outSampleRate = 44100.0f;
		res->voicesMax = 128;

#ifdef TSF_MEM_PROF
		printf("MALLOC struct tsf_voice %ld * %ld = %ld\n", res->voicesMax, sizeof(struct tsf_voice), res->voicesMax * sizeof(struct tsf_voice));
#endif
		res->voices = (struct tsf_voice *)TSF_MALLOC(res->voicesMax * sizeof(struct tsf_voice));
		for (int i = 0; i < res->voicesMax; i++)
			res->voices[i].playingPreset = -1;
		res->voiceNum = res->voicesMax;

		res->stream = stream;
		res->hydra = hydra;
		res->gc = 0;

#ifndef TSF_NO_REVERB
		tsf_reverb_setup(res, 0.0f, 0.7f, 0.7f); // default large hall
#endif

#ifndef TSF_NO_CHORUS
		tsf_chorus_setup(res, 50.0f, 0.5f, 0.4f, 6.3f); // default chorus 3
#endif

		tsf_preload_presets(res);
	}
	return res;
}

TSFDEF void tsf_set_max_voices(tsf* f, int max) {
	f->voicesMax = max;
#ifdef TSF_MEM_PROF
	printf("REALLOC struct tsf_voice %ld * %ld = %ld\n", f->voicesMax, sizeof(struct tsf_voice), f->voicesMax * sizeof(struct tsf_voice));
#endif
	f->voices = (struct tsf_voice *)TSF_REALLOC(f->voices, f->voicesMax * sizeof(struct tsf_voice));
	for (int i = 0; i < f->voicesMax; i++)
		f->voices[i].playingPreset = -1;
	f->voiceNum = f->voicesMax;
}

TSFDEF void tsf_close(tsf* f)
{
	struct tsf_preset *preset, *presetEnd;
	if (!f) return;
	for (preset = f->presets, presetEnd = preset + f->presetNum; preset != presetEnd; preset++) {
		if (preset->loaded)
			TSF_FREE(preset->regions);
	}
	TSF_FREE(f->presets);
	//TSF_FREE(f->fontSamples);
	TSF_FREE(f->voices);
	if (f->channels) { TSF_FREE(f->channels->channels); TSF_FREE(f->channels); }
	TSF_FREE(f->stream);
	TSF_FREE(f);
}

TSFDEF void tsf_reset(tsf* f)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && (v->ampenv.segment < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release))
			tsf_voice_endquick(v, f->outSampleRate);
	if (f->channels) { TSF_FREE(f->channels->channels); TSF_FREE(f->channels); f->channels = TSF_NULL; }
}

TSFDEF int32_t tsf_get_presetindex(const tsf* f, int32_t bank, int32_t preset_number)
{
	const struct tsf_preset *presets;
	int32_t i, iMax;
	for (presets = f->presets, i = 0, iMax = f->presetNum; i < iMax; i++)
		if (presets[i].preset == preset_number && presets[i].bank == bank)
			return i;
	return -1;
}

TSFDEF int32_t tsf_get_presetcount(const tsf* f)
{
	return f->presetNum;
}

TSFDEF const char* tsf_get_presetname(const tsf* f, int32_t preset)
{
#ifndef TSF_NO_PRESET_NAME
	return (preset < 0 || preset >= f->presetNum ? TSF_NULL : f->presets[preset].presetName);
#else
	return TSF_NULL;
#endif
}

TSFDEF const char* tsf_bank_get_presetname(const tsf* f, int32_t bank, int32_t preset_number)
{
	return tsf_get_presetname(f, tsf_get_presetindex(f, bank, preset_number));
}

TSFDEF void tsf_set_output(tsf* f, enum TSFOutputMode outputmode, int32_t samplerate, float global_gain_db)
{
	f->outputmode = outputmode;
	f->outSampleRate = (float)(samplerate >= 1 ? samplerate : 44100.0f);
	f->globalGainDB = global_gain_db;
}

static struct tsf_voice *tsf_reusable_voice(tsf * f, float cap) {
	struct tsf_voice *reuseVoice, *v, *vEnd;
	reuseVoice = TSF_NULL;
	float minLevel = 2.0f;
	v = f->voices, vEnd = v + f->voiceNum;
	for (; v != vEnd; v++) {
		if ((v->ampenv.segment == TSF_SEGMENT_DECAY || v->ampenv.segment == TSF_SEGMENT_SUSTAIN || v->ampenv.segment == TSF_SEGMENT_RELEASE || v->ampenv.segment == TSF_SEGMENT_DONE))
			if (v->locked == 0 && v->ampenv.level < minLevel) {
				minLevel = v->ampenv.level;
				reuseVoice = v;
			}
	}

	if (minLevel < cap)
		return reuseVoice;
	else
		return TSF_NULL;
}


TSFDEF void tsf_note_on(tsf* f, int32_t preset_index, int32_t key, float vel)
{
	int16_t midiVelocity = (int16_t)(vel * 127);
	int32_t voicePlayIndex;
	struct tsf_region *region, *regionEnd;

	if (preset_index < 0 || preset_index >= f->presetNum) return;
	if (vel <= 0.0f) { tsf_note_off(f, preset_index, key); return; }

	if (f->presets[preset_index].loaded == TSF_FALSE) {
		tsf_load_preset(f, preset_index);
	}

	// Play all matching regions.
	voicePlayIndex = f->voicePlayIndex++;
	for (region = f->presets[preset_index].regions, regionEnd = region + f->presets[preset_index].regionNum; region != regionEnd; region++)
	{
		struct tsf_voice *voice, *v, *vEnd; TSF_BOOL doLoop; float lowpassFilterQDB, lowpassFc;
		if (key < region->lokey || key > region->hikey || midiVelocity < region->lovel || midiVelocity > region->hivel) continue;

		voice = TSF_NULL, v = f->voices, vEnd = v + f->voiceNum;
		if (region->group)
		{
			for (; v != vEnd; v++)
				if (v->playingPreset == preset_index && v->region->group == region->group) tsf_voice_endquick(v, f->outSampleRate);
				else if (v->playingPreset == -1 && !voice) voice = v;
		}
		else for (; v != vEnd; v++) if (v->playingPreset == -1) { voice = v; break; }
		if (!voice)
		{
			voice = tsf_reusable_voice(f, TSF_REUSE_LEVEL);
		}

		if (voice) {
			voice->locked = 1;
			voice->region = region;
			voice->playingPreset = preset_index;
			voice->playingKey = key;
			voice->playIndex = voicePlayIndex;
			voice->noteGainDB = f->globalGainDB - region->attenuation - tsf_gainToDecibels(1.0f / vel);

			if (f->channels)
			{
				f->channels->setupVoice(f, voice);
			}
			else
			{
				tsf_voice_calcpitchratio(voice, 0, f->outSampleRate);
				// The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).
				voice->panFactorLeft  = TSF_SQRTF(0.5f - region->pan);
				voice->panFactorRight = TSF_SQRTF(0.5f + region->pan);
			}

			// Offset/end.
			voice->sourceSamplePosition = ((uint64_t)region->offset) << 32;

			// Loop.
			doLoop = (region->loop_mode != TSF_LOOPMODE_NONE && region->loop_start < region->loop_end);
			voice->loopStart = (doLoop ? region->loop_start : 0);
			voice->loopEnd = (doLoop ? region->loop_end : 0);

			// Setup envelopes.
			tsf_voice_envelope_setup(&voice->ampenv, &region->ampenv, key, midiVelocity, TSF_TRUE, f->outSampleRate);
			tsf_voice_envelope_setup(&voice->modenv, &region->modenv, key, midiVelocity, TSF_FALSE, f->outSampleRate);

#ifndef TSF_NO_LOWPASS
			// Setup lowpass filter.
			lowpassFc = (region->initialFilterFc <= 13500 ? tsf_cents2Hertz((float)region->initialFilterFc) / f->outSampleRate : 1.0f);
			lowpassFilterQDB = region->initialFilterQ / 10.0f;
			voice->lowpass.QInv = 1.0 / TSF_POWF(10.0, (lowpassFilterQDB / 20.0));
			voice->lowpass.z1 = voice->lowpass.z2 = 0;
			voice->lowpass.active = (lowpassFc < 0.499f);
			if (voice->lowpass.active) tsf_voice_lowpass_setup(&voice->lowpass, lowpassFc);
#endif
			// Setup LFO filters.
			tsf_voice_lfo_setup(&voice->modlfo, region->delayModLFO, region->freqModLFO, f->outSampleRate);
			tsf_voice_lfo_setup(&voice->viblfo, region->delayVibLFO, region->freqVibLFO, f->outSampleRate);
			voice->locked = 0;
		} else {
			// ignore note on
		}
	}
}

// unload unused presets
TSFDEF void tsf_gc(tsf * f) {
	if (f->gc == TSF_GC_F) {
		f->gc = 0;
		struct tsf_voice *v, *vEnd;

		v = f->voices, vEnd = v + f->voiceNum;
		for (; v != vEnd; v++) {
			if (v->playingPreset != -1) {
				f->presets[v->playingPreset].refCount++;
			}
		}

		for (int32_t i = 0; i < f->presetNum; i++) {
			if (f->presets[i].refCount == 0 && f->presets[i].loaded == TSF_TRUE) {
#ifdef TSF_MEM_PROF
				printf("GC unload preset %d\n", i);
#endif
				tsf_unload_preset(f, i);
			}
			f->presets[i].refCount = 0;
		}
	}

	f->gc++;
}

TSFDEF int32_t tsf_bank_note_on(tsf* f, int32_t bank, int32_t preset_number, int32_t key, float vel)
{
	int32_t preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	tsf_note_on(f, preset_index, key, vel);
	return 1;
}

TSFDEF void tsf_note_off(tsf* f, int32_t preset_index, int32_t key)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching preset, key and look up the smallest play index
		if (v->playingPreset != preset_index || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching preset, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
		        (v->playIndex != vMatchFirst->playIndex || v->playingPreset != preset_index || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE)) continue;
		tsf_voice_end(v, f->outSampleRate);
	}
}

TSFDEF int32_t tsf_bank_note_off(tsf* f, int32_t bank, int32_t preset_number, int32_t key)
{
	int32_t preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	tsf_note_off(f, preset_index, key);
	return 1;
}

TSFDEF void tsf_note_off_all(tsf* f)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++) if (v->playingPreset != -1 && v->ampenv.segment < TSF_SEGMENT_RELEASE)
			tsf_voice_end(v, f->outSampleRate);
}

TSFDEF int32_t tsf_active_voice_count(tsf* f)
{
	int32_t count = 0;
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++) if (v->playingPreset != -1) count++;
	return count;
}

TSFDEF void tsf_render_short(tsf* f, int16_t* buffer, int32_t samples, int32_t flag_mixing)
{
	tsf_gc(f);

	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;

	TSF_MEMSET(f->buffer, 0, sizeof(int32_t) * samples * 2);
	TSF_MEMSET(f->chorusBuffer, 0, sizeof(int32_t) * samples);
	TSF_MEMSET(f->reverbBuffer, 0, sizeof(int32_t) * samples);

	if (!flag_mixing) TSF_MEMSET(buffer, 0, (f->outputmode == TSF_MONO ? 1 : 2) * sizeof(int16_t) * samples);

	for (; v != vEnd; v++) {
		if (v->playingPreset != -1 && v->locked == 0) {
			v->locked = 1;
			tsf_voice_render(f, v, f->buffer, f->chorusBuffer, f->reverbBuffer, samples);
			v->locked = 0;
		}
	}

#ifndef TSF_NO_CHORUS
	chorus_process(&f->chorus, f->chorusBuffer, f->buffer, samples);
#endif

#ifndef TSF_NO_REVERB
	reverb_process(&f->rev, f->reverbBuffer, f->buffer, samples);
#endif

	int32_t *inBuf = f->buffer;
	int blkCnt = (samples * 2) >> 2;

	while (blkCnt--) {
		*buffer++ = __SSAT(*inBuf++ >> 15, 16);
		*buffer++ = __SSAT(*inBuf++ >> 15, 16);
		*buffer++ = __SSAT(*inBuf++ >> 15, 16);
		*buffer++ = __SSAT(*inBuf++ >> 15, 16);
	}
}

static void tsf_channel_setup_voice(tsf* f, struct tsf_voice* v)
{
	struct tsf_channel* c = &f->channels->channels[f->channels->activeChannel];
	float newpan = v->region->pan + c->panOffset;
	v->playingChannel = f->channels->activeChannel;
	v->noteGainDB += c->gainDB;
	tsf_voice_calcpitchratio(v, (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning)), f->outSampleRate);
	if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
	else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
	else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
}

static struct tsf_channel* tsf_channel_init(tsf* f, int32_t channel)
{
	int32_t i;
	if (f->channels && channel < f->channels->channelNum) return &f->channels->channels[channel];
	if (!f->channels)
	{
#ifdef TSF_MEM_PROF
		printf("MALLOC struct tsf_channels %ld\n", sizeof(struct tsf_channels));
#endif
		f->channels = (struct tsf_channels*)TSF_MALLOC(sizeof(struct tsf_channels));
		f->channels->setupVoice = &tsf_channel_setup_voice;
		f->channels->channels = NULL;
		f->channels->channelNum = 0;
		f->channels->activeChannel = 0;
	}
	i = f->channels->channelNum;
	f->channels->channelNum = channel + 1;

#ifdef TSF_MEM_PROF
	printf("REALLOC struct tsf_channel %d * %ld = %ld\n", f->channels->channelNum, sizeof(struct tsf_channel), f->channels->channelNum * sizeof(struct tsf_channel));
#endif
	f->channels->channels = (struct tsf_channel*)TSF_REALLOC(f->channels->channels, f->channels->channelNum * sizeof(struct tsf_channel));
	for (; i <= channel; i++)
	{
		struct tsf_channel* c = &f->channels->channels[i];
		c->presetIndex = c->bank = 0;
		c->pitchWheel = c->midiPan = 8192;
		c->midiVolume = c->midiExpression = 16383;
		c->midiRPN = 0xFFFF;
		c->midiData = 0;
		c->panOffset = 0.0f;
		c->gainDB = 0.0f;
		c->pitchRange = 2.0f;
		c->tuning = 0.0f;
		c->chorus = 0.0f;
		c->reverb = 0.0f;
	}
	return &f->channels->channels[channel];
}

static void tsf_channel_applypitch(tsf* f, int32_t channel, struct tsf_channel* c)
{
	struct tsf_voice *v, *vEnd;
	float pitchShift = (c->pitchWheel == 8192 ? c->tuning : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f) - c->pitchRange + c->tuning));
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingChannel == channel && v->playingPreset != -1)
			tsf_voice_calcpitchratio(v, pitchShift, f->outSampleRate);
}

TSFDEF void tsf_channel_set_presetindex(tsf* f, int32_t channel, int32_t preset_index)
{
	tsf_channel_init(f, channel)->presetIndex = (uint16_t)preset_index;
}

TSFDEF int32_t tsf_channel_set_presetnumber(tsf* f, int32_t channel, int32_t preset_number, int32_t flag_mididrums)
{
	//printf("channel_set_presetnumber preset:%d channel:%d\n",channel,preset_number);

	struct tsf_channel *c = tsf_channel_init(f, channel);
	int32_t preset_index;

	if (flag_mididrums)
	{
		preset_index = tsf_get_presetindex(f, 128 | (c->bank & 0x7FFF), preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, 128, preset_number);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, 128, 0);
		if (preset_index == -1) preset_index = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
	}
	else preset_index = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
	if (preset_index == -1) preset_index = tsf_get_presetindex(f, 0, preset_number);
	if (preset_index != -1)
	{
		c->presetIndex = (uint16_t)preset_index;
		return 1;
	}
	return 0;
}

TSFDEF void tsf_channel_set_bank(tsf* f, int32_t channel, int32_t bank)
{
	tsf_channel_init(f, channel)->bank = (uint16_t)bank;
}

TSFDEF int32_t tsf_channel_set_bank_preset(tsf* f, int32_t channel, int32_t bank, int32_t preset_number)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	int32_t preset_index = tsf_get_presetindex(f, bank, preset_number);
	if (preset_index == -1) return 0;
	c->presetIndex = (uint16_t)preset_index;
	c->bank = (uint16_t)bank;
	return 1;
}

TSFDEF void tsf_channel_set_pan(tsf* f, int32_t channel, float pan)
{
	struct tsf_voice *v, *vEnd;
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingChannel == channel && v->playingPreset != -1)
		{
			float newpan = v->region->pan + pan - 0.5f;
			if      (newpan <= -0.5f) { v->panFactorLeft = 1.0f; v->panFactorRight = 0.0f; }
			else if (newpan >=  0.5f) { v->panFactorLeft = 0.0f; v->panFactorRight = 1.0f; }
			else { v->panFactorLeft = TSF_SQRTF(0.5f - newpan); v->panFactorRight = TSF_SQRTF(0.5f + newpan); }
		}
	tsf_channel_init(f, channel)->panOffset = pan - 0.5f;
}

TSFDEF void tsf_channel_set_volume(tsf* f, int32_t channel, float volume)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	float gainDB = tsf_gainToDecibels(volume), gainDBChange = gainDB - c->gainDB;
	struct tsf_voice *v, *vEnd;
	if (gainDBChange == 0) return;
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingChannel == channel && v->playingPreset != -1)
			v->noteGainDB += gainDBChange;
	c->gainDB = gainDB;
}

TSFDEF void tsf_channel_set_pitchwheel(tsf* f, int32_t channel, int32_t pitch_wheel)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (c->pitchWheel == pitch_wheel) return;
	c->pitchWheel = (uint16_t)pitch_wheel;
	tsf_channel_applypitch(f, channel, c);
}

TSFDEF void tsf_channel_set_pitchrange(tsf* f, int32_t channel, float pitch_range)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (c->pitchRange == pitch_range) return;
	c->pitchRange = pitch_range;
	if (c->pitchWheel != 8192) tsf_channel_applypitch(f, channel, c);
}

TSFDEF void tsf_channel_set_tuning(tsf* f, int32_t channel, float tuning)
{
	struct tsf_channel *c = tsf_channel_init(f, channel);
	if (c->tuning == tuning) return;
	c->tuning = tuning;
	tsf_channel_applypitch(f, channel, c);
}

TSFDEF void tsf_channel_note_on(tsf* f, int32_t channel, int32_t key, float vel)
{
	if (!f->channels || channel >= f->channels->channelNum) return;
	f->channels->activeChannel = channel;
	tsf_note_on(f, f->channels->channels[channel].presetIndex, key, vel);
}

TSFDEF void tsf_channel_note_off(tsf* f, int32_t channel, int32_t key)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum, *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
	for (; v != vEnd; v++)
	{
		//Find the first and last entry in the voices list with matching channel, key and look up the smallest play index
		if (v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE) continue;
		else if (!vMatchFirst || v->playIndex < vMatchFirst->playIndex) vMatchFirst = vMatchLast = v;
		else if (v->playIndex == vMatchFirst->playIndex) vMatchLast = v;
	}
	if (!vMatchFirst) return;
	for (v = vMatchFirst; v <= vMatchLast; v++)
	{
		//Stop all voices with matching channel, key and the smallest play index which was enumerated above
		if (v != vMatchFirst && v != vMatchLast &&
		        (v->playIndex != vMatchFirst->playIndex || v->playingPreset == -1 || v->playingChannel != channel || v->playingKey != key || v->ampenv.segment >= TSF_SEGMENT_RELEASE)) continue;
		tsf_voice_end(v, f->outSampleRate);
	}
}

TSFDEF void tsf_channel_note_off_all(tsf* f, int32_t channel)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && v->ampenv.segment < TSF_SEGMENT_RELEASE)
			tsf_voice_end(v, f->outSampleRate);
}

TSFDEF void tsf_channel_sounds_off_all(tsf* f, int32_t channel)
{
	struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
	for (; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel && (v->ampenv.segment < TSF_SEGMENT_RELEASE || v->ampenv.parameters.release))
			tsf_voice_endquick(v, f->outSampleRate);
}

TSFDEF void tsf_channel_midi_control(tsf* f, int32_t channel, int32_t controller, int32_t control_value)
{
	struct tsf_channel* c = tsf_channel_init(f, channel);
	switch (controller)
	{
	case   7 /*VOLUME_MSB*/      : c->midiVolume     = (uint16_t)((c->midiVolume     & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
	case  39 /*VOLUME_LSB*/      : c->midiVolume     = (uint16_t)((c->midiVolume     & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
	case  11 /*EXPRESSION_MSB*/  : c->midiExpression = (uint16_t)((c->midiExpression & 0x7F  ) | (control_value << 7)); goto TCMC_SET_VOLUME;
	case  43 /*EXPRESSION_LSB*/  : c->midiExpression = (uint16_t)((c->midiExpression & 0x3F80) |  control_value);       goto TCMC_SET_VOLUME;
	case  10 /*PAN_MSB*/         : c->midiPan        = (uint16_t)((c->midiPan        & 0x7F  ) | (control_value << 7)); goto TCMC_SET_PAN;
	case  42 /*PAN_LSB*/         : c->midiPan        = (uint16_t)((c->midiPan        & 0x3F80) |  control_value);       goto TCMC_SET_PAN;
	case   6 /*DATA_ENTRY_MSB*/  : c->midiData       = (uint16_t)((c->midiData       & 0x7F)   | (control_value << 7)); goto TCMC_SET_DATA;
	case  38 /*DATA_ENTRY_LSB*/  : c->midiData       = (uint16_t)((c->midiData       & 0x3F80) |  control_value);       goto TCMC_SET_DATA;
	case   0 /*BANK_SELECT_MSB*/ : c->bank = (uint16_t)(0x8000 | control_value); return; //bank select MSB alone acts like LSB
	case  32 /*BANK_SELECT_LSB*/ : c->bank = (uint16_t)((c->bank & 0x8000 ? ((c->bank & 0x7F) << 7) : 0) | control_value); return;
	case 101 /*RPN_MSB*/         : c->midiRPN = (uint16_t)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x7F  ) | (control_value << 7)); return;
	case 100 /*RPN_LSB*/         : c->midiRPN = (uint16_t)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN) & 0x3F80) |  control_value); return;
	case  98 /*NRPN_LSB*/        : c->midiRPN = 0xFFFF; return;
	case  99 /*NRPN_MSB*/        : c->midiRPN = 0xFFFF; return;
	case 120 /*ALL_SOUND_OFF*/   : tsf_channel_sounds_off_all(f, channel); return;
	case 123 /*ALL_NOTES_OFF*/   : tsf_channel_note_off_all(f, channel);   return;
	case 121 /*ALL_CTRL_OFF*/    :
		c->midiVolume = c->midiExpression = 16383;
		c->midiPan = 8192;
		c->bank = 0;
		tsf_channel_set_volume(f, channel, 1.0f);
		tsf_channel_set_pan(f, channel, 0.5f);
		tsf_channel_set_pitchrange(f, channel, 2.0f);
		return;
	case 91 /* reverb */ : c->reverb = (float)control_value / 127.0f; /* printf("TSF send reverb %f\n", c->reverb); */ return;
	case 93 /* chorus */ : c->chorus = (float)control_value / 127.0f; /* printf("TSF send chorus %f\n", c->chorus); */ return;
		// default: printf("UNKNOWN CC %d(%x) : %d %d\n",controller,controller,channel,control_value); return;
	}
	return;
TCMC_SET_VOLUME:
	//Raising to the power of 3 seems to result in a decent sounding volume curve for MIDI
	tsf_channel_set_volume(f, channel, TSF_POWF((c->midiVolume / 16383.0f) * (c->midiExpression / 16383.0f), 3.0f));
	return;
TCMC_SET_PAN:
	tsf_channel_set_pan(f, channel, c->midiPan / 16383.0f);
	return;
TCMC_SET_DATA:
	if      (c->midiRPN == 0) tsf_channel_set_pitchrange(f, channel, (c->midiData >> 7) + 0.01f * (c->midiData & 0x7F));
	else if (c->midiRPN == 1) tsf_channel_set_tuning(f, channel, (int32_t)c->tuning + ((float)c->midiData - 8192.0f) / 8192.0f); //fine tune
	else if (c->midiRPN == 2 && controller == 6) tsf_channel_set_tuning(f, channel, ((float)control_value - 64.0f) + (c->tuning - (int32_t)c->tuning)); //coarse tune
	return;
}

TSFDEF int32_t tsf_channel_get_preset_index(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].presetIndex : 0);
}

TSFDEF int32_t tsf_channel_get_preset_bank(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? (f->channels->channels[channel].bank & 0x7FFF) : 0);
}

TSFDEF int32_t tsf_channel_get_preset_number(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->presets[f->channels->channels[channel].presetIndex].preset : 0);
}

TSFDEF float tsf_channel_get_pan(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].panOffset - 0.5f : 0.5f);
}

TSFDEF float tsf_channel_get_volume(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? tsf_decibelsToGain(f->channels->channels[channel].gainDB) : 1.0f);
}

TSFDEF int32_t tsf_channel_get_pitchwheel(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchWheel : 8192);
}

TSFDEF float tsf_channel_get_pitchrange(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].pitchRange : 2.0f);
}

TSFDEF float tsf_channel_get_tuning(tsf* f, int32_t channel)
{
	return (f->channels && channel < f->channels->channelNum ? f->channels->channels[channel].tuning : 0.0f);
}

#ifdef __cplusplus
}
#endif

#endif //TSF_IMPLEMENTATION
