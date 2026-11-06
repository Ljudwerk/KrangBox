// Hammarverket – KrangBox trummaskin (fas 1, utbyggd i fas 3c).
// Åtta spår, och sexton helt syntetiska trumljud att välja på per spår: smedja och ångmaskin möter
// ramtrumma, träskedar och bjällror. Fyra färdiga kit (Smedjan, Fabriken, Folkdansen, Ruinen) väljer
// ljud för alla åtta spåren på en gång.
//
// Parameterlås: CC 102 före en ton stämmer just det slaget (värde 64 = ingen ändring, ±12 halvtoner),
// CC 103 ändrar klangens längd (1–7 = ×0,25 … ×4, 0 = ingen ändring). Stegverket skickar dem för
// steg med lås, och de går lika gärna att skicka från DAW:en.
//
// MIDI-mappning per spår (General MIDI där det går):
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

// Spårens namn i samma ordning som Drum (= ljuden i kitet Smedjan). UTF-8.
inline const char* drumName (int i)
{
    static const char* names[] = { "Ångslaget", "Skrotplåten", "Smedjan", "Klappern",
                                   "Ramtrumman", "Ventilen", "Ångröret", "Träskedarna" };
    return names[juce::jlimit (0, numDrums - 1, i)];
}

// Ljuden. De åtta första är de ursprungliga; varje spår kan spela vilket som helst.
enum class DrumSound { Angslaget = 0, Skrotplaten, Smedjan, Klappern, Ramtrumman, Ventilen, Angroret, Traskedarna,
                       Stadet, Tunnan, Kedjan, Pistongen, Angslappet, Fjadern, Bjallran, Hjartslaget, count };
inline constexpr int numDrumSounds = (int) DrumSound::count;

inline const char* drumSoundName (int i)
{
    static const char* names[] = { "Ångslaget", "Skrotplåten", "Smedjan", "Klappern", "Ramtrumman", "Ventilen", "Ångröret",
                                   "Träskedarna", "Städet", "Tunnan", "Kedjan", "Pistongen", "Ångsläppet", "Fjädern",
                                   "Bjällran", "Hjärtslaget" };
    return names[juce::jlimit (0, numDrumSounds - 1, i)];
}

// Färdiga kit: ljud för spår 0–7.
inline constexpr int numDrumKits = 4;
inline const char* drumKitName (int k)
{
    static const char* names[] = { "Smedjan", "Fabriken", "Folkdansen", "Ruinen" };
    return names[juce::jlimit (0, numDrumKits - 1, k)];
}
inline const std::array<int, 8>& drumKit (int k)
{
    static const std::array<std::array<int, 8>, numDrumKits> kits { {
        { 0, 1, 2, 3, 4, 5, 6, 7 },        // Smedjan
        { 11, 8, 2, 10, 9, 5, 12, 13 },    // Fabriken: piston, städ, smedja, kedja, tunna, ventil, ångsläpp, fjäder
        { 15, 14, 4, 3, 9, 7, 6, 10 },     // Folkdansen: hjärtslag, bjällra, ramtrumma, klapp, tunna, skedar, ångrör, kedja
        { 11, 13, 12, 8, 9, 10, 6, 15 },   // Ruinen: piston, fjäder, ångsläpp, städ, tunna, kedja, ångrör, hjärtslag
    } };
    return kits[(size_t) juce::jlimit (0, numDrumKits - 1, k)];
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
    int sound   = -1;    // ljud (DrumSound); -1 = spårets eget (samma nummer som spåret)
};

struct HammarverketParams
{
    std::array<DrumParams, (size_t) numDrums> drums {};
    float rost = 0.3f;   // 0..1: rost och skrap – brus och ojämnheter i alla röster
};

// Parameterlåsens CC-nummer och klanglåsets multiplikatorer.
inline constexpr int lockTuneCC = 102, lockDecayCC = 103;
inline constexpr std::array<float, 8> lockDecayValues { 1.0f, 0.25f, 0.4f, 0.6f, 1.0f, 1.6f, 2.5f, 4.0f };

class Hammarverket
{
public:
    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto& v : voices)
            for (auto* f : { &v.f1, &v.f2 })
            {
                f->prepare (spec);
                f->reset();
            }
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.t = 0.0;
            v.f1.reset();
            v.f2.reset();
        }
        pendingTune = 0.0f;
        pendingDecay = 1.0f;
    }

    void setParams (const HammarverketParams& p) { params = p; }

    // Parameterlås för nästa slag (se överst i filen).
    void controller (int cc, int value)
    {
        if (cc == lockTuneCC)       pendingTune = (float) juce::jlimit (-12, 12, value - 64);
        else if (cc == lockDecayCC) pendingDecay = lockDecayValues[(size_t) juce::jlimit (0, 7, value)];
    }

    void noteOn (int note, float velocity)
    {
        const int d = drumForNote (note);
        if (d < 0) return;

        const auto& dp = params.drums[(size_t) d];
        auto& v = voices[(size_t) d];
        const int sound = juce::jlimit (0, numDrumSounds - 1, dp.sound < 0 ? d : dp.sound);
        if (sound != v.sound || ! v.active)
            setupFilters ((DrumSound) sound, v);
        v.sound = sound;
        v.active = true;
        v.t = 0.0;
        for (auto& ph : v.ph) ph = 0.0;
        v.velocity = 0.35f + 0.65f * velocity;
        v.pitch = std::pow (2.0f, (dp.tune + pendingTune) / 12.0f);
        v.decayMul = juce::jlimit (0.1f, 8.0f, dp.decay * pendingDecay);
        v.jitter = random.nextFloat();
        pendingTune = 0.0f;
        pendingDecay = 1.0f;

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
                out += renderVoice ((DrumSound) v.sound, v) * v.velocity * params.drums[(size_t) d].level;
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
    using SVF = juce::dsp::StateVariableTPTFilter<float>;
    using SVFType = juce::dsp::StateVariableTPTFilterType;

    struct Voice
    {
        bool active = false;
        int sound = -1;
        float velocity = 1.0f, pitch = 1.0f, decayMul = 1.0f, jitter = 0.0f;
        double t = 0.0;                 // sekunder sedan anslaget
        double ph[6] {};                // oscillatorfaser
        SVF f1, f2;                     // varje röst har egna filter, ställda efter ljudet
    };

    static void setFilter (SVF& f, SVFType type, float hz, float res)
    {
        f.setType (type);
        f.setCutoffFrequency (hz);
        f.setResonance (res);
    }

    // Filtren för varje ljud (de åtta första exakt som i den ursprungliga trummaskinen).
    void setupFilters (DrumSound s, Voice& v)
    {
        const float nyq = (float) sr * 0.45f;
        auto lim = [nyq] (float hz) { return juce::jmin (hz, nyq); };
        switch (s)
        {
            case DrumSound::Skrotplaten: setFilter (v.f1, SVFType::bandpass, 2600.0f, 2.5f); break;
            case DrumSound::Smedjan:     setFilter (v.f1, SVFType::bandpass, 3200.0f, 0.8f); break;
            case DrumSound::Klappern:    setFilter (v.f1, SVFType::bandpass, 1150.0f, 1.6f); break;
            case DrumSound::Ramtrumman:  setFilter (v.f1, SVFType::lowpass, 900.0f, 0.7f); break;
            case DrumSound::Ventilen:    setFilter (v.f1, SVFType::highpass, lim (7000.0f), 0.9f); break;
            case DrumSound::Angroret:    setFilter (v.f1, SVFType::highpass, lim (6000.0f), 0.9f); break;
            case DrumSound::Traskedarna: setFilter (v.f1, SVFType::bandpass, 2300.0f, 6.0f); break;
            case DrumSound::Tunnan:      setFilter (v.f1, SVFType::lowpass, 500.0f, 0.9f); break;
            case DrumSound::Kedjan:      setFilter (v.f1, SVFType::bandpass, lim (4200.0f), 3.0f);
                                         setFilter (v.f2, SVFType::bandpass, lim (7400.0f), 4.0f); break;
            case DrumSound::Pistongen:   setFilter (v.f1, SVFType::bandpass, 900.0f, 1.0f); break;
            case DrumSound::Angslappet:  setFilter (v.f1, SVFType::bandpass, 2500.0f, 1.4f);
                                         setFilter (v.f2, SVFType::highpass, lim (5000.0f), 0.7f); break;
            case DrumSound::Bjallran:    setFilter (v.f1, SVFType::bandpass, lim (4800.0f), 6.0f);
                                         setFilter (v.f2, SVFType::bandpass, lim (6900.0f), 7.0f); break;
            case DrumSound::Hjartslaget: setFilter (v.f1, SVFType::lowpass, 300.0f, 0.7f); break;
            case DrumSound::Angslaget: case DrumSound::Stadet: case DrumSound::Fjadern: case DrumSound::count: break;
        }
    }

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

    float renderVoice (DrumSound d, Voice& v)
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
            case DrumSound::Angslaget:
            {
                // Bastrumma: sinus med snabbt tonhöjdsfall + klick + tanh-mättning (ångslag).
                const double f = (48.0 + 140.0 * std::exp (-t / 0.035)) * p;
                const float body = sine (v.ph[0], f) * expEnv (t, 0.26 * dm);
                const float click = noise() * expEnv (t, 0.003) * 0.6f;
                y = std::tanh (1.8f * (body + click));
                end = 2.0 * dm;
                break;
            }
            case DrumSound::Skrotplaten:
            {
                // Rostig plåt: två oharmoniska sinusar ringmodulerade + skrapbrus.
                const float a = sine (v.ph[0], 421.0 * p);
                const float b = sine (v.ph[1], 1137.0 * p);
                const float c = sine (v.ph[2], 1713.0 * p);
                const float ring = a * b * 0.8f + c * 0.3f;
                const float scrape = v.f1.processSample (0, noise()) * (0.4f + rost);
                y = (ring * expEnv (t, 0.22 * dm) + scrape * expEnv (t, 0.06 * dm)) * 0.8f;
                end = 1.5 * dm;
                break;
            }
            case DrumSound::Smedjan:
            {
                // Virvel: två toner (skinn) + bandpassat brus (sejare), med hammarklang.
                const float tone = (sine (v.ph[0], 185.0 * p) * 0.6f + sine (v.ph[1], 330.0 * p) * 0.4f) * expEnv (t, 0.07 * dm);
                const float wires = v.f1.processSample (0, noise()) * expEnv (t, 0.12 * dm) * 1.6f;
                const float anvil = sine (v.ph[2], 2730.0 * p) * expEnv (t, 0.09 * dm) * 0.12f * (0.5f + rost);
                y = tone + wires + anvil;
                end = 1.0 * dm;
                break;
            }
            case DrumSound::Klappern:
            {
                // Handklapp: fyra brusskurar med 11 ms mellanrum, sedan en svans.
                float env = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    const double tk = t - k * 0.011;
                    if (tk >= 0.0) env = juce::jmax (env, expEnv (tk, 0.006));
                }
                env = juce::jmax (env, (t > 0.033 ? 0.7f * expEnv (t - 0.033, 0.12 * dm) : 0.0f));
                y = v.f1.processSample (0, noise()) * env * 2.2f;
                end = 1.0 * dm;
                break;
            }
            case DrumSound::Ramtrumman:
            {
                // Ramtrumma: membranets grundton + Bessel-överton (1,59x) + mjukt skinnbrus.
                const double f = (92.0 + 30.0 * std::exp (-t / 0.02)) * p;
                const float fund = sine (v.ph[0], f) * expEnv (t, 0.45 * dm);
                const float mode2 = sine (v.ph[1], f * 1.593) * expEnv (t, 0.18 * dm) * 0.35f;
                const float skin = v.f1.processSample (0, noise()) * expEnv (t, 0.05 * dm) * (0.5f + rost);
                y = fund + mode2 + skin;
                end = 2.5 * dm;
                break;
            }
            case DrumSound::Ventilen:
            case DrumSound::Angroret:
            {
                // Hihat: sex fyrkantvågor i metalliska förhållanden, högpassade – en ventil som pyser.
                static constexpr double freqs[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
                float metal = 0.0f;
                for (int k = 0; k < 6; ++k)
                    metal += square (v.ph[k], freqs[k] * 1.7 * p);
                metal = metal / 6.0f + noise() * 0.3f * rost;
                const bool open = d == DrumSound::Angroret;
                y = v.f1.processSample (0, metal) * expEnv (t, (open ? 0.42 : 0.045) * dm) * 1.4f;
                end = (open ? 3.0 : 0.6) * dm;
                break;
            }
            case DrumSound::Traskedarna:
            {
                // Träskedar: två trästycken som slår ihop – två resonanta klick 18 ms isär.
                float exc = 0.0f;
                if (t < 0.0015) exc = noise();
                else if (t > 0.018 && t < 0.0195) exc = noise() * 0.7f;
                y = v.f1.processSample (0, exc) * 1.1f;
                end = 0.35 * dm;
                break;
            }
            case DrumSound::Stadet:
            {
                // Städet: ett hammarslag på ett städ – fem oharmoniska stålmoder med ett hårt klick.
                static constexpr double ratio[5] = { 1.0, 2.32, 4.25, 6.63, 9.38 };
                static constexpr float amp[5] = { 1.0f, 0.6f, 0.45f, 0.3f, 0.2f };
                static constexpr double tau[5] = { 1.1, 0.7, 0.45, 0.28, 0.18 };
                const double f0 = 1180.0 * p * (1.0 + 0.004 * (v.jitter - 0.5));
                for (int k = 0; k < 5; ++k)
                    if (f0 * ratio[k] < sr * 0.45)
                        y += sine (v.ph[k], f0 * ratio[k]) * amp[k] * expEnv (t, tau[k] * dm);
                y = y * 0.42f + noise() * expEnv (t, 0.0015) * 0.8f + noise() * 0.04f * rost * expEnv (t, 0.2);
                end = 3.0 * dm;
                break;
            }
            case DrumSound::Tunnan:
            {
                // Oljefatet: djup tom med plåtens oharmoniska moder och ett dovt dunk.
                const double f = (105.0 + 40.0 * std::exp (-t / 0.015)) * p;
                const float body = sine (v.ph[0], f) * expEnv (t, 0.5 * dm)
                                 + sine (v.ph[1], f * 1.48) * expEnv (t, 0.3 * dm) * 0.5f
                                 + sine (v.ph[2], f * 2.31) * expEnv (t, 0.2 * dm) * 0.3f;
                const float tin = sine (v.ph[3], f * 3.1) * sine (v.ph[4], f * 5.37) * expEnv (t, 0.6 * dm) * (0.15f + 0.3f * rost);
                const float thud = v.f1.processSample (0, noise()) * expEnv (t, 0.03) * 1.2f;
                y = std::tanh (1.4f * (body + tin + thud));
                end = 2.5 * dm;
                break;
            }
            case DrumSound::Kedjan:
            {
                // Kedjan: en kedja som släpps på plåt – glesnande klickskurar genom två metallfilter.
                const float density = 0.02f * expEnv (t, 0.09 * dm);
                const float exc = random.nextFloat() < density ? noise() * 2.2f : 0.0f;
                y = (v.f1.processSample (0, exc) + v.f2.processSample (0, exc) * 0.7f) * (0.6f + 0.4f * rost);
                end = 0.8 * dm;
                break;
            }
            case DrumSound::Pistongen:
            {
                // Pistongen: hård bastrumma med fyrkantig kropp, och ångans tjoff 30 ms efter slaget.
                const double f = (42.0 + 90.0 * std::exp (-t / 0.05)) * p;
                const float body = (sine (v.ph[0], f) * 0.7f + square (v.ph[1], f) * 0.3f) * expEnv (t, 0.35 * dm);
                float chuff = 0.0f;
                if (t > 0.03)
                {
                    const double tc = t - 0.03;
                    chuff = v.f1.processSample (0, noise()) * (float) (1.0 - std::exp (-tc / 0.004)) * expEnv (tc, 0.06 * dm) * (0.8f + rost);
                }
                y = std::tanh (2.5f * body + chuff);
                end = 1.5 * dm;
                break;
            }
            case DrumSound::Angslappet:
            {
                // Ångsläppet: en ventil som öppnas – brus som sveper uppåt och sväller innan det tystnar.
                if ((int) (t * sr) % 32 == 0)
                    v.f1.setCutoffFrequency (juce::jmin ((float) sr * 0.45f, (float) ((2500.0 + 4500.0 * (1.0 - std::exp (-t / 0.08))) * p)));
                const float n = noise();
                const float env = (float) (1.0 - std::exp (-t / 0.015)) * expEnv (t, 0.18 * dm);
                y = (v.f1.processSample (0, n) * 1.6f + v.f2.processSample (0, n) * 0.5f) * env;
                end = 1.2 * dm;
                break;
            }
            case DrumSound::Fjadern:
            {
                // Fjädern: en spänd stålfjäder som släpps – svepande ton som dallrar.
                const double wobble = 1.0 + 0.08 * std::sin (juce::MathConstants<double>::twoPi * 28.0 * t) * std::exp (-t / 0.2);
                const double f = (80.0 + 420.0 * std::exp (-t / 0.04)) * p * wobble;
                y = std::tanh (1.5f * (sine (v.ph[0], f) * expEnv (t, 0.5 * dm)
                                     + sine (v.ph[1], f * 2.7) * expEnv (t, 0.15 * dm) * 0.3f)) * 0.9f;
                end = 2.0 * dm;
                break;
            }
            case DrumSound::Bjallran:
            {
                // Bjällran: en bjällerkrans som skakas – fyra skurar av små klingande metallkulor.
                static constexpr double hits[4] = { 0.0, 0.025, 0.05, 0.082 };
                float exc = 0.0f;
                for (int k = 0; k < 4; ++k)
                    if (t >= hits[k] && t < hits[k] + 0.002) exc += noise() * (k == 0 ? 1.0f : 0.6f);
                y = (v.f1.processSample (0, exc) + v.f2.processSample (0, exc) * 0.8f) * 0.6f
                    * (t < 0.1 ? 1.0f : expEnv (t - 0.1, 0.15 * dm));
                end = 0.9 * dm;
                break;
            }
            case DrumSound::Hjartslaget:
            {
                // Hjärtslaget: två mjuka, låga dunk – lubb-dubb – som en stor folkmusiktrumma med filtklubba.
                auto thump = [&] (double tt, double& ph)
                {
                    if (tt < 0.0) return 0.0f;
                    const double f = (52.0 + 30.0 * std::exp (-tt / 0.03)) * p;
                    return sine (ph, f) * (float) (1.0 - std::exp (-tt / 0.004)) * expEnv (tt, 0.22 * dm);
                };
                y = thump (t, v.ph[0]) + 0.7f * thump (t - 0.16, v.ph[1]) + v.f1.processSample (0, noise()) * expEnv (t, 0.02) * 0.5f;
                y = std::tanh (1.6f * y);
                end = 1.6 * dm;
                break;
            }
            case DrumSound::count: break;
        }

        if (t > end)
            v.active = false;
        return y;
    }

    double sr = 44100.0;
    HammarverketParams params;
    std::array<Voice, (size_t) numDrums> voices {};
    float pendingTune = 0.0f, pendingDecay = 1.0f;
    juce::Random random { 0x48414d4d }; // "HAMM"
};

} // namespace krang
