// Automaten – KrangBox fjortonde kammare: Doktor Krangs mekaniska leksaker (fas 3c).
// Tre figurer: Talmaskinen (nedan), Sångfågeln och Urverket (se Leksaker.h).
//
// Talmaskinen är helt syntetisk, som de första talmaskinerna: en röstkälla (stämband, surr eller viskning) går genom
// fem formantfilter som flyttas från ljud till ljud, och brus formar konsonanterna (s, sj, f, h) och
// smällarna (k, t, p, d). Varje tangent säger ett ord på tonens tonhöjd – eller, i läget "Tangenterna",
// väljer tangenten vilket ord som sägs, så att man kan spela meningar.
//   Röst     – 0 stämband (mjuk), 0,5 surr (som en gammal talmaskin), 1 viskning
//   Monoton  – från naturlig sjunkande satsmelodi till helt platt robotröst
//   Tempo    – hur fort maskinen talar
//   Trasig   – hack, upprepade ljud, tappade ljud och tonhopp
//   Upprepa  – säger ordet om och om igen så länge tangenten hålls
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>
#include "Leksaker.h"

namespace krang
{

// Orden. "Tangenterna" (sist) betyder att tangenten väljer ord: C = det första, C# = det andra …
inline constexpr int numAutomatWords = 12;
inline const char* automatWord (int i)
{
    static const char* words[] = { "KRANG", "MASKIN", "HJÄLP", "ÅNGA", "VARNING", "ROST",
                                   "NOLL ETT", "FOLKET", "JA", "NEJ", "MÄNNISKA", "DOKTOR KRANG" };
    return words[juce::jlimit (0, numAutomatWords - 1, i)];
}

enum class AutomatFigure { Talmaskinen = 0, Sangfageln, Urverket };

struct AutomatenParams
{
    AutomatFigure figure = AutomatFigure::Talmaskinen;
    BirdType bird = BirdType::Kanariefagel;
    ClockType clock = ClockType::Pendelur;
    int word        = 0;      // 0..11, eller 12 = tangenterna väljer ord
    float voice     = 0.4f;   // röstkälla 0..1
    float monotone  = 0.6f;   // 0..1
    float tempo     = 0.5f;   // 0..1
    float broken    = 0.1f;   // trasighet 0..1
    bool repeat     = false;
};

class Automaten
{
public:
    static constexpr int numVoices = 4, numFormants = 5;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto& v : voices)
        {
            for (auto& f : v.formants)
            {
                f.prepare (spec);
                f.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            }
            v.fric.prepare (spec);
            v.fric.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        }
        buildWords();
        bird.prepare (sampleRate);
        clock.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        bird.reset();
        clock.reset();
        for (auto& v : voices)
        {
            v.active = false;
            v.note = -1;
            for (auto& f : v.formants) f.reset();
            v.fric.reset();
        }
    }

    void setParams (const AutomatenParams& p)
    {
        params = p;
        const LeksakParams lp { p.voice, p.monotone, p.tempo, p.broken, p.bird, p.clock };
        bird.setParams (lp);
        clock.setParams (lp);
    }

    void noteOn (int note, float velocity)
    {
        if (params.figure == AutomatFigure::Sangfageln) { bird.noteOn (note, velocity); return; }
        if (params.figure == AutomatFigure::Urverket)   { clock.noteOn (note, velocity); return; }
        Voice* target = nullptr;
        for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        v.active = true;
        v.keyDown = true;
        v.note = note;
        v.velocity = 0.5f + 0.5f * velocity;
        v.age = ++ageCounter;
        if (params.word >= numAutomatWords)
        {
            // Tangenterna väljer ord; tonhöjden följer oktaven (rösten talar på oktavens C, plus lite).
            v.wordIndex = ((note % 12) + 12) % 12;
            v.pitchNote = (float) (note - (note % 12) + 4);
        }
        else
        {
            v.wordIndex = params.word;
            v.pitchNote = (float) note;
        }
        startWord (v);
    }

    void noteOff (int note)
    {
        bird.noteOff (note);
        clock.noteOff (note);
        for (auto& v : voices)
            if (v.active && v.note == note) v.keyDown = false;
    }

    void allNotesOff()
    {
        bird.allNotesOff();
        clock.allNotesOff();
        for (auto& v : voices) v.keyDown = false;
    }

    // Ordet och ljudet som sägs just nu, för panelen (-1 = tyst).
    int currentWord() const
    {
        // Leksakerna: 100 + figuren medan de spelar.
        if (bird.isActive()) return 100 + (int) AutomatFigure::Sangfageln;
        if (clock.isActive()) return 100 + (int) AutomatFigure::Urverket;
        const Voice* best = nullptr;
        for (auto& v : voices) if (v.active && (best == nullptr || v.age > best->age)) best = &v;
        return best != nullptr ? best->wordIndex : -1;
    }

    void render (float* dest, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (auto& v : voices)
                if (v.active) out += renderVoice (v);
            dest[i] = out;
        }
        // Leksakerna läggs till ovanpå (de klingar ut även om man byter figur).
        renderToys (dest, numSamples);
    }

private:
    void renderToys (float* dest, int numSamples)
    {
        for (int pos = 0; pos < numSamples; pos += 256)
        {
            const int n = juce::jmin (256, numSamples - pos);
            std::array<float, 256> tmp {};
            bird.render (tmp.data(), n);
            for (int i = 0; i < n; ++i) dest[pos + i] += tmp[(size_t) i];
            tmp.fill (0.0f);
            clock.render (tmp.data(), n);
            for (int i = 0; i < n; ++i) dest[pos + i] += tmp[(size_t) i];
        }
    }

    // Ett språkljud: formanterna F1–F3, hur mycket röst och brus det har, brusets mittfrekvens,
    // längd (ms vid normalt tempo) och om det är en smäll (tystnad + kort brusstöt).
    struct Phone { float f1, f2, f3, voiced, noise, noiseHz, ms; bool plosive; };

    static Phone phone (const juce::String& p)
    {
        // Vokaler (mansröst), sedan konsonanter.
        if (p == "A")  return { 700, 1100, 2550, 1.0f, 0.0f, 0, 150, false };
        if (p == "AA")  return { 450, 750, 2500, 1.0f, 0.0f, 0, 150, false };
        if (p == "E")  return { 420, 1950, 2550, 1.0f, 0.0f, 0, 130, false };
        if (p == "AE")  return { 620, 1700, 2500, 1.0f, 0.0f, 0, 140, false };
        if (p == "I")  return { 290, 2150, 2900, 1.0f, 0.0f, 0, 120, false };
        if (p == "O")  return { 380, 720, 2450, 1.0f, 0.0f, 0, 140, false };
        if (p == "U")  return { 330, 900, 2300, 1.0f, 0.0f, 0, 130, false };
        if (p == "J")  return { 280, 2200, 3000, 0.8f, 0.05f, 3200, 60, false };
        if (p == "L")  return { 380, 1050, 2600, 0.8f, 0.0f, 0, 70, false };
        if (p == "R")  return { 480, 1300, 1700, 0.7f, 0.05f, 1500, 80, false };    // rullat: fladdrar i renderVoice
        if (p == "M")  return { 260, 1000, 2300, 0.55f, 0.0f, 0, 85, false };
        if (p == "N")  return { 260, 1600, 2600, 0.55f, 0.0f, 0, 80, false };
        if (p == "NG") return { 260, 2300, 2700, 0.5f, 0.0f, 0, 100, false };
        if (p == "V")  return { 300, 1200, 2300, 0.5f, 0.25f, 4000, 70, false };
        if (p == "S")  return { 300, 1600, 2600, 0.0f, 1.0f, 6500, 120, false };
        if (p == "SJ") return { 300, 1600, 2600, 0.0f, 1.0f, 2600, 130, false };
        if (p == "F")  return { 300, 1300, 2400, 0.0f, 0.6f, 5500, 100, false };
        if (p == "H")  return { 600, 1300, 2400, 0.0f, 0.5f, 1800, 70, false };
        if (p == "K")  return { 300, 1500, 2500, 0.0f, 1.0f, 2400, 85, true };
        if (p == "T")  return { 300, 1700, 2600, 0.0f, 1.0f, 4500, 75, true };
        if (p == "P")  return { 300, 900, 2300, 0.0f, 0.9f, 1100, 80, true };
        if (p == "D")  return { 300, 1700, 2600, 0.3f, 0.7f, 4000, 65, true };
        return { 500, 1500, 2500, 0.0f, 0.0f, 0, 80, false };   // paus
    }

    void buildWords()
    {
        static const char* spelled[numAutomatWords] = {
            "K R A NG", "M A SJ I N", "J AE L P", "AA NG A", "V A R N I NG", "R O S T",
            "N O L _ E T", "F O L K E T", "J A", "N E J", "M AE N I S K A", "D O K T O R _ K R A NG" };
        for (int w = 0; w < numAutomatWords; ++w)
        {
            words[(size_t) w].clear();
            for (auto& token : juce::StringArray::fromTokens (spelled[w], " ", ""))
                words[(size_t) w].push_back (phone (token));   // Å skrivs AA och Ä skrivs AE
        }
    }

    struct Voice
    {
        bool active = false, keyDown = false, phoneHasBurst = false;
        int note = -1, wordIndex = 0, phoneIndex = 0;
        float velocity = 1.0f, pitchNote = 48.0f, pitchJump = 0.0f, amp = 0.0f, burst = 0.0f;
        double phase = 0.0, phoneT = 0.0, phoneLen = 0.1, wordT = 0.0, wordLen = 1.0, pause = 0.0;
        std::array<float, 3> fNow { 500.0f, 1500.0f, 2500.0f };
        float voicedNow = 0.0f, noiseNow = 0.0f, noiseHzNow = 3000.0f;
        float glottalLp = 0.0f, glottalLp2 = 0.0f, buzzLp = 0.0f;
        std::array<float, numFormants> gain {};
        std::array<juce::dsp::StateVariableTPTFilter<float>, numFormants> formants;
        juce::dsp::StateVariableTPTFilter<float> fric;
        juce::uint32 age = 0, counter = 0;
    };

    double tempoScale() const { return 2.0 / (1.0 + 3.0 * (double) params.tempo); }  // 0 → 2× längre, 1 → 0,5×

    void startWord (Voice& v)
    {
        const auto& w = words[(size_t) v.wordIndex];
        v.wordLen = 0.0;
        for (auto& p : w) v.wordLen += p.ms * 0.001 * tempoScale();
        v.wordT = 0.0;
        v.pause = 0.0;
        v.phoneIndex = -1;
        nextPhone (v);
    }

    void nextPhone (Voice& v)
    {
        const auto& w = words[(size_t) v.wordIndex];
        // Trasig: ibland upprepas ljudet (hack), ibland tappas det, ibland hoppar tonen.
        const float b = params.broken;
        if (v.phoneIndex >= 0 && random.nextFloat() < b * 0.3f) { /* samma ljud igen */ }
        else ++v.phoneIndex;
        if (v.phoneIndex >= 0 && v.phoneIndex < (int) w.size() && random.nextFloat() < b * 0.08f) ++v.phoneIndex;
        v.pitchJump = random.nextFloat() < b * 0.12f ? (random.nextBool() ? 7.0f : -5.0f) : (random.nextFloat() < 0.5f ? v.pitchJump : 0.0f);

        if (v.phoneIndex >= (int) w.size())
        {
            if (params.repeat && v.keyDown)
            {
                v.pause = 0.18 * tempoScale();
                v.phoneIndex = -1;
                v.wordT = 0.0;
                return;
            }
            v.phoneIndex = (int) w.size();   // ordet är slut: klinga av
            return;
        }
        const auto& p = w[(size_t) v.phoneIndex];
        v.phoneT = 0.0;
        v.phoneLen = p.ms * 0.001 * tempoScale() * (0.85 + 0.3 * random.nextFloat() * b);
        v.phoneHasBurst = p.plosive;
        v.burst = 0.0f;
    }

    float renderVoice (Voice& v)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const auto& w = words[(size_t) v.wordIndex];
        const bool speaking = v.phoneIndex >= 0 && v.phoneIndex < (int) w.size();

        // Paus mellan upprepningar.
        if (v.pause > 0.0)
        {
            v.pause -= 1.0 / sr;
            v.amp *= 0.995f;
            if (v.pause <= 0.0) { v.phoneIndex = -1; nextPhone (v); }
            if (! v.keyDown && v.pause > 0.0) { v.active = false; return 0.0f; }
        }

        Phone target = speaking ? w[(size_t) v.phoneIndex] : phone ("_");
        float voiced = target.voiced, noise = target.noise;
        if (speaking && target.plosive)
        {
            // Smäll: först tystnad (stängd mun), sedan en kort stöt brus, sedan aspiration.
            const double closure = v.phoneLen * 0.55;
            if (v.phoneT < closure) { voiced *= 0.1f; noise = 0.0f; }
            else if (v.phoneT < closure + 0.012) noise *= 1.1f;
            else noise *= 0.35f;
        }

        // Formanter och nivåer glider mot nästa ljud (koartikulation).
        const float glide = speaking && target.plosive ? 0.02f : 0.006f;
        if (speaking)
        {
            v.fNow[0] += (target.f1 - v.fNow[0]) * glide;
            v.fNow[1] += (target.f2 - v.fNow[1]) * glide;
            v.fNow[2] += (target.f3 - v.fNow[2]) * glide;
            if (target.noiseHz > 0.0f) v.noiseHzNow = target.noiseHz;
        }
        const float levelGlide = 0.004f;
        v.voicedNow += ((speaking ? voiced : 0.0f) - v.voicedNow) * levelGlide;
        v.noiseNow  += ((speaking ? noise : 0.0f) - v.noiseNow) * (speaking && target.plosive ? 0.05f : levelGlide);

        if ((v.counter++ & 15) == 0)
        {
            static constexpr float bw[numFormants] = { 80.0f, 100.0f, 130.0f, 160.0f, 200.0f };
            static constexpr float gainDb[numFormants] = { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f };
            const float fr[numFormants] = { v.fNow[0], v.fNow[1], v.fNow[2], 3300.0f, 3800.0f };
            for (int k = 0; k < numFormants; ++k)
            {
                const float hz = juce::jmin (fr[k], (float) sr * 0.45f);
                const float q = juce::jlimit (0.5f, 20.0f, hz / bw[k] * 0.5f);
                v.formants[(size_t) k].setCutoffFrequency (hz);
                v.formants[(size_t) k].setResonance (q);
                v.gain[(size_t) k] = juce::Decibels::decibelsToGain (gainDb[k]) / q;
            }
            v.fric.setCutoffFrequency (juce::jmin (v.noiseHzNow, (float) sr * 0.45f));
            v.fric.setResonance (1.2f);
        }

        // Tonhöjd: tangenten, en sjunkande satsmelodi (mindre ju mer monoton) och trasighetens hopp.
        const float progress = (float) juce::jlimit (0.0, 1.0, v.wordT / juce::jmax (0.05, v.wordLen));
        const float contour = (1.0f - params.monotone) * (2.0f - 5.0f * progress);
        const float rRoll = target.voiced > 0.6f && w.size() > 0 && speaking && target.f3 < 1800.0f
                          ? (float) std::sin (twoPi * 28.0 * v.phoneT) : 0.0f;    // rullande R
        const double hz = 440.0 * std::pow (2.0, (v.pitchNote + contour + v.pitchJump - 69.0) / 12.0);
        v.phase += hz / sr;
        v.phase -= std::floor (v.phase);

        // Röstkällan: stämband (mjukad sågtand), surr (rå sågtand) och viskning (brus).
        // Källan lutar nedåt i diskanten som riktiga stämband (−12 dB/oktav för stämbanden, −6 för surret).
        const float saw = (float) (2.0 * v.phase - 1.0);
        v.glottalLp += (saw - v.glottalLp) * 0.18f;
        v.glottalLp2 += (v.glottalLp - v.glottalLp2) * 0.18f;
        v.buzzLp += (saw - v.buzzLp) * 0.45f;
        const float vb = params.voice;
        const float buzzMix = vb < 0.5f ? vb * 2.0f : 1.0f - (vb - 0.5f) * 2.0f;
        const float whisper = juce::jmax (0.0f, vb - 0.5f) * 2.0f;
        const float n = random.nextFloat() * 2.0f - 1.0f;
        const float glottal = (v.glottalLp2 * 1.8f * (1.0f - buzzMix) + v.buzzLp * buzzMix * 0.8f) * (1.0f - whisper) + n * whisper * 0.8f;
        const float source = glottal * v.voicedNow * (1.0f - 0.6f * std::abs (rRoll));

        float y = 0.0f;
        for (int k = 0; k < numFormants; ++k)
            y += v.formants[(size_t) k].processSample (0, source) * v.gain[(size_t) k];
        y = y * 4.0f + v.fric.processSample (0, n) * v.noiseNow * 0.35f;

        // Ordet tar slut: klinga av och frigör rösten.
        const float targetAmp = speaking || v.pause > 0.0 ? 1.0f : 0.0f;
        v.amp += (targetAmp - v.amp) * 0.003f;

        v.phoneT += 1.0 / sr;
        v.wordT += 1.0 / sr;
        if (speaking && v.phoneT >= v.phoneLen) nextPhone (v);
        if (! speaking && v.pause <= 0.0 && v.amp < 0.001f) v.active = false;
        // Högtalaren i automatens bröst: mättar topparna så att smällarna inte sticker iväg.
        return std::tanh (1.6f * y * v.amp * v.velocity) * 0.75f;
    }

    double sr = 44100.0;
    AutomatenParams params;
    std::array<std::vector<Phone>, numAutomatWords> words;
    std::array<Voice, numVoices> voices;
    Sangfageln bird;
    Urverket clock;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x4155544f }; // "AUTO"
};

} // namespace krang
