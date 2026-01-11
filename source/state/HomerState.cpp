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

    currentVoiceParam = new juce::AudioParameterChoice({"curvoice", 1}, "current Voice", voiceNames, 7);
    singParam = new juce::AudioParameterBool({"sing", 1}, "speak/sing", 0);
    phonemeRotationParam = new juce::AudioParameterFloat({"phonemerotation", 1}, "phoneme rotation", 0, 1, 0);
    phonemeStickParam = new juce::AudioParameterFloat({"phonemestick", 1}, "phoneme stick", 0, 1, 0);
}