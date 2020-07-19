#ifndef CONFIG_H
#define CONFIG_H

#define MASTER_VOLUME 70
#define SAMPLE_RATE 48000
#define POLYPHONY 64
#define AUDIO_BUF_SIZE (AUDIO_TOTAL_BUF_SIZE) // from usb_audio, 3840

#define MB_MALLOC malloc
#define MB_REALLOC realloc
#define MB_FREE free
#define MB_MEMSET memset
#define MB_MEMCPY memcpy

//#define MUCISBOARD_USB_AUDIO_ONLY
//#define MUCISBOARD_USB_MIDI_ONLY
//#define MUCISBOARD_USB_MSC_ONLY

//#define MUCISBOARD_USB_MSC_DISABLE

#define MUCISBOARD_USB_COMPOSITE

//#define QUEUED_MIDI_MESSAGES // buggy

#endif