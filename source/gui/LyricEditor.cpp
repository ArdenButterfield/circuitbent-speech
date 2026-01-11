//
// Created by Arden on 1/10/2026.
//

#include "LyricEditor.h"

LyricEditor::LyricEditor(HomerState& hs) : homerState(hs)
{
    addAndMakeVisible (textEditor);
    textEditor.addListener (this);
    textEditor.setReturnKeyStartsNewLine (false);

    addAndMakeVisible (voiceSelect);
    voiceSelect.addListener (this);
    voiceSelect.addItemList (homerState.voiceNames, 1);
}

LyricEditor::~LyricEditor()
{
}
void LyricEditor::textEditorReturnKeyPressed (juce::TextEditor& text_editor)
{
    std::cout << text_editor.getText() << std::endl;
    homerState.lyrics = text_editor.getText();
}
void LyricEditor::textEditorEscapeKeyPressed (juce::TextEditor& text_editor)
{
    text_editor.setText (homerState.lyrics);
}
void LyricEditor::comboBoxChanged (juce::ComboBox*)
{
    auto id = voiceSelect.getSelectedItemIndex();
    homerState.currentVoice = std::min(std::max(0, id), homerState.voiceNames.size() - 1);
}

void LyricEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::yellow);
}

void LyricEditor::resized()
{
    textEditor.setBounds (0, 0, getWidth(), 50);
    voiceSelect.setBounds (textEditor.getBounds().withY (textEditor.getBottom()));
}
