//
// Created by Arden on 1/10/2026.
//

#include "Resampler.h"

#include "juce_dsp/juce_dsp.h"

Resampler::Resampler() : position (0), increment (0), realSampleRate (0), prevSample (0), prev2Sample (0), inputSampleRate (0), aliasingAmount (0)
{
}
Resampler::~Resampler()
{
}
void Resampler::prepareToPlay (double realfs)
{
    if (inputSampleRate <= 0) {
        inputSampleRate = realfs;
    }
    realSampleRate = realfs;
    increment = 1;
    position = 0.5;
    prevSample = 0;
    prev2Sample = 0;
}
void Resampler::setInputSamplerate (float fs)
{
    inputSampleRate = std::min(fs, realSampleRate);
    increment = inputSampleRate / realSampleRate;
}
void Resampler::setAliasingAmount (float amount)
{
    aliasingAmount = amount;
}
float Resampler::interpolate (float a, float b, float x) const
{
    jassert (0 <= x);
    jassert (x <= 1);
    jassert (0 <= aliasingAmount);
    jassert (aliasingAmount <= 1);

    // for high frequencies, we don't want to have exponential decay.
    // so from input sampling rate = 4000 to input sampling rate = 20000 we will shift to only using the first half of the range of interpolation curves.

    auto shift = std::min(std::max(0.0f, (inputSampleRate - 4000) / (20000 - 4000)), 1.0f);
    auto scaledAliasingAmount = aliasingAmount / (shift + 1);

    if (scaledAliasingAmount < 0.5) {
        auto lerp = a * (1 - x) + b * x;
        auto stepAmount = scaledAliasingAmount * 2;
        auto lerpAmount = 1 - stepAmount;
        return lerp * lerpAmount + a * stepAmount;
    }

    return a * exp (-x * 10 * (scaledAliasingAmount - 0.5));
}

int Resampler::getNumSamplesNeeded (int bufferLength) const
{
    return floor(bufferLength * increment + position);
}

void Resampler::resampleIntoBuffer (float* destination, int destinationLength, const float* source, int sourceLength)
{
    int sourceI = 0;
    int destinationI = 0;
    for (destinationI = 0; destinationI < destinationLength; destinationI++) {
        destination[destinationI] = interpolate(prev2Sample, prevSample, position);
        position = position + increment;
        while (position >= 1) {
            position -= 1;
            prev2Sample = prevSample;
            prevSample = source[sourceI];
            sourceI++;
            // jassert (sourceI < sourceLength);
        }
    }
}

void Resampler::releaseResources()
{
}
