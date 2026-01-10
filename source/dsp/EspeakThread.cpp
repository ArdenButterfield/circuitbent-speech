//
// Created by Arden on 1/9/2026.
//

#include "EspeakThread.h"
#include "windows.h"

EspeakThread::EspeakThread() : Thread("EspeakThread"), outputBuffer(nullptr)
{
    resetEspeakContext();
}
EspeakThread::~EspeakThread()
{
}
void EspeakThread::resetEspeakContext()
{
    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
    int buflength = 500, options = 0;

    memset(&epContext, 0, sizeof(EspeakProcessorContext));

    initEspeakContext(&epContext);

    auto fs = espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default
}

int synthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
    std::cout << "callback\n";
    if (wav == nullptr)
    {
        return 1;
    }
    return 0;

}


void EspeakThread::run()
{
    // char text[] = { "I'm just here to make a friend, okay!!! Name's Sea Man got a (voice) in the mix. When it comes to making friends, I got crazy magic tricks" };
    char text[] = { "She sells seashells by the seashore." };
    char voicename[] = { "English (America)" }; // Set voice by its name

    auto voiceResult = espeak_SetVoiceByName(&epContext, voicename);

    std::vector<float> samples;
    samples.clear();

    espeak_SetSynthCallback(&epContext, synthCallback);

    void* user_data = &samples;
    unsigned int *identifier = nullptr;

    auto synthError = espeak_Synth(&epContext, text, 500, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, identifier, user_data);
    epContext.allDone = true;
    epContext.doneProcessing = true;
    WakeByAddressSingle(&epContext.doneProcessing);
}

void EspeakThread::setOutputBuffer (juce::AudioBuffer<float>& buffer)
{
    outputBuffer = &buffer;
}
