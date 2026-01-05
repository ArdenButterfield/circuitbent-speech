/* eSpeak NG API.
 *
 * Copyright (C) 2015-2017 Reece H. Dunn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ESPEAK_NG_H
#define ESPEAK_NG_H

#include <espeak-ng/speak_lib.h>
#include <espeak-ng/common.h>
#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifdef LIBESPEAK_NG_EXPORT
#define ESPEAK_NG_API __declspec(dllexport)
#else
#define ESPEAK_NG_API __declspec(dllimport)
#endif
#else
#define ESPEAK_NG_API
#endif

#define ESPEAKNG_DEFAULT_VOICE "en"



typedef enum {
	ENGENDER_UNKNOWN = 0,
	ENGENDER_MALE = 1,
	ENGENDER_FEMALE = 2,
	ENGENDER_NEUTRAL = 3,
} espeak_ng_VOICE_GENDER;

/* eSpeak NG 1.49.0 */

typedef struct espeak_ng_ERROR_CONTEXT_ *espeak_ng_ERROR_CONTEXT;

ESPEAK_NG_API void
espeak_ng_ClearErrorContext(espeak_ng_ERROR_CONTEXT *context);

ESPEAK_NG_API void
espeak_ng_GetStatusCodeMessage(espeak_ng_STATUS status,
                               char *buffer,
                               size_t length);

ESPEAK_NG_API void
espeak_ng_PrintStatusCodeMessage(espeak_ng_STATUS status,
                                 FILE *out,
                                 espeak_ng_ERROR_CONTEXT context);

ESPEAK_NG_API void
espeak_ng_InitializePath(const char *path);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Initialize(espeak_ng_ERROR_CONTEXT *context);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_InitializeOutput(espeak_ng_OUTPUT_MODE output_mode,
                           int buffer_length,
                           const char *device);

ESPEAK_NG_API int
espeak_ng_GetSampleRate(EspeakProcessorContext* epContext);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetParameter(espeak_PARAMETER parameter,
                       int value,
                       int relative);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetPhonemeEvents(int enable, int ipa);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetPunctuationList(const wchar_t *punctlist);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetVoiceByName(const char *name);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetVoiceByFile(EspeakProcessorContext* epContext, const char *filename);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetVoiceByProperties(espeak_VOICE *voice_selector);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Synthesize(const void *text,
                     size_t size,
                     unsigned int position,
                     espeak_POSITION_TYPE position_type,
                     unsigned int end_position,
                     unsigned int flags,
                     unsigned int *unique_identifier,
                     void *user_data);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SynthesizeMark(const void *text,
                         size_t size,
                         const char *index_mark,
                         unsigned int end_position,
                         unsigned int flags,
                         unsigned int *unique_identifier,
                         void *user_data);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SpeakKeyName(const char *key_name);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SpeakCharacter(wchar_t character);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Cancel(void);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Synchronize(void);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Terminate(void);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompileDictionary(const char *dsource,
                            const char *dict_name,
                            FILE *log,
                            int flags,
                            espeak_ng_ERROR_CONTEXT *context);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompileMbrolaVoice(const char *path,
                             FILE *log,
                             espeak_ng_ERROR_CONTEXT *context);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompilePhonemeData(long rate,
                             FILE *log,
                             espeak_ng_ERROR_CONTEXT *context);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompileIntonation(FILE *log,
                            espeak_ng_ERROR_CONTEXT *context);


ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompileIntonationPath(const char *source_path,
                                const char *destination_path,
                                FILE *log,
                                espeak_ng_ERROR_CONTEXT *context);

/* eSpeak NG 1.49.1 */

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_CompilePhonemeDataPath(long rate,
                                 const char *source_path,
                                 const char *destination_path,
                                 FILE *log,
                                 espeak_ng_ERROR_CONTEXT *context);
                                 
ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetOutputHooks(espeak_ng_OUTPUT_HOOKS* hooks);
ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetConstF0(int f0);

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SetRandSeed(long seed);


#ifdef __cplusplus
}
#endif

#endif
