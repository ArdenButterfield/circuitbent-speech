/*
 * Copyright (C) 2005 to 2015 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015-2018 Reece H. Dunn
 * Copyright (C) 2018 Juho Hiltunen
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

#ifndef ESPEAK_NG_WAVEGEN_H
#define ESPEAK_NG_WAVEGEN_H

#include "voice.h"
#include <espeak-ng/common.h>
#ifdef __cplusplus
extern "C"
{
#endif

int GetAmplitude(EspeakProcessorContext* epContext);
void InitBreath(EspeakProcessorContext* epContext);
int PeaksToHarmspect(EspeakProcessorContext* epContext, wavegen_peaks_t *peaks,
		int pitch,
		int *htab,
		int control);

void SetPitch2(voice_t *voice,
		int pitch1,
		int pitch2,
		int *pitch_base,
		int *pitch_range);

void WavegenInit(EspeakProcessorContext* epContext, int rate,
		int wavemult_fact);

void WavegenFini(void);


int WavegenFill(EspeakProcessorContext* epContext);
void WavegenSetVoice(EspeakProcessorContext* epContext, voice_t *v);
int WcmdqFree(EspeakProcessorContext* epContext);
void WcmdqStop(EspeakProcessorContext* epContext);
int WcmdqUsed(EspeakProcessorContext* epContext);
void WcmdqInc(EspeakProcessorContext* epContext);

#ifdef __cplusplus
}
#endif

#endif

