/*
 * Copyright (C) 2005 to 2014 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015-2017 Reece H. Dunn
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

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <espeak-ng/encoding.h>

#include "synthesize.h"
#include "dictionary.h"           // for WritePhMnemonic, GetTranslatedPhone...
#include "intonation.h"           // for CalcPitches
#include "mbrola.h"               // for MbrolaGenerate, mbrola_name
#include "phoneme.h"              // for PHONEME_TAB, phVOWEL, phLIQUID, phN...
#include "setlengths.h"           // for CalcLengths
#include "soundicon.h"            // for soundicon_tab, n_soundicon
#include "synthdata.h"            // for InterpretPhoneme, GetEnvelope, Inte...
#include "translate.h"            // for translator, LANGUAGE_OPTIONS, Trans...
#include "voice.h"                // for voice_t, voice, LoadVoiceVariant
#include "wavegen.h"              // for WcmdqInc, WcmdqFree, WcmdqStop
#include "speech.h"               // for MAKE_MEM_UNDEFINED

static void SmoothSpect(EspeakProcessorContext* epContext);

static int (*phoneme_callback)(const char *) = NULL;

#define RMS_GLOTTAL1 35   // vowel before glottal stop
#define RMS_START 28  // 28
#define VOWEL_FRONT_LENGTH  50

const char *WordToString(char buf[5], unsigned int word)
{
	// Convert a phoneme mnemonic word into a string
	int ix;

	for (ix = 0; ix < 4; ix++)
		buf[ix] = word >> (ix*8);
	buf[4] = 0;
	return buf;
}

void SynthesizeInit(EspeakProcessorContext* epContext)
{
	epContext->last_pitch_cmd = 0;
	epContext->last_amp_cmd = 0;
	epContext->last_frame = NULL;
	epContext->syllable_centre = -1;
}

static void EndAmplitude(EspeakProcessorContext* epContext)
{
	if (epContext->amp_length > 0) {
		if (epContext->wcmdq[epContext->last_amp_cmd][1] == 0)
			epContext->wcmdq[epContext->last_amp_cmd][1] = epContext->amp_length;
		epContext->amp_length = 0;
	}
}

static void EndPitch(EspeakProcessorContext* epContext, int voice_break)
{
	// possible end of pitch envelope, fill in the length
	if ((epContext->pitch_length > 0) && (epContext->last_pitch_cmd >= 0)) {
		if (epContext->wcmdq[epContext->last_pitch_cmd][1] == 0)
			epContext->wcmdq[epContext->last_pitch_cmd][1] = epContext->pitch_length;
		epContext->pitch_length = 0;
	}

	if (voice_break) {
		epContext->last_wcmdq = -1;
		epContext->last_frame = NULL;
		epContext->syllable_end = epContext->wcmdq_tail;
		SmoothSpect(epContext);
		epContext->syllable_centre = -1;
	}
}

static void DoAmplitude(EspeakProcessorContext* epContext, int amp, const unsigned char *amp_env)
{
	intptr_t *q;

	epContext->last_amp_cmd = epContext->wcmdq_tail;
	epContext->amp_length = 0; // total length of vowel with this amplitude envelope

	q = epContext->wcmdq[epContext->wcmdq_tail];
	q[0] = WCMD_AMPLITUDE;
	q[1] = 0; // fill in later from epContext->amp_length
	q[2] = (intptr_t)amp_env;
	q[3] = amp;
	WcmdqInc(epContext);
}

static void DoPhonemeAlignment(EspeakProcessorContext* epContext, char* pho, int type)
{
	epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_PHONEME_ALIGNMENT;
	epContext->wcmdq[epContext->wcmdq_tail][1] = (intptr_t)pho;
	epContext->wcmdq[epContext->wcmdq_tail][2] = type;
	WcmdqInc(epContext);
}

static void DoPitch(EspeakProcessorContext* epContext, const unsigned char *env, int pitch1, int pitch2)
{
	intptr_t *q;

	EndPitch(epContext, 0);

	if (pitch1 == 255) {
		// pitch was not set
		pitch1 = 55;
		pitch2 = 76;
		env = envelope_data[PITCHfall];
	}
	epContext->last_pitch_cmd = epContext->wcmdq_tail;
	epContext->pitch_length = 0; // total length of spect with this pitch envelope

	if (pitch2 < 0)
		pitch2 = 0;

	q = epContext->wcmdq[epContext->wcmdq_tail];
	q[0] = WCMD_PITCH;
	q[1] = 0; // length, fill in later from epContext->pitch_length
	q[2] = (intptr_t)env;
	q[3] = (pitch1 << 16) + pitch2;
	WcmdqInc(epContext);
}

int PauseLength(EspeakProcessorContext* epContext, int pause, int control)
{
	unsigned int len;

	if (control == 0) {
		if (pause >= 200)
			len = (pause * epContext->speed.clause_pause_factor)/256;
		else
			len = (pause * epContext->speed.pause_factor)/256;
	} else
		len = (pause * epContext->speed.wav_factor)/256;

	if (len < epContext->speed.min_pause)
		len = epContext->speed.min_pause; // mS, limit the amount to which pauses can be shortened
	return len;
}

static void DoPause(EspeakProcessorContext* epContext, int length, int control)
{
	// length in nominal mS
	// control = 1, less shortening at fast speeds

	unsigned int len;

	if (length == 0)
		len = 0;
	else {
		len = PauseLength(epContext, length, control);

		if (len < 90000)
			len = (len * epContext->samplerate) / 1000; // convert from mS to number of samples
		else {
			int srate2 = epContext->samplerate / 25; // avoid overflow
			len = (len * srate2) / 40;
		}
	}

	EndPitch(epContext, 1);
	epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_PAUSE;
	epContext->wcmdq[epContext->wcmdq_tail][1] = len;
	WcmdqInc(epContext);
	epContext->last_frame = NULL;

	if (epContext->fmt_amplitude != 0) {
		epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_FMT_AMPLITUDE;
		epContext->wcmdq[epContext->wcmdq_tail][1] = epContext->fmt_amplitude = 0;
		WcmdqInc(epContext);
	}
}

static int DoSample2(EspeakProcessorContext* epContext, int index, int which, int std_length, int control, int length_mod, int amp)
{
	int length;
	int wav_length;
	int wav_scale;
	int min_length;
	int x;
	int len4;
	intptr_t *q;
	unsigned char *p;

	index = index & 0x7fffff;
	p = &epContext->wavefile_data[index];
	wav_scale = p[2];
	wav_length = (p[1] * 256);
	wav_length += p[0]; // length in bytes

	if (wav_length == 0)
		return 0;

	min_length = epContext->speed.min_sample_len;

	if (wav_scale == 0)
		min_length *= 2; // 16 bit samples

	if (std_length > 0) {
		std_length = (std_length * epContext->samplerate)/1000;
		if (wav_scale == 0)
			std_length *= 2;

		x = (min_length * std_length)/wav_length;
		if (x > min_length)
			min_length = x;
	} else {
		// no length specified, use the length of the stored sound
		std_length = wav_length;
	}

	if (length_mod > 0)
		std_length = (std_length * length_mod)/256;

	length = (std_length * epContext->speed.wav_factor)/256;

	if (control & pd_DONTLENGTHEN) {
		// this option is used for Stops, with short noise bursts.
		// Don't change their length much.
		if (length > std_length) {
			// don't let length exceed std_length
			length = std_length;
		}
	}

	if (length < min_length)
		length = min_length;


	if (wav_scale == 0) {
		// 16 bit samples
		length /= 2;
		wav_length /= 2;
	}

	if (amp < 0)
		return length;

	len4 = wav_length / 4;

	index += 4;

	if (which & 0x100) {
		// mix this with synthesised wave
		epContext->last_wcmdq = epContext->wcmdq_tail;
		q = epContext->wcmdq[epContext->wcmdq_tail];
		q[0] = WCMD_WAVE2;
		q[1] = length | (wav_length << 16); // length in samples
		q[2] = (intptr_t)(&epContext->wavefile_data[index]);
		q[3] = wav_scale + (amp << 8);
		WcmdqInc(epContext);
		return length;
	}

	if (length > wav_length) {
		x = len4*3;
		length -= x;
	} else {
		x = length;
		length = 0;
	}

	epContext->last_wcmdq = epContext->wcmdq_tail;
	q = epContext->wcmdq[epContext->wcmdq_tail];
	q[0] = WCMD_WAVE;
	q[1] = x; // length in samples
	q[2] = (intptr_t)(&epContext->wavefile_data[index]);
	q[3] = wav_scale + (amp << 8);
	WcmdqInc(epContext);

	while (length > len4*3) {
		x = len4;
		if (wav_scale == 0)
			x *= 2;

		epContext->last_wcmdq = epContext->wcmdq_tail;
		q = epContext->wcmdq[epContext->wcmdq_tail];
		q[0] = WCMD_WAVE;
		q[1] = len4*2; // length in samples
		q[2] = (intptr_t)(&epContext->wavefile_data[index+x]);
		q[3] = wav_scale + (amp << 8);
		WcmdqInc(epContext);

		length -= len4*2;
	}

	if (length > 0) {
		x = wav_length - length;
		if (wav_scale == 0)
			x *= 2;
		epContext->last_wcmdq = epContext->wcmdq_tail;
		q = epContext->wcmdq[epContext->wcmdq_tail];
		q[0] = WCMD_WAVE;
		q[1] = length; // length in samples
		q[2] = (intptr_t)(&epContext->wavefile_data[index+x]);
		q[3] = wav_scale + (amp << 8);
		WcmdqInc(epContext);
	}

	return length;
}

int DoSample3(EspeakProcessorContext* epContext, PHONEME_DATA *phdata, int length_mod, int amp)
{
	int amp2;
	int len;
	EndPitch(epContext, 1);

	if (amp == -1) {
		// just get the length, don't produce sound
		amp2 = amp;
	} else {
		amp2 = phdata->sound_param[pd_WAV];
		if (amp2 == 0)
			amp2 = 100;
		amp2 = (amp2 * 32)/100;
	}

	epContext->seq_len_adjust = 0;

	if (phdata->sound_addr[pd_WAV] == 0)
		len = 0;
	else
		len = DoSample2(epContext, phdata->sound_addr[pd_WAV], 2, phdata->pd_param[pd_LENGTHMOD]*2, phdata->pd_control, length_mod, amp2);
	epContext->last_frame = NULL;
	return len;
}

static frame_t *AllocFrame(void)
{
	// Allocate a temporary spectrum frame for the wavegen queue. Use a pool which is big
	// enough to use a round-robin without checks.
	// Only needed for modifying spectra for blending to consonants

	#define N_FRAME_POOL N_WCMDQ
	static int ix = 0;
	static frame_t frame_pool[N_FRAME_POOL];

	ix++;
	if (ix >= N_FRAME_POOL)
		ix = 0;

	MAKE_MEM_UNDEFINED(&frame_pool[ix], sizeof(frame_pool[ix]));
	return &frame_pool[ix];
}

static void set_frame_rms(EspeakProcessorContext* epContext, frame_t *fr, int new_rms)
{
	// Each frame includes its RMS amplitude value, so to set a new
	// RMS just adjust the formant amplitudes by the appropriate ratio

	int x;
	int ix;

	static const short sqrt_tab[200] = {
		  0,  64,  90, 110, 128, 143, 156, 169, 181, 192, 202, 212, 221, 230, 239, 247,
		256, 263, 271, 278, 286, 293, 300, 306, 313, 320, 326, 332, 338, 344, 350, 356,
		362, 367, 373, 378, 384, 389, 394, 399, 404, 409, 414, 419, 424, 429, 434, 438,
		443, 448, 452, 457, 461, 465, 470, 474, 478, 483, 487, 491, 495, 499, 503, 507,
		512, 515, 519, 523, 527, 531, 535, 539, 543, 546, 550, 554, 557, 561, 565, 568,
		572, 576, 579, 583, 586, 590, 593, 596, 600, 603, 607, 610, 613, 617, 620, 623,
		627, 630, 633, 636, 640, 643, 646, 649, 652, 655, 658, 662, 665, 668, 671, 674,
		677, 680, 683, 686, 689, 692, 695, 698, 701, 704, 706, 709, 712, 715, 718, 721,
		724, 726, 729, 732, 735, 738, 740, 743, 746, 749, 751, 754, 757, 759, 762, 765,
		768, 770, 773, 775, 778, 781, 783, 786, 789, 791, 794, 796, 799, 801, 804, 807,
		809, 812, 814, 817, 819, 822, 824, 827, 829, 832, 834, 836, 839, 841, 844, 846,
		849, 851, 853, 856, 858, 861, 863, 865, 868, 870, 872, 875, 877, 879, 882, 884,
		886, 889, 891, 893, 896, 898, 900, 902
	};

	if (epContext->voice->klattv[0]) {
		if (new_rms == -1)
			fr->klattp[KLATT_AV] = 50;
		return;
	}

	if (fr->rms == 0) return; // check for divide by zero
	x = (new_rms * 64)/fr->rms;
	if (x >= 200) x = 199;

	x = sqrt_tab[x]; // sqrt(new_rms/fr->rms)*0x200;

	for (ix = 0; ix < 8; ix++) {
		int h;
		h = fr->fheight[ix] * x;
		fr->fheight[ix] = h/0x200;
	}
}

static void formants_reduce_hf(EspeakProcessorContext* epContext, frame_t *fr, int level)
{
	// change height of peaks 2 to 8, percentage
	if (epContext->voice->klattv[0])
		return;

	for (int ix = 2; ix < 8; ix++) {
		int x;
		x = fr->fheight[ix] * level;
		fr->fheight[ix] = x/100;
	}
}

static frame_t *CopyFrame(frame_t *frame1, int copy)
{
	// create a copy of the specified frame in temporary buffer

	frame_t *frame2;

	if ((copy == 0) && (frame1->frflags & FRFLAG_COPIED)) {
		// this frame has already been copied in temporary rw memory
		return frame1;
	}

	frame2 = AllocFrame();
	if (frame2 != NULL) {
		memcpy(frame2, frame1, sizeof(frame_t));
		frame2->length = 0;
		frame2->frflags |= FRFLAG_COPIED;
	}
	return frame2;
}

static frame_t *DuplicateLastFrame(frameref_t *seq, int n_frames, int length)
{
	frame_t *fr;

	seq[n_frames-1].length = length;
	fr = CopyFrame(seq[n_frames-1].frame, 1);
	seq[n_frames].frame = fr;
	seq[n_frames].length = 0;
	return fr;
}

static void AdjustFormants(EspeakProcessorContext* epContext, frame_t *fr, int target, int min, int max, int f1_adj, int f3_adj, int hf_reduce, int flags)
{
	int x;

	target = (target * epContext->voice->formant_factor)/256;

	x = (target - fr->ffreq[2]) / 2;
	if (x > max) x = max;
	if (x < min) x = min;
	fr->ffreq[2] += x;
	fr->ffreq[3] += f3_adj;

	if (flags & 0x20)
		f3_adj = -f3_adj; // reverse direction for f4,f5 change
	fr->ffreq[4] += f3_adj;
	fr->ffreq[5] += f3_adj;

	if (f1_adj == 1) {
		x = (235 - fr->ffreq[1]);
		if (x < -100) x = -100;
		if (x > -60) x = -60;
		fr->ffreq[1] += x;
	}
	if (f1_adj == 2) {
		x = (235 - fr->ffreq[1]);
		if (x < -300) x = -300;
		if (x > -150) x = -150;
		fr->ffreq[1] += x;
		fr->ffreq[0] += x;
	}
	if (f1_adj == 3) {
		x = (100 - fr->ffreq[1]);
		if (x < -400) x = -400;
		if (x > -300) x = -400;
		fr->ffreq[1] += x;
		fr->ffreq[0] += x;
	}
	formants_reduce_hf(epContext, fr, hf_reduce);
}

static int VowelCloseness(frame_t *fr)
{
	// return a value 0-3 depending on the vowel's f1
	int f1;

	if ((f1 = fr->ffreq[1]) < 300)
		return 3;
	if (f1 < 400)
		return 2;
	if (f1 < 500)
		return 1;
	return 0;
}

int FormantTransition2(EspeakProcessorContext* epContext, frameref_t *seq, int *n_frames, unsigned int data1, unsigned int data2, PHONEME_TAB *other_ph, int which)
{
	int len;
	int rms;
	int f1;
	int f2;
	int f2_min;
	int f2_max;
	int f3_adj;
	int f3_amp;
	int flags;
	int vcolour;

	#define N_VCOLOUR 2
	// percentage change for each formant in 256ths
	static const short vcolouring[N_VCOLOUR][5] = {
		{ 243, 272, 256, 256, 256 }, // palatal consonant follows
		{ 256, 256, 240, 240, 240 }, // retroflex
	};

	frame_t *fr = NULL;

	if (*n_frames < 2)
		return 0;

	len = (data1 & 0x3f) * 2;
	rms = (data1 >> 6) & 0x3f;
	flags = (data1 >> 12);

	f2 = (data2 & 0x3f) * 50;
	f2_min = (((data2 >> 6) & 0x1f) - 15) * 50;
	f2_max = (((data2 >> 11) & 0x1f) - 15) * 50;
	f3_adj = (((data2 >> 16) & 0x1f) - 15) * 50;
	f3_amp = ((data2 >> 21) & 0x1f) * 8;
	f1 = ((data2 >> 26) & 0x7);
	vcolour = (data2 >> 29);

	if ((other_ph != NULL) && (other_ph->mnemonic == '?'))
		flags |= 8;

	if (which == 1) {
		// entry to vowel
		fr = CopyFrame(seq[0].frame, 0);
		seq[0].frame = fr;
		seq[0].length = VOWEL_FRONT_LENGTH;
		if (len > 0)
			seq[0].length = len;
		seq[0].frflags |= FRFLAG_LEN_MOD2; // reduce length modification
		fr->frflags |= FRFLAG_LEN_MOD2;

		int next_rms = seq[1].frame->rms;

		if (epContext->voice->klattv[0])
			fr->klattp[KLATT_AV] = seq[1].frame->klattp[KLATT_AV] - 4;
		if (f2 != 0) {
			if (rms & 0x20)
				set_frame_rms(epContext, fr, (next_rms * (rms & 0x1f))/30);
			AdjustFormants(epContext, fr, f2, f2_min, f2_max, f1, f3_adj, f3_amp, flags);

			if ((rms & 0x20) == 0)
				set_frame_rms(epContext, fr, rms*2);
		} else {
			if (flags & 8)
				set_frame_rms(epContext, fr, (next_rms*24)/32);
			else
				set_frame_rms(epContext, fr, RMS_START);
		}

		if (flags & 8)
			epContext->modn_flags = 0x800 + (VowelCloseness(fr) << 8);
	} else {
		// exit from vowel
		rms = rms*2;
		if ((f2 != 0) || (flags != 0)) {

			if (flags & 8) {
				fr = CopyFrame(seq[*n_frames-1].frame, 0);
				seq[*n_frames-1].frame = fr;
				rms = RMS_GLOTTAL1;

				// degree of glottal-stop effect depends on closeness of vowel (indicated by f1 freq)
				epContext->modn_flags = 0x400 + (VowelCloseness(fr) << 8);
			} else {
				fr = DuplicateLastFrame(seq, (*n_frames)++, len);
				if (len > 36)
					epContext->seq_len_adjust += (len - 36);

				if (f2 != 0)
					AdjustFormants(epContext, fr, f2, f2_min, f2_max, f1, f3_adj, f3_amp, flags);
			}

			set_frame_rms(epContext, fr, rms);

			if ((vcolour > 0) && (vcolour <= N_VCOLOUR)) {
				for (int ix = 0; ix < *n_frames; ix++) {
					fr = CopyFrame(seq[ix].frame, 0);
					seq[ix].frame = fr;

					for (int formant = 1; formant <= 5; formant++) {
						int x;
						x = fr->ffreq[formant] * vcolouring[vcolour-1][formant-1];
						fr->ffreq[formant] = x / 256;
					}
				}
			}
		}
	}

	if (fr != NULL) {
		if (flags & 4)
			fr->frflags |= FRFLAG_FORMANT_RATE;
		if (flags & 2)
			fr->frflags |= FRFLAG_BREAK; // don't merge with next frame
	}

	if (flags & 0x40)
		DoPause(epContext, 20, 0); // add a short pause after the consonant

	if (flags & 16)
		return len;
	return 0;
}

static void SmoothSpect(EspeakProcessorContext* epContext)
{
	// Limit the rate of frequence change of formants, to reduce chirping

	intptr_t *q;
	frame_t *frame;
	frame_t *frame2;
	frame_t *frame1;
	frame_t *frame_centre;
	int ix;
	int len;
	int pk;
	bool modified;
	int allowed;
	int diff;

	if (epContext->syllable_start == epContext->syllable_end)
		return;

	if ((epContext->syllable_centre < 0) || (epContext->syllable_centre == epContext->syllable_start)) {
		epContext->syllable_start = epContext->syllable_end;
		return;
	}

	q = epContext->wcmdq[epContext->syllable_centre];
	frame_centre = (frame_t *)q[2];

	// backwards
	ix = epContext->syllable_centre -1;
	frame = frame2 = frame_centre;
	for (;;) {
		if (ix < 0) ix = N_WCMDQ-1;
	    q = epContext->wcmdq[ix];

		if (q[0] == WCMD_PAUSE || q[0] == WCMD_WAVE)
			break;

		if (q[0] <= WCMD_SPECT2) {
			len = q[1] & 0xffff;

			frame1 = (frame_t *)q[3];
			if (frame1 == frame) {
				q[3] = (intptr_t)frame2;
				frame1 = frame2;
			} else
				break; // doesn't follow on from previous frame

			frame = frame2 = (frame_t *)q[2];
			modified = false;

			if (frame->frflags & FRFLAG_BREAK)
				break;

			if (frame->frflags & FRFLAG_FORMANT_RATE)
				len = (len * 12)/10; // allow slightly greater rate of change for this frame (was 12/10)

			for (pk = 0; pk < 6; pk++) {
				int f1, f2;

				if ((frame->frflags & FRFLAG_BREAK_LF) && (pk < 3))
					continue;

				f1 = frame1->ffreq[pk];
				f2 = frame->ffreq[pk];

				// backwards
				if ((diff = f2 - f1) > 0)
					allowed = f1*2 + f2;
				else
					allowed = f1 + f2*2;

				// the allowed change is specified as percentage (%*10) of the frequency
				// take "frequency" as 1/3 from the lower freq
				allowed = (allowed * epContext->formant_rate[pk])/3000;
				allowed = (allowed * len)/256;

				if (diff > allowed) {
					if (modified == false) {
						frame2 = CopyFrame(frame, 0);
						modified = true;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] + allowed;
					q[2] = (intptr_t)frame2;
				} else if (diff < -allowed) {
					if (modified == false) {
						frame2 = CopyFrame(frame, 0);
						modified = true;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] - allowed;
					q[2] = (intptr_t)frame2;
				}
			}
		}

		if (ix == epContext->syllable_start)
			break;
		ix--;
	}

	// forwards
	ix = epContext->syllable_centre;

	frame = NULL;
	for (;;) {
		q = epContext->wcmdq[ix];

		if (q[0] == WCMD_PAUSE || q[0] == WCMD_WAVE)
			break;

		if (q[0] <= WCMD_SPECT2) {
			len = q[1] & 0xffff;

			frame1 = (frame_t *)q[2];
			if (frame != NULL) {
				if (frame1 == frame) {
					q[2] = (intptr_t)frame2;
					frame1 = frame2;
				} else
					break; // doesn't follow on from previous frame
			}

			frame = frame2 = (frame_t *)q[3];
			modified = false;

			if (frame1->frflags & FRFLAG_BREAK)
				break;

			if (frame1->frflags & FRFLAG_FORMANT_RATE)
				len = (len *6)/5; // allow slightly greater rate of change for this frame

			for (pk = 0; pk < 6; pk++) {
				int f1, f2;
				f1 = frame1->ffreq[pk];
				f2 = frame->ffreq[pk];

				// forwards
				if ((diff = f2 - f1) > 0)
					allowed = f1*2 + f2;
				else
					allowed = f1 + f2*2;
				allowed = (allowed * epContext->formant_rate[pk])/3000;
				allowed = (allowed * len)/256;

				if (diff > allowed) {
					if (modified == false) {
						frame2 = CopyFrame(frame, 0);
						modified = true;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] + allowed;
					q[3] = (intptr_t)frame2;
				} else if (diff < -allowed) {
					if (modified == false) {
						frame2 = CopyFrame(frame, 0);
						modified = true;
					}
					frame2->ffreq[pk] = frame1->ffreq[pk] - allowed;
					q[3] = (intptr_t)frame2;
				}
			}
		}

		ix++;
		if (ix >= N_WCMDQ) ix = 0;
		if (ix == epContext->syllable_end)
			break;
	}

	epContext->syllable_start = epContext->syllable_end;
}

static void StartSyllable(EspeakProcessorContext* epContext)
{
	// start of syllable, if not already started
	if (epContext->syllable_end == epContext->syllable_start)
		epContext->syllable_end = epContext->wcmdq_tail;
}

int DoSpect2(EspeakProcessorContext* epContext, PHONEME_TAB *this_ph, int which, FMT_PARAMS *fmt_params,  PHONEME_LIST *plist, int modulation)
{
	// which:  0 not a vowel, 1  start of vowel,   2 body and end of vowel
	// length_mod: 256 = 100%
	// modulation: -1 = don't write to wcmdq

	int n_frames;
	frameref_t *frames;
	int frameix;
	frame_t *frame1;
	frame_t *frame2;
	intptr_t *q;
	int len;
	int length_mod;
	int length_sum;
	int length_min;
	int total_len = 0;
	static int wave_flag = 0;
	int wcmd_spect = WCMD_SPECT;
	int frame_lengths[N_SEQ_FRAMES];

	if (fmt_params->fmt_addr == 0)
		return 0;

	length_mod = plist->length;
	if (length_mod == 0) length_mod = 256;

	length_min = (epContext->samplerate/70); // greater than one cycle at low pitch (Hz)
	if (which == 2) {
		if ((epContext->translator->langopts.param[LOPT_LONG_VOWEL_THRESHOLD] > 0) && ((this_ph->std_length >= epContext->translator->langopts.param[LOPT_LONG_VOWEL_THRESHOLD]) || (plist->synthflags & SFLAG_LENGTHEN) || (this_ph->phflags & phLONG)))
			length_min *= 2; // ensure long vowels are longer
	}

	if (which == 1) {
		// limit the shortening of sonorants before shortened (eg. unstressed vowels)
		if ((this_ph->type == phLIQUID) || (plist[-1].type == phLIQUID) || (plist[-1].type == phNASAL)) {
			if (length_mod < (len = epContext->translator->langopts.param[LOPT_SONORANT_MIN]))
				length_mod = len;
		}
	}

	epContext->modn_flags = 0;
	frames = LookupSpect(epContext, this_ph, which, fmt_params, &n_frames, plist);
	if (frames == NULL)
		return 0; // not found

	if (fmt_params->fmt_amp != epContext->fmt_amplitude) {
		// an amplitude adjustment is specified for this sequence
		q = epContext->wcmdq[epContext->wcmdq_tail];
		q[0] = WCMD_FMT_AMPLITUDE;
		q[1] = epContext->fmt_amplitude = fmt_params->fmt_amp;
		WcmdqInc(epContext);
	}

	frame1 = frames[0].frame;
	if (epContext->voice->klattv[0])
		wcmd_spect = WCMD_KLATT;

	if (fmt_params->wav_addr == 0) {
		if (wave_flag) {
			// cancel any wavefile that was playing previously
			wcmd_spect = WCMD_SPECT2;
			if (epContext->voice->klattv[0])
				wcmd_spect = WCMD_KLATT2;
			wave_flag = 0;
		} else {
			wcmd_spect = WCMD_SPECT;
			if (epContext->voice->klattv[0])
				wcmd_spect = WCMD_KLATT;
		}
	}

	if (epContext->last_frame != NULL) {
		if (((epContext->last_frame->length < 2) || (epContext->last_frame->frflags & FRFLAG_VOWEL_CENTRE))
		    && !(epContext->last_frame->frflags & FRFLAG_BREAK)) {
			// last frame of previous sequence was zero-length, replace with first of this sequence
			epContext->wcmdq[epContext->last_wcmdq][3] = (intptr_t)frame1;

			if (epContext->last_frame->frflags & FRFLAG_BREAK_LF) {
				// but flag indicates keep HF peaks in last segment
				frame_t *fr;
				fr = CopyFrame(frame1, 1);
				for (int ix = 3; ix < 8; ix++) {
					if (ix < 7)
						fr->ffreq[ix] = epContext->last_frame->ffreq[ix];
					fr->fheight[ix] = epContext->last_frame->fheight[ix];
				}
				epContext->wcmdq[epContext->last_wcmdq][3] = (intptr_t)fr;
			}
		}
	}

	if ((this_ph->type == phVOWEL) && (which == 2)) {
		SmoothSpect(epContext); // process previous syllable

		// remember the point in the output queue of the centre of the vowel
		epContext->syllable_centre = epContext->wcmdq_tail;
	}

	length_sum = 0;
	for (frameix = 1; frameix < n_frames; frameix++) {
		int length_factor = length_mod;
		if (frames[frameix-1].frflags & FRFLAG_LEN_MOD) // reduce effect of length mod
			length_factor = (length_mod*(256-epContext->speed.lenmod_factor) + 256*epContext->speed.lenmod_factor)/256;
		else if (frames[frameix-1].frflags & FRFLAG_LEN_MOD2) // reduce effect of length mod, used for the start of a vowel
			length_factor = (length_mod*(256-epContext->speed.lenmod2_factor) + 256*epContext->speed.lenmod2_factor)/256;

		int frame_length = frames[frameix-1].length;
		len = (frame_length * epContext->samplerate)/1000;
		len = (len * length_factor)/256;
		length_sum += len;
		frame_lengths[frameix] = len;
	}

	if ((length_sum > 0) && (length_sum < length_min)) {
		// lengthen, so that the sequence is greater than one cycle at low pitch
		for (frameix = 1; frameix < n_frames; frameix++)
			frame_lengths[frameix] = (frame_lengths[frameix] * length_min) / length_sum;
	}

	for (frameix = 1; frameix < n_frames; frameix++) {
		frame2 = frames[frameix].frame;

		if ((fmt_params->wav_addr != 0) && ((frame1->frflags & FRFLAG_DEFER_WAV) == 0)) {
			// there is a wave file to play along with this synthesis
			epContext->seq_len_adjust = 0;

			int wavefile_amp;
			if (fmt_params->wav_amp == 0)
				wavefile_amp = 32;
			else
				wavefile_amp = (fmt_params->wav_amp * 32)/100;

			DoSample2(epContext, fmt_params->wav_addr, which+0x100, 0, fmt_params->fmt_control, 0, wavefile_amp);
			wave_flag = 1;
			fmt_params->wav_addr = 0;
		}

		if (modulation >= 0) {
			if (frame1->frflags & FRFLAG_MODULATE)
				modulation = 6;
			if ((frameix == n_frames-1) && (epContext->modn_flags & 0xf00))
				modulation |= epContext->modn_flags; // before or after a glottal stop
		}

		len = frame_lengths[frameix];
		epContext->pitch_length += len;
		epContext->amp_length += len;

		if (len == 0) {
			epContext->last_frame = NULL;
			frame1 = frame2;
		} else {
			epContext->last_wcmdq = epContext->wcmdq_tail;

			if (modulation >= 0) {
				q = epContext->wcmdq[epContext->wcmdq_tail];
				q[0] = wcmd_spect;
				q[1] = len + (modulation << 16);
				q[2] = (intptr_t)frame1;
				q[3] = (intptr_t)frame2;

				WcmdqInc(epContext);
			}
			epContext->last_frame = frame1 = frame2;
			total_len += len;
		}
	}

	if ((which != 1) && (epContext->fmt_amplitude != 0)) {
		q = epContext->wcmdq[epContext->wcmdq_tail];
		q[0] = WCMD_FMT_AMPLITUDE;
		q[1] = epContext->fmt_amplitude = 0;
		WcmdqInc(epContext);
	}

	return total_len;
}

void DoMarker(EspeakProcessorContext* epContext, int type, int char_posn, int length, int value)
{
	// This could be used to return an index to the word currently being spoken
	// Type 1=word, 2=sentence, 3=named marker, 4=play audio, 5=end

	if (WcmdqFree(epContext) > 5) {
		epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_MARKER + (type << 8);
		epContext->wcmdq[epContext->wcmdq_tail][1] = (char_posn & 0xffffff) | (length << 24);
		epContext->wcmdq[epContext->wcmdq_tail][2] = value;
		WcmdqInc(epContext);
	}
}

void DoPhonemeMarker(EspeakProcessorContext* epContext, int type, int char_posn, int length, char *name)
{
	// This could be used to return an index to the word currently being spoken
	// Type 7=phoneme

	if (WcmdqFree(epContext) > 5) {
		epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_MARKER + (type << 8);
		epContext->wcmdq[epContext->wcmdq_tail][1] = (char_posn & 0xffffff) | (length << 24);
		memcpy(&epContext->wcmdq[epContext->wcmdq_tail][2], name, 8); // up to 8 bytes of UTF8 characters
		WcmdqInc(epContext);
	}
}

#if USE_LIBSONIC
void DoSonicSpeed(EspeakProcessorContext* epContext, int value)
{
	// value, multiplier * 1024
	epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_SONIC_SPEED;
	epContext->wcmdq[epContext->wcmdq_tail][1] = value;
	WcmdqInc(epContext);
}
#endif

espeak_ng_STATUS DoVoiceChange(EspeakProcessorContext* epContext, voice_t *v)
{
	// allocate memory for a copy of the voice data, and free it in wavegenfill()
	voice_t *v2;
	if ((v2 = (voice_t *)malloc(sizeof(voice_t))) == NULL)
		return ENOMEM;
	memcpy(v2, v, sizeof(voice_t));
	epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_VOICE;
	epContext->wcmdq[epContext->wcmdq_tail][2] = (intptr_t)v2;
	WcmdqInc(epContext);
	return ENS_OK;
}

void DoEmbedded(EspeakProcessorContext* epContext, int *embix, int sourceix)
{
	// There were embedded commands in the text at this point
	unsigned int word; // bit 7=last command for this word, bits 5,6 sign, bits 0-4 command

	do {
		unsigned int value;
		int command;

		word = epContext->embedded_list[*embix];
		value = word >> 8;
		command = word & 0x7f;

		if (command == 0)
			return; // error

		(*embix)++;

		switch (command & 0x1f)
		{
		case EMBED_S: // speed
			SetEmbedded(epContext, (command & 0x60) + EMBED_S2, value); // adjusts embedded_value[EMBED_S2]
			SetSpeed(epContext, 2);
			break;
		case EMBED_I: // play dynamically loaded wav data (sound icon)
			if ((int)value < epContext->n_soundicon_tab) {
				if (epContext->soundicon_tab[value].length != 0) {
					DoPause(epContext, 10, 0); // ensure a break in the speech
					epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_WAVE;
					epContext->wcmdq[epContext->wcmdq_tail][1] = epContext->soundicon_tab[value].length;
					epContext->wcmdq[epContext->wcmdq_tail][2] = (intptr_t)epContext->soundicon_tab[value].data + 44; // skip WAV header
					epContext->wcmdq[epContext->wcmdq_tail][3] = 0x1500; // 16 bit data, amp=21
					WcmdqInc(epContext);
				}
			}
			break;
		case EMBED_M: // named marker
			DoMarker(epContext, espeakEVENT_MARK, (sourceix & 0x7ff) + epContext->clause_start_char, 0, value);
			break;
		case EMBED_U: // play sound
			DoMarker(epContext, espeakEVENT_PLAY, epContext->count_characters+1, 0, value); // always occurs at end of clause
			break;
		default:
			DoPause(epContext, 10, 0); // ensure a break in the speech
			epContext->wcmdq[epContext->wcmdq_tail][0] = WCMD_EMBEDDED;
			epContext->wcmdq[epContext->wcmdq_tail][1] = command;
			epContext->wcmdq[epContext->wcmdq_tail][2] = value;
			WcmdqInc(epContext);
			break;
		}
	} while ((word & 0x80) == 0);
}

int Generate(EspeakProcessorContext* epContext, PHONEME_LIST *phoneme_list, int *n_ph, bool resume)
{
	static int ix;
	static int embedded_ix;
	static int word_count;
	PHONEME_LIST *p;
	bool released;
	int stress;
	int modulation;
	bool  pre_voiced;
	int free_min;
	int value;
	const unsigned char *pitch_env = NULL;
	const unsigned char *amp_env;
	PHONEME_TAB *ph;
	int use_ipa = 0;
	int vowelstart_prev;
	char phoneme_name[16];
	static int sourceix = 0;

	PHONEME_DATA phdata;
	PHONEME_DATA phdata_prev;
	PHONEME_DATA phdata_next;
	PHONEME_DATA phdata_tone;
	FMT_PARAMS fmtp;
	static WORD_PH_DATA worddata;

	if (epContext->option_phoneme_events & espeakINITIALIZE_PHONEME_IPA)
		use_ipa = 1;

#if USE_MBROLA
	if (mbrola_name[0] != 0)
		return MbrolaGenerate(phoneme_list, n_ph, resume);
#endif

	if (resume == false) {
		ix = 1;
		embedded_ix = 0;
		word_count = 0;
		epContext->pitch_length = 0;
		epContext->amp_length = 0;
		epContext->last_frame = NULL;
		epContext->last_wcmdq = -1;
		epContext->syllable_start = epContext->wcmdq_tail;
		epContext->syllable_end = epContext->wcmdq_tail;
		epContext->syllable_centre = -1;
		epContext->last_pitch_cmd = -1;
		memset(&worddata, 0, sizeof(worddata));
		DoPause(epContext, 0, 0); // isolate from the previous clause
	}

	while ((ix < (*n_ph)) && (ix < N_PHONEME_LIST-2)) {
		p = &phoneme_list[ix];

	    p->ph = p->ph + (epContext->bends.rotatePhonemes); // BEND TODO: actually rotate. what if we go over the end?
	    if (ix > 0 && epContext->bends.stickChance * RAND_MAX > (float)rand())
	    {
	        p->ph = phoneme_list[ix-1].ph;
	    }
	    if(epContext->output_hooks && epContext->output_hooks->outputPhoSymbol)
		{
			char buf[30];
			int dummy=0;
			//WritePhMnemonic(buf, p->ph, p, 0, &dummy);
			WritePhMnemonicWithStress(epContext, buf, p->ph, p, 0, &dummy);

			DoPhonemeAlignment(epContext, strdup(buf),p->type);
		}

		if (p->type == phPAUSE)
			free_min = 10;
		else if (p->type != phVOWEL)
			free_min = 15; // we need less Q space for non-vowels, and we need to generate phonemes after a vowel so that the epContext->pitch_length is filled in
		else
			free_min = MIN_WCMDQ;

		if (WcmdqFree(epContext) <= free_min)
			return 1; // wait

		PHONEME_LIST *prev;
		PHONEME_LIST *next;
		PHONEME_LIST *next2;

		prev = &phoneme_list[ix-1];
		next = &phoneme_list[ix+1];
		next2 = &phoneme_list[ix+2];

		if (p->synthflags & SFLAG_EMBEDDED)
			DoEmbedded(epContext, &embedded_ix, p->sourceix);

		if (p->newword) {
			if (((p->type == phVOWEL) && (epContext->translator->langopts.param[LOPT_WORD_MERGE] & 1)) ||
			    (p->ph->phflags & phNOPAUSE)) {
			} else
				epContext->last_frame = NULL;

			sourceix = (p->sourceix & 0x7ff) + epContext->clause_start_char;

			if (p->newword & PHLIST_START_OF_SENTENCE)
				DoMarker(epContext, espeakEVENT_SENTENCE, sourceix, 0, epContext->count_sentences); // start of sentence

			if (p->newword & PHLIST_START_OF_WORD)
				DoMarker(epContext, espeakEVENT_WORD, sourceix, p->sourceix >> 11, epContext->clause_start_word + word_count++); // NOTE, this count doesn't include multiple-word pronunciations in *_list. eg (of a)
		}

		EndAmplitude(epContext);

		if ((p->prepause > 0) && !(p->ph->phflags & phPREVOICE))
			DoPause(epContext, p->prepause, 1);

		bool done_phoneme_marker = false;
		if (epContext->option_phoneme_events && (p->ph->code != phonEND_WORD)) {
			if ((p->type == phVOWEL) && (prev->type == phLIQUID || prev->type == phNASAL)) {
				// For vowels following a liquid or nasal, do the phoneme event after the vowel-start
			} else {
				//WritePhMnemonic(phoneme_name, p->ph, p, use_ipa, NULL);
				WritePhMnemonicWithStress(epContext, phoneme_name, p->ph, p, use_ipa, NULL);

				DoPhonemeMarker(epContext, espeakEVENT_PHONEME, sourceix, 0, phoneme_name);
				done_phoneme_marker = true;
			}
		}

	    // BEND TODO: something with pitch

		switch (p->type)
		{
		case phPAUSE:
			DoPause(epContext, p->length, 0);
			p->std_length = p->ph->std_length;
			break;
		case phSTOP:
			released = false;
			ph = p->ph;
			if (next->type == phVOWEL)
				released = true;
			else if (!next->newword) {
				if (next->type == phLIQUID) released = true;
			}
			if (released == false)
				p->synthflags |= SFLAG_NEXT_PAUSE;

			if (ph->phflags & phPREVOICE) {
				// a period of voicing before the release
				memset(&fmtp, 0, sizeof(fmtp));
				InterpretPhoneme(epContext, NULL, 0x01, p, phoneme_list, &phdata, &worddata);
				fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
				fmtp.fmt_amp = phdata.sound_param[pd_FMT];

				if (epContext->last_pitch_cmd < 0) {
					DoAmplitude(epContext, next->amp, NULL);
					DoPitch(epContext, envelope_data[p->env], next->pitch1, next->pitch2);
				}

				DoSpect2(epContext, ph, 0, &fmtp, p, 0);
			}

			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);
			phdata.pd_control |= pd_DONTLENGTHEN;
			DoSample3(epContext, &phdata, 0, 0);
			break;
		case phFRICATIVE:
			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);

			if (p->synthflags & SFLAG_LENGTHEN)
				DoSample3(epContext, &phdata, p->length, 0); // play it twice for [s:] etc.
			DoSample3(epContext, &phdata, p->length, 0);
			break;
		case phVSTOP:
			ph = p->ph;
			memset(&fmtp, 0, sizeof(fmtp));
			fmtp.fmt_control = pd_DONTLENGTHEN;

			pre_voiced = false;
			if (next->type == phVOWEL) {
				DoAmplitude(epContext, p->amp, NULL);
				DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
				pre_voiced = true;
			} else if ((next->type == phLIQUID) && !next->newword) {
				DoAmplitude(epContext, next->amp, NULL);
				DoPitch(epContext, envelope_data[next->env], next->pitch1, next->pitch2);
				pre_voiced = true;
			} else {
				if (epContext->last_pitch_cmd < 0) {
					DoAmplitude(epContext, next->amp, NULL);
					DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
				}
			}

			if ((prev->type == phVOWEL) || (ph->phflags & phPREVOICE)) {
				// a period of voicing before the release
				InterpretPhoneme(epContext, NULL, 0x01, p, phoneme_list, &phdata, &worddata);
				fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
				fmtp.fmt_amp = phdata.sound_param[pd_FMT];

				DoSpect2(epContext, ph, 0, &fmtp, p, 0);
				if (p->synthflags & SFLAG_LENGTHEN) {
					DoPause(epContext, 25, 1);
					DoSpect2(epContext, ph, 0, &fmtp, p, 0);
				}
			} else {
				if (p->synthflags & SFLAG_LENGTHEN)
					DoPause(epContext, 50, 0);
			}

			if (pre_voiced) {
				// followed by a vowel, or liquid + vowel
				StartSyllable(epContext);
			} else
				p->synthflags |= SFLAG_NEXT_PAUSE;
			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);
			fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			fmtp.fmt_amp = phdata.sound_param[pd_FMT];
			fmtp.wav_addr = phdata.sound_addr[pd_ADDWAV];
			fmtp.wav_amp = phdata.sound_param[pd_ADDWAV];
			DoSpect2(epContext, ph, 0, &fmtp, p, 0);

			if ((p->newword == 0) && (next2->newword == 0)) {
				if (next->type == phVFRICATIVE)
					DoPause(epContext, 20, 0);
				if (next->type == phFRICATIVE)
					DoPause(epContext, 12, 0);
			}
			break;
		case phVFRICATIVE:
			if (next->type == phVOWEL) {
				DoAmplitude(epContext, p->amp, NULL);
				DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
			} else if (next->type == phLIQUID) {
				DoAmplitude(epContext, next->amp, NULL);
				DoPitch(epContext, envelope_data[next->env], next->pitch1, next->pitch2);
			} else {
				if (epContext->last_pitch_cmd < 0) {
					DoAmplitude(epContext, p->amp, NULL);
					DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
				}
			}

			if ((next->type == phVOWEL) || ((next->type == phLIQUID) && (next->newword == 0))) // ?? test 14.Aug.2007
				StartSyllable(epContext);
			else
				p->synthflags |= SFLAG_NEXT_PAUSE;
			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);
			memset(&fmtp, 0, sizeof(fmtp));
			fmtp.std_length = phdata.pd_param[i_SET_LENGTH]*2;
			fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			fmtp.fmt_amp = phdata.sound_param[pd_FMT];
			fmtp.wav_addr = phdata.sound_addr[pd_ADDWAV];
			fmtp.wav_amp = phdata.sound_param[pd_ADDWAV];

			if (p->synthflags & SFLAG_LENGTHEN)
				DoSpect2(epContext, p->ph, 0, &fmtp, p, 0);
			DoSpect2(epContext, p->ph, 0, &fmtp, p, 0);
			break;
		case phNASAL:
			memset(&fmtp, 0, sizeof(fmtp));
			if (!(p->synthflags & SFLAG_SEQCONTINUE)) {
				DoAmplitude(epContext, p->amp, NULL);
				DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
			}

			if (prev->type == phNASAL)
				epContext->last_frame = NULL;

			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);
			fmtp.std_length = phdata.pd_param[i_SET_LENGTH]*2;
			fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			fmtp.fmt_amp = phdata.sound_param[pd_FMT];

			if (next->type == phVOWEL) {
				StartSyllable(epContext);
				DoSpect2(epContext, p->ph, 0, &fmtp, p, 0);
			} else if (prev->type == phVOWEL && (p->synthflags & SFLAG_SEQCONTINUE))
				DoSpect2(epContext, p->ph, 0, &fmtp, p, 0);
			else {
				epContext->last_frame = NULL; // only for nasal ?
				DoSpect2(epContext, p->ph, 0, &fmtp, p, 0);
				epContext->last_frame = NULL;
			}

			break;
		case phLIQUID:
			memset(&fmtp, 0, sizeof(fmtp));
			modulation = 0;
			if (p->ph->phflags & phTRILL)
				modulation = 5;

			if (!(p->synthflags & SFLAG_SEQCONTINUE)) {
				DoAmplitude(epContext, p->amp, NULL);
				DoPitch(epContext, envelope_data[p->env], p->pitch1, p->pitch2);
			}

			if (prev->type == phNASAL)
				epContext->last_frame = NULL;

			if (next->type == phVOWEL)
				StartSyllable(epContext);
			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);

			if ((value = (phdata.pd_param[i_PAUSE_BEFORE] - p->prepause)) > 0)
				DoPause(epContext, value, 1);
			fmtp.std_length = phdata.pd_param[i_SET_LENGTH]*2;
			fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			fmtp.fmt_amp = phdata.sound_param[pd_FMT];
			fmtp.wav_addr = phdata.sound_addr[pd_ADDWAV];
			fmtp.wav_amp = phdata.sound_param[pd_ADDWAV];
			DoSpect2(epContext, p->ph, 0, &fmtp, p, modulation);
			break;
		case phVOWEL:
			ph = p->ph;
			stress = p->stresslevel & 0xf;

			memset(&fmtp, 0, sizeof(fmtp));

			InterpretPhoneme(epContext, NULL, 0, p, phoneme_list, &phdata, &worddata);
			fmtp.std_length = phdata.pd_param[i_SET_LENGTH] * 2;
			vowelstart_prev = 0;

			if (((fmtp.fmt_addr = phdata.sound_addr[pd_VWLSTART]) != 0) && ((phdata.pd_control & pd_FORNEXTPH) == 0)) {
				// a vowel start has been specified by the Vowel program
				fmtp.fmt_length = phdata.sound_param[pd_VWLSTART];
			} else if (prev->type != phPAUSE) {
				// check the previous phoneme
				InterpretPhoneme(epContext, NULL, 0, prev, phoneme_list, &phdata_prev, NULL);
				if (((fmtp.fmt_addr = phdata_prev.sound_addr[pd_VWLSTART]) != 0) && (phdata_prev.pd_control & pd_FORNEXTPH)) {
					// a vowel start has been specified by the previous phoneme
					vowelstart_prev = 1;
					fmtp.fmt2_lenadj = phdata_prev.sound_param[pd_VWLSTART];
				}
				fmtp.transition0 = phdata_prev.vowel_transition[0];
				fmtp.transition1 = phdata_prev.vowel_transition[1];
			}

			if (fmtp.fmt_addr == 0) {
				// use the default start for this vowel
				fmtp.use_vowelin = 1;
				fmtp.fmt_control = 1;
				fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			}

			fmtp.fmt_amp = phdata.sound_param[pd_FMT];

			pitch_env = envelope_data[p->env];
			amp_env = NULL;
			if (p->tone_ph != 0) {
				InterpretPhoneme2(epContext, p->tone_ph, &phdata_tone);
				pitch_env = GetEnvelope(epContext, phdata_tone.pitch_env);
				if (phdata_tone.amp_env > 0)
					amp_env = GetEnvelope(epContext, phdata_tone.amp_env);
			}

			StartSyllable(epContext);

			modulation = 2;
			if (stress <= 1)
				modulation = 1; // 16ths
			else if (stress >= 7)
				modulation = 3;

			if (prev->type == phVSTOP || prev->type == phVFRICATIVE) {
				DoAmplitude(epContext, p->amp, amp_env);
				DoPitch(epContext, pitch_env, p->pitch1, p->pitch2); // don't use prevocalic rising tone
				DoSpect2(epContext, ph, 1, &fmtp, p, modulation);
			} else if (prev->type == phLIQUID || prev->type == phNASAL) {
				DoAmplitude(epContext, p->amp, amp_env);
				DoSpect2(epContext, ph, 1, &fmtp, p, modulation); // continue with pre-vocalic rising tone
				DoPitch(epContext, pitch_env, p->pitch1, p->pitch2);
			} else if (vowelstart_prev) {
				// VowelStart from the previous phoneme, but not phLIQUID or phNASAL
				DoPitch(epContext, envelope_data[PITCHrise], p->pitch2 - 15, p->pitch2);
				DoAmplitude(epContext, p->amp-1, amp_env);
				DoSpect2(epContext, ph, 1, &fmtp, p, modulation); // continue with pre-vocalic rising tone
				DoPitch(epContext, pitch_env, p->pitch1, p->pitch2);
			} else {
				if (!(p->synthflags & SFLAG_SEQCONTINUE)) {
					DoAmplitude(epContext, p->amp, amp_env);
					DoPitch(epContext, pitch_env, p->pitch1, p->pitch2);
				}

				DoSpect2(epContext, ph, 1, &fmtp, p, modulation);
			}

			if ((epContext->option_phoneme_events) && (done_phoneme_marker == false)) {
				//WritePhMnemonic(phoneme_name, p->ph, p, use_ipa, NULL);
				WritePhMnemonicWithStress(epContext, phoneme_name, p->ph, p, use_ipa, NULL);

				DoPhonemeMarker(epContext, espeakEVENT_PHONEME, sourceix, 0, phoneme_name);
			}

			fmtp.fmt_addr = phdata.sound_addr[pd_FMT];
			fmtp.fmt_amp = phdata.sound_param[pd_FMT];
			fmtp.transition0 = 0;
			fmtp.transition1 = 0;

			if ((fmtp.fmt2_addr = phdata.sound_addr[pd_VWLEND]) != 0)
				fmtp.fmt2_lenadj = phdata.sound_param[pd_VWLEND];
			else if (next->type != phPAUSE) {
				fmtp.fmt2_lenadj = 0;
				InterpretPhoneme(epContext, NULL, 0, next, phoneme_list, &phdata_next, NULL);

				fmtp.use_vowelin = 1;
				fmtp.transition0 = phdata_next.vowel_transition[2]; // always do vowel_transition, even if ph_VWLEND ??  consider [N]
				fmtp.transition1 = phdata_next.vowel_transition[3];

				if ((fmtp.fmt2_addr = phdata_next.sound_addr[pd_VWLEND]) != 0)
					fmtp.fmt2_lenadj = phdata_next.sound_param[pd_VWLEND];
			}

			DoSpect2(epContext, ph, 2, &fmtp, p, modulation);
			break;
		}

	    if (epContext->bends.debugPrintEverything)
	    {
	        printf("p: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i | %u %u %u %u %u %u %u %u %u\n",
                p->synthflags, p->phcode, p->stresslevel, p->sourceix, p->wordstress, p->tone_ph, p->length, p->env, p->type, p->prepause, p->amp, p->newword, p->pitch1, p->pitch2, p->std_length, p->phontab_addr, p->sound_param,
                p->ph->mnemonic, p->ph->phflags, p->ph->program, p->ph->code, p->ph->type, p->ph->start_type, p->ph->end_type, p->ph->std_length, p->ph->length_mod);
	    }

		ix++;
	}
	EndPitch(epContext, 1);
	if (*n_ph > 0) {
		DoMarker(epContext, espeakEVENT_END, epContext->count_characters, 0, epContext->count_sentences); // end of clause
		*n_ph = 0;
	}

	return 0; // finished the phoneme list
}

int SpeakNextClause(EspeakProcessorContext* epContext, int control)
{
	// Speak text from memory (text_in)
	// control 0: start
	//    text_in is set

	// The other calls have text_in = NULL
	// control 1: speak next text
	//         2: stop

	int clause_tone;
	char *voice_change;

	if (control == 2) {
		// stop speaking
		epContext->n_phoneme_list = 0;
		WcmdqStop(epContext);

		return 0;
	}

	if (text_decoder_eof(epContext->p_decoder)) {
		epContext->skipping_text = false;
		return 0;
	}

	SelectPhonemeTable(epContext, epContext->voice->phoneme_tab_ix);

	// read the next clause from the input text file, translate it, and generate
	// entries in the wavegen command queue
	TranslateClause(epContext, epContext->translator, &clause_tone, &voice_change);

	CalcPitches(epContext, epContext->translator, clause_tone);
	CalcLengths(epContext, epContext->translator);

	if ((epContext->option_phonemes & 0xf) || (phoneme_callback != NULL)) {
		const char *phon_out;
		phon_out = GetTranslatedPhonemeString(epContext, epContext->option_phonemes);
		if (epContext->option_phonemes & 0xf)
			fprintf(epContext->f_trans, "%s\n", phon_out);
		if (phoneme_callback != NULL)
			phoneme_callback(phon_out);
	}

	if (epContext->skipping_text) {
		epContext->n_phoneme_list = 0;
		return 1;
	}

	Generate(epContext, epContext->phoneme_list, &epContext->n_phoneme_list, 0);

	if (voice_change != NULL) {
		// voice change at the end of the clause (i.e. clause was terminated by a voice change)
		epContext->new_voice = LoadVoiceVariant(epContext, voice_change, 0); // add a Voice instruction to wavegen at the end of the clause
	}

	if (epContext->new_voice) {
		// finished the current clause, now change the voice if there was an embedded
		// change voice command at the end of it (i.e. clause was broken at the change voice command)
		DoVoiceChange(epContext, epContext->voice);
		epContext->new_voice = NULL;
	}

	return 1;
}

#pragma GCC visibility push(default)
ESPEAK_API void espeak_SetPhonemeCallback(EspeakProcessorContext* epContext, int (*PhonemeCallback)(const char *))
{
	phoneme_callback = PhonemeCallback;
}
#pragma GCC visibility pop
