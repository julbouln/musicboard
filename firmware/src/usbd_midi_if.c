#include "usbd_midi_if.h"
#include "qspi_wrapper.h"
#ifdef TSF_SYNTH
#include "tsf.h"
#else
#include "efluidsynth.h"
#endif

#include "midi.h"

#ifdef TSF_SYNTH
extern tsf* synth;
#else
extern fluid_synth_t* synth;
#endif

static int8_t Midi_Receive(uint8_t *msg, uint32_t len);

extern USBD_HandleTypeDef USBD_Device;
USBD_Midi_ItfTypeDef USBD_Midi_fops = {
	Midi_Receive,
};

static int8_t Midi_Receive(uint8_t *msg, uint32_t len) {
	if (synth && QSPI_ready()) {
		uint8_t chan = msg[1] & 0xf;
		uint8_t msgtype = msg[1] & 0xf0;
		uint8_t b1 =  msg[2];
		uint8_t b2 =  msg[3];
		uint16_t b = ((b2 & 0x7f) << 7) | (b1 & 0x7f);

#ifdef TSF_SYNTH
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
		default:
			break;
		}
#else
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
	}

	return 0;
}