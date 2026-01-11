//
// Created by Arden on 1/11/2026.
//

#include "BendsPanel.h"

BendsPanel::BendsPanel(HomerState& hs) : homerState (hs), singButton ("Speak/Sing")
{
    addAndMakeVisible (phonemeStickLabel);
    addAndMakeVisible (phonemeStickSlider);


    addAndMakeVisible (singLabel);
    addAndMakeVisible (singButton);

    addAndMakeVisible (phonemeRotationLabel);
    addAndMakeVisible (phonemeRotationSlider);

    phonemeStickSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
    phonemeRotationSlider.setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);

    phonemeStickSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 40);
    phonemeRotationSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 40);

    phonemeRotationSlider.setRange (
        homerState.phonemeRotationParam->getNormalisableRange().start, homerState.phonemeRotationParam->getNormalisableRange().end);
    phonemeStickSlider.setRange (
        homerState.phonemeStickParam->getNormalisableRange().start, homerState.phonemeStickParam->getNormalisableRange().end);

    phonemeRotationLabel.setText ("Phoneme Rotation", juce::dontSendNotification);
    phonemeStickLabel.setText ("Phoneme Stick", juce::dontSendNotification);
    singLabel.setText ("Speak/Sing", juce::dontSendNotification);

    phonemeStickSlider.addListener (this);
    phonemeRotationSlider.addListener (this);
    singButton.addListener (this);

    startTimerHz (30);

    phonemeStickSlider.setColour (juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);
    phonemeRotationSlider.setColour (juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);
    phonemeStickSlider.setColour (juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);
    phonemeRotationSlider.setColour (juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);

    phonemeRotationLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::black);
    phonemeStickLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::black);
    singLabel.setColour (juce::Label::ColourIds::textColourId, juce::Colours::black);

    singButton.setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colours::black);
    singButton.setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colours::black);

}
BendsPanel::~BendsPanel()
{
}

void BendsPanel::timerCallback()
{
    phonemeStickSlider.setValue (*homerState.phonemeStickParam);
    phonemeRotationSlider.setValue (*homerState.phonemeRotationParam);
    singButton.setToggleState (*homerState.singParam, juce::sendNotification);
}

void BendsPanel::sliderValueChanged (juce::Slider*)
{
    *homerState.phonemeStickParam = phonemeStickSlider.getValue();
    *homerState.phonemeRotationParam = phonemeRotationSlider.getValue();
}

void BendsPanel::buttonClicked (juce::Button*)
{
    *homerState.singParam = singButton.getToggleState();
}

void BendsPanel::buttonStateChanged (juce::Button* button)
{
    *homerState.singParam = singButton.getToggleState();
}

void BendsPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::yellow);
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds());
}

void BendsPanel::resized()
{
    auto paramBounds = juce::Rectangle<int>(0,0,getWidth() / 3, getHeight());
    auto labelHeight = 50;

    singLabel.setBounds (paramBounds.withHeight (labelHeight));
    singButton.setBounds (paramBounds.withTrimmedTop (labelHeight));

    phonemeRotationLabel.setBounds (paramBounds.withX(paramBounds.getWidth()).withHeight (labelHeight));
    phonemeRotationSlider.setBounds (paramBounds.withX(paramBounds.getWidth()).withTrimmedTop (labelHeight));

    phonemeStickLabel.setBounds (paramBounds.withX(paramBounds.getWidth() * 2).withHeight (labelHeight));
    phonemeStickSlider.setBounds (paramBounds.withX(paramBounds.getWidth() * 2).withTrimmedTop (labelHeight));
}