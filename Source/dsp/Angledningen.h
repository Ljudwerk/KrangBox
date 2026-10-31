// Ångledningen, del 2: filter och Katedralreverb (fas 1).
// Kedjan i KrangEngine: kammare -> Krangdistortion -> Ångfilter -> Katedralreverb (mono in, stereo ut).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

// --- Ångfiltret --------------------------------------------------------------

enum class FxFilterMode { Av = 0, Lagpass, Hogpass, Formant, Kam };

struct FxFilterParams
{
    FxFilterMode mode = FxFilterMode::Av;
    float cutoffHz  = 2000.0f;  // Lågpass/Högpass: brytfrekvens. Kam: grundton.
    float resonance = 0.3f;     // 0..1
    float vowel     = 0.0f;     // Formant: 0 = A, 0.25 = E, 0.5 = I, 0.75 = O, 1 = U
};

class AngFilter
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        ladder.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
        for (auto& f : formants)
        {
            f.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
            f.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        }
        comb.assign ((size_t) (sampleRate / 30.0) + 4, 0.0f);
        reset();
    }

    void reset()
    {
        ladder.reset();
        for (auto& f : formants) f.reset();
        std::fill (comb.begin(), comb.end(), 0.0f);
        combPos = 0;
    }

    void setParams (const FxFilterParams& p) { params = p; }

    void process (float* data, int numSamples)
    {
        switch (params.mode)
        {
            case FxFilterMode::Av: return;
            case FxFilterMode::Lagpass:
            case FxFilterMode::Hogpass:
            {
                ladder.setMode (params.mode == FxFilterMode::Lagpass ? juce::dsp::LadderFilterMode::LPF24
                                                                     : juce::dsp::LadderFilterMode::HPF24);
                ladder.setCutoffFrequencyHz (juce::jlimit (20.0f, (float) sr * 0.45f, params.cutoffHz));
                ladder.setResonance (juce::jlimit (0.0f, 0.95f, params.resonance));
                float* ch[] = { data };
                juce::dsp::AudioBlock<float> block (ch, 1, (size_t) numSamples);
                ladder.process (juce::dsp::ProcessContextReplacing<float> (block));
                return;
            }
            case FxFilterMode::Formant: processFormant (data, numSamples); return;
            case FxFilterMode::Kam:     processComb (data, numSamples);    return;
        }
    }

private:
    // Tre formanter per vokal (Hz), ungefärliga svenska vokaler: A, E, I, O, U.
    void processFormant (float* data, int numSamples)
    {
        static constexpr float table[5][3] = { { 800, 1150, 2900 }, { 400, 2100, 2700 }, { 280, 2250, 2900 },
                                               { 450,  800, 2830 }, { 325,  700, 2700 } };
        static constexpr float gains[3] = { 1.0f, 0.6f, 0.35f };
        const float pos = juce::jlimit (0.0f, 1.0f, params.vowel) * 4.0f;
        const int i0 = juce::jmin (3, (int) pos);
        const float frac = pos - (float) i0;
        const float q = 2.0f + params.resonance * 14.0f;

        for (size_t k = 0; k < 3; ++k)
        {
            const float f = table[i0][k] + (table[i0 + 1][k] - table[i0][k]) * frac;
            formants[k].setCutoffFrequency (f);
            formants[k].setResonance (q);
        }
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = data[i];
            float y = 0.0f;
            for (size_t k = 0; k < 3; ++k)
                y += formants[k].processSample (0, x) * gains[k];
            data[i] = y * 1.6f;
        }
    }

    // Kamfilter med återkoppling: metallisk ton på grundfrekvensen, som ljud genom ett långt rör.
    void processComb (float* data, int numSamples)
    {
        const float f = juce::jlimit (40.0f, 4000.0f, params.cutoffHz);
        const float delay = juce::jlimit (2.0f, (float) comb.size() - 2.0f, (float) sr / f);
        const float fb = juce::jlimit (0.0f, 0.95f, params.resonance * 0.95f);
        const int size = (int) comb.size();

        for (int i = 0; i < numSamples; ++i)
        {
            float readPos = (float) combPos - delay;
            if (readPos < 0) readPos += (float) size;
            const int r0 = (int) readPos;
            const float frac = readPos - (float) r0;
            const float delayed = comb[(size_t) r0] + frac * (comb[(size_t) ((r0 + 1) % size)] - comb[(size_t) r0]);
            const float y = data[i] + fb * delayed;
            comb[(size_t) combPos] = y;
            combPos = (combPos + 1) % size;
            data[i] = 0.5f * (data[i] + y * (1.0f - fb * 0.6f));
        }
    }

    double sr = 44100.0;
    FxFilterParams params;
    juce::dsp::LadderFilter<float> ladder;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 3> formants;
    std::vector<float> comb;
    int combPos = 0;
};

// --- Katedralreverb ----------------------------------------------------------

enum class ReverbMode { Hall = 0, Platta, RostigTank };

struct ReverbParams
{
    ReverbMode mode = ReverbMode::Hall;
    float mix  = 0.15f;  // 0..1
    float size = 0.6f;   // 0..1
    float dark = 0.4f;   // 0..1 – Mörker: dämpar diskanten i svansen
};

class Katedralreverb
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        reverb.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 2 });
        springBuf.assign ((size_t) (sampleRate * 0.08) + 4, 0.0f);
        wetL.assign ((size_t) maxBlockSize, 0.0f);
        wetR.assign ((size_t) maxBlockSize, 0.0f);
        reset();
    }

    void reset()
    {
        reverb.reset();
        std::fill (springBuf.begin(), springBuf.end(), 0.0f);
        for (auto& a : allpass) a = 0.0f;
        springPos = 0;
        springLp = 0.0f;
    }

    void setParams (const ReverbParams& p)
    {
        params = p;
        juce::dsp::Reverb::Parameters rp;
        const float size = juce::jlimit (0.0f, 1.0f, p.size);
        switch (p.mode)
        {
            case ReverbMode::Hall:       rp.roomSize = 0.70f + 0.25f * size; rp.damping = 0.15f + 0.7f * p.dark; rp.width = 1.0f; break;
            case ReverbMode::Platta:     rp.roomSize = 0.40f + 0.40f * size; rp.damping = 0.05f + 0.4f * p.dark; rp.width = 0.9f; break;
            case ReverbMode::RostigTank: rp.roomSize = 0.25f + 0.25f * size; rp.damping = 0.3f + 0.5f * p.dark;  rp.width = 0.6f; break;
        }
        rp.wetLevel = 0.3f;  // Freeverb förstärker svansen ~3x internt; 0.3 ger ungefär samma nivå som torrt
        rp.dryLevel = 0.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
    }

    // Mono in (in), stereo ut (outL, outR). in och outL får vara samma buffert.
    void process (const float* in, float* outL, float* outR, int numSamples)
    {
        // Värden kan skicka större block än förberett: dela upp.
        const int maxChunk = (int) wetL.size();
        while (numSamples > maxChunk)
        {
            processChunk (in, outL, outR, maxChunk);
            in += maxChunk; outL += maxChunk; outR += maxChunk; numSamples -= maxChunk;
        }
        processChunk (in, outL, outR, numSamples);
    }

private:
    void processChunk (const float* in, float* outL, float* outR, int numSamples)
    {
        const float mix = juce::jlimit (0.0f, 1.0f, params.mix);
        if (mix <= 0.0001f)
        {
            for (int i = 0; i < numSamples; ++i) { outL[i] = in[i]; outR[i] = in[i]; }
            return;
        }

        float* wl = wetL.data();
        float* wr = wetR.data();
        if (params.mode == ReverbMode::RostigTank)
            processSpring (in, wl, wr, numSamples);
        else
            for (int i = 0; i < numSamples; ++i) wl[i] = wr[i] = in[i];

        float* ch[] = { wl, wr };
        juce::dsp::AudioBlock<float> block (ch, 2, (size_t) numSamples);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (block));

        const float dryGain = 1.0f - 0.5f * mix;
        for (int i = 0; i < numSamples; ++i)
        {
            const float x = in[i];
            outL[i] = x * dryGain + wl[i] * mix;
            outR[i] = x * dryGain + wr[i] * mix;
        }
    }

    // Fjäderreverb, "rostig tank": en kedja allpassfilter (spridningen ger fjäderns "boing")
    // i en återkopplad fördröjning, med två olika uttag för vänster och höger.
    void processSpring (const float* in, float* l, float* r, int numSamples)
    {
        const int size = (int) springBuf.size();
        const int d = (int) (sr * 0.033), dR = (int) (sr * 0.041);
        const float fb = 0.55f + 0.35f * juce::jlimit (0.0f, 1.0f, params.size);
        const float lpCoef = std::exp (-juce::MathConstants<float>::twoPi * (6500.0f - 5000.0f * params.dark) / (float) sr);

        for (int i = 0; i < numSamples; ++i)
        {
            const int readL = (springPos - d + size) % size, readR = (springPos - dR + size) % size;
            float x = in[i] + fb * springBuf[(size_t) readL];

            // Spridning: första ordningens allpass i serie.
            for (auto& z : allpass)
            {
                const float y = -0.65f * x + z;
                z = x + 0.65f * y;
                x = y;
            }
            springLp = x + lpCoef * (springLp - x);
            springBuf[(size_t) springPos] = springLp;
            springPos = (springPos + 1) % size;

            l[i] = springBuf[(size_t) readL] * 0.7f;
            r[i] = springBuf[(size_t) readR] * 0.7f;
        }
    }

    double sr = 44100.0;
    ReverbParams params;
    juce::dsp::Reverb reverb;
    std::vector<float> springBuf, wetL, wetR;
    std::array<float, 10> allpass {};
    int springPos = 0;
    float springLp = 0.0f;
};

} // namespace krang
