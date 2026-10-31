// Slingan – arpeggiator för Spolslingan (fas 3).
// Tar de toner som hålls nere och spelar dem en i taget i takt: Upp, Ned, Upp/ned eller Slump,
// över 1–3 oktaver. Följer DAW:ens transport när den spelar, annars en egen klocka som startar
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

enum class ArpMode { Av = 0, Upp, Ned, UppNed, Slump };

struct SlinganSettings
{
    ArpMode mode = ArpMode::Av;
    int rate = 1;        // 0 = 1/8, 1 = 1/16, 2 = 1/16-triol, 3 = 1/32
    int octaves = 1;     // 1..3
    double tempo = 96.0; // egen klocka när DAW:en inte spelar
};

class Slingan
{
public:
    void prepare (double sampleRate) { sr = sampleRate; passthrough.ensureSize (2048); reset(); }

    void reset()
    {
        numHeld = 0;
        playing = -1;
        step = 0;
        ppq = 0.0;
        running = false;
    }

    // Ersätter tangenttrycken i midi med arpeggierade toner. Annat (pitch bend, CC) passerar.
    void process (juce::MidiBuffer& midi, const SlinganSettings& s, const Transport& t, int numSamples)
    {
        if (s.mode == ArpMode::Av)
        {
            if (playing >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, playing), 0);
            reset();
            return;
        }

        // Samla tangenttryck (och ta bort dem ur flödet). passthrough är förallokerad i prepare().
        passthrough.clear();
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())        addHeld (m.getNoteNumber());
            else if (m.isNoteOff())  removeHeld (m.getNoteNumber());
            else                     passthrough.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (passthrough);

        if (numHeld == 0)
        {
            if (playing >= 0) { midi.addEvent (juce::MidiMessage::noteOff (1, playing), 0); playing = -1; }
            running = false;
            return;
        }

        static constexpr double stepPpq[4] = { 0.5, 0.25, 1.0 / 6.0, 0.125 };
        const double len = stepPpq[juce::jlimit (0, 3, s.rate)];
        const bool host = t.hostPlaying;
        const double bpm = juce::jlimit (20.0, 400.0, host ? t.bpm : s.tempo);
        const double samplesPerPpq = sr * 60.0 / bpm;

        if (! host && ! running) { ppq = -1.0e-9; running = true; } // egen klocka: starta direkt
        const double start = host ? t.ppq : ppq;
        const double end = start + numSamples / samplesPerPpq;

        for (auto k = (juce::int64) std::floor (start / len); (double) k * len < end; ++k)
        {
            const double at = (double) k * len;
            if (at < start) continue;
            const int offset = juce::jlimit (0, numSamples - 1, (int) ((at - start) * samplesPerPpq));
            if (playing >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, playing), offset);
            playing = nextNote (s);
            midi.addEvent (juce::MidiMessage::noteOn (1, playing, 0.8f), offset);
            // Släpp tonen efter halva steget (staccato), om det ryms i blocket; annars vid nästa steg.
            const int gateOff = offset + (int) (len * 0.5 * samplesPerPpq);
            if (gateOff < numSamples) { midi.addEvent (juce::MidiMessage::noteOff (1, playing), gateOff); playing = -1; }
        }
        if (! host) ppq = end;
    }

private:
    void addHeld (int note)
    {
        for (int i = 0; i < numHeld; ++i) if (held[(size_t) i] == note) return;
        if (numHeld >= (int) held.size()) return;
        // Håll listan sorterad nerifrån och upp.
        int i = numHeld++;
        while (i > 0 && held[(size_t) i - 1] > note) { held[(size_t) i] = held[(size_t) i - 1]; --i; }
        held[(size_t) i] = note;
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int r = 0; r < numHeld; ++r) if (held[(size_t) r] != note) held[(size_t) w++] = held[(size_t) r];
        numHeld = w;
    }

    int nextNote (const SlinganSettings& s)
    {
        const int octaves = juce::jlimit (1, 3, s.octaves);
        const int total = numHeld * octaves;
        auto noteAt = [this] (int i) { return held[(size_t) (i % numHeld)] + 12 * (i / numHeld); };
        int index = 0;
        switch (s.mode)
        {
            case ArpMode::Upp:    index = step % total; break;
            case ArpMode::Ned:    index = total - 1 - (step % total); break;
            case ArpMode::UppNed:
            {
                const int cycle = juce::jmax (1, 2 * total - 2);
                const int p = step % cycle;
                index = p < total ? p : cycle - p;
                break;
            }
            case ArpMode::Slump:  index = random.nextInt (total); break;
            case ArpMode::Av:     break;
        }
        ++step;
        return juce::jlimit (0, 127, noteAt (index));
    }

    double sr = 44100.0;
    std::array<int, 16> held {};
    int numHeld = 0, playing = -1, step = 0;
    double ppq = 0.0;
    bool running = false;
    juce::MidiBuffer passthrough;
    juce::Random random { 0x534c4e47 }; // "SLNG"
};

} // namespace krang
