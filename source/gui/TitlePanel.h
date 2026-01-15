//
// Created by Arden on 1/14/2026.
//

#ifndef HOMER_TITLEPANEL_H
#define HOMER_TITLEPANEL_H

#include "juce_gui_basics/juce_gui_basics.h"

class TitlePanel : public juce::Component
{
public:
    TitlePanel();
    ~TitlePanel();
private:
    void paint(juce::Graphics& g) override;
    void resized() override;
    juce::Label title;
    juce::Label byline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitlePanel);
};

#endif //HOMER_TITLEPANEL_H
