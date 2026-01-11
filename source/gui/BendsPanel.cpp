//
// Created by Arden on 1/11/2026.
//

#include "BendsPanel.h"

BendsPanel::BendsPanel(HomerState& hs) : homerState (hs)
{
    bendParameters.push_back (homerState.phonemeStickParam);
    bendParameters.push_back (homerState.phonemeRotationParam);
    bendParameters.push_back (homerState.clockSpeed);

    toggleParameters.push_back (homerState.singParam);

    for (auto& bendParameter : bendParameters) {
        auto slider = std::make_unique<juce::Slider>();
        addAndMakeVisible (slider.get());
        slider->setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 40);
        slider->setRange (bendParameter->getNormalisableRange().start, bendParameter->getNormalisableRange().end);
        slider->addListener (this);

        auto label = std::make_unique<juce::Label>();
        addAndMakeVisible (label.get());
        label->setText (bendParameter->getName (50), juce::dontSendNotification);
        label->setJustificationType (juce::Justification::centred);

        label->setColour (juce::Label::ColourIds::textColourId, juce::Colours::black);
        slider->setColour (juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colours::black);

        bendSliders.push_back (std::move(slider));
        bendSliderLabels.push_back (std::move (label));
    }

    for (auto& toggleParameter : toggleParameters) {
        auto button = std::make_unique<juce::ToggleButton>(toggleParameter->getName (50));
        addAndMakeVisible (button.get());

        button->addListener (this);

        button->setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colours::black);
        button->setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colours::black);

        toggleButtons.push_back (std::move(button));
    }

    startTimerHz (30);
}
BendsPanel::~BendsPanel()
{
}

void BendsPanel::timerCallback()
{
    for (int i = 0; i < bendParameters.size(); i++) {
        bendSliders[i]->setValue (*bendParameters[i]);
    }

    for (int i = 0; i < toggleParameters.size(); i++) {
        toggleButtons[i]->setToggleState (*toggleParameters[i], juce::dontSendNotification);
    }
}

void BendsPanel::sliderValueChanged (juce::Slider*)
{
    for (int i = 0; i < bendParameters.size(); i++) {
        *bendParameters[i] = bendSliders[i]->getValue();
    }
}

void BendsPanel::buttonClicked (juce::Button*)
{
    for (auto i = 0; i < toggleParameters.size(); i++) {
        *toggleParameters[i] = toggleButtons[i]->getToggleState();
    }
}

void BendsPanel::buttonStateChanged (juce::Button* button)
{
}

void BendsPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::yellow);
    g.setColour (juce::Colours::black);
    g.drawRect (toggleZone);
    g.drawRect (sliderZone);
}

void BendsPanel::resized()
{
    auto usableArea = getLocalBounds().withTrimmedTop (5).withTrimmedLeft (5).withTrimmedRight (5).withTrimmedBottom (5);
    toggleZone = usableArea.withWidth (200);
    sliderZone = usableArea.withLeft (toggleZone.getRight() + 10);
    auto paramBounds = juce::Rectangle<int>(0,0,getWidth() / 3, getHeight());
    auto labelHeight = 50;

    for (int i = 0; i < bendParameters.size(); i++) {
        auto zone = sliderZone.withWidth (sliderZone.getWidth() / bendParameters.size())
            .withX (sliderZone.getX() + sliderZone.getWidth() * i / bendParameters.size());
        bendSliderLabels[i]->setBounds (zone.withHeight (50));
        bendSliders[i]->setBounds (zone.withTrimmedTop (55));
    }

    for (int i = 0; i < toggleParameters.size(); i++) {
        auto zone = toggleZone.withHeight (40).withY (toggleZone.getY() + i * 40);
        toggleButtons[i]->setBounds (zone);
    }
}