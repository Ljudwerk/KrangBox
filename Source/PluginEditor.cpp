// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

namespace
{
    constexpr int topHeight = 230;
    constexpr int keyboardHeight = 110;
    constexpr int stegHeight = 300;
}

KrangBoxEditor::KrangBoxEditor (KrangBoxProcessor& p)
    : AudioProcessorEditor (p), processor (p), generic (p),
      steg (p.pattern, p.stegverket.currentStep),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    namespace id = krang::params::id;

    top.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (top);
    addAndMakeVisible (generic);

    cutoffAtt    = std::make_unique<Attachment> (p.state, id::cutoff,    top.cutoff.slider);
    resonanceAtt = std::make_unique<Attachment> (p.state, id::resonance, top.resonance.slider);
    driveAtt     = std::make_unique<Attachment> (p.state, id::distDrive, top.drive.slider);

    // Kammarväljaren: fyll med parameterns val innan den kopplas.
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p.state.getParameter (id::chamber)))
        top.chamber.addItemList (choice->choices, 1);
    chamberAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.state, id::chamber, top.chamber);

    // Stegverket
    addChildComponent (steg);
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p.state.getParameter (id::seqLength)))
        steg.length.addItemList (choice->choices, 1);
    seqRunAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.state, id::seqRun, steg.play);
    seqTempoAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.state, id::seqTempo, steg.tempo);
    seqSwingAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.state, id::seqSwing, steg.swing);
    seqLengthAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.state, id::seqLength, steg.length);

    top.cutoff.slider.onValueChange    = [this] { lastParam = id::cutoff; };
    top.resonance.slider.onValueChange = [this] { lastParam = id::resonance; };
    top.drive.slider.onValueChange     = [this] { lastParam = id::distDrive; };

    // Keyboard: C1–C7, datorns tangentbord (A W S E D ...) spelar från C2 – bra för bas.
    using namespace krang::ui;
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (24);
    keyboard.setKeyWidth (22.0f);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setVelocity (0.8f, true);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, colours::meterFace);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, colours::bakelite);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, colours::brassDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::tubeOrange.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::brass.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setWantsKeyboardFocus (true);
    addAndMakeVisible (keyboard);

    for (auto* b : { &octaveDown, &octaveUp })
    {
        b->setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
        b->setColour (juce::TextButton::textColourOffId, colours::engraving);
        addAndMakeVisible (b);
    }
    octaveDown.onClick = [this] { shiftOctave (-1); };
    octaveUp.onClick   = [this] { shiftOctave (+1); };

    keyboardHint.setColour (juce::Label::textColourId, colours::brass);
    keyboardHint.setFont (panelFont (11.0f, false));
    keyboardHint.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (keyboardHint);
    shiftOctave (0);

    lastParam = id::cutoff;
    setResizable (true, true);
    setResizeLimits (820, 600, 1600, 1500);
    setSize (900, 820);
    startTimerHz (30);
}

KrangBoxEditor::~KrangBoxEditor()
{
    top.setLookAndFeel (nullptr);
}

void KrangBoxEditor::paint (juce::Graphics& g)
{
    g.fillAll (krang::ui::colours::chassis);
}

void KrangBoxEditor::resized()
{
    auto r = getLocalBounds();
    top.setBounds (r.removeFromTop (topHeight));
    if (showSteg)
        steg.setBounds (r.removeFromTop (stegHeight));

    auto kb = r.removeFromBottom (keyboardHeight).reduced (12, 8);
    auto bar = kb.removeFromTop (26);
    octaveDown.setBounds (bar.removeFromLeft (64));
    bar.removeFromLeft (6);
    octaveUp.setBounds (bar.removeFromLeft (64));
    bar.removeFromLeft (12);
    keyboardHint.setBounds (bar);
    kb.removeFromTop (4);
    keyboard.setBounds (kb);

    generic.setBounds (r);
}

void KrangBoxEditor::shiftOctave (int delta)
{
    keyOctave = juce::jlimit (1, 7, keyOctave + delta);
    keyboard.setKeyPressBaseOctave (keyOctave);
    const auto lowest = juce::MidiMessage::getMidiNoteName (keyOctave * 12, true, true, 4);
    keyboardHint.setText (juce::String::fromUTF8 ("Spela med musen, eller klicka på keyboardet och använd datorns tangenter "
                                                  "A W S E D F T G Y H U J K (från ") + lowest + ").",
                          juce::dontSendNotification);
}

void KrangBoxEditor::showParameter (const char* paramId)
{
    auto* param = processor.state.getParameter (paramId);
    auto* mode    = processor.state.getParameter (krang::params::id::distMode);
    auto* chamber = processor.state.getParameter (krang::params::id::chamber);
    if (param == nullptr || mode == nullptr || chamber == nullptr)
        return;

    top.display.setLines (chamber->getCurrentValueAsText() + "  " + mode->getCurrentValueAsText(),
                          param->getName (24) + "  " + param->getCurrentValueAsText()
                              + " " + param->getLabel());
}

void KrangBoxEditor::timerCallback()
{
    top.meter.setLevel (processor.outputPeak.exchange (0.0f));

    // Visa Stegverket bara när Hammarverket spelar.
    const bool drums = processor.state.getRawParameterValue (krang::params::id::chamber)->load() > 0.5f;
    if (drums != showSteg)
    {
        showSteg = drums;
        steg.setVisible (drums);
        resized();
    }
    showParameter (lastParam.toRawUTF8());
}
