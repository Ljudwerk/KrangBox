// Stålpiano – KrangBox femte kammare: piano, hackbräde och preparerat piano (fas 3).
//
// Modalsyntes, helt syntetiskt: varje ton är en summa av dämpade svängningar – strängens deltoner.
//   Styvhet:   stålsträngar är styva, så deltonerna ligger lite högre än heltalsmultiplar
//              (f_n = n·f0·√(1 + B·n²)). Det är den metalliska, "falska" pianoklangen.
//   Svävning:  två eller tre strängar per ton, lite ostämda mot varandra, så att tonen svävar.
//   Hammaren:  hårdheten bestämmer hur många höga deltoner som väcks; anslagspunkten släcker de deltoner
//              som har en nod där hammaren slår. Varje anslag har ett kort filtduns.
//   Dämparen:  när tangenten släpps (och pedalen är uppe) dämpas strängen. Hackbrädet har inga dämpare.
//   Material:  sex strängmaterial – stål, mässing, koppar, järn, silke och djurhud – med egen styvhet,
//              klingtid, glans och ojämnhet. Silke och djurhud tappar spänning efter anslaget, så tonen sjunker.
//   Rost:      ett rasslande brus som följer strängens svängning, som en kedja som ligger på strängarna.
//   Preparerat: skruvar mellan strängarna – deltonerna förskjuts, klingar kortare, och skruvarna
//              själva klingar med egna, oharmoniska toner.
// Efter rösterna: Klangbotten, tre resonanser som ger en trälåda att klinga i.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include "Stamning.h"
#include <array>
#include <cmath>

namespace krang
{

enum class StalMode { Piano = 0, Hackbrade, Preparerat };
enum class StalMaterial { Stal = 0, Massing, Koppar, Jarn, Silke, Djurhud };

// Vad strängens material gör med klangen. Alla tal är faktorer mot stål.
struct MaterialTraits
{
    float stiffness;  // oharmonicitet
    float decay;      // klingtid
    float hfDamp;     // hur mycket snabbare de höga deltonerna dör
    float bright;     // hur mycket diskant hammaren väcker
    float jitter;     // ojämn tjocklek: deltonerna hamnar slumpvis fel (andel)
    float sagCents;   // spänningsfall: tonen börjar så här mycket högre och sjunker
    float thump;      // hur mycket av anslaget som är duns
    float rattle;     // eget rassel

    static const MaterialTraits& get (StalMaterial m)
    {
        static const MaterialTraits table[6] = {
            { 1.00f, 1.00f, 1.0f, 1.00f, 0.000f,  0.0f, 1.0f, 0.00f }, // Stål: klar och metallisk
            { 0.80f, 0.80f, 1.3f, 0.80f, 0.002f,  0.0f, 1.0f, 0.00f }, // Mässing: varmare, gyllene
            { 0.45f, 1.25f, 1.7f, 0.60f, 0.000f,  0.0f, 1.2f, 0.00f }, // Koppar (lindad): mörk, rund, lång
            { 1.40f, 0.70f, 1.0f, 1.10f, 0.006f,  2.0f, 1.0f, 0.10f }, // Järn: hårt, ojämnt, rostigt
            { 0.15f, 0.35f, 3.0f, 0.45f, 0.003f,  8.0f, 1.5f, 0.00f }, // Silke: mjukt och kort, som en qin
            { 0.30f, 0.30f, 4.0f, 0.30f, 0.020f, 28.0f, 3.5f, 0.15f }, // Djurhud: dov, ojämn, sjunker som ett trumskinn
        };
        return table[juce::jlimit (0, 5, (int) m)];
    }
};

struct StalpianoParams
{
    StalMode mode     = StalMode::Piano;
    StalMaterial material = StalMaterial::Stal;
    float hardness    = 0.5f;   // hammarens hårdhet 0..1
    float strikePos   = 0.12f;  // anslagspunkt, andel av strängens längd 0.05..0.5
    float stiffness   = 0.3f;   // styvhet (oharmonicitet) 0..1
    float detuneCents = 3.0f;   // svävning mellan strängarna 0..20 cent
    float decaySec    = 8.0f;   // klingtid för mellanregistret, 0.5..30 s
    float damperSec   = 0.15f;  // hur snabbt dämparen tystar strängen, 0.03..2 s
    float body        = 0.4f;   // klangbotten 0..1
    float rust        = 0.0f;   // rassel 0..1
};

class Stalpiano
{
public:
    // Stämningen som kammaren spelar i (sätts av motorn; liksvävande om ingen satts).
    void setStamning (const Stamning* s) { stamning = s != nullptr ? s : &Stamning::neutral(); }
    const Stamning* stamning = &Stamning::neutral();
    static constexpr int numVoices = 16;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        static constexpr float bodyHz[3] = { 110.0f, 245.0f, 560.0f };
        for (int i = 0; i < 3; ++i)
        {
            bodyFilters[(size_t) i].prepare ({ sampleRate, 64, 1 });
            bodyFilters[(size_t) i].setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            bodyFilters[(size_t) i].setCutoffFrequency (bodyHz[i]);
            bodyFilters[(size_t) i].setResonance (2.5f);
        }
        reset();
    }

    void reset()
    {
        for (auto& v : voices) { v.active = false; v.keyDown = false; v.note = -1; v.numModes = 0; }
        for (auto& f : bodyFilters) f.reset();
        sustain = false;
    }

    void setParams (const StalpianoParams& p) { params = p; }

    void setSustain (bool down)
    {
        if (down == sustain) return;
        sustain = down;
        if (! sustain)
            for (auto& v : voices)
                if (v.active && ! v.keyDown) setDamped (v, true);
    }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target != nullptr)
        {
            // Samma ton igen: hammaren slår på en sträng som redan svänger – lägg till energi, inget klick.
            strike (*target, velocity, true);
            return;
        }
        for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr) // stjäl den tystaste släppta rösten, annars den äldsta
        {
            for (auto& v : voices)
                if (! v.keyDown && (target == nullptr || v.level < target->level)) target = &v;
            if (target == nullptr)
            {
                target = &voices[0];
                for (auto& v : voices) if (v.age < target->age) target = &v;
            }
        }
        target->note = note;
        strike (*target, velocity, false);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown)
            {
                v.keyDown = false;
                if (! sustain) setDamped (v, true);
            }
    }

    void allNotesOff()
    {
        sustain = false;
        for (auto& v : voices)
            if (v.active) { v.keyDown = false; setDamped (v, true); }
    }

    void render (float* dest, int numSamples)
    {
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.active) renderVoice (v, dest, numSamples);

        if (params.body > 0.001f)
        {
            const float b = params.body * 0.9f;
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = dest[i];
                const float res = bodyFilters[0].processSample (0, x) + 0.8f * bodyFilters[1].processSample (0, x)
                                + 0.6f * bodyFilters[2].processSample (0, x);
                dest[i] = x * (1.0f - 0.3f * params.body) + res * b;
            }
        }

        // Mjuk mättning av anslagstopparna, som en ram av stål som inte orkar svänga hur mycket som helst.
        for (int i = 0; i < numSamples; ++i)
            dest[i] = std::tanh (dest[i] * 1.5f) / 1.5f;
    }

    int getNumActiveVoices() const
    {
        int n = 0;
        for (auto& v : voices) n += v.active ? 1 : 0;
        return n;
    }

private:
    static constexpr int maxModes = 64, maxPartials = 32, beatingPartials = 10, numBolts = 4;

    struct Voice
    {
        bool active = false, keyDown = false, damped = false;
        int note = -1, numModes = 0;
        juce::uint32 age = 0;
        float level = 0.0f, velocity = 0.8f;
        std::array<float, maxModes> y1 {}, y2 {}, a1 {}, a2 {}, cosW {}, sinW {}, omega {}, sigma {}, freq {};
        std::array<int, maxModes> partial {}, group {}; // deltonens nummer (0 = skruv) och antal strängar för den
        // Hammarens filtduns.
        int thumpLeft = 0, thumpLen = 1;
        float thumpAmp = 0.0f, thumpLp = 0.0f, thumpCoef = 0.5f;
        float rattleEnv = 0.0f, rattleHp = 0.0f;
        std::array<float, maxModes> excite {};  // hur hårt hammaren driver varje delton
        int pulsePos = 0, pulseLen = 1;         // hammarens kontakttid med strängen
        StalMode mode = StalMode::Piano;
        StalMaterial material = StalMaterial::Stal;
        float sagCents = 0.0f; // nuvarande spänningsfall; avtar efter anslaget
    };

    // Deterministisk "slump" per ton och delton, så att en preparerad ton alltid låter likadant.
    static float hash (int note, int k)
    {
        juce::uint32 h = (juce::uint32) (note * 7919 + k * 104729 + 12345);
        h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
        return (float) (h & 0xffff) / 65535.0f;
    }

    void strike (Voice& v, float velocity, bool restrike)
    {
        const auto mode = params.mode;
        const int note = v.note;
        const double f0 = stamning->hz (note);
        const float vel = 0.25f + 0.75f * velocity;

        const auto& mat = MaterialTraits::get (params.material);
        if (! restrike || v.mode != mode || v.material != params.material)
        {
            // Bygg deltonerna.
            v.mode = mode;
            v.material = params.material;
            v.sagCents = 0.0f;
            v.numModes = 0;
            std::fill (v.y1.begin(), v.y1.end(), 0.0f);
            std::fill (v.y2.begin(), v.y2.end(), 0.0f);

            const float stiffScale = mode == StalMode::Hackbrade ? 0.1f + 1.5f * params.stiffness
                                   : mode == StalMode::Preparerat ? 0.4f + 5.0f * params.stiffness
                                   : 0.2f + 2.8f * params.stiffness;
            const double B = juce::jmin (0.02, 1.0e-4 * std::pow (2.0, (note - 36) / 12.0 * 0.8) * stiffScale * mat.stiffness);
            const int strings = mode == StalMode::Hackbrade ? 3 : (note < 34 ? 1 : note < 46 ? 2 : 3);
            const float detune = params.detuneCents + (mode == StalMode::Hackbrade ? 4.0f : 0.0f);
            static constexpr float stringCents[3] = { 0.0f, 1.0f, -0.7f };

            const double maxHz = juce::jmin (16000.0, sr * 0.45);
            const float T = juce::jlimit (0.3f, 60.0f, params.decaySec * std::pow (2.0f, -(float) (note - 48) / 18.0f))
                          * (mode == StalMode::Preparerat ? 0.45f : 1.0f) * mat.decay;
            const float hfDamp = (mode == StalMode::Hackbrade ? 0.5f : 1.0f) * mat.hfDamp;

            for (int n = 1; n <= maxPartials; ++n)
            {
                double fn = n * f0 * std::sqrt (1.0 + B * n * n);
                if (mode == StalMode::Preparerat)
                    fn *= 1.0 + 0.04 * (hash (note, n) - 0.5) * std::sqrt ((double) n); // skruvarna förskjuter deltonerna
                if (mat.jitter > 0.0f && n > 1)
                    fn *= 1.0 + mat.jitter * (hash (note, 200 + n) - 0.5) * 2.0 * std::sqrt ((double) n); // ojämn sträng
                if (fn >= maxHz) break;
                const float sig = 6.9f / T * (1.0f + hfDamp * (float) ((fn / 2500.0) * (fn / 2500.0)));
                const int ns = n <= beatingPartials ? strings : 1;
                for (int s = 0; s < ns && v.numModes < maxModes - numBolts; ++s)
                {
                    const double f = fn * std::pow (2.0, stringCents[s] * detune / 1200.0);
                    const auto k = (size_t) v.numModes++;
                    v.omega[k] = (float) (juce::MathConstants<double>::twoPi * f / sr);
                    v.cosW[k] = std::cos (v.omega[k]);
                    v.sinW[k] = std::sin (v.omega[k]);
                    v.sigma[k] = sig;
                    v.freq[k] = (float) f;
                    v.partial[k] = n;
                    v.group[k] = ns;
                }
            }
            if (mode == StalMode::Preparerat)
            {
                // Skruvarna klingar med egna, oharmoniska toner.
                static constexpr float ratios[numBolts] = { 1.41f, 2.23f, 3.71f, 5.29f };
                for (int b = 0; b < numBolts; ++b)
                {
                    const double f = f0 * ratios[b] * (0.92 + 0.16 * hash (note, 100 + b));
                    if (f >= maxHz) continue;
                    const auto k = (size_t) v.numModes++;
                    v.omega[k] = (float) (juce::MathConstants<double>::twoPi * f / sr);
                    v.cosW[k] = std::cos (v.omega[k]);
                    v.sinW[k] = std::sin (v.omega[k]);
                    v.sigma[k] = 6.9f / 0.35f;
                    v.freq[k] = (float) f;
                    v.partial[k] = 0;
                    v.group[k] = 1;
                }
            }
        }

        // Hammarens spektrum: hårdhet och anslagsstyrka lyfter de höga deltonerna.
        const double fc = 300.0 * std::pow (2.0, params.hardness * 5.0 + velocity * 2.0) * (v.mode == StalMode::Hackbrade ? 2.0 : 1.0)
                        * mat.bright;
        const float pos = juce::jlimit (0.05f, 0.5f, params.strikePos);
        for (int k = 0; k < v.numModes; ++k)
        {
            const int n = v.partial[(size_t) k];
            const double fn = v.freq[(size_t) k];
            float amp = n == 0 ? 0.35f // skruv
                               : (float) (std::abs (std::sin (juce::MathConstants<double>::pi * n * pos))
                                          / (1.0 + (fn / fc) * (fn / fc)) / std::sqrt ((double) n));
            amp *= vel / (float) v.group[(size_t) k];
            // Hammaren är en kraftpuls in i varje delton (resonator). Med pulsens yta 1 blir deltonens
            // amplitud amp; pulsen själv mjukar upp anslaget, så att varken nya toner eller omslag klickar.
            const double w = juce::MathConstants<double>::twoPi * fn / sr;
            v.excite[(size_t) k] = amp * (float) std::sin (w);
        }

        // Filthammaren ligger mot strängen 0,15–1 ms; hårdare hammare och hårdare anslag ger kortare kontakt.
        v.pulseLen = juce::jmax (4, (int) (sr * (0.00015 + 0.0009 * (1.0f - params.hardness) * (1.2f - velocity))));
        v.pulsePos = 0;
        v.active = true;
        v.keyDown = true;
        v.velocity = velocity;
        v.age = ++ageCounter;
        v.level = 1.0f;
        // Spänningsfall: ett hårt slag sträcker strängen så att tonen börjar högre och sjunker tillbaka.
        v.sagCents = juce::jmax (v.sagCents, mat.sagCents * (0.3f + 0.7f * velocity));
        setDamped (v, false);

        // Filtduns: ett kort brusstöt, längre och mjukare med mjuk hammare.
        v.thumpLen = juce::jmax (8, (int) (sr * (0.002 + 0.006 * (1.0f - params.hardness))));
        v.thumpLeft = v.thumpLen;
        v.thumpAmp = 0.12f * vel * (0.4f + params.hardness) * mat.thump;
        v.thumpCoef = std::exp (-juce::MathConstants<float>::twoPi * (float) juce::jmin (fc, sr * 0.4) / (float) sr);
    }

    void setDamped (Voice& v, bool damped)
    {
        // Hackbrädet har inga dämpare: strängarna klingar ut av sig själva.
        v.damped = damped && v.mode != StalMode::Hackbrade;
        const float extra = v.damped ? 6.9f / juce::jmax (0.03f, params.damperSec) : 0.0f;
        for (int k = 0; k < v.numModes; ++k)
        {
            const float r = std::exp (-(v.sigma[(size_t) k] + extra) / (float) sr);
            v.a2[(size_t) k] = r * r;
        }
        updateTuning (v);
    }

    // Räknar om svängningarnas frekvens med nuvarande spänningsfall (cos(ω(1+e)) ≈ cos ω − e·ω·sin ω).
    static void updateTuning (Voice& v)
    {
        const float e = v.sagCents > 0.01f ? std::exp2 (v.sagCents / 1200.0f) - 1.0f : 0.0f;
        for (int k = 0; k < v.numModes; ++k)
        {
            const float r = std::sqrt (v.a2[(size_t) k]);
            const float c = v.cosW[(size_t) k] - e * v.omega[(size_t) k] * v.sinW[(size_t) k];
            v.a1[(size_t) k] = 2.0f * r * juce::jlimit (-1.0f, 1.0f, c);
        }
    }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const int n = v.numModes;
        float* y1 = v.y1.data();
        float* y2 = v.y2.data();
        const float* a1 = v.a1.data();
        const float* a2 = v.a2.data();
        const float rust = params.rust + (v.mode == StalMode::Preparerat ? 0.25f : 0.0f) + MaterialTraits::get (v.material).rattle;

        // Spänningsfallet avtar med ungefär 0,15 s tidskonstant; stäm om svängningarna en gång per block.
        if (v.sagCents > 0.01f)
        {
            v.sagCents *= std::exp (-(float) numSamples / (float) (sr * 0.15));
            if (v.sagCents <= 0.01f) v.sagCents = 0.0f;
            updateTuning (v);
        }
        float peak = 0.0f;
        constexpr float gain = 0.6f;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;
            if (v.pulsePos < v.pulseLen)
            {
                // Hammaren trycker mot strängen: upphöjd cosinus med ytan 1.
                const float x = (1.0f - std::cos (juce::MathConstants<float>::twoPi * ((float) v.pulsePos + 0.5f) / (float) v.pulseLen))
                              / (float) v.pulseLen;
                ++v.pulsePos;
                const float* e = v.excite.data();
                for (int k = 0; k < n; ++k)
                {
                    const float y = a1[k] * y1[k] - a2[k] * y2[k] + e[k] * x;
                    y2[k] = y1[k];
                    y1[k] = y;
                    s += y;
                }
            }
            else
            {
                for (int k = 0; k < n; ++k)
                {
                    const float y = a1[k] * y1[k] - a2[k] * y2[k];
                    y2[k] = y1[k];
                    y1[k] = y;
                    s += y;
                }
            }

            if (v.thumpLeft > 0)
            {
                const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) v.thumpLeft / (float) v.thumpLen);
                const float noise = random.nextFloat() * 2.0f - 1.0f;
                v.thumpLp = noise + v.thumpCoef * (v.thumpLp - noise);
                s += v.thumpLp * w * v.thumpAmp;
                --v.thumpLeft;
            }

            if (rust > 0.001f)
            {
                // Kedjan rasslar i takt med strängens rörelse: brus amplitudmodulerat av svängningen.
                const float noise = random.nextFloat() * 2.0f - 1.0f;
                const float hp = noise - v.rattleHp; v.rattleHp = noise;
                v.rattleEnv += 0.45f * (hp - v.rattleEnv);            // grovt lågpass, så att det rasslar och inte fräser
                s += v.rattleEnv * std::abs (s) * rust * 1.6f;
            }

            s *= gain * (v.mode == StalMode::Preparerat ? 1.35f : 1.0f); // preparerade toner klingar kortare
            dest[i] += s;
            peak = juce::jmax (peak, std::abs (s));
        }

        // Tyst nog: frigör rösten och nollställ små rester (undviker denormaler).
        v.level = juce::jmax (peak, v.level * 0.5f);
        if (peak < 2.0e-5f && v.thumpLeft == 0 && v.pulsePos >= v.pulseLen && (v.damped || v.level < 2.0e-5f))
        {
            v.active = false;
            v.numModes = 0;
        }
    }

    double sr = 44100.0;
    StalpianoParams params;
    std::array<Voice, numVoices> voices;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 3> bodyFilters;
    bool sustain = false;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x5354414c }; // "STAL"
};

} // namespace krang
