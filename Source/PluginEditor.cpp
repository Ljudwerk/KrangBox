// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

namespace
{
    constexpr int headerHeight = 44;
}

KrangBoxEditor::KrangBoxEditor (KrangBoxProcessor& p)
    : AudioProcessorEditor (p), generic (p)
{
    addAndMakeVisible (generic);
    setResizable (true, true);
    setSize (520, 760);
}

void KrangBoxEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2b2b2b));

    auto header = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (juce::Colour (0xffb5874a)); // mässing – en försmak av fas 2
    g.fillRect (header.removeFromBottom (2));

    g.setColour (juce::Colour (0xffe8dcc4));
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("KRANGBOX", header.reduced (14, 0), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff8a8a8a));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Basmalen  |  prototyp fas 0", header.reduced (14, 0), juce::Justification::centredRight);
}

void KrangBoxEditor::resized()
{
    generic.setBounds (getLocalBounds().withTrimmedTop (headerHeight));
}
