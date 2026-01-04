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


typedef enum {
    ENOUTPUT_MODE_SYNCHRONOUS = 0x0001,
    ENOUTPUT_MODE_SPEAK_AUDIO = 0x0002,
} espeak_ng_OUTPUT_MODE;

#endif //HOMER_COMMON_H
