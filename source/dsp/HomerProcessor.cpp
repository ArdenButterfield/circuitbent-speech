//
// Created by Arden on 1/8/2026.
//

#include "HomerProcessor.h"

HomerProcessor::HomerProcessor(HomerState& hs) : samplerate (0), homerState (hs)
{
}
HomerProcessor::~HomerProcessor()
{
    releaseResources();
}

void HomerProcessor::prepareToPlay (double fs, int samplesPerBlockExpected)
{
    resampler.prepareToPlay (fs);
    resampler.setInputSamplerate (22050);

    samplerate = static_cast<int>(fs);
    nextEspeakThread = std::make_unique<EspeakThread> (homerState);
    setUpNextEspeakThread();
    currentEspeakThread.reset ();
    inputBuffer.setSize (1, samplesPerBlockExpected * 2);
}

void HomerProcessor::setText (const juce::String& text)
{
}

void HomerProcessor::processBlock (juce::AudioSampleBuffer& buffer, unsigned int startSample, unsigned int numSamples, bool startNewNote)
{
    resetNextEspeakThreadIfNeeded();

    jassert (startSample + numSamples <= buffer.getNumSamples());

    auto speedDuck = 1 - 4 * homerState.peakLevel * *homerState.clockCurrentStealing;
    speedDuck = std::max (speedDuck, 0.1f);

    resampler.setInputSamplerate (*homerState.clockSpeed * speedDuck);
    resampler.setAliasingAmount (*homerState.amountOfAliasing);
    auto ptr = buffer.getWritePointer(0) + startSample;

    if ((startNewNote || homerState.killParam->get()) && currentEspeakThread && currentEspeakThread->isThreadRunning()) {
        currentEspeakThread->endNote();
    }

    if (startNewNote) {
        currentEspeakThread = std::move(nextEspeakThread);
        setUpNextEspeakThread();
        jassert (currentEspeakThread);
        jassert (currentEspeakThread->isThreadRunning());

        while (!currentEspeakThread->readyToGo) {
            currentEspeakThread->notify();
        }
    }

    if (currentEspeakThread && currentEspeakThread->isThreadRunning()) {
        inputBuffer.clear();
        auto numInputSamples = resampler.getNumSamplesNeeded (numSamples);
        if (numInputSamples > inputBuffer.getNumSamples()) {
            inputBuffer.setSize (1, numInputSamples);
        }

        currentEspeakThread->setOutputBuffer (inputBuffer.getWritePointer (0), numInputSamples);

        currentEspeakThread->setBendParametersFromState();

        currentEspeakThread->process();

        resampler.resampleIntoBuffer (ptr, numSamples, inputBuffer.getReadPointer (0), numInputSamples);

        for (int channel = 1; channel < buffer.getNumChannels(); ++channel) {
            buffer.copyFrom (channel, startSample, ptr, numSamples);
        }
    }
}

void HomerProcessor::releaseResources()
{
    while (currentEspeakThread && currentEspeakThread->isThreadRunning()) {
        currentEspeakThread->endNote();
    }
    while (nextEspeakThread && nextEspeakThread->isThreadRunning()) {
        nextEspeakThread->endNote();
    }
    currentEspeakThread.reset();
    nextEspeakThread.reset();
}
void HomerProcessor::setUpNextEspeakThread()
{
    while (nextEspeakThread && nextEspeakThread->isThreadRunning()) {
        nextEspeakThread->endNote();
    }
    nextEspeakThread = std::make_unique<EspeakThread> (homerState);
    auto threadStarted = nextEspeakThread->startThread();
    jassert (threadStarted);
}

void HomerProcessor::resetNextEspeakThreadIfNeeded()
{
    if (nextEspeakThread && nextEspeakThread->isThreadRunning() && nextEspeakThread->readyToWait &&
        (nextEspeakThread->language != homerState.voiceNames[*homerState.languageSelectors[*homerState.lyricSelector - 1]] ||
        nextEspeakThread->lyrics != homerState.lyrics[*homerState.lyricSelector - 1].toStdString())) {
        setUpNextEspeakThread();
    }
}