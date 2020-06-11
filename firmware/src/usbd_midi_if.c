#include "usbd_midi_if.h"
#include "qspi_wrapper.h"
#include "tsf.h"

extern tsf* synth;

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

		switch (msgtype) {
		case 0x80:
			tsf_channel_note_off(synth, chan, b1);
			break;
		case 0x90:
			tsf_channel_note_on(synth, chan, b1, b2 / 127.0f);
			break;
		case 0xB0:
			tsf_channel_midi_control(synth, chan, b1, b2);
			break;
		case 0xC0:
			tsf_channel_set_presetnumber(synth, chan, b1, (chan == 9));
			break;
		case 0xD0:
			break;
		case 0xE0:
			tsf_channel_set_pitchwheel(synth, chan, b);
			break;
		default:
			break;
		}
	}

	return 0;
}