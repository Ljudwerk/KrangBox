// Tonstegverket – Stegverket för melodier, för alla kammare utom Hammarverket (som har sitt trummönster).
// Upp till 64 steg. Varje steg har: gate (av, ton, accent), ton i halvtoner från grundtonen (−12..+12),
// längd (en fjärdedels steg upp till fyra steg) och glid (tonen hålls kvar tills nästa ton börjat, så att kammare med
// legato glider – Basmålen, Kören i kulning, Vattenpumpens såg; i andra kammare blir det bundna toner).
//
// Spelar i DAW:ens takt när DAW:en spelar, annars med egen klocka i den fristående appen. Med "Transponera"
// flyttar en tangent på keyboardet hela slingan (som på en gammal sekvenser); med "Spela med" går tangenterna
// rakt igenom till kammaren, så att man kan spela ovanpå.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Stegverket.h" // Transport, StegverketSettings
#include <array>
#include <atomic>
#include <cmath>

namespace krang
{

// Steglängder i steg, index 0..7.
inline constexpr std::array<double, 8> tonLengths { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 };
inline const char* tonLengthName (int i)
{
    static const char* names[] = { "1/4", "1/2", "3/4", "1", "1½", "2", "3", "4" };
    return names[juce::jlimit (0, 7, i)];
}

// Skalor för slumpgeneratorn och för att visa vilka toner som hör till skalan i rutnätet.
enum class TonScale { Moll = 0, Dur, Dorisk, Frygisk, Pentaton, Hijaz, Kromatisk };
inline bool inScale (TonScale s, int semis)
{
    const int pc = ((semis % 12) + 12) % 12;
    static constexpr int masks[] = {
        0b010110101101, // moll (naturlig): 0 2 3 5 7 8 10
        0b101010110101, // dur: 0 2 4 5 7 9 11
        0b011010101101, // dorisk: 0 2 3 5 7 9 10
        0b010110101011, // frygisk: 0 1 3 5 7 8 10
        0b001010010101, // pentaton (dur): 0 2 4 7 9
        0b010110110011, // hijaz: 0 1 4 5 7 8 10
        0b111111111111,
    };
    return (masks[juce::jlimit (0, 6, (int) s)] >> pc) & 1;
}

struct TonStep
{
    int gate = 0;     // 0 av, 1 ton, 2 accent
    int semis = 0;    // −12..+12 från grundtonen
    int length = 2;   // index i tonLengths (2 = 3/4 steg)
    bool slide = false;

    bool operator== (const TonStep& o) const { return gate == o.gate && semis == o.semis && length == o.length && slide == o.slide; }
};

// Mönstret delas mellan gränssnittet och ljudtråden; varje steg är ett atomiskt ord.
class TonPattern
{
public:
    static constexpr int maxSteps = 64;

    TonPattern() { clear(); }

    TonStep get (int step) const { return unpack (cells[index (step)].load (std::memory_order_relaxed)); }
    void set (int step, const TonStep& s) { cells[index (step)].store (pack (s), std::memory_order_relaxed); }
    void clear() { for (auto& c : cells) c.store (pack ({}), std::memory_order_relaxed); }

    // Text: ett ord per steg, skilda med mellanslag. "." = paus, annars halvtoner med valfria tillägg:
    // "a" accent, "~" glid, "/n" längdindex. Exempel: "0 . 3a 5~/3 7/5".
    juce::String toString (int numSteps = maxSteps) const
    {
        juce::StringArray words;
        for (int i = 0; i < numSteps; ++i)
        {
            const auto s = get (i);
            if (s.gate == 0 && s.semis == 0 && s.length == 2 && ! s.slide) { words.add ("."); continue; }
            juce::String w = s.gate == 0 ? "_" + juce::String (s.semis) : juce::String (s.semis);
            if (s.gate == 2) w << "a";
            if (s.slide) w << "~";
            if (s.length != 2) w << "/" << s.length;
            words.add (w);
        }
        return words.joinIntoString (" ");
    }

    // Läser text som ovan; ett kortare mönster upprepas över alla 64 steg.
    void fromString (const juce::String& text)
    {
        clear();
        const auto words = juce::StringArray::fromTokens (text, " ", "");
        juce::Array<TonStep> steps;
        for (auto w : words)
        {
            w = w.trim();
            if (w.isEmpty()) continue;
            TonStep s;
            if (w == ".") { steps.add (s); continue; }
            bool off = false;
            if (w.startsWithChar ('_')) { off = true; w = w.substring (1); }
            if (w.contains ("/")) { s.length = juce::jlimit (0, 7, w.fromLastOccurrenceOf ("/", false, false).getIntValue()); w = w.upToLastOccurrenceOf ("/", false, false); }
            if (w.containsChar ('~')) { s.slide = true; w = w.removeCharacters ("~"); }
            const bool accent = w.containsChar ('a');
            w = w.removeCharacters ("a");
            s.semis = juce::jlimit (-12, 12, w.getIntValue());
            s.gate = off ? 0 : (accent ? 2 : 1);
            steps.add (s);
        }
        if (steps.isEmpty()) return;
        for (int i = 0; i < maxSteps; ++i)
            set (i, steps[i % steps.size()]);
    }

    // Startmönster: en enkel basgång i moll som visar glid och accent.
    void loadDefault() { fromString ("0a . 0 12/1 . 0 3~ 5 . 7a . 5 3 . 2~/3 0"); }

    // Doktor Krang komponerar: en slumpad slinga i vald skala, med tyngd på grundton och kvint.
    void randomise (TonScale scale, int numSteps, juce::Random& r)
    {
        clear();
        int last = 0;
        for (int i = 0; i < numSteps; ++i)
        {
            const bool downbeat = i % 4 == 0;
            if (r.nextFloat() > (downbeat ? 0.9f : 0.55f)) continue;
            int semis = last;
            for (int tries = 0; tries < 24; ++tries)
            {
                const float u = r.nextFloat();
                semis = u < 0.25f ? 0 : u < 0.35f ? 7 : u < 0.42f ? 12 : juce::jlimit (-12, 12, last + r.nextInt ({ -5, 6 }));
                if (inScale (scale, semis)) break;
            }
            last = semis;
            TonStep s;
            s.gate = downbeat && r.nextFloat() < 0.5f ? 2 : 1;
            s.semis = semis;
            s.length = r.nextFloat() < 0.15f ? 5 : (r.nextFloat() < 0.5f ? 1 : 2);
            s.slide = r.nextFloat() < 0.15f;
            set (i, s);
        }
        // Repetera första takten om slingan är längre, så att den låter som en figur och inte som brus.
        for (int i = juce::jmin (numSteps, 16); i < maxSteps; ++i)
            if (i >= numSteps || r.nextFloat() < 0.6f) set (i, get (i % 16));
    }

private:
    static uint32_t pack (const TonStep& s)
    {
        return (uint32_t) (juce::jlimit (0, 2, s.gate)) | ((uint32_t) (juce::jlimit (-12, 12, s.semis) + 12) << 2)
             | ((uint32_t) juce::jlimit (0, 7, s.length) << 7) | ((s.slide ? 1u : 0u) << 10);
    }
    static TonStep unpack (uint32_t v)
    {
        TonStep s;
        s.gate = (int) (v & 3u);
        s.semis = (int) ((v >> 2) & 31u) - 12;
        s.length = (int) ((v >> 7) & 7u);
        s.slide = ((v >> 10) & 1u) != 0;
        return s;
    }
    static size_t index (int step) { return (size_t) juce::jlimit (0, maxSteps - 1, step); }

    std::array<std::atomic<uint32_t>, (size_t) maxSteps> cells;
};

enum class TonKeyMode { SpelaMed = 0, Transponera };

struct TonSettings
{
    int root = 36;                          // grundton (MIDI), C2
    TonKeyMode keys = TonKeyMode::Transponera;
    int rate = 1;                           // 0 = 1/8, 1 = 1/16, 2 = 1/16-triol, 3 = 1/32
    TonScale scale = TonScale::Moll;
};

class TonStegverket
{
public:
    static constexpr int midiChannel = 1;

    void prepare (double sampleRate) { sr = sampleRate; keep.ensureSize (2048); reset(); }

    void reset()
    {
        internalPpq = 0.0;
        lastEnd = -1.0;
        wasRunning = false;
        numActive = 0;
        transpose = 0;
        shownTranspose.store (0);
        currentStep.store (-1);
    }

    // Lägger in sekvensens toner i midi (och tar hand om tangenterna i läget Transponera).
    void process (juce::MidiBuffer& midi, const TonPattern& pattern, const StegverketSettings& s, const TonSettings& ts,
                  const Transport& t, bool useInternalClock, int numSamples)
    {
        const bool host = t.hostPlaying;
        const bool internal = ! host && useInternalClock && s.run;
        const bool running = s.run && (host || internal);

        // Tangenterna: i Transponera-läget flyttar de slingan och hörs inte själva.
        if (running && ts.keys == TonKeyMode::Transponera)
        {
            keep.clear();
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (m.isNoteOn())
                {
                    transpose = juce::jlimit (-36, 36, m.getNoteNumber() - ts.root);
                    shownTranspose.store (transpose, std::memory_order_relaxed);
                }
                else if (! m.isNoteOff()) keep.addEvent (m, meta.samplePosition);
            }
            midi.swapWith (keep);
        }

        if (! running)
        {
            if (wasRunning) flushAll (midi, 0);
            wasRunning = false;
            currentStep.store (-1);
            return;
        }

        if (internal && ! wasRunning)
            internalPpq = -1.0e-9;          // starta exakt på steg 0
        wasRunning = true;

        const double bpm = juce::jlimit (20.0, 400.0, host ? t.bpm : s.tempo);
        const double samplesPerPpq = sr * 60.0 / bpm;
        const double start = host ? t.ppq : internalPpq;
        const double end = start + numSamples / samplesPerPpq;

        // DAW:en hoppade (loop eller ny position): släpp allt som klingar.
        if (lastEnd >= 0.0 && std::abs (start - lastEnd) > 0.01)
            flushAll (midi, 0);
        lastEnd = end;

        const double stepLen = rateLength (ts.rate);
        const int length = juce::jlimit (1, TonPattern::maxSteps, s.length);
        const double swingOffset = juce::jlimit (0.0f, 0.5f, s.swing) * stepLen;
        auto stepTime = [&] (juce::int64 k) { return (double) k * stepLen + ((k & 1) ? swingOffset : 0.0); };
        auto toSample = [&] (double ppq) { return juce::jlimit (0, numSamples - 1, (int) ((ppq - start) * samplesPerPpq)); };

        // Händelserna i tidsordning: först släpp, sedan nya steg (så att en ton som slutar på samma
        // sample som nästa börjar inte stänger av den nya).
        const auto firstK = (juce::int64) std::floor (start / stepLen) - 1;
        const auto lastK  = (juce::int64) std::floor (end / stepLen);
        for (auto k = juce::jmax ((juce::int64) 0, firstK); k <= lastK; ++k)
        {
            const double at = stepTime (k);
            if (at < start || at >= end)
                continue;
            releaseUntil (midi, at, start, samplesPerPpq, numSamples);

            const int step = (int) (k % length);
            currentStep.store (step);
            const auto st = pattern.get (step);
            if (st.gate == 0) continue;

            const int note = juce::jlimit (0, 127, ts.root + st.semis + transpose);
            double off = at + stepLen * tonLengths[(size_t) st.length];
            if (st.slide)
            {
                // Glid: tonen hålls kvar tills nästa ton har börjat (legato), även över pauser.
                for (int d = 1; d < length; ++d)
                    if (pattern.get ((int) ((k + d) % length)).gate > 0)
                    {
                        off = juce::jmax (off, stepTime (k + d) + stepLen * 0.05);
                        break;
                    }
            }

            if (auto* a = findActive (note))
            {
                // Samma ton klingar redan. Glider förra steget in i den: bind ihop och förläng.
                // Annars slås den an på nytt.
                if (a->offPpq > at + 1.0e-9 && slidingInto (pattern, k, length))
                {
                    a->offPpq = juce::jmax (a->offPpq, off);
                    continue;
                }
                midi.addEvent (juce::MidiMessage::noteOff (midiChannel, note), toSample (at));
                *a = active[(size_t) --numActive];
            }
            midi.addEvent (juce::MidiMessage::noteOn (midiChannel, note, st.gate == 2 ? 1.0f : 0.72f), toSample (at));
            addActive (note, off);
        }
        releaseUntil (midi, end, start, samplesPerPpq, numSamples);

        if (internal)
            internalPpq = end;
    }

    // För gränssnittet: steget som spelas (−1 = stoppad) och hur långt tangenterna har flyttat slingan.
    std::atomic<int> currentStep { -1 }, shownTranspose { 0 };

    static double rateLength (int rate)
    {
        switch (rate)
        {
            case 0:  return 0.5;
            case 2:  return 1.0 / 6.0;
            case 3:  return 0.125;
            default: return 0.25;
        }
    }

private:
    struct Active { int note; double offPpq; };

    // Glider närmast föregående ton (före steg k) in i den här?
    static bool slidingInto (const TonPattern& pattern, juce::int64 k, int length)
    {
        for (int d = 1; d < length; ++d)
        {
            const auto prev = pattern.get ((int) (((k - d) % length + length) % length));
            if (prev.gate > 0) return prev.slide;
        }
        return false;
    }

    Active* findActive (int note)
    {
        for (int i = 0; i < numActive; ++i)
            if (active[(size_t) i].note == note) return &active[(size_t) i];
        return nullptr;
    }

    void addActive (int note, double off)
    {
        if (numActive < (int) active.size())
            active[(size_t) numActive++] = { note, off };
    }

    // Släpper de toner vars längd tar slut före tidpunkten 'until'.
    void releaseUntil (juce::MidiBuffer& midi, double until, double blockStart, double samplesPerPpq, int numSamples)
    {
        for (int i = 0; i < numActive;)
        {
            if (active[(size_t) i].offPpq <= until)
            {
                const int at = juce::jlimit (0, numSamples - 1, (int) ((juce::jmax (blockStart, active[(size_t) i].offPpq) - blockStart) * samplesPerPpq));
                midi.addEvent (juce::MidiMessage::noteOff (midiChannel, active[(size_t) i].note), at);
                active[(size_t) i] = active[(size_t) --numActive];
            }
            else ++i;
        }
    }

    void flushAll (juce::MidiBuffer& midi, int at)
    {
        for (int i = 0; i < numActive; ++i)
            midi.addEvent (juce::MidiMessage::noteOff (midiChannel, active[(size_t) i].note), at);
        numActive = 0;
    }

    double sr = 44100.0, internalPpq = 0.0, lastEnd = -1.0;
    bool wasRunning = false;
    int transpose = 0;
    std::array<Active, 16> active {};
    int numActive = 0;
    juce::MidiBuffer keep;
};

} // namespace krang
