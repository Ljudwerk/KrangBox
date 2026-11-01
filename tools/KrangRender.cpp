// KrangRender – renderar Basmålen och Hammarverket genom KrangEngine till WAV.
// Används för lyssningstestet vid grind 0 och som regressionstest (NaN, toppnivå, DC).
//
//   KrangRender <utkatalog> [samplerate]
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_audio_formats/juce_audio_formats.h>
#include "dsp/KrangEngine.h"
#include "Parameters.h"
#include "presets/PresetManager.h"
#include "dsp/Slingan.h"
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

// Ackordföljd för drönmotorer: Dm – G – C – Am, två slag per ackord, legato.
juce::MidiBuffer buildChordMidi (double sr, double bpm, int& totalSamples)
{
    const int chords[4][3] = { { 50, 53, 57 }, { 55, 59, 62 }, { 48, 52, 55 }, { 45, 48, 52 } };
    juce::MidiBuffer midi;
    const int len = (int) (2.0 * sr * 60.0 / bpm);
    for (int c = 0; c < 4; ++c)
        for (int n : chords[c])
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), c * len);
            midi.addEvent (juce::MidiMessage::noteOff (1, n), (c + 1) * len - 1);
        }
    totalSamples = 4 * len + (int) (sr * 3.0);
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
// Minsta möjliga AudioProcessor: bara för att äga parameterträdet, så att presets kan laddas utan DAW.
struct ParamHost : juce::AudioProcessor
{
    ParamHost() : AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo())) {}
    const juce::String getName() const override { return "KrangRender"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState state { *this, nullptr, "KrangBox", krang::params::createLayout() };
};
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

    // Fabrikspresets: ladda varje preset som pluginet gör och rendera ett prov.
    {
        ParamHost host;
        krang::StepPattern pattern;
        krang::PresetManager presets (host.state, pattern);
        krang::params::Reader reader (host.state);
        int bassTotal = 0;
        const auto bassMidi = buildMidi (sr, 112.0, 1, bassTotal);

        for (int i = 0; i < presets.getNumFactory(); ++i)
        {
            presets.load (i);
            const auto p = reader.read();
            juce::MidiBuffer midi;
            int total = bassTotal + (int) sr * 2;

            if (p.chamber == krang::Chamber::Hammarverket)
            {
                // Två takter via Stegverket på egen klocka, i presetets tempo och gung.
                krang::Stegverket seq;
                seq.prepare (sr);
                auto st = p.stegverket;
                st.run = true;
                const int seqTotal = (int) (2 * 4 * sr * 60.0 / st.tempo);
                for (int pos = 0; pos < seqTotal; pos += blockSize)
                {
                    juce::MidiBuffer m;
                    seq.process (pattern, st, {}, true, juce::jmin (blockSize, seqTotal - pos), m);
                    midi.addEvents (m, 0, -1, pos);
                }
                total = seqTotal + (int) sr * 2;
            }
            else if (p.chamber == krang::Chamber::Bordunorgeln || p.chamber == krang::Chamber::Stalpiano
                     || p.chamber == krang::Chamber::Klockgjuteriet || p.chamber == krang::Chamber::Koren
                     || p.chamber == krang::Chamber::Radiomasten || p.chamber == krang::Chamber::Lurblasaren
                     || p.chamber == krang::Chamber::Lutverket || p.chamber == krang::Chamber::Vattenpumpen)
            {
                midi = buildChordMidi (sr, 80.0, total);
            }
            else if (p.chamber == krang::Chamber::Spolslingan)
            {
                midi = buildChordMidi (sr, 80.0, total);
                if (p.slingan.mode != krang::ArpMode::Av)
                {
                    // Kör ackorden genom Slingan på egen klocka, block för block, som pluginet gör.
                    krang::Slingan arp;
                    arp.prepare (sr);
                    juce::MidiBuffer arped;
                    for (int pos = 0; pos < total; pos += blockSize)
                    {
                        const int n = juce::jmin (blockSize, total - pos);
                        juce::MidiBuffer m;
                        m.addEvents (midi, pos, n, -pos);
                        arp.process (m, p.slingan, {}, n);
                        arped.addEvents (m, 0, n, pos);
                    }
                    midi.swapWith (arped);
                }
            }
            else
            {
                midi = bassMidi;
            }

            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto s = analyse (out);
            const auto label = juce::String ("preset_") + juce::String (i + 1).paddedLeft ('0', 2) + "_"
                             + presets.getEntries()[(size_t) i].name;
            // Ett preset ska höras ordentligt men inte slå i taket hela tiden.
            const bool levelOk = s.rmsDb > -16.0f && s.rmsDb < -7.0f; // presets ska ligga ungefär lika starkt
            failures += report (label.toRawUTF8(), s, latency, levelOk) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label.replace (" ", "_") + ".wav"), out, sr);
        }
    }

    // Slingan: fyra hållna toner, Upp, 1/16 i 120 BPM på egen klocka – rätt ordning och rätt avstånd?
    {
        krang::Slingan arp;
        arp.prepare (sr);
        krang::SlinganSettings st;
        st.mode = krang::ArpMode::Upp; st.rate = 1; st.octaves = 2; st.tempo = 120.0;
        const double stepSamples = sr * 60.0 / 120.0 * 0.25;
        std::vector<std::pair<int, int>> ons;
        const int total = (int) (sr * 2.0); // 4 slag = 16 steg
        for (int pos = 0; pos < total; pos += blockSize)
        {
            juce::MidiBuffer m;
            if (pos == 0) for (int n : { 60, 64, 67, 71 }) m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
            arp.process (m, st, {}, juce::jmin (blockSize, total - pos));
            for (const auto meta : m)
                if (meta.getMessage().isNoteOn()) ons.push_back ({ pos + meta.samplePosition, meta.getMessage().getNoteNumber() });
        }
        const int expected[8] = { 60, 64, 67, 71, 72, 76, 79, 83 };
        bool ok = ons.size() == 16;
        for (size_t i = 0; ok && i < ons.size(); ++i)
            ok = ons[i].second == expected[i % 8] && std::abs (ons[i].first - (int) std::round ((double) i * stepSamples)) <= 1;
        std::cout << "slingan: " << ons.size() << " toner (vantat 16), ordning och avstand " << (ok ? "OK" : "FEL") << "\n";
        failures += ok ? 0 : 1;
    }

    // Bordunorgeln: åtta röster samtidigt – hur mycket av realtid går åt?
    {
        krang::EngineParams p;
        p.chamber = krang::Chamber::Bordunorgeln;
        p.bordunorgeln.sympathetic = 1.0f;
        p.bordunorgeln.bow = 1.0f;
        juce::MidiBuffer midi;
        for (int n = 0; n < 8; ++n)
            midi.addEvent (juce::MidiMessage::noteOn (1, 40 + n * 5, 0.8f), 0);
        const int total = (int) (sr * 10.0);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
        const double ms = juce::Time::getMillisecondCounterHiRes() - start;
        const double load = ms / 10000.0 * 100.0;
        std::cout << "bordunorgeln_cpu: 8 roster, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
        failures += load < 25.0 ? 0 : 1;
        failures += report ("bordunorgeln_8_roster", analyse (out), latency) ? 0 : 1;
    }

    // Krangspaken i botten: varje preset ska förbli stabilt och inte bli mycket starkare.
    {
        ParamHost host;
        krang::StepPattern pattern;
        krang::PresetManager presets (host.state, pattern);
        krang::params::Reader reader (host.state);
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 1, total);
        for (const char* name : { "Init Basmålen", "Kopparrörsacid", "Nyckelharpsdrön", "Gammal rulle", "Ekon från ödemarken" })
        {
            int i = 0;
            while (i < presets.getNumFactory() && presets.getEntries()[(size_t) i].name != juce::String::fromUTF8 (name)) ++i;
            if (i == presets.getNumFactory()) { std::cout << "saknar preset " << name << "  FEL\n"; ++failures; continue; }
            presets.load (i);
            auto* spak = host.state.getParameter (krang::params::id::krang);
            spak->setValueNotifyingHost (1.0f);
            const auto out = renderEngine (reader.read(), midi, total + (int) sr * 2, sr, blockSize, latency);
            const auto s = analyse (out);
            const auto label = "krangspaken_" + presets.getEntries()[(size_t) i].name;
            failures += report (label.toRawUTF8(), s, latency, s.rmsDb < -5.0f) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label.replace (" ", "_") + ".wav"), out, sr);
        }
    }

    // Ekokammaren: rätt ekotid i synkat läge och i fritt läge (impuls in, första ekot ut).
    {
        auto firstEcho = [&] (const krang::EchoParams& ep, double bpm)
        {
            krang::Ekokammare echo;
            echo.prepare (sr, blockSize);
            echo.setParams (ep, bpm);
            std::vector<float> buf ((size_t) (sr * 3.0), 0.0f);
            echo.process (buf.data(), (int) sr);            // låt tidsglidningen landa först
            std::fill (buf.begin(), buf.end(), 0.0f);
            buf[0] = 1.0f;
            echo.process (buf.data(), (int) buf.size());
            int best = 1;
            for (int i = 1; i < (int) buf.size(); ++i)
                if (std::abs (buf[(size_t) i]) > std::abs (buf[(size_t) best])) best = i;
            return best;
        };
        krang::EchoParams ep;
        ep.mix = 1.0f; ep.feedback = 0.3f;
        ep.sync = 2;                                               // 1/8 i 120 BPM = 250 ms
        const int synced = firstEcho (ep, 120.0);
        ep.sync = 3;                                               // punkterad 1/8 i 90 BPM = 500 ms
        const int dotted = firstEcho (ep, 90.0);
        ep.sync = 0; ep.timeMs = 333.0f;                           // fritt läge
        const int free = firstEcho (ep, 120.0);
        const bool ok = std::abs (synced - (int) (sr * 0.25)) <= 2 && std::abs (dotted - (int) (sr * 0.5)) <= 2
                     && std::abs (free - (int) (sr * 0.333)) <= 2;
        std::cout << "ekokammaren_tid: 1/8@120 " << synced << " (vantat " << (int) (sr * 0.25) << "), 1/8 punkt.@90 " << dotted
                  << " (vantat " << (int) (sr * 0.5) << "), fri 333 ms " << free << "  " << (ok ? "OK" : "FEL") << "\n";
        failures += ok ? 0 : 1;
    }

    // Bandmaskin och Ekokammare i botten: självsvängande eko (återkoppling 110 %) med slitet band.
    // Ska förbli ändligt och begränsat även efter att tonerna tystnat.
    {
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 1, total);
        krang::EngineParams p;
        p.tape = { 1.0f, 1.0f, 1.0f, 1.0f };
        p.echo.mix = 0.6f; p.echo.feedback = 1.1f; p.echo.sync = 3; p.echo.dark = 0.5f;
        p.bpm = 112.0;
        const auto out = renderEngine (p, midi, total + (int) sr * 8, sr, blockSize, latency);
        const auto s = analyse (out);
        // Svansen: sista två sekunderna, när bara ekot lever kvar.
        const int tailStart = out.getNumSamples() - (int) sr * 2;
        const float tailRmsDb = juce::Decibels::gainToDecibels (out.getRMSLevel (0, tailStart, (int) sr * 2));
        std::cout << "eko_sjalvsvangning: svans " << tailRmsDb << " dB RMS\n";
        failures += report ("eko_sjalvsvangning", s, latency, s.rmsDb < -5.0f && tailRmsDb < -6.0f) ? 0 : 1;
        writeWav (outDir.getChildFile ("krangbox_eko_sjalvsvangning.wav"), out, sr);

        // Måttlig inställning, som ett ljud man faktiskt skulle använda.
        p.tape = { 0.4f, 0.3f, 0.5f, 0.0f };
        p.echo.feedback = 0.45f; p.echo.mix = 0.35f;
        const auto out2 = renderEngine (p, midi, total + (int) sr * 3, sr, blockSize, latency);
        failures += report ("bandeko", analyse (out2), latency) ? 0 : 1;
        writeWav (outDir.getChildFile ("krangbox_bandeko.wav"), out2, sr);
    }

    // Bandmaskin + Ekokammare: CPU för hela kedjan med allt påslaget.
    {
        krang::EngineParams p;
        p.tape = { 1.0f, 1.0f, 1.0f, 1.0f };
        p.echo.mix = 0.5f; p.echo.feedback = 0.8f;
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 2, total);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        renderEngine (p, midi, total, sr, blockSize, latency);
        const double load = (juce::Time::getMillisecondCounterHiRes() - start) / ((double) total / sr * 1000.0) * 100.0;
        std::cout << "band_eko_cpu: " << load << " % av en karna i realtid  " << (load < 15.0 ? "OK" : "FEL") << "\n";
        failures += load < 15.0 ? 0 : 1;
    }

    // Kaosmotorn: händelser på taktslagen, inga klick från hack och baklänges, stabil i botten.
    {
        auto runSine = [&] (const krang::KaosParams& kp, double bpm, float& maxStep, juce::uint32& events, int seconds)
        {
            krang::Kaosmotorn kaos;
            kaos.prepare (sr, blockSize);
            kaos.setParams (kp, bpm);
            std::vector<float> buf ((size_t) (sr * seconds));
            for (size_t i = 0; i < buf.size(); ++i)
                buf[i] = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * (double) i / sr);
            for (int pos = 0; pos < (int) buf.size(); pos += blockSize)
                kaos.process (buf.data() + pos, juce::jmin (blockSize, (int) buf.size() - pos));
            maxStep = 0.0f;
            for (size_t i = 1; i < buf.size(); ++i)
                maxStep = juce::jmax (maxStep, std::abs (buf[i] - buf[i - 1]));
            events = kaos.getEventCount();
            return buf;
        };

        // Hack och baklänges, alltid (Kaos 100 %), 1/16 i 120 BPM = 8 slag/s i 10 s = 80 slag.
        krang::KaosParams kp;
        kp.amount = 1.0f; kp.sync = 3; kp.drift = 0.0f; kp.stutter = 1.0f; kp.reverse = 1.0f; kp.crush = 0.0f;
        float maxStep = 0.0f; juce::uint32 events = 0;
        runSine (kp, 120.0, maxStep, events, 10);
        const bool clickOk = maxStep < 0.06f;        // en ren 220 Hz-sinus på 0,5 har steg på 0,014
        const bool countOk = events >= 40 && events <= 80; // ett hack kan räcka två slag
        std::cout << "kaosmotorn_hack: " << events << " handelser pa 80 slag, storsta steg " << maxStep << "  "
                  << (clickOk && countOk ? "OK" : "FEL") << "\n";
        failures += clickOk && countOk ? 0 : 1;

        // Av = helt förbikopplad.
        kp.amount = 0.0f;
        runSine (kp, 120.0, maxStep, events, 1);
        std::cout << "kaosmotorn_av: " << events << " handelser  " << (events == 0 ? "OK" : "FEL") << "\n";
        failures += events == 0 ? 0 : 1;

        // Allt i botten genom hela kedjan, med Basmålen.
        int total = 0;
        const auto midi = buildMidi (sr, 112.0, 2, total);
        krang::EngineParams p;
        p.kaos = { 1.0f, 3, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f };
        p.bpm = 112.0;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
        const double load = (juce::Time::getMillisecondCounterHiRes() - start) / ((double) total / sr * 1000.0) * 100.0;
        const auto s = analyse (out);
        failures += report ("kaosmotorn_botten", s, latency, s.rmsDb < -5.0f) ? 0 : 1;
        writeWav (outDir.getChildFile ("krangbox_kaosmotorn_botten.wav"), out, sr);
        std::cout << "kaosmotorn_cpu (hela kedjan): " << load << " % av en karna i realtid  " << (load < 15.0 ? "OK" : "FEL") << "\n";
        failures += load < 15.0 ? 0 : 1;

        // Måttligt kaos, som man skulle använda det.
        p.kaos = { 0.3f, 3, 4.0f, 0.3f, 0.6f, 0.3f, 0.3f };
        const auto out2 = renderEngine (p, midi, total, sr, blockSize, latency);
        failures += report ("kaosmotorn_mattlig", analyse (out2), latency) ? 0 : 1;
        writeWav (outDir.getChildFile ("krangbox_kaosmotorn_mattlig.wav"), out2, sr);
    }

    // Stålpiano: varje läge, pedalen, och CPU med sexton röster.
    {
        // En folkvisa i D-dorisk ovanpå ackorden, så att både diskant och bas hörs.
        auto pianoMidi = [&] (int& total)
        {
            auto midi = buildChordMidi (sr, 80.0, total);
            const int melody[] = { 62, 65, 69, 67, 65, 64, 62, 60, 62, 69, 72, 71, 69, 67, 65, 62 };
            const int step = (int) (sr * 60.0 / 80.0 * 0.5);
            for (int i = 0; i < 16; ++i)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, melody[i] + 12, 0.5f + 0.4f * (float) (i % 3 == 0)), i * step);
                midi.addEvent (juce::MidiMessage::noteOff (1, melody[i] + 12), (i + 1) * step - 10);
            }
            return midi;
        };
        const char* names[] = { "piano", "hackbrade", "preparerat" };
        for (int mode = 0; mode < 3; ++mode)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Stalpiano;
            p.stalpiano.mode = (krang::StalMode) mode;
            p.distortion.mix = 0.0f;
            int total = 0;
            const auto midi = pianoMidi (total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto label = juce::String ("stalpiano_") + names[mode];
            failures += report (label.toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label + ".wav"), out, sr);
        }

        // Pedalen: en ton släpps efter 0,5 s. Utan pedal ska den tystna, med pedal klinga vidare.
        auto tailDb = [&] (bool pedal)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Stalpiano;
            p.distortion.mix = 0.0f;
            p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            if (pedal) midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 10);
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), (int) (sr * 0.5));
            const auto out = renderEngine (p, midi, (int) (sr * 2.0), sr, blockSize, latency);
            return juce::Decibels::gainToDecibels (out.getRMSLevel (0, (int) (sr * 1.5), (int) (sr * 0.5)), -150.0f);
        };
        const float dry = tailDb (false), held = tailDb (true);
        const bool pedalOk = dry < -60.0f && held > -50.0f;
        std::cout << "stalpiano_pedal: utan pedal " << dry << " dB, med pedal " << held << " dB efter 1 s  "
                  << (pedalOk ? "OK" : "FEL") << "\n";
        failures += pedalOk ? 0 : 1;

        // Sexton röster samtidigt, med pedalen nere, i alla tre lägena.
        for (int mode = 0; mode < 3; ++mode)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Stalpiano;
            p.stalpiano.mode = (krang::StalMode) mode;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
            for (int n = 0; n < 16; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 36 + n * 3, 0.9f), n * 64);
            const int total = (int) (sr * 10.0);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "stalpiano_cpu_" << names[mode] << ": 16 roster, " << load << " % av en karna i realtid  "
                      << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ((juce::String ("stalpiano_16_roster_") + names[mode]).toRawUTF8(), analyse (out), latency) ? 0 : 1;
        }
    }

    // Stålpianots sex strängmaterial: samma ton i varje material. Djurhudens ton ska börja högre och sjunka,
    // stålets ska ligga still, och silke och djurhud ska klinga kortare än stål.
    {
        const char* materials[] = { "stal", "massing", "koppar", "jarn", "silke", "djurhud" };
        // Grundtonens frekvens i ett fönster: finsökning med Hann-fönstrad DFT runt 110 Hz.
        auto pitchHz = [&] (const juce::AudioBuffer<float>& b, double from, double len)
        {
            const float* d = b.getReadPointer (0);
            const int s0 = (int) (from * sr), n = (int) (len * sr);
            double best = 0.0, bestHz = 0.0;
            for (double hz = 100.0; hz <= 125.0; hz += 0.02)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; ++i)
                {
                    const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                    const double ph = juce::MathConstants<double>::twoPi * hz * i / sr;
                    re += d[s0 + i] * w * std::cos (ph);
                    im += d[s0 + i] * w * std::sin (ph);
                }
                const double mag = re * re + im * im;
                if (mag > best) { best = mag; bestHz = hz; }
            }
            return bestHz;
        };
        double tailStal = 0.0;
        bool ok = true;
        for (int m = 0; m < 6; ++m)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Stalpiano;
            p.stalpiano.material = (krang::StalMaterial) m;
            p.stalpiano.hardness = 0.1f;          // mjukt slag: grundtonen dominerar, lätt att mäta
            p.stalpiano.body = 0.0f;
            p.distortion.mix = 0.0f;
            p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 45, 1.0f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 3.0), sr, blockSize, latency);
            const double early = pitchHz (out, 0.01, 0.12), late = pitchHz (out, 0.9, 0.3);
            const double sagCents = 1200.0 * std::log2 (early / late);
            const double tail = out.getRMSLevel (0, (int) (sr * 1.5), (int) (sr * 1.0));
            if (m == 0) tailStal = tail;
            const auto s = analyse (out);
            std::cout << "material_" << materials[m] << ": spanningsfall " << sagCents << " cent, svans "
                      << juce::Decibels::gainToDecibels ((float) tail) << " dB\n";
            ok &= report ((juce::String ("material_") + materials[m]).toRawUTF8(), s, latency);
            if (m == 0) ok &= std::abs (sagCents) < 3.0;
            if (m == 5) ok &= sagCents > 5.0;
            if (m == 4 || m == 5) ok &= tail < tailStal * 0.5;
            writeWav (outDir.getChildFile (juce::String ("krangbox_material_") + materials[m] + ".wav"), out, sr);
        }
        std::cout << "stalpiano_material: " << (ok ? "OK" : "FEL") << "\n";
        failures += ok ? 0 : 1;
    }

    // Klockgjuteriet: varje klocktyp spelar en klockringning, en sprucken klocka i botten, svepet och CPU.
    {
        const char* types[] = { "kyrkklocka", "skalla", "gong", "rals", "skalklocka" };
        auto ringing = [&] (int& total)
        {
            // Fyra klockor i växelringning, D-dorisk, med ett andetag mellan omgångarna.
            juce::MidiBuffer midi;
            const int notes[] = { 62, 60, 57, 55 };
            const int step = (int) (sr * 0.45);
            for (int round = 0; round < 3; ++round)
                for (int i = 0; i < 4; ++i)
                {
                    const int at = (round * 5 + i) * step;
                    midi.addEvent (juce::MidiMessage::noteOn (1, notes[i], 0.7f + 0.1f * (float) (i == 0)), at);
                    midi.addEvent (juce::MidiMessage::noteOff (1, notes[i]), at + step / 2);
                }
            total = 15 * step + (int) (sr * 4.0);
            return midi;
        };
        for (int t = 0; t < 5; ++t)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Klockgjuteriet;
            p.klockor.type = (krang::BellType) t;
            p.distortion.mix = 0.0f;
            int total = 0;
            const auto midi = ringing (total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto label = juce::String ("klocka_") + types[t];
            failures += report (label.toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label + ".wav"), out, sr);

            // Sprucken klocka med Krangspaken i botten: ska förbli stabil.
            p.klockor.crack = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            const auto broken = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto sb = analyse (broken);
            failures += report ((label + "_sprucken").toRawUTF8(), sb, latency, sb.rmsDb < -5.0f) ? 0 : 1;
        }

        // Svepet: en gong med Svep +100 % ska börja lägre och stiga (mätt med nollgenomgångar på brumtonen).
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Klockgjuteriet;
            p.klockor.type = krang::BellType::Kyrkklocka;
            p.klockor.sweep = 1.0f;
            p.klockor.hardness = 0.0f;
            p.distortion.mix = 0.0f;
            p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 57, 1.0f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 3.0), sr, blockSize, latency);
            // Brumtonens frekvens (110 Hz när svepet landat): finsökning med Hann-fönstrad DFT mellan 80 och 125 Hz.
            auto hz = [&] (double from, double len)
            {
                const float* d = out.getReadPointer (0);
                const int s0 = (int) (from * sr), n = (int) (len * sr);
                double best = 0.0, bestHz = 0.0;
                for (double f = 80.0; f <= 125.0; f += 0.05)
                {
                    double re = 0.0, im = 0.0;
                    for (int i = 0; i < n; ++i)
                    {
                        const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                        const double ph = juce::MathConstants<double>::twoPi * f * i / sr;
                        re += d[s0 + i] * w * std::cos (ph);
                        im += d[s0 + i] * w * std::sin (ph);
                    }
                    if (re * re + im * im > best) { best = re * re + im * im; bestHz = f; }
                }
                return bestHz;
            };
            const double early = hz (0.02, 0.15), late = hz (1.5, 0.5);
            const double cents = 1200.0 * std::log2 (late / early);
            const bool ok = cents > 100.0;
            std::cout << "klocka_svep: tonen steg " << cents << " cent  " << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;
        }

        // Tolv gonger samtidigt: den tyngsta klocktypen.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Klockgjuteriet;
            p.klockor.type = krang::BellType::Gong;
            juce::MidiBuffer midi;
            for (int n = 0; n < 12; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 36 + n * 4, 0.9f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "klockgjuteriet_cpu: 12 gonger, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("klockgjuteriet_12_gonger", analyse (out), latency) ? 0 : 1;
        }
    }

    // Kören i schaktet: varje röstläge, vokalerna, högtalaren och CPU.
    {
        const char* kinds[] = { "mansroster", "kvinnoroster", "blandad", "kulning" };
        for (int k = 0; k < 4; ++k)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Koren;
            p.koren.voices = (krang::ChoirVoices) k;
            p.distortion.mix = 0.0f;
            int total = 0;
            auto midi = buildChordMidi (sr, 70.0, total);
            if (k == 3) // kulning: en ensam melodi högt upp
            {
                midi.clear();
                const int notes[] = { 74, 79, 81, 79, 74 };
                const int step = (int) (sr * 0.9);
                for (int i = 0; i < 5; ++i)
                {
                    midi.addEvent (juce::MidiMessage::noteOn (1, notes[i], 0.8f), i * step);
                    midi.addEvent (juce::MidiMessage::noteOff (1, notes[i]), (i + 1) * step - (int) (sr * 0.25));
                }
                total = 5 * step + (int) (sr * 2.0);
            }
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto label = juce::String ("koren_") + kinds[k];
            failures += report (label.toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label + ".wav"), out, sr);
        }

        // Vokalerna ska gå att höra: A har mer energi kring 700 Hz än I (första formanten), och I mer kring 1,9 kHz (andra).
        auto bandDb = [&] (float vowel, double centre)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Koren;
            p.koren.voices = krang::ChoirVoices::Mansroster;
            p.koren.vowel = vowel;
            p.distortion.mix = 0.0f;
            p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.8f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 2.0), sr, blockSize, latency);
            juce::dsp::IIR::Filter<float> bp;
            bp.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, centre, 8.0);
            double sum = 0.0, all = 0.0;
            const float* d = out.getReadPointer (0);
            for (int i = 0; i < out.getNumSamples(); ++i)
            {
                const float y = bp.processSample (d[i]);
                if (i > (int) sr) { sum += (double) y * y; all += (double) d[i] * d[i]; }
            }
            return 10.0 * std::log10 ((sum + 1e-20) / (all + 1e-20)); // bandets andel av hela ljudet
        };
        // A: första formanten kring 650–700 Hz och andra kring 1080. I: första låg, andra uppe vid 1870.
        const double a700 = bandDb (0.0f, 700.0), i700 = bandDb (0.5f, 700.0);
        const double aTilt = bandDb (0.0f, 1870.0) - bandDb (0.0f, 1080.0);
        const double iTilt = bandDb (0.5f, 1870.0) - bandDb (0.5f, 1080.0);
        const bool vowelOk = a700 > i700 + 3.0 && iTilt > aTilt + 6.0;
        std::cout << "koren_vokaler: A-I vid 700 Hz " << (a700 - i700) << " dB, andra formanten (1870 mot 1080 Hz) I "
                  << iTilt << " dB, A " << aTilt << " dB  " << (vowelOk ? "OK" : "FEL") << "\n";
        failures += vowelOk ? 0 : 1;

        // Trasig högtalare i botten med Krangspaken: stabil och inte för stark.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Koren;
            p.koren.speaker = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            int total = 0;
            const auto midi = buildChordMidi (sr, 70.0, total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto st = analyse (out);
            failures += report ("koren_trasig_hogtalare", st, latency, st.rmsDb < -5.0f) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_koren_trasig_hogtalare.wav"), out, sr);
        }

        // Åtta toner med åtta sångare var, högtalaren på.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Koren;
            p.koren.singers = 8;
            p.koren.speaker = 0.5f;
            juce::MidiBuffer midi;
            for (int n = 0; n < 8; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 43 + n * 4, 0.8f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "koren_cpu: 8 toner x 8 sangare, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("koren_64_sangare", analyse (out), latency) ? 0 : 1;
        }
    }

    // Radiomasten: varje sändning, telegrafens takt, storm i botten och CPU.
    {
        const char* modes[] = { "kortvag", "morse", "vind", "interferens" };
        for (int m = 0; m < 4; ++m)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Radiomasten;
            p.radio.mode = (krang::RadioMode) m;
            p.distortion.mix = 0.0f;
            p.bpm = 100.0;
            int total = 0;
            const auto midi = buildChordMidi (sr, 70.0, total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto label = juce::String ("radio_") + modes[m];
            failures += report (label.toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_" + label + ".wav"), out, sr);
        }

        // Telegrafen: ton 60 sänder KRANG. K = streck prick streck; i 120 BPM är en prick 0,125 s.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Radiomasten;
            p.radio.mode = krang::RadioMode::Morse;
            p.radio.noise = 0.0f; p.radio.storm = 0.0f; p.radio.fading = 0.0f; p.radio.drift = 0.0f;
            p.radio.attackSec = 0.002f;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            p.bpm = 120.0;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 1.2), sr, blockSize, latency);
            auto rmsDb = [&] (double a, double b)
            { return juce::Decibels::gainToDecibels (out.getRMSLevel (0, (int) (a * sr), (int) ((b - a) * sr)), -120.0f); };
            const float dash = rmsDb (0.05, 0.33), gap = rmsDb (0.40, 0.47), dot = rmsDb (0.52, 0.60), gap2 = rmsDb (0.66, 0.72);
            const bool ok = dash > gap + 30.0f && dot > gap + 30.0f && dot > gap2 + 30.0f;
            std::cout << "radio_morse: streck " << dash << " dB, mellanrum " << gap << " dB, prick " << dot << " dB  "
                      << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;
        }

        // Storm, fading och drift i botten med Krangspaken: stabilt och inte för starkt.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Radiomasten;
            p.radio.storm = 1.0f; p.radio.noise = 1.0f; p.radio.signal = 1.0f; p.radio.bandwidth = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            int total = 0;
            const auto midi = buildChordMidi (sr, 70.0, total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto st = analyse (out);
            failures += report ("radio_storm", st, latency, st.rmsDb < -5.0f) ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_radio_storm.wav"), out, sr);
        }

        // Åtta stationer samtidigt, i det tyngsta läget.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Radiomasten;
            p.radio.mode = krang::RadioMode::Vind;
            juce::MidiBuffer midi;
            for (int n = 0; n < 8; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + n * 5, 0.8f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "radio_cpu: 8 stationer, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("radio_8_stationer", analyse (out), latency) ? 0 : 1;
        }
    }

    // Lurblåsaren: varje instrument ska ljuda på rätt ton, naturtonerna ska fungera, och det ska vara stabilt i botten.
    {
        // Grundtonen ur ett fönster: finsökning med Hann-fönstrad DFT kring en förväntad ton.
        auto pitchNear = [&] (const juce::AudioBuffer<float>& b, double from, double len, double expect)
        {
            const float* d = b.getReadPointer (0);
            const int s0 = (int) (from * sr), n = (int) (len * sr);
            double best = 0.0, bestHz = 0.0;
            for (double f = expect * 0.8; f <= expect * 1.25; f += expect * 0.0005)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; i += 2)
                {
                    const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                    const double ph = juce::MathConstants<double>::twoPi * f * i / sr;
                    re += d[s0 + i] * w * std::cos (ph);
                    im += d[s0 + i] * w * std::sin (ph);
                }
                if (re * re + im * im > best) { best = re * re + im * im; bestHz = f; }
            }
            return bestHz;
        };
        const char* types[] = { "naverlur", "bockhorn", "skalmeja", "angvissla" };
        const int notes[] = { 57, 57, 57, 69 };
        for (int t = 0; t < 4; ++t)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lurblasaren;
            p.lur.type = (krang::WindType) t;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, notes[t], 0.8f), 0);
            midi.addEvent (juce::MidiMessage::noteOff (1, notes[t]), (int) (sr * 1.5));
            const auto out = renderEngine (p, midi, (int) (sr * 2.0), sr, blockSize, latency);
            const double expect = 440.0 * std::pow (2.0, (notes[t] - 69) / 12.0);
            const double hz = pitchNear (out, 0.6, 0.5, expect);
            const double cents = 1200.0 * std::log2 (hz / expect);
            const float level = juce::Decibels::gainToDecibels (out.getRMSLevel (0, (int) (sr * 0.6), (int) (sr * 0.5)), -120.0f);
            const auto st = analyse (out);
            const bool ok = std::abs (cents) < 60.0 && level > -30.0f;
            std::cout << "lur_" << types[t] << ": " << hz << " Hz (" << cents << " cent fran tonen), nivå " << level << " dB  "
                      << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;
            failures += report ((juce::String ("lur_") + types[t]).toRawUTF8(), st, latency) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_lur_") + types[t] + ".wav"), out, sr);
        }

        // Naturtoner: en lur i D (73,4 Hz) kan bara spela sina övertoner. H3 (247 Hz) dras till tredje deltonen, A3 (220 Hz).
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lurblasaren;
            p.lur.type = krang::WindType::Naverlur;
            p.lur.natural = true; p.lur.key = 2;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 59, 0.8f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 1.2), sr, blockSize, latency);
            const double expect = 3.0 * 440.0 * std::pow (2.0, (38 - 69) / 12.0);
            const double hz = pitchNear (out, 0.6, 0.5, expect);
            const bool ok = std::abs (1200.0 * std::log2 (hz / expect)) < 30.0;
            std::cout << "lur_naturtoner: H3 i en D-lur ger " << hz << " Hz (vantat " << expect << ")  " << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;
        }

        // Allt i botten med Krangspaken, i alla fyra instrumenten: stabilt.
        for (int t = 0; t < 4; ++t)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lurblasaren;
            p.lur.type = (krang::WindType) t;
            p.lur.breath = 1.0f; p.lur.rasp = 1.0f; p.lur.tension = 1.0f; p.lur.noise = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            int total = 0;
            const auto midi = buildChordMidi (sr, 80.0, total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto st = analyse (out);
            failures += report ((juce::String ("lur_botten_") + types[t]).toRawUTF8(), st, latency, st.rmsDb < -4.0f) ? 0 : 1;
        }

        // Sex röster samtidigt.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lurblasaren;
            p.lur.type = krang::WindType::Naverlur;
            juce::MidiBuffer midi;
            for (int n = 0; n < 6; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 45 + n * 5, 0.8f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "lur_cpu: 6 roster, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("lur_6_roster", analyse (out), latency) ? 0 : 1;
        }
    }

    // Lutverket: varje instrument på rätt ton, dämparen, surr i botten och CPU.
    {
        auto pitchNear = [&] (const juce::AudioBuffer<float>& b, double from, double len, double expect)
        {
            const float* d = b.getReadPointer (0);
            const int s0 = (int) (from * sr), n = (int) (len * sr);
            double best = 0.0, bestHz = 0.0;
            for (double f = expect * 0.9; f <= expect * 1.1; f += expect * 0.0002)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; i += 2)
                {
                    const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                    const double ph = juce::MathConstants<double>::twoPi * f * i / sr;
                    re += d[s0 + i] * w * std::cos (ph);
                    im += d[s0 + i] * w * std::sin (ph);
                }
                if (re * re + im * im > best) { best = re * re + im * im; bestHz = f; }
            }
            return bestHz;
        };
        const char* types[] = { "kantele", "luta", "cittra", "rostharpa" };
        for (int t = 0; t < 4; ++t)
        {
            for (int note : { 45, 69, 88 })
            {
                krang::EngineParams p;
                p.chamber = krang::Chamber::Lutverket;
                p.lut.type = (krang::LuteType) t;
                p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
                juce::MidiBuffer midi;
                midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
                const auto out = renderEngine (p, midi, (int) (sr * 0.8), sr, blockSize, latency);
                const double expect = 440.0 * std::pow (2.0, (note - 69) / 12.0);
                const double hz = pitchNear (out, 0.05, 0.4, expect);
                const double cents = 1200.0 * std::log2 (hz / expect);
                const bool ok = std::abs (cents) < (t == 3 ? 25.0 : 10.0); // rostharpan vinglar med flit
                std::cout << "lut_" << types[t] << "_" << note << ": " << hz << " Hz (" << cents << " cent)  " << (ok ? "OK" : "FEL") << "\n";
                failures += ok ? 0 : 1;
            }
            // En slinga plockade toner, till WAV.
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lutverket;
            p.lut.type = (krang::LuteType) t;
            p.distortion.mix = 0.0f;
            juce::MidiBuffer midi;
            const int notes[] = { 50, 57, 62, 65, 69, 65, 62, 57, 50, 57, 62, 64, 65, 64, 62, 57 };
            const int step = (int) (sr * 0.2);
            for (int i = 0; i < 16; ++i)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, notes[i], 0.6f + 0.3f * (float) (i % 4 == 0)), i * step);
                midi.addEvent (juce::MidiMessage::noteOff (1, notes[i]), i * step + step * 3);
            }
            const auto out = renderEngine (p, midi, 16 * step + (int) (sr * 2.0), sr, blockSize, latency);
            failures += report ((juce::String ("lut_") + types[t]).toRawUTF8(), analyse (out), latency) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_lut_") + types[t] + ".wav"), out, sr);
        }

        // Dämparen: en släppt ton tystnar, en hållen klingar vidare.
        auto tailDb = [&] (bool hold)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lutverket;
            p.lut.decaySec = 8.0f;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            if (! hold) midi.addEvent (juce::MidiMessage::noteOff (1, 60), (int) (sr * 0.3));
            const auto out = renderEngine (p, midi, (int) (sr * 1.5), sr, blockSize, latency);
            return juce::Decibels::gainToDecibels (out.getRMSLevel (0, (int) (sr * 1.0), (int) (sr * 0.5)), -150.0f);
        };
        const float released = tailDb (false), held = tailDb (true);
        const bool damperOk = released < held - 30.0f;
        std::cout << "lut_dampare: slappt " << released << " dB, hallen " << held << " dB  " << (damperOk ? "OK" : "FEL") << "\n";
        failures += damperOk ? 0 : 1;

        // Surr i botten med Krangspaken, i alla instrument.
        for (int t = 0; t < 4; ++t)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lutverket;
            p.lut.type = (krang::LuteType) t;
            p.lut.buzz = 1.0f; p.lut.hardness = 1.0f; p.lut.decaySec = 20.0f; p.lut.brightness = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            int total = 0;
            const auto midi = buildChordMidi (sr, 80.0, total);
            const auto out = renderEngine (p, midi, total, sr, blockSize, latency);
            const auto st = analyse (out);
            failures += report ((juce::String ("lut_botten_") + types[t]).toRawUTF8(), st, latency, st.rmsDb < -4.0f) ? 0 : 1;
        }

        // Tolv toner i cittran (tre strängar per ton = 36 strängar).
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Lutverket;
            p.lut.type = krang::LuteType::Cittra;
            p.lut.decaySec = 20.0f;
            juce::MidiBuffer midi;
            for (int n = 0; n < 12; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + n * 3, 0.8f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "lut_cpu: 36 strangar, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("lut_36_strangar", analyse (out), latency) ? 0 : 1;
        }
    }

    // Vattenpumpen: varje metall på rätt ton, sågens glid, allt i botten och CPU.
    {
        auto pitchNear = [&] (const juce::AudioBuffer<float>& b, double from, double len, double lo, double hi)
        {
            const float* d = b.getReadPointer (0);
            const int s0 = (int) (from * sr), n = (int) (len * sr);
            double best = 0.0, bestHz = 0.0;
            for (double f = lo; f <= hi; f += lo * 0.0003)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; i += 2)
                {
                    const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                    const double ph = juce::MathConstants<double>::twoPi * f * i / sr;
                    re += d[s0 + i] * w * std::cos (ph);
                    im += d[s0 + i] * w * std::sin (ph);
                }
                if (re * re + im * im > best) { best = re * re + im * im; bestHz = f; }
            }
            return bestHz;
        };
        const char* types[] = { "vattenfon", "sag", "plat", "cymbal" };
        for (int t = 0; t < 4; ++t)
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Vattenpumpen;
            p.pump.type = (krang::BowedType) t;
            p.pump.water = 0.0f; p.pump.vibrato = 0.0f;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
            const auto out = renderEngine (p, midi, (int) (sr * 1.5), sr, blockSize, latency);
            const double hz = pitchNear (out, 0.6, 0.6, 200.0, 240.0);
            const double cents = 1200.0 * std::log2 (hz / 220.0);
            const float level = juce::Decibels::gainToDecibels (out.getRMSLevel (0, (int) (sr * 0.6), (int) (sr * 0.6)), -120.0f);
            const bool ok = std::abs (cents) < 10.0 && level > -30.0f;
            std::cout << "pump_" << types[t] << ": " << hz << " Hz (" << cents << " cent), niva " << level << " dB  " << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;

            p.pump.water = 0.5f; p.pump.vibrato = 0.3f;
            int total = 0;
            const auto chords = buildChordMidi (sr, 70.0, total);
            const auto music = renderEngine (p, chords, total, sr, blockSize, latency);
            failures += report ((juce::String ("pump_") + types[t]).toRawUTF8(), analyse (music), latency) ? 0 : 1;
            writeWav (outDir.getChildFile (juce::String ("krangbox_pump_") + types[t] + ".wav"), music, sr);

            // Allt i botten med Krangspaken.
            p.pump.pressure = 1.0f; p.pump.speed = 1.0f; p.pump.rust = 1.0f; p.pump.water = 1.0f;
            krang::applyKrangspaken (p, 1.0f);
            const auto wild = renderEngine (p, chords, total, sr, blockSize, latency);
            const auto sw = analyse (wild);
            failures += report ((juce::String ("pump_botten_") + types[t]).toRawUTF8(), sw, latency, sw.rmsDb < -4.0f) ? 0 : 1;
        }

        // Sågens glid: A3 hålls, E4 spelas legato – halvvägs genom glidet ligger tonen mellan dem.
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Vattenpumpen;
            p.pump.type = krang::BowedType::Sag;
            p.pump.glideSec = 1.0f; p.pump.vibrato = 0.0f; p.pump.water = 0.0f;
            p.distortion.mix = 0.0f; p.reverb.mix = 0.0f;
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), (int) (sr * 1.0));
            midi.addEvent (juce::MidiMessage::noteOff (1, 57), (int) (sr * 1.0) + 10);
            const auto out = renderEngine (p, midi, (int) (sr * 3.5), sr, blockSize, latency);
            const double start = pitchNear (out, 0.5, 0.4, 200.0, 360.0), end = pitchNear (out, 2.8, 0.5, 200.0, 360.0);
            const bool ok = std::abs (1200.0 * std::log2 (start / 220.0)) < 15.0 && std::abs (1200.0 * std::log2 (end / 329.63)) < 15.0;
            std::cout << "pump_glid: " << start << " Hz -> " << end << " Hz (vantat 220 -> 329,6)  " << (ok ? "OK" : "FEL") << "\n";
            failures += ok ? 0 : 1;
            writeWav (outDir.getChildFile ("krangbox_pump_sagglid.wav"), out, sr);
        }

        // Åtta plåtar samtidigt (96 resonatorer).
        {
            krang::EngineParams p;
            p.chamber = krang::Chamber::Vattenpumpen;
            p.pump.type = krang::BowedType::Plat;
            juce::MidiBuffer midi;
            for (int n = 0; n < 8; ++n)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + n * 4, 0.8f), n * 64);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = renderEngine (p, midi, (int) (sr * 10.0), sr, blockSize, latency);
            const double load = (juce::Time::getMillisecondCounterHiRes() - start) / 10000.0 * 100.0;
            std::cout << "pump_cpu: 8 platar, " << load << " % av en karna i realtid  " << (load < 25.0 ? "OK" : "FEL") << "\n";
            failures += load < 25.0 ? 0 : 1;
            failures += report ("pump_8_platar", analyse (out), latency) ? 0 : 1;
        }
    }

    return failures == 0 ? 0 : 2;
}
