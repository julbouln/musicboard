#ifndef _FLUID_ALTSFONT_H
#define _FLUID_ALTSFONT_H

#include <stdint.h>

#include "riff.h"

#include "fluid_mod.h"
#include "fluid_types.h"
#include "fluid_sfont.h"
#include "fluid_list.h"

#define CID(a, b, c, d)    (((d)<<24)+((c)<<16)+((b)<<8)+((a)))
#define CID_RIFF  CID('R','I','F','F')
#define CID_LIST  CID('L','I','S','T')
#define CID_INFO  CID('I','N','F','O')
#define CID_sdta  CID('s','d','t','a')
#define CID_snam  CID('s','n','a','m')
#define CID_smpl  CID('s','m','p','l')
#define CID_pdta  CID('p','d','t','a')
#define CID_phdr  CID('p','h','d','r')
#define CID_pbag  CID('p','b','a','g')
#define CID_pmod  CID('p','m','o','d')
#define CID_pgen  CID('p','g','e','n')
#define CID_inst  CID('i','n','s','t')
#define CID_ibag  CID('i','b','a','g')
#define CID_imod  CID('i','m','o','d')
#define CID_igen  CID('i','g','e','n')
#define CID_shdr  CID('s','h','d','r')
#define CID_ifil  CID('i','f','i','l')
#define CID_isng  CID('i','s','n','g')
#define CID_irom  CID('i','r','o','m')
#define CID_iver  CID('i','v','e','r')
#define CID_INAM  CID('I','N','A','M')
#define CID_IPRD  CID('I','P','R','D')
#define CID_ICOP  CID('I','C','O','P')
#define CID_sfbk  CID('s','f','b','k')
#define CID_ICRD  CID('I','C','R','D')
#define CID_IENG  CID('I','E','N','G')
#define CID_ICMT  CID('I','C','M','T')
#define CID_ISFT  CID('I','S','F','T')

/* SoundFont generator types */
#define SFGEN_startAddrsOffset         0
#define SFGEN_endAddrsOffset           1
#define SFGEN_startloopAddrsOffset     2
#define SFGEN_endloopAddrsOffset       3
#define SFGEN_startAddrsCoarseOffset   4
#define SFGEN_modLfoToPitch            5
#define SFGEN_vibLfoToPitch            6
#define SFGEN_modEnvToPitch            7
#define SFGEN_initialFilterFc          8
#define SFGEN_initialFilterQ           9
#define SFGEN_modLfoToFilterFc         10
#define SFGEN_modEnvToFilterFc         11
#define SFGEN_endAddrsCoarseOffset     12
#define SFGEN_modLfoToVolume           13
#define SFGEN_unused1                  14
#define SFGEN_chorusEffectsSend        15
#define SFGEN_reverbEffectsSend        16
#define SFGEN_pan                      17
#define SFGEN_unused2                  18
#define SFGEN_unused3                  19
#define SFGEN_unused4                  20
#define SFGEN_delayModLFO              21
#define SFGEN_freqModLFO               22
#define SFGEN_delayVibLFO              23
#define SFGEN_freqVibLFO               24
#define SFGEN_delayModEnv              25
#define SFGEN_attackModEnv             26
#define SFGEN_holdModEnv               27
#define SFGEN_decayModEnv              28
#define SFGEN_sustainModEnv            29
#define SFGEN_releaseModEnv            30
#define SFGEN_keynumToModEnvHold       31
#define SFGEN_keynumToModEnvDecay      32
#define SFGEN_delayVolEnv              33
#define SFGEN_attackVolEnv             34
#define SFGEN_holdVolEnv               35
#define SFGEN_decayVolEnv              36
#define SFGEN_sustainVolEnv            37
#define SFGEN_releaseVolEnv            38
#define SFGEN_keynumToVolEnvHold       39
#define SFGEN_keynumToVolEnvDecay      40
#define SFGEN_instrument               41
#define SFGEN_reserved1                42
#define SFGEN_keyRange                 43
#define SFGEN_velRange                 44
#define SFGEN_startloopAddrsCoarse     45
#define SFGEN_keynum                   46
#define SFGEN_velocity                 47
#define SFGEN_initialAttenuation       48
#define SFGEN_reserved2                49
#define SFGEN_endloopAddrsCoarse       50
#define SFGEN_coarseTune               51
#define SFGEN_fineTune                 52
#define SFGEN_sampleID                 53
#define SFGEN_sampleModes              54
#define SFGEN_reserved3                55
#define SFGEN_scaleTuning              56
#define SFGEN_exclusiveClass           57
#define SFGEN_overridingRootKey        58
#define SFGEN_unused5                  59
#define SFGEN_endOper                  60

typedef struct sfVersionTag {
  uint16_t wMajor;
  uint16_t wMinor;
} sfVersionTag;

typedef struct sfPresetHeader {
  char achPresetName[20]; 
  uint16_t wPreset;
  uint16_t wBank;
  uint16_t wPresetBagNdx; 
  uint32_t dwLibrary; 
  uint32_t dwGenre; 
  uint32_t dwMorphology;
} sfPresetHeader;

typedef struct sfPresetBag {
  uint16_t wGenNdx;
  uint16_t wModNdx; 
} sfPresetBag;


typedef struct {
  int8_t byLo;
  int8_t byHi; 
} rangesType;

typedef union {
  rangesType ranges; 
  int16_t shAmount; 
  uint16_t wAmount;
} genAmountType;

typedef  uint16_t SFGenerator;

typedef struct sfGenList {
  SFGenerator sfGenOper;
  genAmountType genAmount; 
} sfGenList;

typedef uint16_t SFModulator;
typedef uint16_t SFTransform;

typedef struct sfModList
{
SFModulator sfModSrcOper;
uint16_t sfModDestOper;
int16_t modAmount;
SFModulator sfModAmtSrcOper;
SFTransform sfModTransOper;
} sfModList;

typedef struct sfInstGenList {
  SFGenerator sfGenOper;
  genAmountType genAmount; 
} sfInstGenList;

typedef struct sfInst {
  char achInstName[20];
  uint16_t wInstBagNdx; 
} sfInst;

typedef struct sfInstBag {
  uint16_t wInstGenNdx;
  uint16_t wInstModNdx; 
} sfInstBag;

typedef enum {
  monoSample = 1, 
  rightSample = 2,
  leftSample = 4, 
  linkedSample = 8, 
  RomMonoSample = 0x8001, 
  RomRightSample = 0x8002, 
  RomLeftSample = 0x8004, 
  RomLinkedSample = 0x8008 
} SFSampleLink;

typedef struct sfSample {
  char achSampleName[20]; 
  uint32_t dwStart;
  uint32_t dwEnd;
  uint32_t dwStartloop; 
  uint32_t dwEndloop; 
  uint32_t dwSampleRate; 
  uint8_t byOriginalPitch; 
  char chPitchCorrection; 
  uint16_t wSampleLink; 
  SFSampleLink sfSampleType;
} sfSample;

#define inst_size 22
#define ibag_size 4
#define igen_size 4
#define imod_size 10
#define shdr_size 46
#define phdr_size 38
#define pbag_size 4
#define pgen_size 4
#define pmod_size 10

#include "fluid_gen.h"

/* basic struct */
typedef struct sf2 {
  riff_handle *rh;

  uint32_t ifil_pos;
  uint32_t INAM_pos;
  uint32_t isng_pos;
  uint32_t IENG_pos;
  uint32_t ISFT_pos;
  uint32_t ICMT_pos;
  uint32_t ICOP_pos;

  uint32_t smpl_pos;

  uint32_t phdr_pos;
  uint32_t pbag_pos;
  uint32_t pmod_pos;
  uint32_t pgen_pos;
  uint32_t inst_pos;
  uint32_t ibag_pos;
  uint32_t imod_pos;
  uint32_t igen_pos;
  uint32_t shdr_pos;

  fluid_list_t *insts;
  fluid_list_t *banks;

  fluid_sampledata* sampledata;        /* the sample data, loaded in ram */
  uint32_t samplepos;   /* the position in the file at which the sample data starts */
  uint32_t samplesize;  /* the size of the sample data */

  char *filename;
} sf2;

typedef struct sf2_bank {
  uint16_t num;

  fluid_list_t *presets;
} sf2_bank;

typedef struct sf2_inst_zone {
  int keylo;
  int keyhi;
  int vello;
  int velhi;

  fluid_sample_t* sample;
  fluid_list_t *gen;

  fluid_mod_t *mod;

} sf2_inst_zone;

typedef struct sf2_inst {
  uint16_t id;
  uint32_t bags_pos;
  uint16_t bags_size;

  fluid_list_t *inst_zones;
  sf2_inst_zone *global_inst_zone;

  uint8_t parsed;
  uint32_t refcount;
} sf2_inst;

typedef struct sf2_preset_zone {

  int keylo;
  int keyhi;
  int vello;
  int velhi;

  sf2_inst *inst;
  fluid_list_t *gen;

  fluid_mod_t *mod;

} sf2_preset_zone;

typedef struct sf2_preset {
  uint16_t num;
  uint32_t bags_pos;
  uint16_t bags_size;

  sf2_preset_zone *global_preset_zone;
  fluid_list_t *preset_zones;
  sf2_bank *bank;

  uint8_t parsed;
} sf2_preset;


fluid_sfloader_t* new_fluid_altsfloader();

#endif
