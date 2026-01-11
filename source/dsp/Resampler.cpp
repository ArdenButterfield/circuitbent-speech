//
// Created by Arden on 1/10/2026.
//

#include "Resampler.h"

Resampler::Resampler() : position (0), increment (0), realSampleRate (0), prevSample (0), inputSampleRate (0)
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

int Resampler::getNumSamplesNeeded (int bufferLength) const
{
    return floor(bufferLength * increment + position);
}

void Resampler::resampleIntoBuffer (float* destination, int destinationLength, const float* source, int sourceLength)
{
    int sourceI = 0;
    int destinationI = 0;
    for (destinationI = 0; destinationI < destinationLength; destinationI++) {
        destination[destinationI] = prevSample * position + prev2Sample * (1 - position);
        position = position + increment;
        while (position >= 1) {
            position -= 1;
            prev2Sample = prevSample;
            prevSample = source[sourceI];
            sourceI++;
            jassert (sourceI < sourceLength);
        }
    }
}

void Resampler::releaseResources()
{
}
