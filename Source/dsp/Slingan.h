// Slingan – arpeggiator för Spolslingan (fas 3).
// Tar de toner som hålls nere och spelar dem i takt: Upp, Ned, Upp/ned, Slump, Som spelat, Ackord,
// Tvärs (utifrån och in) eller Trappa, över 1–3 oktaver, med valbar tonlängd (gate) och Fäst (latch). Följer DAW:ens transport när den spelar, annars en egen klocka som startar
// när första tangenten trycks ned. Arbetar på MIDI-bufferten före ljudmotorn.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Stegverket.h" // Transport
#include <array>
#include <cmath>

namespace krang
{

// Lägen. Nya lägen läggs alltid sist, så att sparade projekt behåller sitt läge.
enum class ArpMode { Av = 0, Upp, Ned, UppNed, Slump, SomSpelat, Ackord, Tvars, Trappa };

struct SlinganSettings
{
    ArpMode mode = ArpMode::Av;
    int rate = 1;        // 0 = 1/8, 1 = 1/16, 2 = 1/16-triol, 3 = 1/32
    int octaves = 1;     // 1..3
    double tempo = 96.0; // egen klocka när DAW:en inte spelar
    float gate = 0.5f;   // hur stor del av steget tonen klingar, 0.1..1
    bool latch = false;  // Fäst: tonerna fortsätter när tangenterna släpps, tills nästa ackord
};

class Slingan
{
public:
    void prepare (double sampleRate) { sr = sampleRate; passthrough.ensureSize (2048); reset(); }

    void reset()
    {
        numHeld = numOrder = keysDown = 0;
        numSounding = 0;
        step = 0;
        ppq = 0.0;
        running = false;
    }

    // Ersätter tangenttrycken i midi med arpeggierade toner. Annat (pitch bend, CC) passerar.
    void process (juce::MidiBuffer& midi, const SlinganSettings& s, const Transport& t, int numSamples)
    {
        if (s.mode == ArpMode::Av)
        {
            releaseAll (midi, 0);
            reset();
            return;
        }

        // Samla tangenttryck (och ta bort dem ur flödet). passthrough är förallokerad i prepare().
        passthrough.clear();
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
            {
                // Med Fäst börjar ett nytt ackord när man trycker efter att ha släppt allt.
                if (s.latch && keysDown == 0) { numHeld = numOrder = 0; step = 0; }
                ++keysDown;
                addHeld (m.getNoteNumber());
            }
            else if (m.isNoteOff())
            {
                keysDown = juce::jmax (0, keysDown - 1);
                if (! s.latch) removeHeld (m.getNoteNumber());
            }
            else passthrough.addEvent (m, meta.samplePosition);
        }
        if (! s.latch && keysDown == 0) numHeld = numOrder = 0;   // Fäst av: släppta toner tystnar
        midi.swapWith (passthrough);

        static constexpr double stepPpq[4] = { 0.5, 0.25, 1.0 / 6.0, 0.125 };
        const double len = stepPpq[juce::jlimit (0, 3, s.rate)];
        const bool host = t.hostPlaying;
        const double bpm = juce::jlimit (20.0, 400.0, host ? t.bpm : s.tempo);
        const double samplesPerPpq = sr * 60.0 / bpm;

        if (numHeld == 0)
        {
            releaseAll (midi, 0);
            running = false;
            step = 0;
            return;
        }

        if (! host && ! running) { ppq = -1.0e-9; running = true; } // egen klocka: starta direkt
        const double start = host ? t.ppq : ppq;
        const double end = start + numSamples / samplesPerPpq;
        const double gate = juce::jlimit (0.1, 1.0, (double) s.gate);

        // Toner vars gate tar slut i blocket (från tidigare steg).
        releaseUntil (midi, end, start, samplesPerPpq, numSamples, false);

        for (auto k = (juce::int64) std::floor (start / len); (double) k * len < end; ++k)
        {
            const double at = (double) k * len;
            if (at < start) continue;
            const int offset = juce::jlimit (0, numSamples - 1, (int) ((at - start) * samplesPerPpq));
            releaseUntil (midi, at + 1.0e-9, start, samplesPerPpq, numSamples, true);   // allt som klingar släpps före nästa steg
            const double off = at + len * gate - (gate >= 0.999 ? 1.0e-6 : 0.0);
            if (s.mode == ArpMode::Ackord)
            {
                // Hela ackordet på varje steg, i en oktav i taget uppåt.
                const int octave = (step++ % juce::jlimit (1, 3, s.octaves));
                for (int i = 0; i < numHeld; ++i)
                    sound (midi, juce::jlimit (0, 127, held[(size_t) i] + 12 * octave), offset, off);
            }
            else
                sound (midi, nextNote (s), offset, off);
            releaseUntil (midi, end, start, samplesPerPpq, numSamples, false);
        }
        if (! host) ppq = end;
    }

private:
    void sound (juce::MidiBuffer& midi, int note, int offset, double offPpq)
    {
        midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), offset);
        if (numSounding < (int) sounding.size())
            sounding[(size_t) numSounding++] = { note, offPpq };
    }

    // Släpper toner vars slut ligger före 'until' (eller alla, med 'all').
    void releaseUntil (juce::MidiBuffer& midi, double until, double blockStart, double samplesPerPpq, int numSamples, bool all)
    {
        for (int i = 0; i < numSounding;)
        {
            auto& n = sounding[(size_t) i];
            if (all || n.offPpq < until)
            {
                const int at = juce::jlimit (0, numSamples - 1, (int) ((juce::jmax (blockStart, juce::jmin (n.offPpq, until)) - blockStart) * samplesPerPpq));
                midi.addEvent (juce::MidiMessage::noteOff (1, n.note), at);
                n = sounding[(size_t) --numSounding];
            }
            else ++i;
        }
    }

    void releaseAll (juce::MidiBuffer& midi, int at)
    {
        for (int i = 0; i < numSounding; ++i)
            midi.addEvent (juce::MidiMessage::noteOff (1, sounding[(size_t) i].note), at);
        numSounding = 0;
    }

    void addHeld (int note)
    {
        for (int i = 0; i < numHeld; ++i) if (held[(size_t) i] == note) return;
        if (numHeld >= (int) held.size()) return;
        // Håll listan sorterad nerifrån och upp, och en lista i den ordning tonerna spelades.
        int i = numHeld++;
        while (i > 0 && held[(size_t) i - 1] > note) { held[(size_t) i] = held[(size_t) i - 1]; --i; }
        held[(size_t) i] = note;
        order[(size_t) numOrder++] = note;
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int r = 0; r < numHeld; ++r) if (held[(size_t) r] != note) held[(size_t) w++] = held[(size_t) r];
        numHeld = w;
        w = 0;
        for (int r = 0; r < numOrder; ++r) if (order[(size_t) r] != note) order[(size_t) w++] = order[(size_t) r];
        numOrder = w;
    }

    int nextNote (const SlinganSettings& s)
    {
        const int octaves = juce::jlimit (1, 3, s.octaves);
        const int total = numHeld * octaves;
        auto noteAt = [this] (int i) { return held[(size_t) (i % numHeld)] + 12 * (i / numHeld); };
        int note = held[0];
        switch (s.mode)
        {
            case ArpMode::Upp:    note = noteAt (step % total); break;
            case ArpMode::Ned:    note = noteAt (total - 1 - (step % total)); break;
            case ArpMode::UppNed:
            {
                const int cycle = juce::jmax (1, 2 * total - 2);
                const int p = step % cycle;
                note = noteAt (p < total ? p : cycle - p);
                break;
            }
            case ArpMode::Slump:  note = noteAt (random.nextInt (total)); break;
            case ArpMode::SomSpelat:
            {
                // I den ordning tangenterna trycktes, oktav för oktav.
                const int i = step % (numOrder * octaves);
                note = order[(size_t) (i % numOrder)] + 12 * (i / numOrder);
                break;
            }
            case ArpMode::Tvars:
            {
                // Utifrån och in: lägsta, högsta, näst lägsta, näst högsta …
                const int i = step % total;
                note = noteAt ((i % 2 == 0) ? i / 2 : total - 1 - i / 2);
                break;
            }
            case ArpMode::Trappa:
            {
                // Två steg upp, ett ned: 1 3 2 4 3 5 … som en trappa.
                const int cycle = juce::jmax (1, total);
                const int pair = step / 2, second = step % 2;
                note = noteAt ((pair + (second == 0 ? 0 : 2)) % cycle);
                if (total < 3) note = noteAt (step % cycle);
                break;
            }
            case ArpMode::Ackord: case ArpMode::Av: break;
        }
        ++step;
        return juce::jlimit (0, 127, note);
    }

    struct Sounding { int note; double offPpq; };

    double sr = 44100.0;
    std::array<int, 16> held {}, order {};
    int numHeld = 0, numOrder = 0, keysDown = 0, step = 0;
    std::array<Sounding, 64> sounding {};
    int numSounding = 0;
    double ppq = 0.0;
    bool running = false;
    juce::MidiBuffer passthrough;
    juce::Random random { 0x534c4e47 }; // "SLNG"
};

} // namespace krang
