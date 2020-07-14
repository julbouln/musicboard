#ifndef __MIDI_H
#define __MIDI_H

#define MIDI_BAUD_RATE 31250 

#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_POLY_PRESSURE 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_BEND 0xE0

#define MAX_MIDI_SYSEX_LEN 512

#define MIDI_SYSEX_START 0xF0
#define MIDI_SYSEX_END 0xF7

#define SYSEX_MANUFACTURER_ROLAND 0x41
#define SYSEX_MANUFACTURER_YAMAHA 0x43

#define SYSEX_MANUFACTURER_UNIVERSAL_NONREALTIME 0x7E
#define SYSEX_MANUFACTURER_UNIVERSAL_REALTIME 0x7F

#define SYSEX_ROLAND_MODEL_GS 0x42

#define SYSEX_GS_RESET 0x40007f
#define SYSEX_GS_SET_MASTER_VOLUME 0x400004
#define SYSEX_GS_SET_REVERB_TYPE 0x400130
#define SYSEX_GS_SET_CHORUS_TYPE 0x400138

#define SYSEX_UNIVERSAL_RESET 0x0901
#define SYSEX_UNIVERSAL_SET_MASTER_VOLUME 0x0401


struct midi_sysex {
	uint16_t manufacturer;
	uint8_t data[MAX_MIDI_SYSEX_LEN];
	uint16_t pos;
	uint16_t len;

	uint8_t started, ended;

	void *manufacturer_data;
};

struct midi_gs_sysex {
	uint8_t device_id;
	uint8_t model_id;
	uint8_t command_id;
	uint32_t addr;
	uint8_t data;
	uint8_t checksum;
};

struct midi_universal_sysex {
	uint16_t addr;
	uint16_t data;
};

struct midi_sysex_functions {
	void (*reset)(void);
	void (*set_reverb_type)(uint8_t);
};

void midi_process(void *userdata, uint8_t *msg, uint32_t len);

#endif 