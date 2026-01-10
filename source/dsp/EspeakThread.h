//
// Created by Arden on 1/9/2026.
//

#ifndef HOMER_ESPEAKTHREAD_H
#define HOMER_ESPEAKTHREAD_H

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <espeak-ng/speak_lib.h>

class EspeakThread : public juce::Thread
{
public:
    EspeakThread();
    ~EspeakThread() override;
    void resetEspeakContext();

    void run() override;

    void setOutputBuffer(juce::AudioBuffer<float>& buffer);
    EspeakProcessorContext epContext;
private:
    juce::AudioBuffer<float>* outputBuffer;
};

#endif //HOMER_ESPEAKTHREAD_H
