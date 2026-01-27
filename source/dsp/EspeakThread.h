//
// Created by Arden on 1/9/2026.
//

#ifndef HOMER_ESPEAKTHREAD_H
#define HOMER_ESPEAKTHREAD_H

#include "juce_core/juce_core.h"
#include "../state/HomerState.h"

#include <espeak-ng/speak_lib.h>

class EspeakThread : public juce::Thread
{
public:
    EspeakThread(HomerState& homer_state);
    ~EspeakThread() override;
    void resetEspeakContext();

    void endNote();

    void run() override;

    void setBendParametersFromState();
    void setOutputBuffer(float* ptr, int numSamples);
    void process();
    EspeakProcessorContext epContext;
    HomerState& homerState;

    bool readyToGo;
};

#endif //HOMER_ESPEAKTHREAD_H
