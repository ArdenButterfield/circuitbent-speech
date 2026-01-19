#ifndef SPEAK_LIB_H
#define SPEAK_LIB_H
/***************************************************************************
 *   Copyright (C) 2005 to 2012 by Jonathan Duddington                     *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see:                                 *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/


/*************************************************************/
/* This is the header file for the library version of espeak */
/*                                                           */
/*************************************************************/



#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <espeak-ng/common.h>

#if USE_LIBSONIC
#include "sonic.h"
#endif


#if defined(_WIN32) || defined(_WIN64)
#ifdef LIBESPEAK_NG_EXPORT
#define ESPEAK_API __declspec(dllexport)
#else
#define ESPEAK_API __declspec(dllimport)
#endif
#else
#define ESPEAK_API
#endif

#define ESPEAK_API_REVISION  12
/*
Revision 2
   Added parameter "options" to eSpeakInitialize()

Revision 3
   Added espeakWORDGAP to  espeak_PARAMETER

Revision 4
   Added flags parameter to espeak_CompileDictionary()

Revision 5
   Added espeakCHARS_16BIT

Revision 6
  Added macros: espeakRATE_MINIMUM, espeakRATE_MAXIMUM, espeakRATE_NORMAL

Revision 7  24.Dec.2011
  Changed espeak_EVENT structure to add id.string[] for phoneme mnemonics.
  Added espeakINITIALIZE_PHONEME_IPA option for espeak_Initialize() to report phonemes as IPA names.

Revision 8  26.Apr.2013
  Added function espeak_TextToPhonemes().

Revision 9  30.May.2013
  Changed function espeak_TextToPhonemes().

Revision 10 29.Aug.2014
  Changed phonememode parameter to espeak_TextToPhonemes() and espeak_SetPhonemeTrace

Revision 11 (espeak-ng)
  Made ESPEAK_API import/export symbols correctly on Windows.

Revision 12 (espeak-ng)
  Exposed espeak_SetPhonemeCallback. This is available in eSpeak, but was not exposed in this header.




*/

// voice table
typedef struct {
    const char *name;      // a given name for this voice. UTF8 string.
    const char *languages;       // list of pairs of (byte) priority + (string) language (and dialect qualifier)
    const char *identifier;      // the filename for this voice within espeak-ng-data/voices
    unsigned char gender;  // 0=none 1=male, 2=female,
    unsigned char age;     // 0=not specified, or age in years
    unsigned char variant; // only used when passed as a parameter to espeak_SetVoiceByProperties
    unsigned char xx1;     // for internal use
    int score;       // for internal use
    void *spare;     // for internal use
} espeak_VOICE;

/* Note: The espeak_VOICE structure is used for two purposes:
  1.  To return the details of the available voices.
  2.  As a parameter to  espeak_SetVoiceByProperties() in order to specify selection criteria.

   In (1), the "languages" field consists of a list of (UTF8) language names for which this voice
   may be used, each language name in the list is terminated by a zero byte and is also preceded by
   a single byte which gives a "priority" number.  The list of languages is terminated by an
   additional zero byte.

   A language name consists of a language code, optionally followed by one or more qualifier (dialect)
   names separated by hyphens (eg. "en-uk").  A voice might, for example, have languages "en-uk" and
   "en".  Even without "en" listed, voice would still be selected for the "en" language (because
   "en-uk" is related) but at a lower priority.

   The priority byte indicates how the voice is preferred for the language. A low number indicates a
   more preferred voice, a higher number indicates a less preferred voice.

   In (2), the "languages" field consists simply of a single (UTF8) language name, with no preceding
   priority byte.
*/

         /********************/
         /*  Initialization  */
         /********************/

// values for 'value' in espeak_SetParameter(espeakRATE, value, 0), nominally in words-per-minute
#define espeakRATE_MINIMUM  80
#define espeakRATE_MAXIMUM  450
#define espeakRATE_NORMAL   175

typedef struct epc EspeakProcessorContext;

typedef enum {
  espeakEVENT_LIST_TERMINATED = 0, // Retrieval mode: terminates the event list.
  espeakEVENT_WORD = 1,            // Start of word
  espeakEVENT_SENTENCE = 2,        // Start of sentence
  espeakEVENT_MARK = 3,            // Mark
  espeakEVENT_PLAY = 4,            // Audio element
  espeakEVENT_END = 5,             // End of sentence or clause
  espeakEVENT_MSG_TERMINATED = 6,  // End of message
  espeakEVENT_PHONEME = 7,         // Phoneme, if enabled in espeak_Initialize()
  espeakEVENT_SAMPLERATE = 8       // Set sample rate
} espeak_EVENT_TYPE;



typedef struct {
	espeak_EVENT_TYPE type;
	unsigned int unique_identifier; // message identifier (or 0 for key or character)
	int text_position;    // the number of characters from the start of the text
	int length;           // word length, in characters (for espeakEVENT_WORD)
	int audio_position;   // the time in mS within the generated speech output data
	int sample;           // sample id (internal use)
	void* user_data;      // pointer supplied by the calling program
	union {
		int number;        // used for WORD and SENTENCE events.
		const char *name;  // used for MARK and PLAY events.  UTF8 string
		char string[8];    // used for phoneme names (UTF8). Terminated by a zero byte unless the name needs the full 8 bytes.
	} id;
} espeak_EVENT;
/*
   When a message is supplied to espeak_synth, the request is buffered and espeak_synth returns. When the message is really processed, the callback function will be repetedly called.


   In RETRIEVAL mode, the callback function supplies to the calling program the audio data and an event list terminated by 0 (LIST_TERMINATED).

   In PLAYBACK mode, the callback function is called as soon as an event happens.

   For example suppose that the following message is supplied to espeak_Synth:
   "hello, hello."


   * Once processed in RETRIEVAL mode, it could lead to 3 calls of the callback function :

   ** Block 1:
   <audio data> +
   List of events: SENTENCE + WORD + LIST_TERMINATED

   ** Block 2:
   <audio data> +
   List of events: WORD + END + LIST_TERMINATED

   ** Block 3:
   no audio data
   List of events: MSG_TERMINATED + LIST_TERMINATED


   * Once processed in PLAYBACK mode, it could lead to 5 calls of the callback function:

   ** SENTENCE
   ** WORD (call when the sounds are actually played)
   ** WORD
   ** END (call when the end of sentence is actually played.)
   ** MSG_TERMINATED


   The MSG_TERMINATED event is the last event. It can inform the calling program to clear the user data related to the message.
   So if the synthesis must be stopped, the callback function is called for each pending message with the MSG_TERMINATED event.

   A MARK event indicates a <mark> element in the text.
   A PLAY event indicates an <audio> element in the text, for which the calling program should play the named sound file.
*/



typedef enum {
	POS_CHARACTER = 1,
	POS_WORD,
	POS_SENTENCE
} espeak_POSITION_TYPE;


typedef enum {
	/* PLAYBACK mode: plays the audio data, supplies events to the calling program*/
	AUDIO_OUTPUT_PLAYBACK,

	/* RETRIEVAL mode: supplies audio data and events to the calling program */
	AUDIO_OUTPUT_RETRIEVAL,

	/* SYNCHRONOUS mode: as RETRIEVAL but doesn't return until synthesis is completed */
	AUDIO_OUTPUT_SYNCHRONOUS,

	/* Synchronous playback */
	AUDIO_OUTPUT_SYNCH_PLAYBACK

} espeak_AUDIO_OUTPUT;


typedef enum {
	EE_OK=0,
	EE_INTERNAL_ERROR=-1,
	EE_BUFFER_FULL=1,
	EE_NOT_FOUND=2
} espeak_ERROR;

#define espeakINITIALIZE_PHONEME_EVENTS 0x0001
#define espeakINITIALIZE_PHONEME_IPA   0x0002
#define espeakINITIALIZE_DONT_EXIT     0x8000

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API int espeak_Initialize(EspeakProcessorContext* epContext, espeak_AUDIO_OUTPUT output, int buflength, const char *path, int options);
/* Must be called before any synthesis functions are called.
   output: the audio data can either be played by eSpeak or passed back by the SynthCallback function.

   buflength:  The length in mS of sound buffers passed to the SynthCallback function.
            Value=0 gives a default of 60mS.
            This parameter is only used for AUDIO_OUTPUT_RETRIEVAL and AUDIO_OUTPUT_SYNCHRONOUS modes.

   path: The directory which contains the espeak-ng-data directory, or NULL for the default location.

   options: bit 0:  1=allow espeakEVENT_PHONEME events.
            bit 1:  1= espeakEVENT_PHONEME events give IPA phoneme names, not eSpeak phoneme names
            bit 15: 1=don't exit if espeak_data is not found (used for --help)

   Returns: sample rate in Hz, or -1 (EE_INTERNAL_ERROR).
*/

typedef int (t_espeak_callback)(short*, int, espeak_EVENT*);

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API void espeak_SetSynthCallback(EspeakProcessorContext* epContext, t_espeak_callback* SynthCallback);
/* Must be called before any synthesis functions are called.
   This specifies a function in the calling program which is called when a buffer of
   speech sound data has been produced.


   The callback function is of the form:

int SynthCallback(short *wav, int numsamples, espeak_EVENT *events);

   wav:  is the speech sound data which has been produced.
      NULL indicates that the synthesis has been completed.

   numsamples: is the number of entries in wav.  This number may vary, may be less than
      the value implied by the buflength parameter given in espeak_Initialize, and may
      sometimes be zero (which does NOT indicate end of synthesis).

   events: an array of espeak_EVENT items which indicate word and sentence events, and
      also the occurrence if <mark> and <audio> elements within the text.  The list of
      events is terminated by an event of type = 0.


   Callback returns: 0=continue synthesis,  1=abort synthesis.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API void espeak_SetUriCallback(int (*UriCallback)(int, const char*, const char*));
/* This function may be called before synthesis functions are used, in order to deal with
   <audio> tags.  It specifies a callback function which is called when an <audio> element is
   encountered and allows the calling program to indicate whether the sound file which
   is specified in the <audio> element is available and is to be played.

   The callback function is of the form:

int UriCallback(int type, const char *uri, const char *base);

   type:  type of callback event.  Currently only 1= <audio> element

   uri:   the "src" attribute from the <audio> element

   base:  the "xml:base" attribute (if any) from the <speak> element

   Return: 1=don't play the sound, but speak the text alternative.
           0=place a PLAY event in the event list at the point where the <audio> element
             occurs.  The calling program can then play the sound at that point.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API void espeak_SetPhonemeCallback(EspeakProcessorContext* epContext, int (*PhonemeCallback)(const char *));


         /********************/
         /*    Synthesis     */
         /********************/


#define espeakCHARS_AUTO   0
#define espeakCHARS_UTF8   1
#define espeakCHARS_8BIT   2
#define espeakCHARS_WCHAR  3
#define espeakCHARS_16BIT  4

#define espeakSSML        0x10
#define espeakPHONEMES    0x100
#define espeakENDPAUSE    0x1000
#define espeakKEEP_NAMEDATA 0x2000

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Synth(EspeakProcessorContext* epContext, const void *text,
	size_t size,
	unsigned int position,
	espeak_POSITION_TYPE position_type,
	unsigned int end_position,
	unsigned int flags,
	unsigned int* unique_identifier,
	void* user_data);
/* Synthesize speech for the specified text.  The speech sound data is passed to the calling
   program in buffers by means of the callback function specified by espeak_SetSynthCallback(). The command is asynchronous: it is internally buffered and returns as soon as possible. If espeak_Initialize was previously called with AUDIO_OUTPUT_PLAYBACK as argument, the sound data are played by eSpeak.

   text: The text to be spoken, terminated by a zero character. It may be either 8-bit characters,
      wide characters (wchar_t), or UTF8 encoding.  Which of these is determined by the "flags"
      parameter.

   size: Equal to (or greatrer than) the size of the text data, in bytes.  This is used in order
      to allocate internal storage space for the text.  This value is not used for
      AUDIO_OUTPUT_SYNCHRONOUS mode.

   position:  The position in the text where speaking starts. Zero indicates speak from the
      start of the text.

   position_type:  Determines whether "position" is a number of characters, words, or sentences.
      Values:

   end_position:  If set, this gives a character position at which speaking will stop.  A value
      of zero indicates no end position.

   flags:  These may be OR'd together:
      Type of character codes, one of:
         espeakCHARS_UTF8     UTF8 encoding
         espeakCHARS_8BIT     The 8 bit ISO-8859 character set for the particular language.
         espeakCHARS_AUTO     8 bit or UTF8  (this is the default)
         espeakCHARS_WCHAR    Wide characters (wchar_t)
         espeakCHARS_16BIT    16 bit characters.

      espeakSSML   Elements within < > are treated as SSML elements, or if not recognised are ignored.

      espeakPHONEMES  Text within [[ ]] is treated as phonemes codes (in espeak's Kirshenbaum encoding).

      espeakENDPAUSE  If set then a sentence pause is added at the end of the text.  If not set then
         this pause is suppressed.

   unique_identifier: This must be either NULL, or point to an integer variable to
       which eSpeak writes a message identifier number.
       eSpeak includes this number in espeak_EVENT messages which are the result of
       this call of espeak_Synth().

   user_data: a pointer (or NULL) which will be passed to the callback function in
       espeak_EVENT messages.

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Synth_Mark(EspeakProcessorContext* epContext,
    const void *text,
	size_t size,
	const char *index_mark,
	unsigned int end_position,
	unsigned int flags,
	unsigned int* unique_identifier,
	void* user_data);
/* Synthesize speech for the specified text.  Similar to espeak_Synth() but the start position is
   specified by the name of a <mark> element in the text.

   index_mark:  The "name" attribute of a <mark> element within the text which specified the
      point at which synthesis starts.  UTF8 string.

   For the other parameters, see espeak_Synth()

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Key(EspeakProcessorContext* epContext, const char *key_name);
/* Speak the name of a keyboard key.
   If key_name is a single character, it speaks the name of the character.
   Otherwise, it speaks key_name as a text string.

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Char(EspeakProcessorContext* epContext, wchar_t character);
/* Speak the name of the given character

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/




         /***********************/
         /*  Speech Parameters  */
         /***********************/

typedef enum {
  espeakSILENCE=0, /* internal use */
  espeakRATE=1,
  espeakVOLUME=2,
  espeakPITCH=3,
  espeakRANGE=4,
  espeakPUNCTUATION=5,
  espeakCAPITALS=6,
  espeakWORDGAP=7,
  espeakOPTIONS=8,   // reserved for misc. options.  not yet used
  espeakINTONATION=9,
  espeakSSML_BREAK_MUL=10,

  espeakRESERVED2=11,
  espeakEMPHASIS=12,   /* internal use */
  espeakLINELENGTH=13, /* internal use */
  espeakVOICETYPE=14,  // internal, 1=mbrola
  N_SPEECH_PARAM=15    /* last enum */
} espeak_PARAMETER;

typedef enum {
  espeakPUNCT_NONE=0,
  espeakPUNCT_ALL=1,
  espeakPUNCT_SOME=2
} espeak_PUNCT_TYPE;

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_SetParameter(EspeakProcessorContext* epContext, espeak_PARAMETER parameter, int value, int relative);
/* Sets the value of the specified parameter.
   relative=0   Sets the absolute value of the parameter.
   relative=1   Sets a relative value of the parameter.

   parameter:
      espeakRATE:    speaking speed in word per minute.  Values 80 to 450.

      espeakVOLUME:  volume in range 0-200 or more.
                     0=silence, 100=normal full volume, greater values may produce amplitude compression or distortion

      espeakPITCH:   base pitch, range 0-100.  50=normal

      espeakRANGE:   pitch range, range 0-100. 0-monotone, 50=normal

      espeakPUNCTUATION:  which punctuation characters to announce:
         value in espeak_PUNCT_TYPE (none, all, some),
         see espeak_GetParameter() to specify which characters are announced.

      espeakCAPITALS: announce capital letters by:
         0=none,
         1=sound icon,
         2=spelling,
         3 or higher, by raising pitch.  This values gives the amount in Hz by which the pitch
            of a word raised to indicate it has a capital letter.

      espeakWORDGAP:  pause between words, units of 10mS (at the default speed)

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API int espeak_GetParameter(EspeakProcessorContext* epContext, espeak_PARAMETER parameter, int current);
/* current=0  Returns the default value of the specified parameter.
   current=1  Returns the current value of the specified parameter, as set by SetParameter()
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_SetPunctuationList(EspeakProcessorContext* epContext, const wchar_t *punctlist);
/* Specified a list of punctuation characters whose names are to be spoken when the
   value of the Punctuation parameter is set to "some".

   punctlist:  A list of character codes, terminated by a zero character.

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#define espeakPHONEMES_SHOW    0x01
#define espeakPHONEMES_IPA     0x02
#define espeakPHONEMES_TRACE   0x08
#define espeakPHONEMES_MBROLA  0x10
#define espeakPHONEMES_TIE     0x80

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API void espeak_SetPhonemeTrace(EspeakProcessorContext* epContext, int phonememode, FILE *stream);
/* phonememode:  Controls the output of phoneme symbols for the text
      bits 0-2:
         value=0  No phoneme output (default)
         value=1  Output the translated phoneme symbols for the text
         value=2  as (1), but produces IPA phoneme names rather than ascii
      bit 3:   output a trace of how the translation was done (showing the matching rules and list entries)
      bit 4:   produce pho data for mbrola
      bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names
      bits 8-23:  separator character, between phoneme names

   stream   output stream for the phoneme symbols (and trace).  If stream=NULL then it uses stdout.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API const char *espeak_TextToPhonemes(EspeakProcessorContext* epContext, const void **textptr, int textmode, int phonememode);
/* Translates text into phonemes.  Call espeak_SetVoiceByName() first, to select a language.

   It returns a pointer to a character string which contains the phonemes for the text up to
   end of a sentence, or comma, semicolon, colon, or similar punctuation.

   textptr: The address of a pointer to the input text which is terminated by a zero character.
      On return, the pointer has been advanced past the text which has been translated, or else set
      to NULL to indicate that the end of the text has been reached.

   textmode: Type of character codes, one of:
         espeakCHARS_UTF8     UTF8 encoding
         espeakCHARS_8BIT     The 8 bit ISO-8859 character set for the particular language.
         espeakCHARS_AUTO     8 bit or UTF8  (this is the default)
         espeakCHARS_WCHAR    Wide characters (wchar_t)
         espeakCHARS_16BIT    16 bit characters.

   phoneme_mode
	    bit 1:   0=eSpeak's ascii phoneme names, 1= International Phonetic Alphabet (as UTF-8 characters).
        bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names
        bits 8-23:  separator character, between phoneme names

*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API const char *espeak_TextToPhonemesWithTerminator(EspeakProcessorContext* epContext, const void **textptr, int textmode, int phonememode, int *terminator);
/* Version of espeak_TextToPhonemes that also returns the clause terminator (e.g., CLAUSE_INTONATION_FULL_STOP) */

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API void espeak_CompileDictionary(EspeakProcessorContext* epContext, const char *path, FILE *log, int flags);
/* Compile pronunciation dictionary for a language which corresponds to the currently
   selected voice.  The required voice should be selected before calling this function.

   path:  The directory which contains the language's '_rules' and '_list' files.
          'path' should end with a path separator character ('/').
   log:   Stream for error reports and statistics information. If log=NULL then stderr will be used.

   flags:  Bit 0: include source line information for debug purposes (This is displayed with the
          -X command line option).
*/
         /***********************/
         /*   Voice Selection   */
         /***********************/


#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API const espeak_VOICE **espeak_ListVoices(EspeakProcessorContext* epContext, espeak_VOICE *voice_spec);
/* Reads the voice files from espeak-ng-data/voices and creates an array of espeak_VOICE pointers.
   The list is terminated by a NULL pointer

   If voice_spec is NULL then all voices are listed.
   If voice spec is given, then only the voices which are compatible with the voice_spec
   are listed, and they are listed in preference order.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_SetVoiceByFile(EspeakProcessorContext* epContext, const char *filename);
/* Loads a voice given the file path.  Language is not considered.
   "filename" is a UTF8 string.

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_SetVoiceByName(EspeakProcessorContext* epContext, const char *name);
/* Searches for a voice with a matching "name" field.  Language is not considered.
   "name" is a UTF8 string.

   Return: EE_OK: operation achieved
           EE_BUFFER_FULL: the command can not be buffered;
             you may try after a while to call the function again.
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_SetVoiceByProperties(EspeakProcessorContext* epContext, espeak_VOICE *voice_spec);
/* An espeak_VOICE structure is used to pass criteria to select a voice.  Any of the following
   fields may be set:

   name     NULL, or a voice name

   languages  NULL, or a single language string (with optional dialect), eg. "en-uk", or "en"

   gender   0=not specified, 1=male, 2=female

   age      0=not specified, or an age in years

   variant  After a list of candidates is produced, scored and sorted, "variant" is used to index
            that list and choose a voice.
            variant=0 takes the top voice (i.e. best match). variant=1 takes the next voice, etc
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_VOICE *espeak_GetCurrentVoice(EspeakProcessorContext* epContext);
/* Returns the espeak_VOICE data for the currently selected voice.
   This is not affected by temporary voice changes caused by SSML elements such as <voice> and <s>
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Cancel(EspeakProcessorContext* epContext);
/* Stop immediately synthesis and audio output of the current text. When this
   function returns, the audio output is fully stopped and the synthesizer is ready to
   synthesize a new message.

   Return: EE_OK: operation achieved
	   EE_INTERNAL_ERROR.
*/


#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API int espeak_IsPlaying(EspeakProcessorContext* epContext);
/* Returns 1 if audio is played, 0 otherwise.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Synchronize(EspeakProcessorContext* epContext);
/* This function returns when all data have been spoken.
   Return: EE_OK: operation achieved
	   EE_INTERNAL_ERROR.
*/

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API espeak_ERROR espeak_Terminate(EspeakProcessorContext* epContext);
/* last function to be called.
   Return: EE_OK: operation achieved
	   EE_INTERNAL_ERROR.
*/


#ifdef __cplusplus
extern "C"
#endif
ESPEAK_API const char *espeak_Info(EspeakProcessorContext* epContext, const char **path_data);
/* Returns the version number string.
   path_data  returns the path to espeak_data
*/

typedef struct BendRescaler
{
    float start;
    float end;
    float curve;
} BendRescaler;

inline float applyBendRescaler(BendRescaler* rescaler, float x, float newMin, float newMax)
{
    float sloped = powf (x, powf(5.f, -rescaler->curve));
    float rescaled = (rescaler->end - rescaler->start) * sloped + rescaler->start;
    rescaled = rescaled < 0 ? 0 : rescaled;
    rescaled = rescaled > newMax ? newMax : rescaled;
    return rescaled * (newMax - newMin) + newMin;
}

typedef struct esb
{
    int rotatePhonemes;
    float stickChance;
    bool debugPrintEverything;
    float fundamentalFreq;
    bool freeze;
    float wavetableShape;
    float detuneHarmonics;

    BendRescaler formantFrequencyRescaler;
    BendRescaler formantHeightRescaler;

    unsigned short vibratoWavePosition;
    float vibratoAmount;
    float pitchbendMultiplier;
} EspeakBends;

typedef struct {
    int name; // used for detecting punctuation
    int length;
    char *data;
    char *filename;
} SOUND_ICON;

#define N_SOUNDICON_TAB  80   // total entries for dynamic loading of audio files


typedef struct {
    int type;
    int parameter[N_SPEECH_PARAM];
} PARAM_STACK;

typedef struct Translator Translator;

struct epc
{
    // Buffer and variables for passing data to plugin
    int pluginBufferSize;
    float* pluginBuffer;
    int pluginBufferPosition;
    bool readyToProcess;
    bool doneProcessing;
    bool allDone;
    bool noteEndingEarly;

    EspeakBends bends;

    // dictionary.c
    int dictionary_skipwords;
    char dictionary_name[40];

    // TODO: come back to event.c questions on thread library

    // intonation.c
    int tone_pitch_env; // used to return pitch envelope

    int number_pre;
    int number_tail;
    int last_primary;
    int tone_posn;
    int tone_posn2;
    int no_tonic;

    // klatt.c
    unsigned char *out_ptr;
    unsigned char *out_end;
    int nsamples_klatt;
    int sample_count;

    klatt_frame_t kt_frame;
    klatt_global_t kt_globals;

    // mbrola.h
    int mbrola_delay;
    char mbrola_name[20];

    // numbers.c
    int n_digit_lookup;
    char *digit_lookup;
    int speak_missing_thousands;
    int number_control;

    // phonemelist.c
    int n_ph_list2;
    PHONEME_LIST2 ph_list2[N_PHONEME_LIST]; // first stage of text->phonemes

    // readclause.c
    const char *xmlbase; // = ""; // base URL from <speak>

    int namedata_ix; // = 0;
    int n_namedata; // = 0;
    char *namedata; // = NULL;

    int ungot_char2; // = 0;
    espeak_ng_TEXT_DECODER *p_decoder; // = NULL;
    int ungot_char;

    bool ignore_text; // = false; // set during <sub> ... </sub>  to ignore text which has been replaced by an alias
    bool audio_text; // = false; // set during <audio> ... </audio>
    bool clear_skipping_text; // = false; // next clause should clear the skipping_text flag
    int count_characters; // = 0;
    int sayas_mode;
    int sayas_start;

    #define N_SSML_STACK  20
    int n_ssml_stack;
    SSML_STACK ssml_stack[N_SSML_STACK];

    espeak_VOICE base_voice;
    char base_voice_variant_name[40]; // = { 0 };
    char current_voice_id[40]; // = { 0 };

    int n_param_stack;
    PARAM_STACK param_stack[N_PARAM_STACK];

    int speech_parameters[N_SPEECH_PARAM]; // current values, from param_stack
    int saved_parameters[N_SPEECH_PARAM]; // Parameters saved on synthesis start

    // setlengths.c

    int len_speeds[3]; // = { 130, 121, 118 };

    // soundicon.c

    int n_soundicon_tab;
    SOUND_ICON soundicon_tab[N_SOUNDICON_TAB];

    // speech.c
    unsigned char *outbuf; // = NULL;
    int outbuf_size; // = 0;
    unsigned char *out_start;

    espeak_EVENT *event_list; // = NULL;
    int event_list_ix; // = 0;
    int n_event_list;
    long count_samples;
    #if USE_LIBPCAUDIO
    static struct audio_object *my_audio = NULL;
    #endif

    unsigned int my_unique_identifier; // = 0;
    void *my_user_data; // = NULL;
    espeak_ng_OUTPUT_MODE my_mode; // = ENOUTPUT_MODE_SYNCHRONOUS;
    int out_samplerate; // = 0;
    int voice_samplerate; // = 22050;
    espeak_ng_STATUS err; // = ENS_OK;

     t_espeak_callback *synth_callback; // = NULL;

    char path_home[N_PATH_HOME]; // this is the espeak-ng-data directory


    // synthdata.c
    int n_tunes;// = 0;
    TUNE *tunes;// = NULL;
    // copy the current phoneme table into here
    int n_phoneme_tab;
    int current_phoneme_table;
    PHONEME_TAB *phoneme_tab[N_PHONEME_TAB];

    unsigned short *phoneme_index;// = NULL;
    char *phondata_ptr;// = NULL;
    unsigned char *wavefile_data;// = NULL;
    unsigned char *phoneme_tab_data;// = NULL;

    int n_phoneme_tables;
    PHONEME_TAB_LIST phoneme_tab_list[N_PHONEME_TABS];
    int phoneme_tab_number;// = 0;

    int seq_len_adjust;

    // synthesize.c
    // list of phonemes in a clause
    int n_phoneme_list;// = 0;
    PHONEME_LIST phoneme_list[N_PHONEME_LIST+1];

    SPEED_FACTORS speed;

    int last_pitch_cmd;
    int last_amp_cmd;
    frame_t  *last_frame;
    int last_wcmdq;
    int pitch_length;
    int amp_length;
    int modn_flags;
    int fmt_amplitude;// = 0;

    int syllable_start;
    int syllable_end;
    int syllable_centre;

    voice_t *new_voice;// = NULL;

    // translate.c
    Translator *translator;// = NULL; // the main translator
    Translator *translator2;// = NULL; // secondary translator for certain words
    char translator2_language[20];// = { 0 };
    Translator *translator3;// = NULL; // tertiary translator for certain words
    char translator3_language[20];// = { 0 };

    FILE *f_trans;// = NULL; // phoneme output text
    int option_tone_flags;// = 0; // bit 8=emphasize allcaps, bit 9=emphasize penultimate stress
    int option_phonemes;// = 0;
    int option_phoneme_events;// = 0;
    int option_endpause;// = 0; // suppress pause after end of text
    int option_capitals;// = 0;
    int option_punctuation;// = 0;
    int option_sayas;// = 0;
     int option_sayas2;// = 0; // used in translate_clause()
     int option_emphasis;// = 0; // 0=normal, 1=normal, 2=weak, 3=moderate, 4=strong
    int option_ssml;// = 0;
    int option_phoneme_input;// = 0; // allow [[phonemes]] in input
    int option_wordgap;// = 0;

    int count_sayas_digits;
    int skip_sentences;
    int skip_words;
    int skip_characters;
    char skip_marker[N_MARKER_LENGTH];
    bool skipping_text; // waiting until word count, sentence count, or named marker is reached
    int end_character_position;
    int count_sentences;
    int count_words;
    int clause_start_char;
    int clause_start_word;
    bool new_sentence;
    int word_emphasis; // = 0; // set if emphasis level 3 or 4
    int embedded_flag; // = 0; // there are embedded commands to be applied to the next phoneme, used in TranslateWord2()

    int max_clause_pause; // = 0;
    bool any_stressed_words;
    int pre_pause;
    ALPHABET *current_alphabet;

    char word_phonemes[N_WORD_PHONEMES]; // a word translated into phoneme codes

    wchar_t option_punctlist[N_PUNCTLIST_TRANS]; // = { 0 };

    // these are overridden by defaults set in the "speak" file
    int option_linelength;// = 0;

    #define N_EMBEDDED_LIST  250
    int embedded_ix;
    int embedded_read;
    unsigned int embedded_list[N_EMBEDDED_LIST];

    // the source text of a single clause (UTF8 bytes)
    char source[N_TR_SOURCE+40]; // extra space for embedded command & voice change info at end

    int n_replace_phonemes;
    REPLACE_PHONEMES replace_phonemes[N_REPLACE_PHONEMES];

    // voices.c
    int tone_points[12]; // = { 600, 170, 1200, 135, 2000, 110, 3000, 110, -1, 0 };
    int formant_rate[9]; // values adjusted for actual sample rate
    int n_voices_list;// = 0;
    espeak_VOICE *voices_list[N_VOICES_LIST];

    espeak_VOICE current_voice_selected;

    voice_t voicedata;
    voice_t *voice;

    // wavegen.c
    voice_t *wvoice;// = NULL;

    int option_harmonic1;// = 10;
    int flutter_amp;// = 64;

    int general_amplitude;// = 60;
    int consonant_amp;// = 26;

    int embedded_value[N_EMBEDDED_VALUES];

    int PHASE_INC_FACTOR;
    int samplerate;// = 0; // this is set by Wavegeninit()

    wavegen_peaks_t peaks[N_PEAKS];
    int peak_harmonic[N_PEAKS];
    int peak_height[N_PEAKS];

    int echo_head;
    int echo_tail;
    int echo_amp;
    short echo_buf[N_ECHO_BUF];
    int echo_length;// = 0; // period (in sample\) to ensure completion of echo at the end of speech, set in WavegenSetEcho()

    int voicing;
    RESONATOR rbreath[N_PEAKS];

    int harm_inc[N_LOWHARM]; // only for these harmonics do we interpolate amplitude between steps
    int *harmspect;
    int hswitch;// = 0;
    int hspect[2][MAX_HARMONIC]; // 2 copies, we interpolate between then

    int nsamples;// = 0; // number to do
    int modulation_type;// = 0;
    int glottal_flag;// = 0;
    int glottal_reduce;// = 0;

    WGEN_DATA wdata;

    int amp_ix;
    int amp_inc;
    unsigned char *amplitude_env;// = NULL;

    int samplecount;// = 0; // number done
    int samplecount_start;// = 0; // count at start of this segment
    int end_wave;// = 0; // continue to end of wave cycle
    int wavephase;
    int phaseinc;
    int cycle_samples; // number of samples in a cycle at current pitch
    int cbytes;
    int hf_factor;

    double minus_pi_t;
    double two_pi_t;

    espeak_ng_OUTPUT_HOOKS* output_hooks;
    int const_f0;

    // the queue of operations passed to wavegen from sythesize
    intptr_t wcmdq[N_WCMDQ][4];
    int wcmdq_head;// = 0;
    int wcmdq_tail;// = 0;


    #if USE_LIBSONIC
    sonicStream sonicSpeedupStream;
    double sonicSpeed;
    #endif

    int Flutter_inc;

    // waveform shape table for HF peaks, formants 6,7,8
    int wavemult_offset;// = 0;
    int wavemult_max;// = 0;

    // the presets are for 22050 Hz sample rate.
    // A different rate will need to recalculate the presets in WavegenInit()
    unsigned char wavemult[N_WAVEMULT];


};

#ifdef __cplusplus
extern "C"
#endif
ESPEAK_NG_API void
initEspeakContext(EspeakProcessorContext* epContext);


#endif