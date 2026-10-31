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
    inline constexpr auto krang       = "krang";

    inline constexpr auto chamber     = "chamber";
    inline constexpr auto hvRost      = "hvRost";

    inline constexpr auto boTimbre    = "boTimbre";
    inline constexpr auto boBow       = "boBow";
    inline constexpr auto boDrone     = "boDrone";
    inline constexpr auto boDroneNote = "boDroneNote";
    inline constexpr auto boSymp      = "boSymp";
    inline constexpr auto boDrift     = "boDrift";
    inline constexpr auto boAttack    = "boAttack";
    inline constexpr auto boRelease   = "boRelease";

    inline constexpr auto spRatio     = "spRatio";
    inline constexpr auto spIndex     = "spIndex";
    inline constexpr auto spDecay     = "spDecay";
    inline constexpr auto spHarp      = "spHarp";
    inline constexpr auto spMouth     = "spMouth";
    inline constexpr auto spAttack    = "spAttack";
    inline constexpr auto spRelease   = "spRelease";
    inline constexpr auto spGrain     = "spGrain";
    inline constexpr auto spGrainMs   = "spGrainMs";
    inline constexpr auto spSpread    = "spSpread";
    inline constexpr auto spArp       = "spArp";
    inline constexpr auto spArpRate   = "spArpRate";
    inline constexpr auto spArpOct    = "spArpOct";

    inline constexpr auto fxMode      = "fxMode";
    inline constexpr auto fxCutoff    = "fxCutoff";
    inline constexpr auto fxReso      = "fxReso";
    inline constexpr auto fxVowel     = "fxVowel";

    inline constexpr auto rvbMode     = "rvbMode";
    inline constexpr auto rvbMix      = "rvbMix";
    inline constexpr auto rvbSize     = "rvbSize";
    inline constexpr auto rvbDark     = "rvbDark";

    inline constexpr auto seqRun      = "seqRun";
    inline constexpr auto seqTempo    = "seqTempo";
    inline constexpr auto seqSwing    = "seqSwing";
    inline constexpr auto seqLength   = "seqLength";

    // Hammarverket: hv<röst><Tune|Decay|Level>, t.ex. "hv0Tune". Röstnummer enligt krang::Drum.
    inline juce::String drum (int index, const char* what) { return "hv" + juce::String (index) + what; }
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto add = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, const char* label = "")
    {
        // Antal decimaler efter enhet, så att DAW och gränssnitt visar "400 Hz" och inte "400.0000305".
        const String unit = String::fromUTF8 (label);
        const int decimals = unit == "s" ? 3
                           : (unit == "dB" || unit == "okt") ? 1
                           : (unit.isEmpty() && r.interval < 1.0f) ? 2
                           : 0;

        // Namnen innehåller å, ä, ö: tolka dem uttryckligen som UTF-8.
        p.push_back (std::make_unique<P> (ParameterID { pid, 1 }, String::fromUTF8 (name), r, def,
                                          AudioParameterFloatAttributes()
                                              .withLabel (unit)
                                              .withStringFromValueFunction ([decimals] (float v, int)
                                              {
                                                  // OBS: String (v, 0) skriver ut alla decimaler, därför heltal separat.
                                                  return decimals == 0 ? String (roundToInt (v)) : String (v, decimals);
                                              })));
    };
    auto time = [] (float maxSec) { NormalisableRange<float> r (0.001f, maxSec); r.setSkewForCentre (maxSec * 0.1f); return r; };

    // Kammare
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::chamber, 1 }, "Kammare",
                                                         StringArray { String::fromUTF8 ("Basmålen"), "Hammarverket", "Bordunorgeln", "Spolslingan" }, 0));

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
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::distMode, 1 }, String::fromUTF8 ("Distortion-läge"),
                                                         StringArray { String::fromUTF8 ("Rör"), "Fuzz", "Vikning", "Bitkross" }, 0));
    add (id::distDrive, "Drive",      { 0.0f, 48.0f }, 18.0f, "dB");
    add (id::distMix,   "Dist Mix",   { 0.0f, 1.0f }, 1.0f);
    add (id::distBits,  "Bitar",      { 1.0f, 16.0f, 1.0f }, 6.0f);
    add (id::distDown,  "Nedsampling",{ 1.0f, 32.0f, 1.0f }, 4.0f, "x");
    add (id::distOut,   "Dist Ut",    { -24.0f, 12.0f }, -6.0f, "dB");

    // Ångfiltret (efter distortion)
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::fxMode, 1 }, String::fromUTF8 ("Ångfilter"),
                                                         StringArray { "Av", String::fromUTF8 ("Lågpass"), String::fromUTF8 ("Högpass"), "Formant", "Kam" }, 0));
    add (id::fxCutoff, "Ångfilter frekvens", { 20.0f, 18000.0f, 0.0f, 0.25f }, 2000.0f, "Hz");
    add (id::fxReso,   "Ångfilter resonans", { 0.0f, 1.0f }, 0.3f);
    p.push_back (std::make_unique<P> (ParameterID { id::fxVowel, 1 }, "Vokal", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                      {
                                          static const char* vowels[] = { "A", "E", "I", "O", "U" };
                                          return String (vowels[jlimit (0, 4, roundToInt (v * 4.0f))]);
                                      })));

    // Katedralreverb
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::rvbMode, 1 }, "Katedralreverb",
                                                         StringArray { "Hall", "Platta", "Rostig tank" }, 0));
    auto percent = [] (float v, int) { return String (roundToInt (v * 100.0f)); };
    p.push_back (std::make_unique<P> (ParameterID { id::rvbMix, 1 }, "Reverb mix", NormalisableRange<float> (0.0f, 1.0f), 0.15f,
                                      AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percent)));
    p.push_back (std::make_unique<P> (ParameterID { id::rvbSize, 1 }, "Reverb storlek", NormalisableRange<float> (0.0f, 1.0f), 0.6f,
                                      AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percent)));
    p.push_back (std::make_unique<P> (ParameterID { id::rvbDark, 1 }, String::fromUTF8 ("Reverb mörker"), NormalisableRange<float> (0.0f, 1.0f), 0.4f,
                                      AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percent)));

    add (id::master, "Master", { -48.0f, 6.0f }, 0.0f, "dB");
    p.push_back (std::make_unique<P> (ParameterID { id::krang, 1 }, "Krangspaken", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));

    // Stegverket (sekvenser för Hammarverket)
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id::seqRun, 1 }, "Stegverket spela", false));
    add (id::seqTempo, "Tempo", { 40.0f, 240.0f, 1.0f }, 96.0f, "BPM");
    p.push_back (std::make_unique<P> (ParameterID { id::seqSwing, 1 }, "Gung", NormalisableRange<float> (0.0f, 0.5f), 0.0f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v * 200.0f)); })
                                          .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue() / 200.0f; })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::seqLength, 1 }, String::fromUTF8 ("Steglängd"),
                                                         StringArray { "16", "32", "48", "64" }, 0));

    // Bordunorgeln
    p.push_back (std::make_unique<P> (ParameterID { id::boTimbre, 1 }, String::fromUTF8 ("Klangfärg"), NormalisableRange<float> (0.0f, 1.0f), 0.5f,
                                      AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                      {
                                          if (v < 0.2f) return String::fromUTF8 ("Vallflöjt");
                                          if (v < 0.4f) return String::fromUTF8 ("Flöjt/pipa");
                                          if (v < 0.6f) return String::fromUTF8 ("Säckpipa");
                                          if (v < 0.8f) return String::fromUTF8 ("Pipa/harpa");
                                          return String ("Nyckelharpa");
                                      })));
    add (id::boBow,  "Stråke",  { 0.0f, 1.0f }, 0.3f);
    add (id::boDrone, "Bordun", { 0.0f, 1.0f }, 0.5f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::boDroneNote, 1 }, "Bordunton",
                                                         StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" }, 2));
    add (id::boSymp,  "Resonanssträngar", { 0.0f, 1.0f }, 0.4f);
    add (id::boDrift, "Orgeldrift", { 0.0f, 1.0f }, 0.3f);
    {
        NormalisableRange<float> a (0.002f, 5.0f); a.setSkewForCentre (0.4f);
        NormalisableRange<float> r (0.01f, 8.0f);  r.setSkewForCentre (1.0f);
        add (id::boAttack,  "Orgel attack",  a, 0.25f, "s");
        add (id::boRelease, "Orgel release", r, 1.2f, "s");
    }

    // Spolslingan
    {
        NormalisableRange<float> ratio (0.5f, 8.0f); ratio.setSkewForCentre (2.0f);
        p.push_back (std::make_unique<P> (ParameterID { id::spRatio, 1 }, "FM-ratio", ratio, 2.0f,
                                          AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
        add (id::spIndex, "FM-index", { 0.0f, 10.0f }, 3.0f, "");
        NormalisableRange<float> dec (0.01f, 2.0f); dec.setSkewForCentre (0.3f);
        add (id::spDecay, "Index-avklingning", dec, 0.35f, "s");
        add (id::spHarp,  "Munharpa", { 0.0f, 1.0f }, 0.0f);
        add (id::spMouth, "Munhåla", { 0.0f, 1.0f }, 0.4f);
        NormalisableRange<float> a (0.001f, 3.0f); a.setSkewForCentre (0.1f);
        NormalisableRange<float> r (0.005f, 5.0f); r.setSkewForCentre (0.5f);
        add (id::spAttack,  "Slinga attack",  a, 0.005f, "s");
        add (id::spRelease, "Slinga release", r, 0.3f, "s");
        add (id::spGrain,   "Radiokorn", { 0.0f, 1.0f }, 0.0f);
        add (id::spGrainMs, "Kornstorlek", { 10.0f, 300.0f, 0.0f, 0.5f }, 80.0f, "ms");
        add (id::spSpread,  "Spridning", { 0.0f, 1.0f }, 0.4f);
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::spArp, 1 }, "Slinga",
                                                             StringArray { "Av", "Upp", "Ned", "Upp/ned", "Slump" }, 0));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::spArpRate, 1 }, "Slingtakt",
                                                             StringArray { "1/8", "1/16", "1/16T", "1/32" }, 1));
        add (id::spArpOct, "Oktaver", { 1.0f, 3.0f, 1.0f }, 1.0f);
    }

    // Hammarverket
    add (id::hvRost, "Rost", { 0.0f, 1.0f }, 0.3f);
    for (int d = 0; d < krang::numDrums; ++d)
    {
        const auto name = String::fromUTF8 (krang::drumName (d));
        const auto tuneId = id::drum (d, "Tune"), decayId = id::drum (d, "Decay"), levelId = id::drum (d, "Level");
        auto dec = [] (int n) { return [n] (float v, int) { return String (v, n); }; };
        p.push_back (std::make_unique<P> (ParameterID { tuneId, 1 }, name + String::fromUTF8 (" Stämning"),
                                          NormalisableRange<float> (-12.0f, 12.0f), 0.0f,
                                          AudioParameterFloatAttributes().withLabel ("st").withStringFromValueFunction (dec (1))));
        NormalisableRange<float> decayRange (0.25f, 4.0f);
        decayRange.setSkewForCentre (1.0f);
        p.push_back (std::make_unique<P> (ParameterID { decayId, 1 }, name + " Klang", decayRange, 1.0f,
                                          AudioParameterFloatAttributes().withLabel ("x").withStringFromValueFunction (dec (2))));
        p.push_back (std::make_unique<P> (ParameterID { levelId, 1 }, name + String::fromUTF8 (" Nivå"),
                                          NormalisableRange<float> (0.0f, 1.0f), 0.8f,
                                          AudioParameterFloatAttributes().withStringFromValueFunction (dec (2))));
    }

    return { p.begin(), p.end() };
}

// Läser parameterträdet till motorns struktur. Körs i början av varje ljudblock.
class Reader
{
public:
    explicit Reader (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        // Slå upp trumparametrarna en gång: ljudtråden får inte skapa strängar.
        for (int d = 0; d < numDrums; ++d)
        {
            drumTune[(size_t) d]  = s.getRawParameterValue (id::drum (d, "Tune"));
            drumDecay[(size_t) d] = s.getRawParameterValue (id::drum (d, "Decay"));
            drumLevel[(size_t) d] = s.getRawParameterValue (id::drum (d, "Level"));
        }
    }

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
        const float krangspaken = v (id::krang);

        e.filter.mode      = (FxFilterMode) (int) v (id::fxMode);
        e.filter.cutoffHz  = v (id::fxCutoff);
        e.filter.resonance = v (id::fxReso);
        e.filter.vowel     = v (id::fxVowel);

        e.reverb.mode = (ReverbMode) (int) v (id::rvbMode);
        e.reverb.mix  = v (id::rvbMix);
        e.reverb.size = v (id::rvbSize);
        e.reverb.dark = v (id::rvbDark);

        e.chamber = (Chamber) (int) v (id::chamber);

        auto& sp = e.spolslingan;
        sp.ratio      = v (id::spRatio);
        sp.index      = v (id::spIndex);
        sp.indexDecay = v (id::spDecay);
        sp.harp       = v (id::spHarp);
        sp.mouth      = v (id::spMouth);
        sp.attackSec  = v (id::spAttack);
        sp.releaseSec = v (id::spRelease);
        sp.grain      = v (id::spGrain);
        sp.grainMs    = v (id::spGrainMs);
        sp.spread     = v (id::spSpread);
        e.slingan.mode    = (ArpMode) (int) v (id::spArp);
        e.slingan.rate    = (int) v (id::spArpRate);
        e.slingan.octaves = (int) v (id::spArpOct);

        auto& bo = e.bordunorgeln;
        bo.timbre      = v (id::boTimbre);
        bo.bow         = v (id::boBow);
        bo.drone       = v (id::boDrone);
        bo.droneNote   = (int) v (id::boDroneNote);
        bo.sympathetic = v (id::boSymp);
        bo.drift       = v (id::boDrift);
        bo.attackSec   = v (id::boAttack);
        bo.releaseSec  = v (id::boRelease);
        e.stegverket.run    = v (id::seqRun) > 0.5f;
        e.stegverket.tempo  = v (id::seqTempo);
        e.stegverket.swing  = v (id::seqSwing);
        e.stegverket.length = 16 * (1 + (int) v (id::seqLength));
        e.slingan.tempo = e.stegverket.tempo; // samma tempo som Stegverket när DAW:en inte spelar
        e.hammarverket.rost = v (id::hvRost);
        for (size_t k = 0; k < (size_t) numDrums; ++k)
        {
            auto& dp = e.hammarverket.drums[k];
            dp.tune  = drumTune[k]->load();
            dp.decay = drumDecay[k]->load();
            dp.level = drumLevel[k]->load();
        }
        applyKrangspaken (e, krangspaken);
        return e;
    }

private:
    float v (juce::StringRef pid) const { return state.getRawParameterValue (pid)->load(); }
    juce::AudioProcessorValueTreeState& state;
    std::array<std::atomic<float>*, (size_t) numDrums> drumTune {}, drumDecay {}, drumLevel {};
};

} // namespace krang::params
