//
// Created by Arden on 1/10/2026.
//

#include "LyricLineEditor.h"

LyricLineEditor::LyricLineEditor(HomerState& hs, int n) : homerState(hs), lineNumber (n)
{
    setTitle (
        juce::String("Lyric ") +
        juce::String(lineNumber) +
        juce::String(" editor"));

    addAndMakeVisible (textEditor);
    textEditor.addListener (this);
    textEditor.setReturnKeyStartsNewLine (false);

    addAndMakeVisible (voiceSelect);
    voiceSelect.addListener (this);
    voiceSelect.addItemList (homerState.voiceNames, 1);

    voiceSelect.addListener (this);

    voiceSelect.setSelectedItemIndex (homerState.languageSelectors[lineNumber]->getIndex());
}

LyricLineEditor::~LyricLineEditor()
{
}
void LyricLineEditor::textEditorReturnKeyPressed (juce::TextEditor& text_editor)
{
    std::cout << text_editor.getText() << std::endl;
    homerState.lyrics[lineNumber] = text_editor.getText();
}
void LyricLineEditor::textEditorEscapeKeyPressed (juce::TextEditor& text_editor)
{
    text_editor.setText (homerState.lyrics[lineNumber]);
}
void LyricLineEditor::comboBoxChanged (juce::ComboBox*)
{
    auto id = voiceSelect.getSelectedItemIndex();
    *homerState.languageSelectors[lineNumber] = std::min(std::max(0, id), homerState.voiceNames.size() - 1);
}

void LyricLineEditor::paint (juce::Graphics& g)
{

}

void LyricLineEditor::resized()
{
    textEditor.setBounds (getLocalBounds().withTrimmedRight (100));
    voiceSelect.setBounds (getLocalBounds().withTrimmedLeft (textEditor.getRight()));
}
