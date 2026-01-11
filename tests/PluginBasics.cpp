#include "dsp/EspeakThread.h"
#include "dsp/Resampler.h"
#include "helpers/test_helpers.h"

#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vector>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("Homer"));
    }
}

#include <espeak-ng/speak_lib.h>
#include <juce_audio_formats/juce_audio_formats.h>

int testSynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
    auto samples = static_cast<std::vector<float>*>(events->user_data);
    for (int i = 0; i < numsamples; i++)
    {
        samples->push_back (wav[i] / 32768.0f);
    }

    if (wav == nullptr)
    {
        return 1;
    }
    return 0;
}

int prepareForEpBendTest(EspeakProcessorContext& epContext)
{
    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCHRONOUS;
    int buflength = 500, options = 0;

    memset(&epContext, 0, sizeof(EspeakProcessorContext));

    initEspeakContext(&epContext);

    auto fs = espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default
    REQUIRE (fs > 0);

    return fs;
}

void carryOut(EspeakProcessorContext& epContext, int fs, juce::String outFileName)
{
    // char text[] = { "I'm just here to make a friend, okay!!! Name's Sea Man got a (voice) in the mix. When it comes to making friends, I got crazy magic tricks" };
    char text[] = { "She sells seashells by the seashore." };
    char voicename[] = { "English (America)" }; // Set voice by its name

    auto voiceResult = espeak_SetVoiceByName(&epContext, voicename);
    REQUIRE (voiceResult == EE_OK);

    std::vector<float> samples;
    samples.clear();

    espeak_SetSynthCallback(&epContext, testSynthCallback);

    void* user_data = &samples;
    unsigned int *identifier = nullptr;
    auto synthError = espeak_Synth(&epContext, text, 500, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, identifier, user_data);

    REQUIRE (synthError == EE_OK);

    // juce::Time::waitForMillisecondCounter(juce::Time::getMillisecondCounter() + 400);
    REQUIRE(samples.size() > 0);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (1, samples.size());

    for (int i = 0; i < samples.size(); i++)
    {
        buffer.setSample (0, i, samples[i]);
    }

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (juce::File(juce::File::getCurrentWorkingDirectory().getChildFile (outFileName))),
                                          fs,
                                          buffer.getNumChannels(),
                                          24,
                                          {},
                                          0));
    REQUIRE (writer != nullptr);
    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

TEST_CASE ("Basics of espeak", "[espeak]")
{
    EspeakProcessorContext epContext;
    auto fs = prepareForEpBendTest(epContext);
    carryOut (epContext, fs, juce::String("testOutHelloWorld.wav"));
}
TEST_CASE ("Rotate phonemes", "[espeak-rotphonemes]")
{
    EspeakProcessorContext epContext;
    auto fs = prepareForEpBendTest(epContext);
    epContext.bends.rotatePhonemes = 1;
    carryOut (epContext, fs, juce::String("testRotatePhonemes.wav"));
}
TEST_CASE ("stick chance", "[espeak-stickchance]")
{
    EspeakProcessorContext epContext;
    auto fs = prepareForEpBendTest(epContext);
    epContext.bends.stickChance = 0.5;
    carryOut (epContext, fs, juce::String("testStickChance.wav"));
}

TEST_CASE("debug print it all", "[printeverything]")
{
    EspeakProcessorContext epContext;
    auto fs = prepareForEpBendTest(epContext);
    epContext.bends.debugPrintEverything = true;
    carryOut (epContext, fs, juce::String("debugPrintEverything.wav"));
}

TEST_CASE("constant pitch", "[constantf0]")
{
    EspeakProcessorContext epContext;
    auto fs = prepareForEpBendTest(epContext);
    epContext.bends.bendPitch = 440.0f;
    carryOut (epContext, fs, juce::String("debugContstantPitch.wav"));

}

#include <windows.h> // mutex

TEST_CASE("Espeak thread", "[espeakthread]")
{
    EspeakThread espeakThread;
    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> finalOutBuffer;
    finalOutBuffer.setSize(1, 40000);
    finalOutBuffer.clear();
    buffer.setSize (1, 1024);

    espeakThread.epContext.pluginBuffer = buffer.getWritePointer (0);
    espeakThread.epContext.pluginBufferSize = buffer.getNumSamples();
    espeakThread.epContext.pluginBufferPosition = 0;

    espeakThread.epContext.readyToProcess = true;
    espeakThread.epContext.doneProcessing = false;
    espeakThread.epContext.allDone = false;

    espeakThread.startThread ();

    bool stillProcessing = false;
    int finalBufferI = 0;
    while (!espeakThread.epContext.allDone)
    {
        while (espeakThread.epContext.doneProcessing == stillProcessing)
        {
            WaitOnAddress(&espeakThread.epContext.doneProcessing, &stillProcessing, sizeof(bool), INFINITE);
        }
        espeakThread.epContext.doneProcessing = false;
        REQUIRE(buffer.getMagnitude (0,0,buffer.getNumSamples()) > 0);
        finalOutBuffer.copyFrom (0, finalBufferI, buffer, 0, 0, buffer.getNumSamples());
        finalBufferI += buffer.getNumSamples();
        buffer.clear();
        buffer.setNotClear();
        espeakThread.epContext.readyToProcess = true;
        WakeByAddressSingle(&espeakThread.epContext.readyToProcess);

    }

    juce::Time::waitForMillisecondCounter(juce::Time::getMillisecondCounter() + 400);

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (juce::File(juce::File::getCurrentWorkingDirectory().getChildFile ("espeakDirectToBuffer.wav"))),
                                          22050,
                                          finalOutBuffer.getNumChannels(),
                                          24,
                                          {},
                                          0));
    REQUIRE (writer != nullptr);
    writer->writeFromAudioSampleBuffer (finalOutBuffer, 0, finalOutBuffer.getNumSamples());
}

TEST_CASE("Ending early", "[espeakthreadendearly]")
{
    EspeakThread espeakThread;
    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> finalOutBuffer;
    finalOutBuffer.setSize(1, 40000);
    finalOutBuffer.clear();
    buffer.setSize (1, 1024);

    espeakThread.epContext.pluginBuffer = buffer.getWritePointer (0);
    espeakThread.epContext.pluginBufferSize = buffer.getNumSamples();
    espeakThread.epContext.pluginBufferPosition = 0;

    espeakThread.epContext.readyToProcess = true;
    espeakThread.epContext.doneProcessing = false;
    espeakThread.epContext.allDone = false;

    espeakThread.startThread ();

    bool stillProcessing = false;
    int finalBufferI = 0;
    int cycleI = 0;
    while (!espeakThread.epContext.allDone)
    {
        if (cycleI == 2)
        {
            espeakThread.epContext.noteEndingEarly = true;
            espeakThread.epContext.readyToProcess = true;
            WakeByAddressSingle(&espeakThread.epContext.readyToProcess);
            bool exitedNormally = espeakThread.stopThread (1);
            REQUIRE(exitedNormally);
            break;
        }
        while (espeakThread.epContext.doneProcessing == stillProcessing)
        {
            WaitOnAddress(&espeakThread.epContext.doneProcessing, &stillProcessing, sizeof(bool), INFINITE);
        }
        espeakThread.epContext.doneProcessing = false;
        REQUIRE(buffer.getMagnitude (0,0,buffer.getNumSamples()) > 0);
        finalOutBuffer.copyFrom (0, finalBufferI, buffer, 0, 0, buffer.getNumSamples());
        finalBufferI += buffer.getNumSamples();
        buffer.clear();
        buffer.setNotClear();
        espeakThread.epContext.readyToProcess = true;
        WakeByAddressSingle(&espeakThread.epContext.readyToProcess);
        cycleI++;
    }


    juce::Time::waitForMillisecondCounter(juce::Time::getMillisecondCounter() + 400);

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (juce::File(juce::File::getCurrentWorkingDirectory().getChildFile ("espeakDirectToBufferEndearly.wav"))),
                                          22050,
                                          finalOutBuffer.getNumChannels(),
                                          24,
                                          {},
                                          0));
    REQUIRE (writer != nullptr);
    writer->writeFromAudioSampleBuffer (finalOutBuffer, 0, finalOutBuffer.getNumSamples());
}

TEST_CASE("No pops on resampler", "[resampler]")
{
    Resampler resampler;
    resampler.prepareToPlay (44100);
    resampler.setInputSamplerate (22050);

    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> inBuffer;
    buffer.setSize (1, 480);
    inBuffer.setSize (1, 500);
    float angle = 0;
    float next, prev = 0;
    float next_out, prev_out = 0;
    for (auto iter = 0; iter < 5; ++iter) {
        auto numSamplesNeeded = resampler.getNumSamplesNeeded (buffer.getNumSamples());
        for (auto samp = 0; samp < numSamplesNeeded; ++samp) {
            next = sin (angle);
            REQUIRE(abs(next - prev) < 0.02);
            inBuffer.setSample(0, samp, next);
            prev = next;
            angle += 0.01;
            while (angle > 2 * M_PI) {
                angle -= 2 * M_PI;
            }
        }
        resampler.resampleIntoBuffer (buffer.getWritePointer (0,0), buffer.getNumSamples(), inBuffer.getReadPointer (0,0), numSamplesNeeded);
        for (auto samp = 0; samp < buffer.getNumSamples(); ++samp) {
            next_out = buffer.getSample (0, samp);
            REQUIRE(abs(next_out - prev_out) < 0.02);
            prev_out = next_out;
        }

    }
    resampler.prepareToPlay (44100);
    resampler.setInputSamplerate (2000);

    buffer.setSize (1, 67);
    inBuffer.setSize (1, 500);
    angle = 0;
    prev = 0;
    prev_out = 0;
    for (auto iter = 0; iter < 20; ++iter) {
        auto numSamplesNeeded = resampler.getNumSamplesNeeded (buffer.getNumSamples());
        for (auto samp = 0; samp < numSamplesNeeded; ++samp) {
            next = sin (angle);
            REQUIRE(abs(next - prev) < 0.02);
            inBuffer.setSample(0, samp, next);
            prev = next;
            angle += 0.01;
            while (angle > 2 * M_PI) {
                angle -= 2 * M_PI;
            }
        }
        resampler.resampleIntoBuffer (buffer.getWritePointer (0,0), buffer.getNumSamples(), inBuffer.getReadPointer (0,0), numSamplesNeeded);
        for (auto samp = 0; samp < buffer.getNumSamples(); ++samp) {
            next_out = buffer.getSample (0, samp);
            std::cout << next_out << " ";
            // REQUIRE(abs(next_out - prev_out) < 0.002);
            prev_out = next_out;
        }

    }
}

#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    #if defined(__APPLE__)
        // macOS uses 2021.9.1 from pip wheel (only x86_64 version available)
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2021.9.1 (r0x7e208212)"));
    #else
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2022.3.0 (r0x0fc08bb1)"));
    #endif
}
#endif
