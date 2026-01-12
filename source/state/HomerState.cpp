//
// Created by Arden on 1/10/2026.
//

#include "HomerState.h"

#include "espeak-ng/speak_lib.h"

HomerState::HomerState() : formantFrequencyRescaler ("ffrescale", "formant frequency rescale")
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

    // ownership passed to juce in AudioProcessor constructor, which handles deletion
    currentVoiceParam = new juce::AudioParameterChoice({"curvoice", 1}, "current Voice", voiceNames, 33);

    singParam = new juce::AudioParameterBool({"sing", 1}, "speak/sing", false);
    freezeParam = new juce::AudioParameterBool({"freeze", 1}, "freeze", false);

    phonemeRotationParam = new juce::AudioParameterFloat({"phonemerotation", 1}, "phoneme rotation", 0, 1, 0);
    phonemeStickParam = new juce::AudioParameterFloat({"phonemestick", 1}, "phoneme stick", 0, 1, 0);
    clockSpeed = new juce::AudioParameterFloat({"clockspeed", 1}, "clock speed", 0, 22050, 22050);
    amountOfAliasing = new juce::AudioParameterFloat({"aliasingamount", 1}, "aliasing amount", 0, 1, 0);
    wavetableShape = new juce::AudioParameterFloat({"wavetableshape", 1}, "wavetable shape", 0, 1, 0);

    params.push_back(currentVoiceParam);
    params.push_back(singParam);
    params.push_back (freezeParam);
    params.push_back(phonemeRotationParam);
    params.push_back(phonemeStickParam);
    params.push_back(clockSpeed);
    params.push_back(amountOfAliasing);

    params.push_back(formantFrequencyRescaler.start);
    params.push_back(formantFrequencyRescaler.end);
    params.push_back(formantFrequencyRescaler.curve);
}