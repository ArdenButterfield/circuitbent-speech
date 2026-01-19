//
// Created by Arden on 1/10/2026.
//

#include "HomerState.h"

#include "espeak-ng/speak_lib.h"

HomerState::HomerState() : formantFrequencyRescaler ("ffrescale", "formant frequency rescale"), formantHeightRescaler("fhrescale", "formant height rescaler"), peakLevel (0), rmsLevel (0)
{
    EspeakProcessorContext epContext;

    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
    int buflength = 500, options = 0;
    memset(&epContext, 0, sizeof(EspeakProcessorContext));
    initEspeakContext(&epContext);
    espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default

    auto voices = espeak_ListVoices(&epContext, nullptr);
    for (int i = 0; voices[i] != nullptr; i++) {
        voiceNames.add((const char8_t* const)voices[i]->name);
    }

    lyricSelector = new juce::AudioParameterInt({"lyricsselector", 1}, "Lyric line", 1, numLyricLines, 1);

    for (int i = 0; i < numLyricLines; i++) {
        languageSelectors[i] = new juce::AudioParameterChoice(
            {juce::String("language") + juce::String(i)},
            juce::String("Language for line ") + juce::String(i),
            voiceNames, 33); // 33: default to american english
    }

    singParam = new juce::AudioParameterBool({"sing", 1}, "speak/sing", false);
    freezeParam = new juce::AudioParameterBool({"freeze", 1}, "freeze", false);
    killParam = new juce::AudioParameterBool({"kill", 1}, "kill", false);

    phonemeRotationParam = new juce::AudioParameterFloat({"phonemerotation", 1}, "phoneme rotation", 0, 1, 0);
    phonemeStickParam = new juce::AudioParameterFloat({"phonemestick", 1}, "phoneme stick", 0, 1, 0);
    clockSpeed = new juce::AudioParameterFloat({"clockspeed", 1}, "clock speed", 0, 22050, 22050);
    amountOfAliasing = new juce::AudioParameterFloat({"aliasingamount", 1}, "aliasing amount", 0, 1, 0);
    wavetableShape = new juce::AudioParameterFloat({"wavetableshape", 1}, "wavetable shape", 0, 1, 0);
    clockCurrentStealing = new juce::AudioParameterFloat({"clockcurrentstealing", 1}, "clock current stealing", 0, 1, 0);
    detuneHarmonics = new juce::AudioParameterFloat({"detuneharmonics", 1}, "detune harmonics", -1, 1, 0);
    pitchBend = new juce::AudioParameterFloat({"pitchbend", 1}, "pitch bend", -1, 1, 0);
    vibrato = new juce::AudioParameterFloat({"vibrato", 1}, "vibrato", 0, 1, 0);

    params.push_back(lyricSelector);
    for (int i = 0; i < numLyricLines; i++) {
        params.push_back(languageSelectors[i]);
    }
    params.push_back (singParam);
    params.push_back (freezeParam);
    params.push_back (killParam);

    params.push_back (phonemeRotationParam);
    params.push_back (phonemeStickParam);
    params.push_back (clockSpeed);
    params.push_back (amountOfAliasing);
    params.push_back (clockCurrentStealing);
    params.push_back (detuneHarmonics);
    params.push_back (pitchBend);
    params.push_back (vibrato);

    params.push_back(formantFrequencyRescaler.start);
    params.push_back(formantFrequencyRescaler.end);
    params.push_back(formantFrequencyRescaler.curve);

    params.push_back (formantHeightRescaler.start);
    params.push_back (formantHeightRescaler.end);
    params.push_back (formantHeightRescaler.curve);
}