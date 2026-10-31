// Spolslingan – KrangBox motor för leads, arpeggion och slingor (fas 3).
// Sex röster med två ljudkällor som blandas:
//   FM: en modulator (valfritt förhållande) styr en bärvåg; modulationsindex klingar av efter anslaget,
//       så att tonen "plinkar" från ljus till mörk – som en metallspole som slås an.
//   Munharpa: ett surrande, smalt pulståg (lamellen) genom två resonanta filter vars frekvens sveper
//       över deltonerna – munhålan som formar övertonsmelodin på en munharpa.
// Efter rösterna: Radiokorn, en granulär processor som klipper sönder ljudet i korta korn med
// slumpad position, tonhöjd och riktning – som en trasig radio som tappar och hittar signalen.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

struct SpolslinganParams
{
    float ratio      = 2.0f;   // modulator / bärvåg, 0.5..8
    float index      = 3.0f;   // modulationsindex 0..10
    float indexDecay = 0.35f;  // sekunder till indexet klingat av
    float harp       = 0.0f;   // munharpans andel 0..1
    float mouth      = 0.4f;   // munhålans svephastighet 0..1 (≈ 0,2–6 Hz)
    float attackSec  = 0.005f;
    float releaseSec = 0.3f;
    float grain      = 0.0f;   // radiokornens andel 0..1
    float grainMs    = 80.0f;  // kornstorlek 10..300 ms
    float spread     = 0.4f;   // spridning i tid och tonhöjd 0..1
};

class Spolslingan
{
public:
    static constexpr int numVoices = 8;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices)
        {
            v.env.setSampleRate (sampleRate);
            for (auto& f : v.mouthFilters)
            {
                f.prepare ({ sampleRate, 64, 1 });
                f.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
                f.setResonance (7.0f);
            }
        }
        grainBuffer.assign ((size_t) (sampleRate * 2.0), 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) { v.env.reset(); v.note = -1; v.keyDown = false; for (auto& f : v.mouthFilters) f.reset(); }
        std::fill (grainBuffer.begin(), grainBuffer.end(), 0.0f);
        for (auto& g : grains) g.active = false;
        writePos = 0;
        grainTimer = 0;
    }

    void setParams (const SpolslinganParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.001f, p.attackSec), 0.4f, 0.75f, juce::jmax (0.005f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
    }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.env.isActive()) { target = &v; break; }
        if (target == nullptr) // stjäl helst en röst som redan släppts, annars den äldsta
            for (auto& v : voices)
                if (! v.keyDown && (target == nullptr || v.age < target->age)) target = &v;
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        // Nollställ bara faserna för en tyst röst – annars blir det ett klick när en röst återanvänds.
        if (! v.env.isActive())
            v.carrier = v.modulator = v.harpPhase = 0.0;
        v.note = note;
        v.keyDown = true;
        v.velocity = 0.4f + 0.6f * velocity;
        v.age = ++ageCounter;
        v.t = 0.0;
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
        const float decay = juce::jmax (0.005f, params.indexDecay);
        const double mouthHz = 0.2 * std::pow (30.0, (double) params.mouth); // 0,2–6 Hz

        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (auto& v : voices)
            {
                if (! v.env.isActive()) continue;
                const float env = v.env.getNextSample();
                const double hz = 440.0 * std::pow (2.0, (v.note - 69) / 12.0);

                // FM: index klingar av från fullt till 30 % efter anslaget.
                const float idx = params.index * (0.3f + 0.7f * (float) std::exp (-v.t / decay));
                v.modulator += hz * params.ratio / sr;  v.modulator -= std::floor (v.modulator);
                v.carrier   += hz / sr;                 v.carrier   -= std::floor (v.carrier);
                const double mod = std::sin (twoPi * v.modulator) * idx;
                const float fm = (float) std::sin (twoPi * v.carrier + mod);

                // Munharpa: smalt pulståg (lamellen) genom två svepande resonanser (munhålan).
                float harp = 0.0f;
                if (params.harp > 0.001f)
                {
                    v.harpPhase += hz / sr;  v.harpPhase -= std::floor (v.harpPhase);
                    const float buzz = v.harpPhase < 0.08 ? 1.0f - (float) v.harpPhase / 0.08f : 0.0f;
                    if ((v.mouthCounter++ & 15) == 0)
                    {
                        const double sweep = 0.5 + 0.5 * std::sin (twoPi * mouthHz * v.t);
                        const double h1 = juce::jlimit (60.0, sr * 0.4, hz * (4.0 + 6.0 * sweep));
                        const double h2 = juce::jlimit (60.0, sr * 0.4, hz * (9.0 + 5.0 * (1.0 - sweep)));
                        v.mouthFilters[0].setCutoffFrequency ((float) h1);
                        v.mouthFilters[1].setCutoffFrequency ((float) h2);
                    }
                    harp = (v.mouthFilters[0].processSample (0, buzz) + 0.6f * v.mouthFilters[1].processSample (0, buzz)) * 3.0f
                         + buzz * 0.15f;
                }

                out += (fm * (1.0f - params.harp) + harp * params.harp) * env * v.velocity;
                v.t += 1.0 / sr;
            }
            out *= 0.25f;
            dest[i] = params.grain > 0.001f ? processGrains (out) : out;
        }
    }

private:
    // --- Radiokorn ------------------------------------------------------------
    float processGrains (float in)
    {
        const int size = (int) grainBuffer.size();
        grainBuffer[(size_t) writePos] = in;
        writePos = (writePos + 1) % size;

        const int grainLen = juce::jmax (16, (int) (params.grainMs * 0.001f * (float) sr));
        if (--grainTimer <= 0)
        {
            grainTimer = juce::jmax (8, grainLen / 3); // ungefär tre korn överlappar
            for (auto& g : grains)
                if (! g.active)
                {
                    const float maxBack = 0.05f + params.spread * 0.9f;  // sekunder bakåt
                    const int back = grainLen + (int) (random.nextFloat() * maxBack * (float) sr);
                    g.pos = (double) ((writePos - back + size) % size);
                    g.len = grainLen;
                    g.age = 0;
                    const float semis = (random.nextFloat() * 2.0f - 1.0f) * params.spread * 12.0f;
                    g.rate = std::pow (2.0, semis / 12.0) * (random.nextFloat() < 0.3f * params.spread ? -1.0 : 1.0);
                    g.gain = random.nextFloat() < 0.15f * params.spread ? 0.0f : 1.0f; // ibland tappar radion signalen
                    g.active = true;
                    break;
                }
        }

        float wet = 0.0f;
        for (auto& g : grains)
        {
            if (! g.active) continue;
            const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) g.age / (float) g.len);
            const int i0 = ((int) g.pos % size + size) % size;
            wet += grainBuffer[(size_t) i0] * w * g.gain;
            g.pos += g.rate;
            if (g.pos < 0) g.pos += size;
            if (g.pos >= size) g.pos -= size;
            if (++g.age >= g.len) g.active = false;
        }
        wet *= 0.6f;
        return in * (1.0f - params.grain) + wet * params.grain;
    }

    struct Voice
    {
        juce::ADSR env;
        std::array<juce::dsp::StateVariableTPTFilter<float>, 2> mouthFilters;
        int note = -1;
        bool keyDown = false;
        float velocity = 1.0f;
        double t = 0.0, carrier = 0.0, modulator = 0.0, harpPhase = 0.0;
        juce::uint32 age = 0, mouthCounter = 0;
    };

    struct Grain
    {
        bool active = false;
        double pos = 0.0, rate = 1.0;
        int len = 0, age = 0;
        float gain = 1.0f;
    };

    double sr = 44100.0;
    SpolslinganParams params;
    std::array<Voice, numVoices> voices;
    std::vector<float> grainBuffer;
    std::array<Grain, 16> grains;
    int writePos = 0, grainTimer = 0;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x53504f4c }; // "SPOL"
};

} // namespace krang
