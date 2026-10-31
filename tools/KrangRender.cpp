// KrangRender – renderar Basmålen och Hammarverket genom KrangEngine till WAV.
// Används för lyssningstestet vid grind 0 och som regressionstest (NaN, toppnivå, DC).
//
//   KrangRender <utkatalog> [samplerate]
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_audio_formats/juce_audio_formats.h>
#include "dsp/KrangEngine.h"
#include <iostream>

namespace
{
struct Step { int note; double beats; bool legato; };

// En slinga i D-dorisk – mörk folkton, med legatoglid på var fjärde ton.
const Step riff[] = {
    { 38, 0.5, false }, { 38, 0.5, false }, { 45, 0.5, false }, { 48, 0.5, true },
    { 47, 0.5, false }, { 45, 0.5, false }, { 41, 1.0, true  },
    { 38, 0.5, false }, { 50, 0.5, false }, { 38, 0.5, false }, { 43, 0.5, true },
    { 41, 0.5, false }, { 40, 0.5, false }, { 36, 1.0, true  },
};

juce::MidiBuffer buildMidi (double sr, double bpm, int loops, int& totalSamples)
{
    juce::MidiBuffer midi;
    const double spb = sr * 60.0 / bpm;
    double t = 0.0;
    int prevNote = -1;

    for (int l = 0; l < loops; ++l)
        for (const auto& s : riff)
        {
            const int start = (int) t;
            const int len = (int) (s.beats * spb);
            if (s.legato && prevNote >= 0)
            {
                // Legato: nya tonen före gamla släpps -> glid utan omtrigg.
                midi.addEvent (juce::MidiMessage::noteOn (1, s.note, 0.9f), start);
                midi.addEvent (juce::MidiMessage::noteOff (1, prevNote), start + 1);
            }
            else
            {
                if (prevNote >= 0)
                    midi.addEvent (juce::MidiMessage::noteOff (1, prevNote), start - 1);
                midi.addEvent (juce::MidiMessage::noteOn (1, s.note, 0.8f), start);
            }
            prevNote = s.note;
            t += len;
        }

    midi.addEvent (juce::MidiMessage::noteOff (1, prevNote), (int) t - (int) (0.25 * spb));
    totalSamples = (int) t + (int) sr; // en sekund svans
    return midi;
}
// En takt i 6/8-känsla på 16-delar: ångslag och smedja, ramtrumma och träskedar.
juce::MidiBuffer buildDrumMidi (double sr, double bpm, int bars, int& totalSamples)
{
    using krang::Drum;
    struct Hit { int step; int note; float vel; };
    // 16 steg per takt. Noter enligt Hammarverkets MIDI-mappning.
    const Hit pattern[] = {
        { 0, 36, 1.0f }, { 6, 36, 0.8f }, { 10, 36, 0.9f },
        { 4, 38, 0.9f }, { 12, 38, 1.0f }, { 12, 39, 0.6f },
        { 0, 42, 0.7f }, { 2, 42, 0.5f }, { 4, 42, 0.7f }, { 8, 42, 0.7f }, { 10, 42, 0.5f }, { 14, 46, 0.7f },
        { 3, 41, 0.6f }, { 7, 41, 0.8f }, { 15, 41, 0.7f },
        { 5, 75, 0.6f }, { 9, 75, 0.5f }, { 11, 75, 0.6f },
        { 13, 37, 0.7f },
    };
    juce::MidiBuffer midi;
    const double stepLen = sr * 60.0 / bpm / 4.0;
    for (int b = 0; b < bars; ++b)
        for (const auto& h : pattern)
        {
            const int at = (int) ((b * 16 + h.step) * stepLen);
            midi.addEvent (juce::MidiMessage::noteOn (10, h.note, h.vel), at);
            midi.addEvent (juce::MidiMessage::noteOff (10, h.note), at + (int) (stepLen * 0.5));
        }
    totalSamples = (int) (bars * 16 * stepLen + sr * 1.5);
    return midi;
}

juce::AudioBuffer<float> renderEngine (const krang::EngineParams& p, const juce::MidiBuffer& midi,
                                       int total, double sr, int blockSize, int& latency)
{
    krang::KrangEngine engine;
    engine.prepare (sr, blockSize);
    engine.setParams (p);
    engine.reset();
    latency = engine.getLatencyInSamples();

    juce::AudioBuffer<float> out (2, total);
    out.clear();
    auto it = midi.cbegin();
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total - pos);
        float* l = out.getWritePointer (0, pos);
        float* r = out.getWritePointer (1, pos);
        int local = 0;
        while (it != midi.cend() && (*it).samplePosition < pos + n)
        {
            const int at = juce::jmax (0, (*it).samplePosition - pos);
            if (at > local) { engine.render (l + local, r + local, at - local); local = at; }
            engine.handleMidi ((*it).getMessage());
            ++it;
        }
        if (local < n) engine.render (l + local, r + local, n - local);
    }
    return out;
}

struct Stats { float peakDb, rmsDb; double dc; bool finite; };

Stats analyse (const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    const float* d = b.getReadPointer (0);
    double sum = 0.0;
    bool finite = true;
    for (int i = 0; i < n; ++i) { finite &= std::isfinite (d[i]); sum += d[i]; }
    return { juce::Decibels::gainToDecibels (b.getMagnitude (0, n)),
             juce::Decibels::gainToDecibels (0.5f * (b.getRMSLevel (0, 0, n) + b.getRMSLevel (1, 0, n))), sum / n, finite };
}

void writeWav (const juce::File& file, const juce::AudioBuffer<float>& b, double sr)
{
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), sr, 2, 24, {}, 0));
    if (writer) writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

bool report (const char* name, const Stats& s, int latency, bool extraOk = true)
{
    const bool ok = s.finite && s.peakDb > -26.0f && s.peakDb <= 0.0f && std::abs (s.dc) < 0.05 && extraOk;
    std::cout << name << ": topp " << s.peakDb << " dBFS, RMS " << s.rmsDb << " dBFS, DC " << s.dc
              << ", latens " << latency << " samples  " << (ok ? "OK" : "FEL") << "\n";
    return ok;
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Användning: KrangRender <utkatalog> [samplerate]\n";
        return 1;
    }

    const juce::File outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    outDir.createDirectory();
    const double sr = argc > 2 ? juce::String (argv[2]).getDoubleValue() : 48000.0;
    constexpr int blockSize = 256;
    const char* modes[] = { "ror", "fuzz", "vikning", "bitkross" };
    int failures = 0, latency = 0;

    // Basmålen genom alla fyra distortionslägen, med standardinställningar som i pluginet.
    {
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 2, total);
        float referenceRmsDb = 0.0f;
        for (int mode = 0; mode < 4; ++mode)
        {
            krang::EngineParams p;
            p.distortion.mode = (krang::DistMode) mode;
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto s = analyse (out);
            if (mode == 0) referenceRmsDb = s.rmsDb;
            const bool levelOk = std::abs (s.rmsDb - referenceRmsDb) <= 1.5f; // inga volymhopp mellan lägen
            failures += report ((juce::String ("basmalen_") + modes[mode]).toRawUTF8(), s, latency, levelOk) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_basmalen_") + modes[mode] + ".wav"), out, sr);
        }
    }

    // Hammarverket: torrt (för att höra rösterna) och genom Rör och Bitkross.
    {
        int total = 0;
        const auto midi = buildDrumMidi (sr, 96.0, 4, total);
        struct Variant { const char* name; float mix; krang::DistMode mode; };
        const Variant variants[] = { { "torr", 0.0f, krang::DistMode::Ror },
                                     { "ror", 1.0f, krang::DistMode::Ror },
                                     { "bitkross", 1.0f, krang::DistMode::Bitkross } };
        for (const auto& v : variants)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Hammarverket;
            p.distortion.mode = v.mode;
            p.distortion.mix = v.mix;
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            failures += report ((juce::String ("hammarverket_") + v.name).toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_hammarverket_") + v.name + ".wav"), out, sr);
        }
    }

    // Varje trumröst för sig, torrt: för ljuddesign och för att hitta trasiga röster.
    {
        for (int d = 0; d < krang::numDrums; ++d)
        {
            static const int notes[] = { 36, 37, 38, 39, 41, 42, 46, 75 };
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (10, notes[d], 1.0f), 100);
            midi.addEvent (juce::MidiMessage::noteOff (10, notes[d]), 2000);
            krang::EngineParams p;
            p.chamber = krang::Chamber::Hammarverket;
            p.distortion.mix = 0.0f;
            const auto out = renderEngine (p, midi, (int) (sr * 2.0), sr, blockSize, latency);
            const auto s = analyse (out);
            const auto name = juce::String ("rost_") + juce::String (d) + "_" + juce::String::fromUTF8 (krang::drumName (d));
            failures += report (name.toRawUTF8(), s, latency) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + name.replace (" ", "_") + ".wav"), out, sr);
        }
    }

    // Ångfilter och Katedralreverb: varje läge, med högt ställd resonans och stort rum (stresstest för stabilitet).
    {
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 1, total);
        struct Fx { const char* name; krang::FxFilterMode filter; krang::ReverbMode reverb; float mix; };
        const Fx fx[] = { { "lagpass",     krang::FxFilterMode::Lagpass, krang::ReverbMode::Hall,       0.0f },
                          { "hogpass",     krang::FxFilterMode::Hogpass, krang::ReverbMode::Hall,       0.0f },
                          { "formant",     krang::FxFilterMode::Formant, krang::ReverbMode::Hall,       0.0f },
                          { "kam",         krang::FxFilterMode::Kam,     krang::ReverbMode::Hall,       0.0f },
                          { "hall",        krang::FxFilterMode::Av,      krang::ReverbMode::Hall,       0.6f },
                          { "platta",      krang::FxFilterMode::Av,      krang::ReverbMode::Platta,     0.6f },
                          { "rostig_tank", krang::FxFilterMode::Av,      krang::ReverbMode::RostigTank, 0.6f } };
        for (const auto& f : fx)
        {
            krang::EngineParams p;
            p.filter.mode = f.filter;
            p.filter.cutoffHz = f.filter == krang::FxFilterMode::Kam ? 110.0f : 900.0f;
            p.filter.resonance = 0.95f;
            p.filter.vowel = 0.5f;
            p.reverb.mode = f.reverb;
            p.reverb.mix = f.mix;
            p.reverb.size = 1.0f;
            const auto out = renderEngine (p, midi, total + (int) sr * 2, sr, blockSize, latency);
            failures += report ((juce::String ("effekt_") + f.name).toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_effekt_") + f.name + ".wav"), out, sr);
        }
    }

    // Stegverket: timing mot en simulerad DAW och intern klocka, plus en rendering via sekvensern.
    {
        krang::StepPattern pattern;
        pattern.loadDefault();
        const double bpm = 96.0, samplesPerPpq = sr * 60.0 / bpm;
        const int bars = 4, total = (int) (bars * 4 * samplesPerPpq);

        auto run = [&] (bool host, float swing, std::vector<std::pair<int, int>>& hits)
        {
            krang::Stegverket seq;
            seq.prepare (sr);
            krang::StegverketSettings st;
            st.run = true; st.tempo = bpm; st.swing = swing;
            for (int pos = 0; pos < total; pos += blockSize)
            {
                const int n = juce::jmin (blockSize, total - pos);
                krang::Transport t { host, pos / samplesPerPpq, bpm };
                juce::MidiBuffer m;
                seq.process (pattern, st, t, ! host, n, m);
                for (const auto meta : m)
                    hits.push_back ({ pos + meta.samplePosition, meta.getMessage().getNoteNumber() });
            }
        };

        std::vector<std::pair<int, int>> hostHits, internalHits, swingHits;
        run (true, 0.0f, hostHits);
        run (false, 0.0f, internalHits);
        run (true, 0.5f, swingHits);

        auto firstOf = [] (const std::vector<std::pair<int, int>>& v, int note, int after = -1)
        {
            for (auto& h : v) if (h.second == note && h.first > after) return h.first;
            return -1;
        };
        const int expectedHits = 19 * bars;                              // 19 slag per takt i startmönstret
        const int snareAt = firstOf (hostHits, 38);                       // steg 4 = ett slag in
        const int hatOdd = firstOf (swingHits, 42, (int) (0.4 * samplesPerPpq)); // steg 2 (jämnt, ingen swing)
        const int spoonSwing = firstOf (swingHits, 75);                   // steg 5 (udda): 1,25 + 0,125 ppq
        const bool timingOk = (int) hostHits.size() == expectedHits
                           && hostHits.size() == internalHits.size()
                           && std::abs (snareAt - (int) samplesPerPpq) <= 1
                           && std::abs (hatOdd - (int) (0.5 * samplesPerPpq)) <= 1
                           && std::abs (spoonSwing - (int) (1.375 * samplesPerPpq)) <= 1;
        std::cout << "stegverket: " << hostHits.size() << " slag (vantat " << expectedHits << "), intern klocka "
                  << internalHits.size() << ", virvel vid " << snareAt << " (vantat " << (int) samplesPerPpq
                  << "), gung-sked vid " << spoonSwing << " (vantat " << (int) (1.375 * samplesPerPpq) << ")  "
                  << (timingOk ? "OK" : "FEL") << "\n";
        failures += timingOk ? 0 : 1;

        // Rendera ljudet som Stegverket spelar, genom Rör.
        juce::MidiBuffer midi;
        for (auto& h : internalHits)
            midi.addEvent (juce::MidiMessage::noteOn (10, h.second, 0.8f), h.first);
        krang::EngineParams p;
        p.chamber = krang::Chamber::Hammarverket;
        const auto out = renderEngine (p, midi, total + (int) sr, sr, blockSize, latency);
        failures += report ("stegverket_ror", analyse (out), latency) ? 0 : 1;
        writeWav (outDir.getChildFile ("krangbox_stegverket_ror.wav"), out, sr);
    }

    return failures == 0 ? 0 : 2;
}
