//
// Created by Arden on 1/12/2026.
//

#ifndef HOMER_RESCALEPARAMETERS_H
#define HOMER_RESCALEPARAMETERS_H

#include "juce_audio_processors/juce_audio_processors.h"

struct RescaleParameters
{
    RescaleParameters(const juce::String baseID, const juce::String name);
    juce::AudioParameterFloat* start;
    juce::AudioParameterFloat* end;
    juce::AudioParameterFloat* curve;

    juce::String name;

    float rescale(float x) const;
};

#endif //HOMER_RESCALEPARAMETERS_H
