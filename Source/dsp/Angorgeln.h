// Ångorgeln – KrangBox tolfte kammare: en orgel som drivs av ånga i stället för luft (fas 3c).
//
// Helt syntetiskt. Varje ton är fem pipregister (16', 8', 4', 2⅔' och 2') som dras ut som på en
// riktig orgel. Pipornas klang kommer från vågtabeller för fyra orgeltyper:
//   Kyrkorgel  – principalpipor, fyllig och rund
//   Kalliope   – ångvisslor på en mässingsorgel (som på en tivolikarusell): ren ton, mycket ånga, ostämd
//   Harmonium  – tungor i stället för pipor: ljus och nasal
//   Positiv    – täckta flöjtpipor: mjuk och ihålig, mest udda övertoner
// Det som gör den till en ångorgel:
//   Ångtryck  – med lågt tryck sjunker tonen och vacklar, och ju fler toner som hålls desto mer
//               sjunker trycket (som när en orgelbälg inte hinner med)
//   Pipblås   – det korta pipljudet när en pipa börjar tala
//   Läckage   – ånga som pyser ur trasiga ventiler, och pipor som har dragit sig ur stämningen
//   Tremulant – trycket fladdrar, som orgelns tremulant
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Stamning.h"
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

enum class OrganType { Kyrkorgel = 0, Kalliope, Harmonium, Positiv };

struct AngorgelnParams
{
    OrganType type = OrganType::Kyrkorgel;
    std::array<float, 5> ranks { 0.4f, 1.0f, 0.6f, 0.0f, 0.3f };   // 16', 8', 4', 2⅔', 2' (0..1)
    float pressure  = 0.75f;   // ångtryck 0..1
    float chiff     = 0.4f;    // pipblås 0..1
    float leak      = 0.1f;    // läckage 0..1
    float tremulant = 0.0f;    // 0..1
    float attackSec = 0.03f;
    float releaseSec = 0.25f;
};

class Angorgeln
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    static constexpr int numVoices = 12, numRanks = 5, tableSize = 2048, numBands = 4;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        buildTables();
        for (auto& v : voices) v.env.setSampleRate (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) { v.env.reset(); v.note = -1; v.keyDown = false; }
        windDrop = 0.0f;
        wander = wanderTarget = 0.0f;
        wanderTimer = 0;
        tremPhase = 0.0;
        hissLp = hissHp = 0.0f;
    }

    void setParams (const AngorgelnParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.005f, p.attackSec), 0.1f, 1.0f, juce::jmax (0.02f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
    }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.note == note && v.env.isActive()) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.env.isActive()) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        if (! v.env.isActive())
            for (auto& ph : v.phase) ph = random.nextDouble();   // piporna börjar inte i fas
        v.note = note;
        v.keyDown = true;
        v.velocity = 0.75f + 0.25f * velocity;    // en orgel bryr sig nästan inte om anslaget
        v.age = ++ageCounter;
        v.t = 0.0;
        // Pipor som dragit sig ur stämningen: samma pipa är lika ostämd varje gång.
        const auto h = (uint32_t) (note * 2654435761u);
        v.cipherCents = ((float) ((h >> 8) & 1023u) / 1023.0f * 2.0f - 1.0f);
        v.env.noteOn();
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.note == note && v.keyDown) { v.env.noteOff(); v.keyDown = false; }
    }

    void allNotesOff()
    {
        for (auto& v : voices) { v.env.noteOff(); v.keyDown = false; }
    }

    void render (float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        static constexpr double footage[numRanks] = { 0.5, 1.0, 2.0, 3.0, 4.0 };
        // Större pipor talar långsammare.
        static constexpr double speech[numRanks] = { 0.06, 0.035, 0.022, 0.016, 0.012 };
        const int type = (int) params.type;
        const bool calliope = params.type == OrganType::Kalliope;
        const float lack = 1.0f - juce::jlimit (0.0f, 1.0f, params.pressure);

        int held = 0;
        for (auto& v : voices) if (v.env.isActive()) ++held;

        for (int i = 0; i < numSamples; ++i)
        {
            // Trycket: bälgen hinner inte med när många toner hålls, och ångan vandrar långsamt.
            const float dropTarget = lack * (float) held / 6.0f;
            windDrop += (dropTarget - windDrop) * 0.0004f;
            if (--wanderTimer <= 0)
            {
                wanderTarget = random.nextFloat() * 2.0f - 1.0f;
                wanderTimer = (int) (sr * (0.15 + 0.5 * random.nextFloat()));
            }
            wander += (wanderTarget - wander) * 0.0003f;
            tremPhase += 6.2 / sr;  tremPhase -= std::floor (tremPhase);
            const float trem = params.tremulant * (float) std::sin (twoPi * tremPhase);

            const float pitchCents = -lack * (calliope ? 45.0f : 25.0f) - windDrop * 40.0f
                                   + wander * (lack * 18.0f + (calliope ? 8.0f : 0.0f)) + trem * 9.0f;
            const float windGain = (1.0f - 0.35f * windDrop) * (1.0f + 0.25f * trem);

            float out = 0.0f;
            float breathSum = 0.0f;
            for (auto& v : voices)
            {
                if (! v.env.isActive()) continue;
                const float env = v.env.getNextSample();
                const double baseHz = stamning->hz (v.note) * std::exp2 ((pitchCents + v.cipherCents * params.leak * 35.0f) / 1200.0);
                float pipes = 0.0f;
                for (int r = 0; r < numRanks; ++r)
                {
                    const float level = params.ranks[(size_t) r];
                    if (level < 0.001f) continue;
                    const double hz = baseHz * footage[r];
                    if (hz > sr * 0.45) continue;
                    v.phase[(size_t) r] += hz / sr;
                    v.phase[(size_t) r] -= std::floor (v.phase[(size_t) r]);
                    const float rise = 1.0f - (float) std::exp (-v.t / speech[r]);
                    pipes += lookup (type, bandFor (type, hz), v.phase[(size_t) r]) * level * rise;
                }
                // Pipblås: ett kort brusande "tju" när pipan börjar tala (kalliopen blåser hela tiden).
                const float chiff = params.chiff * (float) std::exp (-v.t / 0.035) + (calliope ? 0.12f + 0.25f * lack : 0.0f);
                breathSum += chiff * env * v.velocity;
                out += pipes * env * v.velocity;
                v.t += 1.0 / sr;
            }

            // Ånga och pipblås: brus i ett band runt 2–4 kHz, och läckaget som pyser hela tiden när orgeln spelar.
            const float n = random.nextFloat() * 2.0f - 1.0f;
            hissLp += (n - hissLp) * 0.35f;
            hissHp += (hissLp - hissHp) * 0.06f;
            const float hiss = hissLp - hissHp;
            const float leakHiss = held > 0 ? params.leak * 0.08f : 0.0f;
            out = out * windGain * 0.24f + hiss * (breathSum * 0.25f + leakHiss);
            dest[i] = out;
        }
        for (auto& v : voices) if (! v.env.isActive()) v.note = -1;
    }

private:
    struct Voice
    {
        juce::ADSR env;
        int note = -1;
        bool keyDown = false;
        float velocity = 1.0f, cipherCents = 0.0f;
        double t = 0.0;
        std::array<double, numRanks> phase {};
        juce::uint32 age = 0;
    };

    // Vågtabeller: fyra orgeltyper, var och en i fyra bandbredder (8, 4, 2 och 1 överton) så att höga
    // pipor inte viker.
    void buildTables()
    {
        static constexpr float recipes[4][8] = {
            { 1.0f, 0.55f, 0.32f, 0.18f, 0.12f, 0.07f, 0.04f, 0.03f },    // principal
            { 1.0f, 0.12f, 0.06f, 0.02f, 0.0f, 0.0f, 0.0f, 0.0f },        // kalliope: nästan ren
            { 1.0f, 0.7f, 0.55f, 0.45f, 0.38f, 0.3f, 0.25f, 0.2f },       // tunga
            { 1.0f, 0.03f, 0.22f, 0.02f, 0.08f, 0.01f, 0.03f, 0.0f } };   // täckt flöjt
        static constexpr int maxH[numBands] = { 8, 4, 2, 1 };
        tables.assign ((size_t) (4 * numBands * (tableSize + 1)), 0.0f);
        for (int t = 0; t < 4; ++t)
            for (int b = 0; b < numBands; ++b)
            {
                float* tab = &tables[(size_t) ((t * numBands + b) * (tableSize + 1))];
                float peak = 0.0f;
                for (int i = 0; i <= tableSize; ++i)
                {
                    const double ph = (double) i / tableSize;
                    double s = 0.0;
                    for (int h = 1; h <= maxH[b]; ++h)
                        s += recipes[t][h - 1] * std::sin (juce::MathConstants<double>::twoPi * h * ph);
                    tab[i] = (float) s;
                    peak = juce::jmax (peak, std::abs ((float) s));
                }
                for (int i = 0; i <= tableSize; ++i) tab[i] /= juce::jmax (0.001f, peak);
            }
    }

    int bandFor (int, double hz) const
    {
        const double room = sr * 0.45 / hz;   // hur många övertoner som ryms
        return room >= 8.0 ? 0 : room >= 4.0 ? 1 : room >= 2.0 ? 2 : 3;
    }

    float lookup (int type, int band, double phase) const
    {
        const float* tab = &tables[(size_t) ((type * numBands + band) * (tableSize + 1))];
        const double x = phase * tableSize;
        const int i = (int) x;
        const float f = (float) (x - i);
        return tab[i] + f * (tab[i + 1] - tab[i]);
    }

    double sr = 44100.0;
    AngorgelnParams params;
    std::array<Voice, numVoices> voices;
    std::vector<float> tables;
    float windDrop = 0.0f, wander = 0.0f, wanderTarget = 0.0f, hissLp = 0.0f, hissHp = 0.0f;
    int wanderTimer = 0;
    double tremPhase = 0.0;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x414e474f }; // "ANGO"
};

} // namespace krang
