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

}

void synth_update(uint8_t *buf, uint32_t bufpos, uint32_t bufsize) {
  if (synth_available()) {
    QSPI_readonly_mode();
    if (QSPI_wrote_get()) {
#ifdef TSF_SYNTH
      tsf_close(synth);
#else
      delete_fluid_synth(synth);
#endif
      synth_init();
      QSPI_wrote_clear();
    }
#ifdef TSF_SYNTH
    tsf_render_short(synth, (int16_t *)&synth_buf[bufpos], bufsize / 4, 0);
#else
    fluid_synth_write_s16(synth, bufsize / 4, (int16_t *)&synth_buf[bufpos], 0, 2, (int16_t *)&synth_buf[bufpos], 1, 2 );
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
  } else {
    memset(&buf[0] + bufpos, 0, bufsize);
  }
}

uint8_t synth_available() {
  return (synth && QSPI_ready());
}
