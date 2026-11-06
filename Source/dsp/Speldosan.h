// Speldosan – KrangBox trettonde kammare: en speldosa vars mekanik är på väg att ge upp (fas 3c).
//
// Modalsyntes, helt syntetiskt. Varje ton är en stålstav i en kam: en fast inspänd stav svänger med
// oharmoniska deltoner (1 : 6,27 : 17,55 : 34,4), och en stift på valsen knäpper den med en kort puls.
// Om staven redan svänger när stiftet kommer surrar den mot stiftet en kort stund – speldosans "zzt".
//   Speldosa     – en vanlig liten kam: ljus och klar
//   Kyrkspeldosa – en stor kam med klockklang: djupare, längre, med en klockdelton
//   Glasdosa     – stavar av glas: mjukare, mer harmoniska deltoner
// Mekaniken:
//   Låda      – trälådans resonans som förstärker kammen
//   Fjäder    – fjädern som håller på att ta slut: tonen sjunker och vacklar
//   Rost      – rostiga och lagade stavar: ostämda, surrande, kortare
//   Verk      – urverket som tickar och regulatorns vinge som surrar
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Stamning.h"
#include <array>
#include <cmath>

namespace krang
{

enum class SpeldosaType { Speldosa = 0, Kyrkspeldosa, Glasdosa };

struct SpeldosanParams
{
    SpeldosaType type = SpeldosaType::Speldosa;
    float box     = 0.5f;    // lådans resonans 0..1
    float spring  = 0.2f;    // fjädern som tar slut 0..1
    float rust    = 0.1f;    // rost 0..1
    float works   = 0.2f;    // urverkets ljud 0..1
    float decay   = 1.0f;    // klingtid, multiplikator 0.25..4
    float hardness = 0.5f;   // stiftets hårdhet 0..1
};

class Speldosan
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    static constexpr int numVoices = 24, numModes = 4;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto* f : { &body1, &body2, &whirr })
        {
            f->prepare (spec);
            f->reset();
        }
        body1.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        body1.setCutoffFrequency (320.0f);  body1.setResonance (2.5f);
        body2.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        body2.setCutoffFrequency (1250.0f); body2.setResonance (2.0f);
        whirr.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        whirr.setCutoffFrequency (900.0f);  whirr.setResonance (1.2f);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v = {};
        body1.reset(); body2.reset(); whirr.reset();
        springCents = 0.0f;
        springTarget = 0.0f;
        wowPhase = tickPhase = 0.0;
        tickEnv = 0.0f;
        held = 0;
    }

    void setParams (const SpeldosanParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        ++held;
        // Samma stav knäpps igen: den som redan svänger används (och surrar mot stiftet).
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        const bool ringing = v.active && v.note == note && v.level > 0.02f;
        if (! (v.active && v.note == note))
            for (auto& m : v.modes) m = {};
        v.active = true;
        v.note = note;
        v.age = ++ageCounter;
        v.velocity = 0.3f + 0.7f * velocity;
        v.t = 0.0;
        v.buzz = ringing ? 0.05f : 0.0f;   // surret när stiftet möter en stav som redan svänger

        // Rostiga stavar: samma stav är lika ostämd och dämpad varje gång.
        const auto h = (uint32_t) (note * 2654435761u) ^ 0x5eedu;
        const float r1 = (float) ((h >> 4) & 1023u) / 1023.0f, r2 = (float) ((h >> 14) & 1023u) / 1023.0f;
        v.rustCents = (r1 * 2.0f - 1.0f) * params.rust * 45.0f;
        v.rustDamp = 1.0f - params.rust * 0.6f * r2;
        v.rustBuzz = params.rust > 0.3f && r2 > 0.75f;   // några stavar har spruckit och surrar alltid
        setupModes (v);
        tickEnv = juce::jmax (tickEnv, 0.4f + 0.6f * params.works);
    }

    void noteOff (int)
    {
        held = juce::jmax (0, held - 1);   // stavarna klingar ut av sig själva
    }

    void allNotesOff() { held = 0; }

    void render (float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        for (int i = 0; i < numSamples; ++i)
        {
            // Fjädern: tonen sjunker långsamt och vacklar när den tar slut.
            if ((counter & 255) == 0)
            {
                springTarget = -params.spring * (25.0f + 35.0f * random.nextFloat());
                for (auto& v : voices) if (v.active) setupModes (v);
            }
            springCents += (springTarget - springCents) * 0.00005f;
            wowPhase += (0.4 + 0.5 * params.spring) / sr;  wowPhase -= std::floor (wowPhase);
            ++counter;

            float out = 0.0f;
            for (auto& v : voices)
            {
                if (! v.active) continue;
                // Stiftet: en kort upphöjd cosinus (hårdare stift = kortare puls = ljusare).
                const double pulseLen = 0.0012 - 0.0009 * params.hardness;
                float force = 0.0f;
                if (v.t < pulseLen)
                    force = (float) (0.5 - 0.5 * std::cos (twoPi * v.t / pulseLen)) * v.velocity;
                // Surret: stiftet skrapar mot den svängande staven.
                if (v.buzz > 0.0f && v.t < 0.04)
                    force += (random.nextFloat() * 2.0f - 1.0f) * v.buzz * (float) (1.0 - v.t / 0.04);
                if (v.rustBuzz)
                    force += (random.nextFloat() * 2.0f - 1.0f) * 0.0015f * v.level;

                float y = 0.0f;
                for (auto& m : v.modes)
                {
                    const float s = m.b0 * force + m.a1 * m.y1 - m.a2 * m.y2;
                    m.y2 = m.y1;
                    m.y1 = s;
                    y += s * m.amp;
                }
                v.level = juce::jmax (std::abs (y), v.level * 0.9995f);
                out += y;
                v.t += 1.0 / sr;
                if (v.t > 0.2 && v.level < 1.0e-4f) v.active = false;
            }

            // Lådan: två trä-resonanser som färgar kammen.
            const float boxed = out + params.box * (body1.processSample (0, out) * 1.6f + body2.processSample (0, out) * 0.9f);

            // Urverket: tick från spärrhjulet och regulatorns vinge som surrar medan dosan spelar.
            tickEnv *= 0.99995f;
            float works = 0.0f;
            if (params.works > 0.001f && tickEnv > 0.01f)
            {
                tickPhase += 11.0 * (1.0 + 0.02 * std::sin (twoPi * wowPhase)) / sr;
                if (tickPhase >= 1.0) { tickPhase -= 1.0; tickClick = 1.0f; }
                tickClick *= 0.92f;
                const float n = random.nextFloat() * 2.0f - 1.0f;
                works = (n * tickClick * 0.35f + whirr.processSample (0, n) * 0.04f) * params.works * tickEnv;
            }
            dest[i] = boxed * 0.5f + works;
        }
    }

private:
    struct Mode { float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f, y1 = 0.0f, y2 = 0.0f, amp = 0.0f; };

    struct Voice
    {
        bool active = false, rustBuzz = false;
        int note = -1;
        float velocity = 1.0f, buzz = 0.0f, level = 0.0f, rustCents = 0.0f, rustDamp = 1.0f;
        double t = 0.0;
        std::array<Mode, numModes> modes {};
        juce::uint32 age = 0;
    };

    // Stavens deltoner som tvåpoliga resonatorer (behåller tillståndet när tonhöjden ändras).
    void setupModes (Voice& v)
    {
        static constexpr float ratios[3][numModes] = { { 1.0f, 6.267f, 17.55f, 34.39f },
                                                       { 1.0f, 2.01f, 6.27f, 17.5f },     // stor kam: en klockdelton i oktaven
                                                       { 1.0f, 2.76f, 5.40f, 8.93f } };   // glas
        static constexpr float amps[3][numModes] = { { 1.0f, 0.32f, 0.12f, 0.05f },
                                                     { 1.0f, 0.25f, 0.2f, 0.06f },
                                                     { 1.0f, 0.18f, 0.08f, 0.03f } };
        static constexpr float decays[3] = { 2.2f, 4.0f, 1.6f };
        const int t = (int) params.type;
        const double cents = springCents + params.spring * 12.0 * std::sin (juce::MathConstants<double>::twoPi * wowPhase) + v.rustCents;
        const double f0 = stamning->hz (v.note) * std::exp2 (cents / 1200.0);
        const double baseTau = decays[t] * juce::jlimit (0.25, 4.0, (double) params.decay) * v.rustDamp
                             * juce::jlimit (0.25, 2.0, std::sqrt (523.0 / f0));    // låga stavar klingar längre
        for (int k = 0; k < numModes; ++k)
        {
            auto& m = v.modes[(size_t) k];
            const double f = f0 * ratios[t][k];
            if (f > sr * 0.45) { m.amp = 0.0f; continue; }
            const double tau = baseTau / (1.0 + 1.5 * k);
            const double r = std::exp (-1.0 / (tau * sr));
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            m.a1 = (float) (2.0 * r * std::cos (w));
            m.a2 = (float) (r * r);
            // Jämn nivå över registret: en puls med ytan A ger svängningen ungefär A / sin(w); dela bort det.
            const double pulseSamples = (0.0012 - 0.0009 * params.hardness) * sr * 0.5;
            m.b0 = (float) (0.6 * std::sin (w) / juce::jmax (1.0, pulseSamples));
            m.amp = amps[t][k] * (k == 0 ? 1.0f : 0.6f + 0.8f * params.hardness);
        }
    }

    double sr = 44100.0;
    SpeldosanParams params;
    std::array<Voice, numVoices> voices {};
    juce::dsp::StateVariableTPTFilter<float> body1, body2, whirr;
    float springCents = 0.0f, springTarget = 0.0f, tickEnv = 0.0f, tickClick = 0.0f;
    double wowPhase = 0.0, tickPhase = 0.0;
    int held = 0;
    juce::uint32 counter = 0, ageCounter = 0;
    juce::Random random { 0x5350454c }; // "SPEL"
};

} // namespace krang
