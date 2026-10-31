// Fas 0: Krang-stilprovet ovanpå ett generiskt testgränssnitt med alla parametrar.
// Hela panelen ersätts av Krang-panelen i fas 2 (Philip).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/KrangStyle.h"
#include "ui/StegverketPanel.h"
#include <juce_audio_utils/juce_audio_utils.h>

class KrangBoxEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KrangBoxEditor (KrangBoxProcessor&);
    ~KrangBoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showParameter (const char* paramId);

    KrangBoxProcessor& processor;
    krang::ui::KrangLookAndFeel lookAndFeel;
    krang::ui::KrangTopPanel top;
    juce::GenericAudioProcessorEditor generic;

    // Stegverket visas när Hammarverket är vald kammare.
    krang::ui::StegverketPanel steg;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqRunAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqTempoAtt, seqSwingAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> seqLengthAtt;
    bool showSteg = false;

    // Virtuellt keyboard för att provspela utan MIDI-keyboard.
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton octaveDown { "OKT -" }, octaveUp { "OKT +" };
    juce::Label keyboardHint;
    void shiftOctave (int delta);
    int keyOctave = 3; // datortangentbordets lägsta C = keyOctave * 12 (3 -> C2)

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> cutoffAtt, resonanceAtt, driveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chamberAtt;

    juce::String lastParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxEditor)
};
