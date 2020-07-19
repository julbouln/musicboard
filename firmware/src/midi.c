#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef TSF_SYNTH
#include "tsf.h"
#else
#include "efluidsynth.h"
#endif

#include "midi.h"
#include "config.h"

//#define SYSEX_DEBUG

struct midi_sysex sysex;

__attribute__((weak)) void midi_sysex_reset(void) {
}

__attribute__((weak)) void midi_sysex_set_master_volume(uint8_t vol) {
}

__attribute__((weak)) void midi_sysex_set_reverb_type(uint8_t rev_type) {
}

__attribute__((weak)) void midi_sysex_set_chorus_type(uint8_t chorus_type) {
}

void midi_sysex_universal_process(struct midi_universal_sysex *man_sysex) {
	switch (man_sysex->addr) {
	case SYSEX_UNIVERSAL_RESET:
		midi_sysex_reset();
		break;
	case SYSEX_UNIVERSAL_SET_MASTER_VOLUME:
		midi_sysex_set_master_volume(man_sysex->data & 0xFF);
		break;
	}
}

void midi_sysex_gs_process(struct midi_gs_sysex *man_sysex) {
	switch (man_sysex->addr) {
	case SYSEX_GS_RESET:
		midi_sysex_reset();
		break;
	case SYSEX_GS_SET_MASTER_VOLUME:
		midi_sysex_set_master_volume(man_sysex->data);
		break;
	case SYSEX_GS_SET_REVERB_TYPE:
		midi_sysex_set_reverb_type(man_sysex->data);
		break;
	case SYSEX_GS_SET_CHORUS_TYPE:
		midi_sysex_set_chorus_type(man_sysex->data);
		break;
	}
}

void midi_sysex_end_process() {
	switch (sysex.manufacturer) {
	case SYSEX_MANUFACTURER_UNIVERSAL_NONREALTIME:
	case SYSEX_MANUFACTURER_UNIVERSAL_REALTIME:
	{
		struct midi_universal_sysex *man_sysex = (struct midi_universal_sysex *)MB_MALLOC(sizeof(struct midi_universal_sysex));
		man_sysex->addr = sysex.data[1] << 8 | sysex.data[2];
		if (sysex.data[3] != 0xF7) {
			man_sysex->data = sysex.data[3] << 8 | sysex.data[4];
		} else {
			man_sysex->data = 0;
		}
		sysex.manufacturer_data = man_sysex;
#ifdef SYSEX_DEBUG
		printf("UNIVERSAL SYSEX addr:%x data:%x\n",
		       man_sysex->addr, man_sysex->data
		      );
#endif
		midi_sysex_universal_process(man_sysex);
	}
	break;
	case SYSEX_MANUFACTURER_ROLAND:
		if (sysex.data[1] == SYSEX_ROLAND_MODEL_GS) {
			struct midi_gs_sysex *man_sysex = (struct midi_gs_sysex *)MB_MALLOC(sizeof(struct midi_gs_sysex));
			man_sysex->device_id = sysex.data[0];
			man_sysex->model_id = sysex.data[1];
			man_sysex->command_id = sysex.data[2];
			man_sysex->addr = sysex.data[3] << 16 | sysex.data[4] << 8 | sysex.data[5];
			man_sysex->data = sysex.data[6];
			man_sysex->checksum = sysex.data[7];
			sysex.manufacturer_data = man_sysex;
#ifdef SYSEX_DEBUG
			printf("GS SYSEX device_id:%x model_id:%x command_id:%x addr:%x data:%x checksum:%x\n",
			       man_sysex->device_id, man_sysex->model_id, man_sysex->command_id,
			       man_sysex->addr, man_sysex->data, man_sysex->checksum
			      );
#endif
			midi_sysex_gs_process(man_sysex);
		}
		break;
	}
}

void midi_sysex_process(uint8_t *msg, uint32_t len) {
	if (msg[0] == MIDI_SYSEX_START) {
		memset(&sysex, 0, sizeof(struct midi_sysex));
		sysex.len = len;
		sysex.len--;
		sysex.manufacturer = msg[1];
		sysex.len--;
		if (sysex.manufacturer == 0x00) {
			sysex.manufacturer = msg[2] << 16 | msg[3];
			sysex.len -= 2;
		}
		sysex.started = 1;
	}
	if (sysex.started && !sysex.ended) {
		for (int i = 0; i < sysex.len; i++) {
			int j = i + (len - sysex.len);
			if (msg[j] == MIDI_SYSEX_END) {
				sysex.ended = 1;
				midi_sysex_end_process();
				sysex.len--;
			} else {
				sysex.data[i] = msg[j];
				sysex.pos++;
			}
		}
	}

}

void midi_process(void *userdata, uint8_t *msg, uint32_t len) {
	uint8_t chan = msg[0] & 0xf;
	uint8_t msgtype = msg[0] & 0xf0;
	uint8_t b1 =  msg[1];
	uint8_t b2 =  msg[2];
	uint16_t b = ((b2 & 0x7f) << 7) | (b1 & 0x7f);

#ifdef TSF_SYNTH
	tsf* synth = (tsf *)userdata;
	switch (msgtype) {
	case MIDI_NOTE_OFF:
		tsf_channel_note_off(synth, chan, b1);
		break;
	case MIDI_NOTE_ON:
		tsf_channel_note_on(synth, chan, b1, b2 / 127.0f);
		break;
	case MIDI_CONTROL_CHANGE:
		tsf_channel_midi_control(synth, chan, b1, b2);
		break;
	case MIDI_PROGRAM_CHANGE:
		tsf_channel_set_presetnumber(synth, chan, b1, (chan == 9));
		break;
	case MIDI_CHANNEL_PRESSURE:
		// unsupported
		break;
	case MIDI_PITCH_BEND:
		tsf_channel_set_pitchwheel(synth, chan, b);
		break;
		break;
	default:
		break;
	}
#else
	fluid_synth_t* synth = (fluid_synth_t*)userdata;
	switch (msgtype) {
	case MIDI_NOTE_OFF:
		fluid_synth_noteoff(synth, chan, b1);
		break;
	case MIDI_NOTE_ON:
		fluid_synth_noteon(synth, chan, b1, b2);
		break;
	case MIDI_CONTROL_CHANGE:
		fluid_synth_cc(synth, chan, b1, b2);
		break;
	case MIDI_PROGRAM_CHANGE:
		fluid_synth_program_change(synth, chan, b1);
		break;
	case MIDI_CHANNEL_PRESSURE:
		fluid_synth_channel_pressure(synth, chan, b1);
		break;
	case MIDI_PITCH_BEND:
		fluid_synth_pitch_bend(synth, chan, b);
		break;
	default:
		break;
	}
#endif

#if 1
	midi_sysex_process(msg, len);
	if (sysex.ended == 1) {
#ifdef SYSEX_DEBUG
		printf("SYSEX (processed) manufacturer:%x len:%d data:", sysex.manufacturer, sysex.len);
		for (int i = 0; i < sysex.len; i++) {
			printf("%x ", sysex.data[i]);
		}
		printf("\n");
#endif
		if (sysex.manufacturer_data) {
			MB_FREE(sysex.manufacturer_data);
		}
		memset(&sysex, 0, sizeof(struct midi_sysex));
	}
#endif

}