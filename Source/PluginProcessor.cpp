// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

KrangBoxProcessor::KrangBoxProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "KrangBox", krang::params::createLayout())
{
    pattern.loadDefault();
}

void KrangBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    stegverket.prepare (sampleRate);
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
    const auto params = reader.read();
    engine.setParams (params);

    // Lägg till noter från det virtuella keyboardet (och datorns tangentbord).
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // Stegverket spelar Hammarverket: följer DAW:ens transport, eller egen klocka i den fristående appen.
    if (params.chamber == krang::Chamber::Hammarverket)
    {
        krang::Transport transport;
        if (auto* ph = getPlayHead())
            if (const auto pos = ph->getPosition())
            {
                transport.hostPlaying = pos->getIsPlaying();
                transport.ppq = pos->getPpqPosition().orFallback (0.0);
                transport.bpm = pos->getBpm().orFallback (120.0);
            }
        stegverket.process (pattern, params.stegverket, transport,
                            wrapperType == wrapperType_Standalone, buffer.getNumSamples(), midi);
    }
    else
    {
        stegverket.reset();
    }

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
        engine.handleMidi (meta.getMessage());
    }
    if (pos < total)
        engine.render (left + pos, right + pos, total - pos);

    if (buffer.getNumChannels() == 1) // monobuss: summera vänster och höger
    {
        buffer.addFrom (0, 0, scratchRight, 0, 0, total);
        buffer.applyGain (0, 0, total, 0.5f);
    }

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
    copy.removeChild (copy.getChildWithName ("Stegverket"), nullptr);
    copy.appendChild (juce::ValueTree ("Stegverket").setProperty ("pattern", pattern.toString(), nullptr), nullptr);
    if (auto xml = copy.createXml())
        copyXmlToBinary (*xml, destData);
}

void KrangBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto steg = tree.getChildWithName ("Stegverket");
            if (steg.isValid())
            {
                pattern.fromString (steg["pattern"].toString());
                tree.removeChild (steg, nullptr);
            }
            state.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KrangBoxProcessor();
}
