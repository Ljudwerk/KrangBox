// Krigstuban – KrangBox femtonde kammare: en mur av mässing från en dystopisk framtid (fas 3d).
//
// Fysiskt modellerat som Lurblåsarens bleckblås: läpparna släpper ut luftpuffar som driver en luftpelare
// (en fördröjning), och klockstycket strålar ut ljudet. Men här blåser en hel orkester:
//   Orkester   – från en ensam tuba till en mur av mässing: fler blåsare, lite ostämda mot varandra,
//                en oktav under och i toppen också en kvint över
//   Lufttryck  – från en mjuk, mörk ton till en stöt där läpparna smäller: luftpelaren mättas och tonen
//                blir ljusare och mer skärande ju hårdare man blåser, som i riktig mässing
//   Morr       – blåsaren nynnar i instrumentet: undertonen en oktav ned och ett rått brummande
//   Fladder    – fladdertunga, en snabb rullning med tungan
//   Svällning  – hur långsamt tonen sväller upp till full styrka
//   Fall       – hur långt tonen faller när den släpps
//   Mullret    – ett mullrande djup under alltihop, för de riktigt stora slagen
//   Tratt      – Mässing (en vanlig klocka), Ljudtratt (första världskrigets jättelika lyssnartrattar:
//                ihåligt och avlägset) eller Megafon (en trasig högtalare)
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

enum class TubaBell { Massing = 0, Ljudtratt, Megafon };

struct KrigstubanParams
{
    TubaBell bell    = TubaBell::Massing;
    float ensemble   = 0.5f;   // 0 = en tuba, 1 = en mur av mässing
    float pressure   = 0.6f;   // lufttryck 0..1
    float growl      = 0.2f;   // morr 0..1
    float flutter    = 0.0f;   // fladdertunga 0..1
    float swell      = 0.3f;   // svällning 0..1 (0,05–3 s)
    float fall       = 0.3f;   // fall vid släpp 0..1 (upp till en kvart)
    float rumble     = 0.3f;   // mullret 0..1
    float releaseSec = 0.5f;
};

class Krigstuban
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    static constexpr int numVoices = 6, maxPlayers = 8;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        const size_t boreLen = (size_t) (sampleRate / 15.0) + 16;      // räcker ned till 15 Hz
        for (auto& v : voices)
            for (auto& pl : v.players) pl.bore.assign (boreLen, 0.0f);
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto* f : { &bellBand, &bellLow, &megaHp, &megaLp, &rumbleLp })
        {
            f->prepare (spec);
            f->reset();
        }
        bellBand.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        bellBand.setCutoffFrequency (700.0f);  bellBand.setResonance (1.4f);
        bellLow.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        bellLow.setCutoffFrequency (2400.0f);  bellLow.setResonance (0.7f);
        megaHp.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        megaHp.setCutoffFrequency (500.0f);    megaHp.setResonance (0.9f);
        megaLp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        megaLp.setCutoffFrequency (3200.0f);   megaLp.setResonance (1.3f);
        rumbleLp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        rumbleLp.setCutoffFrequency (55.0f);   rumbleLp.setResonance (0.7f);
        comb.assign ((size_t) (sampleRate * 0.05) + 4, 0.0f);
        for (int k = 0; k <= sineSize; ++k)
            sineTable[(size_t) k] = (float) std::sin (juce::MathConstants<double>::twoPi * k / sineSize);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) clearVoice (v);
        for (auto* f : { &bellBand, &bellLow, &megaHp, &megaLp, &rumbleLp }) f->reset();
        std::fill (comb.begin(), comb.end(), 0.0f);
        combPos = 0;
    }

    void setParams (const KrigstubanParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        if (! v.active) clearVoice (v);
        v.active = v.keyDown = true;
        v.note = note;
        v.velocity = 0.5f + 0.5f * velocity;
        v.age = ++ageCounter;
        v.fallSemis = 0.0f;
        v.tail = 0;
        // Varje blåsare är lite ostämd och kommer in lite olika fort, som en riktig sektion.
        for (int k = 0; k < maxPlayers; ++k)
        {
            auto& pl = v.players[(size_t) k];
            pl.detune = (random.nextFloat() * 2.0f - 1.0f);
            pl.delayIn = random.nextFloat() * 0.06f;
            pl.vibPhase = random.nextDouble();
        }
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note) v.keyDown = false;
    }

    void allNotesOff() { for (auto& v : voices) v.keyDown = false; }

    void render (float* dest, int numSamples)
    {
        // Läppöppningens form för blockets lufttryck (anslaget skalar trycket, inte formen).
        const float sharpness = 1.0f + 13.0f * params.pressure;
        if (sharpness != tableSharpness)
        {
            tableSharpness = sharpness;
            for (int k = 0; k < lipTableSize; ++k)
                lipTable[(size_t) k] = std::pow ((float) k / (float) (lipTableSize - 1), sharpness);
        }
        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f, lowest = 0.0f;
            double lowestHz = 0.0;
            for (auto& v : voices)
            {
                if (! v.active) continue;
                out += renderVoice (v);
                const double hz = stamning->hz (v.note);
                if (lowestHz == 0.0 || hz < lowestHz) { lowestHz = hz; lowest = v.amp; }
            }

            // Mullret: en sinus en oktav under den lägsta tonen, och ett dovt brus.
            if (params.rumble > 0.001f)
            {
                if (lowestHz > 0.0) { subPhase += lowestHz * 0.5 / sr; subPhase -= std::floor (subPhase); }
                subAmp += ((float) lowest - subAmp) * 0.0005f;
                const float n = random.nextFloat() * 2.0f - 1.0f;
                out += (float) std::sin (juce::MathConstants<double>::twoPi * subPhase) * subAmp * params.rumble * 0.6f
                     + rumbleLp.processSample (0, n) * subAmp * params.rumble * 1.5f;
            }

            // Tratten.
            switch (params.bell)
            {
                case TubaBell::Massing: break;
                case TubaBell::Ljudtratt:
                {
                    // Ihåligt och avlägset: två kamfilter (trattens längd) och ett smalt band.
                    const int size = (int) comb.size();
                    const int d1 = (int) (0.0043 * sr), d2 = (int) (0.0117 * sr);
                    const float c1 = comb[(size_t) ((combPos - d1 + size) % size)], c2 = comb[(size_t) ((combPos - d2 + size) % size)];
                    const float x = out + 0.55f * c1 + 0.35f * c2;
                    comb[(size_t) combPos] = std::tanh (x);
                    combPos = (combPos + 1) % size;
                    out = bellLow.processSample (0, bellBand.processSample (0, x) * 1.8f + x * 0.25f);
                    break;
                }
                case TubaBell::Megafon:
                {
                    // En trasig högtalare: smalt band, hårt klippt.
                    const float band = megaLp.processSample (0, megaHp.processSample (0, out * 2.5f));
                    out = juce::jlimit (-0.6f, 0.6f, band) * 1.4f;
                    break;
                }
            }
            dest[i] = out;
        }
    }

private:
    struct Player
    {
        std::vector<float> bore;
        int pos = 0;
        double lipPhase = 0.0, vibPhase = 0.0;
        float lp = 0.0f, out = 0.0f, dcx = 0.0f, dcy = 0.0f, detune = 0.0f, delayIn = 0.0f, gain = 1.0f;
        double hz = 0.0;
    };

    struct Voice
    {
        bool active = false, keyDown = false;
        int note = 48, tail = 0;
        float velocity = 1.0f, amp = 0.0f, fallSemis = 0.0f, flutterLp = 0.0f;
        double t = 0.0, growlPhase = 0.0, flutterPhase = 0.0;
        std::array<Player, maxPlayers> players;
        juce::uint32 age = 0, counter = 0;
    };

    void clearVoice (Voice& v)
    {
        v.active = v.keyDown = false;
        v.amp = 0.0f;
        v.t = 0.0;
        for (auto& pl : v.players)
        {
            std::fill (pl.bore.begin(), pl.bore.end(), 0.0f);
            pl.pos = 0;
            pl.lp = pl.out = pl.dcx = pl.dcy = 0.0f;
            pl.lipPhase = 0.0;
            pl.hz = 0.0;
        }
    }

    static float readDelay (const std::vector<float>& buf, int writePos, double delay)
    {
        const int size = (int) buf.size();
        double rp = (double) writePos - delay;
        while (rp < 0.0) rp += size;
        const int i0 = (int) rp;
        const float f = (float) (rp - i0);
        return buf[(size_t) i0] + f * (buf[(size_t) ((i0 + 1) % size)] - buf[(size_t) i0]);
    }

    float renderVoice (Voice& v)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        // Hur många blåsare som spelar, och var: grundtonen, en oktav under, och i toppen en kvint över.
        const float ens = juce::jlimit (0.0f, 1.0f, params.ensemble);
        const int players = 1 + (int) std::round (ens * (maxPlayers - 1));
        const float spreadCents = 4.0f + 14.0f * ens;

        // Svällningen och släppet: tonen sväller upp, och faller när den släpps.
        const double swellSec = 0.05 + 3.0 * params.swell * params.swell;
        const float target = v.keyDown ? 1.0f : 0.0f;
        const float rate = v.keyDown ? (float) (1.0 / (swellSec * sr)) * 4.0f : (float) (1.0 / (juce::jmax (0.02f, params.releaseSec) * sr)) * 4.0f;
        v.amp += (target - v.amp) * juce::jmin (1.0f, rate);
        if (! v.keyDown) v.fallSemis -= params.fall * 5.0f * 1.2f / (float) sr / juce::jmax (0.05f, params.releaseSec);

        // Morr och fladder: läpparnas luftflöde moduleras.
        v.growlPhase += stamning->hz (v.note) * 0.5 / sr;
        v.growlPhase -= std::floor (v.growlPhase);
        v.flutterPhase += 26.0 / sr; v.flutterPhase -= std::floor (v.flutterPhase);
        const float n0 = random.nextFloat() * 2.0f - 1.0f;
        v.flutterLp += (n0 - v.flutterLp) * 0.01f;
        const float growlMod = 1.0f + params.growl * (0.7f * (float) std::sin (twoPi * v.growlPhase) + 2.5f * v.flutterLp);
        const float flutterMod = 1.0f - params.flutter * 0.85f * (v.flutterPhase < 0.5 ? 1.0f : 0.0f);

        const float pressure = (0.35f + 0.65f * params.pressure) * v.velocity;
        float sum = 0.0f;
        for (int k = 0; k < players; ++k)
        {
            auto& pl = v.players[(size_t) k];
            if (v.t < pl.delayIn) continue;
            // Stämman: var fjärde blåsare spelar en oktav under, den sjunde en kvint över (när orkestern är stor).
            int key = v.note;                         // tangenten blåsaren spelar (stäms efter stämningen)
            if (k % 4 == 3) key -= 12;
            if (k == 6 && ens > 0.7f) key += 7;
            double semis = v.fallSemis + pl.detune * spreadCents / 100.0;   // fall och ostämdhet ovanpå
            // Tonhöjden räknas om var 16:e sample (vibrato och fall rör sig långsamt).
            if ((v.counter & 15) == 0 || pl.hz == 0.0)
            {
                pl.vibPhase += 16.0 * 4.6 / sr; pl.vibPhase -= std::floor (pl.vibPhase);
                semis += 0.08 * std::sin (twoPi * pl.vibPhase) * juce::jmin (1.0, v.t / 0.8);
                pl.hz = stamning->hz (key) * std::exp2 (semis / 12.0);
                // Höga toner tappar energi i röret: lyft dem så att alla registren låter lika starka.
                pl.gain = (float) juce::jlimit (1.0, 10.0, std::pow (pl.hz / 55.0, 1.35));
            }
            const double hz = pl.hz;
            const double boreDelay = juce::jlimit (4.0, (double) pl.bore.size() - 4.0, sr / hz);

            // Läpparna: smala puffar vid högt tryck, breda vid lågt (s upphöjt till skärpan, ur en tabell).
            pl.lipPhase += hz / sr; pl.lipPhase -= std::floor (pl.lipPhase);
            const double sx = pl.lipPhase * sineSize;
            const int si = (int) sx;
            const float s = sineTable[(size_t) si] + (float) (sx - si) * (sineTable[(size_t) si + 1] - sineTable[(size_t) si]);
            float open = 0.0f;
            if (s > 0.0f)
            {
                const float x = s * (float) (lipTableSize - 1);
                const int ix = juce::jmin (lipTableSize - 2, (int) x);
                open = lipTable[(size_t) ix] + (x - (float) ix) * (lipTable[(size_t) ix + 1] - lipTable[(size_t) ix]);
            }
            const float flow = open * pressure * growlMod * flutterMod * (1.0f + n0 * 0.04f * params.pressure);

            // Luftpelaren, med förluster i klockstycket, och mässingens mättning vid högt tryck.
            const float boreOut = readDelay (pl.bore, pl.pos, boreDelay);
            pl.lp += 0.45f * (boreOut - pl.lp);
            float frame = flow + 0.9f * pl.lp;
            frame = std::tanh (frame * (1.0f + 2.5f * params.pressure)) / (1.0f + 0.8f * params.pressure);
            pl.bore[(size_t) pl.pos] = juce::jlimit (-4.0f, 4.0f, frame);
            pl.pos = (pl.pos + 1) % (int) pl.bore.size();
            const float radiated = frame - 0.5f * pl.out;
            pl.out = frame;
            const float dc = radiated - pl.dcx + 0.995f * pl.dcy;
            pl.dcx = radiated; pl.dcy = dc;
            sum += dc * pl.gain;
        }
        v.t += 1.0 / sr;
        ++v.counter;
        const float y = std::tanh (sum * 4.0f / 3.0f / std::sqrt ((float) players)) * v.amp * 0.55f;
        if (! v.keyDown && v.amp < 0.0005f && ++v.tail > (int) (sr * 0.3)) clearVoice (v);
        return std::isfinite (y) ? y : 0.0f;
    }

    static constexpr int lipTableSize = 512, sineSize = 2048;
    std::array<float, sineSize + 1> sineTable {};
    std::array<float, lipTableSize> lipTable {};
    float tableSharpness = -1.0f;
    double sr = 44100.0;
    KrigstubanParams params;
    std::array<Voice, numVoices> voices;
    juce::dsp::StateVariableTPTFilter<float> bellBand, bellLow, megaHp, megaLp, rumbleLp;
    std::vector<float> comb;
    int combPos = 0;
    double subPhase = 0.0;
    float subAmp = 0.0f;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x4b524947 }; // "KRIG"
};

} // namespace krang
