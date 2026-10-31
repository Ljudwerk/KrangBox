// KrangRender – renderar en basslinga genom KrangEngine till WAV, ett per distortionsläge.
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

    const char* names[] = { "ror", "fuzz", "vikning", "bitkross" };
    int failures = 0;
    float referenceRmsDb = 0.0f;

    for (int mode = 0; mode < 4; ++mode)
    {
        krang::EngineParams p;
        p.distortion.mode = (krang::DistMode) mode;
        // Standardinställningar som i pluginet: byter man läge ska nivån hållas.

        krang::KrangEngine engine;
        engine.prepare (sr, blockSize);
        engine.setParams (p);
        engine.reset();

        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 2, total);

        juce::AudioBuffer<float> out (2, total);
        out.clear();

        auto it = midi.cbegin();
        for (int pos = 0; pos < total; pos += blockSize)
        {
            const int n = juce::jmin (blockSize, total - pos);
            float* d = out.getWritePointer (0, pos);
            int local = 0;
            while (it != midi.cend() && (*it).samplePosition < pos + n)
            {
                const int at = juce::jmax (0, (*it).samplePosition - pos);
                if (at > local) { engine.render (d + local, at - local); local = at; }
                engine.handleMidi ((*it).getMessage());
                ++it;
            }
            if (local < n) engine.render (d + local, n - local);
        }
        out.copyFrom (1, 0, out, 0, 0, total);

        // Kontroller
        const float* d = out.getReadPointer (0);
        double sum = 0.0;
        bool finite = true;
        for (int i = 0; i < total; ++i) { finite &= std::isfinite (d[i]); sum += d[i]; }
        const float peak = out.getMagnitude (0, 0, total);
        const float rms  = out.getRMSLevel (0, 0, total);
        const double dc  = sum / total;
        const float rmsDb = juce::Decibels::gainToDecibels (rms);
        if (mode == 0) referenceRmsDb = rmsDb;
        const bool levelOk = std::abs (rmsDb - referenceRmsDb) <= 1.5f; // inga volymhopp mellan lägen
        const bool ok = finite && peak > 0.05f && peak <= 1.0f && std::abs (dc) < 0.05 && levelOk;
        failures += ok ? 0 : 1;

        std::cout << names[mode] << ": topp " << juce::Decibels::gainToDecibels (peak) << " dBFS, RMS "
                  << juce::Decibels::gainToDecibels (rms) << " dBFS, DC " << dc
                  << ", latens " << engine.getLatencyInSamples() << " samples  " << (ok ? "OK" : "FEL") << "\n";

        auto file = outDir.getChildFile (juce::String ("krangbox_basmalen_") + names[mode] + ".wav");
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (
            new juce::FileOutputStream (file), sr, 2, 24, {}, 0));
        if (writer) writer->writeFromAudioSampleBuffer (out, 0, total);
    }

    return failures == 0 ? 0 : 2;
}
