// Krang-panelen (fas 2): hela gränssnittet ritas i en fast designstorlek (1200 × 970)
// och skalas proportionellt när fönstret ändrar storlek (60–160 %).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/KrangStyle.h"
#include "ui/Panels.h"
#include "ui/ModulationPanel.h"
#include "ui/TonStegverketPanel.h"

class KrangBoxEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KrangBoxEditor (KrangBoxProcessor&);
    ~KrangBoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 1200, designHeight = 970;

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
    krang::ui::StalpianoPanel stalpiano;
    krang::ui::KlockgjuterietPanel klockor;
    krang::ui::KorenPanel koren;
    krang::ui::RadiomastenPanel radio;
    krang::ui::LurblasarenPanel lur;
    krang::ui::LutverketPanel lut;
    krang::ui::VattenpumpenPanel pump;
    krang::ui::AngledningenPanel angledningen;
    krang::ui::KaosPanel kaos;
    krang::ui::ModulationPanel modulation;
    krang::ui::TonStegverketPanel tonsteg;

    // Virtuellt keyboard för att provspela utan MIDI-keyboard.
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton octaveDown { "OKT -" }, octaveUp { "OKT +" };
    juce::Label keyboardHint;
    // Växlar mellan kammarmodulen, Tonstegverket och Modulationsraden på samma plats.
    juce::TextButton showChamber { "KAMMARE" }, showSteg { "STEGVERKET" }, showModulation { "MODULATION" };
    enum class View { Kammare, Steg, Modulation };
    View view = View::Kammare;
    int shownModCount = -1;
    void updateCentreView();
    int keyOctave = 3; // datortangentbordets lägsta C = keyOctave * 12 (3 -> C2)

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqRunAtt, tonRunAtt;

    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::AlertWindow> saveDialog;
    juce::String lastParam { "distDrive" }, shownPreset;
    int shownChamber = -1;
    juce::uint32 shownKaosEvents = 0;
    std::array<float, KrangBoxProcessor::ScopeTap::size> scopeCopy {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxEditor)
};
