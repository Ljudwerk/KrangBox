// Bordunorgeln – KrangBox drönmotor (fas 3).
// Polyfon (8 röster) vågtabellssynt vars klangfärg glider från vallflöjt via säckpipa till nyckelharpa,
// med stråkbrus, en bordunton som ligger kvar under allt man spelar, och resonanssträngar:
// en bank av sympatiska strängar (fysiskt modellerade som återkopplade fördröjningar) som klingar med,
// som understrängarna på en nyckelharpa. Helt syntetiskt.
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

struct BordunorgelnParams
{
    float timbre    = 0.5f;   // 0 = vallflöjt, 0.5 = säckpipa, 1 = nyckelharpa
    float bow       = 0.3f;   // stråk-/andningsbrus 0..1
    float drone     = 0.5f;   // bordunens nivå 0..1
    int droneNote   = 2;      // 0 = C ... 11 = H (standard D)
    float sympathetic = 0.4f; // resonanssträngarnas nivå 0..1
    float drift     = 0.3f;   // långsam ostämdhet 0..1
    float attackSec = 0.25f;
    float releaseSec = 1.2f;
};

// Bandbegränsade vågtabeller: 33 klangfärgssteg × 10 oktaver, byggs en gång.
class BordunTables
{
public:
    static constexpr int size = 2048, timbres = 33, octaves = 10;

    static const BordunTables& get()
    {
        static const BordunTables instance;
        return instance;
    }

    // Läser vågformen med linjär interpolation, för klangfärg t (0..1) och grundton hz.
    float read (double phase, float t, double hz) const
    {
        const int oct = juce::jlimit (0, octaves - 1, (int) std::floor (std::log2 (juce::jmax (1.0, hz / 27.5))));
        const float tp = juce::jlimit (0.0f, 1.0f, t) * (float) (timbres - 1);
        const int t0 = juce::jmin (timbres - 2, (int) tp);
        const float tf = tp - (float) t0;
        const double pos = phase * size;
        const int i0 = (int) pos & (size - 1), i1 = (i0 + 1) & (size - 1);
        const float f = (float) (pos - std::floor (pos));
        const float* a = table (t0, oct);
        const float* b = table (t0 + 1, oct);
        const float va = a[i0] + f * (a[i1] - a[i0]);
        const float vb = b[i0] + f * (b[i1] - b[i0]);
        return va + tf * (vb - va);
    }

private:
    BordunTables() : data ((size_t) (size * timbres * octaves))
    {
        for (int t = 0; t < timbres; ++t)
            for (int o = 0; o < octaves; ++o)
                build (t, o);
    }

    const float* table (int t, int o) const { return data.data() + (size_t) ((t * octaves + o) * size); }

    // Spektrum för klangfärg t: tre ankare som tonas mellan.
    static float amplitude (int n, float t)
    {
        const float fn = (float) n;
        // Vallflöjt: nästan bara grundton och udda övertoner, snabbt avtagande.
        const float flute = (n % 2 == 1 ? 1.0f : 0.25f) / (fn * fn);
        // Säckpipa: nasal rörbladston, starka mellanövertoner (formant runt deltoner 3–7).
        const float pipe = (1.0f / fn) * (1.0f + 1.8f * std::exp (-0.5f * (fn - 5.0f) * (fn - 5.0f) / 4.0f));
        // Nyckelharpa: stråkton (sågtand) med kroppsresonanser runt deltoner 3 och 9–11.
        const float harpa = (1.0f / fn) * (1.0f + 0.9f * std::exp (-0.5f * (fn - 3.0f) * (fn - 3.0f))
                                             + 0.7f * std::exp (-0.5f * (fn - 10.0f) * (fn - 10.0f) / 3.0f));
        if (t < 0.5f) return flute + (pipe - flute) * (t * 2.0f);
        return pipe + (harpa - pipe) * ((t - 0.5f) * 2.0f);
    }

    void build (int t, int o)
    {
        float* out = data.data() + (size_t) ((t * octaves + o) * size);
        const float timbre = (float) t / (float) (timbres - 1);
        const double topHz = 27.5 * std::pow (2.0, o + 1);
        const int maxPartial = juce::jlimit (1, 64, (int) (20000.0 / topHz));
        std::fill (out, out + size, 0.0f);
        for (int n = 1; n <= maxPartial; ++n)
        {
            const float a = amplitude (n, timbre);
            // Rotera en fasvektor i stället för att anropa sin() för varje sampel.
            const double w = juce::MathConstants<double>::twoPi * n / size;
            double c = 1.0, s = 0.0;
            const double cw = std::cos (w), sw = std::sin (w);
            for (int i = 0; i < size; ++i)
            {
                out[i] += a * (float) s;
                const double nc = c * cw - s * sw;
                s = s * cw + c * sw;
                c = nc;
            }
        }
        float peak = 0.0f;
        for (int i = 0; i < size; ++i) peak = juce::jmax (peak, std::abs (out[i]));
        if (peak > 0.0f)
            for (int i = 0; i < size; ++i) out[i] /= peak;
    }

    std::vector<float> data;
};

class Bordunorgeln
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    static constexpr int numVoices = 8;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        juce::ignoreUnused (BordunTables::get()); // bygg tabellerna nu, inte i ljudtråden
        for (auto& v : voices)
        {
            v.env.setSampleRate (sampleRate);
            v.bowFilter.prepare ({ sampleRate, 64, 1 });
            v.bowFilter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            v.bowFilter.setResonance (5.0f); // smalt band: brus som låter som stråke, inte som sus
        }
        droneEnv.setSampleRate (sampleRate);
        for (auto& s : strings)
            s.buffer.assign ((size_t) (sampleRate / 40.0) + 4, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) { v.env.reset(); v.note = -1; v.phase = 0.0; v.bowFilter.reset(); }
        droneEnv.reset();
        dronePhase[0] = dronePhase[1] = 0.0;
        for (auto& s : strings) { std::fill (s.buffer.begin(), s.buffer.end(), 0.0f); s.pos = 0; s.lp = 0.0f; }
        droneOn = false;
        for (auto& v : voices) v.keyDown = false;
    }

    void setParams (const BordunorgelnParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.002f, p.attackSec), 0.6f, 0.85f, juce::jmax (0.01f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
        droneEnv.setParameters ({ 0.8f, 0.1f, 1.0f, juce::jmax (0.3f, p.releaseSec * 1.5f) });

        if (p.droneNote != tunedDrone || stamning->getVersion() != tunedStamning)
            tuneStrings (p.droneNote);
    }

    void noteOn (int note, float velocity)
    {
        // Samma ton igen: återanvänd rösten. Annars ledig röst, annars den äldsta.
        Voice* target = nullptr;
        for (auto& v : voices) if (v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.env.isActive()) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        target->note = note;
        target->velocity = 0.4f + 0.6f * velocity;
        target->age = ++ageCounter;
        target->driftValue = 0.0f;
        target->env.noteOn();
        target->keyDown = true;
        updateDrone();
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.note == note && v.keyDown)
            {
                v.env.noteOff();
                v.keyDown = false;
            }
        updateDrone();
    }

    void allNotesOff()
    {
        for (auto& v : voices) { v.env.noteOff(); v.keyDown = false; }
        updateDrone();
    }

    void render (float* dest, int numSamples)
    {
        const auto& tables = BordunTables::get();
        const float t = params.timbre;
        const double droneHz = midiHz (36 + params.droneNote); // bordunen ligger i stora oktaven

        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (auto& v : voices)
            {
                if (! v.env.isActive()) continue;
                const float env = v.env.getNextSample();

                // Långsam drift: slumpvandring, högst ±15 cent vid drift = 1 (mer med Krangspaken).
                if ((++v.driftCounter & 255) == 0)
                    v.driftTarget = (random.nextFloat() * 2.0f - 1.0f) * params.drift * 0.15f;
                v.driftValue += (v.driftTarget - v.driftValue) * 0.0005f;

                const double hz = midiHz (v.note) * std::exp2 (v.driftValue / 12.0);
                v.phase += hz / sr;
                v.phase -= std::floor (v.phase);
                float s = tables.read (v.phase, t, hz);

                // Stråk-/andningsbrus kring tredje deltonen, följer envelopen.
                if (params.bow > 0.001f)
                {
                    if ((v.driftCounter & 31) == 0)
                        v.bowFilter.setCutoffFrequency ((float) juce::jmin (sr * 0.4, hz * 3.0));
                    s += v.bowFilter.processSample (0, random.nextFloat() * 2.0f - 1.0f) * params.bow * 0.45f;
                }
                out += s * env * v.velocity;
            }

            // Bordun: grundton och kvint i stora oktaven, ligger kvar så länge något spelas.
            const float dEnv = droneEnv.getNextSample();
            if (dEnv > 0.0f && params.drone > 0.001f)
            {
                float d = 0.0f;
                const double hzs[2] = { droneHz, droneHz * 1.5 };
                for (int k = 0; k < 2; ++k)
                {
                    dronePhase[k] += hzs[k] / sr;
                    dronePhase[k] -= std::floor (dronePhase[k]);
                    d += tables.read (dronePhase[k], t, hzs[k]) * (k == 0 ? 1.0f : 0.6f);
                }
                out += d * dEnv * params.drone * 0.6f;
            }

            // Resonanssträngar: exciteras av allt som spelas och klingar med.
            float sym = 0.0f;
            if (params.sympathetic > 0.001f)
            {
                const float exc = out * 0.02f;
                for (auto& st : strings)
                {
                    const int size = (int) st.buffer.size();
                    int r = st.pos - st.delay;
                    if (r < 0) r += size;
                    const float y = st.buffer[(size_t) r];
                    st.lp += (y - st.lp) * 0.5f;                 // dämpning: diskanten dör först
                    st.buffer[(size_t) st.pos] = exc + st.lp * 0.996f;
                    st.pos = (st.pos + 1) % size;
                    sym += y;
                }
                sym *= params.sympathetic * 1.4f;
            }

            dest[i] = (out * 0.22f) + sym;
        }
    }

private:
    // Bordunen ligger kvar så länge minst en tangent hålls nere.
    void updateDrone()
    {
        bool any = false;
        for (auto& v : voices) any = any || v.keyDown;
        if (any && ! droneOn) droneEnv.noteOn();
        if (! any && droneOn) droneEnv.noteOff();
        droneOn = any;
    }

    double midiHz (double note) const { return stamning->hz (note); }

    // Stämmer resonansssträngarna efter bordunen: grundton, kvint, oktav, ters, kvart, oktav+kvint.
    void tuneStrings (int droneNote)
    {
        if (strings[0].buffer.size() < 8) return; // inte förberedd än
        tunedDrone = droneNote;
        tunedStamning = stamning->getVersion();
        static constexpr int intervals[numStrings] = { 12, 19, 24, 27, 29, 31 };
        for (int k = 0; k < numStrings; ++k)
        {
            const double hz = midiHz (48 + droneNote + intervals[k] - 12);
            strings[(size_t) k].delay = juce::jlimit (2, (int) strings[(size_t) k].buffer.size() - 2, (int) std::round (sr / hz));
        }
    }

    struct Voice
    {
        juce::ADSR env;
        juce::dsp::StateVariableTPTFilter<float> bowFilter;
        int note = -1;
        bool keyDown = false;
        float velocity = 1.0f;
        double phase = 0.0;
        juce::uint32 age = 0, driftCounter = 0;
        float driftValue = 0.0f, driftTarget = 0.0f;
    };

    struct SympatheticString
    {
        std::vector<float> buffer;
        int pos = 0, delay = 100;
        float lp = 0.0f;
    };

    static constexpr int numStrings = 6;

    double sr = 44100.0;
    BordunorgelnParams params;
    std::array<Voice, numVoices> voices;
    std::array<SympatheticString, numStrings> strings;
    juce::ADSR droneEnv;
    double dronePhase[2] {};
    int tunedDrone = -1, tunedStamning = -1;
    bool droneOn = false;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x424f5244 }; // "BORD"
};

} // namespace krang
