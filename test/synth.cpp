/* complete linux synthesizer using RtAudio and RtMidi */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

//#define TSF_NO_STDIO
#define TSF_RENDER_EFFECTSAMPLEBLOCK 256

#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "RtAudio.h"
#include "RtMidi.h"

#define SAMPLE_RATE 44100

#define AUDIO_FORMAT RTAUDIO_SINT16

#define FRAME_SIZE 1024

using namespace std;

void midiCallback(double deltatime, vector<uint8_t>* msg, void* userData)
{
  tsf *synth = (tsf *)userData;

  int chan = (*msg)[0] & 0xf;
  int msgtype = (*msg)[0] & 0xf0;
  int b1 =  (*msg)[1];
  int b2 =  (*msg)[2];
  int b = ((b2 & 0x7f) << 7) | (b1 & 0x7f);

//  printf("%f ", deltatime);

  switch (msgtype) {
  case 0x80:
//    std::cout << "MIDI Note OFF  C: " << chan << " N: " << b1 << std::endl;
    tsf_channel_note_off(synth, chan, b1);
    break;
  case 0x90:
//    std::cout << "MIDI Note ON   C: " << chan << " N: " << b1 << " V: " << b2 << std::endl;
    tsf_channel_note_on(synth, chan, b1, b2 / 127.0f);
    break;
  case 0xB0:
    tsf_channel_midi_control(synth, chan, b1, b2);
//    std::cout << "MIDI Control change   C: " << chan << " B1: " << b1 << " B2: " << b2 << std::endl;
    break;
  case 0xC0:
//    std::cout << "MIDI Program change   C: " << chan << " P: " << b1 << std::endl;
    tsf_channel_set_presetnumber(synth, chan, b1, (chan == 9));
    break;
  case 0xD0:
//    std::cout << "MIDI Channel presure   C: " << chan << " P: " << b1 << std::endl;
    break;
  case 0xE0:
    tsf_channel_set_pitchwheel(synth, chan, b);
//    std::cout << "MIDI Pitch Bend   C: " << chan << " P: " << b << std::endl;
    break;
  default:
    std::cout << "MIDI msg   C: " << chan << " B1: " << b1 << " B2: " << b2 << std::endl;
    break;

  }
}

int audioCallback( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                   double streamTime, RtAudioStreamStatus status, void *userData )
{
  tsf *synth = (tsf *)userData;

  //std::cout << "AUDIO CALLBACK "<<nBufferFrames<<std::endl;
  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  int16_t *buf = (int16_t *)outputBuffer;
  tsf_render_short(synth, buf, nBufferFrames, 0);

  return 0;
}

#include <sys/mman.h>

int main(int argc, char** argv)
{

  tsf* synth;
  if (argc != 2) {
    printf("Usage: synth file.sf2\n");

  } else {
    synth = tsf_load_filename(argv[1]);
  }
  
  tsf_set_output(synth, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);


  RtMidiIn *midiIn = new RtMidiIn();
  if (midiIn->getPortCount() == 0) {
    std::cout << "No MIDI ports available!\n";
    exit(0);
  }
  midiIn->openPort(0);
  midiIn->setCallback( &midiCallback, (void *)synth);
  midiIn->ignoreTypes( false, false, false );

//   RtAudio dac(RtAudio::LINUX_PULSE);
  RtAudio dac;
  RtAudio::StreamParameters rtParams;

  // Determine the number of devices available
  unsigned int devices = dac.getDeviceCount();
  // Scan through devices for various capabilities

  if (devices == 0) {
    std::cout << "No audio device found\n";
    exit(0);
  }
  RtAudio::DeviceInfo info;
  for ( unsigned int i = 0; i < devices; i++ ) {
    info = dac.getDeviceInfo( i );
    if ( info.probed == true ) {
      std::cout << "device " << " = " << info.name;
      std::cout << ": maximum output channels = " << info.outputChannels;
      if (info.isDefaultOutput) {
        std::cout << " (default)";
      }

      std::cout << "\n";
    }
  }

//  printf("Default: %d\n",dac.getDefaultOutputDevice());
//  rtParams.deviceId = 3;
//  rtParams.deviceId = dac.getDefaultOutputDevice();
  rtParams.nChannels = 2;
  unsigned int bufferFrames = FRAME_SIZE;

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_ALSA_USE_DEFAULT;

  dac.openStream( &rtParams, NULL, AUDIO_FORMAT, SAMPLE_RATE, &bufferFrames, &audioCallback, (void *)synth, &options );
  dac.startStream();

  printf("\n\nPress Enter to stop\n\n");
  char str[256];
  cin.get(str, 256);
  dac.stopStream();

  tsf_close(synth);
}