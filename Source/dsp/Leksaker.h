// Doktor Krangs leksaker – figurerna i Automaten utöver talmaskinen (fas 3c).
//
//   Sångfågeln – en mekanisk sångfågel som i de gamla fågelburarna: en liten bälg blåser genom en visselpipa
//                vars kolv flyttas av kammar, så att tonen sveper, drillar och kvittrar. Fyra fåglar sjunger
//                på olika sätt (kanariefågel, näktergal, lärka, trast). Tonen styr hur högt fågeln sjunger.
//   Urverket   – ett urverk som tickar och sjunger: gången tickar och tackar, kugghjulen surrar, och med jämna
//                mellanrum slår klockorna, gökar göken eller spelar klockspelet. Tonen styr klockornas ton.
//                När tangenten trycks dras uret upp (spärrhjulet knattrar); när den släpps går fjädern ut och
//                uret saktar in och stannar.
// Automatens fyra rattar betyder olika saker för varje figur:
//   färg     – fågeln: luft i pipan       uret: klockornas styrka mot tickandet (0 = bara tick)
//   lugn     – fågeln: hur stilla sången är (mindre tonomfång)   uret: hur jämnt det går
//   tempo    – fågeln: kammarnas fart     uret: gångens takt
//   trasig   – fågeln: läckande bälg, hack och falska toner     uret: tappade tick, slirande kugghjul, ostämda klockor
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace krang
{

enum class BirdType { Kanariefagel = 0, Naktergal = 1, Larka = 2, Trast = 3 };
enum class ClockType { Fickur = 0, Pendelur, Gokur, Klockspelsur };

struct LeksakParams
{
    float colour = 0.4f, calm = 0.6f, tempo = 0.5f, broken = 0.1f;
    BirdType bird = BirdType::Kanariefagel;
    ClockType clock = ClockType::Pendelur;
};

// --- Sångfågeln ------------------------------------------------------------------

class Sangfageln
{
public:
    static constexpr int numVoices = 3;

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { for (auto& v : voices) v = {}; }
    void setParams (const LeksakParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr) { target = &voices[0]; for (auto& v : voices) if (v.age < target->age) target = &v; }
        auto& v = *target;
        v = {};
        v.active = v.keyDown = true;
        v.note = note;
        v.velocity = 0.5f + 0.5f * velocity;
        v.age = ++ageCounter;
        v.gap = 0.02;                       // kort andhämtning innan fågeln börjar
        v.motifLeft = 0;
    }

    void noteOff (int note) { for (auto& v : voices) if (v.active && v.note == note) v.keyDown = false; }
    void allNotesOff() { for (auto& v : voices) v.keyDown = false; }
    bool isActive() const { for (auto& v : voices) if (v.active) return true; return false; }

    void render (float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (auto& v : voices)
            {
                if (! v.active) continue;
                if (v.gap > 0.0)
                {
                    v.gap -= 1.0 / sr;
                    if (v.gap <= 0.0)
                    {
                        if (! v.keyDown) { v.active = false; continue; }   // sången tar slut efter frasen
                        nextSyllable (v);
                    }
                    v.env *= 0.995f;
                }
                else
                {
                    v.t += 1.0 / sr;
                    if (v.t >= v.len) { v.gap = v.gapLen; continue; }
                }

                // Tonen: stavelsens svep, drill eller kvitter, runt fågelns tonläge.
                const double x = v.len > 0.0 ? v.t / v.len : 0.0;
                double semis = v.from + (v.to - v.from) * x;
                if (v.trillHz > 0.0) semis += v.trillDepth * (std::sin (twoPi * v.trillHz * v.t) > 0.0 ? 1.0 : -1.0) * 0.5;
                if (v.warbleHz > 0.0) semis += v.trillDepth * std::sin (twoPi * v.warbleHz * v.t);
                semis += v.wobble * std::sin (twoPi * 7.0 * v.t);                   // sliten kolv
                const double hz = juce::jmin (sr * 0.42, 440.0 * std::pow (2.0, (v.note + 24 - 69 + semis) / 12.0));
                v.phase += hz / sr;
                v.phase -= std::floor (v.phase);

                // Bälgen: mjukt in och ut för varje stavelse, med lite fladder.
                float target = v.gap > 0.0 ? 0.0f : (float) juce::jmin (1.0, juce::jmin (v.t / 0.006, (v.len - v.t) / 0.012));
                if (v.chip) target *= (float) std::exp (-v.t / 0.02);
                v.env += (target - v.env) * 0.02f;
                const float bellows = 1.0f + 0.12f * (float) std::sin ((double) v.age * 1.7 + twoPi * 11.0 * v.t);

                // Visselpipan: nästan ren ton, en svag överton, och luft (mer med Luft och Trasig).
                const float tone = (float) (std::sin (twoPi * v.phase) + 0.12 * std::sin (2.0 * twoPi * v.phase));
                const float n = random.nextFloat() * 2.0f - 1.0f;
                v.airLp += (n - v.airLp) * 0.3f;
                const float air = (n - v.airLp) * (0.05f + 0.35f * params.colour + 0.3f * params.broken);
                out += (tone * (1.0f - 0.4f * params.colour) + air) * v.env * bellows * v.velocity;
            }
            dest[i] = out * 0.35f;
        }
    }

private:
    struct Voice
    {
        bool active = false, keyDown = false, chip = false;
        int note = 60, motifLeft = 0, repeatLeft = 0;
        float velocity = 1.0f, env = 0.0f, airLp = 0.0f;
        double t = 0.0, len = 0.1, gap = 0.0, gapLen = 0.05, from = 0.0, to = 0.0;
        double trillHz = 0.0, warbleHz = 0.0, trillDepth = 0.0, wobble = 0.0, phase = 0.0;
        // Motivet som trasten upprepar.
        double mFrom = 0.0, mTo = 0.0, mLen = 0.1, mTrill = 0.0;
        juce::uint32 age = 0;
    };

    double rnd (double a, double b) { return a + (b - a) * random.nextDouble(); }

    // Nästa stavelse i sången, efter fågelns sätt att sjunga.
    void nextSyllable (Voice& v)
    {
        const double range = 12.0 * (1.0 - 0.8 * params.calm);
        const double speed = 0.5 + 1.5 * params.tempo;         // kammarnas fart
        v.t = 0.0;
        v.chip = false;
        v.trillHz = v.warbleHz = v.trillDepth = 0.0;
        v.wobble = params.broken * rnd (0.0, 1.5);
        switch (params.bird)
        {
            case BirdType::Kanariefagel:
            {
                // Rullande drillar: samma korta svep upprepas snabbt, sedan en ny drill på en annan höjd.
                if (v.motifLeft <= 0) { v.motifLeft = 5 + random.nextInt (8); v.mFrom = rnd (-range * 0.5, range * 0.5); v.mTo = v.mFrom + rnd (-6.0, 6.0); v.mLen = rnd (0.035, 0.07); }
                --v.motifLeft;
                v.from = v.mFrom; v.to = v.mTo; v.len = v.mLen / speed;
                v.gapLen = (v.motifLeft == 0 ? rnd (0.12, 0.3) : rnd (0.008, 0.02)) / speed;
                break;
            }
            case BirdType::Naktergal:
            {
                // Omväxlande: långa visslingar som stiger, hårda "tjack", snabba drillar.
                const double r = random.nextDouble();
                if (r < 0.3)      { v.from = rnd (-range, 0.0); v.to = v.from + rnd (2.0, 8.0); v.len = rnd (0.25, 0.6); }
                else if (r < 0.55){ v.from = v.to = rnd (-range * 0.5, range * 0.5); v.len = 0.05; v.chip = true; }
                else              { v.from = v.to = rnd (-range * 0.3, range * 0.6); v.len = rnd (0.3, 0.7); v.trillHz = rnd (18.0, 30.0); v.trillDepth = rnd (2.0, 5.0); }
                v.len /= speed;
                v.gapLen = rnd (0.05, 0.25) / speed;
                break;
            }
            case BirdType::Larka:
            {
                // Ett oavbrutet kvitter som sakta stiger.
                v.motifLeft = (v.motifLeft + 1) % 40;
                const double climb = (double) v.motifLeft / 40.0 * range;
                v.from = climb + rnd (-3.0, 3.0); v.to = v.from + rnd (-4.0, 4.0);
                v.len = rnd (0.04, 0.12) / speed;
                v.warbleHz = rnd (25.0, 45.0); v.trillDepth = rnd (0.5, 2.0);
                v.gapLen = rnd (0.004, 0.02) / speed;
                break;
            }
            case BirdType::Trast:
            {
                // Varje motiv sjungs två eller tre gånger, sedan ett nytt.
                if (v.repeatLeft <= 0)
                {
                    v.repeatLeft = 2 + random.nextInt (2);
                    v.mFrom = rnd (-range * 0.6, range * 0.6); v.mTo = v.mFrom + rnd (-7.0, 7.0); v.mLen = rnd (0.08, 0.25);
                    v.mTrill = random.nextDouble() < 0.4 ? rnd (15.0, 25.0) : 0.0;
                }
                --v.repeatLeft;
                v.from = v.mFrom; v.to = v.mTo; v.len = v.mLen / speed;
                v.trillHz = v.mTrill; v.trillDepth = v.mTrill > 0.0 ? 3.0 : 0.0;
                v.gapLen = (v.repeatLeft == 0 ? rnd (0.25, 0.5) : rnd (0.06, 0.1)) / speed;
                break;
            }
        }
        // Trasig: ibland en falsk ton eller en tappad stavelse.
        if (random.nextFloat() < params.broken * 0.3f) { v.from += rnd (-7.0, 7.0); v.to += rnd (-7.0, 7.0); }
        if (random.nextFloat() < params.broken * 0.15f) v.len *= 0.2;
    }

    double sr = 44100.0;
    LeksakParams params;
    std::array<Voice, numVoices> voices {};
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x46414745 }; // "FAGE"
};

// --- Urverket ----------------------------------------------------------------------

class Urverket
{
public:
    static constexpr int numVoices = 2, numChimes = 6;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec { sampleRate, 64, 1 };
        for (auto& v : voices)
            for (auto* f : { &v.tickA, &v.tickB, &v.casing, &v.whirr })
            {
                f->prepare (spec);
                f->setType (juce::dsp::StateVariableTPTFilterType::bandpass);
            }
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.active = false;
            for (auto* f : { &v.tickA, &v.tickB, &v.casing, &v.whirr }) f->reset();
            for (auto& c : v.chimes) c.active = false;
        }
    }

    void setParams (const LeksakParams& p) { params = p; }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr) { target = &voices[0]; for (auto& v : voices) if (v.age < target->age) target = &v; }
        auto& v = *target;
        v.active = v.keyDown = true;
        v.note = note;
        v.velocity = 0.5f + 0.5f * velocity;
        v.age = ++ageCounter;
        v.windUp = 0.45;               // spärrhjulet knattrar när uret dras upp
        v.speed = 1.0;
        v.tickTimer = 0.5;             // första ticket kommer strax efter uppdragningen
        v.ticks = 0;
        v.tock = false;
        v.click = 0.0f;
        v.melodyLeft = 0;
        setupFilters (v);
    }

    void noteOff (int note) { for (auto& v : voices) if (v.active && v.note == note) v.keyDown = false; }
    void allNotesOff() { for (auto& v : voices) v.keyDown = false; }
    bool isActive() const { for (auto& v : voices) if (v.active) return true; return false; }

    void render (float* dest, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float out = 0.0f;
            for (auto& v : voices)
                if (v.active) out += renderVoice (v);
            dest[i] = out;
        }
    }

private:
    struct Chime { bool active = false; float amp = 0.0f; double t = 0.0; std::array<double, 4> phase {}; double hz = 440.0; bool whistle = false; double len = 0.2; };

    struct Voice
    {
        bool active = false, keyDown = false, tock = false;
        int note = 60, ticks = 0, melodyLeft = 0, melodyIndex = 0;
        float velocity = 1.0f, click = 0.0f;
        double windUp = 0.0, speed = 1.0, tickTimer = 0.0, melodyTimer = 0.0, ratchet = 0.0;
        juce::dsp::StateVariableTPTFilter<float> tickA, tickB, casing, whirr;
        std::array<Chime, numChimes> chimes {};
        juce::uint32 age = 0;
    };

    void setupFilters (Voice& v)
    {
        const float nyq = (float) sr * 0.45f;
        const float scale = params.clock == ClockType::Fickur ? 1.6f : params.clock == ClockType::Pendelur ? 0.7f : 1.0f;
        v.tickA.setCutoffFrequency (juce::jmin (nyq, 3200.0f * scale)); v.tickA.setResonance (8.0f);
        v.tickB.setCutoffFrequency (juce::jmin (nyq, 5100.0f * scale)); v.tickB.setResonance (10.0f);
        v.casing.setCutoffFrequency (820.0f * scale);                    v.casing.setResonance (3.0f);
        v.whirr.setCutoffFrequency (juce::jmin (nyq, 1800.0f * scale));  v.whirr.setResonance (1.5f);
    }

    // Takten i tick per sekund: pendeluret går långsamt, fickuret fort.
    double tickRate() const
    {
        const double base = 0.5 * std::pow (2.0, params.tempo * 3.0);   // 0,5–4 Hz
        switch (params.clock)
        {
            case ClockType::Fickur:   return base * 2.5;
            case ClockType::Pendelur: return base * 0.6;
            default:                  return base;
        }
    }

    void ring (Voice& v, int semis, bool whistle, double len)
    {
        for (auto& c : v.chimes)
            if (! c.active)
            {
                const float wear = params.broken * (random.nextFloat() * 2.0f - 1.0f) * 0.6f;   // ostämda klockor
                c = {};
                c.active = true;
                c.hz = 440.0 * std::pow (2.0, (v.note + semis + wear - 69) / 12.0);
                c.whistle = whistle;
                c.len = len;
                c.amp = 1.0f;
                return;
            }
    }

    // Vad uret spelar när det är dags: varje urtyp har sin egen sång.
    void startMelody (Voice& v)
    {
        switch (params.clock)
        {
            case ClockType::Fickur:       v.melodyLeft = 3; break;    // repeterverkets små pling
            case ClockType::Pendelur:     v.melodyLeft = 4; break;    // Westminster, fyra slag
            case ClockType::Gokur:        v.melodyLeft = 2; break;    // ko-ko
            case ClockType::Klockspelsur: v.melodyLeft = 7; break;    // klockspelets lilla visa
        }
        v.melodyIndex = 0;
        v.melodyTimer = 0.0;
    }

    void melodyNote (Voice& v)
    {
        static constexpr int westminster[2][4] = { { 4, 0, 2, -5 }, { -5, 2, 4, 0 } };
        static constexpr int glock[7] = { 12, 16, 19, 24, 19, 16, 12 };
        switch (params.clock)
        {
            case ClockType::Fickur:       ring (v, 24, false, 0.6); v.melodyTimer = 0.18; break;
            case ClockType::Pendelur:     ring (v, westminster[(v.ticks / 16) % 2][v.melodyIndex], false, 3.0); v.melodyTimer = 0.75; break;
            case ClockType::Gokur:        ring (v, v.melodyIndex == 0 ? 12 : 8, true, 0.22); v.melodyTimer = 0.38; break;
            case ClockType::Klockspelsur: ring (v, glock[v.melodyIndex], false, 1.2); v.melodyTimer = 0.14; break;
        }
        ++v.melodyIndex;
        --v.melodyLeft;
    }

    float renderVoice (Voice& v)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const float n = random.nextFloat() * 2.0f - 1.0f;
        float excite = 0.0f;

        // Uppdragningen: spärrhjulets knatter.
        if (v.windUp > 0.0)
        {
            v.windUp -= 1.0 / sr;
            v.ratchet += 22.0 / sr;
            if (v.ratchet >= 1.0) { v.ratchet -= 1.0; v.click = 0.5f; }
        }
        // Fjädern går ut när tangenten släpps: uret saktar in och stannar.
        if (! v.keyDown) v.speed *= 1.0 - 1.2 / sr;

        // Gången: tick och tack, med ojämnhet och tappade tick.
        if (v.windUp <= 0.0 && v.speed > 0.08)
        {
            v.tickTimer -= v.speed / sr;
            if (v.tickTimer <= 0.0)
            {
                const double jitter = (1.0 - params.calm) * 0.18 * (random.nextDouble() * 2.0 - 1.0);
                v.tickTimer += (1.0 + jitter) / tickRate();
                const bool skip = random.nextFloat() < params.broken * 0.12f;
                if (! skip) { v.click = 1.0f; v.tock = ! v.tock; }
                if (random.nextFloat() < params.broken * 0.08f) v.tickTimer *= 0.15;   // kugghjulet slirar: dubbeltick
                ++v.ticks;
                const int every = params.clock == ClockType::Fickur ? 12 : params.clock == ClockType::Klockspelsur ? 16 : params.clock == ClockType::Gokur ? 12 : 8;
                if ((v.ticks == 2 || v.ticks % every == 0) && v.keyDown) startMelody (v);   // första sången strax efter uppdragningen
            }
        }
        if (v.click > 0.0f) { excite = n * v.click; v.click *= 0.6f; if (v.click < 0.001f) v.click = 0.0f; }

        // Klockornas sång.
        if (v.melodyLeft > 0)
        {
            v.melodyTimer -= 1.0 / sr;
            if (v.melodyTimer <= 0.0) melodyNote (v);
        }

        const float tickLevel = 1.0f - 0.5f * params.colour;
        float y = (v.tock ? v.tickA.processSample (0, excite) * 1.2f + v.tickB.processSample (0, excite) * 0.3f
                          : v.tickB.processSample (0, excite) * 1.0f + v.tickA.processSample (0, excite) * 0.5f)
                  + v.casing.processSample (0, excite) * 0.8f;
        y *= tickLevel * 2.2f;
        // Kugghjulen surrar svagt medan uret går.
        y += v.whirr.processSample (0, n) * 0.012f * (float) juce::jmin (1.0, v.speed) * (v.windUp > 0.0 ? 4.0f : 1.0f);

        // Klockorna och gökens pipor.
        float bells = 0.0f;
        for (auto& c : v.chimes)
        {
            if (! c.active) continue;
            c.t += 1.0 / sr;
            if (c.whistle)
            {
                // Gökens pipa: bälg genom en träpipa, mjuk ton med luft.
                c.phase[0] += c.hz / sr; c.phase[0] -= std::floor (c.phase[0]);
                const double env = juce::jmin (1.0, juce::jmin (c.t / 0.02, (c.len - c.t) / 0.04));
                bells += (float) ((std::sin (twoPi * c.phase[0]) + 0.25 * std::sin (2.0 * twoPi * c.phase[0])) * juce::jmax (0.0, env)) * 0.7f + n * 0.05f * (float) juce::jmax (0.0, env);
                if (c.t >= c.len) c.active = false;
            }
            else
            {
                // Klocka: fyra deltoner (ett slag, grundton, ters, oktav) som klingar ut i olika takt.
                static constexpr double ratio[4] = { 0.5, 1.0, 1.19, 2.0 };
                static constexpr float amp[4] = { 0.4f, 1.0f, 0.35f, 0.3f };
                double s = 0.0;
                for (int k = 0; k < 4; ++k)
                {
                    const double hz = c.hz * ratio[k];
                    if (hz > sr * 0.45) continue;
                    c.phase[(size_t) k] += hz / sr; c.phase[(size_t) k] -= std::floor (c.phase[(size_t) k]);
                    s += std::sin (twoPi * c.phase[(size_t) k]) * amp[k] * std::exp (-c.t / (c.len / (1.0 + k * 0.6)));
                }
                bells += (float) s * (float) juce::jmin (1.0, c.t / 0.002) * 0.45f;
                if (c.t > c.len * 2.5) c.active = false;
            }
        }
        y += bells * 1.3f * params.colour;   // Klockor på noll: bara tickandet

        // Uret har stannat och klockorna tystnat: frigör rösten.
        bool ringing = false;
        for (auto& c : v.chimes) ringing = ringing || c.active;
        if (! v.keyDown && v.speed <= 0.08 && ! ringing && v.click == 0.0f) v.active = false;
        return y * v.velocity * 0.5f;
    }

    double sr = 44100.0;
    LeksakParams params;
    std::array<Voice, numVoices> voices;
    juce::uint32 ageCounter = 0;
    juce::Random random { 0x55524b56 }; // "URKV"
};

} // namespace krang
