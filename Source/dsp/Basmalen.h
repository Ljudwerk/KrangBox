// Basmålen – KrangBox basmotor (fas 0-prototyp)
// Två bandbegränsade oscillatorer + sub, ladder-filter, två envelopes,
// legato-glid ("ångvisslande") och långsam tonhöjdsdrift.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Stamning.h"
#include <array>
#include <cmath>

namespace krang
{

struct BasmalenParams
{
    float osc1Level   = 0.8f;   // sågtand
    float osc2Level   = 0.5f;   // fyrkant
    float osc2Semi    = -12.0f; // halvtoner
    float osc2Fine    = 7.0f;   // cent
    float subLevel    = 0.6f;   // sinus en oktav ned
    float glideMs     = 60.0f;
    float drift       = 0.3f;   // 0..1, upp till ±25 cent

    float cutoffHz    = 400.0f;
    float resonance   = 0.45f;  // 0..1
    float filterDrive = 1.5f;   // 1..4
    float envAmount   = 3.0f;   // oktaver, -4..+6
    float keyTrack    = 0.5f;   // 0..1

    juce::ADSR::Parameters ampEnv    { 0.003f, 0.25f, 0.8f, 0.12f };
    juce::ADSR::Parameters filterEnv { 0.002f, 0.30f, 0.15f, 0.20f };
};

class Basmalen
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        filter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
        filter.setMode (juce::dsp::LadderFilterMode::LPF24);
        ampEnv.setSampleRate (sampleRate);
        filterEnv.setSampleRate (sampleRate);
        reset();
    }

    void reset()
    {
        numHeld = 0;
        ph1 = ph2 = phSub = 0.0;
        ampEnv.reset();
        filterEnv.reset();
        filter.reset();
        driftValue = driftTarget = 0.0f;
    }

    void setParams (const BasmalenParams& p)
    {
        params = p;
        ampEnv.setParameters (p.ampEnv);
        filterEnv.setParameters (p.filterEnv);
        filter.setResonance (juce::jlimit (0.0f, 0.98f, p.resonance));
        filter.setDrive (juce::jmax (1.0f, p.filterDrive));
    }

    void setPitchBend (float semitones) { bendSemis = semitones; }

    void noteOn (int note, float vel)
    {
        removeHeld (note);
        if (numHeld < (int) held.size())
            held[(size_t) numHeld++] = note;

        targetNote = (double) note;
        const bool legato = ampEnv.isActive() && numHeld > 1;

        if (! legato)
        {
            currentNote = targetNote;   // nytt anslag: hoppa direkt, trigga om
            velocity = vel;
            ampEnv.noteOn();
            filterEnv.noteOn();
        }
    }

    void noteOff (int note)
    {
        removeHeld (note);
        if (numHeld > 0)
            targetNote = (double) held[(size_t) numHeld - 1]; // glid tillbaka till föregående ton
        else
        {
            ampEnv.noteOff();
            filterEnv.noteOff();
        }
    }

    void allNotesOff()
    {
        numHeld = 0;
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    // Renderar mono in i dest (adderar inte – skriver över).
    void render (float* dest, int numSamples)
    {
        const double glideCoef = params.glideMs <= 0.5f
            ? 1.0
            : 1.0 - std::exp (-1.0 / (params.glideMs * 0.001 * sr));

        int done = 0;
        while (done < numSamples)
        {
            const int n = juce::jmin (controlRate, numSamples - done);
            float* out = dest + done;

            // --- Drift: långsam slumpvandring, uppdateras per kontrollblock
            if (++driftCounter >= driftSteps)
            {
                driftCounter = 0;
                driftTarget = (random.nextFloat() * 2.0f - 1.0f);
            }
            driftValue += (driftTarget - driftValue) * 0.01f;
            const double driftSemis = driftValue * params.drift * 0.25;

            // --- Oscillatorer
            for (int i = 0; i < n; ++i)
            {
                currentNote += (targetNote - currentNote) * glideCoef;
                const double baseNote = currentNote + bendSemis + driftSemis;

                const double f1  = noteToHz (baseNote);
                const double f2  = noteToHz (baseNote + params.osc2Semi) * std::exp2 (params.osc2Fine * 0.01 / 12.0);
                const double fs  = f1 * 0.5;
                const double dt1 = f1 / sr, dt2 = f2 / sr, dts = fs / sr;

                const float saw = (float) (2.0 * ph1 - 1.0 - polyBlep (ph1, dt1));
                double sq = ph2 < 0.5 ? 1.0 : -1.0;
                sq += polyBlep (ph2, dt2);
                sq -= polyBlep (std::fmod (ph2 + 0.5, 1.0), dt2);
                const float sub = (float) std::sin (juce::MathConstants<double>::twoPi * phSub);

                out[i] = params.osc1Level * saw
                       + params.osc2Level * (float) sq
                       + params.subLevel * sub;

                ph1 += dt1;   if (ph1 >= 1.0) ph1 -= 1.0;
                ph2 += dt2;   if (ph2 >= 1.0) ph2 -= 1.0;
                phSub += dts; if (phSub >= 1.0) phSub -= 1.0;
            }

            // --- Filter: cutoff modulerad av envelope och tangentföljning
            float fEnv = 0.0f;
            for (int i = 0; i < n; ++i)
                fEnv = filterEnv.getNextSample();

            const double keyOct = (currentNote - 48.0) / 12.0 * params.keyTrack;
            const double cutoff = params.cutoffHz * std::pow (2.0, params.envAmount * fEnv + keyOct);
            filter.setCutoffFrequencyHz ((float) juce::jlimit (20.0, sr * 0.45, cutoff));

            float* chans[] = { out };
            juce::dsp::AudioBlock<float> block (chans, 1, (size_t) n);
            filter.process (juce::dsp::ProcessContextReplacing<float> (block));

            // --- Förstärkare
            const float velGain = 0.5f + 0.5f * velocity;
            for (int i = 0; i < n; ++i)
                out[i] *= ampEnv.getNextSample() * velGain * 0.4f;

            done += n;
        }
    }

    bool isActive() const { return ampEnv.isActive(); }

private:
    double noteToHz (double note) const { return stamning->hz (note); }

    static double polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int r = 0; r < numHeld; ++r)
            if (held[(size_t) r] != note)
                held[(size_t) w++] = held[(size_t) r];
        numHeld = w;
    }

    static constexpr int controlRate = 16;   // samples per filteruppdatering
    static constexpr int driftSteps  = 2000; // kontrollblock mellan nya driftmål

    double sr = 44100.0;
    BasmalenParams params;

    std::array<int, 16> held {};
    int numHeld = 0;
    double targetNote = 48.0, currentNote = 48.0;
    float velocity = 1.0f, bendSemis = 0.0f;

    double ph1 = 0.0, ph2 = 0.0, phSub = 0.0;
    float driftValue = 0.0f, driftTarget = 0.0f;
    int driftCounter = 0;
    juce::Random random { 0x4b52414e47 }; // "KRANG"

    juce::dsp::LadderFilter<float> filter;
    juce::ADSR ampEnv, filterEnv;
};

} // namespace krang
