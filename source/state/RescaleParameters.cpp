//
// Created by Arden on 1/12/2026.
//

#include "RescaleParameters.h"
RescaleParameters::RescaleParameters(const juce::String baseID, const juce::String n) : name(n)
{
    start = new juce::AudioParameterFloat({baseID + "start", 1}, name + " start", 0, 1, 0);
    end = new juce::AudioParameterFloat({baseID + "end", 1}, name + " end", 0, 1, 1);
    curve = new juce::AudioParameterFloat({baseID + "curve", 1}, name + " curve", -1, 1, 0);
}
float RescaleParameters::rescale (float x) const
{
    float sloped = std::pow (x, std::pow(5.f, -*curve));
    return (*end - *start) * sloped + *start;
}