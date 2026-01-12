//
// Created by Arden on 1/12/2026.
//

#ifndef HOMER_INVISIBLESLIDER_H
#define HOMER_INVISIBLESLIDER_H

#include "juce_gui_basics/juce_gui_basics.h"

class InvisibleSlider : public juce::Slider
{
public:
    InvisibleSlider();
    ~InvisibleSlider() override;
    void paint (juce::Graphics& g) override;
};

#endif //HOMER_INVISIBLESLIDER_H
