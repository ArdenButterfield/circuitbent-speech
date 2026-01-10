//
// Created by Arden on 1/9/2026.
//

#ifndef HOMER_ESPEAKTHREAD_H
#define HOMER_ESPEAKTHREAD_H

#include "juce_core/juce_core.h"
#include <espeak-ng/speak_lib.h>

class EspeakThread : public juce::Thread
{
public:
    EspeakThread();
    ~EspeakThread() override;
    void resetEspeakContext();

    void endNote();

    void run() override;

    void setOutputBuffer(float* ptr, int numSamples);
    void process();
    EspeakProcessorContext epContext;
};

#endif //HOMER_ESPEAKTHREAD_H
