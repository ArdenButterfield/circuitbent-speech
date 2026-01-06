/*
 * Copyright (C) 2008 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015-2017 Reece H. Dunn
 *
 * Based on a re-implementation by:
 * (c) 1993,94 Jon Iles and Nick Ing-Simmons
 * of the Klatt cascade-parallel formant synthesizer
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

#ifndef ESPEAK_NG_KLATT_H
#define ESPEAK_NG_KLATT_H

#include "voice.h"                    // for voice_t
#include "synthesize.h"              // for frame_t, WGEN_DATA

#ifdef __cplusplus
extern "C"
{
#endif

#define CASCADE_PARALLEL 1         /* Type of synthesis model */
#define ALL_PARALLEL     2

#define IMPULSIVE        1         /* Type of voicing source */
#define NATURAL          2
#define SAMPLED          3
#define SAMPLED2         4


/* Resonator Structure */




/* Structure for Klatt Parameters */

#define F_NZ   0  // nasal zero formant
#define F1     1
#define F2     2
#define F3     3
#define F4     4
#define F5     5
#define F6     6
#define F_NP   9  // nasal pole formant



typedef struct {
	int freq;     // Hz
	int bw;   // klatt bandwidth
	int ap;   // parallel amplitude
	int bp;   // parallel bandwidth
	double freq1; // floating point versions of the above
	double bw1;
	double ap1;
	double bp1;
	double freq_inc;    // increment by this every 64 samples
	double bw_inc;
	double ap_inc;
	double bp_inc;
}  klatt_peaks_t;

void KlattInit(EspeakProcessorContext* epContext);
void KlattFini(void);
void KlattReset(EspeakProcessorContext* epContext, int control);
int Wavegen_Klatt(EspeakProcessorContext* epContext, int length, int resume, frame_t *fr1, frame_t *fr2, WGEN_DATA *wdata, voice_t *wvoice);

#ifdef __cplusplus
}
#endif

#endif
