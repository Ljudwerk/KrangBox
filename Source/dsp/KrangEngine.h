// KrangEngine – kopplar ihop kammare och Ångledningen:
// kammare -> Kaosmotorn -> Krangdistortion -> Ångfilter -> Bandmaskin -> Ekokammare -> DC-spärr/master -> Katedralreverb (stereo) -> limiter.
// Delas av pluginet och KrangRender så att lyssningstest låter exakt som pluginet.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Basmalen.h"
#include "Hammarverket.h"
#include "Bordunorgeln.h"
#include "Spolslingan.h"
#include "Stalpiano.h"
#include "Klockgjuteriet.h"
#include "Koren.h"
#include "Radiomasten.h"
#include "Lurblasaren.h"
#include "Lutverket.h"
#include "Vattenpumpen.h"
#include "Modulation.h"
#include "Slingan.h"
#include "TonStegverket.h"
#include "KrangDistortion.h"
#include "Stegverket.h"
#include "Angledningen.h"
#include "Bandmaskin.h"
#include "Kaosmotorn.h"

namespace krang
{

// Vilken kammare (ljudmotor) som spelar. Fler tillkommer i fas 3.
enum class Chamber { Basmalen = 0, Hammarverket, Bordunorgeln, Spolslingan, Stalpiano, Klockgjuteriet, Koren, Radiomasten, Lurblasaren, Lutverket, Vattenpumpen };

struct EngineParams
{
    Chamber chamber = Chamber::Basmalen;
    BasmalenParams basmalen;
    HammarverketParams hammarverket;
    BordunorgelnParams bordunorgeln;
    SpolslinganParams spolslingan;
    StalpianoParams stalpiano;
    KlockgjuterietParams klockor;
    KorenParams koren;
    RadiomastenParams radio;
    LurblasarenParams lur;
    LutverketParams lut;
    VattenpumpenParams pump;
    SlinganSettings slingan;
    StegverketSettings stegverket;
    TonSettings ton;
    DistortionParams distortion;
    FxFilterParams filter;
    KaosParams kaos;
    TapeParams tape;
    EchoParams echo;
    ReverbParams reverb;
    float masterDb = 0.0f;
    ModParams mod;
    double bpm = 120.0;        // DAW:ens tempo när den spelar, annars Stegverkets; används av synkat eko
    float krangspaken = 0.0f; // 0..1, redan inräknad i fälten ovan av applyKrangspaken()
    // Spakens läge som det står på panelen. Sätts av Reader; motorn lägger då på spaken själv, efter
    // modulationen (så att modulationen kan dra i spaken). −1 = redan inräknad (applyKrangspaken anropad).
    float krangspakenRaw = -1.0f;
};

// Krangspaken: en makrospak som drar hela maskinen mot sönderfall på en gång.
// Ändrar de vanliga parametrarna i stället för att ha egen DSP, så att allt den gör syns
// och hörs som vanliga inställningar. Nivån kompenseras så att spaken inte bara blir en volymratt.
inline void applyKrangspaken (EngineParams& e, float k)
{
    k = juce::jlimit (0.0f, 1.0f, k);
    e.krangspaken = k;
    if (k <= 0.0f) return;

    auto toward = [k] (float value, float target, float amount = 1.0f) { return value + (target - value) * k * amount; };

    auto& d = e.distortion;
    d.driveDb  = juce::jmin (60.0f, d.driveDb + 24.0f * k);
    d.mix      = toward (d.mix, 1.0f);
    d.bits     = toward (d.bits, 3.0f, 0.7f);
    d.downsample = toward (d.downsample, 12.0f, 0.7f);
    d.outputDb -= 6.0f * k;

    auto& b = e.basmalen;
    b.drift     = b.drift + 2.0f * k;               // upp till ±75 cent: maskinen tappar stämningen
    b.resonance = toward (b.resonance, 0.9f, 0.6f);
    b.filterDrive = toward (b.filterDrive, 4.0f);

    e.hammarverket.rost = toward (e.hammarverket.rost, 1.0f);

    e.bordunorgeln.drift = e.bordunorgeln.drift + 2.0f * k;
    e.bordunorgeln.bow   = toward (e.bordunorgeln.bow, 1.0f, 0.6f);

    e.spolslingan.index  = toward (e.spolslingan.index, 10.0f, 0.6f);
    e.spolslingan.grain  = toward (e.spolslingan.grain, 0.6f, 0.8f);
    e.spolslingan.spread = toward (e.spolslingan.spread, 1.0f);

    e.stalpiano.rust        = toward (e.stalpiano.rust, 0.7f, 0.8f);
    e.stalpiano.detuneCents = e.stalpiano.detuneCents + 8.0f * k;   // strängarna glider isär
    e.stalpiano.stiffness   = toward (e.stalpiano.stiffness, 1.0f, 0.5f);

    e.klockor.crack     = toward (e.klockor.crack, 0.7f, 0.7f);  // klockan spricker
    e.klockor.beatCents = e.klockor.beatCents + 10.0f * k;

    e.koren.speaker     = toward (e.koren.speaker, 0.8f, 0.8f);   // kören hörs genom en allt sämre högtalare
    e.koren.spreadCents = e.koren.spreadCents + 15.0f * k;

    e.radio.storm   = toward (e.radio.storm, 0.8f, 0.8f);         // åskan drar in över masten
    e.radio.fading  = toward (e.radio.fading, 0.7f, 0.7f);
    e.radio.drift   = toward (e.radio.drift, 0.9f, 0.6f);

    e.lur.rasp    = toward (e.lur.rasp, 0.8f, 0.8f);              // luren skorrar och väser
    e.lur.noise   = toward (e.lur.noise, 0.6f, 0.6f);
    e.lur.vibrato = toward (e.lur.vibrato, 0.6f, 0.5f);

    e.lut.buzz        = toward (e.lut.buzz, 0.7f, 0.8f);         // strängarna surrar mot rostig metall
    e.lut.courseCents = e.lut.courseCents + 10.0f * k;

    e.pump.pressure = toward (e.pump.pressure, 1.0f, 0.6f);       // stråken trycks hårt: metallen skriker
    e.pump.rust     = toward (e.pump.rust, 0.7f, 0.7f);
    e.pump.water    = toward (e.pump.water, 1.0f, 0.5f);

    e.tape.wow     = toward (e.tape.wow, 0.7f, 0.6f);
    e.tape.age     = toward (e.tape.age, 0.8f, 0.6f);
    e.tape.dropout = toward (e.tape.dropout, 0.5f, 0.5f);
    if (e.echo.mix > 0.001f)
        e.echo.feedback = toward (e.echo.feedback, 1.05f, 0.5f); // ekot börjar självsvänga

    // Kaosmotorn vaknar: maskinen börjar hacka även om kaoset var avstängt.
    auto& ka = e.kaos;
    ka.amount  = juce::jmax (ka.amount, toward (ka.amount, 0.4f, 0.8f));
    ka.drift   = juce::jmax (ka.drift, 0.5f * k);
    if (ka.stutter + ka.reverse + ka.crush < 0.3f)
        ka.stutter = juce::jmax (ka.stutter, 0.6f * k);

    e.reverb.mix  = juce::jmax (e.reverb.mix, toward (e.reverb.mix, 0.45f, 0.6f));
    e.reverb.dark = toward (e.reverb.dark, 0.8f, 0.5f);
}

// Lägger på modulationsraden: varje mål får summan av sina kopplingar (ungefär −1..1).
inline void applyModulation (EngineParams& e, const Modulation& m)
{
    auto amount = [&m] (ModDest d) { return m.destValue (d); };
    auto clamp01 = [] (float v) { return juce::jlimit (0.0f, 1.0f, v); };

    if (const float k = amount (ModDest::Klang); k != 0.0f)
    {
        // Klang: det som gör just den här kammaren ljusare eller skarpare (i Hammarverket: rost och stämning).
        switch (e.chamber)
        {
            case Chamber::Basmalen:       e.basmalen.cutoffHz = juce::jlimit (20.0f, 18000.0f, e.basmalen.cutoffHz * std::exp2 (k * 4.0f)); break;
            case Chamber::Hammarverket:   e.hammarverket.rost = clamp01 (e.hammarverket.rost + k);
                                          for (auto& d : e.hammarverket.drums) d.tune = juce::jlimit (-12.0f, 12.0f, d.tune + k * 7.0f);
                                          break;
            case Chamber::Bordunorgeln:   e.bordunorgeln.timbre = clamp01 (e.bordunorgeln.timbre + k * 0.5f); break;
            case Chamber::Spolslingan:    e.spolslingan.index = juce::jlimit (0.0f, 10.0f, e.spolslingan.index + k * 5.0f); break;
            case Chamber::Stalpiano:      e.stalpiano.hardness = clamp01 (e.stalpiano.hardness + k * 0.5f); break;
            case Chamber::Klockgjuteriet: e.klockor.hardness = clamp01 (e.klockor.hardness + k * 0.5f); break;
            case Chamber::Koren:          e.koren.vowel = clamp01 (e.koren.vowel + k * 0.5f); break;
            case Chamber::Radiomasten:    e.radio.bandwidth = clamp01 (e.radio.bandwidth + k * 0.5f); break;
            case Chamber::Lurblasaren:    e.lur.breath = clamp01 (e.lur.breath + k * 0.5f); break;
            case Chamber::Lutverket:      e.lut.brightness = clamp01 (e.lut.brightness + k * 0.5f); break;
            case Chamber::Vattenpumpen:   e.pump.pressure = clamp01 (e.pump.pressure + k * 0.5f); break;
        }
    }
    e.distortion.driveDb = juce::jlimit (0.0f, 60.0f, e.distortion.driveDb + amount (ModDest::Krangdrive) * 24.0f);
    e.filter.cutoffHz = juce::jlimit (20.0f, 18000.0f, e.filter.cutoffHz * std::exp2 (amount (ModDest::Filterfrekvens) * 4.0f));
    e.filter.vowel = clamp01 (e.filter.vowel + amount (ModDest::Filtervokal));
    e.masterDb = juce::jlimit (-60.0f, 12.0f, e.masterDb + amount (ModDest::Volym) * 24.0f);
    e.echo.mix = clamp01 (e.echo.mix + amount (ModDest::Eko));
    e.reverb.mix = clamp01 (e.reverb.mix + amount (ModDest::Katedral));
    e.kaos.amount = clamp01 (e.kaos.amount + amount (ModDest::Kaos));
    e.tape.wow = clamp01 (e.tape.wow + amount (ModDest::Bandwow));
    if (e.krangspakenRaw >= 0.0f)
        e.krangspakenRaw = clamp01 (e.krangspakenRaw + amount (ModDest::Krangspaken));
}

class KrangEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        basmalen.prepare (sampleRate, maxBlockSize);
        hammarverket.prepare (sampleRate, maxBlockSize);
        bordunorgeln.prepare (sampleRate, maxBlockSize);
        spolslingan.prepare (sampleRate, maxBlockSize);
        stalpiano.prepare (sampleRate, maxBlockSize);
        klockor.prepare (sampleRate, maxBlockSize);
        koren.prepare (sampleRate, maxBlockSize);
        radio.prepare (sampleRate, maxBlockSize);
        lur.prepare (sampleRate, maxBlockSize);
        lut.prepare (sampleRate, maxBlockSize);
        pump.prepare (sampleRate, maxBlockSize);
        distortion.prepare (sampleRate, maxBlockSize);
        filter.prepare (sampleRate, maxBlockSize);
        kaos.prepare (sampleRate, maxBlockSize);
        tape.prepare (sampleRate, maxBlockSize);
        echo.prepare (sampleRate, maxBlockSize);
        reverb.prepare (sampleRate, maxBlockSize);
        modulation.prepare (sampleRate);
        masterGain.reset (sampleRate, 0.02); // volymen glider, så att modulerad volym inte knastrar
        limiter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 2 });
        limiter.setThreshold (-0.5f);
        limiter.setRelease (80.0f);
        // DC-spärr (~8 Hz): rörläget förspänner signalen och skapar likspänning.
        dcCoef = (float) std::exp (-juce::MathConstants<double>::twoPi * 8.0 / sampleRate);
    }

    void reset()
    {
        basmalen.reset();
        hammarverket.reset();
        bordunorgeln.reset();
        spolslingan.reset();
        stalpiano.reset();
        klockor.reset();
        koren.reset();
        radio.reset();
        lur.reset();
        lut.reset();
        pump.reset();
        distortion.reset();
        filter.reset();
        kaos.reset();
        tape.reset();
        echo.reset();
        reverb.reset();
        limiter.reset();
        modulation.reset();
        dcX = dcY = 0.0f;
    }

    // Tar emot parametrarna som de står på panelen. Modulationen och Krangspaken läggs på i configure().
    void setParams (const EngineParams& p)
    {
        base = p;
        modulation.setParams (p.mod);
        modActive = p.mod.active();
        configure();
    }

    // DAW:ens transport, för LFO:er som är synkade till takten.
    void setTransport (bool playing, double ppq, double bpm) { modulation.setTransport (playing, ppq, bpm); }

    const Modulation& getModulation() const { return modulation; }

private:
    void configure()
    {
        EngineParams p = base;
        if (modActive) applyModulation (p, modulation);
        if (p.krangspakenRaw >= 0.0f) applyKrangspaken (p, p.krangspakenRaw);
        if (p.chamber != chamber)
        {
            // Byte av kammare: tysta den gamla så att inga toner hänger kvar.
            basmalen.allNotesOff();
            hammarverket.allNotesOff();
            bordunorgeln.allNotesOff();
            spolslingan.allNotesOff();
            stalpiano.allNotesOff();
            klockor.allNotesOff();
            koren.allNotesOff();
            radio.allNotesOff();
            lur.allNotesOff();
            lut.allNotesOff();
            pump.allNotesOff();
            chamber = p.chamber;
        }
        basmalen.setParams (p.basmalen);
        hammarverket.setParams (p.hammarverket);
        bordunorgeln.setParams (p.bordunorgeln);
        spolslingan.setParams (p.spolslingan);
        stalpiano.setParams (p.stalpiano);
        klockor.setParams (p.klockor);
        koren.setParams (p.koren);
        radio.setParams (p.radio, p.bpm);
        lur.setParams (p.lur);
        lut.setParams (p.lut);
        pump.setParams (p.pump);
        distortion.setParams (p.distortion);
        filter.setParams (p.filter);
        kaos.setParams (p.kaos, p.bpm);
        tape.setParams (p.tape);
        echo.setParams (p.echo, p.bpm);
        reverb.setParams (p.reverb);
        masterGain.setTargetValue (juce::Decibels::decibelsToGain (p.masterDb));
    }

public:
    void handleMidi (const juce::MidiMessage& m)
    {
        modulation.handleMidi (m);
        if (modActive && (m.isNoteOn() || m.isController() || m.isChannelPressure() || m.isAftertouch()))
        {
            // Anslag, modhjul och LFO:er som börjar om ska gälla redan i anslaget – inte först i nästa bit.
            modulation.advance (0);
            configure();
        }
        if (m.isNoteOn())
        {
            switch (chamber)
            {
                case Chamber::Hammarverket: hammarverket.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Bordunorgeln: bordunorgeln.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Spolslingan:  spolslingan.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Stalpiano:    stalpiano.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Klockgjuteriet: klockor.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Koren:        koren.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Radiomasten:  radio.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Lurblasaren:  lur.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Lutverket:    lut.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Vattenpumpen: pump.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
                case Chamber::Basmalen:     basmalen.noteOn (m.getNoteNumber(), m.getFloatVelocity()); break;
            }
        }
        else if (m.isNoteOff())
        {
            switch (chamber)
            {
                case Chamber::Hammarverket: hammarverket.noteOff (m.getNoteNumber()); break;
                case Chamber::Bordunorgeln: bordunorgeln.noteOff (m.getNoteNumber()); break;
                case Chamber::Spolslingan:  spolslingan.noteOff (m.getNoteNumber()); break;
                case Chamber::Stalpiano:    stalpiano.noteOff (m.getNoteNumber()); break;
                case Chamber::Klockgjuteriet: klockor.noteOff (m.getNoteNumber()); break;
                case Chamber::Koren:        koren.noteOff (m.getNoteNumber()); break;
                case Chamber::Radiomasten:  radio.noteOff (m.getNoteNumber()); break;
                case Chamber::Lurblasaren:  lur.noteOff (m.getNoteNumber()); break;
                case Chamber::Lutverket:    lut.noteOff (m.getNoteNumber()); break;
                case Chamber::Vattenpumpen: pump.noteOff (m.getNoteNumber()); break;
                case Chamber::Basmalen:     basmalen.noteOff (m.getNoteNumber()); break;
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            basmalen.allNotesOff();
            hammarverket.allNotesOff();
            bordunorgeln.allNotesOff();
            spolslingan.allNotesOff();
            stalpiano.allNotesOff();
            klockor.allNotesOff();
            koren.allNotesOff();
            radio.allNotesOff();
            lur.allNotesOff();
            lut.allNotesOff();
            pump.allNotesOff();
        }
        else if (m.isSustainPedalOn() || m.isSustainPedalOff())
        {
            stalpiano.setSustain (m.isSustainPedalOn()); // högerpedalen: bara Stålpianot har dämpare
        }
        else if (m.isPitchWheel())
        {
            basmalen.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
        }
    }

    // Renderar stereo. Med modulation påslagen i bitar om 64 sample, och parametrarna läggs om mellan bitarna.
    void render (float* left, float* right, int numSamples)
    {
        if (! modActive)
        {
            modulation.advance (numSamples);
            renderChunk (left, right, numSamples);
            return;
        }
        for (int pos = 0; pos < numSamples; pos += modChunk)
        {
            const int n = juce::jmin (modChunk, numSamples - pos);
            modulation.advance (n);
            configure();
            renderChunk (left + pos, right + pos, n);
        }
    }

private:
    static constexpr int modChunk = 64;

    // Kammaren och distortion/filter i mono i left, reverben sprider till left/right.
    void renderChunk (float* left, float* right, int numSamples)
    {
        switch (chamber)
        {
            case Chamber::Hammarverket: hammarverket.render (left, numSamples); break;
            case Chamber::Bordunorgeln: bordunorgeln.render (left, numSamples); break;
            case Chamber::Spolslingan:  spolslingan.render (left, numSamples); break;
            case Chamber::Stalpiano:    stalpiano.render (left, numSamples); break;
            case Chamber::Klockgjuteriet: klockor.render (left, numSamples); break;
            case Chamber::Koren:        koren.render (left, numSamples); break;
            case Chamber::Radiomasten:  radio.render (left, numSamples); break;
            case Chamber::Lurblasaren:  lur.render (left, numSamples); break;
            case Chamber::Lutverket:    lut.render (left, numSamples); break;
            case Chamber::Vattenpumpen: pump.render (left, numSamples); break;
            case Chamber::Basmalen:     basmalen.render (left, numSamples); break;
        }
        kaos.process (left, numSamples);
        distortion.process (left, numSamples);
        filter.process (left, numSamples);
        tape.process (left, numSamples);
        echo.process (left, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = left[i];
            dcY = x - dcX + dcCoef * dcY;
            dcX = x;
            left[i] = dcY * masterGain.getNextValue();
        }

        reverb.process (left, left, right, numSamples);

        float* chans[] = { left, right };
        juce::dsp::AudioBlock<float> block (chans, 2, (size_t) numSamples);
        limiter.process (juce::dsp::ProcessContextReplacing<float> (block));

        // Säkerhetstak: limitern saknar framförhållning och kan släppa igenom enstaka toppar.
        constexpr float ceiling = 0.966f; // -0,3 dBFS
        juce::FloatVectorOperations::clip (left, left, -ceiling, ceiling, numSamples);
        juce::FloatVectorOperations::clip (right, right, -ceiling, ceiling, numSamples);
    }

public:
    juce::uint32 getKaosEventCount() const { return kaos.getEventCount(); }

    int getLatencyInSamples() const { return distortion.getLatencyInSamples(); }

private:
    Chamber chamber = Chamber::Basmalen;
    Basmalen basmalen;
    Hammarverket hammarverket;
    Bordunorgeln bordunorgeln;
    Spolslingan spolslingan;
    Stalpiano stalpiano;
    Klockgjuteriet klockor;
    Koren koren;
    Radiomasten radio;
    Lurblasaren lur;
    Lutverket lut;
    Vattenpumpen pump;
    KrangDistortion distortion;
    AngFilter filter;
    Kaosmotorn kaos;
    Bandmaskin tape;
    Ekokammare echo;
    Katedralreverb reverb;
    juce::dsp::Limiter<float> limiter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> masterGain { 1.0f };
    Modulation modulation;
    EngineParams base;
    bool modActive = false;
    float dcCoef = 0.999f, dcX = 0.0f, dcY = 0.0f;
};

} // namespace krang
