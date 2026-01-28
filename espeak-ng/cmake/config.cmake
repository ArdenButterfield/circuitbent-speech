include(CheckSymbolExists)
check_symbol_exists(mkstemp "stdlib.h" HAVE_MKSTEMP)

option(USE_MBROLA "Use mbrola for speech synthesis" ${HAVE_MBROLA})
#option(USE_LIBSONIC "Use libsonit for faster speech rates" ${HAVE_LIBSONIC})
set(USE_LIBSONIC OFF)
option(USE_LIBPCAUDIO "Use libPcAudio for sound output" OFF)

option(USE_KLATT "Use klatt for speech synthesis" ON)
option(USE_SPEECHPLAYER "Use speech-player for speech synthesis" ON)
set(USE_ASYNC OFF)

option(ESPEAK_COMPAT "Install compat binary symlinks" OFF)