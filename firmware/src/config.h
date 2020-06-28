#ifndef CONFIG_H
#define CONFIG_H

#define MASTER_VOLUME 70
#define SAMPLE_RATE 48000
#define POLYPHONY 48 // with chorus & reverb, we can only achieve 48 voices
#define AUDIO_BUF_SIZE (AUDIO_TOTAL_BUF_SIZE) // from usb_audio

#endif