// Lurblåsaren – KrangBox nionde kammare: blåsinstrument, fysiskt modellerade (fas 3b).
//
// Vågledarsyntes, helt syntetiskt: en fördröjning är instrumentets rör, och ett munstycke driver luftpelaren.
//   Läppar (Näverlur, Bockhorn): läpparna surrar och släpper ut luftpuffar som driver röret; högre tryck
//   ger smalare puffar och ljusare ton, som på en riktig lur. Klockstycket strålar ut diskanten.
//   Rörblad (Skalmeja): ett rörblad som slår mot munstycket, som en klarinett eller säckpipans pipa
//   (Perry Cooks klarinettmodell ur STK).
//   Ångvissla: en ton som vevar upp när ångtrycket byggs upp, med fladder och väsande ånga omkring sig.
// Naturtoner: med påslaget har luren en fast längd (Lurens ton), och läpparna kan bara hitta rörets
// naturtoner – precis som på en riktig näverlur. Tonen du spelar drar mot den närmaste.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace krang
{

enum class WindType { Naverlur = 0, Bockhorn, Skalmeja, Angvissla };

struct LurblasarenParams
{
    WindType type    = WindType::Naverlur;
    float breath     = 0.7f;   // blåstryck 0..1
    float tension    = 0.5f;   // läppspänning / rörbladets styvhet / strålens längd 0..1 (0,5 = i stämning)
    float noise      = 0.2f;   // andning 0..1
    float vibrato    = 0.2f;   // 0..1
    float rasp       = 0.1f;   // skrovlighet 0..1
    bool natural     = false;  // naturtoner (fast rörlängd)
    int key          = 2;      // lurens grundton 0 = C .. 11 = H
    float attackSec  = 0.08f;
    float releaseSec = 0.25f;
};

class Lurblasaren
{
public:
    static constexpr int numVoices = 6;

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& v : voices)
        {
            v.env.setSampleRate (sampleRate);
            v.bore.assign ((size_t) (sampleRate / 20.0 * 2.0) + 16, 0.0f); // räcker för 20 Hz med dubbel längd
        }
        reset();
    }

    void reset()
    {
        for (auto& v : voices) clearVoice (v);
    }

    void setParams (const LurblasarenParams& p)
    {
        params = p;
        const juce::ADSR::Parameters ep { juce::jmax (0.005f, p.attackSec), 0.1f, 0.9f, juce::jmax (0.02f, p.releaseSec) };
        for (auto& v : voices) v.env.setParameters (ep);
    }

    void noteOn (int note, float velocity)
    {
        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == note) target = &v;
        if (target == nullptr) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices) if (v.age < target->age) target = &v;
        }
        auto& v = *target;
        if (! v.active || v.type != params.type) clearVoice (v);
        v.type = params.type;
        v.note = note;
        v.velocity = velocity;
        v.age = ++ageCounter;
        v.active = true;
        v.keyDown = true;
        v.tail = 0;
        v.rise = -250.0f * (0.4f + 0.6f * params.breath); // ångvisslan börjar lågt och vevar upp
        v.env.noteOn();
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.keyDown) { v.env.noteOff(); v.keyDown = false; }
    }

    void allNotesOff()
    {
        for (auto& v : voices) if (v.active) { v.env.noteOff(); v.keyDown = false; }
    }

    void render (float* dest, int numSamples)
    {
        juce::FloatVectorOperations::clear (dest, numSamples);
        for (auto& v : voices)
            if (v.active) renderVoice (v, dest, numSamples);
    }

private:
    struct Voice
    {
        juce::ADSR env;
        std::vector<float> bore;
        int borePos = 0;
        bool active = false, keyDown = false;
        int note = -1, tail = 0;
        float velocity = 0.8f;
        juce::uint32 age = 0;
        WindType type = WindType::Naverlur;
        double vibPhase = 0.0, lipPhase = 0.0;
        float rise = 0.0f, flutter = 0.0f;   // ångvisslans uppvevning (cent) och fladder
        // DC-spärr.
        float dcx = 0, dcy = 0;
        // Rörets reflektionsfilter.
        float lp = 0;
        float out = 0;
    };

    void clearVoice (Voice& v)
    {
        v.env.reset();
        std::fill (v.bore.begin(), v.bore.end(), 0.0f);
        v.borePos = 0;
        v.dcx = v.dcy = v.lp = v.out = v.rise = v.flutter = 0.0f;
        v.lipPhase = 0.0;
        v.active = false; v.keyDown = false; v.note = -1;
    }

    static float readDelay (const std::vector<float>& buf, int writePos, double delay)
    {
        const int size = (int) buf.size();
        double rp = (double) writePos - juce::jlimit (1.0, (double) size - 2.0, delay);
        while (rp < 0) rp += size;
        const int i0 = (int) rp;
        const float f = (float) (rp - i0);
        return buf[(size_t) i0] + f * (buf[(size_t) ((i0 + 1) % size)] - buf[(size_t) i0]);
    }

    float noise() { seed = seed * 1664525u + 1013904223u; return (float) (int) seed * (1.0f / 2147483648.0f); }

    void renderVoice (Voice& v, float* dest, int numSamples)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const double noteHz = 440.0 * std::pow (2.0, (v.note - 69) / 12.0);
        const float tension = juce::jlimit (0.0f, 1.0f, params.tension);
        const float rasp = juce::jlimit (0.0f, 1.0f, params.rasp) + (v.type == WindType::Bockhorn ? 0.25f : 0.0f);
        const float noiseGain = params.noise * 0.3f + rasp * 0.1f;
        const float vibGain = params.vibrato * 0.06f;
        const float pressure = (0.35f + 0.65f * params.breath) * (0.6f + 0.4f * v.velocity);
        const int boreSize = (int) v.bore.size();

        // Rörets längd och munstyckets stämning.
        double boreDelay = 0.0;
        double lipHz = noteHz;
        float sharpness = 4.0f, lossCoef = 0.5f;
        switch (v.type)
        {
            case WindType::Naverlur:
            case WindType::Bockhorn:
            {
                // Med naturtoner har luren en fast längd: läpparna kan bara hitta rörets naturtoner (övertonerna
                // till lurens grundton), så den spelade tonen dras till den närmaste. Annars följer röret tonen.
                if (params.natural && v.type == WindType::Naverlur)
                {
                    const double lurHz = 440.0 * std::pow (2.0, (36 + params.key - 69) / 12.0);
                    const double n = juce::jmax (1.0, std::round (noteHz / lurHz));
                    lipHz = lurHz * n;
                    boreDelay = sr / lurHz;
                }
                else
                {
                    lipHz = noteHz;
                    boreDelay = sr / noteHz;
                }
                lipHz *= std::pow (2.0, (tension - 0.5) * 0.4 / 12.0) * (v.type == WindType::Bockhorn ? 1.003 : 1.0); // spänningen drar tonen
                sharpness = 1.0f + 10.0f * params.breath * (0.6f + 0.4f * v.velocity) + 4.0f * tension;
                lossCoef = v.type == WindType::Bockhorn ? 0.35f : 0.5f;
                break;
            }
            case WindType::Skalmeja:
                boreDelay = sr / noteHz * 0.5 - 1.5;
                break;
            case WindType::Angvissla:
            default:
                boreDelay = 16.0; // ångvisslan använder inget rör
                break;
        }
        boreDelay = juce::jlimit (4.0, (double) boreSize - 4.0, boreDelay);
        const float reedSlope = -0.2f - 0.25f * tension;          // styvare rörblad
        // Ångvisslans väsresonans (Q ≈ 30) och hur fort tonen vevar upp.
        const double wW = twoPi * juce::jmin (noteHz, sr * 0.45) / sr, rW = 1.0 - wW / 60.0;
        const float whistleA1 = (float) (-2.0 * rW * std::cos (wW)), whistleA2 = (float) (rW * rW);
        const float whistleB0 = (float) (1.0 - rW) * 2.0f;
        const float riseDecay = (float) std::exp (-1.0 / (sr * 0.25));
        const float overblow = tension;
        float peak = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = v.env.getNextSample();
            v.vibPhase += 5.2 / sr; v.vibPhase -= std::floor (v.vibPhase);
            const float vib = (float) std::sin (twoPi * v.vibPhase) * vibGain;
            const float breath = pressure * env;
            const float boreOut = readDelay (v.bore, v.borePos, boreDelay);
            float y = 0.0f;

            switch (v.type)
            {
                case WindType::Naverlur:
                case WindType::Bockhorn:
                {
                    // Läpparna öppnar och stänger i takt med tonen: smala pulser vid högt tryck (ljust och skarpt),
                    // breda vid lågt (mjukt). Pulserna driver röret, som klingar med på sina resonanser.
                    v.lipPhase += lipHz / sr; v.lipPhase -= std::floor (v.lipPhase);
                    const float s = (float) std::sin (twoPi * v.lipPhase);
                    const float open = s > 0.0f ? std::pow (s, sharpness) : 0.0f;
                    const float flow = open * breath * (1.0f + vib + noise() * noiseGain);
                    // Röret: återkopplad fördröjning med förluster i klockstycket (lågpass).
                    v.lp += lossCoef * (boreOut - v.lp);
                    float frame = flow + 0.92f * v.lp;
                    if (rasp > 0.001f) frame = std::tanh (frame * (1.0f + 3.0f * rasp)) / (1.0f + 1.5f * rasp);
                    v.bore[(size_t) v.borePos] = juce::jlimit (-4.0f, 4.0f, frame);
                    // Klockstycket strålar ut diskanten: derivera lite, och ta bort likspänning.
                    const float radiated = frame - 0.5f * v.out;
                    v.out = frame;
                    const float dc = radiated - v.dcx + 0.995f * v.dcy;
                    v.dcx = radiated; v.dcy = dc;
                    y = dc * (v.type == WindType::Bockhorn ? 9.0f : 5.0f);
                    break;
                }
                case WindType::Skalmeja:
                {
                    const float bp = env * (0.55f + 0.3f * v.velocity) * (0.7f + 0.5f * params.breath) * (1.0f + noise() * noiseGain + vib);
                    v.lp = 0.5f * (boreOut + v.lp);                          // rörets förluster (enkel nolla)
                    float diff = -0.95f * v.lp - bp;
                    const float reed = juce::jlimit (-1.0f, 1.0f, 0.7f + reedSlope * diff);
                    float frame = bp + diff * reed;
                    if (rasp > 0.001f) frame = std::tanh (frame * (1.0f + 2.0f * rasp));
                    v.bore[(size_t) v.borePos] = juce::jlimit (-2.0f, 2.0f, frame);
                    y = frame * 0.9f;
                    break;
                }
                case WindType::Angvissla:
                default:
                {
                    // Ångvisslan: en ton som vevar upp när ångtrycket byggs upp, med väsande ånga runt sig.
                    v.rise *= riseDecay;
                    v.lipPhase += noteHz * std::exp2 (v.rise / 1200.0f) * (1.0 + vib) / sr;
                    v.lipPhase -= std::floor (v.lipPhase);
                    const double ph = twoPi * v.lipPhase;
                    // Spänningen överblåser: andra och tredje deltonen kommer in.
                    const float tone = (float) (std::sin (ph) + overblow * 0.5 * std::sin (2.0 * ph) + overblow * overblow * 0.35 * std::sin (3.0 * ph));
                    // Fladder: ångan är aldrig helt jämn.
                    v.flutter += 0.002f * (noise() - v.flutter);
                    // Väsandet: brus genom en smal resonans på tonen.
                    const float n = noise();
                    const float hiss = whistleB0 * n - whistleA1 * v.dcx - whistleA2 * v.dcy;
                    v.dcy = v.dcx; v.dcx = hiss;
                    y = 1.6f * breath * (tone * 0.55f * (1.0f + 3.0f * v.flutter) + hiss * (0.6f + 2.0f * noiseGain) + n * noiseGain * 0.15f);
                    break;
                }
            }
            v.borePos = (v.borePos + 1) % boreSize;
            y = std::tanh (y);
            dest[i] += y * 0.5f;
            peak = juce::jmax (peak, std::abs (y));
        }

        // Säkerhet: en luftpelare som skenat (eller gett NaN) tystas.
        if (! std::isfinite (peak)) { clearVoice (v); return; }
        if (! v.env.isActive())
        {
            v.tail += numSamples;
            if (peak < 1.0e-4f || v.tail > (int) (sr * 2.0)) clearVoice (v); // vänta tills röret klingat ut, annars klickar det
        }
    }

    double sr = 44100.0;
    LurblasarenParams params;
    std::array<Voice, numVoices> voices;
    juce::uint32 ageCounter = 0, seed = 0x4c555245; // "LURE"
};

} // namespace krang
