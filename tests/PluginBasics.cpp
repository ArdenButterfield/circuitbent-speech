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
    std::cout << "callback " << numsamples << std::endl;

    if (wav == nullptr)
    {
        return 1;
    }
    return 0;
}

TEST_CASE ("Basics of espeak", "[espeak]")
{
    const char* path = R"(D:\projects\circuitbent-speech\espeak-ng\espeak-ng-data)";
    espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_RETRIEVAL;
    int buflength = 500, options = 0;

    EspeakProcessorContext epContext;
    memset(&epContext, 0, sizeof(EspeakProcessorContext));

    initEspeakContext(&epContext);

    auto fs = espeak_Initialize (&epContext, output, buflength, path, options); // 22050 is default
    REQUIRE (fs > 0);

    char text[] = { "I'm just here to make a friend, okay!!! Name's Sea Man got a (voice) in the mix. When it comes to making friends, I got crazy magic tricks" };
    char voicename[] = { "English (America)" }; // Set voice by its name

    std::vector<float> samples;
    samples.clear();

    auto voiceResult = espeak_SetVoiceByName(&epContext, voicename);
    REQUIRE (voiceResult == EE_OK);

    espeak_SetSynthCallback(&epContext, testSynthCallback);

    void* user_data = &samples;
    unsigned int *identifier = nullptr;
    auto synthError = espeak_Synth(&epContext, text, buflength, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, identifier, user_data);

     REQUIRE (synthError == EE_OK);

     juce::Time::waitForMillisecondCounter(juce::Time::getMillisecondCounter() + 1000);
     std::cout << "writing samples\n";
     REQUIRE(samples.size() > 0);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (1, samples.size());

    for (int i = 0; i < samples.size(); i++)
    {
        buffer.setSample (0, i, samples[i]);
    }

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (juce::File(juce::File::getCurrentWorkingDirectory().getChildFile ("testHelloWorld.wav"))),
                                          fs,
                                          buffer.getNumChannels(),
                                          24,
                                          {},
                                          0));
    REQUIRE (writer != nullptr);
    writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
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
