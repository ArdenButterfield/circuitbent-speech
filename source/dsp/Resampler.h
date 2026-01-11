//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_RESAMPLER_H
#define HOMER_RESAMPLER_H
#include "juce_audio_basics/juce_audio_basics.h"

class Resampler
{
public:
    Resampler();
    ~Resampler();

    void prepareToPlay(double realSampleRate);
    void setInputSamplerate(float fs);
    int getNumSamplesNeeded(int bufferLength) const;
    void resampleIntoBuffer(float* destination, int destinationLength, const float* source, int sourceLength);
    void releaseResources();
private:
    float position;
    float increment;
    float inputSampleRate;
    float realSampleRate;
    float prevSample;
    float prev2Sample;
};

#endif //HOMER_RESAMPLER_H
