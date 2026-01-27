//
// Created by Arden on 1/8/2026.
//

#ifndef HOMER_HOMERPROCESSOR_H
#define HOMER_HOMERPROCESSOR_H

#include "juce_audio_basics/juce_audio_basics.h"
#include "../state/HomerState.h"
#include "EspeakThread.h"
#include "Resampler.h"

class HomerProcessor
{
public:
    HomerProcessor(HomerState& homerState);
    ~HomerProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlockExpected);
    void setText(const juce::String &text);
    void processBlock(juce::AudioSampleBuffer &buffer, unsigned int startSample, unsigned int numSamples, bool startNewNote);
    void releaseResources();
private:
    void setUpNextEspeakThread();

    juce::AudioBuffer<float> inputBuffer;
    std::unique_ptr<EspeakThread> currentEspeakThread;
    std::unique_ptr<EspeakThread> nextEspeakThread;
    int samplerate;
    Resampler resampler;
    HomerState& homerState;
};

#endif //HOMER_HOMERPROCESSOR_H
