// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

KrangBoxProcessor::KrangBoxProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "KrangBox", krang::params::createLayout())
{
}

void KrangBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    engine.prepare (sampleRate, samplesPerBlock);
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
    engine.setParams (reader.read());

    // Lägg till noter från det virtuella keyboardet (och datorns tangentbord).
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    float* mono = buffer.getWritePointer (0);
    const int total = buffer.getNumSamples();
    int pos = 0;

    // Rendera fram till varje MIDI-händelse så att noter hamnar sample-exakt.
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, total, meta.samplePosition);
        if (at > pos)
        {
            engine.render (mono + pos, at - pos);
            pos = at;
        }
        engine.handleMidi (meta.getMessage());
    }
    if (pos < total)
        engine.render (mono + pos, total - pos);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, total);

    const float peak = buffer.getMagnitude (0, 0, total);
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

juce::AudioProcessorEditor* KrangBoxProcessor::createEditor()
{
    return new KrangBoxEditor (*this);
}

void KrangBoxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KrangBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KrangBoxProcessor();
}
