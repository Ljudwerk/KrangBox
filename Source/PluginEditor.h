// Krang-panelen (fas 2): hela gränssnittet ritas i en fast designstorlek (1200 × 850)
// och skalas proportionellt när fönstret ändrar storlek (60–160 %).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/KrangStyle.h"
#include "ui/Panels.h"

class KrangBoxEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KrangBoxEditor (KrangBoxProcessor&);
    ~KrangBoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 1200, designHeight = 850;

private:
    void timerCallback() override;
    void layoutContent();
    void bindAll();
    void bindSlider (juce::Slider&, const juce::String& paramId);
    void bindCombo (juce::ComboBox&, const juce::String& paramId);
    void showParameter (const juce::String& paramId);
    void shiftOctave (int delta);
    void fillPresetBox();
    void showSaveDialog();

    KrangBoxProcessor& processor;
    krang::ui::KrangLookAndFeel lookAndFeel;

    // Allt innehåll ligger i en komponent i designstorlek som skalas med en transform.
    juce::Component content;
    krang::ui::KrangTopPanel top;
    krang::ui::BasmalenPanel basmalen;
    krang::ui::HammarverketPanel hammarverket;
    krang::ui::BordunorgelnPanel bordunorgeln;
    krang::ui::SpolslinganPanel spolslingan;
    krang::ui::AngledningenPanel angledningen;

    // Virtuellt keyboard för att provspela utan MIDI-keyboard.
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton octaveDown { "OKT -" }, octaveUp { "OKT +" };
    juce::Label keyboardHint;
    int keyOctave = 3; // datortangentbordets lägsta C = keyOctave * 12 (3 -> C2)

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqRunAtt;

    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::AlertWindow> saveDialog;
    juce::String lastParam { "distDrive" }, shownPreset;
    int shownChamber = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxEditor)
};
