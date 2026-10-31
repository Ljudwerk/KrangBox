// Hammarverket – KrangBox trummaskin (fas 1).
// Åtta helt syntetiska trumröster: smedja och ångmaskin möter ramtrumma och träskedar.
//
// MIDI-mappning (General MIDI där det går):
//   36 Ångslaget (bastrumma)   37 Skrotplåten (rostig plåt)   38 Smedjan (virvel)
//   39 Klappern (handklapp)    41 Ramtrumman (folkrumma)     42 Ventilen (stängd hihat)
//   46 Ångröret (öppen hihat)  75 Träskedarna (folkligt skedspel)
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace krang
{

enum class Drum { Angslaget = 0, Skrotplaten, Smedjan, Klappern, Ramtrumman, Ventilen, Angroret, Traskedarna, count };
inline constexpr int numDrums = (int) Drum::count;

// Namn i samma ordning som Drum. UTF-8.
inline const char* drumName (int i)
{
    static const char* names[] = { "Ångslaget", "Skrotplåten", "Smedjan", "Klappern",
                                   "Ramtrumman", "Ventilen", "Ångröret", "Träskedarna" };
    return names[juce::jlimit (0, numDrums - 1, i)];
}

// MIDI-not -> röst, -1 om noten inte används.
inline int drumForNote (int note)
{
    switch (note)
    {
        case 35: case 36: return (int) Drum::Angslaget;
        case 37:          return (int) Drum::Skrotplaten;
        case 38: case 40: return (int) Drum::Smedjan;
        case 39:          return (int) Drum::Klappern;
        case 41: case 43: case 45: return (int) Drum::Ramtrumman;
        case 42: case 44: return (int) Drum::Ventilen;
        case 46:          return (int) Drum::Angroret;
        case 75: case 76: return (int) Drum::Traskedarna;
        default:          return -1;
    }
}

struct DrumParams
{
    float tune  = 0.0f;  // halvtoner, -12..+12
    float decay = 1.0f;  // multiplikator, 0.25..4
    float level = 0.8f;  // 0..1
};

struct HammarverketParams
{
    std::array<DrumParams, (size_t) numDrums> drums {};
    float rost = 0.3f;   // 0..1: rost och skrap – brus och ojämnheter i alla röster
};

class Hammarverket
{
public:
    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto* f : { &snareNoise, &clapFilter, &hatFilter, &openHatFilter, &skinFilter, &spoonFilter, &scrapNoise })
        {
            f->prepare (spec);
            f->reset();
        }
        snareNoise.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        snareNoise.setCutoffFrequency (3200.0f);   snareNoise.setResonance (0.8f);
        clapFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        clapFilter.setCutoffFrequency (1150.0f);   clapFilter.setResonance (1.6f);
        hatFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        hatFilter.setCutoffFrequency (7000.0f);    hatFilter.setResonance (0.9f);
        openHatFilter.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        openHatFilter.setCutoffFrequency (6000.0f); openHatFilter.setResonance (0.9f);
        skinFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        skinFilter.setCutoffFrequency (900.0f);    skinFilter.setResonance (0.7f);
        spoonFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        spoonFilter.setCutoffFrequency (2300.0f);  spoonFilter.setResonance (6.0f);
        scrapNoise.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        scrapNoise.setCutoffFrequency (2600.0f);   scrapNoise.setResonance (2.5f);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v = {};
    }

    void setParams (const HammarverketParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        const int d = drumForNote (note);
        if (d < 0) return;

        auto& v = voices[(size_t) d];
        v = {};
        v.active = true;
        v.velocity = 0.35f + 0.65f * velocity;
        v.pitch = std::pow (2.0f, params.drums[(size_t) d].tune / 12.0f);
        v.decayMul = juce::jlimit (0.25f, 4.0f, params.drums[(size_t) d].decay);

        // Stängd hihat stryper den öppna, som på en riktig trummaskin.
        if (d == (int) Drum::Ventilen)
            voices[(size_t) Drum::Angroret].active = false;
    }

    // Trummor ignorerar note off: varje slag klingar ut av sig självt.
    void noteOff (int) {}
    void allNotesOff() { reset(); }

    void render (float* dest, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (int d = 0; d < numDrums; ++d)
            {
                auto& v = voices[(size_t) d];
                if (! v.active) continue;
                out += renderVoice ((Drum) d, v) * v.velocity * params.drums[(size_t) d].level;
            }
            dest[i] = out * 0.5f;
        }
    }

    bool isActive() const
    {
        for (auto& v : voices) if (v.active) return true;
        return false;
    }

private:
    struct Voice
    {
        bool active = false;
        float velocity = 1.0f, pitch = 1.0f, decayMul = 1.0f;
        double t = 0.0;                 // sekunder sedan anslaget
        double ph[6] {};                // oscillatorfaser
    };

    float noise() { return random.nextFloat() * 2.0f - 1.0f; }

    static float expEnv (double t, double tau) { return (float) std::exp (-t / tau); }

    // Fasackumulator; returnerar sinus.
    float sine (double& ph, double hz)
    {
        ph += hz / sr;
        ph -= std::floor (ph);
        return (float) std::sin (juce::MathConstants<double>::twoPi * ph);
    }

    float square (double& ph, double hz)
    {
        ph += hz / sr;
        ph -= std::floor (ph);
        return ph < 0.5 ? 1.0f : -1.0f;
    }

    float renderVoice (Drum d, Voice& v)
    {
        const double t = v.t;
        v.t += 1.0 / sr;
        const double dm = v.decayMul;
        const float p = v.pitch;
        const float rost = params.rost;
        float y = 0.0f;
        double end = 1.0; // röst stängs av efter så här många sekunder

        switch (d)
        {
            case Drum::Angslaget:
            {
                // Bastrumma: sinus med snabbt tonhöjdsfall + klick + tanh-mättning (ångslag).
                const double f = (48.0 + 140.0 * std::exp (-t / 0.035)) * p;
                const float body = sine (v.ph[0], f) * expEnv (t, 0.26 * dm);
                const float click = noise() * expEnv (t, 0.003) * 0.6f;
                y = std::tanh (1.8f * (body + click));
                end = 2.0 * dm;
                break;
            }
            case Drum::Skrotplaten:
            {
                // Rostig plåt: två oharmoniska sinusar ringmodulerade + skrapbrus.
                const float a = sine (v.ph[0], 421.0 * p);
                const float b = sine (v.ph[1], 1137.0 * p);
                const float c = sine (v.ph[2], 1713.0 * p);
                const float ring = a * b * 0.8f + c * 0.3f;
                const float scrape = scrapNoise.processSample (0, noise()) * (0.4f + rost);
                y = (ring * expEnv (t, 0.22 * dm) + scrape * expEnv (t, 0.06 * dm)) * 0.8f;
                end = 1.5 * dm;
                break;
            }
            case Drum::Smedjan:
            {
                // Virvel: två toner (skinn) + bandpassat brus (sejare), med hammarklang.
                const float tone = (sine (v.ph[0], 185.0 * p) * 0.6f + sine (v.ph[1], 330.0 * p) * 0.4f) * expEnv (t, 0.07 * dm);
                const float wires = snareNoise.processSample (0, noise()) * expEnv (t, 0.12 * dm) * 1.6f;
                const float anvil = sine (v.ph[2], 2730.0 * p) * expEnv (t, 0.09 * dm) * 0.12f * (0.5f + rost);
                y = tone + wires + anvil;
                end = 1.0 * dm;
                break;
            }
            case Drum::Klappern:
            {
                // Handklapp: fyra brusskurar med 11 ms mellanrum, sedan en svans.
                float env = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    const double tk = t - k * 0.011;
                    if (tk >= 0.0) env = juce::jmax (env, expEnv (tk, 0.006));
                }
                env = juce::jmax (env, (t > 0.033 ? 0.7f * expEnv (t - 0.033, 0.12 * dm) : 0.0f));
                y = clapFilter.processSample (0, noise()) * env * 2.2f;
                end = 1.0 * dm;
                break;
            }
            case Drum::Ramtrumman:
            {
                // Ramtrumma: membranets grundton + Bessel-överton (1,59x) + mjukt skinnbrus.
                const double f = (92.0 + 30.0 * std::exp (-t / 0.02)) * p;
                const float fund = sine (v.ph[0], f) * expEnv (t, 0.45 * dm);
                const float mode2 = sine (v.ph[1], f * 1.593) * expEnv (t, 0.18 * dm) * 0.35f;
                const float skin = skinFilter.processSample (0, noise()) * expEnv (t, 0.05 * dm) * (0.5f + rost);
                y = fund + mode2 + skin;
                end = 2.5 * dm;
                break;
            }
            case Drum::Ventilen:
            case Drum::Angroret:
            {
                // Hihat: sex fyrkantvågor i metalliska förhållanden, högpassade – en ventil som pyser.
                static constexpr double freqs[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
                float metal = 0.0f;
                for (int k = 0; k < 6; ++k)
                    metal += square (v.ph[k], freqs[k] * 1.7 * p);
                metal = metal / 6.0f + noise() * 0.3f * rost;
                const bool open = d == Drum::Angroret;
                auto& f = open ? openHatFilter : hatFilter;
                y = f.processSample (0, metal) * expEnv (t, (open ? 0.42 : 0.045) * dm) * 1.4f;
                end = (open ? 3.0 : 0.6) * dm;
                break;
            }
            case Drum::Traskedarna:
            {
                // Träskedar: två trästycken som slår ihop – två resonanta klick 18 ms isär.
                float exc = 0.0f;
                if (t < 0.0015) exc = noise();
                else if (t > 0.018 && t < 0.0195) exc = noise() * 0.7f;
                y = spoonFilter.processSample (0, exc) * 1.1f;
                end = 0.35 * dm;
                break;
            }
            case Drum::count: break;
        }

        if (t > end)
            v.active = false;
        return y;
    }

    double sr = 44100.0;
    HammarverketParams params;
    std::array<Voice, (size_t) numDrums> voices {};
    juce::Random random { 0x48414d4d }; // "HAMM"

    // Ett filter per röst som behöver ett; delas inte mellan röster.
    juce::dsp::StateVariableTPTFilter<float> snareNoise, clapFilter, hatFilter, openHatFilter,
                                             skinFilter, spoonFilter, scrapNoise;
};

} // namespace krang
