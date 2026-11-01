// Kören i schaktet – KrangBox sjunde kammare: syntetisk kör, psalmsång och kulning (fas 3b).
//
// Formantsyntes, helt syntetiskt. Varje ton sjungs av flera sångare (1–8):
//   Röstkällan: en mjuk pulsvåg (stämbanden), med andning ovanpå.
//   Varje sångare är lite ostämd, har eget vibrato, egen långsam drift och börjar lite senare än de andra –
//   det är det som gör en kör till en kör och inte en orgel.
//   Formanterna: fem resonanser per röstläge (mans- och kvinnoröster) som formar vokalen A, E, I, O eller U.
//   Kulning: en ensam, hög röst som glider upp i tonen och faller av i slutet, som när man lockar på korna.
// Efter rösterna: Högtalaren – kören hörs genom en trasig högtalare i ett schakt: smal, skorrande och
// med glapp i kontakten.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace krang
{

enum class ChoirVoices { Mansroster = 0, Kvinnoroster, Blandad, Kulning };

struct KorenParams
{
    ChoirVoices voices = ChoirVoices::Blandad;
    float vowel      = 0.0f;   // 0 = A, 0.25 = E, 0.5 = I, 0.75 = O, 1 = U (glider mellan dem)
    int singers      = 6;      // sångare per ton, 1..8
    float spreadCents = 8.0f;  // hur ostämda och osynkade sångarna är, 0..30 cent
    float vibrato    = 0.3f;   // 0..1
    float breath     = 0.2f;   // andning 0..1
    float speaker    = 0.0f;   // trasig högtalare 0..1
    float attackSec  = 0.4f;
    float releaseSec = 1.2f;
};

class Koren
{
public:
    static constexpr int numVoices = 8, maxSingers = 8, numFormants = 5;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices)
        {
            v.env.setSampleRate (sampleRate);
            for (auto& bank : v.formants)
                for (auto& f : bank)
                {
                    f.prepare ({ sampleRate, 64, 1 });
                    f.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
                }
        }
        speakerHp.prepare ({ sampleRate, 64, 1 });
        speakerHp.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        speakerHp.setCutoffFrequency (350.0f);
        speakerLp.prepare ({ sampleRate, 64, 1 });
        speakerLp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        speakerLp.setCutoffFrequency (3200.0f);
        speakerLp.setResonance (1.4f);
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.env.reset(); v.note = -1; v.keyDown = false;
            for (auto& bank : v.formants) for (auto& f : bank) f.reset();
        }
        speakerHp.reset(); speakerLp.reset();
        crackleGain = 1.0f; crackleTarget = 1.0f; crackleTimer = 0;
    }

    void setParams (const KorenParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.005f, p.attackSec), 0.3f, 1.0f, juce::jmax (0.02f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
    }

    void noteOn (int note, float velocity)
    {
        if (params.voices == ChoirVoices::Kulning)
        {
            // Kulning är en ensam röst: den senaste rösten glider vidare till den nya tonen i stället för att en ny börjar.
            Voice* last = nullptr;
            for (auto& v : voices)
                if (v.env.isActive() && (last == nullptr || v.age > last->age)) last = &v;
            if (last != nullptr)
            {
                for (auto& v : voices) if (&v != last && v.env.isActive()) v.env.noteOff();
                last->glideCents = juce::jlimit (-1200.0f, 1200.0f, last->glideCents + last->fallCents + (float) (last->note - note) * 100.0f);
                if (std::abs (last->glideCents) < 1.0f) last->glideCents = -1.0f;
                last->fallCents = 0.0f;
                last->note = note;
                last->velocity = 0.5f + 0.5f * velocity;
                last->age = ++ageCounter;
                if (! last->keyDown) last->env.noteOn();
                last->keyDown = true;
                return;
            }
        }

        Voice* target = nullptr;
        for (auto& v : voices) if (v.note == note && v.env.isActive()) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.env.isActive()) { target = &v; break; }
        if (target == nullptr)
            for (auto& v : voices)
                if (! v.keyDown && (target == nullptr || v.age < target->age)) target = &v;
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        const bool fresh = ! v.env.isActive();
        v.note = note;
        v.keyDown = true;
        v.velocity = 0.5f + 0.5f * velocity;
        v.age = ++ageCounter;
        v.t = 0.0;
        // Kulningen glider upp i tonen från en kvart under.
        v.glideCents = params.voices == ChoirVoices::Kulning ? -500.0f : 0.0f;
        v.fallCents = 0.0f;
        if (fresh)
            for (int s = 0; s < maxSingers; ++s)
            {
                auto& sg = v.singers[(size_t) s];
                sg.phase = random.nextDouble();
                sg.vibPhase = random.nextDouble();
                sg.drift = 0.0f; sg.driftTarget = 0.0f; sg.driftTimer = 0; sg.inc = 0.0;
                sg.lp = 0.0f;
                sg.delay = (int) (sr * 0.06 * (double) (params.spreadCents / 30.0f) * random.nextDouble()); // sångarna kommer in olika
            }
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
        if (params.speaker > 0.001f)
            renderSpeaker (dest, numSamples);
    }

private:
    struct Singer
    {
        double phase = 0.0, vibPhase = 0.0, inc = 0.0;
        float drift = 0.0f, driftTarget = 0.0f, lp = 0.0f;
        int delay = 0, driftTimer = 0;
    };

    struct Voice
    {
        juce::ADSR env;
        std::array<Singer, maxSingers> singers {};
        // Två formantbanker: [0] mansröster, [1] kvinnoröster.
        std::array<std::array<juce::dsp::StateVariableTPTFilter<float>, numFormants>, 2> formants;
        std::array<std::array<float, numFormants>, 2> formantGain {};
        int note = -1;
        bool keyDown = false;
        float velocity = 1.0f, glideCents = 0.0f, fallCents = 0.0f;
        double t = 0.0;
        juce::uint32 age = 0;
        int counter = 0;
    };

    struct Formant { float hz, gainDb, bw; };

    // Formanttabeller (Hz, nivå i dB, bandbredd i Hz) för vokalerna A, E, I, O, U.
    static const Formant& formant (int bank, int vowel, int k)
    {
        static const Formant male[5][numFormants] = {
            { { 650, 0, 80 }, { 1080, -6, 90 }, { 2650, -7, 120 }, { 2900, -8, 130 }, { 3250, -22, 140 } },
            { { 400, 0, 70 }, { 1700, -14, 80 }, { 2600, -12, 100 }, { 3200, -14, 120 }, { 3580, -20, 120 } },
            { { 290, 0, 40 }, { 1870, -15, 90 }, { 2800, -18, 100 }, { 3250, -20, 120 }, { 3540, -30, 120 } },
            { { 400, 0, 70 }, { 800, -10, 80 }, { 2600, -12, 100 }, { 2800, -12, 130 }, { 3000, -26, 135 } },
            { { 350, 0, 40 }, { 600, -20, 80 }, { 2700, -17, 100 }, { 2900, -14, 120 }, { 3300, -26, 120 } } };
        static const Formant female[5][numFormants] = {
            { { 800, 0, 80 }, { 1150, -4, 90 }, { 2800, -20, 120 }, { 3500, -36, 130 }, { 4950, -60, 140 } },
            { { 350, 0, 60 }, { 2000, -20, 100 }, { 2800, -15, 120 }, { 3600, -40, 150 }, { 4950, -56, 200 } },
            { { 270, 0, 60 }, { 2140, -12, 90 }, { 2950, -26, 100 }, { 3900, -26, 120 }, { 4950, -44, 120 } },
            { { 450, 0, 70 }, { 800, -11, 80 }, { 2830, -22, 100 }, { 3500, -22, 130 }, { 4950, -50, 135 } },
            { { 325, 0, 50 }, { 700, -16, 60 }, { 2530, -35, 170 }, { 3500, -40, 180 }, { 4950, -60, 200 } } };
        return bank == 0 ? male[vowel][k] : female[vowel][k];
    }

    void updateFormants (Voice& v)
    {
        const float pos = juce::jlimit (0.0f, 1.0f, params.vowel) * 4.0f;
        const int i0 = juce::jmin (3, (int) pos);
        const float f = pos - (float) i0;
        for (int bank = 0; bank < 2; ++bank)
            for (int k = 0; k < numFormants; ++k)
            {
                const auto& a = formant (bank, i0, k);
                const auto& b = formant (bank, i0 + 1, k);
                const float hz = a.hz + f * (b.hz - a.hz);
                const float bw = a.bw + f * (b.bw - a.bw);
                const float db = a.gainDb + f * (b.gainDb - a.gainDb);
                auto& flt = v.formants[(size_t) bank][(size_t) k];
                flt.setCutoffFrequency (juce::jmin (hz, (float) sr * 0.45f));
                const float q = juce::jlimit (0.5f, 20.0f, hz / bw * 0.5f);
                flt.setResonance (q);
                // Bandpassets topp förstärker med q; dela bort det så att formanten får sin egen nivå.
                v.formantGain[(size_t) bank][(size_t) k] = juce::Decibels::decibelsToGain (db) / q;
            }
    }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const bool kulning = params.voices == ChoirVoices::Kulning;
        const int singers = kulning ? 1 : juce::jlimit (1, maxSingers, params.singers);
        const float singerGain = 1.0f / std::sqrt ((float) singers);
        const double baseHz = 440.0 * std::pow (2.0, (v.note - 69) / 12.0);
        const float vibDepth = params.vibrato * (kulning ? 25.0f : 45.0f); // cent
        const float lpCoef = std::exp (-juce::MathConstants<float>::twoPi * 3500.0f / (float) sr); // stämbandens lutning (strålning från munnen lyfter diskanten igen)

        updateFormants (v);

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = v.env.getNextSample();

            // Kulningens glid upp, och fallet när tonen släpps.
            if (v.glideCents != 0.0f) { v.glideCents *= 0.9993f; if (std::abs (v.glideCents) < 0.05f) v.glideCents = 0.0f; }
            if (kulning && ! v.keyDown) v.fallCents -= 0.004f;

            float male = 0.0f, female = 0.0f;
            for (int s = 0; s < singers; ++s)
            {
                auto& sg = v.singers[(size_t) s];
                if (sg.delay > 0) { --sg.delay; continue; }

                // Tonhöjden räknas om var 16:e sample: drift, vibrato och glid ändras långsamt.
                if ((v.counter & 15) == 0 || sg.inc == 0.0)
                {
                    // Långsam drift: varje sångare letar efter tonen hela tiden.
                    sg.driftTimer -= 16;
                    if (sg.driftTimer <= 0)
                    {
                        sg.driftTarget = (random.nextFloat() * 2.0f - 1.0f) * params.spreadCents * 0.5f;
                        sg.driftTimer = (int) (sr * (0.3 + 0.7 * random.nextFloat()));
                    }
                    sg.drift += (sg.driftTarget - sg.drift) * 0.003f;

                    const float vibRate = 5.0f + 0.8f * (float) ((s * 37) % 10) / 10.0f;
                    sg.vibPhase += 16.0 * vibRate / sr; sg.vibPhase -= std::floor (sg.vibPhase);
                    // Vibratot kommer in först efter en stund, som hos en riktig sångare.
                    const float vibOn = (float) juce::jmin (1.0, v.t / 0.6);
                    const float offsetCents = ((float) s - (float) (singers - 1) * 0.5f) * params.spreadCents / (float) juce::jmax (1, singers - 1) * 0.8f
                                            + sg.drift + vibOn * vibDepth * (float) std::sin (twoPi * sg.vibPhase)
                                            + v.glideCents + v.fallCents;
                    sg.inc = baseHz * std::exp2 (offsetCents / 1200.0f) / sr;
                }
                const double inc = sg.inc;
                sg.phase += inc;
                if (sg.phase >= 1.0) sg.phase -= 1.0;

                // Stämbanden: en bandbegränsad sågtand (PolyBLEP), mjukad, med andning.
                float saw = (float) (2.0 * sg.phase - 1.0);
                saw -= polyBlep (sg.phase, inc);
                sg.lp = saw + lpCoef * (sg.lp - saw);
                const float noise = (random.nextFloat() * 2.0f - 1.0f) * params.breath * 0.6f;
                const float src = (sg.lp * 1.6f + noise) * singerGain;

                // I blandad kör sjunger varannan sångare med kvinnoröst.
                const bool isFemale = params.voices == ChoirVoices::Kvinnoroster || kulning
                                   || (params.voices == ChoirVoices::Blandad && (s % 2) == 1);
                (isFemale ? female : male) += src;
            }

            float out = 0.0f;
            for (int k = 0; k < numFormants; ++k)
            {
                out += v.formants[0][(size_t) k].processSample (0, male) * v.formantGain[0][(size_t) k];
                out += v.formants[1][(size_t) k].processSample (0, female) * v.formantGain[1][(size_t) k];
            }
            dest[i] += out * env * v.velocity * 0.7f;
            v.t += 1.0 / sr;
            ++v.counter;
        }
        if (! v.env.isActive()) v.note = -1;
    }

    static float polyBlep (double t, double dt)
    {
        if (t < dt) { const double x = t / dt; return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt) { const double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }

    // Den trasiga högtalaren: smal, överstyrd, med glapp i kontakten.
    void renderSpeaker (float* d, int numSamples)
    {
        const float h = params.speaker;
        const float drive = 1.0f + 8.0f * h;
        for (int i = 0; i < numSamples; ++i)
        {
            float x = speakerLp.processSample (0, speakerHp.processSample (0, d[i]));
            x = std::tanh (x * drive) / std::sqrt (drive) * 1.6f;

            if (--crackleTimer <= 0)
            {
                const bool glapp = random.nextFloat() < 0.3f * h;
                crackleTarget = glapp ? 0.1f * random.nextFloat() : 1.0f;
                crackleTimer = (int) (sr * (glapp ? 0.01 + 0.06 * random.nextFloat() : 0.05 + 0.4 * random.nextFloat()));
            }
            crackleGain += (crackleTarget - crackleGain) * 0.02f;
            const float crackle = random.nextFloat() < 0.002f * h ? (random.nextFloat() - 0.5f) * 0.5f * h : 0.0f;
            x = x * crackleGain + crackle;
            d[i] += (x - d[i]) * h;
        }
    }

    double sr = 44100.0;
    KorenParams params;
    std::array<Voice, numVoices> voices;
    juce::dsp::StateVariableTPTFilter<float> speakerHp, speakerLp;
    float crackleGain = 1.0f, crackleTarget = 1.0f;
    int crackleTimer = 0;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x4b4f5245 }; // "KORE"
};

} // namespace krang
