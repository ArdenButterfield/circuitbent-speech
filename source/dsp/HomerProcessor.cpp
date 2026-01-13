//
// Created by Arden on 1/8/2026.
//

#include "HomerProcessor.h"

HomerProcessor::HomerProcessor(HomerState& hs) : homerState(hs)
{
}

void HomerProcessor::prepareToPlay (double fs, int samplesPerBlockExpected)
{
    resampler.prepareToPlay (fs);
    resampler.setInputSamplerate (22050);

    samplerate = static_cast<int>(fs);
    espeakThread = std::make_unique<EspeakThread> (homerState);
    inputBuffer.setSize (1, samplesPerBlockExpected * 2);
}

void HomerProcessor::setText (const juce::String& text)
{
}

void HomerProcessor::processBlock (juce::AudioSampleBuffer& buffer, unsigned int startSample, unsigned int numSamples, bool startNewNote)
{
    jassert (startSample + numSamples <= buffer.getNumSamples());

    auto speedDuck = 1 - 4 * homerState.peakLevel * *homerState.clockCurrentStealing;
    speedDuck = std::max (speedDuck, 0.1f);

    resampler.setInputSamplerate (*homerState.clockSpeed * speedDuck);
    resampler.setAliasingAmount (*homerState.amountOfAliasing);
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

        espeakThread->setBendParametersFromState();

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