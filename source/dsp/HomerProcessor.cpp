//
// Created by Arden on 1/8/2026.
//

#include "HomerProcessor.h"

HomerProcessor::HomerProcessor()
{
}

void HomerProcessor::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    espeakThread = std::make_unique<EspeakThread> ();
}

void HomerProcessor::setBendParameters (int paramID, float paramValue)
{
}
void HomerProcessor::setText (const juce::String& text)
{
}

void HomerProcessor::processBlock (juce::AudioSampleBuffer& buffer, unsigned int startSample, unsigned int numSamples, bool startNewNote)
{
    jassert (startSample + numSamples <= buffer.getNumSamples());
    auto ptr = buffer.getWritePointer(0) + startSample;

    if (startNewNote && espeakThread->isThreadRunning()) {
        espeakThread->endNote();
    }

    if (startNewNote) {
        espeakThread->resetEspeakContext();
        auto started = espeakThread->startThread();
        jassert (started && espeakThread->isThreadRunning());
    }

    if (espeakThread->isThreadRunning()) {
        espeakThread->setOutputBuffer (ptr, numSamples);
        espeakThread->process();

        for (int channel = 1; channel < buffer.getNumChannels(); ++channel) {
            buffer.copyFrom (channel, startSample, ptr, numSamples);
        }
    }
}

void HomerProcessor::releaseResources()
{
}