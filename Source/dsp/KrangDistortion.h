// Krangdistortion – första modulen i Ångledningen.
// Fyra lägen: Rör, Fuzz, Vikning (wavefolder) och Bitkross.
// Rör/Fuzz/Vikning körs 4x översamplat för att hålla aliasing nere;
// Bitkross körs avsiktligt utan översampling – där ÄR aliasingen ljudet.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace krang
{

enum class DistMode { Ror = 0, Fuzz, Vikning, Bitkross };

struct DistortionParams
{
    DistMode mode     = DistMode::Ror;
    float driveDb     = 18.0f; // 0..48
    float mix         = 1.0f;  // 0..1
    float bits        = 6.0f;  // 1..16   (Bitkross)
    float downsample  = 4.0f;  // 1..32   (Bitkross)
    float outputDb    = -6.0f;
};

class KrangDistortion
{
public:
    KrangDistortion()
        : oversampler (1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true)
    {}

    void prepare (double sampleRate, int maxBlockSize)
    {
        oversampler.initProcessing ((size_t) maxBlockSize);
        latency = (int) std::lround (oversampler.getLatencyInSamples());

        dryDelay.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
        dryDelay.setMaximumDelayInSamples (juce::jmax (1, latency + 1));
        dryDelay.setDelay ((float) latency);

        dry.setSize (1, maxBlockSize);
        driveSmooth.reset (sampleRate, 0.03);
        mixSmooth.reset (sampleRate, 0.03);
        reset();
    }

    void reset()
    {
        oversampler.reset();
        dryDelay.reset();
        holdCounter = 0;
        holdValue = 0.0f;
    }

    void setParams (const DistortionParams& p)
    {
        params = p;
        driveSmooth.setTargetValue (juce::Decibels::decibelsToGain (p.driveDb));
        mixSmooth.setTargetValue (p.mix);
    }

    int getLatencyInSamples() const { return latency; }

    void process (float* data, int numSamples)
    {
        // Spara torr signal, fördröjd lika mycket som översamplingen.
        float* d = dry.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
        {
            dryDelay.pushSample (0, data[i]);
            d[i] = dryDelay.popSample (0);
        }

        float* chans[] = { data };
        juce::dsp::AudioBlock<float> block (chans, 1, (size_t) numSamples);

        // Driven ändras mjukt men vi läser den per block i översamplat läge.
        const float drive = driveSmooth.skip (numSamples);

        if (params.mode == DistMode::Bitkross)
        {
            processBitcrush (data, numSamples, drive);
            // Håll latensen konstant mellan lägen: fördröj våt signal lika mycket.
            auto up = oversampler.processSamplesUp (block);
            juce::ignoreUnused (up);
            oversampler.processSamplesDown (block);
        }
        else
        {
            auto up = oversampler.processSamplesUp (block);
            float* u = up.getChannelPointer (0);
            const auto n = (int) up.getNumSamples();

            switch (params.mode)
            {
                case DistMode::Ror:
                {
                    // Asymmetrisk tanh: förspänning ger jämna övertoner som ett överstyrt rör.
                    constexpr float bias = 0.25f;
                    const float bOff = std::tanh (bias);
                    const float comp = 1.0f / std::tanh (juce::jmax (1.0f, drive * 0.5f));
                    for (int i = 0; i < n; ++i)
                        u[i] = (std::tanh (u[i] * drive + bias) - bOff) * comp;
                    break;
                }
                case DistMode::Fuzz:
                {
                    // Hård, asymmetrisk klippning – trasig transistorfuzz.
                    for (int i = 0; i < n; ++i)
                        u[i] = juce::jlimit (-1.0f, 0.75f, u[i] * drive);
                    break;
                }
                case DistMode::Vikning:
                {
                    // Sinusvikare: signalen viks tillbaka i stället för att klippas.
                    const float g = drive * juce::MathConstants<float>::halfPi * 0.5f;
                    for (int i = 0; i < n; ++i)
                        u[i] = std::sin (u[i] * g);
                    break;
                }
                case DistMode::Bitkross: break;
            }

            oversampler.processSamplesDown (block);
        }

        // Blanda och sätt utnivå.
        const float out = juce::Decibels::decibelsToGain (params.outputDb);
        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mixSmooth.getNextValue();
            data[i] = (d[i] * (1.0f - m) + data[i] * m) * out;
        }
    }

private:
    void processBitcrush (float* data, int numSamples, float drive)
    {
        const int hold = juce::jmax (1, (int) params.downsample);
        const float levels = std::pow (2.0f, juce::jlimit (1.0f, 16.0f, params.bits) - 1.0f);
        const float pre = std::sqrt (drive); // mildare drive: bitkrossen ska knastra, inte bara klippa

        for (int i = 0; i < numSamples; ++i)
        {
            if (holdCounter++ % hold == 0)
            {
                const float x = juce::jlimit (-1.0f, 1.0f, data[i] * pre);
                holdValue = std::round (x * levels) / levels;
            }
            data[i] = holdValue;
        }
    }

    DistortionParams params;
    juce::dsp::Oversampling<float> oversampler;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 64 };
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveSmooth { 1.0f };
    juce::SmoothedValue<float> mixSmooth { 1.0f };
    int latency = 0;
    int holdCounter = 0;
    float holdValue = 0.0f;
};

} // namespace krang
