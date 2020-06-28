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

void midi_process(uint8_t *msg, uint32_t len);

#endif 