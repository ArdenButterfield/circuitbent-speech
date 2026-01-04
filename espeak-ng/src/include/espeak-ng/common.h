//
// Created by Arden on 1/4/2026.
//

#ifndef HOMER_COMMON_H
#define HOMER_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

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

#endif //HOMER_COMMON_H
