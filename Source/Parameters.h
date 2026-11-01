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

    inline constexpr auto stMode      = "stMode";
    inline constexpr auto stMaterial  = "stMaterial";
    inline constexpr auto stHard      = "stHard";
    inline constexpr auto stPos       = "stPos";
    inline constexpr auto stStiff     = "stStiff";
    inline constexpr auto stDetune    = "stDetune";
    inline constexpr auto stDecay     = "stDecay";
    inline constexpr auto stDamper    = "stDamper";
    inline constexpr auto stBody      = "stBody";
    inline constexpr auto stRust      = "stRust";

    inline constexpr auto klType      = "klType";
    inline constexpr auto klHard      = "klHard";
    inline constexpr auto klDecay     = "klDecay";
    inline constexpr auto klRelease   = "klRelease";
    inline constexpr auto klBeat      = "klBeat";
    inline constexpr auto klCrack     = "klCrack";
    inline constexpr auto klHum       = "klHum";
    inline constexpr auto klSweep     = "klSweep";

    inline constexpr auto koVoices    = "koVoices";
    inline constexpr auto koVowel     = "koVowel";
    inline constexpr auto koSingers   = "koSingers";
    inline constexpr auto koSpread    = "koSpread";
    inline constexpr auto koVibrato   = "koVibrato";
    inline constexpr auto koBreath    = "koBreath";
    inline constexpr auto koSpeaker   = "koSpeaker";
    inline constexpr auto koAttack    = "koAttack";
    inline constexpr auto koRelease   = "koRelease";

    inline constexpr auto raMode      = "raMode";
    inline constexpr auto raSignal    = "raSignal";
    inline constexpr auto raNoise     = "raNoise";
    inline constexpr auto raBandwidth = "raBandwidth";
    inline constexpr auto raDrift     = "raDrift";
    inline constexpr auto raFading    = "raFading";
    inline constexpr auto raStorm     = "raStorm";
    inline constexpr auto raAttack    = "raAttack";
    inline constexpr auto raRelease   = "raRelease";

    inline constexpr auto luType      = "luType";
    inline constexpr auto luBreath    = "luBreath";
    inline constexpr auto luTension   = "luTension";
    inline constexpr auto luNoise     = "luNoise";
    inline constexpr auto luVibrato   = "luVibrato";
    inline constexpr auto luRasp      = "luRasp";
    inline constexpr auto luNatural   = "luNatural";
    inline constexpr auto luKey       = "luKey";
    inline constexpr auto luAttack    = "luAttack";
    inline constexpr auto luRelease   = "luRelease";

    inline constexpr auto ltType      = "ltType";
    inline constexpr auto ltPos       = "ltPos";
    inline constexpr auto ltHard      = "ltHard";
    inline constexpr auto ltDecay     = "ltDecay";
    inline constexpr auto ltBright    = "ltBright";
    inline constexpr auto ltBuzz      = "ltBuzz";
    inline constexpr auto ltCourse    = "ltCourse";
    inline constexpr auto ltBody      = "ltBody";
    inline constexpr auto ltDamper    = "ltDamper";

    inline constexpr auto kaAmount    = "kaAmount";
    inline constexpr auto kaSync      = "kaSync";
    inline constexpr auto kaRate      = "kaRate";
    inline constexpr auto kaDrift     = "kaDrift";
    inline constexpr auto kaStutter   = "kaStutter";
    inline constexpr auto kaReverse   = "kaReverse";
    inline constexpr auto kaCrush     = "kaCrush";

    inline constexpr auto tpWow       = "tpWow";
    inline constexpr auto tpFlutter   = "tpFlutter";
    inline constexpr auto tpAge       = "tpAge";
    inline constexpr auto tpDropout   = "tpDropout";

    inline constexpr auto ecSync      = "ecSync";
    inline constexpr auto ecTime      = "ecTime";
    inline constexpr auto ecFeedback  = "ecFeedback";
    inline constexpr auto ecDark      = "ecDark";
    inline constexpr auto ecMix       = "ecMix";

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
                                                         StringArray { String::fromUTF8 ("Basmålen"), "Hammarverket", "Bordunorgeln", "Spolslingan", String::fromUTF8 ("Stålpiano"), "Klockgjuteriet",
                                                                       String::fromUTF8 ("Kören i schaktet"), "Radiomasten",
                                                                       String::fromUTF8 ("Lurblåsaren"), "Lutverket" }, 0));

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

    // Stålpiano
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::stMode, 1 }, String::fromUTF8 ("Stålpianoläge"),
                                                         StringArray { "Piano", String::fromUTF8 ("Hackbräde"), "Preparerat" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::stMaterial, 1 }, "Strängmaterial",
                                                         StringArray { String::fromUTF8 ("Stål"), String::fromUTF8 ("Mässing"), "Koppar",
                                                                       String::fromUTF8 ("Järn"), "Silke", "Djurhud" }, 0));
    add (id::stHard,   "Hammarhårdhet", { 0.0f, 1.0f }, 0.5f);
    p.push_back (std::make_unique<P> (ParameterID { id::stPos, 1 }, "Anslagspunkt", NormalisableRange<float> (0.05f, 0.5f), 0.12f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));
    add (id::stStiff,  "Styvhet",       { 0.0f, 1.0f }, 0.3f);
    add (id::stDetune, "Svävning",      { 0.0f, 20.0f }, 3.0f, "ct");
    {
        NormalisableRange<float> d (0.5f, 30.0f); d.setSkewForCentre (6.0f);
        p.push_back (std::make_unique<P> (ParameterID { id::stDecay, 1 }, "Klingtid", d, 8.0f,
                                          AudioParameterFloatAttributes().withLabel ("s")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
        NormalisableRange<float> m (0.03f, 2.0f); m.setSkewForCentre (0.25f);
        p.push_back (std::make_unique<P> (ParameterID { id::stDamper, 1 }, String::fromUTF8 ("Dämpare"), m, 0.15f,
                                          AudioParameterFloatAttributes().withLabel ("s")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
    }
    add (id::stBody,   "Klangbotten",   { 0.0f, 1.0f }, 0.4f);
    add (id::stRust,   "Pianorost",     { 0.0f, 1.0f }, 0.0f);

    // Klockgjuteriet
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::klType, 1 }, "Klocktyp",
                                                         StringArray { "Kyrkklocka", String::fromUTF8 ("Skälla"), "Gong", String::fromUTF8 ("Räls"),
                                                                       String::fromUTF8 ("Skålklocka") }, 0));
    add (id::klHard, "Kläpp", { 0.0f, 1.0f }, 0.5f);
    {
        NormalisableRange<float> d (0.5f, 40.0f); d.setSkewForCentre (8.0f);
        p.push_back (std::make_unique<P> (ParameterID { id::klDecay, 1 }, "Klockans klingtid", d, 10.0f,
                                          AudioParameterFloatAttributes().withLabel ("s")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
        NormalisableRange<float> r (0.05f, 40.0f); r.setSkewForCentre (2.0f);
        // Ingen enhet i etiketten: i botten står det "Klingar ut" i stället för en tid.
        p.push_back (std::make_unique<P> (ParameterID { id::klRelease, 1 }, String::fromUTF8 ("Släpp"), r, 40.0f,
                                          AudioParameterFloatAttributes()
                                              .withStringFromValueFunction ([] (float v, int)
                                              {
                                                  return v >= 39.0f ? String::fromUTF8 ("Klingar ut") : String (v, v < 1.0f ? 2 : 1) + " s";
                                              })));
    }
    add (id::klBeat,  "Klocksvävning", { 0.0f, 30.0f }, 4.0f, "ct");
    add (id::klCrack, "Spricka", { 0.0f, 1.0f }, 0.0f);
    add (id::klHum,   "Brumton", { 0.0f, 1.0f }, 0.6f);
    p.push_back (std::make_unique<P> (ParameterID { id::klSweep, 1 }, "Svep", NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withLabel ("ct")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 300.0f)); })));

    // Kören i schaktet
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::koVoices, 1 }, String::fromUTF8 ("Röster"),
                                                         StringArray { String::fromUTF8 ("Mansröster"), String::fromUTF8 ("Kvinnoröster"),
                                                                       String::fromUTF8 ("Blandad kör"), "Kulning" }, 2));
    p.push_back (std::make_unique<P> (ParameterID { id::koVowel, 1 }, String::fromUTF8 ("Körvokal"), NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                      {
                                          static const char* vowels[] = { "A", "E", "I", "O", "U" };
                                          return String (vowels[jlimit (0, 4, roundToInt (v * 4.0f))]);
                                      })));
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { id::koSingers, 1 }, String::fromUTF8 ("Sångare"), 1, 8, 6));
    add (id::koSpread,  "Körspridning", { 0.0f, 30.0f }, 8.0f, "ct");
    add (id::koVibrato, "Körvibrato",   { 0.0f, 1.0f }, 0.3f);
    add (id::koBreath,  "Andning",      { 0.0f, 1.0f }, 0.2f);
    add (id::koSpeaker, "Trasig högtalare", { 0.0f, 1.0f }, 0.0f);
    add (id::koAttack,  "Körens insats", time (4.0f), 0.4f, "s");
    add (id::koRelease, "Körens avklang", time (6.0f), 1.2f, "s");

    // Radiomasten
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::raMode, 1 }, String::fromUTF8 ("Sändning"),
                                                         StringArray { String::fromUTF8 ("Kortvåg"), "Morse", "Vind i ledningar", "Interferens" }, 0));
    add (id::raSignal,    "Signal",       { 0.0f, 1.0f }, 0.6f);
    add (id::raNoise,     "Radiobrus",    { 0.0f, 1.0f }, 0.4f);
    add (id::raBandwidth, "Bandbredd",    { 0.0f, 1.0f }, 0.5f);
    add (id::raDrift,     "Radiodrift",   { 0.0f, 1.0f }, 0.3f);
    add (id::raFading,    "Fading",       { 0.0f, 1.0f }, 0.3f);
    add (id::raStorm,     "Storm",        { 0.0f, 1.0f }, 0.2f);
    add (id::raAttack,    "Radions insats",  time (4.0f), 0.05f, "s");
    add (id::raRelease,   "Radions avklang", time (6.0f), 0.6f, "s");

    // Lurblåsaren
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::luType, 1 }, String::fromUTF8 ("Blåsinstrument"),
                                                         StringArray { String::fromUTF8 ("Näverlur"), "Bockhorn", "Skalmeja",
                                                                       String::fromUTF8 ("Ångvissla") }, 0));
    add (id::luBreath,  "Blåstryck",  { 0.0f, 1.0f }, 0.7f);
    add (id::luTension, "Läppspänning", { 0.0f, 1.0f }, 0.5f);
    add (id::luNoise,   "Lurandning", { 0.0f, 1.0f }, 0.2f);
    add (id::luVibrato, "Lurvibrato", { 0.0f, 1.0f }, 0.2f);
    add (id::luRasp,    "Rasp",       { 0.0f, 1.0f }, 0.1f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::luNatural, 1 }, "Naturtoner",
                                                         StringArray { "Av", String::fromUTF8 ("På") }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::luKey, 1 }, "Lurens ton",
                                                         StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" }, 2));
    add (id::luAttack,  "Lurens insats",  time (2.0f), 0.08f, "s");
    add (id::luRelease, "Lurens avklang", time (3.0f), 0.25f, "s");

    // Lutverket
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::ltType, 1 }, "Stränginstrument",
                                                         StringArray { "Kantele", "Luta", "Cittra", "Rostharpa" }, 0));
    p.push_back (std::make_unique<P> (ParameterID { id::ltPos, 1 }, String::fromUTF8 ("Knäppunkt"), NormalisableRange<float> (0.05f, 0.5f), 0.2f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));
    add (id::ltHard,   "Knäpphårdhet", { 0.0f, 1.0f }, 0.5f);
    {
        NormalisableRange<float> d (0.3f, 20.0f); d.setSkewForCentre (4.0f);
        p.push_back (std::make_unique<P> (ParameterID { id::ltDecay, 1 }, String::fromUTF8 ("Strängarnas klingtid"), d, 4.0f,
                                          AudioParameterFloatAttributes().withLabel ("s")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
        NormalisableRange<float> m (0.03f, 3.0f); m.setSkewForCentre (0.3f);
        p.push_back (std::make_unique<P> (ParameterID { id::ltDamper, 1 }, String::fromUTF8 ("Strängdämpare"), m, 0.25f,
                                          AudioParameterFloatAttributes().withLabel ("s")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
    }
    add (id::ltBright, "Glans",       { 0.0f, 1.0f }, 0.6f);
    add (id::ltBuzz,   "Surr",        { 0.0f, 1.0f }, 0.0f);
    add (id::ltCourse, "Kör",         { 0.0f, 20.0f }, 4.0f, "ct");
    add (id::ltBody,   "Klanglåda",   { 0.0f, 1.0f }, 0.4f);

    // Kaosmotorn
    p.push_back (std::make_unique<P> (ParameterID { id::kaAmount, 1 }, "Kaos", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::kaSync, 1 }, "Kaostakt",
                                                         StringArray { "Fri", "1/4", "1/8", "1/16", "1/32" }, 3));
    {
        NormalisableRange<float> r (0.25f, 32.0f); r.setSkewForCentre (4.0f);
        p.push_back (std::make_unique<P> (ParameterID { id::kaRate, 1 }, "Kaosfrekvens", r, 4.0f,
                                          AudioParameterFloatAttributes().withLabel ("Hz")
                                              .withStringFromValueFunction ([] (float v, int) { return String (v, v < 10.0f ? 1 : 0); })));
    }
    add (id::kaDrift,   "Slumpdrift", { 0.0f, 1.0f }, 0.3f);
    add (id::kaStutter, "Hack",       { 0.0f, 1.0f }, 0.6f);
    add (id::kaReverse, "Baklänges",  { 0.0f, 1.0f }, 0.3f);
    add (id::kaCrush,   "Söndring",   { 0.0f, 1.0f }, 0.3f);

    // Bandmaskinen
    add (id::tpWow,     "Wow",     { 0.0f, 1.0f }, 0.0f);
    add (id::tpFlutter, "Flutter", { 0.0f, 1.0f }, 0.0f);
    add (id::tpAge,     "Bandålder", { 0.0f, 1.0f }, 0.0f);
    add (id::tpDropout, "Avbrott", { 0.0f, 1.0f }, 0.0f);

    // Ekokammaren
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::ecSync, 1 }, "Ekotakt",
                                                         StringArray { "Fri", "1/4", "1/8", "1/8 punkt.", "1/16", "1/4 triol" }, 2));
    {
        NormalisableRange<float> t (20.0f, 1500.0f); t.setSkewForCentre (300.0f);
        add (id::ecTime, "Ekotid", t, 350.0f, "ms");
    }
    p.push_back (std::make_unique<P> (ParameterID { id::ecFeedback, 1 }, String::fromUTF8 ("Återkoppling"), NormalisableRange<float> (0.0f, 1.1f), 0.4f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));
    add (id::ecDark, "Ekomörker", { 0.0f, 1.0f }, 0.4f);
    p.push_back (std::make_unique<P> (ParameterID { id::ecMix, 1 }, "Ekomix", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                      AudioParameterFloatAttributes().withLabel ("%")
                                          .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); })));

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

        auto& st = e.stalpiano;
        st.mode        = (StalMode) (int) v (id::stMode);
        st.material    = (StalMaterial) (int) v (id::stMaterial);
        st.hardness    = v (id::stHard);
        st.strikePos   = v (id::stPos);
        st.stiffness   = v (id::stStiff);
        st.detuneCents = v (id::stDetune);
        st.decaySec    = v (id::stDecay);
        st.damperSec   = v (id::stDamper);
        st.body        = v (id::stBody);
        st.rust        = v (id::stRust);

        auto& kl = e.klockor;
        kl.type       = (BellType) (int) v (id::klType);
        kl.hardness   = v (id::klHard);
        kl.decaySec   = v (id::klDecay);
        kl.releaseSec = v (id::klRelease);
        kl.beatCents  = v (id::klBeat);
        kl.crack      = v (id::klCrack);
        kl.hum        = v (id::klHum);
        kl.sweep      = v (id::klSweep);

        auto& ko = e.koren;
        ko.voices      = (ChoirVoices) (int) v (id::koVoices);
        ko.vowel       = v (id::koVowel);
        ko.singers     = (int) v (id::koSingers);
        ko.spreadCents = v (id::koSpread);
        ko.vibrato     = v (id::koVibrato);
        ko.breath      = v (id::koBreath);
        ko.speaker     = v (id::koSpeaker);
        ko.attackSec   = v (id::koAttack);
        ko.releaseSec  = v (id::koRelease);

        auto& ra = e.radio;
        ra.mode       = (RadioMode) (int) v (id::raMode);
        ra.signal     = v (id::raSignal);
        ra.noise      = v (id::raNoise);
        ra.bandwidth  = v (id::raBandwidth);
        ra.drift      = v (id::raDrift);
        ra.fading     = v (id::raFading);
        ra.storm      = v (id::raStorm);
        ra.attackSec  = v (id::raAttack);
        ra.releaseSec = v (id::raRelease);

        auto& lu = e.lur;
        lu.type       = (WindType) (int) v (id::luType);
        lu.breath     = v (id::luBreath);
        lu.tension    = v (id::luTension);
        lu.noise      = v (id::luNoise);
        lu.vibrato    = v (id::luVibrato);
        lu.rasp       = v (id::luRasp);
        lu.natural    = v (id::luNatural) > 0.5f;
        lu.key        = (int) v (id::luKey);
        lu.attackSec  = v (id::luAttack);
        lu.releaseSec = v (id::luRelease);

        auto& lt = e.lut;
        lt.type        = (LuteType) (int) v (id::ltType);
        lt.pluckPos    = v (id::ltPos);
        lt.hardness    = v (id::ltHard);
        lt.decaySec    = v (id::ltDecay);
        lt.brightness  = v (id::ltBright);
        lt.buzz        = v (id::ltBuzz);
        lt.courseCents = v (id::ltCourse);
        lt.body        = v (id::ltBody);
        lt.damperSec   = v (id::ltDamper);

        e.kaos.amount  = v (id::kaAmount);
        e.kaos.sync    = (int) v (id::kaSync);
        e.kaos.rateHz  = v (id::kaRate);
        e.kaos.drift   = v (id::kaDrift);
        e.kaos.stutter = v (id::kaStutter);
        e.kaos.reverse = v (id::kaReverse);
        e.kaos.crush   = v (id::kaCrush);

        e.tape.wow     = v (id::tpWow);
        e.tape.flutter = v (id::tpFlutter);
        e.tape.age     = v (id::tpAge);
        e.tape.dropout = v (id::tpDropout);

        e.echo.sync     = (int) v (id::ecSync);
        e.echo.timeMs   = v (id::ecTime);
        e.echo.feedback = v (id::ecFeedback);
        e.echo.dark     = v (id::ecDark);
        e.echo.mix      = v (id::ecMix);

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
        e.bpm = e.stegverket.tempo;           // processorn byter till DAW:ens tempo när den spelar
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
