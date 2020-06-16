#include "efluidsynth.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <sndfile.h>

#define	BUFFER_LEN 1024
#define SAMPLE_RATE 44100

int main(int argc, char** argv)
{
	double msec = 0;

	if(argc < 3) {
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

	fluid_synth_t* synth;
	fluid_settings_t settings;

	fluid_synth_settings(&settings);
	settings.sample_rate = SAMPLE_RATE;
//    settings.verbose = 1;
	settings.gain = 1.0f;
	settings.reverb = 0;
	settings.chorus = 0;
	settings.polyphony = 64;

	/* Create the synthesizer. */
	synth = new_fluid_synth(&settings);

//    fluid_synth_set_interp_method(synth, -1, FLUID_INTERP_LINEAR);
	fluid_synth_set_interp_method(synth, -1, FLUID_INTERP_NONE);

	fluid_synth_sfload(synth, argv[2], 1);


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

	while (tml && tml->next) {
		for (msec += (float)BUFFER_LEN/2.0 * (1000.0 / SAMPLE_RATE); tml && msec >= tml->time; tml = tml->next)
		{
			//printf("READ MID %d %f \n", tml->type,msec);
			switch (tml->type)
			{
			case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
				fluid_synth_program_change(synth, tml->channel, tml->program);
				break;
			case TML_NOTE_ON: //play a note
				fluid_synth_noteon(synth, tml->channel, tml->key, tml->velocity);
				break;
			case TML_NOTE_OFF: //stop a note
				fluid_synth_noteoff(synth, tml->channel, tml->key);
				break;
			case TML_PITCH_BEND: //pitch wheel modification
				fluid_synth_pitch_bend(synth, tml->channel, tml->pitch_bend);
				break;
			case TML_CHANNEL_PRESSURE: //pitch wheel modification
				fluid_synth_channel_pressure(synth, tml->channel, tml->velocity);
				break;
			case TML_CONTROL_CHANGE: //MIDI controller messages
				fluid_synth_cc(synth, tml->channel, tml->control, tml->control_value);
				break;
			}
		}

		short buf[BUFFER_LEN];
		fluid_synth_write_s16(synth, BUFFER_LEN / 2, buf, 0, 2, buf, 1, 2);
		int n = sf_write_short(outfile, buf, BUFFER_LEN);
		//printf("WRITE SND %d\n",n);

	}
	sf_close(outfile);

}