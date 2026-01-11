//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_HOMERSTATE_H
#define HOMER_HOMERSTATE_H

#include <vector>
#include <juce_core/juce_core.h>

struct HomerState
{
    HomerState();

    juce::String lyrics;
    juce::StringArray voiceNames;
    int currentVoice = 0;
};

#endif //HOMER_HOMERSTATE_H
