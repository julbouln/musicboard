#include "fluid_synth.h"
#include "fluid_altsfont.h"
#include "riff.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

sf2_bank *sf2_bank_get(fluid_list_t *banks, uint16_t num) {
	sf2_bank *bank;
	fluid_list_t *p = banks;
	while (p != NULL) {
		bank = (sf2_bank *) p->data;
		if (bank->num == num)
			return bank;

		p = fluid_list_next(p);
	}
	return NULL;
}

sf2_preset *sf2_preset_get(fluid_list_t *presets, uint16_t num) {
	sf2_preset *preset;
	fluid_list_t *p = presets;
	while (p != NULL) {
		preset = (sf2_preset *) p->data;
		if (preset->num == num)
			return preset;

		p = fluid_list_next(p);
	}
	return NULL;
}

sf2_inst *sf2_inst_get(sf2 *sf, uint16_t id) {
	sf2_inst *inst;
	fluid_list_t *p = sf->insts;
	while (p != NULL) {
		inst = (sf2_inst *) p->data;
		if (inst->id == id)
			return inst;

		p = fluid_list_next(p);
	}
	return NULL;
}

sf2_preset *sf2_bank_preset_get(sf2 *sf, uint16_t bank_num, uint16_t preset_num) {
	sf2_bank *bank = sf2_bank_get(sf->banks, bank_num);
	if (bank) {
		return sf2_preset_get(bank->presets, preset_num);
	} else {
		return NULL;
	}
}

riff_handle *sf2_open(const char *filename) {
	fluid_file f = FLUID_FOPEN(filename, "rb");
	//get size
	FLUID_FSEEK(f, 0, SEEK_END);
	int fsize = FLUID_FTELL(f);
	FLUID_FSEEK(f, 0, SEEK_SET);

	//allocate initialized handle struct
	riff_handle *rh = riff_handleAllocate();

	//after allocation rh->fp_fprintf == fprintf
	//you can change the rh->fp_fprintf function pointer here for error output
	rh->fp_printf = NULL;  //set to NULL to disable any error printing

	//open file, use build in input wrappers for file
	//open RIFF file via file handle -> reads RIFF header and first chunk header
	if (riff_open_file(rh, f, fsize) != RIFF_ERROR_NONE) {
		return NULL;
	}

	return rh;
}

void sf2_init_rec(sf2 *sf) {
	riff_handle *rh = sf->rh;
	int err;

	int k = 0;

	while (1) {
		//if current chunk not a chunk list
		if (strcmp(rh->c_id, "LIST") != 0  &&  strcmp(rh->c_id, "RIFF") != 0) {
			switch (CID(rh->c_id[0], rh->c_id[1], rh->c_id[2], rh->c_id[3])) {
			case CID_ifil:
				sf->ifil_pos = rh->c_pos_start;
				break;
			case CID_INAM:
				sf->INAM_pos = rh->c_pos_start;
				break;
			// ...
			case CID_smpl:
				sf->smpl_pos = rh->c_pos_start;
				break;
			case CID_phdr:
				sf->phdr_pos = rh->c_pos_start;
				break;
			case CID_pbag:
				sf->pbag_pos = rh->c_pos_start;
				break;
			case CID_pmod:
				sf->pmod_pos = rh->c_pos_start;
				break;
			case CID_pgen:
				sf->pgen_pos = rh->c_pos_start;
				break;
			case CID_inst:
				sf->inst_pos = rh->c_pos_start;
				break;
			case CID_ibag:
				sf->ibag_pos = rh->c_pos_start;
				break;
			case CID_imod:
				sf->imod_pos = rh->c_pos_start;
				break;
			case CID_igen:
				sf->igen_pos = rh->c_pos_start;
				break;
			case CID_shdr:
				sf->shdr_pos = rh->c_pos_start;
				break;
			default:
				break;
			}
		}
		else {
			err = riff_seekLevelSub(rh);
			if (err) {
			}
			sf2_init_rec(sf); //recursive call
		}
		k++;

		err = riff_seekNextChunk(rh);
		if (err >= RIFF_ERROR_CRITICAL) {
			printf("%s", riff_errorToString(err));
			break;
		}
		else if (err < RIFF_ERROR_CRITICAL  &&  err != RIFF_ERROR_NONE) {
			//go back from sub level
			riff_levelParent(rh);
			//file pos has not changed by going a level back, we are now within that parent's data
			break;
		}
	}
}

sf2 *sf2_load(const char *filename) {
	riff_handle *rh = sf2_open(filename);
	sf2 *sf = FLUID_NEW(sf2);
	sf->rh = rh;
	sf->banks = NULL;
	sf->insts = NULL;

	sf2_init_rec(sf);

	return sf;
}

void sf2_load_presets(sf2 *sf) {
	riff_handle *rh = sf->rh;
	size_t pos;
	riff_seek(rh, sf->phdr_pos);

	// init presets
	for (pos = 0; pos < rh->c_size - phdr_size; pos += phdr_size) {
		sfPresetHeader phdr;
		riff_seekInChunk(rh, pos);
		riff_readInChunk(rh, &phdr, phdr_size);

		sfPresetHeader n_phdr;
		riff_seekInChunk(rh, pos + phdr_size);
		riff_readInChunk(rh, &n_phdr, phdr_size);

		sf2_preset *preset;
		preset = FLUID_NEW(sf2_preset);
		preset->num = phdr.wPreset;
		preset->bags_pos = phdr.wPresetBagNdx * pbag_size;
		preset->bags_size = n_phdr.wPresetBagNdx * pbag_size - phdr.wPresetBagNdx * pbag_size;

		sf2_bank *bank = sf2_bank_get(sf->banks, phdr.wBank);
		if (!bank) {
			bank = FLUID_NEW(sf2_bank);
			bank->num = phdr.wBank;
			bank->presets = NULL;

			sf->banks = fluid_list_append(sf->banks, bank);
		}

		preset->bank = bank;
		preset->preset_zones = NULL;
		preset->global_preset_zone = NULL;
		preset->parsed = 0;
		bank->presets = fluid_list_append(bank->presets, preset);
	}
}

void sf2_parse_sample(sf2 *sf, sf2_inst_zone *isz, uint16_t id) {
	fluid_sample_t* sample;

	riff_handle *rh = sf->rh;
	size_t pos = id * shdr_size;
	riff_seek(rh, sf->shdr_pos);

	sfSample shdr;
	riff_seekInChunk(rh, pos);
	riff_readInChunk(rh, &shdr, shdr_size);

	sample = FLUID_NEW(fluid_sample_t);

	sample->data = sf->sampledata;

	sample->userdata = sf;

	sample->start = shdr.dwStart;
	sample->end = shdr.dwEnd;
	sample->loopstart = shdr.dwStartloop;
	sample->loopend = shdr.dwEndloop;
	sample->samplerate = shdr.dwSampleRate;
	sample->origpitch = shdr.byOriginalPitch;
	sample->pitchadj = shdr.chPitchCorrection;
	sample->sampletype = shdr.sfSampleType;

	isz->sample = sample;
}

void sf2_load_inst(sf2 *sf, uint16_t id, sf2_inst *is) {
	riff_handle *rh = sf->rh;
	size_t pos = id * inst_size;
	riff_seek(rh, sf->inst_pos);

	sfInst inst;
	riff_seekInChunk(rh, pos);
	riff_readInChunk(rh, &inst, inst_size);

	sfInst n_inst;
	riff_seekInChunk(rh, pos + inst_size);
	riff_readInChunk(rh, &n_inst, inst_size);
	is->id = id;
	is->bags_pos = inst.wInstBagNdx * ibag_size;
	is->bags_size = n_inst.wInstBagNdx * ibag_size - inst.wInstBagNdx * ibag_size;
	is->inst_zones = NULL;
	is->global_inst_zone = NULL;
	is->parsed = 0;
	sf->insts = fluid_list_append(sf->insts, is);
}

void sf2_parse_inst(sf2 *sf, sf2_inst *is) {
	riff_handle *rh = sf->rh;
	size_t pos;

	for (pos = is->bags_pos; pos < is->bags_pos + is->bags_size; pos += ibag_size) {
		sfInstBag ibag;
		riff_seek(rh, sf->ibag_pos);
		riff_seekInChunk(rh, pos);
		riff_readInChunk(rh, &ibag, ibag_size);
		sfInstBag n_ibag;
		riff_seekInChunk(rh, pos + ibag_size);
		riff_readInChunk(rh, &n_ibag, ibag_size);
		uint16_t igen_count = n_ibag.wInstGenNdx * igen_size - ibag.wInstGenNdx * igen_size;
		uint16_t imod_count = n_ibag.wInstModNdx * imod_size - ibag.wInstModNdx * imod_size;

		sfInstGenList igen;
		riff_seek(rh, sf->igen_pos);
		uint16_t curGen = 0;

		uint8_t global = 1;
		sf2_inst_zone *isz;
		isz = FLUID_NEW(sf2_inst_zone);
		isz->keylo = 0;
		isz->keyhi = 128;
		isz->vello = 0;
		isz->velhi = 128;

		isz->sample = NULL;
		isz->gen = NULL;
		isz->mod = NULL;

		for (curGen = 0; curGen < igen_count; curGen += igen_size) {
			riff_seekInChunk(rh, ibag.wInstGenNdx * igen_size + curGen);
			riff_readInChunk(rh, &igen, igen_size);
			switch (igen.sfGenOper) {
			case SFGEN_sampleID:
				global = 0;
				if (!isz->sample)
					sf2_parse_sample(sf, isz, igen.genAmount.wAmount);
				break;
			case SFGEN_keyRange:
				isz->keylo = igen.genAmount.ranges.byLo;
				isz->keyhi = igen.genAmount.ranges.byHi;
				break;
			case SFGEN_velRange:
				isz->vello = igen.genAmount.ranges.byLo;
				isz->velhi = igen.genAmount.ranges.byHi;
				break;
			default:
				if (fluid_gen_get_default_value(igen.sfGenOper) != (fluid_real_t) igen.genAmount.shAmount) {
					fluid_gen_t *gen = NULL;
					gen = fluid_gen_get(isz->gen, igen.sfGenOper);
					if (!gen) {
						gen = fluid_gen_create(igen.sfGenOper);
						isz->gen = fluid_list_append(isz->gen, gen);
					}

					gen->val = (fluid_real_t) igen.genAmount.shAmount;
					gen->flags = GEN_SET;
				}

				break;
			}
		}

		sfModList imod;
		riff_seek(rh, sf->imod_pos);
		uint16_t curMod = 0;

		for (curMod = 0; curMod < imod_count; curMod += imod_size) {
			riff_seekInChunk(rh, ibag.wInstModNdx * imod_size + curMod);
			riff_readInChunk(rh, &imod, imod_size);

			int type;
			fluid_mod_t* mod_dest;

			mod_dest = fluid_mod_new();
			if (mod_dest == NULL) {
				return;
			}

			mod_dest->next = NULL; /* pointer to next modulator, this is the end of the list now.*/

			/* *** Amount *** */
			mod_dest->amount = imod.modAmount;

			/* *** Source *** */
			mod_dest->src1 = imod.sfModSrcOper & 127; /* index of source 1, seven-bit value, SF2.01 section 8.2, page 50 */
			mod_dest->flags1 = 0;

			/* Bit 7: CC flag SF 2.01 section 8.2.1 page 50*/
			if (imod.sfModSrcOper & (1 << 7)) {
				mod_dest->flags1 |= FLUID_MOD_CC;
			} else {
				mod_dest->flags1 |= FLUID_MOD_GC;
			}

			/* Bit 8: D flag SF 2.01 section 8.2.2 page 51*/
			if (imod.sfModSrcOper & (1 << 8)) {
				mod_dest->flags1 |= FLUID_MOD_NEGATIVE;
			} else {
				mod_dest->flags1 |= FLUID_MOD_POSITIVE;
			}

			/* Bit 9: P flag SF 2.01 section 8.2.3 page 51*/
			if (imod.sfModSrcOper & (1 << 9)) {
				mod_dest->flags1 |= FLUID_MOD_BIPOLAR;
			} else {
				mod_dest->flags1 |= FLUID_MOD_UNIPOLAR;
			}

			/* modulator source types: SF2.01 section 8.2.1 page 52 */
			type = (imod.sfModSrcOper) >> 10;
			type &= 63; /* type is a 6-bit value */
			if (type == 0) {
				mod_dest->flags1 |= FLUID_MOD_LINEAR;
			} else if (type == 1) {
				mod_dest->flags1 |= FLUID_MOD_CONCAVE;
			} else if (type == 2) {
				mod_dest->flags1 |= FLUID_MOD_CONVEX;
			} else if (type == 3) {
				mod_dest->flags1 |= FLUID_MOD_SWITCH;
			} else {
				/* This shouldn't happen - unknown type!
				 * Deactivate the modulator by setting the amount to 0. */
				mod_dest->amount = 0;
			}

			/* *** Dest *** */
			mod_dest->dest = imod.sfModDestOper; /* index of controlled generator */

			/* *** Amount source *** */
			mod_dest->src2 = imod.sfModAmtSrcOper & 127; /* index of source 2, seven-bit value, SF2.01 section 8.2, page 50 */
			mod_dest->flags2 = 0;

			/* Bit 7: CC flag SF 2.01 section 8.2.1 page 50*/
			if (imod.sfModAmtSrcOper & (1 << 7)) {
				mod_dest->flags2 |= FLUID_MOD_CC;
			} else {
				mod_dest->flags2 |= FLUID_MOD_GC;
			}

			/* Bit 8: D flag SF 2.01 section 8.2.2 page 51*/
			if (imod.sfModAmtSrcOper & (1 << 8)) {
				mod_dest->flags2 |= FLUID_MOD_NEGATIVE;
			} else {
				mod_dest->flags2 |= FLUID_MOD_POSITIVE;
			}

			/* Bit 9: P flag SF 2.01 section 8.2.3 page 51*/
			if (imod.sfModAmtSrcOper & (1 << 9)) {
				mod_dest->flags2 |= FLUID_MOD_BIPOLAR;
			} else {
				mod_dest->flags2 |= FLUID_MOD_UNIPOLAR;
			}

			/* modulator source types: SF2.01 section 8.2.1 page 52 */
			type = (imod.sfModAmtSrcOper) >> 10;
			type &= 63; /* type is a 6-bit value */
			if (type == 0) {
				mod_dest->flags2 |= FLUID_MOD_LINEAR;
			} else if (type == 1) {
				mod_dest->flags2 |= FLUID_MOD_CONCAVE;
			} else if (type == 2) {
				mod_dest->flags2 |= FLUID_MOD_CONVEX;
			} else if (type == 3) {
				mod_dest->flags2 |= FLUID_MOD_SWITCH;
			} else {
				/* This shouldn't happen - unknown type!
				 * Deactivate the modulator by setting the amount to 0. */
				mod_dest->amount = 0;
			}

			/* *** Transform *** */
			/* SF2.01 only uses the 'linear' transform (0).
			 * Deactivate the modulator by setting the amount to 0 in any other case.
			 */
			if (imod.sfModTransOper != 0) {
				mod_dest->amount = 0;
			}

			/* Store the new modulator in the zone
			* The order of modulators will make a difference, at least in an instrument context:
			* The second modulator overwrites the first one, if they only differ in amount. */
			if (curMod == 0) {
				isz->mod = mod_dest;
			} else {
				fluid_mod_t * last_mod = isz->mod;
				/* Find the end of the list */
				while (last_mod->next != NULL) {
					last_mod = last_mod->next;
				}
				last_mod->next = mod_dest;
			}
		}

		if (global) {
			is->global_inst_zone = isz;
		} else {
			is->inst_zones = fluid_list_append(is->inst_zones, isz);
		}
	}
	is->parsed = 1;
}

void sf2_parse_preset(sf2 *sf, sf2_preset *ps) {
	riff_handle *rh = sf->rh;
	size_t pos;

	for (pos = ps->bags_pos; pos < ps->bags_pos + ps->bags_size; pos += pbag_size) {
		sfPresetBag pbag;
		riff_seek(rh, sf->pbag_pos);
		riff_seekInChunk(rh, pos);
		riff_readInChunk(rh, &pbag, pbag_size);
		sfPresetBag n_pbag;
		riff_seekInChunk(rh, pos + pbag_size);
		riff_readInChunk(rh, &n_pbag, pbag_size);
		uint16_t pgen_count = n_pbag.wGenNdx * pgen_size - pbag.wGenNdx * pgen_size;
		uint16_t pmod_count = n_pbag.wModNdx * pmod_size - pbag.wModNdx * pmod_size;
		sfGenList pgen;
		riff_seek(rh, sf->pgen_pos);
		uint16_t curGen = 0;

		uint8_t global = 1;

		sf2_preset_zone *psz;
		psz = FLUID_NEW(sf2_preset_zone);
		psz->inst = NULL;
		psz->keylo = 0;
		psz->keyhi = 128;
		psz->vello = 0;
		psz->velhi = 128;
		psz->gen = NULL;
		psz->mod = NULL;

		for (curGen = 0; curGen < pgen_count; curGen += pgen_size) {
			riff_seekInChunk(rh, pbag.wGenNdx * pgen_size + curGen);
			riff_readInChunk(rh, &pgen, pgen_size);
			switch (pgen.sfGenOper) {
			case SFGEN_instrument:
				global = 0;
				sf2_inst *inst = sf2_inst_get(sf, pgen.genAmount.wAmount);
				if (!inst) {
					inst = FLUID_NEW(sf2_inst);
					inst->refcount = 0;
					sf2_load_inst(sf, pgen.genAmount.wAmount, inst);
				}
				if (!inst->parsed)
					sf2_parse_inst(sf, inst);
				inst->refcount++;
				psz->inst = inst;

				break;
			case SFGEN_keyRange:
				psz->keylo = pgen.genAmount.ranges.byLo;
				psz->keyhi = pgen.genAmount.ranges.byHi;
				break;
			case SFGEN_velRange:
				psz->vello = pgen.genAmount.ranges.byLo;
				psz->velhi = pgen.genAmount.ranges.byHi;
				break;
			default:
				if (fluid_gen_get_default_value(pgen.sfGenOper) != (fluid_real_t) pgen.genAmount.shAmount) {
					fluid_gen_t *gen = NULL;
					gen = fluid_gen_get(psz->gen, pgen.sfGenOper);
					if (!gen) {
						gen = fluid_gen_create(pgen.sfGenOper);
						psz->gen = fluid_list_append(psz->gen, gen);
					}

					gen->val = (fluid_real_t) pgen.genAmount.shAmount;
					gen->flags = GEN_SET;
				}
				break;
			}
		}

		sfModList pmod;
		riff_seek(rh, sf->pmod_pos);
		uint16_t curMod = 0;

		/* Import the modulators (only SF2.1 and higher) */
		for (curMod = 0; curMod < pmod_count; curMod += pmod_size) {
			riff_seekInChunk(rh, pbag.wModNdx * pmod_size + curMod);
			riff_readInChunk(rh, &pmod, pmod_size);

			fluid_mod_t * mod_dest = fluid_mod_new();
			int type;

			if (mod_dest == NULL) {
				return;
			}
			mod_dest->next = NULL; /* pointer to next modulator, this is the end of the list now.*/

			/* *** Amount *** */
			mod_dest->amount = pmod.modAmount;

			/* *** Source *** */
			mod_dest->src1 = pmod.sfModSrcOper & 127; /* index of source 1, seven-bit value, SF2.01 section 8.2, page 50 */
			mod_dest->flags1 = 0;

			/* Bit 7: CC flag SF 2.01 section 8.2.1 page 50*/
			if (pmod.sfModSrcOper & (1 << 7)) {
				mod_dest->flags1 |= FLUID_MOD_CC;
			} else {
				mod_dest->flags1 |= FLUID_MOD_GC;
			}

			/* Bit 8: D flag SF 2.01 section 8.2.2 page 51*/
			if (pmod.sfModSrcOper & (1 << 8)) {
				mod_dest->flags1 |= FLUID_MOD_NEGATIVE;
			} else {
				mod_dest->flags1 |= FLUID_MOD_POSITIVE;
			}

			/* Bit 9: P flag SF 2.01 section 8.2.3 page 51*/
			if (pmod.sfModSrcOper & (1 << 9)) {
				mod_dest->flags1 |= FLUID_MOD_BIPOLAR;
			} else {
				mod_dest->flags1 |= FLUID_MOD_UNIPOLAR;
			}

			/* modulator source types: SF2.01 section 8.2.1 page 52 */
			type = (pmod.sfModSrcOper) >> 10;
			type &= 63; /* type is a 6-bit value */
			if (type == 0) {
				mod_dest->flags1 |= FLUID_MOD_LINEAR;
			} else if (type == 1) {
				mod_dest->flags1 |= FLUID_MOD_CONCAVE;
			} else if (type == 2) {
				mod_dest->flags1 |= FLUID_MOD_CONVEX;
			} else if (type == 3) {
				mod_dest->flags1 |= FLUID_MOD_SWITCH;
			} else {
				/* This shouldn't happen - unknown type!
				 * Deactivate the modulator by setting the amount to 0. */
				mod_dest->amount = 0;
			}

			/* *** Dest *** */
			mod_dest->dest = pmod.sfModDestOper; /* index of controlled generator */

			/* *** Amount source *** */
			mod_dest->src2 = pmod.sfModAmtSrcOper & 127; /* index of source 2, seven-bit value, SF2.01 section 8.2, p.50 */
			mod_dest->flags2 = 0;

			/* Bit 7: CC flag SF 2.01 section 8.2.1 page 50*/
			if (pmod.sfModAmtSrcOper & (1 << 7)) {
				mod_dest->flags2 |= FLUID_MOD_CC;
			} else {
				mod_dest->flags2 |= FLUID_MOD_GC;
			}

			/* Bit 8: D flag SF 2.01 section 8.2.2 page 51*/
			if (pmod.sfModAmtSrcOper & (1 << 8)) {
				mod_dest->flags2 |= FLUID_MOD_NEGATIVE;
			} else {
				mod_dest->flags2 |= FLUID_MOD_POSITIVE;
			}

			/* Bit 9: P flag SF 2.01 section 8.2.3 page 51*/
			if (pmod.sfModAmtSrcOper & (1 << 9)) {
				mod_dest->flags2 |= FLUID_MOD_BIPOLAR;
			} else {
				mod_dest->flags2 |= FLUID_MOD_UNIPOLAR;
			}

			/* modulator source types: SF2.01 section 8.2.1 page 52 */
			type = (pmod.sfModAmtSrcOper) >> 10;
			type &= 63; /* type is a 6-bit value */
			if (type == 0) {
				mod_dest->flags2 |= FLUID_MOD_LINEAR;
			} else if (type == 1) {
				mod_dest->flags2 |= FLUID_MOD_CONCAVE;
			} else if (type == 2) {
				mod_dest->flags2 |= FLUID_MOD_CONVEX;
			} else if (type == 3) {
				mod_dest->flags2 |= FLUID_MOD_SWITCH;
			} else {
				/* This shouldn't happen - unknown type!
				 * Deactivate the modulator by setting the amount to 0. */
				mod_dest->amount = 0;
			}

			/* *** Transform *** */
			/* SF2.01 only uses the 'linear' transform (0).
			 * Deactivate the modulator by setting the amount to 0 in any other case.
			 */
			if (pmod.sfModTransOper != 0) {
				mod_dest->amount = 0;
			}

			/* Store the new modulator in the zone The order of modulators
			 * will make a difference, at least in an instrument context: The
			 * second modulator overwrites the first one, if they only differ
			 * in amount. */
			if (curMod == 0) {
				psz->mod = mod_dest;
			} else {
				fluid_mod_t * last_mod = psz->mod;

				/* Find the end of the list */
				while (last_mod->next != NULL) {
					last_mod = last_mod->next;
				}

				last_mod->next = mod_dest;
			}
		}

		if (global) {
			ps->global_preset_zone = psz;
		} else {
			ps->preset_zones = fluid_list_append(ps->preset_zones, psz);
		}
	}
	ps->parsed = 1;
}

void sf2_delete_inst_zone(sf2_inst_zone *inst_zone) {
	fluid_list_t *next;
	fluid_list_t *list = inst_zone->gen;
	while (list) {
		next = list->next;
		FLUID_FREE(list->data);
		FLUID_FREE(list);
		list = next;
	}

	if (inst_zone->sample)
		FLUID_FREE(inst_zone->sample);
	FLUID_FREE(inst_zone);
}


void sf2_delete_inst(sf2_inst *inst) {
	fluid_list_t *next;
	fluid_list_t *list = inst->inst_zones;

	while (list) {
		next = list->next;
		sf2_delete_inst_zone(list->data);
		FLUID_FREE(list);
		list = next;
	}

	if (inst->global_inst_zone)
		sf2_delete_inst_zone(inst->global_inst_zone);

	inst->global_inst_zone = NULL;
	inst->inst_zones = NULL;
	inst->parsed = 0;
}

void sf2_delete_preset_zone(sf2_preset_zone *preset_zone) {
	fluid_list_t *next;
	fluid_list_t *list = preset_zone->gen;
	while (list) {
		next = list->next;
		FLUID_FREE(list->data);
		FLUID_FREE(list);
		list = next;
	}

	if (preset_zone->inst) {
		preset_zone->inst->refcount--;

		if (preset_zone->inst->refcount == 0) {
			sf2_delete_inst(preset_zone->inst);
		}
	}

	FLUID_FREE(preset_zone);
}

void sf2_delete_preset(sf2_preset *preset) {
	fluid_list_t *next;
	fluid_list_t *list = preset->preset_zones;

	while (list) {
		next = list->next;
		sf2_delete_preset_zone(list->data);
		FLUID_FREE(list);
		list = next;
	}

	if (preset->global_preset_zone)
		sf2_delete_preset_zone(preset->global_preset_zone);

	preset->global_preset_zone = 0;
	preset->preset_zones = NULL;
	preset->parsed = 0;
}

void sf2_delete_bank(sf2_bank *bank) {
	fluid_list_t *next;
	fluid_list_t *list = bank->presets;
	while (list) {
		next = list->next;
		sf2_delete_preset(list->data);
		FLUID_FREE(list->data);
		FLUID_FREE(list);
		list = next;
	}
	FLUID_FREE(bank);
}

void sf2_delete(sf2 *sf) {
	fluid_list_t *next;
	fluid_list_t *list = sf->insts;

	while (list) {
		next = list->next;
		sf2_delete_inst(list->data);
		FLUID_FREE(list->data);
		FLUID_FREE(list);
		list = next;
	}

	list = sf->banks;
	while (list) {
		next = list->next;
		sf2_delete_bank(list->data);
		FLUID_FREE(list);
		list = next;
	}
	riff_handleFree(sf->rh);
	FLUID_FREE(sf);
}

int
sf2_preset_zone_inside_range(sf2_preset_zone* zone, int key, int vel)
{
	return ((zone->keylo <= key) &&
	        (zone->keyhi >= key) &&
	        (zone->vello <= vel) &&
	        (zone->velhi >= vel));
}

int
sf2_inst_zone_inside_range(sf2_inst_zone* zone, int key, int vel)
{
	return ((zone->keylo <= key) &&
	        (zone->keyhi >= key) &&
	        (zone->vello <= vel) &&
	        (zone->velhi >= vel));
}


#include "fluid_sys.h"
#include "fluid_voice.h"
#include "fluid_log.h"

/***************************************************************
 *
 *                           SFONT LOADER
 */


char* fluid_altpreset_preset_get_name(fluid_preset_t* preset)
{
	return NULL;
}

int fluid_altpreset_preset_get_banknum(fluid_preset_t* preset)
{
	sf2_preset *sfpreset = (sf2_preset *)preset->data;
	return sfpreset->bank->num;
}

int fluid_altpreset_preset_get_num(fluid_preset_t* preset)
{
	sf2_preset *sfpreset = (sf2_preset *)preset->data;
	return sfpreset->num;
}

int
fluid_altpreset_preset_noteon(fluid_preset_t* preset, fluid_synth_t* synth, int chan, int key, int vel)
{
	sf2* sf = (sf2 *)preset->sfont->data;
	sf2_preset *sfpreset = (sf2_preset *)preset->data;

	if (!sfpreset)
		return 0;

	fluid_sample_t* sample;
	fluid_voice_t* voice;
	fluid_mod_t * mod;
	fluid_mod_t * mod_list[FLUID_NUM_MOD]; /* list for 'sorting' preset modulators */
	int mod_list_count;
	int i;

	if (!sfpreset->parsed)
		sf2_parse_preset(sf, sfpreset);

	fluid_list_t *pzone = sfpreset->preset_zones;
	sf2_preset_zone *gpsz = sfpreset->global_preset_zone;

	while (pzone != NULL) {
		sf2_preset_zone *psz = (sf2_preset_zone*)pzone->data;
		if (sf2_preset_zone_inside_range(psz, key, vel)) {
			sf2_inst *inst = psz->inst;
			if (inst) {
				fluid_list_t *izone = inst->inst_zones;
				sf2_inst_zone *gisz = inst->global_inst_zone;

				while (izone != NULL) {
					sf2_inst_zone *isz = (sf2_inst_zone*)izone->data;
					if (sf2_inst_zone_inside_range(isz, key, vel)) {
						if (isz->sample) {
							voice = fluid_synth_alloc_voice(synth, isz->sample, chan, key, vel);
							if (voice == NULL) {
								return FLUID_FAILED;
							}

							// instrument zone
							uint8_t inst_excluded[GEN_LAST] = {0};
							fluid_gen_t *gen;
							fluid_list_t *p;

							p = isz->gen;
							while (p != NULL) {
								gen = (fluid_gen_t *) p->data;
								uint8_t i = gen->num;
								fluid_voice_gen_set(voice, i, gen->val);
								inst_excluded[i] = 1;
								p = fluid_list_next(p);
							}

							if (gisz) {
								p = gisz->gen;
								while (p != NULL) {
									gen = (fluid_gen_t *) p->data;
									uint8_t i = gen->num;
									if (!inst_excluded[i])
										fluid_voice_gen_set(voice, i, gen->val);

									p = fluid_list_next(p);
								}
							}

							/* global instrument zone, modulators: Put them all into a list. */
							mod_list_count = 0;

							if (gisz) {
								mod = gisz->mod;
								while (mod) {
									mod_list[mod_list_count++] = mod;
									mod = mod->next;
								}
							}

							/* local instrument zone, modulators.
							 * Replace modulators with the same definition in the list:
							 * SF 2.01 page 69, 'bullet' 8
							 */
							mod = isz->mod;

							while (mod) {
								/* 'Identical' modulators will be deleted by setting their
								 *  list entry to NULL.  The list length is known, NULL
								 *  entries will be ignored later.  SF2.01 section 9.5.1
								 *  page 69, 'bullet' 3 defines 'identical'.  */

								for (i = 0; i < mod_list_count; i++) {
									if (mod_list[i] && fluid_mod_test_identity(mod, mod_list[i])) {
										mod_list[i] = NULL;
									}
								}

								/* Finally add the new modulator to to the list. */
								mod_list[mod_list_count++] = mod;
								mod = mod->next;
							}

							/* Add instrument modulators (global / local) to the voice. */
							for (i = 0; i < mod_list_count; i++) {

								mod = mod_list[i];

								if (mod != NULL) { /* disabled modulators CANNOT be skipped. */

									/* Instrument modulators -supersede- existing (default)
									 * modulators.  SF 2.01 page 69, 'bullet' 6 */
									fluid_voice_add_mod(voice, mod, FLUID_VOICE_OVERWRITE);
								}
							}

							// preset zone
							uint8_t preset_excluded[GEN_LAST] = {0};

							p = psz->gen;
							while (p != NULL) {
								gen = (fluid_gen_t *) p->data;
								uint8_t i = gen->num;
								if ((i != GEN_STARTADDROFS)
								        && (i != GEN_ENDADDROFS)
								        && (i != GEN_STARTLOOPADDROFS)
								        && (i != GEN_ENDLOOPADDROFS)
								        && (i != GEN_STARTADDRCOARSEOFS)
								        && (i != GEN_ENDADDRCOARSEOFS)
								        && (i != GEN_STARTLOOPADDRCOARSEOFS)
								        && (i != GEN_KEYNUM)
								        && (i != GEN_VELOCITY)
								        && (i != GEN_ENDLOOPADDRCOARSEOFS)
								        && (i != GEN_SAMPLEMODE)
								        && (i != GEN_EXCLUSIVECLASS)
								        && (i != GEN_OVERRIDEROOTKEY)) {
									fluid_voice_gen_incr(voice, i, gen->val);
									preset_excluded[i] = 1;
								}
								p = fluid_list_next(p);
							}

							if (gpsz) {
								p = gpsz->gen;
								while (p != NULL) {
									gen = (fluid_gen_t *) p->data;
									uint8_t i = gen->num;
									if ((i != GEN_STARTADDROFS)
									        && (i != GEN_ENDADDROFS)
									        && (i != GEN_STARTLOOPADDROFS)
									        && (i != GEN_ENDLOOPADDROFS)
									        && (i != GEN_STARTADDRCOARSEOFS)
									        && (i != GEN_ENDADDRCOARSEOFS)
									        && (i != GEN_STARTLOOPADDRCOARSEOFS)
									        && (i != GEN_KEYNUM)
									        && (i != GEN_VELOCITY)
									        && (i != GEN_ENDLOOPADDRCOARSEOFS)
									        && (i != GEN_SAMPLEMODE)
									        && (i != GEN_EXCLUSIVECLASS)
									        && (i != GEN_OVERRIDEROOTKEY)) {
										if (!preset_excluded[i])
											fluid_voice_gen_incr(voice, i, gen->val);
									}
									p = fluid_list_next(p);
								}
							}



							/* Global preset zone, modulators: put them all into a
							 * list. */
							mod_list_count = 0;
							if (gpsz) {
								mod = gpsz->mod;
								while (mod) {
									mod_list[mod_list_count++] = mod;
									mod = mod->next;
								}
							}

							/* Process the modulators of the local preset zone.  Kick
							 * out all identical modulators from the global preset zone
							 * (SF 2.01 page 69, second-last bullet) */

							mod = psz->mod;
							while (mod) {
								for (i = 0; i < mod_list_count; i++) {
									if (mod_list[i] && fluid_mod_test_identity(mod, mod_list[i])) {
										mod_list[i] = NULL;
									}
								}

								/* Finally add the new modulator to the list. */
								mod_list[mod_list_count++] = mod;
								mod = mod->next;
							}

							/* Add preset modulators (global / local) to the voice. */
							for (i = 0; i < mod_list_count; i++) {
								mod = mod_list[i];
								if ((mod != NULL) && (mod->amount != 0)) { /* disabled modulators can be skipped. */

									/* Preset modulators -add- to existing instrument /
									 * default modulators.  SF2.01 page 70 first bullet on
									 * page */
									fluid_voice_add_mod(voice, mod, FLUID_VOICE_ADD);
								}
							}

							fluid_synth_start_voice(synth, voice);

						}
					}
					izone = fluid_list_next(izone);
				}
			}
		}
		pzone = fluid_list_next(pzone);
	}

	return FLUID_OK;
}


int fluid_altsfont_sfont_delete(fluid_sfont_t* sfont)
{
	sf2* sf = (sf2 *)sfont->data;
	sf2_delete(sf);

	FLUID_FREE(sfont);
	return 0;
}

char* fluid_altsfont_sfont_get_name(fluid_sfont_t* sfont)
{
	sf2* sf = (sf2 *)sfont->data;

	return NULL;
}

int fluid_altpreset_preset_delete(fluid_preset_t* preset)
{
	sf2_preset *sfpreset = (sf2_preset *)preset->data;

	if (sfpreset)
		sf2_delete_preset(sfpreset);
	FLUID_FREE(preset);

	/* TODO: free modulators */

	return 0;
}

fluid_preset_t*
fluid_altsfont_sfont_get_preset(fluid_sfont_t* sfont, unsigned int bank, unsigned int prenum)
{
	fluid_preset_t* preset;
	sf2* sf = (sf2 *)sfont->data;
	sf2_preset *sfpreset;

	preset = FLUID_NEW(fluid_preset_t);
	if (preset == NULL) {
		FLUID_LOG(FLUID_ERR, "Out of memory");
		return NULL;
	}

	sfpreset = sf2_bank_preset_get(sf, bank, prenum);

	preset->sfont = sfont;
	preset->data = sfpreset;
	preset->free = fluid_altpreset_preset_delete;
	preset->get_name = fluid_altpreset_preset_get_name;
	preset->get_banknum = fluid_altpreset_preset_get_banknum;
	preset->get_num = fluid_altpreset_preset_get_num;
	preset->noteon = fluid_altpreset_preset_noteon;
	preset->notify = NULL;

	return preset;
}

int delete_fluid_altsfloader(fluid_sfloader_t* loader)
{
	if (loader) {
		FLUID_FREE(loader);
	}
	return FLUID_OK;
}

fluid_sfont_t* fluid_altsfloader_load(fluid_sfloader_t* loader, const char* filename)
{
	sf2* sf;
	fluid_sfont_t* sfont;

	sf = sf2_load(filename);
	sf2_load_presets(sf);
	sf->filename = filename;

	sfont = FLUID_NEW(fluid_sfont_t);
	if (sfont == NULL) {
		FLUID_LOG(FLUID_ERR, "Out of memory");
		return NULL;
	}

	riff_handle *rh = sf->rh;
	riff_seek(rh, sf->smpl_pos);

	sf->samplepos = sf->smpl_pos;
	sf->samplesize = rh->c_size;

	fluid_file fd = rh->fh;

#ifdef FLUID_SAMPLE_MMAP
	sf->sampledata = (int16_t *)FLUID_MMAP(sf->samplepos, sf->samplesize, fd);
#else
	sf->sampledata = (short*) FLUID_MALLOC(sf->samplesize);
	if (sf->sampledata == NULL) {
		FLUID_LOG(FLUID_ERR, "Out of memory");
		return FLUID_FAILED;
	}
	if (FLUID_FREAD(sf->sampledata, 1, sf->samplesize, fd) < sf->samplesize) {
		FLUID_LOG(FLUID_ERR, "Failed to read sample data");
		return FLUID_FAILED;
	}
#endif
	sfont->data = sf;
	sfont->free = fluid_altsfont_sfont_delete;
	sfont->get_name = fluid_altsfont_sfont_get_name;
	sfont->get_preset = fluid_altsfont_sfont_get_preset;
	sfont->iteration_start = NULL;
	sfont->iteration_next = NULL;

	return sfont;
}

fluid_sfloader_t* new_fluid_altsfloader()
{
	fluid_sfloader_t* loader;

	loader = FLUID_NEW(fluid_sfloader_t);
	if (loader == NULL) {
		FLUID_LOG(FLUID_ERR, "Out of memory");
		return NULL;
	}

	loader->data = NULL;
	loader->free = delete_fluid_altsfloader;
	loader->load = fluid_altsfloader_load;

	return loader;
}
