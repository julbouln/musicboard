CFLAGS=-O3 -march=native -g -I../firmware/src -I. -Wall #-fno-inline

mid2wav_tsf:
	gcc $(CFLAGS) mid2wav_tsf.c -o mid2wav_tsf $^ -lc -lm -lsndfile

mid2wav_efluidsynth:
	gcc $(CFLAGS) -DFLUID_NO_NAMES -DFLUID_SAMPLE_MMAP -DFLUID_NO_NRPN_EXT -DFLUID_CALC_FORMAT_FLOAT -DFLUID_BUFFER_S16 -DFLUID_FIXED_POINT \
	-g -I../firmware/src/efluidsynth \
	../firmware/src/efluidsynth/fluid_altsfont.c ../firmware/src/efluidsynth/fluid_conv.c ../firmware/src/efluidsynth/fluid_mod.c \
	../firmware/src/efluidsynth/fluid_sys.c ../firmware/src/efluidsynth/riff.c ../firmware/src/efluidsynth/fluid_chan.c \
	../firmware/src/efluidsynth/fluid_gen.c ../firmware/src/efluidsynth/fluid_rev.c ../firmware/src/efluidsynth/fluid_tuning.c \
	../firmware/src/efluidsynth/fluid_chorus.c ../firmware/src/efluidsynth/fluid_list.c ../firmware/src/efluidsynth/fluid_synth.c \
	../firmware/src/efluidsynth/fluid_voice.c \
	mid2wav_efluidsynth.c -o mid2wav_efluidsynth $^ -lc -lm -lsndfile

clean:
	rm -f mid2wav_tsf
	rm -f mid2wav_efluidsynth