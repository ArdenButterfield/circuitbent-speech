//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_HOMERSTATE_H
#define HOMER_HOMERSTATE_H

#include <vector>
#include <array>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "RescaleParameters.h"
#include "juce_dsp/juce_dsp.h"
struct HomerState
{
    class LyricsListener
    {
    public:
        virtual ~LyricsListener() = default;
        virtual void lyricsChangeCallback() {}
    };


    static constexpr int numLyricLines = 8;

    const juce::String defaultDataDirectory =
#if JUCE_WINDOWS
     R"(C:/ProgramData/Homer)";
#elif JUCE_LINUX
    R"(~/.config/Homer)";
#elif JUCE_MAC
    R"(~/Library/Application Support/Homer)";
#endif


    HomerState();

    void exportToXml(juce::XmlElement* parent) const
    {
        auto paramsElement = parent->createNewChildElement ("params");

        for (int i = 0; i < params.size(); ++i) {
            paramsElement->setAttribute (juce::String("param-") + juce::String(i), params[i]->getValue());
        }

        auto lyricsElement = parent->createNewChildElement ("lyrics");

        for (int i = 0; i < lyrics.size(); ++i) {
            auto lElement = lyricsElement->createNewChildElement ("line-" + std::to_string(i));
            lElement->addTextElement (lyrics[i]);
        }

    }

    void importFromXml(juce::XmlElement* parent)
    {
        for (auto& child : parent->getChildIterator()) {
            if (child->getTagName() == "params") {
                for (int i = 0; i < params.size(); ++i) {
                    auto pValue = child->getDoubleAttribute (juce::String("param-") + juce::String(i), -1);
                    if (pValue >= 0 && pValue <= 1) {
                        params[i]->beginChangeGesture();
                        params[i]->setValueNotifyingHost (pValue);
                        params[i]->endChangeGesture();
                    }
                }
            } else if (child->getTagName() == "lyrics") {
                for (int lyricI = 0; lyricI < lyrics.size(); ++lyricI) {
                    auto lyricChild = child->getChildByName ("line-" + std::to_string(lyricI));
                    if (lyricChild) {
                        lyrics[lyricI] = lyricChild->getAllSubText();
                    }
                }
                for (auto& listener : lyricsListeners) {
                    listener->lyricsChangeCallback();
                }
            } else {
                jassertfalse;
            }
        }
    }

    void addLyricsListener(LyricsListener* listener)
    {
        lyricsListeners.push_back(listener);
    }

    void removeLyricsListener(LyricsListener* listener)
    {
        auto it = std::find(lyricsListeners.begin(), lyricsListeners.end(), listener);

        if (it != lyricsListeners.end())
            lyricsListeners.erase(it);
    }

    std::array<juce::String, numLyricLines> lyrics;
    std::array<juce::AudioParameterChoice*, numLyricLines> languageSelectors;

    juce::AudioParameterInt* lyricSelector;

    juce::StringArray voiceNames;

    // juce::AudioParameterChoice* currentVoiceParam;

    juce::AudioParameterBool* singParam;
    juce::AudioParameterBool* freezeParam;
    juce::AudioParameterBool* killParam;

    juce::AudioParameterFloat* phonemeRotationParam;
    juce::AudioParameterFloat* phonemeStickParam;
    juce::AudioParameterFloat* clockSpeed;
    juce::AudioParameterFloat* amountOfAliasing;
    juce::AudioParameterFloat* wavetableShape;
    juce::AudioParameterFloat* clockCurrentStealing;
    juce::AudioParameterFloat* detuneHarmonics;
    juce::AudioParameterFloat* pitchBend;
    juce::AudioParameterFloat* vibrato;
    juce::AudioParameterFloat* consonantVowelBlend;

    RescaleParameters formantFrequencyRescaler;
    RescaleParameters formantHeightRescaler;

    std::vector<juce::AudioProcessorParameter*> params;

    std::vector<int> currentMidiNotes;
    float keyFrequency = 0;
    bool noteCurrentlyDown = false;

    float rmsLevel;
    juce::dsp::BallisticsFilter<float> inputTracker;

    std::vector<LyricsListener*> lyricsListeners;
};

#endif //HOMER_HOMERSTATE_H
