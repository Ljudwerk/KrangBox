// Kaosmotorn (fas 3): slumpmässig drift och glitch, direkt efter kammaren.
//
//   Varje taktslag (synkat till tempot eller fritt i Hz) slår motorn en tärning. Med sannolikheten Kaos
//   händer något under nästa slag, och vad som händer väljs viktat efter tre rattar:
//     Hack       – en bit av det som just spelats upprepas, som en hackande skiva (1/1, 1/2 eller 1/4 slag)
//     Baklänges  – senaste slaget spelas baklänges
//     Söndring   – slaget samplas ned och bitreduceras, som en trasig D/A-omvandlare
//   Drift låter tonhöjden vandra slumpmässigt, som en maskin vars motor går ojämnt.
//   Med Kaos på noll är motorn helt förbikopplad.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

namespace krang
{

struct KaosParams
{
    float amount  = 0.0f;   // 0..1, sannolikhet för en händelse per slag (0 = av)
    int sync      = 3;      // 0 = fri, 1 = 1/4, 2 = 1/8, 3 = 1/16, 4 = 1/32
    float rateHz  = 4.0f;   // slag per sekund i fritt läge
    float drift   = 0.3f;   // 0..1, slumpmässig tonhöjdsvandring
    float stutter = 0.6f;   // vikt för Hack
    float reverse = 0.3f;   // vikt för Baklänges
    float crush   = 0.3f;   // vikt för Söndring
};

class Kaosmotorn
{
public:
    enum class Event { Ingen, Hack, Baklanges, Sondring };

    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        history.assign ((size_t) (sampleRate * 2.5) + 4, 0.0f);   // räcker för ett 1/4-slag i 30 BPM
        driftBuffer.assign ((size_t) (sampleRate * 0.03) + 4, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (history.begin(), history.end(), 0.0f);
        std::fill (driftBuffer.begin(), driftBuffer.end(), 0.0f);
        writePos = driftPos = 0;
        tickCounter = 0;
        event = Event::Ingen;
        eventLeft = 0; eventAge = 0;
        driftValue = driftTarget = 0.0f; driftTimer = 0;
        holdValue = 0.0f; holdCounter = 0;
    }

    void setParams (const KaosParams& p, double bpm)
    {
        params = p;
        static constexpr double beats[5] = { 0.0, 1.0, 0.5, 0.25, 0.125 };
        const double seconds = p.sync > 0 ? beats[juce::jlimit (1, 4, p.sync)] * 60.0 / juce::jlimit (30.0, 400.0, bpm)
                                          : 1.0 / juce::jlimit (0.25, 32.0, (double) p.rateHz);
        tickLength = juce::jmax (64, (int) (seconds * sr));
    }

    bool isActive() const { return params.amount > 0.001f || event != Event::Ingen; }

    // Antal händelser hittills; gränssnittets kaoslampa blinkar när det ändras.
    juce::uint32 getEventCount() const { return eventCount; }

    void process (float* data, int numSamples)
    {
        if (! isActive()) return;
        const int size = (int) history.size();
        const float fadeLen = (float) juce::jmin (128, tickLength / 8);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            history[(size_t) writePos] = dry;

            if (--tickCounter <= 0)
            {
                tickCounter = tickLength;
                if (eventLeft <= 0)
                    startEvent();
            }

            float out = dry;
            if (event != Event::Ingen)
            {
                float wet = dry;
                switch (event)
                {
                    case Event::Hack:
                    {
                        const int k = eventAge % sliceLen;
                        wet = history[(size_t) ((sliceStart + k) % size)];
                        // Mjuka kanter på varje upprepning så att hacket inte klickar.
                        wet *= juce::jmin (1.0f, (float) k / 32.0f, (float) (sliceLen - k) / 32.0f);
                        break;
                    }
                    case Event::Baklanges:
                        wet = history[(size_t) (((sliceStart - eventAge) % size + size) % size)];
                        break;
                    case Event::Sondring:
                        if (--holdCounter <= 0) { holdCounter = holdFactor; holdValue = dry; }
                        wet = std::round (holdValue * crushSteps) / crushSteps;
                        break;
                    case Event::Ingen: break;
                }
                // Tona in och ut händelsen.
                const float g = juce::jlimit (0.0f, 1.0f, juce::jmin ((float) eventAge / fadeLen, (float) eventLeft / fadeLen));
                out = dry + (wet - dry) * g;
                ++eventAge;
                if (--eventLeft <= 0) event = Event::Ingen;
            }

            writePos = (writePos + 1) % size;
            data[i] = processDrift (out); // med Drift på noll blir det bara ett samples fördröjning
        }
    }

private:
    void startEvent()
    {
        event = Event::Ingen;
        if (random.nextFloat() >= params.amount) return;
        const float total = params.stutter + params.reverse + params.crush;
        if (total <= 0.001f) return;

        const int size = (int) history.size();
        float r = random.nextFloat() * total;
        eventAge = 0;
        if ((r -= params.stutter) < 0.0f)
        {
            static constexpr int divisions[3] = { 1, 2, 4 };
            sliceLen = juce::jmax (64, tickLength / divisions[random.nextInt (3)]);
            sliceStart = ((writePos - sliceLen) % size + size) % size;
            eventLeft = tickLength * (1 + random.nextInt (2));
            event = Event::Hack;
        }
        else if ((r -= params.reverse) < 0.0f)
        {
            sliceStart = ((writePos - 1) % size + size) % size;
            eventLeft = tickLength;
            event = Event::Baklanges;
        }
        else
        {
            holdFactor = 4 + random.nextInt (21);
            crushSteps = std::pow (2.0f, (float) (2 + random.nextInt (4))); // 3–6 bitar
            holdCounter = 0;
            eventLeft = tickLength;
            event = Event::Sondring;
        }
        ++eventCount;
    }

    float processDrift (float x)
    {
        const int size = (int) driftBuffer.size();
        driftBuffer[(size_t) driftPos] = x;

        // Slumpvandring: nytt mål med ojämna mellanrum, mjukt utjämnat.
        if (--driftTimer <= 0)
        {
            driftTarget = random.nextFloat() * 2.0f - 1.0f;
            driftTimer = (int) (sr * (0.15 + 0.6 * random.nextFloat()));
        }
        driftValue += (driftTarget - driftValue) * (float) (12.0 / sr);

        // Fördröjningen 0 .. 12 ms * drift * kaos; en förändring i fördröjning ger tonhöjdsvandring.
        const double depth = sr * 0.012 * params.drift * juce::jmin (1.0f, params.amount * 2.0f);
        const double d = 1.0 + depth * (0.5 + 0.5 * driftValue);
        double rp = (double) driftPos - d;
        while (rp < 0) rp += size;
        const int r0 = (int) rp;
        const float f = (float) (rp - r0);
        const float y = driftBuffer[(size_t) r0] + f * (driftBuffer[(size_t) ((r0 + 1) % size)] - driftBuffer[(size_t) r0]);
        driftPos = (driftPos + 1) % size;
        return y;
    }

    double sr = 44100.0;
    KaosParams params;
    std::vector<float> history, driftBuffer;
    int writePos = 0, driftPos = 0, tickLength = 11025, tickCounter = 0;
    Event event = Event::Ingen;
    int eventLeft = 0, eventAge = 0, sliceLen = 1024, sliceStart = 0;
    int holdFactor = 8, holdCounter = 0;
    float holdValue = 0.0f, crushSteps = 16.0f;
    float driftValue = 0.0f, driftTarget = 0.0f;
    int driftTimer = 0;
    juce::uint32 eventCount = 0;
    juce::Random random { 0x4b414f53 }; // "KAOS"
};

} // namespace krang
