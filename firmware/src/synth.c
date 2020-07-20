#include "synth.h"
#include "config.h"
#include "qspi_wrapper.h"

#ifdef TSF_SYNTH
#define TSF_RENDER_EFFECTSAMPLEBLOCK 512
#define TSF_NO_PRESET_NAME
//#define TSF_NO_INTERPOLATION
//#define TSF_NO_LOWPASS
//#define TSF_NO_REVERB
//#define TSF_NO_CHORUS

#define TSF_FILE QSPI_FILE
#define TSF_MMAP(p,s,f) QSPI_mmap(0,s,f)
#define TSF_FOPEN QSPI_fopen
#define TSF_FREAD QSPI_fread
#define TSF_FTELL QSPI_ftell
#define TSF_FSEEK QSPI_fseek

#define TSF_MALLOC MB_MALLOC
#define TSF_REALLOC MB_REALLOC
#define TSF_FREE MB_FREE

#ifdef QUEUED_MIDI_MESSAGES

void synth_mutex_lock(osMutexId_t mutex) {
  osMutexAcquire(mutex, osWaitForever);
}

void synth_mutex_unlock(osMutexId_t mutex) {
  osMutexRelease(mutex);
}

#define TSF_MUTEX_TYPEDEF osMutexId_t
#define TSF_MUTEX_INIT osMutexNew(NULL)
#define TSF_MUTEX_DEINIT osMutexDelete
#define TSF_MUTEX_LOCK synth_mutex_lock
#define TSF_MUTEX_UNLOCK synth_mutex_unlock
#endif

#define TSF_IMPLEMENTATION
#include "tsf.h"
#else
#include "efluidsynth.h"
#endif

uint8_t synth_buf[AUDIO_BUF_SIZE];
uint8_t initialized = 0;

#ifdef USE_FREERTOS
extern osMutexId_t synth_mutex;
#endif

#ifdef TSF_SYNTH
tsf* synth = NULL;
#else
fluid_synth_t* synth = NULL;
#endif

/* SYSEX callbacks */
void midi_sysex_reset(void) {
#ifndef QUEUED_MIDI_MESSAGES // FIXME
  synth_reset();
#endif
}

#ifdef TSF_SYNTH
void midi_sysex_set_master_volume(uint8_t vol) {
  synth_set_volume((float)vol / 127.0f);
}

void midi_sysex_set_reverb_type(uint8_t rev_type) {
#ifndef TSF_NO_REVERB
  switch (rev_type) {
  case 0:
    tsf_reverb_setup(synth, 0.0f, 0.1f, 0.4f);
    break;
  case 1:
    tsf_reverb_setup(synth, 0.0f, 0.2f, 0.5f);
    break;
  case 2:
    tsf_reverb_setup(synth, 0.0f, 0.4f, 0.6f);
    break;
  case 3:
    tsf_reverb_setup(synth, 0.0f, 0.6f, 0.7f);
    break;
  case 4:
    tsf_reverb_setup(synth, 0.0f, 0.7f, 0.7f); // default large hall
    break;
  case 8:
    tsf_reverb_setup(synth, 0.0f, 0.7f, 0.5f);
    break;
  }
#endif
}

void midi_sysex_set_chorus_type(uint8_t chorus_type) {
#ifndef TSF_NO_CHORUS
  switch (chorus_type) {
  case 0:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 0.4f, 1.9f);
    break;
  case 1:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 1.1f, 6.3f);
    break;
  case 2:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 0.4f, 6.3f); // default chorus 3
    break;
  case 3:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 1.1f, 5.3f);
    break;
  case 4:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 0.2f, 7.8f);
    break;
  case 5:
    tsf_chorus_setup(synth, 50.0f, 0.5f, 0.1f, 1.9f);
    break;
  }
#endif
}
#endif

void synth_buffer_init(void) {
  memset(synth_buf, 0, AUDIO_BUF_SIZE);
}

void synth_init() {
#ifdef TSF_SYNTH
  synth = tsf_load_filename(NULL);
  if (synth) {
    tsf_set_max_voices(synth, POLYPHONY);
    tsf_set_output(synth, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    tsf_channel_set_presetnumber(synth, 0, 0, 0);
    initialized = 1;
  }
#else
  fluid_settings_t settings;
  fluid_synth_settings(&settings);
  settings.sample_rate = SAMPLE_RATE;
  settings.reverb = 0;
  settings.chorus = 0;
  settings.polyphony = POLYPHONY;

  synth = new_fluid_synth(&settings);
  if (synth) {
    fluid_synth_set_interp_method(synth, -1, FLUID_INTERP_NONE);
    fluid_synth_sfload(synth, "font.sf2", 1);
    initialized = 1;
  }
#endif
}

// free/reinit synth
void synth_reset() {
//  osMutexAcquire(synth_mutex, osWaitForever);
  initialized = 0;
#ifdef TSF_SYNTH
  tsf_close(synth);
#else
  delete_fluid_synth(synth);
#endif
  synth = NULL;
  synth_init();
//  osMutexRelease(synth_mutex);
}

// set master volume
void synth_set_volume(float vol) {
#ifdef TSF_SYNTH
  tsf_set_volume(synth, vol);
#endif
}

// reset synthesizer if ROM was updated
void synth_reset_updated() {
  QSPI_readonly_mode();
  if (QSPI_wrote_get()) {
    synth_reset();
    QSPI_wrote_clear();
    QSPI_clear_writing();
  }
}

uint8_t synth_available() {
  return (initialized && QSPI_ready());
}

void synth_midi_process(uint8_t *msg, uint32_t len) {
  midi_process(synth, msg, len);
}

#ifdef USE_FREERTOS
void synth_render (uint32_t bufpos, uint32_t bufsize) {
  if (synth_available()) {
    synth_reset_updated();
//    osMutexAcquire(synth_mutex, osWaitForever);
    tsf_render_short(synth, (int16_t *)&synth_buf[bufpos], bufsize / 4, 0);
//    osMutexRelease(synth_mutex);
  }
}
#endif

void synth_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize) {
#ifndef USE_FREERTOS
  if (synth_available()) {
    synth_reset_updated();
#ifdef TSF_SYNTH
    tsf_render_short(synth, (int16_t *)&synth_buf[bufpos], bufsize / 4, 0);
#else
    fluid_synth_write_s16(synth, bufsize / 4, (int16_t *)&synth_buf[bufpos], 0, 2, (int16_t *)&synth_buf[bufpos], 1, 2 );
#endif
#endif

    int16_t *out = (int16_t *)&buf[0] + bufpos / 2;
    int16_t *in = (int16_t *)synth_buf + bufpos / 2;

    int blkCnt = (bufsize / 2) >> 2;
    while (blkCnt--) {
      *out++ = *in++ >> 1;
      *out++ = *in++ >> 1;
      *out++ = *in++ >> 1;
      *out++ = *in++ >> 1;
    }
#ifndef USE_FREERTOS
  } else {
    memset(&buf[0] + bufpos, 0, bufsize);
  }
#endif
}

