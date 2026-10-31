// Parameterträdet: alla automationsbara parametrar och hur de mappas till motorn.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/KrangEngine.h"

namespace krang::params
{

// Parameter-ID:n är kontrakt med DAW-projekt och presets: byt ALDRIG namn på ett ID.
namespace id
{
    inline constexpr auto osc1Level   = "osc1Level";
    inline constexpr auto osc2Level   = "osc2Level";
    inline constexpr auto osc2Semi    = "osc2Semi";
    inline constexpr auto osc2Fine    = "osc2Fine";
    inline constexpr auto subLevel    = "subLevel";
    inline constexpr auto glide       = "glide";
    inline constexpr auto drift       = "drift";

    inline constexpr auto cutoff      = "cutoff";
    inline constexpr auto resonance   = "resonance";
    inline constexpr auto filterDrive = "filterDrive";
    inline constexpr auto envAmount   = "envAmount";
    inline constexpr auto keyTrack    = "keyTrack";

    inline constexpr auto ampA = "ampA", ampD = "ampD", ampS = "ampS", ampR = "ampR";
    inline constexpr auto fltA = "fltA", fltD = "fltD", fltS = "fltS", fltR = "fltR";

    inline constexpr auto distMode    = "distMode";
    inline constexpr auto distDrive   = "distDrive";
    inline constexpr auto distMix     = "distMix";
    inline constexpr auto distBits    = "distBits";
    inline constexpr auto distDown    = "distDown";
    inline constexpr auto distOut     = "distOut";

    inline constexpr auto master      = "master";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto add = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, const char* label = "")
    {
        p.push_back (std::make_unique<P> (ParameterID { pid, 1 }, name, r, def, AudioParameterFloatAttributes().withLabel (label)));
    };
    auto time = [] (float maxSec) { NormalisableRange<float> r (0.001f, maxSec); r.setSkewForCentre (maxSec * 0.1f); return r; };

    // Basmålen – oscillatorer
    add (id::osc1Level, "Osc 1 Sågtand", { 0.0f, 1.0f }, 0.8f);
    add (id::osc2Level, "Osc 2 Fyrkant", { 0.0f, 1.0f }, 0.5f);
    add (id::osc2Semi,  "Osc 2 Halvton", { -24.0f, 24.0f, 1.0f }, -12.0f, "st");
    add (id::osc2Fine,  "Osc 2 Fin",     { -50.0f, 50.0f }, 7.0f, "ct");
    add (id::subLevel,  "Sub",           { 0.0f, 1.0f }, 0.6f);
    add (id::glide,     "Glid",          { 0.0f, 1000.0f, 0.0f, 0.4f }, 60.0f, "ms");
    add (id::drift,     "Drift",         { 0.0f, 1.0f }, 0.3f);

    // Filter
    add (id::cutoff,      "Cutoff",       { 20.0f, 18000.0f, 0.0f, 0.25f }, 400.0f, "Hz");
    add (id::resonance,   "Resonans",     { 0.0f, 1.0f }, 0.45f);
    add (id::filterDrive, "Filterdrive",  { 1.0f, 4.0f }, 1.5f);
    add (id::envAmount,   "Env-mängd",    { -4.0f, 6.0f }, 3.0f, "okt");
    add (id::keyTrack,    "Tangentfölj",  { 0.0f, 1.0f }, 0.5f);

    // Envelopes
    add (id::ampA, "Amp Attack",  time (5.0f), 0.003f, "s");
    add (id::ampD, "Amp Decay",   time (5.0f), 0.25f,  "s");
    add (id::ampS, "Amp Sustain", { 0.0f, 1.0f }, 0.8f);
    add (id::ampR, "Amp Release", time (5.0f), 0.12f,  "s");
    add (id::fltA, "Filt Attack",  time (5.0f), 0.002f, "s");
    add (id::fltD, "Filt Decay",   time (5.0f), 0.30f,  "s");
    add (id::fltS, "Filt Sustain", { 0.0f, 1.0f }, 0.15f);
    add (id::fltR, "Filt Release", time (5.0f), 0.20f,  "s");

    // Krangdistortion
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::distMode, 1 }, "Distortion-läge",
                                                         StringArray { "Rör", "Fuzz", "Vikning", "Bitkross" }, 0));
    add (id::distDrive, "Drive",      { 0.0f, 48.0f }, 18.0f, "dB");
    add (id::distMix,   "Dist Mix",   { 0.0f, 1.0f }, 1.0f);
    add (id::distBits,  "Bitar",      { 1.0f, 16.0f, 1.0f }, 6.0f);
    add (id::distDown,  "Nedsampling",{ 1.0f, 32.0f, 1.0f }, 4.0f, "x");
    add (id::distOut,   "Dist Ut",    { -24.0f, 12.0f }, -6.0f, "dB");

    add (id::master, "Master", { -48.0f, 6.0f }, 0.0f, "dB");

    return { p.begin(), p.end() };
}

// Läser parameterträdet till motorns struktur. Körs i början av varje ljudblock.
class Reader
{
public:
    explicit Reader (juce::AudioProcessorValueTreeState& s) : state (s) {}

    EngineParams read() const
    {
        EngineParams e;
        auto& b = e.basmalen;
        b.osc1Level   = v (id::osc1Level);
        b.osc2Level   = v (id::osc2Level);
        b.osc2Semi    = v (id::osc2Semi);
        b.osc2Fine    = v (id::osc2Fine);
        b.subLevel    = v (id::subLevel);
        b.glideMs     = v (id::glide);
        b.drift       = v (id::drift);
        b.cutoffHz    = v (id::cutoff);
        b.resonance   = v (id::resonance);
        b.filterDrive = v (id::filterDrive);
        b.envAmount   = v (id::envAmount);
        b.keyTrack    = v (id::keyTrack);
        b.ampEnv      = { v (id::ampA), v (id::ampD), v (id::ampS), v (id::ampR) };
        b.filterEnv   = { v (id::fltA), v (id::fltD), v (id::fltS), v (id::fltR) };

        auto& d = e.distortion;
        d.mode       = (DistMode) (int) v (id::distMode);
        d.driveDb    = v (id::distDrive);
        d.mix        = v (id::distMix);
        d.bits       = v (id::distBits);
        d.downsample = v (id::distDown);
        d.outputDb   = v (id::distOut);

        e.masterDb = v (id::master);
        return e;
    }

private:
    float v (const char* pid) const { return state.getRawParameterValue (pid)->load(); }
    juce::AudioProcessorValueTreeState& state;
};

} // namespace krang::params
