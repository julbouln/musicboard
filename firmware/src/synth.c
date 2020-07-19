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

#define TSF_IMPLEMENTATION
#include "tsf.h"
#else
#include "efluidsynth.h"
#endif

uint8_t synth_buf[AUDIO_BUF_SIZE];

#ifdef TSF_SYNTH
tsf* synth = NULL;
#else
fluid_synth_t* synth = NULL;
#endif

/* SYSEX callbacks */
void midi_sysex_reset(void) {
  synth_reset();
}

#ifdef TSF_SYNTH
void midi_sysex_set_master_volume(uint8_t vol) {
  synth_set_volume((float)vol/127.0f);
}

void midi_sysex_set_reverb_type(uint8_t rev_type) {
#ifndef TSF_NO_REVERB
  switch(rev_type) {
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
  switch(chorus_type) {
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

void synth_init() {
#ifdef TSF_SYNTH
  synth = tsf_load_filename(NULL);
  if (synth) {
    tsf_set_max_voices(synth, POLYPHONY);
    tsf_set_output(synth, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    tsf_channel_set_presetnumber(synth, 0, 0, 0);
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
  }
#endif

  memset(synth_buf, 0, AUDIO_BUF_SIZE);

}

// free/reinit synth
void synth_reset() {
#ifdef TSF_SYNTH
      tsf_close(synth);
#else
      delete_fluid_synth(synth);
#endif
      synth_init();
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
    }  
}

uint8_t synth_available() {
  return (synth && QSPI_ready());
}

void synth_render (uint32_t bufpos, uint32_t bufsize) {
  if (synth_available()) {
//  #ifdef LED2_PIN
//    BSP_LED_Toggle(LED2);
//  #endif

    synth_reset_updated();
    tsf_render_short(synth, (int16_t *)&synth_buf[bufpos], bufsize / 4, 0);
  }
}

void synth_midi_process(uint8_t *msg, uint32_t len) {
    midi_process(synth, msg, len);
}

void synth_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize) {
#if 0
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
#if 0
  } else {
    memset(&buf[0] + bufpos, 0, bufsize);
  }
#endif
}

