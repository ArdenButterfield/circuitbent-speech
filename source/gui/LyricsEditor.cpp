//
// Created by Arden on 1/14/2026.
//

#include "LyricsEditor.h"
LyricsEditor::LyricsEditor (HomerState& hs) : homerState (hs)
{
    currentLyricSlider.setRange (homerState.lyricSelector->getRange().getStart(), homerState.lyricSelector->getRange().getEnd(), 1);
    currentLyricSlider.setTitle ("Index of current lyric");
    currentLyricSlider.setWantsKeyboardFocus (true);
    currentLyricSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0,0);
    currentLyricSlider.setSliderStyle (juce::Slider::LinearVertical);
    currentLyricSlider.addListener (this);

    addAndMakeVisible (currentLyricSlider);

    for (int i = 0; i < HomerState::numLyricLines; i++) {
        lyrics[i] = std::make_unique<LyricLineEditor> (homerState, i);
        addAndMakeVisible (lyrics[i].get());
    }
}
LyricsEditor::~LyricsEditor()
{
}
void LyricsEditor::paint (juce::Graphics& g)
{
    Component::paint (g);
}
void LyricsEditor::resized()
{
    auto area = getLocalBounds();
    currentLyricSlider.setBounds (area.withWidth (20));

    for (int i = 0; i < HomerState::numLyricLines; i++) {
        lyrics[i]->setBounds (area
            .withTrimmedLeft (currentLyricSlider.getWidth())
            .withHeight(area.getHeight() / HomerState::numLyricLines)
            .withY (area.getY() + area.getHeight() * i / HomerState::numLyricLines));
    }
}
void LyricsEditor::timerCallback()
{
    currentLyricSlider.setValue (*homerState.lyricSelector, juce::dontSendNotification);
}
void LyricsEditor::sliderValueChanged (juce::Slider*)
{
    homerState.lyricSelector->beginChangeGesture();
    *homerState.lyricSelector = currentLyricSlider.getValue();
    homerState.lyricSelector->endChangeGesture();
}