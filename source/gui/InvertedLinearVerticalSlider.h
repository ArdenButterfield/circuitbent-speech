//
// Created by Arden on 1/14/2026.
//

#ifndef HOMER_INVERTEDLINEARVERTICALSLIDER_H
#define HOMER_INVERTEDLINEARVERTICALSLIDER_H

#include "juce_gui_basics/juce_gui_basics.h"

class InvertedLinearVerticalSlider : public juce::Slider
{
public:
    InvertedLinearVerticalSlider()
        : juce::Slider (juce::Slider::LinearVertical, juce::Slider::NoTextBox)
    {
    }
protected:
    // Value -> screen position
    double valueToProportionOfLength (double value) override
    {
        return 1.0 - juce::Slider::valueToProportionOfLength (value);
    }

    // Screen position -> value
    double proportionOfLengthToValue (double proportion) override
    {
        return juce::Slider::proportionOfLengthToValue (1.0 - proportion);
    }
};

#endif //HOMER_INVERTEDLINEARVERTICALSLIDER_H
