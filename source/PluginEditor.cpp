#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), lyricsEditor (processorRef.homerState), bendsPanel (processorRef.homerState)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible (lyricsEditor);
    addAndMakeVisible (bendsPanel);
    addAndMakeVisible (titlePanel);

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 400);
    setResizable (true, true);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds();
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    auto helloWorld = juce::String ("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (5);
    titlePanel.setBounds (area.withHeight (100));
    bendsPanel.setBounds (area.withTrimmedTop (titlePanel.getBottom() + 5).withLeft (std::max(getWidth() / 2, getRight() - 400)));
    lyricsEditor.setBounds (bendsPanel.getBounds().withLeft (area.getX()).withRight (bendsPanel.getX() - 5));
    inspectButton.setBounds (titlePanel.getBounds().withWidth (100).reduced (5));
}
