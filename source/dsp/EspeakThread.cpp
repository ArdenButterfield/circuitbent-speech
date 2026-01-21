//
// Created by Arden on 1/9/2026.
//

#include "EspeakThread.h"
#include "espeak-ng/espeak_ng.h"
#include "windows.h"

EspeakThread::EspeakThread(HomerState& hs) : Thread("EspeakThread"), homerState(hs)
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
    auto language = homerState.voiceNames[*homerState.languageSelectors[*homerState.lyricSelector - 1]];
    auto voiceResult = espeak_SetVoiceByName(&epContext, language.toRawUTF8());
    jassert (voiceResult == 0);
    std::vector<float> samples;
    samples.clear();

    espeak_SetSynthCallback(&epContext, synthCallback);

    void* user_data = &samples;
    unsigned int *identifier = nullptr;

    auto lyrics = homerState.lyrics[*homerState.lyricSelector - 1].toStdString();

    // epContext.bends.debugPrintEverything = true;

    auto synthError = espeak_Synth(&epContext, lyrics.c_str(), 500, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, identifier, user_data);
    jassert (synthError == 0);
    epContext.allDone = true;
    epContext.doneProcessing = true;
    WakeByAddressSingle(&epContext.doneProcessing);
}
void EspeakThread::setBendParametersFromState()
{
    if (homerState.singParam->get()) {
        espeak_ng_SetConstF0(&epContext, homerState.keyFrequency);
    }

    epContext.bends.rotatePhonemes = homerState.phonemeRotationParam->get() * 10;
    epContext.bends.stickChance = homerState.phonemeStickParam->get();
    epContext.bends.freeze = homerState.freezeParam->get();
    epContext.bends.wavetableShape = homerState.wavetableShape->get();
    epContext.bends.detuneHarmonics = homerState.detuneHarmonics->get();
    epContext.bends.pitchbendMultiplier = std::pow(2.0f, *homerState.pitchBend / 12.f);
    epContext.bends.vibratoAmount = *homerState.vibrato;

    epContext.bends.formantFrequencyRescaler.start = *homerState.formantFrequencyRescaler.start;
    epContext.bends.formantFrequencyRescaler.end = *homerState.formantFrequencyRescaler.end;
    epContext.bends.formantFrequencyRescaler.curve = *homerState.formantFrequencyRescaler.curve;

    epContext.bends.formantHeightRescaler.start = *homerState.formantHeightRescaler.start;
    epContext.bends.formantHeightRescaler.end = *homerState.formantHeightRescaler.end;
    epContext.bends.formantHeightRescaler.curve = *homerState.formantHeightRescaler.curve;

    if (*homerState.consonantVowelBlend > 0) {
        epContext.bends.vowelLevel = 1;
        epContext.bends.consonantLevel = 1 - *homerState.consonantVowelBlend;
    } else {
        epContext.bends.consonantLevel = 1;
        epContext.bends.vowelLevel = *homerState.consonantVowelBlend + 1;
    }
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
