// Modulationsraden – två LFO:er, två modulationsenvelopes, modhjul och aftertouch, och en matris med sex
// kopplingar där vilken källa som helst kan styra vilket mål som helst.
//
// Modulationen räknas om var 64:e sample och läggs på motorns parametrar innan de går ut till kamrarna,
// precis som Krangspaken – så allt den gör hörs som vanliga inställningar som rör sig.
// Målet "Klang" betyder olika saker i olika kammare: det som gör ljudet ljusare eller skarpare där
// (filtret i Basmålen, vokalen i Kören, stråktrycket i Vattenpumpen, och så vidare).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

namespace krang
{

enum class LfoShape { Sinus = 0, Triangel, Fyrkant, Sagtand, Slump, Glid };
enum class ModSource { Ingen = 0, Lfo1, Lfo2, Env1, Env2, Modhjul, Aftertouch, Anslag };
enum class ModDest { Inget = 0, Klang, Krangdrive, Filterfrekvens, Filtervokal, Volym, Eko, Katedral, Kaos, Bandwow, Krangspaken };

struct LfoParams
{
    LfoShape shape = LfoShape::Sinus;
    float rateHz = 2.0f;    // i fritt läge
    int sync = 0;           // 0 = fri, 1 = 4 takter, 2 = 2 takter, 3 = 1 takt, 4 = 1/2, 5 = 1/4, 6 = 1/8, 7 = 1/16, 8 = 1/4 triol
    bool retrigger = false; // börja om vid varje ny ton
};

struct ModEnvParams { float attackSec = 0.01f, decaySec = 0.3f, sustain = 0.0f, releaseSec = 0.3f; };

struct ModSlot { ModSource source = ModSource::Ingen; ModDest dest = ModDest::Inget; float amount = 0.0f; };

struct ModParams
{
    std::array<LfoParams, 2> lfo;
    std::array<ModEnvParams, 2> env;
    std::array<ModSlot, 6> slots;

    bool active() const
    {
        for (auto& s : slots)
            if (s.source != ModSource::Ingen && s.dest != ModDest::Inget && std::abs (s.amount) > 0.0005f) return true;
        return false;
    }
};

class Modulation
{
public:
    static constexpr int numSources = 8, numDests = 11;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& e : envs) e.setSampleRate (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& p : phase) p = 0.0;
        lastTurn = { -1, -1 };
        for (auto& v : held) v = 0.0f;
        for (auto& v : target) v = 0.0f;
        for (auto& e : envs) e.reset();
        notesDown = 0;
        values.fill (0.0f);
    }

    void setParams (const ModParams& p)
    {
        params = p;
        for (int i = 0; i < 2; ++i)
            envs[(size_t) i].setParameters ({ juce::jmax (0.001f, p.env[(size_t) i].attackSec), juce::jmax (0.001f, p.env[(size_t) i].decaySec),
                                              juce::jlimit (0.0f, 1.0f, p.env[(size_t) i].sustain), juce::jmax (0.001f, p.env[(size_t) i].releaseSec) });
    }

    // Var i takten DAW:en är (för synkade LFO:er); utan DAW går LFO:erna fritt i tempot.
    void setTransport (bool playing, double ppq, double bpm) { hostPlaying = playing; hostPpq = ppq; tempo = bpm; resync = playing; }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
        {
            ++notesDown;
            velocity = m.getFloatVelocity();
            for (auto& e : envs) e.noteOn();               // varje ny ton startar envelopen från början
            for (int i = 0; i < 2; ++i)
                if (params.lfo[(size_t) i].retrigger) phase[(size_t) i] = 0.0;
        }
        else if (m.isNoteOff())
        {
            notesDown = juce::jmax (0, notesDown - 1);
            if (notesDown == 0) for (auto& e : envs) e.noteOff();
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            notesDown = 0;
            for (auto& e : envs) e.noteOff();
        }
        else if (m.isController() && m.getControllerNumber() == 1)
            wheel = (float) m.getControllerValue() / 127.0f;
        else if (m.isChannelPressure())
            pressure = (float) m.getChannelPressureValue() / 127.0f;
        else if (m.isAftertouch())
            pressure = (float) m.getAfterTouchValue() / 127.0f;
    }

    // Stegar fram modulationen numSamples sample och räknar ut källornas nya värden.
    void advance (int numSamples)
    {
        for (int i = 0; i < 2; ++i)
        {
            const auto& lp = params.lfo[(size_t) i];
            static constexpr double beats[9] = { 0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 2.0 / 3.0 };
            double cycle; // fas i varv
            if (lp.sync > 0)
            {
                const double len = beats[juce::jlimit (1, 8, lp.sync)];
                if (resync && hostPlaying && ! lp.retrigger)
                    phase[(size_t) i] = hostPpq / len;         // låst till DAW:ens takt vid varje block
                cycle = phase[(size_t) i];
                phase[(size_t) i] += (double) numSamples * tempo / 60.0 / sr / len;
            }
            else
            {
                phase[(size_t) i] += (double) numSamples * (double) lp.rateHz / sr;
                cycle = phase[(size_t) i];
            }
            const double frac = cycle - std::floor (cycle);
            const auto turn = (long long) std::floor (cycle);
            if (turn != lastTurn[(size_t) i])
            {
                lastTurn[(size_t) i] = turn;
                held[(size_t) i] = target[(size_t) i];
                target[(size_t) i] = random.nextFloat() * 2.0f - 1.0f;   // nytt slumpvärde varje varv
            }
            float v = 0.0f;
            switch (lp.shape)
            {
                case LfoShape::Sinus:    v = (float) std::sin (juce::MathConstants<double>::twoPi * frac); break;
                case LfoShape::Triangel: v = (float) (frac < 0.5 ? 4.0 * frac - 1.0 : 3.0 - 4.0 * frac); break;
                case LfoShape::Fyrkant:  v = frac < 0.5 ? 1.0f : -1.0f; break;
                case LfoShape::Sagtand:  v = (float) (1.0 - 2.0 * frac); break;
                case LfoShape::Slump:    v = target[(size_t) i]; break;
                case LfoShape::Glid:     v = held[(size_t) i] + (target[(size_t) i] - held[(size_t) i]) * (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * frac)); break;
            }
            values[(size_t) ModSource::Lfo1 + (size_t) i] = v;
        }
        resync = false;
        for (int i = 0; i < 2 && numSamples > 0; ++i)
        {
            float e = 0.0f;
            for (int s = 0; s < numSamples; ++s) e = envs[(size_t) i].getNextSample();
            values[(size_t) ModSource::Env1 + (size_t) i] = e;
        }
        values[(size_t) ModSource::Modhjul] = wheel;
        values[(size_t) ModSource::Aftertouch] = pressure;
        values[(size_t) ModSource::Anslag] = velocity;
    }

    float sourceValue (ModSource s) const { return values[(size_t) s]; }

    // Summan av alla kopplingar till ett mål, ungefär −1..1.
    float destValue (ModDest d) const
    {
        float sum = 0.0f;
        for (auto& s : params.slots)
            if (s.dest == d && s.source != ModSource::Ingen) sum += values[(size_t) s.source] * s.amount;
        return juce::jlimit (-2.0f, 2.0f, sum);
    }

    const ModParams& getParams() const { return params; }

private:
    double sr = 44100.0, tempo = 120.0, hostPpq = 0.0;
    bool hostPlaying = false, resync = false;
    ModParams params;
    std::array<juce::ADSR, 2> envs;
    std::array<double, 2> phase {};
    std::array<long long, 2> lastTurn { -1, -1 };
    std::array<float, 2> held {}, target {};
    std::array<float, numSources> values {};
    int notesDown = 0;
    float wheel = 0.0f, pressure = 0.0f, velocity = 0.0f;
    juce::Random random { 0x4d4f4455 }; // "MODU"
};

} // namespace krang
