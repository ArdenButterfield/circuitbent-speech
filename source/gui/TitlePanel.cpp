//
// Created by Arden on 1/14/2026.
//

#include "TitlePanel.h"
TitlePanel::TitlePanel()
{
    title.setName ("Plugin Name: Homer");
    byline.setName ("Plugin byline: Wildergarden Audio");
    title.setText ("Homer", juce::dontSendNotification);
    byline.setText ("Wildergarden Audio", juce::dontSendNotification);
    title.setFont (40);
    title.setJustificationType (juce::Justification::centred);
    byline.setFont (20);
    byline.setJustificationType (juce::Justification::centred);
    title.setColour (juce::Label::textColourId, juce::Colours::black);
    byline.setColour (juce::Label::textColourId, juce::Colours::darkgrey);

    addAndMakeVisible (title);
    addAndMakeVisible (byline);
}
TitlePanel::~TitlePanel()
{
}
void TitlePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::yellow);
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds());
}
void TitlePanel::resized()
{
    title.setBounds (getLocalBounds().withHeight (getHeight() * 0.75));
    byline.setBounds (getLocalBounds().withTop (title.getBottom()));
}