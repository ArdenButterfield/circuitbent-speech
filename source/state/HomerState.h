//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_HOMERSTATE_H
#define HOMER_HOMERSTATE_H

#include <vector>
#include <stack>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

struct HomerState
{
    HomerState();

    juce::String lyrics;
    juce::StringArray voiceNames;

    juce::AudioParameterChoice* currentVoiceParam;
    juce::AudioParameterBool* singParam;
    juce::AudioParameterBool* freezeParam;
    juce::AudioParameterFloat* phonemeRotationParam;
    juce::AudioParameterFloat* phonemeStickParam;
    juce::AudioParameterFloat* clockSpeed;
    juce::AudioParameterFloat* amountOfAliasing;

    std::vector<juce::AudioProcessorParameter*> params;

    std::vector<int> currentMidiNotes;
    float keyFrequency = 0;
    bool noteCurrentlyDown = false;
};

#endif //HOMER_HOMERSTATE_H
