//
// Created by Arden on 1/9/2026.
//

#include "EspeakThread.h"
#include "windows.h"

EspeakThread::EspeakThread() : Thread("EspeakThread")
{
    resetEspeakContext(22050);
}

EspeakThread::~EspeakThread()
{
}

void EspeakThread::resetEspeakContext(int fs)
{
    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
    int buflength = 500, options = 0;

    memset(&epContext, 0, sizeof(EspeakProcessorContext));

    initEspeakContext(&epContext);

    espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default
    // epContext.samplerate = fs;
}

void EspeakThread::endNote()
{
    epContext.noteEndingEarly = true;
    epContext.readyToProcess = true;
    WakeByAddressSingle(&epContext.readyToProcess);
    stopThread (1);
}

int synthCallback(short *wav, int, espeak_EVENT*)
{
    if (wav == nullptr) {
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

void EspeakThread::setOutputBuffer (float* ptr, int numSamples)
{
    epContext.pluginBuffer = ptr;
    epContext.pluginBufferSize = numSamples;
    epContext.pluginBufferPosition = 0;
}

void EspeakThread::process()
{
    epContext.readyToProcess = true;
    epContext.doneProcessing = false;
    epContext.allDone = false;

    bool stillProcessing = false;

    WakeByAddressSingle(&epContext.readyToProcess);

    while (epContext.doneProcessing == stillProcessing)
    {
        WaitOnAddress(&epContext.doneProcessing, &stillProcessing, sizeof(bool), INFINITE);
    }
}
