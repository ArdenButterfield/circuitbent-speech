#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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

int testSynthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
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
    auto fs = espeak_Initialize (output, buflength, path, options); // 22050 is default
    REQUIRE (fs > 0);

    char text[] = { "Hello world!" };
    char voicename[] = { "English (America)" }; // Set voice by its name

    auto voiceResult = espeak_SetVoiceByName(voicename);
    REQUIRE (voiceResult == EE_OK);

    espeak_SetSynthCallback(testSynthCallback);

    void* user_data = nullptr;
    unsigned int *identifier = nullptr;
    auto synthError = espeak_Synth(text, buflength, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, identifier, user_data);

    REQUIRE (synthError == EE_OK);
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
