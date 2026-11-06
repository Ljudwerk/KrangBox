// Stegverket – stegsekvenser för Hammarverket (fas 1, utbyggt i fas 3c).
// 8 spår × upp till 64 steg om en 16-del vardera. Varje steg kan ha chans (100/75/50/25 %), ratchet
// (1–4 slag inom steget) och parameterlås för stämning och klang, som skickas som CC före slaget.
// Följer DAW:ens transport när den spelar; i den fristående appen går en egen klocka.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Hammarverket.h"
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace krang
{

// Ett steg i trummönstret: slag, chans, ratchet och två parameterlås.
struct DrumStep
{
    int gate = 0;       // 0 av, 1 slag, 2 accent
    int chance = 0;     // 0 = 100 %, 1 = 75 %, 2 = 50 %, 3 = 25 %
    int ratchet = 0;    // 0..3: 1–4 slag inom steget
    int tune = 0;       // stämningslås i halvtoner, −12..+12
    bool tuneLocked = false;
    int decay = 0;      // klanglås: 0 = inget, 1..7 = ×0,25 … ×4 (lockDecayValues)

    bool hasExtras() const { return chance != 0 || ratchet != 0 || tuneLocked || decay != 0; }
};

inline constexpr std::array<float, 4> drumChanceValues { 1.0f, 0.75f, 0.5f, 0.25f };

// Mönstret delas mellan gränssnittet (skriver) och ljudtråden (läser), därför atomiskt.
class StepPattern
{
public:
    static constexpr int lanes = numDrums;
    static constexpr int maxSteps = 64;

    StepPattern() { clear(); }

    // Bara slaget (0 av, 1 slag, 2 accent); set behåller stegets chans, ratchet och lås.
    int get (int lane, int step) const { return getStep (lane, step).gate; }
    void set (int lane, int step, int value)
    {
        auto s = getStep (lane, step);
        s.gate = juce::jlimit (0, 2, value);
        setStep (lane, step, s);
    }

    DrumStep getStep (int lane, int step) const { return unpack (cells[index (lane, step)].load (std::memory_order_relaxed)); }
    void setStep (int lane, int step, const DrumStep& s) { cells[index (lane, step)].store (pack (s), std::memory_order_relaxed); }
    void clear() { for (auto& c : cells) c.store (0, std::memory_order_relaxed); }

    // Text för att spara i pluginets läge: en rad per spår, rader skilda med '/'. Varje steg är en siffra
    // (0 av, 1 slag, 2 accent), följd av {…} när steget har mer: c = chans, r = ratchet, t = stämning, d = klang.
    // Exempel: "1{c2r1}0002{t-5d6}". Äldre mönster med bara siffror läses som förut.
    juce::String toString() const
    {
        juce::String s;
        for (int l = 0; l < lanes; ++l)
        {
            if (l > 0) s << "/";
            for (int st = 0; st < maxSteps; ++st)
            {
                const auto d = getStep (l, st);
                s << juce::String (d.gate);
                if (d.hasExtras())
                {
                    s << "{";
                    if (d.chance != 0)  s << "c" << d.chance;
                    if (d.ratchet != 0) s << "r" << d.ratchet;
                    if (d.tuneLocked)   s << "t" << (d.tune >= 0 ? "+" : "") << d.tune;
                    if (d.decay != 0)   s << "d" << d.decay;
                    s << "}";
                }
            }
        }
        return s;
    }

    void fromString (const juce::String& text)
    {
        clear();
        const auto rows = juce::StringArray::fromTokens (text, "/", "");
        for (int l = 0; l < juce::jmin (lanes, rows.size()); ++l)
        {
            const auto steps = parseRow (rows[l]);
            for (int st = 0; st < juce::jmin (maxSteps, (int) steps.size()); ++st)
                setStep (l, st, steps[(size_t) st]);
        }
    }

    // Ett spårs text till steg (se toString).
    static std::vector<DrumStep> parseRow (const juce::String& row)
    {
        std::vector<DrumStep> steps;
        for (int i = 0; i < row.length() && (int) steps.size() < maxSteps;)
        {
            DrumStep d;
            d.gate = juce::jlimit (0, 2, (int) (row[i++] - '0'));
            if (i < row.length() && row[i] == '{')
            {
                const int close = row.indexOfChar (i, '}');
                const auto inner = row.substring (i + 1, close < 0 ? row.length() : close);
                i = close < 0 ? row.length() : close + 1;
                for (int j = 0; j < inner.length();)
                {
                    const auto key = inner[j++];
                    int k = j;
                    while (k < inner.length() && (juce::CharacterFunctions::isDigit (inner[k]) || inner[k] == '-' || inner[k] == '+')) ++k;
                    const int value = inner.substring (j, k).getIntValue();
                    j = k;
                    if (key == 'c')      d.chance = juce::jlimit (0, 3, value);
                    else if (key == 'r') d.ratchet = juce::jlimit (0, 3, value);
                    else if (key == 't') { d.tune = juce::jlimit (-12, 12, value); d.tuneLocked = true; }
                    else if (key == 'd') d.decay = juce::jlimit (0, 7, value);
                }
            }
            steps.push_back (d);
        }
        return steps;
    }

    // Startmönster: en takt i 6/8-känsla, ångslag och smedja, ramtrumma och träskedar.
    void loadDefault()
    {
        clear();
        auto hits = [this] (Drum d, std::initializer_list<int> steps, int accentStep = -1)
        {
            for (int st : steps)
                for (int bar = 0; bar < maxSteps / 16; ++bar)
                    set ((int) d, bar * 16 + st, st == accentStep ? 2 : 1);
        };
        hits (Drum::Angslaget,   { 0, 6, 10 }, 0);
        hits (Drum::Smedjan,     { 4, 12 }, 12);
        hits (Drum::Klappern,    { 12 });
        hits (Drum::Ventilen,    { 0, 2, 4, 8, 10 });
        hits (Drum::Angroret,    { 14 });
        hits (Drum::Ramtrumman,  { 3, 7, 15 });
        hits (Drum::Traskedarna, { 5, 9, 11 });
        hits (Drum::Skrotplaten, { 13 });
    }

private:
    static uint16_t pack (const DrumStep& s)
    {
        return (uint16_t) ((uint32_t) juce::jlimit (0, 2, s.gate)
                         | ((uint32_t) juce::jlimit (0, 3, s.chance) << 2)
                         | ((uint32_t) juce::jlimit (0, 3, s.ratchet) << 4)
                         | ((uint32_t) (s.tuneLocked ? juce::jlimit (-12, 12, s.tune) + 13 : 0) << 6)
                         | ((uint32_t) juce::jlimit (0, 7, s.decay) << 11));
    }
    static DrumStep unpack (uint16_t v)
    {
        DrumStep s;
        s.gate = (int) (v & 3u);
        s.chance = (int) ((v >> 2) & 3u);
        s.ratchet = (int) ((v >> 4) & 3u);
        const int t = (int) ((v >> 6) & 31u);
        s.tuneLocked = t != 0;
        s.tune = t != 0 ? t - 13 : 0;
        s.decay = (int) ((v >> 11) & 7u);
        return s;
    }
    static size_t index (int lane, int step)
    {
        return (size_t) (juce::jlimit (0, lanes - 1, lane) * maxSteps + juce::jlimit (0, maxSteps - 1, step));
    }
    std::array<std::atomic<uint16_t>, (size_t) (lanes * maxSteps)> cells;
};

// MIDI-noten som varje spår spelar, i samma ordning som Drum.
inline int noteForLane (int lane)
{
    static constexpr int notes[numDrums] = { 36, 37, 38, 39, 41, 42, 46, 75 };
    return notes[juce::jlimit (0, numDrums - 1, lane)];
}

struct StegverketSettings
{
    bool run       = false;  // spela/stopp
    int length     = 16;     // 16, 32, 48 eller 64 steg
    float swing    = 0.0f;   // 0..0.5: hur mycket varannan 16-del släpar
    double tempo   = 96.0;   // BPM för den interna klockan
};

// Var transporten står vid blockets början.
struct Transport
{
    bool hostPlaying = false;
    double ppq = 0.0;        // position i fjärdedelar
    double bpm = 120.0;
};

class Stegverket
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        internalPpq = 0.0;
        wasRunning = false;
        currentStep.store (-1);
    }

    // Lägger till noter för de steg som infaller i blocket. useInternalClock = fristående app.
    void process (const StepPattern& pattern, const StegverketSettings& s, const Transport& t,
                  bool useInternalClock, int numSamples, juce::MidiBuffer& out)
    {
        const bool host = t.hostPlaying;
        const bool internal = ! host && useInternalClock && s.run;

        if (! s.run || (! host && ! internal))
        {
            wasRunning = false;
            currentStep.store (-1);
            return;
        }

        if (internal && ! wasRunning)
            internalPpq = -1.0e-9; // starta exakt på steg 0
        wasRunning = true;

        const double bpm = juce::jlimit (20.0, 400.0, host ? t.bpm : s.tempo);
        const double samplesPerPpq = sr * 60.0 / bpm;
        const double start = host ? t.ppq : internalPpq;
        const double end = start + numSamples / samplesPerPpq;
        const int length = juce::jlimit (1, StepPattern::maxSteps, s.length);
        const double swingOffset = juce::jlimit (0.0f, 0.5f, s.swing) * stepLen;

        const auto firstK = (juce::int64) std::floor (start / stepLen) - 1;
        const auto lastK  = (juce::int64) std::floor (end / stepLen);

        for (auto k = juce::jmax ((juce::int64) 0, firstK); k <= lastK; ++k)
        {
            const double at = (double) k * stepLen + ((k & 1) ? swingOffset : 0.0);
            const double nextAt = (double) (k + 1) * stepLen + (((k + 1) & 1) ? swingOffset : 0.0);
            if (at >= end || nextAt <= start)
                continue;

            const int step = (int) (k % length);
            if (at >= start)
                currentStep.store (step);

            for (int lane = 0; lane < StepPattern::lanes; ++lane)
            {
                const auto st = pattern.getStep (lane, step);
                if (st.gate == 0) continue;
                // Chansen slås en gång per steg och varv, och samma steg ger samma utfall i hela steget.
                if (st.chance > 0 && chanceRoll (k, lane) >= drumChanceValues[(size_t) st.chance]) continue;

                const int hits = st.ratchet + 1;
                const double sub = (nextAt - at) / hits;
                for (int h = 0; h < hits; ++h)
                {
                    const double hitAt = at + h * sub;
                    if (hitAt < start || hitAt >= end) continue;
                    const int offset = juce::jlimit (0, numSamples - 1, (int) ((hitAt - start) * samplesPerPpq));
                    if (st.tuneLocked) out.addEvent (juce::MidiMessage::controllerEvent (10, lockTuneCC, 64 + st.tune), offset);
                    if (st.decay > 0)  out.addEvent (juce::MidiMessage::controllerEvent (10, lockDecayCC, st.decay), offset);
                    const float vel = (st.gate == 2 ? 1.0f : 0.7f) * (h == 0 ? 1.0f : 0.85f);
                    out.addEvent (juce::MidiMessage::noteOn (10, noteForLane (lane), vel), offset);
                }
            }
        }

        if (internal)
            internalPpq = end;
    }

    // Steget som spelades senast, för lampan i gränssnittet. -1 = stoppad.
    std::atomic<int> currentStep { -1 };

private:
    static constexpr double stepLen = 0.25; // en 16-del i fjärdedelar

    // Ett fast slumptal 0..1 för steg k i spåret: samma i hela steget (även över blockgränser),
    // olika från varv till varv, och likadant varje gång samma låt spelas.
    static float chanceRoll (juce::int64 k, int lane)
    {
        auto h = (uint32_t) ((uint64_t) k * 2654435761u) ^ (uint32_t) (lane * 40503 + 0x9e3779b9);
        h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
        return (float) (h & 0xffffu) / 65536.0f;
    }

    double sr = 44100.0;
    double internalPpq = 0.0;
    bool wasRunning = false;
};

} // namespace krang
