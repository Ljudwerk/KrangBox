// Fas 0: enkelt grått testgränssnitt. Ersätts av Krang-panelen i fas 2 (Philip).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class KrangBoxEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KrangBoxEditor (KrangBoxProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::GenericAudioProcessorEditor generic;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrangBoxEditor)
};
