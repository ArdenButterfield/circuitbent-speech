//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_HOMERSTATE_H
#define HOMER_HOMERSTATE_H

#include <vector>
#include <array>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "RescaleParameters.h"

struct HomerState
{
    static constexpr int numLyricLines = 8;

    HomerState();

    std::array<juce::String, numLyricLines> lyrics;
    std::array<juce::AudioParameterChoice*, numLyricLines> languageSelectors;

    juce::AudioParameterInt* lyricSelector;

    juce::StringArray voiceNames;

    // juce::AudioParameterChoice* currentVoiceParam;

    juce::AudioParameterBool* singParam;
    juce::AudioParameterBool* freezeParam;
    juce::AudioParameterBool* killParam;

    juce::AudioParameterFloat* phonemeRotationParam;
    juce::AudioParameterFloat* phonemeStickParam;
    juce::AudioParameterFloat* clockSpeed;
    juce::AudioParameterFloat* amountOfAliasing;
    juce::AudioParameterFloat* wavetableShape;
    juce::AudioParameterFloat* clockCurrentStealing;
    juce::AudioParameterFloat* detuneHarmonics;
    juce::AudioParameterFloat* pitchBend;
    juce::AudioParameterFloat* vibrato;
    juce::AudioParameterFloat* consonantVowelBlend;

    RescaleParameters formantFrequencyRescaler;
    RescaleParameters formantHeightRescaler;

    std::vector<juce::AudioProcessorParameter*> params;

    std::vector<int> currentMidiNotes;
    float keyFrequency = 0;
    bool noteCurrentlyDown = false;

    float peakLevel;
    float rmsLevel;

};

#endif //HOMER_HOMERSTATE_H
