// Stegverket – stegsekvenser för Hammarverket (fas 1).
// 8 spår (ett per trumröst) × upp till 64 steg om en 16-del vardera.
// Följer DAW:ens transport när den spelar; i den fristående appen går en egen klocka.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Hammarverket.h"
#include <array>
#include <atomic>
#include <cmath>

namespace krang
{

// Mönstret delas mellan gränssnittet (skriver) och ljudtråden (läser), därför atomiskt.
// Cellvärde: 0 = av, 1 = slag, 2 = accent.
class StepPattern
{
public:
    static constexpr int lanes = numDrums;
    static constexpr int maxSteps = 64;

    StepPattern() { clear(); }

    int get (int lane, int step) const { return cells[index (lane, step)].load (std::memory_order_relaxed); }
    void set (int lane, int step, int value) { cells[index (lane, step)].store ((uint8_t) juce::jlimit (0, 2, value), std::memory_order_relaxed); }
    void clear() { for (auto& c : cells) c.store (0, std::memory_order_relaxed); }

    // Text för att spara i pluginets läge: en rad siffror per spår, rader skilda med '/'.
    juce::String toString() const
    {
        juce::String s;
        for (int l = 0; l < lanes; ++l)
        {
            if (l > 0) s << "/";
            for (int st = 0; st < maxSteps; ++st)
                s << juce::String (get (l, st));
        }
        return s;
    }

    void fromString (const juce::String& s)
    {
        clear();
        const auto rows = juce::StringArray::fromTokens (s, "/", "");
        for (int l = 0; l < juce::jmin (lanes, rows.size()); ++l)
            for (int st = 0; st < juce::jmin (maxSteps, rows[l].length()); ++st)
                set (l, st, rows[l][st] - '0');
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
    static size_t index (int lane, int step)
    {
        return (size_t) (juce::jlimit (0, lanes - 1, lane) * maxSteps + juce::jlimit (0, maxSteps - 1, step));
    }
    std::array<std::atomic<uint8_t>, (size_t) (lanes * maxSteps)> cells;
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
            if (at < start || at >= end)
                continue;

            const int offset = juce::jlimit (0, numSamples - 1, (int) ((at - start) * samplesPerPpq));
            const int step = (int) (k % length);
            for (int lane = 0; lane < StepPattern::lanes; ++lane)
                if (const int v = pattern.get (lane, step); v > 0)
                    out.addEvent (juce::MidiMessage::noteOn (10, noteForLane (lane), v == 2 ? 1.0f : 0.7f), offset);
            currentStep.store (step);
        }

        if (internal)
            internalPpq = end;
    }

    // Steget som spelades senast, för lampan i gränssnittet. -1 = stoppad.
    std::atomic<int> currentStep { -1 };

private:
    static constexpr double stepLen = 0.25; // en 16-del i fjärdedelar

    double sr = 44100.0;
    double internalPpq = 0.0;
    bool wasRunning = false;
};

} // namespace krang
