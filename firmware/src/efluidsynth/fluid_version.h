#ifndef _FLUID_VERSION_H
#define _FLUID_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define FLUIDSYNTH_VERSION       "1.0.9"
#define FLUIDSYNTH_VERSION_MAJOR 1
#define FLUIDSYNTH_VERSION_MINOR 0
#define FLUIDSYNTH_VERSION_MICRO 9

void fluid_version(int *major, int *minor, int *micro);
char* fluid_version_str(void);

#ifdef __cplusplus
}
#endif

#endif /* _FLUIDSYNTH_VERSION_H */
