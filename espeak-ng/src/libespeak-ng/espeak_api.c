/* An implementation of the eSpeak API using the espeak-ng API.
 *
 * Copyright (C) 2005 to 2013 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2016 Reece H. Dunn
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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <espeak-ng/encoding.h>

#include "compiledict.h"

#include "synthesize.h"           // for espeakINITIALIZE_PHONEME_IPA
#include "translate.h"            // for dictionary_name, option_phoneme_events

static espeak_ERROR status_to_espeak_error(espeak_ng_STATUS status)
{
	switch (status)
	{
	case ENS_OK:                     return EE_OK;
	case ENS_SPEECH_STOPPED:         return EE_OK;
	case ENS_VOICE_NOT_FOUND:        return EE_NOT_FOUND;
	case ENS_MBROLA_NOT_FOUND:       return EE_NOT_FOUND;
	case ENS_MBROLA_VOICE_NOT_FOUND: return EE_NOT_FOUND;
	case ENS_FIFO_BUFFER_FULL:       return EE_BUFFER_FULL;
	default:                         return EE_INTERNAL_ERROR;
	}
}

#pragma GCC visibility push(default)

ESPEAK_API int espeak_Initialize(EspeakProcessorContext* epContext, espeak_AUDIO_OUTPUT output_type, int buf_length, const char *path, int options)
{
	espeak_ng_InitializePath(epContext, path);
	espeak_ng_ERROR_CONTEXT context = NULL;
	espeak_ng_STATUS result = espeak_ng_Initialize(epContext, &context);
	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, context);
		espeak_ng_ClearErrorContext(&context);
		if ((options & espeakINITIALIZE_DONT_EXIT) == 0)
			exit(1);
	}

	switch (output_type)
	{
	case AUDIO_OUTPUT_PLAYBACK:
		espeak_ng_InitializeOutput(epContext, ENOUTPUT_MODE_SPEAK_AUDIO, buf_length, NULL);
		break;
	case AUDIO_OUTPUT_RETRIEVAL:
		espeak_ng_InitializeOutput(epContext, 0, buf_length, NULL);
		break;
	case AUDIO_OUTPUT_SYNCHRONOUS:
		espeak_ng_InitializeOutput(epContext, ENOUTPUT_MODE_SYNCHRONOUS, buf_length, NULL);
		break;
	case AUDIO_OUTPUT_SYNCH_PLAYBACK:
		espeak_ng_InitializeOutput(epContext, ENOUTPUT_MODE_SYNCHRONOUS | ENOUTPUT_MODE_SPEAK_AUDIO, buf_length, NULL);
		break;
	}

	epContext->option_phoneme_events = (options & (espeakINITIALIZE_PHONEME_EVENTS | espeakINITIALIZE_PHONEME_IPA));

	return espeak_ng_GetSampleRate(epContext);
}

ESPEAK_NG_API void
initEspeakContext(EspeakProcessorContext* epContext)
{
    epContext->agc = 256;

    epContext->namedata_ix = 0;
    epContext->n_namedata = 0;
    epContext->namedata = NULL;

    epContext->wvoice = NULL;

    epContext->option_harmonic1 = 10;
    epContext->flutter_amp = 64;

    epContext->general_amplitude = 60;
    epContext->consonant_amp = 26;
    epContext->samplerate = 0;;
    epContext->echo_amp = 0;

    epContext->echo_length = 0; // period (in sample\) to ensure completion of echo at the end of speech, set in WavegenSetEcho()

    epContext->hswitch = 0;

    epContext->nsamples = 0; // number to do
    epContext->modulation_type = 0;
    epContext->glottal_flag = 0;
    epContext->glottal_reduce = 0;

    epContext->amplitude_env = NULL;

    epContext->samplecount = 0; // number done
    epContext->samplecount_start = 0; // count at start of this segment
    epContext->end_wave = 0; // continue to end of wave cycle
    epContext->output_hooks = NULL;
    epContext->const_f0 = 0;

    // the queue of operations passed to wavegen from sythesize
    epContext->wcmdq_head = 0;
    epContext->wcmdq_tail = 0;

    #if USE_LIBSONIC
    epContext->sonicSpeedupStream = NULL;
    epContext->sonicSpeed = 1.0;
    #endif


  epContext->len_speeds[0] = 130;
  epContext->len_speeds[1] = 121;
  epContext->len_speeds[2] = 118;

  epContext->n_soundicon_tab = 0;

  epContext->outbuf = NULL;
  epContext->outbuf_size = 0;
  epContext->event_list = NULL;
  epContext->event_list_ix = 0;
  epContext->my_unique_identifier = 0;
  epContext->my_user_data = NULL;
  epContext->my_mode = ENOUTPUT_MODE_SYNCHRONOUS;
  epContext->out_samplerate = 0;
  epContext->voice_samplerate = 22050;
  epContext->err = ENS_OK;
  epContext->synth_callback = NULL;
  epContext->n_soundicon_tab = 0;

  epContext->n_tunes = 0;
  epContext->tunes = NULL;
  epContext->phoneme_index = NULL;
  epContext->phondata_ptr = NULL;
  epContext->wavefile_data = NULL;
  epContext->phoneme_tab_data = NULL;
  epContext->phoneme_tab_number = 0;

  epContext->n_phoneme_list = 0;
  epContext->fmt_amplitude = 0;
  epContext->new_voice = NULL;

  epContext->option_linelength = 0;
  epContext->option_punctuation = 0;
  epContext->max_clause_pause = 0;
  epContext->embedded_flag = 0;
  epContext->word_emphasis = 0;

  epContext->translator= NULL; // the main translator
  epContext->translator2= NULL; // secondary translator for certain words
  epContext->translator2_language[0] =  0 ;
  epContext->translator3 = NULL; // tertiary translator for certain words
  epContext->translator3_language[0] = 0 ;
  epContext->f_trans = NULL; // phoneme output text
  epContext->option_tone_flags = 0; // bit 8=emphasize allcaps, bit 9=emphasize penultimate stress
  epContext->option_phonemes = 0;
  epContext->option_phoneme_events = 0;
  epContext->option_endpause = 0; // suppress pause after end of text
  epContext->option_capitals = 0;
  epContext->option_punctuation = 0;
  epContext->option_sayas = 0;
  epContext->option_sayas2 = 0; // used in translate_clause()
  epContext->option_emphasis = 0; // 0=normal, 1=normal, 2=weak, 3=moderate, 4=strong
  epContext->option_ssml = 0;
  epContext->option_phoneme_input = 0; // allow [[phonemes]] in input
  epContext->option_wordgap = 0;

  epContext->tone_points[0] = 600;
  epContext->tone_points[1] = 170;
  epContext->tone_points[2] = 1200;
  epContext->tone_points[3] = 135;
  epContext->tone_points[4] = 2000;
  epContext->tone_points[5] = 110;
  epContext->tone_points[6] = 3000;
  epContext->tone_points[7] = 110;
  epContext->tone_points[8] = -1;
  epContext->tone_points[9] = 0;
  epContext->tone_points[10] = 0;
  epContext->tone_points[11] = 0;

  epContext->n_voices_list = 0;

  epContext->voice = &epContext->voicedata;

  epContext->wavemult_offset = 0;
  epContext->wavemult_max = 0;

    const unsigned char defaultWavemult[] = {
        0, 0, 0, 2, 3, 5, 8, 11, 14, 18, 22, 27, 32, 37, 43, 49, 55, 62, 69, 76, 83, 90, 98, 105, 113, 121, 128, 136, 144, 152, 159, 166, 174, 181, 188, 194, 201, 207, 213, 218, 224, 228, 233, 237, 240, 244, 246, 249, 251, 252, 253, 253, 253, 253, 252, 251, 249, 246, 244, 240, 237, 233, 228, 224, 218, 213, 207, 201, 194, 188, 181, 174, 166, 159, 152, 144, 136, 128, 121, 113, 105, 98, 90, 83, 76, 69, 62, 55, 49, 43, 37, 32, 27, 22, 18, 14, 11, 8, 5, 3, 2
    };

    memcpy (epContext->wavemult, defaultWavemult, 101 * sizeof(unsigned char));

    epContext->pluginBuffer = NULL;
    epContext->pluginBufferPosition = 0;
    epContext->pluginBufferSize = 0;
}


ESPEAK_API espeak_ERROR espeak_Synth(EspeakProcessorContext* epContext, const void *text, size_t size,
                                     unsigned int position,
                                     espeak_POSITION_TYPE position_type,
                                     unsigned int end_position, unsigned int flags,
                                     unsigned int *unique_identifier, void *user_data)
{
	return status_to_espeak_error(espeak_ng_Synthesize(epContext, text, size, position, position_type, end_position, flags, unique_identifier, user_data));
}

ESPEAK_API espeak_ERROR espeak_Synth_Mark(EspeakProcessorContext* epContext,
                                            const void *text, size_t size,
                                          const char *index_mark,
                                          unsigned int end_position,
                                          unsigned int flags,
                                          unsigned int *unique_identifier,
                                          void *user_data)
{
	return status_to_espeak_error(espeak_ng_SynthesizeMark(epContext, text, size, index_mark, end_position, flags, unique_identifier, user_data));
}

ESPEAK_API espeak_ERROR espeak_Key(EspeakProcessorContext* epContext, const char *key_name)
{
	return status_to_espeak_error(espeak_ng_SpeakKeyName(epContext, key_name));
}

ESPEAK_API espeak_ERROR espeak_Char(EspeakProcessorContext* epContext, wchar_t character)
{
	return status_to_espeak_error(espeak_ng_SpeakCharacter(epContext, character));
}

ESPEAK_API espeak_ERROR espeak_SetParameter(EspeakProcessorContext* epContext, espeak_PARAMETER parameter, int value, int relative)
{
	return status_to_espeak_error(espeak_ng_SetParameter(epContext, parameter, value, relative));
}

ESPEAK_API espeak_ERROR espeak_SetPunctuationList(EspeakProcessorContext* epContext, const wchar_t *punctlist)
{
	return status_to_espeak_error(espeak_ng_SetPunctuationList(epContext, punctlist));
}

ESPEAK_API espeak_ERROR espeak_SetVoiceByName(EspeakProcessorContext* epContext, const char *name)
{
	return status_to_espeak_error(espeak_ng_SetVoiceByName(epContext, name));
}

ESPEAK_API espeak_ERROR espeak_SetVoiceByFile(EspeakProcessorContext* epContext, const char *filename)
{
	return status_to_espeak_error(espeak_ng_SetVoiceByFile(epContext, filename));
}

ESPEAK_API espeak_ERROR espeak_SetVoiceByProperties(EspeakProcessorContext* epContext, espeak_VOICE *voice_selector)
{
	return status_to_espeak_error(espeak_ng_SetVoiceByProperties(epContext, voice_selector));
}

ESPEAK_API espeak_ERROR espeak_Cancel(EspeakProcessorContext* epContext)
{
	return status_to_espeak_error(espeak_ng_Cancel(epContext));
}

ESPEAK_API espeak_ERROR espeak_Synchronize(EspeakProcessorContext* epContext)
{
	return status_to_espeak_error(espeak_ng_Synchronize(epContext));
}

ESPEAK_API espeak_ERROR espeak_Terminate(EspeakProcessorContext* epContext)
{
	return status_to_espeak_error(espeak_ng_Terminate(epContext));
}

ESPEAK_API void espeak_CompileDictionary(EspeakProcessorContext* epContext, const char *path, FILE *log, int flags)
{
	espeak_ng_ERROR_CONTEXT context = NULL;
	espeak_ng_STATUS result = espeak_ng_CompileDictionary(epContext, path, epContext->dictionary_name, log, flags, &context);
	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, context);
		espeak_ng_ClearErrorContext(&context);
	}
}

#pragma GCC visibility pop
