//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_LYRICEDITOR_H
#define HOMER_LYRICEDITOR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "../state/HomerState.h"

class LyricEditor : public juce::Component, public juce::TextEditor::Listener
{
public:
    LyricEditor(HomerState& homerState);
    ~LyricEditor();
private:
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;
    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::TextEditor textEditor;
    juce::ComboBox comboBox;

    HomerState& homerState;
};

#endif //HOMER_LYRICEDITOR_H
