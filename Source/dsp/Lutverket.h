// Lutverket – KrangBox tionde kammare: knäppta strängar (fas 3b).
//
// Utökad Karplus-Strong (Jaffe–Smith), helt syntetiskt. Varje sträng är en återkopplad fördröjning:
//   Knäppet:   ett kort brusutbrott, mjukat efter hårdheten och kamfiltrerat efter knäppunkten
//              (knäpper man mitt på strängen försvinner de jämna övertonerna).
//   Förlusterna: ett lågpass i slingan (Glans) och en dämpning per varv (Klingtid); en allpass stämmer in
//              bråkdelen av ett sample så att alla toner ligger rätt.
//   Kör:       lutor har dubbla strängar per ton (körer), lite ostämda mot varandra.
//   Surr:      strängen slår mot rostig metall när den svänger för långt – den surrar och skorrar.
// Efter strängarna: Botten, en klanglåda med tre resonanser.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

enum class LuteType { Kantele = 0, Luta, Cittra, Rostharpa };

struct LutverketParams
{
    LuteType type    = LuteType::Kantele;
    float pluckPos   = 0.2f;   // knäppunkt 0.05..0.5
    float hardness   = 0.5f;   // knäppets hårdhet 0..1
    float decaySec   = 4.0f;   // klingtid 0.3..20 s
    float brightness = 0.6f;   // glans 0..1
    float buzz       = 0.0f;   // surr 0..1
    float courseCents = 4.0f;  // körens ostämdhet 0..20 cent
    float body       = 0.4f;   // klanglåda 0..1
    float damperSec  = 0.25f;  // dämpning när tangenten släpps 0.03..3 s
};

class Lutverket
{
public:
    static constexpr int numVoices = 12, maxStrings = 3;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices)
            for (auto& s : v.strings)
                s.buf.assign ((size_t) (sampleRate / 25.0) + 8, 0.0f);
        static constexpr float bodyHz[3] = { 120.0f, 260.0f, 480.0f };
        for (int i = 0; i < 3; ++i)
        {
            bodyFilters[(size_t) i].prepare ({ sampleRate, 64, 1 });
            bodyFilters[(size_t) i].setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            bodyFilters[(size_t) i].setCutoffFrequency (bodyHz[i]);
            bodyFilters[(size_t) i].setResonance (2.2f);
        }
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.active = false; v.keyDown = false; v.note = -1;
            for (auto& s : v.strings) clearString (s);
        }
        for (auto& f : bodyFilters) f.reset();
    }

    void setParams (const LutverketParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;   // samma sträng knäpps igen
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.level < target->level) target = &v;
        }
        pluck (*target, note, velocity);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown) { v.keyDown = false; v.damped = true; }
    }

    void allNotesOff()
    {
        for (auto& v : voices) if (v.active) { v.keyDown = false; v.damped = true; }
    }

    void render (float* dest, int numSamples)
    {
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.active) renderVoice (v, dest, numSamples);

        const auto& t = traits (params.type);
        const float body = juce::jlimit (0.0f, 1.0f, params.body * t.body);
        for (int i = 0; i < numSamples; ++i)
        {
            float x = dest[i];
            if (body > 0.001f)
            {
                const float res = bodyFilters[0].processSample (0, x) + 0.8f * bodyFilters[1].processSample (0, x)
                                + 0.5f * bodyFilters[2].processSample (0, x);
                x = x * (1.0f - 0.3f * body) + res * body * 1.2f;
            }
            dest[i] = std::tanh (x * 2.4f) / 1.2f; // plockade toner har höga toppar; tanh tar knäppet
        }
    }

private:
    struct Traits { float decay, bright, body, buzz, jitter; int strings; };

    static const Traits& traits (LuteType t)
    {
        static const Traits table[4] = {
            { 1.4f, 1.20f, 0.5f, 0.00f, 0.0f,    1 },  // Kantele: metallsträngar, klar och lång
            { 0.8f, 0.65f, 1.4f, 0.00f, 0.0f,    2 },  // Luta: tarmsträngar i körer, varm, med klanglåda
            { 1.1f, 1.10f, 0.8f, 0.00f, 0.0f,    3 },  // Cittra: tre metallsträngar per ton, skimrande
            { 0.8f, 0.95f, 0.7f, 0.35f, 0.004f,  2 },  // Rostharpa: rostiga strängar som surrar och vinglar
        };
        return table[juce::jlimit (0, 3, (int) t)];
    }

    struct String
    {
        std::vector<float> buf;
        int pos = 0, length = 100;
        float apCoef = 0.0f, apX1 = 0.0f, apY1 = 0.0f;   // allpass för bråkdelen
        float lp = 0.0f;
        double hz = 220.0;
    };

    struct Voice
    {
        std::array<String, maxStrings> strings;
        int numStrings = 1, note = -1;
        bool active = false, keyDown = false, damped = false;
        float level = 0.0f, wobble = 0.0f, wobbleTarget = 0.0f;
        int wobbleTimer = 0, quiet = 0;
    };

    static void clearString (String& s)
    {
        std::fill (s.buf.begin(), s.buf.end(), 0.0f);
        s.pos = 0; s.apX1 = s.apY1 = s.lp = 0.0f;
    }

    float noise() { seed = seed * 1664525u + 1013904223u; return (float) (int) seed * (1.0f / 2147483648.0f); }

    // Stämmer en sträng: heltalsfördröjning + allpass för bråkdelen, med hänsyn till lågpassets fördröjning.
    void tune (String& s, double hz, float lpCoef) const
    {
        s.hz = hz;
        const double total = sr / hz - (double) lpCoef / (1.0 - (double) lpCoef);
        const double whole = std::floor (total - 0.5);
        s.length = juce::jlimit (2, (int) s.buf.size() - 2, (int) whole);
        const double frac = juce::jlimit (0.1, 1.1, total - (double) s.length);
        s.apCoef = (float) ((1.0 - frac) / (1.0 + frac));
    }

    float lpCoefFor (const String& s) const
    {
        // Glansen: mindre lågpass i slingan = ljusare och längre diskant.
        const float bright = juce::jlimit (0.0f, 1.0f, params.brightness * traits (params.type).bright);
        return juce::jlimit (0.02f, 0.9f, 0.6f - 0.55f * bright + (float) (s.hz / 20000.0) * 0.2f);
    }

    void pluck (Voice& v, int note, float velocity)
    {
        const auto& t = traits (params.type);
        const bool restrike = v.active && v.note == note && v.numStrings == t.strings;
        v.numStrings = t.strings;
        v.note = note;
        v.active = true;
        v.keyDown = true;
        v.damped = false;
        v.level = 1.0f;
        v.quiet = 0;

        const double baseHz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        const float vel = 0.3f + 0.7f * velocity;
        // Knäppets ljushet: hårdare knäpp och hårdare anslag ger mer diskant.
        const float exciteCoef = juce::jlimit (0.0f, 0.95f, 0.85f - 0.75f * params.hardness - 0.15f * velocity);
        const float pos = juce::jlimit (0.05f, 0.5f, params.pluckPos);

        for (int k = 0; k < v.numStrings; ++k)
        {
            auto& s = v.strings[(size_t) k];
            if (! restrike) clearString (s);
            // Körens strängar sprids symmetriskt kring tonen, så att kören som helhet ligger rätt.
            const float offset = v.numStrings > 1 ? ((float) k - (float) (v.numStrings - 1) * 0.5f) / (float) (v.numStrings - 1) : 0.0f;
            const double hz = baseHz * std::exp2 (offset * params.courseCents / 1200.0);
            tune (s, hz, lpCoefFor (s));

            // Brusutbrottet, mjukat och kamfiltrerat efter knäppunkten, läggs ovanpå det strängen redan gör.
            const int n = s.length;
            const int combDelay = juce::jmax (1, (int) (pos * (float) n));
            std::vector<float>& b = s.buf;
            float lp = 0.0f;
            std::array<float, 4096> burst {};
            const int len = juce::jmin (n, (int) burst.size());
            float mean = 0.0f;
            for (int i = 0; i < len; ++i)
            {
                lp = noise() * (1.0f - exciteCoef) + exciteCoef * lp;
                burst[(size_t) i] = lp;
                mean += lp;
            }
            mean /= (float) len;
            // Normera på energi (inte topp), så att mjuka och hårda knäpp blir lika starka.
            float energy = 1.0e-9f;
            for (int i = 0; i < len; ++i) { burst[(size_t) i] -= mean; energy += burst[(size_t) i] * burst[(size_t) i]; }
            const float rms = std::sqrt (energy / (float) len);
            const int size = (int) b.size();
            for (int i = 0; i < len; ++i)
            {
                const float x = burst[(size_t) i] - (i >= combDelay ? burst[(size_t) (i - combDelay)] : 0.0f);
                const int idx = ((s.pos - n + i) % size + size) % size;
                b[(size_t) idx] += x / rms * 0.3f * vel / std::sqrt ((float) v.numStrings);
            }
        }
    }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const auto& t = traits (params.type);
        const float buzz = juce::jlimit (0.0f, 1.0f, params.buzz + t.buzz);
        const float thr = 0.35f * (1.0f - 0.75f * buzz);
        float peak = 0.0f;

        // Rostharpans vinglande stämning: strängarna glider lite hela tiden.
        if (t.jitter > 0.0f)
        {
            v.wobbleTimer -= numSamples;
            if (v.wobbleTimer <= 0) { v.wobbleTarget = noise(); v.wobbleTimer = (int) (sr * 0.2); }
            v.wobble += (v.wobbleTarget - v.wobble) * 0.1f;
        }

        for (int k = 0; k < v.numStrings; ++k)
        {
            auto& s = v.strings[(size_t) k];
            const float lpCoef = lpCoefFor (s);
            if (t.jitter > 0.0f)
            {
                const float offset = v.numStrings > 1 ? ((float) k - (float) (v.numStrings - 1) * 0.5f) / (float) (v.numStrings - 1) : 0.0f;
                tune (s, 440.0 * std::pow (2.0, (v.note - 69) / 12.0) * std::exp2 (offset * params.courseCents / 1200.0)
                         * (1.0 + t.jitter * (double) v.wobble * (1.0 + 0.5 * k)), lpCoef);
            }
            // Dämpning per varv: klingtiden (kortare i diskanten), eller dämparen när tangenten släppts.
            const float T = v.damped ? juce::jmax (0.03f, params.damperSec)
                                     : juce::jlimit (0.1f, 40.0f, params.decaySec * t.decay * std::pow (2.0f, -(float) (v.note - 48) / 30.0f));
            const float g = std::exp (-6.9f / (T * (float) s.hz));
            const int size = (int) s.buf.size();
            float* b = s.buf.data();

            for (int i = 0; i < numSamples; ++i)
            {
                const int readIdx = ((s.pos - s.length) % size + size) % size;
                float x = b[readIdx];
                // Allpass för bråkdelen av ett sample.
                const float ap = s.apCoef * x + s.apX1 - s.apCoef * s.apY1;
                s.apX1 = x; s.apY1 = ap;
                // Förluster.
                s.lp = (1.0f - lpCoef) * ap + lpCoef * s.lp;
                float y = s.lp * g;
                // Surr: strängen slår i metallen när den svänger för långt.
                if (buzz > 0.001f && std::abs (y) > thr)
                {
                    const float over = std::abs (y) - thr;
                    y = std::copysign (thr + over * (1.0f - 0.8f * buzz), y) + noise() * over * buzz * 0.3f;
                }
                b[s.pos] = y;
                s.pos = (s.pos + 1) % size;
                dest[i] += y;
                peak = juce::jmax (peak, std::abs (y));
            }
        }

        if (! std::isfinite (peak)) { v.active = false; for (auto& s : v.strings) clearString (s); return; }
        v.level = juce::jmax (peak, v.level * 0.5f);
        if (peak < 3.0e-5f && ++v.quiet > 4) v.active = false;
    }

    double sr = 44100.0;
    LutverketParams params;
    std::array<Voice, numVoices> voices;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 3> bodyFilters;
    juce::uint32 seed = 0x4c555445; // "LUTE"
};

} // namespace krang
