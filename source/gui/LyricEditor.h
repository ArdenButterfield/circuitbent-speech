//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_LYRICEDITOR_H
#define HOMER_LYRICEDITOR_H

#include "../state/HomerState.h"
#include "juce_gui_basics/juce_gui_basics.h"

class LyricEditor : public juce::Component, public juce::TextEditor::Listener, public juce::ComboBox::Listener
{
public:
    LyricEditor(HomerState& homerState);
    ~LyricEditor();
private:
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;

    void comboBoxChanged(juce::ComboBox*) override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::TextEditor textEditor;
    juce::ComboBox voiceSelect;

    HomerState& homerState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricEditor);
};

#endif //HOMER_LYRICEDITOR_H
