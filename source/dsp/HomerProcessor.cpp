//
// Created by Arden on 1/8/2026.
//

#include "HomerProcessor.h"

HomerProcessor::HomerProcessor()
{
}

void HomerProcessor::prepareToPlay (double fs, int samplesPerBlockExpected)
{
    resampler.prepareToPlay (fs);
    resampler.setInputSamplerate (22050);

    samplerate = static_cast<int>(fs);
    espeakThread = std::make_unique<EspeakThread> ();
    inputBuffer.setSize (1, samplesPerBlockExpected * 2);
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
        espeakThread->resetEspeakContext(samplerate);
        auto started = espeakThread->startThread();
        jassert (started && espeakThread->isThreadRunning());
    }

    if (espeakThread->isThreadRunning()) {
        inputBuffer.clear();
        auto numInputSamples = resampler.getNumSamplesNeeded (numSamples);
        if (numInputSamples > inputBuffer.getNumSamples()) {
            inputBuffer.setSize (1, numInputSamples);
        }

        espeakThread->setOutputBuffer (inputBuffer.getWritePointer (0), numInputSamples);
        espeakThread->process();

        resampler.resampleIntoBuffer (ptr, numSamples, inputBuffer.getReadPointer (0), numInputSamples);

        for (int channel = 1; channel < buffer.getNumChannels(); ++channel) {
            buffer.copyFrom (channel, startSample, ptr, numSamples);
        }
    }
}

void HomerProcessor::releaseResources()
{
}