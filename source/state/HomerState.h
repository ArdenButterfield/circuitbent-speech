//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_HOMERSTATE_H
#define HOMER_HOMERSTATE_H

#include <vector>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

struct HomerState
{
    HomerState();

    juce::String lyrics;
    juce::StringArray voiceNames;

    juce::AudioParameterChoice* currentVoiceParam;
    juce::AudioParameterBool* singParam;
    juce::AudioParameterFloat* phonemeRotationParam;
    juce::AudioParameterFloat* phonemeStickParam;

    const std::vector<juce::AudioProcessorParameter*> params = {
        currentVoiceParam,
        singParam,
        phonemeRotationParam,
        phonemeStickParam
    };

    float keyFrequency = 0;
};

#endif //HOMER_HOMERSTATE_H
