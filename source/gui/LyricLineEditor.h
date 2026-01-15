//
// Created by Arden on 1/10/2026.
//

#ifndef HOMER_LYRICEDITOR_H
#define HOMER_LYRICEDITOR_H

#include "../state/HomerState.h"
#include "juce_gui_basics/juce_gui_basics.h"

class LyricLineEditor : public juce::Component, public juce::TextEditor::Listener, public juce::ComboBox::Listener
{
public:
    LyricLineEditor(HomerState& homerState, int lineNumber);
    ~LyricLineEditor();
private:
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;

    void comboBoxChanged(juce::ComboBox*) override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::TextEditor textEditor;
    juce::ComboBox voiceSelect;

    HomerState& homerState;

    const int lineNumber;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricLineEditor);
};

#endif //HOMER_LYRICEDITOR_H
