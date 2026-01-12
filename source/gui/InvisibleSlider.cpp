//
// Created by Arden on 1/12/2026.
//

#include "InvisibleSlider.h"
InvisibleSlider::InvisibleSlider()
{
}
InvisibleSlider::~InvisibleSlider()
{
}
void InvisibleSlider::paint (juce::Graphics& g)
{
    if (isMouseOverOrDragging ()) {
        g.fillAll(juce::Colours::white.withAlpha (0.4f));
    }
}