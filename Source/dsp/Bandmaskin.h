// Ångledningen, del 3 (fas 3): Bandmaskinen och Ekokammaren.
// Kedjan: kammare -> Krangdistortion -> Ångfilter -> Bandmaskin -> Ekokammare -> Katedralreverb.
//
//   Bandmaskinen: ett slitet rullband. Wow (långsam tonhöjdsvajning), flutter (snabb darrning),
//   ålder (bandmättning, sus och förlorad diskant) och avbrott (bandet tappar kontakten med tonhuvudet).
//   Ekokammaren: bandeko där varje upprepning blir mörkare och mättas i återkopplingen, så att
//   ekot kan drivas till självsvängning utan att skena. Tiden glider som på en riktig bandeko när den ändras.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

namespace krang
{

// --- Bandmaskinen ---------------------------------------------------------------

struct TapeParams
{
    float wow     = 0.0f;  // 0..1
    float flutter = 0.0f;  // 0..1
    float age     = 0.0f;  // 0..1
    float dropout = 0.0f;  // 0..1
};

class Bandmaskin
{
public:
    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        buffer.assign ((size_t) (sampleRate * 0.05) + 4, 0.0f); // 50 ms räcker för djupaste wow
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        pos = 0; wowPhase = flutterPhase = 0.0; lp = 0.0f; dropGain = 1.0f; dropTarget = 1.0f; dropTimer = 0;
    }

    void setParams (const TapeParams& p) { params = p; }

    bool isActive() const { return params.wow + params.flutter + params.age + params.dropout > 0.001f; }

    void process (float* data, int numSamples)
    {
        if (! isActive()) return;
        const int size = (int) buffer.size();
        const double baseDelay = sr * 0.008;                         // 8 ms mitt i bufferten
        const double wowDepth = sr * 0.004 * params.wow;              // upp till ±4 ms
        const double flutterDepth = sr * 0.0004 * params.flutter;     // upp till ±0,4 ms
        const double wowHz = 0.6, flutterHz = 9.0;
        const float cutoff = 18000.0f - 15000.0f * params.age;        // åldrat band tappar diskanten
        const float lpCoef = std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);
        const float drive = 1.0f + 3.0f * params.age;

        for (int i = 0; i < numSamples; ++i)
        {
            buffer[(size_t) pos] = data[i];

            wowPhase += wowHz / sr;          wowPhase -= std::floor (wowPhase);
            flutterPhase += flutterHz / sr;  flutterPhase -= std::floor (flutterPhase);
            const double d = baseDelay + wowDepth * std::sin (juce::MathConstants<double>::twoPi * wowPhase)
                                       + flutterDepth * std::sin (juce::MathConstants<double>::twoPi * flutterPhase);
            double rp = (double) pos - d;
            while (rp < 0) rp += size;
            const int r0 = (int) rp;
            const float f = (float) (rp - r0);
            float y = buffer[(size_t) r0] + f * (buffer[(size_t) ((r0 + 1) % size)] - buffer[(size_t) r0]);
            pos = (pos + 1) % size;

            // Ålder: mättning, diskantförlust och sus.
            if (params.age > 0.001f)
            {
                y = std::tanh (y * drive) / drive * (1.0f + 0.3f * params.age);
                lp = y + lpCoef * (lp - y);
                y = lp + (random.nextFloat() - 0.5f) * 0.006f * params.age;
            }

            // Avbrott: då och då tappar bandet kontakten under 20–150 ms.
            if (params.dropout > 0.001f && --dropTimer <= 0)
            {
                const bool drop = random.nextFloat() < 0.25f * params.dropout;
                dropTarget = drop ? 0.05f + 0.4f * random.nextFloat() : 1.0f;
                dropTimer = (int) (sr * (drop ? 0.02 + 0.13 * random.nextFloat() : 0.1 + 0.4 * random.nextFloat()));
            }
            dropGain += (dropTarget - dropGain) * 0.003f;
            data[i] = y * (params.dropout > 0.001f ? dropGain : 1.0f);
        }
    }

private:
    double sr = 44100.0;
    TapeParams params;
    std::vector<float> buffer;
    int pos = 0, dropTimer = 0;
    double wowPhase = 0.0, flutterPhase = 0.0;
    float lp = 0.0f, dropGain = 1.0f, dropTarget = 1.0f;
    juce::Random random { 0x42414e44 }; // "BAND"
};

// --- Ekokammaren ----------------------------------------------------------------

struct EchoParams
{
    float mix      = 0.0f;    // 0..1 (0 = av)
    float timeMs   = 350.0f;  // fri tid, 20..1500 ms
    int sync       = 0;       // 0 = fri, 1 = 1/4, 2 = 1/8, 3 = punkterad 1/8, 4 = 1/16, 5 = 1/4-triol
    float feedback = 0.4f;    // 0..1.1 (över 1 = självsvängning, hålls i schack av mättningen)
    float dark     = 0.4f;    // 0..1, hur mycket mörkare varje upprepning blir
};

class Ekokammare
{
public:
    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        buffer.assign ((size_t) (sampleRate * 2.1) + 4, 0.0f);
        delaySmooth.reset (sampleRate, 0.25); // tidsändring glider på en kvarts sekund, som en bandeko
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        pos = 0; lp = 0.0f; hp = 0.0f; hpX = 0.0f;
        delaySmooth.setCurrentAndTargetValue (sr * 0.35);
    }

    void setParams (const EchoParams& p, double bpm)
    {
        params = p;
        static constexpr double beats[6] = { 0.0, 1.0, 0.5, 0.75, 0.25, 2.0 / 3.0 };
        const double seconds = p.sync > 0 ? beats[juce::jlimit (1, 5, p.sync)] * 60.0 / juce::jlimit (20.0, 400.0, bpm)
                                          : (double) p.timeMs * 0.001;
        delaySmooth.setTargetValue (juce::jlimit (sr * 0.01, sr * 2.0, seconds * sr));
    }

    void process (float* data, int numSamples)
    {
        if (params.mix <= 0.001f) return; // av
        const int size = (int) buffer.size();
        const float cutoff = 12000.0f - 10500.0f * params.dark;
        const float lpCoef = std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);
        const float hpCoef = std::exp (-juce::MathConstants<float>::twoPi * 90.0f / (float) sr);
        const float fb = juce::jlimit (0.0f, 1.1f, params.feedback);

        for (int i = 0; i < numSamples; ++i)
        {
            const double d = delaySmooth.getNextValue();
            double rp = (double) pos - d;
            while (rp < 0) rp += size;
            const int r0 = (int) rp;
            const float f = (float) (rp - r0);
            const float delayed = buffer[(size_t) r0] + f * (buffer[(size_t) ((r0 + 1) % size)] - buffer[(size_t) r0]);

            // Återkopplingen: mörkare för varje varv, utan bas som dånar, och mättad så att den aldrig skenar.
            lp = delayed + lpCoef * (lp - delayed);
            const float hpOut = hpCoef * (hp + lp - hpX);
            hpX = lp; hp = hpOut;
            buffer[(size_t) pos] = std::tanh ((data[i] + hpOut * fb) * 0.9f) / 0.9f;
            pos = (pos + 1) % size;

            data[i] += delayed * params.mix;
        }
    }

private:
    double sr = 44100.0;
    EchoParams params;
    std::vector<float> buffer;
    int pos = 0;
    float lp = 0.0f, hp = 0.0f, hpX = 0.0f;
    juce::SmoothedValue<double> delaySmooth;
};

} // namespace krang
