//
// Created by Arden on 1/8/2026.
//

#ifndef HOMER_HOMERPROCESSOR_H
#define HOMER_HOMERPROCESSOR_H

#include "juce_audio_basics/juce_audio_basics.h"
#include "EspeakThread.h"

class HomerProcessor
{
public:
    HomerProcessor();
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void setBendParameters(int paramID, float paramValue);
    void setText(const juce::String &text);
    void processBlock(juce::AudioSampleBuffer &buffer, unsigned int startSample, unsigned int numSamples, bool startNewNote);
    void releaseResources();
private:
    std::vector<float> samples;
    std::unique_ptr<EspeakThread> espeakThread;
};

#endif //HOMER_HOMERPROCESSOR_H
