// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include <atomic>

class KrangBoxProcessor final : public juce::AudioProcessor
{
public:
    KrangBoxProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;

    // Senaste toppnivå på utgången, läses (och nollställs) av mätaren i gränssnittet.
    std::atomic<float> outputPeak { 0.0f };

    // Det virtuella keyboardet i gränssnittet skriver hit; noterna blandas in i MIDI-flödet.
    juce::MidiKeyboardState keyboardState;

    // Stegverkets mönster (gränssnittet skriver, ljudtråden läser) och sekvensern själv.
    krang::StepPattern pattern;
    krang::Stegverket stegverket;

private:
    krang::params::Reader reader { state };
    krang::KrangEngine engine;
    juce::AudioBuffer<float> scratchRight; // höger kanal när värden bara ger en monobuss

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxProcessor)
};
