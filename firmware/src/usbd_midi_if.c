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

extern osMessageQueueId_t midi_queue;

extern USBD_HandleTypeDef USBD_Device;
USBD_Midi_ItfTypeDef USBD_Midi_fops = {
	Midi_Receive,
};

static int8_t Midi_Receive(uint8_t *msg, uint32_t len) {
  #ifdef LED2_PIN
//    BSP_LED_Toggle(LED2);
  #endif

	if(len <= MAX_MIDI_SYSEX_LEN) {
	    struct midi_message midi_msg;
    	memcpy(midi_msg.data, msg, len);
	    midi_msg.len = len;

		osMessageQueuePut(midi_queue, &midi_msg, 0U, 0U);
	    // osSemaphoreRelease(queue_sem);
	}

/*	if (synth_available()) {
		midi_process(synth, msg, len);
	}
	*/

	return 0;
}