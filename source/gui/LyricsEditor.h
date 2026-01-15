//
// Created by Arden on 1/14/2026.
//

#ifndef HOMER_LYRICSEDITOR_H
#define HOMER_LYRICSEDITOR_H

#include "LyricLineEditor.h"

class LyricsEditor : public juce::Component, public juce::Timer, public juce::Slider::Listener
{
public:
    explicit LyricsEditor(HomerState& homerState);
    ~LyricsEditor() override;
private:
    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    void sliderValueChanged(juce::Slider*) override;

    std::array<std::unique_ptr<LyricLineEditor>, HomerState::numLyricLines> lyrics;

    juce::Slider currentLyricSlider;
    HomerState& homerState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricsEditor);
};

#endif //HOMER_LYRICSEDITOR_H
