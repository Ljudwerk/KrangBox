// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

KrangBoxProcessor::KrangBoxProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "KrangBox", krang::params::createLayout())
{
    pattern.loadDefault();
    melody.loadDefault();
    state.state.setProperty ("presetName", juce::String::fromUTF8 ("Init Basmålen"), nullptr);
}

void KrangBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    stegverket.prepare (sampleRate);
    slingan.prepare (sampleRate);
    tonverk.prepare (sampleRate);
    engine.prepare (sampleRate, samplesPerBlock);
    scratchRight.setSize (1, juce::jmax (1, samplesPerBlock));
    engine.setParams (reader.read());
    engine.reset();
    setLatencySamples (engine.getLatencyInSamples());
}

bool KrangBoxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void KrangBoxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    auto params = reader.read();

    // Var DAW:ens transport står (används av Stegverket, Slingan och synkat eko).
    krang::Transport transport;
    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            transport.hostPlaying = pos->getIsPlaying();
            transport.ppq = pos->getPpqPosition().orFallback (0.0);
            transport.bpm = pos->getBpm().orFallback (120.0);
        }
    if (transport.hostPlaying)
        params.bpm = transport.bpm;
    engine.setParams (params);
    engine.setTransport (transport.hostPlaying, transport.ppq, params.bpm);

    // Lägg till noter från det virtuella keyboardet (och datorns tangentbord).
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // Stegverket spelar Hammarverket: följer DAW:ens transport, eller egen klocka i den fristående appen.
    if (params.chamber == krang::Chamber::Hammarverket)
        stegverket.process (pattern, params.stegverket, transport,
                            wrapperType == wrapperType_Standalone, buffer.getNumSamples(), midi);
    else
        stegverket.reset();

    // Tonstegverket spelar melodin i alla andra kammare (och släpper sina toner när man byter till Hammarverket).
    {
        auto settings = params.stegverket;
        if (params.chamber == krang::Chamber::Hammarverket) settings.run = false;
        tonverk.process (midi, melody, settings, params.ton, transport,
                         wrapperType == wrapperType_Standalone, buffer.getNumSamples());
    }

    // Slingan arpeggierar det som spelas i Spolslingan.
    if (params.chamber == krang::Chamber::Spolslingan)
        slingan.process (midi, params.slingan, transport, buffer.getNumSamples());
    else
        slingan.reset();

    const int total = buffer.getNumSamples();
    if (scratchRight.getNumSamples() < total)
        scratchRight.setSize (1, total, false, false, true); // nödfall: värden bröt mot maxblockstorleken
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : scratchRight.getWritePointer (0);
    int pos = 0;

    // Rendera fram till varje MIDI-händelse så att noter hamnar sample-exakt.
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, total, meta.samplePosition);
        if (at > pos)
        {
            engine.render (left + pos, right + pos, at - pos);
            pos = at;
        }
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) lastNote.store (msg.getNoteNumber(), std::memory_order_relaxed);
        engine.handleMidi (msg);
    }
    if (pos < total)
        engine.render (left + pos, right + pos, total - pos);

    if (buffer.getNumChannels() == 1) // monobuss: summera vänster och höger
    {
        buffer.addFrom (0, 0, scratchRight, 0, 0, total);
        buffer.applyGain (0, 0, total, 0.5f);
    }

    scope.push (buffer.getReadPointer (0), total);
    kaosEvents.store (engine.getKaosEventCount(), std::memory_order_relaxed);
    for (int i = 0; i < (int) modSources.size(); ++i)
        modSources[(size_t) i].store (engine.getModulation().sourceValue ((krang::ModSource) (i + 1)), std::memory_order_relaxed);

    const float peak = buffer.getMagnitude (0, total);
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

juce::AudioProcessorEditor* KrangBoxProcessor::createEditor()
{
    return new KrangBoxEditor (*this);
}

void KrangBoxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto copy = state.copyState();
    krang::PresetManager::writePatterns (copy, pattern, &melody);
    if (auto xml = copy.createXml())
        copyXmlToBinary (*xml, destData);
}

void KrangBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            krang::PresetManager::readPatterns (tree, pattern, &melody);
            state.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KrangBoxProcessor();
}
