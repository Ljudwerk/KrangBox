// KrangEngine – kopplar ihop kammare och Ångledningen:
// kammare -> Krangdistortion -> Ångfilter -> DC-spärr/master -> Katedralreverb (stereo) -> limiter.
// Delas av pluginet och KrangRender så att lyssningstest låter exakt som pluginet.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Basmalen.h"
#include "Hammarverket.h"
#include "Bordunorgeln.h"
#include "Spolslingan.h"
#include "Slingan.h"
#include "KrangDistortion.h"
#include "Stegverket.h"
#include "Angledningen.h"

namespace krang
{

// Vilken kammare (ljudmotor) som spelar. Fler tillkommer i fas 3.
enum class Chamber { Basmalen = 0, Hammarverket, Bordunorgeln, Spolslingan };

struct EngineParams
{
    Chamber chamber = Chamber::Basmalen;
    BasmalenParams basmalen;
    HammarverketParams hammarverket;
    BordunorgelnParams bordunorgeln;
    SpolslinganParams spolslingan;
    SlinganSettings slingan;
    StegverketSettings stegverket;
    DistortionParams distortion;
    FxFilterParams filter;
    ReverbParams reverb;
    float masterDb = 0.0f;
    float krangspaken = 0.0f; // 0..1, redan inräknad i fälten ovan av applyKrangspaken()
};

// Krangspaken: en makrospak som drar hela maskinen mot sönderfall på en gång.
// Ändrar de vanliga parametrarna i stället för att ha egen DSP, så att allt den gör syns
// och hörs som vanliga inställningar. Nivån kompenseras så att spaken inte bara blir en volymratt.
inline void applyKrangspaken (EngineParams& e, float k)
{
    k = juce::jlimit (0.0f, 1.0f, k);
    e.krangspaken = k;
    if (k <= 0.0f) return;

    auto toward = [k] (float value, float target, float amount = 1.0f) { return value + (target - value) * k * amount; };

    auto& d = e.distortion;
    d.driveDb  = juce::jmin (60.0f, d.driveDb + 24.0f * k);
    d.mix      = toward (d.mix, 1.0f);
    d.bits     = toward (d.bits, 3.0f, 0.7f);
    d.downsample = toward (d.downsample, 12.0f, 0.7f);
    d.outputDb -= 6.0f * k;

    auto& b = e.basmalen;
    b.drift     = b.drift + 2.0f * k;               // upp till ±75 cent: maskinen tappar stämningen
    b.resonance = toward (b.resonance, 0.9f, 0.6f);
    b.filterDrive = toward (b.filterDrive, 4.0f);

    e.hammarverket.rost = toward (e.hammarverket.rost, 1.0f);

    e.bordunorgeln.drift = e.bordunorgeln.drift + 2.0f * k;
    e.bordunorgeln.bow   = toward (e.bordunorgeln.bow, 1.0f, 0.6f);

    e.spolslingan.index  = toward (e.spolslingan.index, 10.0f, 0.6f);
    e.spolslingan.grain  = toward (e.spolslingan.grain, 0.6f, 0.8f);
    e.spolslingan.spread = toward (e.spolslingan.spread, 1.0f);

    e.reverb.mix  = juce::jmax (e.reverb.mix, toward (e.reverb.mix, 0.45f, 0.6f));
    e.reverb.dark = toward (e.reverb.dark, 0.8f, 0.5f);
}

class KrangEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        basmalen.prepare (sampleRate, maxBlockSize);
        hammarverket.prepare (sampleRate, maxBlockSize);
        bordunorgeln.prepare (sampleRate, maxBlockSize);
        spolslingan.prepare (sampleRate, maxBlockSize);
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
        bordunorgeln.reset();
        spolslingan.reset();
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
            bordunorgeln.allNotesOff();
            spolslingan.allNotesOff();
            chamber = p.chamber;
        }
        basmalen.setParams (p.basmalen);
        hammarverket.setParams (p.hammarverket);
        bordunorgeln.setParams (p.bordunorgeln);
        spolslingan.setParams (p.spolslingan);
        distortion.setParams (p.distortion);
        filter.setParams (p.filter);
        reverb.setParams (p.reverb);
        masterGain = juce::Decibels::decibelsToGain (p.masterDb);
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
        {
            switch (chamber)
            {
                case Chamber::Hammarverket: hammarverket.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Bordunorgeln: bordunorgeln.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Spolslingan:  spolslingan.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Basmalen:     basmalen.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
            }
        }
        else if (m.isNoteOff())
        {
            switch (chamber)
            {
                case Chamber::Hammarverket: hammarverket.noteOff (m.getNoteNumber()); break;
                case Chamber::Bordunorgeln: bordunorgeln.noteOff (m.getNoteNumber()); break;
                case Chamber::Spolslingan:  spolslingan.noteOff (m.getNoteNumber()); break;
                case Chamber::Basmalen:     basmalen.noteOff (m.getNoteNumber()); break;
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            basmalen.allNotesOff();
            hammarverket.allNotesOff();
            bordunorgeln.allNotesOff();
            spolslingan.allNotesOff();
        }
        else if (m.isPitchWheel())
        {
            basmalen.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
        }
    }

    // Renderar stereo: kammaren och distortion/filter i mono i left, reverben sprider till left/right.
    void render (float* left, float* right, int numSamples)
    {
        switch (chamber)
        {
            case Chamber::Hammarverket: hammarverket.render (left, numSamples); break;
            case Chamber::Bordunorgeln: bordunorgeln.render (left, numSamples); break;
            case Chamber::Spolslingan:  spolslingan.render (left, numSamples); break;
            case Chamber::Basmalen:     basmalen.render (left, numSamples); break;
        }
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
    Bordunorgeln bordunorgeln;
    Spolslingan spolslingan;
    KrangDistortion distortion;
    AngFilter filter;
    Katedralreverb reverb;
    juce::dsp::Limiter<float> limiter;
    float masterGain = 1.0f;
    float dcCoef = 0.999f, dcX = 0.0f, dcY = 0.0f;
};

} // namespace krang
