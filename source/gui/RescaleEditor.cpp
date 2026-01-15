//
// Created by Arden on 1/12/2026.
//

#include "RescaleEditor.h"
#include "../state/RescaleParameters.h"

RescaleEditor::RescaleEditor (RescaleParameters& p) : parameters (p)
{
    sliders[0].setRange (parameters.start->range.start, parameters.start->range.end);
    sliders[1].setRange (parameters.curve->range.start, parameters.curve->range.end);
    sliders[2].setRange (parameters.end->range.start, parameters.end->range.end);

    sliders[0].setDoubleClickReturnValue (true, 0);
    sliders[1].setDoubleClickReturnValue (true, 0);
    sliders[2].setDoubleClickReturnValue (true, 1);

    for (auto& slider : sliders) {
        addAndMakeVisible (slider);
        slider.setSliderStyle (juce::Slider::SliderStyle::LinearBarVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.addListener (this);
    }

    startTimerHz (30);
}

RescaleEditor::~RescaleEditor()
{
}

void RescaleEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour (juce::Colours::white);
    float startY = drawArea.getBottom() - drawArea.getHeight() * parameters.start->get();
    float endY = drawArea.getBottom() - drawArea.getHeight() * parameters.end->get();

    auto path = juce::Path();
    path.startNewSubPath (drawArea.getX(), startY);
    for (int i = 1; i < 100; ++i) {
        auto x = i / 100.f * drawArea.getWidth() + drawArea.getX();
        auto y = drawArea.getBottom() - drawArea.getHeight() * parameters.rescale(i / 100.f);
        path.lineTo (x, y);
    }
    g.strokePath (path, juce::PathStrokeType(3));

    g.setColour (juce::Colours::black);
    auto radius = 5;
    g.fillEllipse (drawArea.getX() - radius, startY - radius, 2 *  radius, 2 * radius);
    g.fillEllipse (drawArea.getRight() - radius, endY - radius, 2 * radius, 2 * radius);
    g.setColour(juce::Colours::white);
    g.drawEllipse (drawArea.getX() - radius, startY - radius, 2 *  radius, 2 * radius, 3);
    g.drawEllipse (drawArea.getRight() - radius, endY - radius, 2 * radius, 2 * radius, 3);

}
void RescaleEditor::resized()
{
    auto w = getWidth() / 3;
    sliders[0].setBounds (getLocalBounds().withWidth (w));
    sliders[1].setBounds (getLocalBounds().withWidth (w).withX (w));
    sliders[2].setBounds (getLocalBounds().withWidth (w).withX (2*w));



    drawArea = getLocalBounds().withSizeKeepingCentre (getWidth() - 20, getHeight() - 20);
}
void RescaleEditor::timerCallback()
{
    repaint();
}
void RescaleEditor::sliderValueChanged (juce::Slider*)
{
    *parameters.start = sliders[0].getValue();
    *parameters.curve = sliders[1].getValue();
    *parameters.end = sliders[2].getValue();
}
void RescaleEditor::sliderDragEnded (juce::Slider* slider)
{
    SliderListener<juce::Slider>::sliderDragEnded (slider);
}
void RescaleEditor::sliderDragStarted (juce::Slider* slider)
{
    SliderListener<juce::Slider>::sliderDragStarted (slider);
}
