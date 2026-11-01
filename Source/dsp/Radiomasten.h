// Radiomasten – KrangBox åttonde kammare: brus, signaler och atmosfär (fas 3b).
//
// En spelbar radiomottagare. Varje ton man spelar ställer in en station på tonens frekvens:
//   Kortvåg      – en visslande bärvåg som driver, och en avlägsen station som pratar i bruset
//   Morse        – telegrafsignaler på tonen, i takt med tempot; olika toner sänder olika meddelanden
//   Vind i ledningar – vinden sjunger i telegraftrådarna: brus genom trådens övertoner, i byar
//   Interferens  – en fjärrskrivare som växlar mellan två toner, svävande störtoner och nätbrum
// Över allt: etern. Brus (mottagarens sus, smalare med Bandbredd), fading (signalen sjunker undan och
// kommer tillbaka) och storm (åskknaster).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

enum class RadioMode { Kortvag = 0, Morse, Vind, Interferens };

struct RadiomastenParams
{
    RadioMode mode   = RadioMode::Kortvag;
    float signal     = 0.6f;   // hur stark stationen är mot bruset 0..1
    float noise      = 0.4f;   // mottagarbrus 0..1
    float bandwidth  = 0.5f;   // mottagarens bandbredd 0..1 (smal och burkig .. bred och fräsig)
    float drift      = 0.3f;   // stationen glider ur inställning 0..1
    float fading     = 0.3f;   // signalen sjunker undan och kommer tillbaka 0..1
    float storm      = 0.2f;   // åskknaster 0..1
    float attackSec  = 0.05f;
    float releaseSec = 0.6f;
};

class Radiomasten
{
public:
    static constexpr int numVoices = 8;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices)
        {
            v.env.setSampleRate (sampleRate);
            for (auto* f : { &v.receiver, &v.babble1, &v.babble2 })
            {
                f->prepare ({ sampleRate, 64, 1 });
                f->setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            }
            v.receiver.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            for (auto& w : v.wire)
            {
                w.prepare ({ sampleRate, 64, 1 });
                w.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            }
        }
        buildMorse();
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.env.reset(); v.note = -1; v.keyDown = false;
            for (auto* f : { &v.receiver, &v.babble1, &v.babble2 }) f->reset();
            for (auto& w : v.wire) w.reset();
        }
    }

    void setParams (const RadiomastenParams& p, double bpm)
    {
        params = p;
        morseUnit = juce::jmax (16, (int) (sr * 60.0 / juce::jlimit (30.0, 300.0, bpm) / 4.0)); // en prick = en sextondel
        const juce::ADSR::Parameters ep { juce::jmax (0.002f, p.attackSec), 0.1f, 1.0f, juce::jmax (0.01f, p.releaseSec) };
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
        v.note = note;
        v.keyDown = true;
        v.velocity = 0.4f + 0.6f * velocity;
        v.age = ++ageCounter;
        v.morsePos = 0; v.morseCounter = 0;
        v.message = messages.empty() ? 0 : (size_t) (note % (int) messages.size());
        v.fade = 1.0f; v.fadeTarget = 1.0f; v.fadeTimer = 0;
        v.driftCents = 0.0f; v.driftTarget = 0.0f; v.driftTimer = 0;
        v.syllableTimer = 0;
        v.baudTimer = 0;
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
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.env.isActive()) renderVoice (v, dest, numSamples);
    }

private:
    struct Voice
    {
        juce::ADSR env;
        juce::dsp::StateVariableTPTFilter<float> receiver, babble1, babble2;
        std::array<juce::dsp::StateVariableTPTFilter<float>, 4> wire;
        int note = -1;
        bool keyDown = false;
        float velocity = 1.0f;
        juce::uint32 age = 0;
        double phase = 0.0, phase2 = 0.0, humPhase = 0.0;
        // Morse
        size_t message = 0;
        int morsePos = 0, morseCounter = 0;
        float keyGain = 0.0f;
        // Fading, drift och den pratande stationen
        float fade = 1.0f, fadeTarget = 1.0f, driftCents = 0.0f, driftTarget = 0.0f;
        int fadeTimer = 0, driftTimer = 0, syllableTimer = 0;
        float syllableGain = 0.0f, syllableTarget = 0.0f;
        // Vind
        float gust = 0.5f, gustTarget = 0.5f;
        int gustTimer = 0;
        // Fjärrskrivaren
        int baudTimer = 0;
        bool mark = true;
        // Åskknaster
        float crash = 0.0f, crashLp = 0.0f;
    };

    // Telegrafmeddelanden, ett per ton (tonen modulo antal). Prick = 1, streck = 3 enheter.
    void buildMorse()
    {
        static const char* texts[] = { "KRANG", "SOS", "VERNE", "CQ CQ" };
        static const std::pair<char, const char*> code[] = {
            { 'A', ".-" }, { 'C', "-.-." }, { 'E', "." }, { 'G', "--." }, { 'K', "-.-" }, { 'N', "-." }, { 'O', "---" },
            { 'Q', "--.-" }, { 'R', ".-." }, { 'S', "..." }, { 'V', "...-" } };
        messages.clear();
        for (auto* t : texts)
        {
            std::vector<bool> units;
            for (const char* c = t; *c != 0; ++c)
            {
                if (*c == ' ') { units.insert (units.end(), 4, false); continue; } // 3 + 4 = ordmellanrum på 7
                const char* sym = "";
                for (auto& [ch, s] : code) if (ch == *c) sym = s;
                for (const char* d = sym; *d != 0; ++d)
                {
                    units.insert (units.end(), *d == '-' ? 3 : 1, true);
                    units.push_back (false);
                }
                units.insert (units.end(), 2, false); // bokstavsmellanrum 3
            }
            units.insert (units.end(), 7, false);     // paus innan meddelandet upprepas
            messages.push_back (std::move (units));
        }
    }

    float noise() { seed = seed * 1664525u + 1013904223u; return (float) (int) seed * (1.0f / 2147483648.0f); }
    float uni()   { return 0.5f + 0.5f * noise(); }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const double baseHz = 440.0 * std::pow (2.0, (v.note - 69) / 12.0);
        const float bw = juce::jlimit (0.0f, 1.0f, params.bandwidth);
        v.receiver.setCutoffFrequency (800.0f * std::pow (11.0f, bw));    // 0,8–8,8 kHz
        v.receiver.setResonance (0.7f + 2.5f * (1.0f - bw));               // smal mottagare blir burkig
        const float sig = params.signal, nz = params.noise * 0.5f;
        static const std::vector<bool> silence { false };
        const auto& units = messages.empty() ? silence : messages[v.message];

        // Vinden: trådens övertoner, smalare med starkare signal.
        if (params.mode == RadioMode::Vind)
        {
            static constexpr float harmonics[4] = { 1.0f, 2.0f, 3.0f, 5.0f };
            for (int k = 0; k < 4; ++k)
            {
                v.wire[(size_t) k].setCutoffFrequency (juce::jmin ((float) (baseHz * harmonics[k]), (float) sr * 0.45f));
                v.wire[(size_t) k].setResonance (4.0f + 30.0f * sig);
            }
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = v.env.getNextSample();

            // Fading och drift: långsamma slumpvandringar.
            if (--v.fadeTimer <= 0)
            {
                v.fadeTarget = 1.0f - params.fading * uni();
                v.fadeTimer = (int) (sr * (0.4 + 2.0 * uni()));
            }
            v.fade += (v.fadeTarget - v.fade) * 0.00015f;
            if (--v.driftTimer <= 0)
            {
                v.driftTarget = noise() * params.drift * 60.0f;
                v.driftTimer = (int) (sr * (0.3 + 1.2 * uni()));
            }
            v.driftCents += (v.driftTarget - v.driftCents) * 0.0002f;
            const double hz = baseHz * std::exp2 (v.driftCents / 1200.0f);

            float station = 0.0f;
            switch (params.mode)
            {
                case RadioMode::Kortvag:
                {
                    // Bärvågen visslar på tonen …
                    v.phase += hz / sr; v.phase -= std::floor (v.phase);
                    const float whistle = (float) std::sin (twoPi * v.phase) * 0.35f;
                    // … och en avlägsen station pratar: brus genom två formanter som byter vokal i stavelsetakt.
                    if (--v.syllableTimer <= 0)
                    {
                        static constexpr float f1[] = { 700, 400, 300, 450, 350 }, f2[] = { 1150, 2000, 2200, 800, 700 };
                        const int vowel = (int) (uni() * 4.99f);
                        v.babble1.setCutoffFrequency (f1[vowel]); v.babble1.setResonance (4.0f);
                        v.babble2.setCutoffFrequency (f2[vowel]); v.babble2.setResonance (5.0f);
                        v.syllableTarget = uni() < 0.25f ? 0.0f : 0.5f + 0.5f * uni();
                        v.syllableTimer = (int) (sr * (0.08 + 0.17 * uni()));
                    }
                    v.syllableGain += (v.syllableTarget - v.syllableGain) * 0.004f;
                    const float n = noise();
                    const float voice = (v.babble1.processSample (0, n) + 0.7f * v.babble2.processSample (0, n)) * v.syllableGain * 0.9f;
                    station = whistle + voice;
                    break;
                }
                case RadioMode::Morse:
                {
                    if (++v.morseCounter >= morseUnit)
                    {
                        v.morseCounter = 0;
                        v.morsePos = (v.morsePos + 1) % (int) units.size();
                    }
                    const float target = units[(size_t) v.morsePos] ? 1.0f : 0.0f;
                    v.keyGain += (target - v.keyGain) * 0.008f; // mjuka flanker, inga klick
                    v.phase += hz / sr; v.phase -= std::floor (v.phase);
                    station = (float) std::sin (twoPi * v.phase) * v.keyGain * 0.6f;
                    break;
                }
                case RadioMode::Vind:
                {
                    if (--v.gustTimer <= 0)
                    {
                        v.gustTarget = 0.2f + 0.8f * uni();
                        v.gustTimer = (int) (sr * (0.5 + 2.5 * uni()));
                    }
                    v.gust += (v.gustTarget - v.gust) * 0.0001f;
                    const float n = noise();
                    // Hårdare vind väcker de högre övertonerna.
                    float w = v.wire[0].processSample (0, n) * (1.2f - v.gust) + v.wire[1].processSample (0, n) * 0.8f
                            + v.wire[2].processSample (0, n) * v.gust * 0.8f + v.wire[3].processSample (0, n) * v.gust * v.gust * 0.6f;
                    station = w * v.gust * 0.6f;
                    break;
                }
                case RadioMode::Interferens:
                default:
                {
                    // Fjärrskrivaren: växlar mellan två toner med 45,45 baud.
                    if (--v.baudTimer <= 0) { v.mark = uni() < 0.5f; v.baudTimer = (int) (sr / 45.45); }
                    const double fsk = v.mark ? hz : hz * 1.17;
                    v.phase += fsk / sr; v.phase -= std::floor (v.phase);
                    // Störtonen svävar mot den, och nätbrummet ligger under.
                    v.phase2 += hz * (1.0 + 0.004 + 0.01 * params.drift) / sr; v.phase2 -= std::floor (v.phase2);
                    v.humPhase += 50.0 / sr; v.humPhase -= std::floor (v.humPhase);
                    const float hum = (float) (v.humPhase < 0.1 ? 1.0 : 0.0) - 0.1f;
                    station = (float) (std::sin (twoPi * v.phase) * 0.35 + std::sin (twoPi * v.phase2) * 0.2) + hum * 0.15f;
                    break;
                }
            }

            // Åskknaster: korta smällar, oftare med mer storm.
            if (uni() < params.storm * params.storm * 0.0006f) v.crash = 0.6f + 0.4f * uni();
            v.crash *= 0.9985f;
            const float crashNoise = noise() * v.crash * v.crash;
            v.crashLp += 0.35f * (crashNoise - v.crashLp);

            float out = station * sig * v.fade + noise() * nz + v.crashLp * params.storm;
            out = v.receiver.processSample (0, out);
            dest[i] += out * env * v.velocity;
        }
    }

    double sr = 44100.0;
    RadiomastenParams params;
    std::array<Voice, numVoices> voices;
    std::vector<std::vector<bool>> messages;
    int morseUnit = 6000;
    juce::uint32 ageCounter = 0, seed = 0x52414449; // "RADI"
};

} // namespace krang
