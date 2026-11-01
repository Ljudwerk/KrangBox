// Klockgjuteriet – KrangBox sjätte kammare: klockor, gonger och metallslag (fas 3b).
//
// Modalsyntes, helt syntetiskt, som Stålpianot – men med klockornas oharmoniska deltoner.
// Fem klocktyper, var och en en tabell med deltoner (förhållande till grundtonen, styrka och klingtid):
//   Kyrkklocka  – brumton under, ters och kvint ovanför: den klassiska, lite molliga kyrkklockan
//   Skälla      – fäbodens kogälla: kort, ihålig och skramlande
//   Gong        – täta deltoner som klingar länge; Svep låter tonen glida som en operagong
//   Räls        – en fri stålbalk: få, glesa och mycket oharmoniska deltoner
//   Skålklocka  – sjungande skål: långa toner som svävar i par
// Gjutningen: varje delton är ett par (klockan är aldrig helt rund), så att tonen svävar. Spricka förskjuter
// deltonerna, kortar klangen och får sprickan att skorra.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace krang
{

enum class BellType { Kyrkklocka = 0, Skalla, Gong, Rals, Skalklocka };

struct KlockgjuterietParams
{
    BellType type    = BellType::Kyrkklocka;
    float hardness   = 0.5f;   // kläppens hårdhet 0..1
    float decaySec   = 10.0f;  // klingtid 0.5..40 s
    float releaseSec = 40.0f;  // dämpning när tangenten släpps; 40 = klockan klingar ut av sig själv
    float beatCents  = 4.0f;   // svävning mellan deltonernas par 0..30 cent
    float crack      = 0.0f;   // spricka 0..1
    float hum        = 0.6f;   // brumtonens styrka 0..1
    float sweep      = 0.0f;   // tonhöjdssvep efter slaget, -1..1 (±300 cent)
};

class Klockgjuteriet
{
public:
    static constexpr int numVoices = 12;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        for (auto& v : voices) { v.active = false; v.keyDown = false; v.note = -1; v.numModes = 0; }
    }

    void setParams (const KlockgjuterietParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.level < target->level) target = &v; // den tystaste klockan får ge plats
        }
        strike (*target, note, velocity);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown)
            {
                v.keyDown = false;
                if (params.releaseSec < 39.0f) { v.damped = true; updateCoefficients (v); }
            }
    }

    void allNotesOff()
    {
        for (auto& v : voices)
            if (v.active) { v.keyDown = false; v.damped = true; updateCoefficients (v, 0.3f); }
    }

    void render (float* dest, int numSamples)
    {
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.active) renderVoice (v, dest, numSamples);
        // Mjuk mättning av slagtopparna.
        for (int i = 0; i < numSamples; ++i)
            dest[i] = std::tanh (dest[i] * 1.4f) / 1.4f;
    }

private:
    static constexpr int maxPartials = 24, maxModes = maxPartials * 2;

    struct Partial { float ratio, amp, decay; };

    struct Voice
    {
        bool active = false, keyDown = false, damped = false;
        int note = -1, numModes = 0;
        float level = 0.0f;
        BellType type = BellType::Kyrkklocka;
        std::array<float, maxModes> y1 {}, y2 {}, a1 {}, a2 {}, omega {}, sigma {}, excite {};
        int pulsePos = 0, pulseLen = 1;
        float sweepCents = 0.0f;
        int thumpLeft = 0, thumpLen = 1;
        float thumpAmp = 0.0f, thumpLp = 0.0f, thumpCoef = 0.3f;
        float crackLp = 0.0f, crackHp = 0.0f;
    };

    static float hash (int a, int b)
    {
        juce::uint32 h = (juce::uint32) (a * 7919 + b * 104729 + 777);
        h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
        return (float) (h & 0xffff) / 65535.0f;
    }

    // Deltonstabellerna. Förhållanden mot grundtonen (den ton man spelar).
    int partialsFor (BellType t, std::array<Partial, maxPartials>& out) const
    {
        auto copy = [&out] (std::initializer_list<Partial> list)
        {
            int n = 0;
            for (auto& p : list) out[(size_t) n++] = p;
            return n;
        };
        switch (t)
        {
            case BellType::Kyrkklocka:
                return copy ({ { 0.5f, params.hum, 2.2f }, { 1.0f, 0.6f, 1.4f }, { 1.183f, 0.75f, 1.0f },
                               { 1.506f, 0.35f, 0.8f }, { 2.0f, 0.9f, 0.65f }, { 2.514f, 0.35f, 0.45f }, { 2.662f, 0.3f, 0.4f },
                               { 3.011f, 0.3f, 0.35f }, { 4.166f, 0.25f, 0.25f }, { 5.433f, 0.18f, 0.18f },
                               { 6.79f, 0.12f, 0.14f }, { 8.3f, 0.08f, 0.1f } });
            case BellType::Skalla:
                return copy ({ { 1.0f, 1.0f, 0.25f }, { 1.58f, 0.7f, 0.2f }, { 2.37f, 0.6f, 0.16f }, { 3.11f, 0.5f, 0.12f },
                               { 3.79f, 0.3f, 0.1f }, { 4.42f, 0.4f, 0.08f }, { 5.6f, 0.25f, 0.06f }, { 6.9f, 0.15f, 0.05f } });
            case BellType::Rals:
                return copy ({ { 1.0f, 1.0f, 1.0f }, { 2.756f, 0.7f, 0.7f }, { 5.404f, 0.5f, 0.5f }, { 8.933f, 0.35f, 0.38f },
                               { 13.34f, 0.25f, 0.28f }, { 18.64f, 0.15f, 0.2f } });
            case BellType::Skalklocka:
                return copy ({ { 1.0f, 1.0f, 1.6f }, { 2.71f, 0.6f, 1.0f }, { 5.15f, 0.35f, 0.6f }, { 8.17f, 0.2f, 0.4f },
                               { 11.8f, 0.1f, 0.3f } });
            case BellType::Gong:
            default:
            {
                // Täta deltoner med en fast "slumpad" spridning, så att gongen alltid låter likadant.
                int n = 0;
                out[(size_t) n++] = { 0.5f, 0.8f * params.hum, 1.5f };
                for (int k = 1; k < 20; ++k)
                {
                    const float r = 1.0f + 0.46f * std::pow ((float) (k - 1), 1.18f) + 0.08f * (hash (k, 3) - 0.5f);
                    out[(size_t) n++] = { r, 0.9f / std::sqrt ((float) k), 1.2f / (1.0f + 0.12f * (float) k) };
                }
                return n;
            }
        }
    }

    void strike (Voice& v, int note, float velocity)
    {
        const bool restrike = v.active && v.note == note && v.type == params.type;
        const double f0 = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        const float vel = 0.25f + 0.75f * velocity;
        const double maxHz = juce::jmin (16000.0, sr * 0.45);
        const float crack = juce::jlimit (0.0f, 1.0f, params.crack);
        const double fc = 400.0 * std::pow (2.0, params.hardness * 5.0 + velocity * 2.0);

        std::array<Partial, maxPartials> parts {};
        const int np = partialsFor (params.type, parts);

        if (! restrike)
        {
            v.numModes = 0;
            std::fill (v.y1.begin(), v.y1.end(), 0.0f);
            std::fill (v.y2.begin(), v.y2.end(), 0.0f);
        }
        const float beat = params.beatCents * (params.type == BellType::Skalklocka ? 2.0f : 1.0f);
        const float T = juce::jlimit (0.2f, 60.0f, params.decaySec * std::pow (2.0f, -(float) (note - 60) / 24.0f))
                      * (1.0f - 0.6f * crack);

        int k = 0;
        for (int i = 0; i < np; ++i)
        {
            const auto& p = parts[(size_t) i];
            // Sprickan förskjuter deltonerna åt olika håll.
            const double shift = 1.0 + crack * 0.03 * (hash (note, i) - 0.5) * 2.0;
            const double f = f0 * p.ratio * shift;
            if (f >= maxHz || f < 20.0) continue;
            const float sig = 6.9f / (T * p.decay) * (1.0f + 0.3f * crack * (float) i);
            const float amp = p.amp * vel / (float) (1.0 + (f / fc) * (f / fc)) * 0.5f;
            for (int d = 0; d < 2; ++d) // paret: klockan är inte helt rund
            {
                const float cents = (d == 0 ? -0.5f : 0.5f) * beat * (0.5f + hash (note, 50 + i));
                const double fd = f * std::pow (2.0, cents / 1200.0);
                const auto m = (size_t) k++;
                v.omega[m] = (float) (juce::MathConstants<double>::twoPi * fd / sr);
                v.sigma[m] = sig;
                v.excite[m] = amp * std::sin (v.omega[m]); // ett nytt slag på en klocka som redan klingar läggs ovanpå
            }
        }
        v.numModes = k;
        v.type = params.type;
        v.note = note;
        v.active = true;
        v.keyDown = true;
        v.damped = false;
        v.level = 1.0f;
        v.sweepCents = params.sweep * 300.0f * (0.5f + 0.5f * velocity);
        v.pulseLen = juce::jmax (4, (int) (sr * (0.0002 + 0.0015 * (1.0f - params.hardness))));
        v.pulsePos = 0;
        v.thumpLen = juce::jmax (8, (int) (sr * 0.004));
        v.thumpLeft = v.thumpLen;
        v.thumpAmp = 0.08f * vel * (0.3f + params.hardness) * (params.type == BellType::Skalla ? 2.5f : 1.0f);
        v.thumpCoef = std::exp (-juce::MathConstants<float>::twoPi * (float) juce::jmin (fc, sr * 0.4) / (float) sr);
        updateCoefficients (v);
    }

    // Räknar om svängningarna: dämpning (klingtid, släpp) och tonhöjd (svepet).
    void updateCoefficients (Voice& v, float forcedReleaseSec = 0.0f) const
    {
        const float rel = forcedReleaseSec > 0.0f ? forcedReleaseSec : params.releaseSec;
        const float extra = v.damped ? 6.9f / juce::jmax (0.05f, rel) : 0.0f;
        const float factor = std::exp2 (-v.sweepCents / 1200.0f); // positivt svep: tonen börjar lägre och stiger
        for (int k = 0; k < v.numModes; ++k)
        {
            const float r = std::exp (-(v.sigma[(size_t) k] + extra) / (float) sr);
            const float w = juce::jmin (3.0f, v.omega[(size_t) k] * factor);
            v.a1[(size_t) k] = 2.0f * r * std::cos (w);
            v.a2[(size_t) k] = r * r;
        }
    }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        if (std::abs (v.sweepCents) > 0.05f)
        {
            v.sweepCents *= std::exp (-(float) numSamples / (float) (sr * 0.35));
            if (std::abs (v.sweepCents) <= 0.05f) v.sweepCents = 0.0f;
            updateCoefficients (v);
        }

        const int n = v.numModes;
        float* y1 = v.y1.data();
        float* y2 = v.y2.data();
        const float* a1 = v.a1.data();
        const float* a2 = v.a2.data();
        const float crack = params.crack;
        const float rattle = crack * 1.4f + (v.type == BellType::Skalla ? 0.25f : 0.0f);
        float peak = 0.0f;
        constexpr float gain = 0.5f;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            if (v.pulsePos < v.pulseLen)
            {
                const float x = (1.0f - std::cos (juce::MathConstants<float>::twoPi * ((float) v.pulsePos + 0.5f) / (float) v.pulseLen))
                              / (float) v.pulseLen;
                ++v.pulsePos;
                const float* e = v.excite.data();
                for (int k = 0; k < n; ++k)
                {
                    const float y = a1[k] * y1[k] - a2[k] * y2[k] + e[k] * x;
                    y2[k] = y1[k]; y1[k] = y; s += y;
                }
            }
            else
            {
                for (int k = 0; k < n; ++k)
                {
                    const float y = a1[k] * y1[k] - a2[k] * y2[k];
                    y2[k] = y1[k]; y1[k] = y; s += y;
                }
            }

            if (v.thumpLeft > 0)
            {
                const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) v.thumpLeft / (float) v.thumpLen);
                const float noise = random.nextFloat() * 2.0f - 1.0f;
                v.thumpLp = noise + v.thumpCoef * (v.thumpLp - noise);
                s += v.thumpLp * w * v.thumpAmp;
                --v.thumpLeft;
            }

            if (rattle > 0.001f)
            {
                // Sprickan skorrar: brus som följer klockans svängning.
                const float noise = random.nextFloat() * 2.0f - 1.0f;
                const float hp = noise - v.crackHp; v.crackHp = noise;
                v.crackLp += 0.5f * (hp - v.crackLp);
                s += v.crackLp * std::abs (s) * rattle;
            }

            s *= gain * (v.type == BellType::Skalla ? 2.2f : 1.0f); // skällan klingar kort och behöver lyftas
            dest[i] += s;
            peak = juce::jmax (peak, std::abs (s));
        }

        v.level = juce::jmax (peak, v.level * 0.5f);
        if (peak < 2.0e-5f && v.thumpLeft == 0 && v.pulsePos >= v.pulseLen)
        {
            v.active = false;
            v.numModes = 0;
        }
    }

    double sr = 44100.0;
    KlockgjuterietParams params;
    std::array<Voice, numVoices> voices;
    juce::Random random { 0x4b4c4f4b }; // "KLOK"
};

} // namespace krang
