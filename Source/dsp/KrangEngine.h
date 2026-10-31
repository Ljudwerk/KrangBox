// KrangEngine – kopplar ihop kammare och Ångledningen.
// Delas av pluginet och KrangRender så att lyssningstest låter exakt som pluginet.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Basmalen.h"
#include "Hammarverket.h"
#include "KrangDistortion.h"

namespace krang
{

// Vilken kammare (ljudmotor) som spelar. Fler tillkommer i fas 3.
enum class Chamber { Basmalen = 0, Hammarverket };

struct EngineParams
{
    Chamber chamber = Chamber::Basmalen;
    BasmalenParams basmalen;
    HammarverketParams hammarverket;
    DistortionParams distortion;
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
        limiter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
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

    // Renderar mono till data[0..numSamples).
    void render (float* data, int numSamples)
    {
        if (chamber == Chamber::Hammarverket) hammarverket.render (data, numSamples);
        else                                  basmalen.render (data, numSamples);
        distortion.process (data, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = data[i];
            dcY = x - dcX + dcCoef * dcY;
            dcX = x;
            data[i] = dcY * masterGain;
        }

        float* chans[] = { data };
        juce::dsp::AudioBlock<float> block (chans, 1, (size_t) numSamples);
        limiter.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    int getLatencyInSamples() const { return distortion.getLatencyInSamples(); }

private:
    Chamber chamber = Chamber::Basmalen;
    Basmalen basmalen;
    Hammarverket hammarverket;
    KrangDistortion distortion;
    juce::dsp::Limiter<float> limiter;
    float masterGain = 1.0f;
    float dcCoef = 0.999f, dcX = 0.0f, dcY = 0.0f;
};

} // namespace krang
