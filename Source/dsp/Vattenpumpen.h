// Vattenpumpen – KrangBox elfte kammare: stråkad metall (fas 3b).
//
// Modalsyntes med stråke, helt syntetiskt. Metallens svängningar är en bank resonatorer med metallens
// oharmoniska deltoner, och stråken driver dem hela tiden medan tangenten hålls:
//   Stråken: en ström av "släpp" i takt med grundtonen (stick-slip, när hartset släpper taget om metallen)
//   och friktionsbrus. Mer tryck ger skarpare släpp och mer brus – från ren ton till skrik.
//   Vatten: som i en vattenfon skvalpar vattnet i behållaren, så att deltonerna glider fram och tillbaka.
//   Glid: sågen glider mellan tonerna när man spelar legato.
//   Rost: rasslande skrot som skakar med när metallen svänger.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace krang
{

enum class BowedType { Vattenfon = 0, Sag, Plat, Cymbal };

struct VattenpumpenParams
{
    BowedType type   = BowedType::Vattenfon;
    float pressure   = 0.5f;   // stråktryck 0..1
    float speed      = 0.7f;   // stråkfart (styrka) 0..1
    float water      = 0.4f;   // vattnets skvalp 0..1
    float vibrato    = 0.2f;   // 0..1
    float glideSec   = 0.0f;   // glid mellan tonerna, 0 = av (polyfont), annars legato
    float rust       = 0.0f;   // rassel 0..1
    float attackSec  = 0.3f;
    float releaseSec = 1.0f;   // stråken lyfts; metallen klingar ut av sig själv
};

class Vattenpumpen
{
public:
    static constexpr int numVoices = 8;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices) v.env.setSampleRate (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) clearVoice (v);
    }

    void setParams (const VattenpumpenParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.01f, p.attackSec), 0.2f, 0.85f, juce::jmax (0.02f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
    }

    void noteOn (int note, float velocity)
    {
        // Legato med glid: den hållna rösten glider till den nya tonen, som en såg som böjs.
        if (params.glideSec > 0.01f)
            for (auto& v : voices)
                if (v.active && v.keyDown)
                {
                    v.targetNote = (float) note;
                    v.note = note;
                    v.velocity = velocity;
                    return;
                }

        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        if (! v.active || v.type != params.type) clearVoice (v);
        v.type = params.type;
        v.note = note;
        v.currentNote = v.targetNote = (float) note;
        v.velocity = velocity;
        v.age = ++ageCounter;
        v.active = true;
        v.keyDown = true;
        v.quiet = 0;
        buildModes (v);
        v.env.noteOn();
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown) { v.env.noteOff(); v.keyDown = false; }
    }

    void allNotesOff()
    {
        for (auto& v : voices) if (v.active) { v.env.noteOff(); v.keyDown = false; }
    }

    void render (float* dest, int numSamples)
    {
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.active) renderVoice (v, dest, numSamples);
        for (int i = 0; i < numSamples; ++i)
            dest[i] = std::tanh (dest[i] * 1.3f) / 1.3f;
    }

private:
    static constexpr int maxModes = 12;

    struct Mode { float ratio, amp, decay; };

    struct Voice
    {
        juce::ADSR env;
        bool active = false, keyDown = false;
        int note = -1, numModes = 0, quiet = 0;
        float velocity = 0.8f, currentNote = 60.0f, targetNote = 60.0f;
        juce::uint32 age = 0;
        BowedType type = BowedType::Vattenfon;
        std::array<float, maxModes> ratio {}, amp {}, sigma {}, slosh {}, sloshTarget {};
        std::array<float, maxModes> x1 {}, x2 {}, y1 {}, y2 {}, a1 {}, a2 {}, b0 {};
        double slipPhase = 0.0, vibPhase = 0.0;
        int sloshTimer = 0;
        float noiseLp = 0.0f, rattleLp = 0.0f, lastEnv = 0.0f;
    };

    void clearVoice (Voice& v)
    {
        v.env.reset();
        v.active = false; v.keyDown = false; v.note = -1; v.numModes = 0;
        v.x1.fill (0.0f); v.x2.fill (0.0f); v.y1.fill (0.0f); v.y2.fill (0.0f);
        v.slosh.fill (0.0f); v.sloshTarget.fill (0.0f);
        v.noiseLp = v.rattleLp = v.lastEnv = 0.0f;
    }

    static const std::array<Mode, maxModes>& table (BowedType t, int& n)
    {
        // Förhållande till grundtonen, styrka och klingtid (sekunder).
        static const std::array<Mode, maxModes> vattenfon {{ { 1.0f, 1.0f, 6.0f }, { 1.59f, 0.6f, 5.0f }, { 2.14f, 0.5f, 4.5f }, { 2.30f, 0.45f, 4.0f },
                                                              { 2.65f, 0.4f, 3.5f }, { 2.92f, 0.35f, 3.0f }, { 3.16f, 0.3f, 2.8f }, { 3.50f, 0.3f, 2.5f },
                                                              { 4.12f, 0.2f, 2.0f }, { 4.71f, 0.15f, 1.8f }, { 5.33f, 0.12f, 1.5f }, { 6.08f, 0.1f, 1.2f } }};
        static const std::array<Mode, maxModes> sag {{ { 1.0f, 1.0f, 3.0f }, { 2.01f, 0.12f, 2.0f }, { 3.02f, 0.05f, 1.5f }, { 4.05f, 0.02f, 1.0f } }};
        static const std::array<Mode, maxModes> plat {{ { 1.0f, 1.0f, 4.0f }, { 1.69f, 0.8f, 3.5f }, { 2.71f, 0.7f, 3.0f }, { 3.47f, 0.6f, 2.5f },
                                                         { 4.31f, 0.5f, 2.2f }, { 5.49f, 0.45f, 2.0f }, { 6.65f, 0.4f, 1.8f }, { 7.18f, 0.35f, 1.6f },
                                                         { 8.47f, 0.3f, 1.4f }, { 9.91f, 0.25f, 1.2f }, { 11.4f, 0.2f, 1.0f }, { 13.2f, 0.15f, 0.8f } }};
        static const std::array<Mode, maxModes> cymbal {{ { 1.0f, 0.5f, 8.0f }, { 2.31f, 0.6f, 7.0f }, { 3.89f, 0.8f, 6.0f }, { 5.12f, 0.9f, 6.0f },
                                                           { 6.77f, 1.0f, 5.0f }, { 8.43f, 0.9f, 5.0f }, { 10.2f, 0.8f, 4.0f }, { 12.3f, 0.7f, 4.0f },
                                                           { 14.6f, 0.6f, 3.0f }, { 17.1f, 0.5f, 3.0f }, { 19.9f, 0.4f, 2.5f }, { 22.8f, 0.3f, 2.0f } }};
        switch (t)
        {
            case BowedType::Sag:    n = 4;  return sag;
            case BowedType::Plat:   n = 12; return plat;
            case BowedType::Cymbal: n = 12; return cymbal;
            case BowedType::Vattenfon:
            default:                n = 12; return vattenfon;
        }
    }

    void buildModes (Voice& v)
    {
        int n = 0;
        const auto& t = table (v.type, n);
        v.numModes = n;
        float sum = 0.0f;
        for (int k = 0; k < n; ++k) sum += t[(size_t) k].amp;
        for (int k = 0; k < n; ++k)
        {
            v.ratio[(size_t) k] = t[(size_t) k].ratio;
            v.amp[(size_t) k] = t[(size_t) k].amp / sum;
            v.sigma[(size_t) k] = 6.9f / t[(size_t) k].decay;
        }
        updateCoefficients (v, 0.0f);
    }

    // Räknar om resonatorerna: tonhöjd (glid, vibrato, vatten) en gång per block.
    void updateCoefficients (Voice& v, float vibCents)
    {
        const double baseHz = 440.0 * std::pow (2.0, (v.currentNote - 69.0f + vibCents / 100.0f) / 12.0);
        for (int k = 0; k < v.numModes; ++k)
        {
            const double f = baseHz * v.ratio[(size_t) k] * std::exp2 (v.slosh[(size_t) k] / 1200.0f);
            const auto m = (size_t) k;
            if (f >= sr * 0.45) { v.a1[m] = 0.0f; v.a2[m] = 0.0f; v.b0[m] = 0.0f; continue; }
            const double r = std::exp (-v.sigma[m] / sr);
            v.a1[m] = (float) (-2.0 * r * std::cos (juce::MathConstants<double>::twoPi * f / sr));
            v.a2[m] = (float) (r * r);
            v.b0[m] = (float) ((1.0 - r * r) * 0.5); // normerat bandpass: toppförstärkning 1
        }
    }

    float noise() { seed = seed * 1664525u + 1013904223u; return (float) (int) seed * (1.0f / 2147483648.0f); }
    float uni() { return 0.5f + 0.5f * noise(); }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const float pressure = juce::jlimit (0.0f, 1.0f, params.pressure);

        // Glid, vibrato och vatten ändras långsamt: räkna om resonatorerna en gång per block.
        if (params.glideSec > 0.01f)
        {
            const float k = 1.0f - std::exp (-(float) numSamples / (float) (sr * params.glideSec * 0.3f));
            v.currentNote += (v.targetNote - v.currentNote) * k;
        }
        else
            v.currentNote = v.targetNote;
        v.vibPhase += (double) numSamples * (v.type == BowedType::Sag ? 5.5 : 4.5) / sr;
        v.vibPhase -= std::floor (v.vibPhase);
        const float vibDepth = params.vibrato * (v.type == BowedType::Sag ? 40.0f : 15.0f);
        const float vibCents = vibDepth * (float) std::sin (twoPi * v.vibPhase) * juce::jmin (1.0f, v.lastEnv * 2.0f);
        v.sloshTimer -= numSamples;
        if (v.sloshTimer <= 0)
        {
            for (int k = 0; k < v.numModes; ++k) v.sloshTarget[(size_t) k] = noise() * params.water * 60.0f;
            v.sloshTimer = (int) (sr * (0.25 + 0.8 * uni()));
        }
        for (int k = 0; k < v.numModes; ++k)
            v.slosh[(size_t) k] += (v.sloshTarget[(size_t) k] - v.slosh[(size_t) k]) * (float) (numSamples / (sr * 0.35)); // vattnet skvalpar mjukt
        updateCoefficients (v, vibCents);

        const double f0 = 440.0 * std::pow (2.0, (v.currentNote - 69.0f + vibCents / 100.0f) / 12.0);
        const float slipSharp = 1.0f + 12.0f * pressure;                        // högre tryck: kortare, skarpare släpp
        const float noiseAmt = 0.15f + 0.6f * pressure * pressure;
        const float drive = (0.4f + 0.6f * params.speed) * (0.5f + 0.5f * v.velocity) * 3.0f;
        const float rust = params.rust + (v.type == BowedType::Plat ? 0.15f : 0.0f);
        // Nivå per metall: sågen är nästan en ren ton, cymbalen sprider energin över många höga deltoner.
        static constexpr float gains[4] = { 6.0f, 3.0f, 6.0f, 22.0f };
        const float typeGain = gains[juce::jlimit (0, 3, (int) v.type)];
        float peak = 0.0f;
        const int n = v.numModes;

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = v.env.getNextSample();
            v.lastEnv = env;
            // Stråken: släppen i takt med grundtonen, och friktionsbrus.
            v.slipPhase += f0 / sr; v.slipPhase -= std::floor (v.slipPhase);
            const float s = (float) std::sin (twoPi * v.slipPhase);
            const float slip = (s > 0.0f ? std::pow (s, slipSharp) : 0.0f) - 1.0f / (1.0f + slipSharp);
            v.noiseLp += 0.3f * (noise() - v.noiseLp);
            const float bow = (slip + v.noiseLp * noiseAmt) * env * drive;

            float out = 0.0f;
            for (int k = 0; k < n; ++k)
            {
                const auto m = (size_t) k;
                const float x = bow * v.amp[m];
                const float y = v.b0[m] * (x - v.x2[m]) - v.a1[m] * v.y1[m] - v.a2[m] * v.y2[m];
                v.x2[m] = v.x1[m]; v.x1[m] = x;
                v.y2[m] = v.y1[m]; v.y1[m] = y;
                out += y;
            }
            if (rust > 0.001f)
            {
                v.rattleLp += 0.5f * (noise() - v.rattleLp);
                out += v.rattleLp * std::abs (out) * rust * 2.0f;
            }
            out *= typeGain;
            dest[i] += out;
            peak = juce::jmax (peak, std::abs (out));
        }

        if (! std::isfinite (peak)) { clearVoice (v); return; }
        if (! v.env.isActive() && peak < 2.0e-5f && ++v.quiet > 4) clearVoice (v);
    }

    double sr = 44100.0;
    VattenpumpenParams params;
    std::array<Voice, numVoices> voices;
    juce::uint32 ageCounter = 0, seed = 0x56415454; // "VATT"
};

} // namespace krang
