// KrangEngine – kopplar ihop kammare och Ångledningen:
// kammare -> Krangdistortion -> Ångfilter -> DC-spärr/master -> Katedralreverb (stereo) -> limiter.
// Delas av pluginet och KrangRender så att lyssningstest låter exakt som pluginet.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Basmalen.h"
#include "Hammarverket.h"
#include "KrangDistortion.h"
#include "Stegverket.h"
#include "Angledningen.h"

namespace krang
{

// Vilken kammare (ljudmotor) som spelar. Fler tillkommer i fas 3.
enum class Chamber { Basmalen = 0, Hammarverket };

struct EngineParams
{
    Chamber chamber = Chamber::Basmalen;
    BasmalenParams basmalen;
    HammarverketParams hammarverket;
    StegverketSettings stegverket;
    DistortionParams distortion;
    FxFilterParams filter;
    ReverbParams reverb;
    float masterDb = 0.0f;
};

class KrangEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        basmalen.prepare (sampleRate, maxBlockSize);
        hammarverket.prepare (sampleRate, maxBlockSize);
        distortion.prepare (sampleRate, maxBlockSize);
        filter.prepare (sampleRate, maxBlockSize);
        reverb.prepare (sampleRate, maxBlockSize);
        limiter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 2 });
        limiter.setThreshold (-0.5f);
        limiter.setRelease (80.0f);
        // DC-spärr (~8 Hz): rörläget förspänner signalen och skapar likspänning.
        dcCoef = (float) std::exp (-juce::MathConstants<double>::twoPi * 8.0 / sampleRate);
    }

    void reset()
    {
        basmalen.reset();
        hammarverket.reset();
        distortion.reset();
        filter.reset();
        reverb.reset();
        limiter.reset();
        dcX = dcY = 0.0f;
    }

    void setParams (const EngineParams& p)
    {
        if (p.chamber != chamber)
        {
            // Byte av kammare: tysta den gamla så att inga toner hänger kvar.
            basmalen.allNotesOff();
            hammarverket.allNotesOff();
            chamber = p.chamber;
        }
        basmalen.setParams (p.basmalen);
        hammarverket.setParams (p.hammarverket);
        distortion.setParams (p.distortion);
        filter.setParams (p.filter);
        reverb.setParams (p.reverb);
        masterGain = juce::Decibels::decibelsToGain (p.masterDb);
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        const bool drums = chamber == Chamber::Hammarverket;
        if (m.isNoteOn())
        {
            if (drums) hammarverket.noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else       basmalen.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        }
        else if (m.isNoteOff())
        {
            if (drums) hammarverket.noteOff (m.getNoteNumber());
            else       basmalen.noteOff (m.getNoteNumber());
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            basmalen.allNotesOff();
            hammarverket.allNotesOff();
        }
        else if (m.isPitchWheel())
        {
            basmalen.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
        }
    }

    // Renderar stereo: kammaren och distortion/filter i mono i left, reverben sprider till left/right.
    void render (float* left, float* right, int numSamples)
    {
        if (chamber == Chamber::Hammarverket) hammarverket.render (left, numSamples);
        else                                  basmalen.render (left, numSamples);
        distortion.process (left, numSamples);
        filter.process (left, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = left[i];
            dcY = x - dcX + dcCoef * dcY;
            dcX = x;
            left[i] = dcY * masterGain;
        }

        reverb.process (left, left, right, numSamples);

        float* chans[] = { left, right };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) numSamples);
        limiter.process (juce::dsp::ProcessContextReplacing<float> (block));

        // Säkerhetstak: limitern saknar framförhållning och kan släppa igenom enstaka toppar.
        constexpr float ceiling = 0.966f; // -0,3 dBFS
        juce::FloatVectorOperations::clip (left, left, -ceiling, ceiling, numSamples);
        juce::FloatVectorOperations::clip (right, right, -ceiling, ceiling, numSamples);
    }

    int getLatencyInSamples() const { return distortion.getLatencyInSamples(); }

private:
    Chamber chamber = Chamber::Basmalen;
    Basmalen basmalen;
    Hammarverket hammarverket;
    KrangDistortion distortion;
    AngFilter filter;
    Katedralreverb reverb;
    juce::dsp::Limiter<float> limiter;
    float masterGain = 1.0f;
    float dcCoef = 0.999f, dcX = 0.0f, dcY = 0.0f;
};

} // namespace krang
