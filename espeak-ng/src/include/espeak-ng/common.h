//
// Created by Arden on 1/4/2026.
//

#ifndef HOMER_COMMON_H
#define HOMER_COMMON_H

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

typedef enum {
    ENS_GROUP_MASK               = 0x70000000,
    ENS_GROUP_ERRNO              = 0x00000000, /* Values 0-255 map to errno error codes. */
    ENS_GROUP_ESPEAK_NG          = 0x10000000, /* eSpeak NG error codes. */

    /* eSpeak NG 1.49.0 */
    ENS_OK                       = 0,
    ENS_COMPILE_ERROR            = 0x100001FF,
    ENS_VERSION_MISMATCH         = 0x100002FF,
    ENS_FIFO_BUFFER_FULL         = 0x100003FF,
    ENS_NOT_INITIALIZED          = 0x100004FF,
    ENS_AUDIO_ERROR              = 0x100005FF,
    ENS_VOICE_NOT_FOUND          = 0x100006FF,
    ENS_MBROLA_NOT_FOUND         = 0x100007FF,
    ENS_MBROLA_VOICE_NOT_FOUND   = 0x100008FF,
    ENS_EVENT_BUFFER_FULL        = 0x100009FF,
    ENS_NOT_SUPPORTED            = 0x10000AFF,
    ENS_UNSUPPORTED_PHON_FORMAT  = 0x10000BFF,
    ENS_NO_SPECT_FRAMES          = 0x10000CFF,
    ENS_EMPTY_PHONEME_MANIFEST   = 0x10000DFF,
    ENS_SPEECH_STOPPED           = 0x10000EFF,

    /* eSpeak NG 1.49.2 */
    ENS_UNKNOWN_PHONEME_FEATURE  = 0x10000FFF,
    ENS_UNKNOWN_TEXT_ENCODING    = 0x100010FF,
} espeak_ng_STATUS;

#if defined(_WIN32) || defined(_WIN64) // Windows

#define PLATFORM_WINDOWS 1
#define PATHSEP '\\'
#define N_PATH_HOME_DEF  230
#define NO_VARIADIC_MACROS

#else

#define PLATFORM_POSIX 1
#define PATHSEP  '/'
#if defined(__linux__) // Linux
#  include <linux/limits.h>
#  define N_PATH_HOME_DEF  PATH_MAX
#else
#  define N_PATH_HOME_DEF  160
#endif
#define USE_NANOSLEEP
#define __cdecl

#endif

#ifndef N_PATH_HOME
#define N_PATH_HOME N_PATH_HOME_DEF
#endif

#ifdef __cplusplus
}
#endif

#define N_PEAKS   9

#define N_MARKER_LENGTH 50   // max.length of a mark name

#define N_WORD_PHONEMES  200 // max phonemes in a word
#define N_WORD_BYTES     160 // max bytes for the UTF8 characters in a word
#define N_PHONEME_BYTES  160 // max bytes for a phoneme
#define N_CLAUSE_WORDS   300 // max words in a clause
#define N_TR_SOURCE      800 // the source text of a single clause (UTF8 bytes)

#define N_PUNCTLIST_TRANS  60

#define N_VOICES_LIST  350


typedef struct {
    const char *name;
    int offset;
    unsigned int range_min, range_max;
    int language;
    int flags;
} ALPHABET;


typedef struct {
    char v_name[40];
    char language_name[20];

    int phoneme_tab_ix; // phoneme table number
    int pitch_base; // Hz<<12
    int pitch_range; // standard = 0x1000

    int speedf1;
    int speedf2;
    int speedf3;

    int speed_percent;      // adjust the WPM speed by this percentage
    int flutter;
    int roughness;
    int echo_delay;
    int echo_amp;
    int n_harmonic_peaks;  // highest formant which is formed from adding harmonics
    int peak_shape;        // alternative shape for formant peaks (0=standard 1=squarer)
    int voicing;           // 100% = 64, level of formant-synthesized sound
    int formant_factor;    // adjust nominal formant frequencies by this  because of the voice's pitch (256ths)
    int consonant_amp;     // amplitude of unvoiced consonants
    int consonant_ampv;    // amplitude of the noise component of voiced consonants
    int samplerate;
    int klattv[8];

    // parameters used by Wavegen
    short freq[N_PEAKS];    // 100% = 256
    short height[N_PEAKS];  // 100% = 256
    short width[N_PEAKS];   // 100% = 256
    short freqadd[N_PEAKS]; // Hz

    // copies without temporary adjustments from embedded commands
    short freq2[N_PEAKS];    // 100% = 256
    short height2[N_PEAKS];  // 100% = 256

    int breath[N_PEAKS];  // amount of breath for each formant. breath[0] indicates whether any are set.
    int breathw[N_PEAKS]; // width of each breath formant

    // This table provides the opportunity for tone control.
    // Adjustment of harmonic amplitudes, steps of 8Hz
    // value of 128 means no change
    #define N_TONE_ADJUST  1000
    unsigned char tone_adjust[N_TONE_ADJUST];   //  8Hz steps * 1000 = 8kHz

} voice_t;

typedef enum {
    ENOUTPUT_MODE_SYNCHRONOUS = 0x0001,
    ENOUTPUT_MODE_SPEAK_AUDIO = 0x0002,
} espeak_ng_OUTPUT_MODE;


#define N_LOWHARM  30
#define MAX_HARMONIC 400 // 400 * 50Hz = 20 kHz, more than enough

typedef struct {
    int freq;     // Hz<<16
    int height;   // height<<15
    int left;     // Hz<<16
    int right;    // Hz<<16
    double freq1; // floating point versions of the above
    double height1;
    double left1;
    double right1;
    double freq_inc; // increment by this every 64 samples
    double height_inc;
    double left_inc;
    double right_inc;
} wavegen_peaks_t;

typedef struct
{
    void (*outputPhoSymbol)(char* pho_code,int pho_type);
    void (*outputSilence)(short echo_tail);
    void (*outputVoiced)(short sample);
    void (*outputUnvoiced)(short sample);
} espeak_ng_OUTPUT_HOOKS;

#define N_WAVEMULT 128

#define N_PUNCTLIST 100

// a clause translated into phoneme codes (first stage)
typedef struct {
    unsigned short synthflags; // NOTE Put shorts on 32bit boundaries, because of RISC OS compiler bug?
    unsigned char phcode;
    unsigned char stresslevel;
    unsigned short sourceix;  // ix into the original source text string, only set at the start of a word
    unsigned char wordstress; // the highest level stress in this word
    unsigned char tone_ph;    // tone phoneme to use with this vowel
} PHONEME_LIST2;

#define N_PHONEME_LIST 1000 // enough for source[N_TR_SOURCE] full of text, else it will truncate

typedef struct {
    int pause_factor;
    int clause_pause_factor;
    unsigned int min_pause;
    int wav_factor;
    int lenmod_factor;
    int lenmod2_factor;
    int min_sample_len;
    int fast_settings;	// TODO: rename this variable to better explain the purpose, or delete if there is none
} SPEED_FACTORS;

typedef struct {
    char name[12];
    unsigned char flags[4];
    signed char head_extend[8];

    unsigned char prehead_start;
    unsigned char prehead_end;
    unsigned char stressed_env;
    unsigned char stressed_drop;
    unsigned char secondary_drop;
    unsigned char unstressed_shape;

    unsigned char onset;
    unsigned char head_start;
    unsigned char head_end;
    unsigned char head_last;

    unsigned char head_max_steps;
    unsigned char n_head_extend;

    signed char unstr_start[3]; // for: onset, head, last
    signed char unstr_end[3];

    unsigned char nucleus0_env; // pitch envelope, tonic syllable is at end, no tail
    unsigned char nucleus0_max;
    unsigned char nucleus0_min;

    unsigned char nucleus1_env; // when followed by a tail
    unsigned char nucleus1_max;
    unsigned char nucleus1_min;
    unsigned char tail_start;
    unsigned char tail_end;

    unsigned char split_nucleus_env;
    unsigned char split_nucleus_max;
    unsigned char split_nucleus_min;
    unsigned char split_tail_start;
    unsigned char split_tail_end;
    unsigned char split_tune;

    unsigned char spare[8];
    int spare2; // the struct length should be a multiple of 4 bytes
} TUNE;

typedef struct {
    unsigned int mnemonic;       // Up to 4 characters.  The first char is in the l.s.byte
    unsigned int phflags;        // bits 16-19 place of articulation
    unsigned short program;      // index into phondata file
    unsigned char code;          // the phoneme number
    unsigned char type;          // phVOWEL, phPAUSE, phSTOP etc
    unsigned char start_type;
    unsigned char end_type;      // vowels: endtype; consonant: voicing switch
    unsigned char std_length;    // for vowels, in mS/2;  for phSTRESS phonemes, this is the stress/tone type
    unsigned char length_mod;    // a length_mod group number, used to access length_mod_tab
} PHONEME_TAB;

#define N_PHONEME_TABS     150     // number of phoneme tables
#define N_PHONEME_TAB      256     // max phonemes in a phoneme table
#define N_PHONEME_TAB_NAME  32     // must be multiple of 4

// table of phonemes to be replaced with different phonemes, for the current voice
#define N_REPLACE_PHONEMES   60
typedef struct {
    unsigned char old_ph;
    unsigned char new_ph;
    char type;   // 0=always replace, 1=only at end of word
} REPLACE_PHONEMES;

#define N_EMBEDDED_VALUES    15
#define N_ECHO_BUF 5500   // max of 250mS at 22050 Hz

typedef struct {
    double a;
    double b;
    double c;
    double x1;
    double x2;
} RESONATOR;


typedef struct {
    char name[N_PHONEME_TAB_NAME];
    PHONEME_TAB *phoneme_tab_ptr;
    int n_phonemes;
    int includes;            // also include the phonemes from this other phoneme table
} PHONEME_TAB_LIST;

typedef struct {
    int F0hz10; /* Voicing fund freq in Hz                          */
    int AVdb;   /* Amp of voicing in dB,            0 to   70       */
    int Fhz[10];  // formant Hz, F_NZ to F6 to F_NP
    int Bhz[10];
    int Ap[10];   /* Amp of parallel formants in dB,    0 to   80       */
    int Bphz[10]; /* Parallel formants bw in Hz,       40 to 1000      */

    int ASP;    /* Amp of aspiration in dB,         0 to   70       */
    int Kopen;  /* # of samples in open period,     10 to   65      */
    int Aturb;  /* Breathiness in voicing,          0 to   80       */
    int TLTdb;  /* Voicing spectral tilt in dB,     0 to   24       */
    int AF;     /* Amp of frication in dB,          0 to   80       */
    int Kskew;  /* Skewness of alternate periods,   0 to   40 in sample#/2  */

    int AB;     /* Amp of bypass fric. in dB,       0 to   80       */
    int AVpdb;  /* Amp of voicing,  par in dB,      0 to   70       */
    int Gain0;  /* Overall gain, 60 dB is unity,    0 to   60       */

    int AVdb_tmp;      // copy of AVdb, which is changed within parwave()
    int Fhz_next[10];    // Fhz for the next chunk, so we can do interpolation of resonator (a,b,c) parameters
    int Bhz_next[10];
} klatt_frame_t, *klatt_frame_ptr;

/* Structure for Klatt Globals */

typedef struct {
    double a;
    double b;
    double c;
    double p1;
    double p2;
    double a_inc;
    double b_inc;
    double c_inc;
} resonator_t, *resonator_ptr;

/* typedef's that need to be exported */

typedef long flag; // TODO: just for klatt i think, does this break anything?


typedef struct {
	flag synthesis_model; /* cascade-parallel or all-parallel */
	flag outsl;     /* Output waveform selector                      */
	long samrate;   /* Number of output samples per second           */
	long FLPhz;     /* Frequeny of glottal downsample low-pass filter */
	long BLPhz;     /* Bandwidth of glottal downsample low-pass filter */
	flag glsource;  /* Type of glottal source */
	int f0_flutter; /* Percentage of f0 flutter 0-100 */
	long nspfr;     /* number of samples per frame */
	long nper;      /* Counter for number of samples in a pitch period */
	long ns;
	long T0;        /* Fundamental period in output samples times 4 */
	long nopen;     /* Number of samples in open phase of period    */
	long nmod;      /* Position in period to begin noise amp. modul */
	long nrand;     /* Variable used by random number generator      */
	double pulse_shape_a; /* Makes waveshape of glottal pulse when open   */
	double pulse_shape_b; /* Makes waveshape of glottal pulse when open   */
	double minus_pi_t;
	double two_pi_t;
	double onemd;
	double decay;
	double amp_bypas; /* AB converted to linear gain              */
	double amp_voice; /* AVdb converted to linear gain            */
	double par_amp_voice; /* AVpdb converted to linear gain       */
	double amp_aspir; /* AP converted to linear gain              */
	double amp_frica; /* AF converted to linear gain              */
	double amp_breth; /* ATURB converted to linear gain           */
	double amp_gain0; /* G0 converted to linear gain              */
	int num_samples; /* number of glottal samples */
	double sample_factor; /* multiplication factor for glottal samples */
	const short *natural_samples; /* pointer to an array of glottal samples */
	long original_f0; /* original value of f0 not modified by flutter */

	int fadein;
	int fadeout;       // set to 64 to cause fadeout over 64 samples
	int scale_wav;     // depends on the voicing source

#define N_RSN 20
#define Rnz  0   // nasal zero, anti-resonator
#define R1c  1
#define R2c  2
#define R3c  3
#define R4c  4
#define R5c  5
#define R6c  6
#define R7c  7
#define R8c  8
#define Rnpc 9   // nasal pole

#define Rparallel 10
#define Rnpp 10
#define R1p  11
#define R2p  12
#define R6p  16

#define RGL  17
#define RLP  18
#define Rout 19

	resonator_t rsn[N_RSN];  // internal storage for resonators
	resonator_t rsn_next[N_RSN];

} klatt_global_t, *klatt_global_ptr;

typedef struct {
    // The first section is a copy of PHONEME_LIST2
    unsigned short synthflags;
    unsigned char phcode;
    unsigned char stresslevel;
    unsigned short sourceix;  // ix into the original source text string, only set at the start of a word
    unsigned char wordstress; // the highest level stress in this word
    unsigned char tone_ph;    // tone phoneme to use with this vowel

    PHONEME_TAB *ph;
    unsigned int length;  // length_mod
    unsigned char env;    // pitch envelope number
    unsigned char type;
    unsigned char prepause;
    unsigned char amp;
    unsigned char newword;   // bit flags, see PHLIST_(START|END)_OF_*
    unsigned char pitch1;
    unsigned char pitch2;
    unsigned char std_length;
    unsigned int phontab_addr;
    int sound_param;
} PHONEME_LIST;

typedef struct { // 64 bytes
    short frflags;
    short ffreq[7];
    unsigned char length;
    unsigned char rms;
    unsigned char fheight[8];
    unsigned char fwidth[6];   // width/4  f0-5
    unsigned char fright[3];   // width/4  f0-2
    unsigned char bw[4];       // Klatt bandwidth BNZ /2, f1,f2,f3
    unsigned char klattp[5];   // AV, FNZ, Tilt, Aspr, Skew
    unsigned char klattp2[5];  // continuation of klattp[],  Avp, Fric, FricBP, Turb
    unsigned char klatt_ap[7]; // Klatt parallel amplitude
    unsigned char klatt_bp[7]; // Klatt parallel bandwidth  /2
    unsigned char spare;       // pad to multiple of 4 bytes
} frame_t; // with extra Klatt parameters for parallel resonators

typedef struct {
    const unsigned char *pitch_env;
    int pitch;      // pitch Hz*256
    int pitch_ix;   // index into pitch envelope (*256)
    int pitch_inc;  // increment to pitch_ix
    int pitch_base; // Hz*256 low, before modified by envelope
    int pitch_range; // Hz*256 range of envelope

    unsigned char *mix_wavefile; // wave file to be added to synthesis
    int n_mix_wavefile; // length in bytes
    int mix_wave_scale; // 0=2 byte samples
    int mix_wave_amp;
    int mix_wavefile_ix;
    int mix_wavefile_max; // length of available WAV data (in bytes)
    int mix_wavefile_offset;

    int amplitude;
    int amplitude_v;
    int amplitude_fmt; // percentage amplitude adjustment for formant synthesis
} WGEN_DATA;

typedef struct t_node {
    void *data;
    struct t_node *next;
} node;

#define N_KLATTP   10 // this affects the phoneme data file format

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


#define N_WCMDQ   170

// stack for language and voice properties
// frame 0 is for the defaults, before any ssml tags.
typedef struct {
    int tag_type;
    int voice_variant_number;
    int voice_gender;
    int voice_age;
    char voice_name[40];
    char language[20];
} SSML_STACK;

#define N_PARAM_STACK  20

// alphabet flags
#define AL_DONT_NAME    0x01 // don't speak the alphabet name
#define AL_NOT_LETTERS  0x02 // don't use the language for speaking letters
#define AL_WORDS        0x04 // use the language to speak words
#define AL_NOT_CODE     0x08 // don't speak the character code
#define AL_NO_SYMBOL    0x10 // don't repeat "symbol" or "character"

#define N_LOPTS       18
#define LOPT_DIERESES  0
// 1=remove [:] from unstressed syllables, 2= remove from unstressed or non-penultimate syllables
// bit 4=0, if stress < 4,  bit 4=1, if not the highest stress in the word
#define LOPT_IT_LENGTHEN 1

// 1=german
#define LOPT_PREFIXES 2

// non-zero, change voiced/unoiced to match last consonant in a cluster
// bit 0=use regressive voicing
// bit 1=LANG=cz,bg  don't propagate over [v]
// bit 2=don't propagate acress word boundaries
// bit 3=LANG=pl,  propagate over liquids and nasals
// bit 4=LANG=cz,sk  don't propagate to [v]
// bit 8=devoice word-final consonants
#define LOPT_REGRESSIVE_VOICING 3

// 0=default, 1=no check, other allow this character as an extra initial letter (default is 's')
#define LOPT_UNPRONOUNCABLE 4

// increase this to prevent sonorants being shortened before shortened (eg. unstressed) vowels
#define LOPT_SONORANT_MIN 5

// bit 0: don't break vowels at word boundary
#define LOPT_WORD_MERGE 6


// max. amplitude for vowel at the end of a clause
#define LOPT_MAXAMP_EOC 7

// bit 0=reduce even if phonemes are specified in the **_list file
// bit 1=don't reduce the strongest vowel in a word which is marked 'unstressed'
#define LOPT_REDUCE 8

// LANG=cs,sk  combine some prepositions with the following word, if the combination has N or fewer syllables
// bits 0-3  N syllables
// bit 4=only if the second word has $alt attribute
// bit 5=not if the second word is end-of-sentence
#define LOPT_COMBINE_WORDS 9

// 1 = stressed syllable is indicated by capitals
#define LOPT_CAPS_IN_WORD 10

// Call ApplySpecialAttributes() if $alt or $alt2 is set for a word
// bit 1: stressed syllable: $alt change [e],[o] to [E],[O],  $alt2 change [E],[O] to [e],[o]
#define LOPT_ALT 11

// pause for bracket (default=4), also see LOPT_BRACKET_PAUSE_ANNOUNCED
#define LOPT_BRACKET_PAUSE 12

// bit 1, don't break clause before annoucning . ? !
#define LOPT_ANNOUNCE_PUNCT 13

// recognize long vowels (0 = don't recognize)
#define LOPT_LONG_VOWEL_THRESHOLD 14

// bit 0:  Don't allow suffices if there is no previous syllable
#define LOPT_SUFFIX 15

// bit 0  Apostrophe at start of word is part of the word
// bit 1  Apostrophe at end of word is part of the word
#define LOPT_APOSTROPHE 16

// pause when announcing bracket names (default=2), also see LOPT_BRACKET_PAUSE
#define LOPT_BRACKET_PAUSE_ANNOUNCED 17

typedef struct espeak_ng_TEXT_DECODER_ espeak_ng_TEXT_DECODER;

#endif //HOMER_COMMON_H
