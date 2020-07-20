#include "usbd_midi_if.h"
#include "qspi_wrapper.h"
#ifdef TSF_SYNTH
#include "tsf.h"
#else
#include "efluidsynth.h"
#endif

#include "config.h"
#include "midi.h"
#include "synth.h"

static int8_t Midi_Receive(uint8_t *msg, uint32_t len);

extern osMessageQueueId_t midi_queue;

extern USBD_HandleTypeDef USBD_Device;
USBD_Midi_ItfTypeDef USBD_Midi_fops = {
	Midi_Receive,
};

static int8_t Midi_Receive(uint8_t *msg, uint32_t len) {

#ifdef QUEUED_MIDI_MESSAGES
	if(len <= MAX_MIDI_LEN) {
	    struct midi_message midi_msg;
    	memcpy(midi_msg.data, msg, len);
	    midi_msg.len = len;

		osMessageQueuePut(midi_queue, &midi_msg, 0U, 0U);
	}
#else
	if (synth_available()) {
	  #ifdef LED2_PIN
    	BSP_LED_On(LED2);
	  #endif
		synth_midi_process(msg, len);
	  #ifdef LED2_PIN
    	BSP_LED_Off(LED2);
	  #endif
	}
#endif	

	return 0;
}