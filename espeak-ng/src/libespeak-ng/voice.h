/*
 * Copyright (C) 2005 to 2007 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015 Reece H. Dunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

#ifndef ESPEAK_NG_VOICE_H
#define ESPEAK_NG_VOICE_H

#include <espeak-ng/espeak_ng.h>

#include "mnemonics.h"
#include <espeak-ng/common.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	V_NAME = 1,
	V_LANGUAGE,
	V_GENDER,
	V_PHONEMES,
	V_DICTIONARY,
	V_VARIANTS,

	V_MAINTAINER,
	V_STATUS,

	// these affect voice quality, are independent of language
	V_FORMANT,
	V_PITCH,
	V_ECHO,
	V_FLUTTER,
	V_ROUGHNESS,
	V_CLARITY,
	V_TONE,
	V_VOICING,
	V_BREATH,
	V_BREATHW,

	// these override defaults set by the translator
	V_LOWERCASE_SENTENCE,
	V_WORDGAP,
	V_INTONATION,
	V_TUNES,
	V_SPELLINGSTRESS,
	V_STRESSLENGTH,
	V_STRESSAMP,
	V_STRESSADD,
	V_DICTRULES,
	V_STRESSRULE,
	V_STRESSOPT,
	V_NUMBERS,

	V_MBROLA,
	V_KLATT,
	V_FAST,
	V_SPEED,
	V_DICTMIN,

	// these need a phoneme table to have been specified
	V_REPLACE,
	V_CONSONANTS
} VOICELANGATTRIBUTES;

static const MNEM_TAB langopts_tab[] = {
	{ "apostrophe",       0x100+LOPT_APOSTROPHE },
	{ "brackets",       0x100+LOPT_BRACKET_PAUSE },
	{ "bracketsAnnounced",       0x100+LOPT_BRACKET_PAUSE_ANNOUNCED },
	{ "dict_min",     V_DICTMIN },
	{ "dictrules",    V_DICTRULES },
	{ "intonation",   V_INTONATION },
	{ "l_dieresis",       0x100+LOPT_DIERESES },
	{ "l_prefix",         0x100+LOPT_PREFIXES },
	{ "l_regressive_v",   0x100+LOPT_REGRESSIVE_VOICING },
	{ "l_unpronouncable", 0x100+LOPT_UNPRONOUNCABLE },
	{ "l_sonorant_min",   0x100+LOPT_SONORANT_MIN },
	{ "lowercaseSentence",	V_LOWERCASE_SENTENCE },
	{ "numbers",      V_NUMBERS },
    { "spellingStress",    V_SPELLINGSTRESS },
    { "stressAdd",    V_STRESSADD },
    { "stressAmp",    V_STRESSAMP },
    { "stressLength", V_STRESSLENGTH },
    { "stressOpt",    V_STRESSOPT },
    { "stressRule",   V_STRESSRULE },
    { "tunes",        V_TUNES },
    { "words",        V_WORDGAP },

    { NULL, 0 }
};

static const MNEM_TAB keyword_tab[] = {
	{ "name",         V_NAME },
	{ "language",     V_LANGUAGE },
	{ "gender",       V_GENDER },
	{ "variants",     V_VARIANTS },
	{ "formant",      V_FORMANT },
	{ "pitch",        V_PITCH },
	{ "phonemes",     V_PHONEMES },
	{ "dictionary",   V_DICTIONARY },
	{ "replace",      V_REPLACE },
	{ "echo",         V_ECHO },
	{ "flutter",      V_FLUTTER },
	{ "roughness",    V_ROUGHNESS },
	{ "clarity",      V_CLARITY },
	{ "tone",         V_TONE },
	{ "voicing",      V_VOICING },
	{ "breath",       V_BREATH },
	{ "breathw",      V_BREATHW },
	{ "mbrola",       V_MBROLA },
	{ "consonants",   V_CONSONANTS },
	{ "klatt",        V_KLATT },
	{ "fast_test2",   V_FAST },
	{ "speed",        V_SPEED },


	{ "maintainer",   V_MAINTAINER },
    { "status",       V_STATUS },

	{ NULL, 0 }
};

const char *SelectVoice(EspeakProcessorContext* epContext, espeak_VOICE *voice_select, int *found);
espeak_VOICE *SelectVoiceByName(EspeakProcessorContext* epContext, espeak_VOICE **voices, const char *name);
voice_t *LoadVoice(EspeakProcessorContext* epContext, const char *voice_name, int control);
voice_t *LoadVoiceVariant(EspeakProcessorContext* epContext, const char *voice_name, int variant);
espeak_ng_STATUS DoVoiceChange(EspeakProcessorContext* epContext, voice_t *v);
void WavegenSetVoice(EspeakProcessorContext* epContext, voice_t *v);
void ReadNumbers(char *p, int *flags, int maxValue,  const MNEM_TAB *keyword_tab, int key);
int Read8Numbers(char *data_in, int data[8]);
void ReadTonePoints(char *string, int *tone_pts);
void VoiceReset(EspeakProcessorContext* epContext, int control);
void FreeVoiceList(EspeakProcessorContext* epContext);

#ifdef __cplusplus
}
#endif

#endif
