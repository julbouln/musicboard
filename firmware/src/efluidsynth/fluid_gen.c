#include "fluid_mod.h"
#include "fluid_gen.h"
#include "fluid_chan.h"

/* See SFSpec21 $8.1.3 */
fluid_gen_info_t fluid_gen_info[] = {
    /* number/name             init  scale         min        max         def */
    { GEN_STARTADDROFS,           1,     1,       0.0f,     1e10f,       0.0f },
    { GEN_ENDADDROFS,             1,     1,     -1e10f,      0.0f,       0.0f },
    { GEN_STARTLOOPADDROFS,       1,     1,     -1e10f,     1e10f,       0.0f },
    { GEN_ENDLOOPADDROFS,         1,     1,     -1e10f,     1e10f,       0.0f },
    { GEN_STARTADDRCOARSEOFS,     0,     1,       0.0f,     1e10f,       0.0f },
    { GEN_MODLFOTOPITCH,          1,     2,  -12000.0f,  12000.0f,       0.0f },
    { GEN_VIBLFOTOPITCH,          1,     2,  -12000.0f,  12000.0f,       0.0f },
    { GEN_MODENVTOPITCH,          1,     2,  -12000.0f,  12000.0f,       0.0f },
    { GEN_FILTERFC,               1,     2,    1500.0f,  13500.0f,   13500.0f },
    { GEN_FILTERQ,                1,     1,       0.0f,    960.0f,       0.0f },
    { GEN_MODLFOTOFILTERFC,       1,     2,  -12000.0f,  12000.0f,       0.0f },
    { GEN_MODENVTOFILTERFC,       1,     2,  -12000.0f,  12000.0f,       0.0f },
    { GEN_ENDADDRCOARSEOFS,       0,     1,     -1e10f,      0.0f,       0.0f },
    { GEN_MODLFOTOVOL,            1,     1,    -960.0f,    960.0f,       0.0f },
    { GEN_UNUSED1,                0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_CHORUSSEND,             1,     1,       0.0f,   1000.0f,       0.0f },
    { GEN_REVERBSEND,             1,     1,       0.0f,   1000.0f,       0.0f },
    { GEN_PAN,                    1,     1,    -500.0f,    500.0f,       0.0f },
    { GEN_UNUSED2,                0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_UNUSED3,                0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_UNUSED4,                0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_MODLFODELAY,            1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_MODLFOFREQ,             1,     4,  -16000.0f,   4500.0f,       0.0f },
    { GEN_VIBLFODELAY,            1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_VIBLFOFREQ,             1,     4,  -16000.0f,   4500.0f,       0.0f },
    { GEN_MODENVDELAY,            1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_MODENVATTACK,           1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_MODENVHOLD,             1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_MODENVDECAY,            1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_MODENVSUSTAIN,          0,     1,       0.0f,   1000.0f,       0.0f },
    { GEN_MODENVRELEASE,          1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_KEYTOMODENVHOLD,        0,     1,   -1200.0f,   1200.0f,       0.0f },
    { GEN_KEYTOMODENVDECAY,       0,     1,   -1200.0f,   1200.0f,       0.0f },
    { GEN_VOLENVDELAY,            1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_VOLENVATTACK,           1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_VOLENVHOLD,             1,     2,  -12000.0f,   5000.0f,  -12000.0f },
    { GEN_VOLENVDECAY,            1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_VOLENVSUSTAIN,          0,     1,       0.0f,   1440.0f,       0.0f },
    { GEN_VOLENVRELEASE,          1,     2,  -12000.0f,   8000.0f,  -12000.0f },
    { GEN_KEYTOVOLENVHOLD,        0,     1,   -1200.0f,   1200.0f,       0.0f },
    { GEN_KEYTOVOLENVDECAY,       0,     1,   -1200.0f,   1200.0f,       0.0f },
    { GEN_INSTRUMENT,             0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_RESERVED1,              0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_KEYRANGE,               0,     0,       0.0f,    127.0f,       0.0f },
    { GEN_VELRANGE,               0,     0,       0.0f,    127.0f,       0.0f },
    { GEN_STARTLOOPADDRCOARSEOFS, 0,     1,     -1e10f,     1e10f,       0.0f },
    { GEN_KEYNUM,                 1,     0,       0.0f,    127.0f,      -1.0f },
    { GEN_VELOCITY,               1,     1,       0.0f,    127.0f,      -1.0f },
    { GEN_ATTENUATION,            1,     1,       0.0f,   1440.0f,       0.0f },
    { GEN_RESERVED2,              0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_ENDLOOPADDRCOARSEOFS,   0,     1,     -1e10f,     1e10f,       0.0f },
    { GEN_COARSETUNE,             0,     1,    -120.0f,    120.0f,       0.0f },
    { GEN_FINETUNE,               0,     1,     -99.0f,     99.0f,       0.0f },
    { GEN_SAMPLEID,               0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_SAMPLEMODE,             0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_RESERVED3,              0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_SCALETUNE,              0,     1,       0.0f,   1200.0f,     100.0f },
    { GEN_EXCLUSIVECLASS,         0,     0,       0.0f,      0.0f,       0.0f },
    { GEN_OVERRIDEROOTKEY,        1,     0,       0.0f,    127.0f,      -1.0f },
    { GEN_PITCH,                  1,     0,       0.0f,    127.0f,       0.0f }
};

fluid_gen_t * fluid_gen_create(uint8_t num) {
    fluid_gen_t *gen = FLUID_NEW(fluid_gen_t);
    gen->num = num;
    gen->flags = GEN_UNUSED;
    gen->mod = 0.0;
#ifndef FLUID_NO_NRPN_EXT
    gen->nrpn = 0.0;
#endif
    gen->val = fluid_gen_info[num].def;
    return gen;
}

fluid_gen_t *fluid_gen_get(fluid_list_t *gen_list, uint8_t num) {
    fluid_gen_t *gen;
    fluid_list_t *p = gen_list;
    while (p != NULL) {
        gen = (fluid_gen_t *) p->data;
        if (gen->num == num)
            return gen;

        p = fluid_list_next(p);
    }
    return NULL;
}


fluid_real_t fluid_gen_get_default_value(uint8_t num) {
    return fluid_gen_info[num].def;
}

void fluid_gen_delete(fluid_gen_t *gen) {
    FLUID_FREE(gen);
}

/**
 * Set an array of generators to their default values.
 * @param gen Array of generators (should be #GEN_LAST in size).
 * @return Always returns 0
 */
int fluid_gen_set_default_values(fluid_gen_t* gen)
{
    int i;

    for (i = 0; i < GEN_LAST; i++) {
        gen[i].flags = GEN_UNUSED;
        gen[i].mod = 0.0;
#ifndef FLUID_NO_NRPN_EXT
        gen[i].nrpn = 0.0;
#endif
        gen[i].val = fluid_gen_info[i].def;
    }

    return FLUID_OK;
}

/* fluid_gen_init
 *
 * Set an array of generators to their initial value
 */
int fluid_gen_init(fluid_gen_t* gen, fluid_channel_t* channel)
{
    int i;

    fluid_gen_set_default_values(gen);

#ifndef FLUID_NO_NRPN_EXT
    for (i = 0; i < GEN_LAST; i++) {
        gen[i].nrpn = fluid_channel_get_gen(channel, i);

        /* This is an extension to the SoundFont standard. More
         * documentation is available at the fluid_synth_set_gen2()
         * function. */
        if (fluid_channel_get_gen_abs(channel, i)) {
            gen[i].flags = GEN_ABS_NRPN;
        }
    }
#endif

    return FLUID_OK;
}

fluid_real_t fluid_gen_scale(int gen, fluid_real_t value)
{
    return (fluid_gen_info[gen].min
            + value * (fluid_gen_info[gen].max - fluid_gen_info[gen].min));
}

#ifndef FLUID_NO_NRPN_EXT
fluid_real_t fluid_gen_scale_nrpn(int gen, int data)
{
    fluid_real_t value = (float) data - 8192.0f;
    fluid_clip(value, -8192, 8192);
    return value * (float) fluid_gen_info[gen].nrpn_scale;
}
#endif