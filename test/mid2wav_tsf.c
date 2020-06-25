#define TSF_RENDER_EFFECTSAMPLEBLOCK 64
#define TSF_NO_PRESET_NAME
//#define TSF_NO_INTERPOLATION
//#define TSF_NO_LOWPASS
//#define TSF_NO_REVERB
//#define TSF_NO_CHORUS

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <sndfile.h>

#define	BUFFER_LEN 1024
#define SAMPLE_RATE 44100

#include "chorus.h"

int main(int argc, char** argv)
{
	double msec = 0;

	if (argc < 3) {
		printf("Usage:\n mid2wav file.mid file.sf2 file.wav\n");
		return 1;
	}

	tml_message* tml = NULL;
	tml = tml_load_filename(argv[1]);
	if (!tml)
	{
		fprintf(stderr, "Could not load MIDI file\n");
		return 1;
	}

	tsf* synth = tsf_load_filename(argv[2]);
	if (!synth) {
		fprintf(stderr, "Could not create synth\n");
		return 1;
	}
	tsf_set_max_voices(synth, 64);
	tsf_set_output(synth, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
	#ifndef TSF_NO_REVERB
	//printf("sizeof(reverb_t) = %ld\n",sizeof(reverb_t));
	tsf_reverb_setup(synth, 0.0f, 0.7f, 0.7f);
	#endif

	SNDFILE	*outfile;
	SF_INFO		sfinfo;
	memset (&sfinfo, 0, sizeof (sfinfo)) ;
	sfinfo.samplerate = SAMPLE_RATE;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	sfinfo.channels = 2;
	if (! (outfile = sf_open (argv[3], SFM_WRITE, &sfinfo)))
	{	printf ("Not able to open output file.\n") ;
		puts (sf_strerror (NULL)) ;
		return 1 ;
	} ;

	short buf[BUFFER_LEN];

	while (tml && tml->next) {
		for (msec += (float)BUFFER_LEN / 2.0 * (1000.0 / SAMPLE_RATE); tml && msec >= tml->time; tml = tml->next)
		{
			//printf("READ MID %d %f \n", tml->type,msec);
			switch (tml->type)
			{
			case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
				tsf_channel_set_presetnumber(synth, tml->channel, tml->program, (tml->channel == 9));
				break;
			case TML_NOTE_ON: //play a note
				tsf_channel_note_on(synth, tml->channel, tml->key, tml->velocity / 127.0f);
				break;
			case TML_NOTE_OFF: //stop a note
				tsf_channel_note_off(synth, tml->channel, tml->key);
				break;
			case TML_PITCH_BEND: //pitch wheel modification
				tsf_channel_set_pitchwheel(synth, tml->channel, tml->pitch_bend);
				break;
			case TML_CONTROL_CHANGE: //MIDI controller messages
				tsf_channel_midi_control(synth, tml->channel, tml->control, tml->control_value);
				break;
			default:
				//printf("UNSUPPORTED TML TYPE %d\n", tml->type);
				break;
			}
		}

		tsf_render_short(synth, buf, BUFFER_LEN / 2, 0);

		int n = sf_write_short(outfile, buf, BUFFER_LEN);
	}
	sf_close(outfile);

	// printf("voices %d\n",tsf_active_voice_count(synth));

	tsf_close(synth);

}