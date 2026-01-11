//
// Created by Arden on 1/10/2026.
//

#include "HomerState.h"

#include "espeak-ng/speak_lib.h"

HomerState::HomerState()
{
    EspeakProcessorContext epContext;

    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
    int buflength = 500, options = 0;
    memset(&epContext, 0, sizeof(EspeakProcessorContext));
    initEspeakContext(&epContext);
    espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default

    auto voices = espeak_ListVoices(&epContext, nullptr);
    for (int i = 0; voices[i] != nullptr; i++) {
        voiceNames.add((const char8_t* const)voices[i]->name);
    }
}