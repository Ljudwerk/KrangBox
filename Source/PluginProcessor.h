// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "presets/PresetManager.h"
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

    // DAW:ens programlista = fabrikspresets. Egna presets nås från gränssnittet.
    int getNumPrograms() override { return presets.getNumFactory(); }
    int getCurrentProgram() override { return juce::jlimit (0, presets.getNumFactory() - 1, presets.getCurrentIndex()); }
    void setCurrentProgram (int index) override { presets.load (index); }
    const juce::String getProgramName (int index) override
    {
        const auto& e = presets.getEntries();
        return juce::isPositiveAndBelow (index, (int) e.size()) ? e[(size_t) index].name : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;

    // Senaste toppnivå på utgången, läses (och nollställs) av mätaren i gränssnittet.
    std::atomic<float> outputPeak { 0.0f };

    // Kaosskopet: de senaste utgångssamplen (vänster kanal), skrivna av ljudtråden och lästa av oscilloskopet.
    struct ScopeTap
    {
        static constexpr int size = 4096;
        std::array<std::atomic<float>, size> samples {};
        std::atomic<int> writeIndex { 0 };

        void push (const float* data, int n)
        {
            int w = writeIndex.load (std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
            {
                samples[(size_t) w].store (data[i], std::memory_order_relaxed);
                w = (w + 1) & (size - 1);
            }
            writeIndex.store (w, std::memory_order_release);
        }
    };
    ScopeTap scope;

    // Antal händelser i Kaosmotorn; kaoslampan blinkar när det ändras.
    std::atomic<juce::uint32> kaosEvents { 0 };

    // Senast spelade ton; Radiomastens skala ställer in nålen efter den.
    std::atomic<int> lastNote { -1 };
    // Ordet som Automaten säger just nu (−1 = tyst), för panelen.
    std::atomic<int> automatWord { -1 };
    // Modulationskällornas senaste värden (LFO 1–2, envelope 1–2, modhjul, aftertouch, anslag) för lamporna.
    std::array<std::atomic<float>, 7> modSources {};

    // Det virtuella keyboardet i gränssnittet skriver hit; noterna blandas in i MIDI-flödet.
    juce::MidiKeyboardState keyboardState;

    // Stegverkets mönster (gränssnittet skriver, ljudtråden läser) och sekvensern själv.
    krang::StepPattern pattern;
    krang::Stegverket stegverket;
    krang::Slingan slingan;
    // Tonstegverket: melodin för alla kammare utom Hammarverket.
    krang::TonPattern melody;
    krang::TonStegverket tonverk;

    // Fabrikspresets och egna presets.
    krang::PresetManager presets { state, pattern, &melody };

private:
    krang::params::Reader reader { state };
    krang::KrangEngine engine;
    juce::AudioBuffer<float> scratchRight; // höger kanal när värden bara ger en monobuss

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxProcessor)
};
