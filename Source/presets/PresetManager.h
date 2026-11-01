// PresetManager – fabrikspresets och egna presets.
// Egna presets sparas som .krang-filer (XML) i ~/Library/Application Support/KrangBox/Presets.
// Används bara från meddelandetråden (gränssnittet och värdens programbyte).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"
#include "../dsp/Stegverket.h"
#include "../dsp/TonStegverket.h"

namespace krang
{

class PresetManager
{
public:
    struct Entry
    {
        juce::String name, category;
        bool factory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    static inline const juce::String fileExtension { ".krang" };

    PresetManager (juce::AudioProcessorValueTreeState& s, StepPattern& p, TonPattern* t = nullptr) : state (s), pattern (p), ton (t) { refresh(); }

    // Mönstren sparas som ett barn "Stegverket" i läget: trummönstret och melodin.
    static void writePatterns (juce::ValueTree& tree, const StepPattern& drums, const TonPattern* melody)
    {
        tree.removeChild (tree.getChildWithName ("Stegverket"), nullptr);
        juce::ValueTree steg ("Stegverket");
        steg.setProperty ("pattern", drums.toString(), nullptr);
        if (melody != nullptr) steg.setProperty ("melody", melody->toString(), nullptr);
        tree.appendChild (steg, nullptr);
    }

    // Läser och tar bort barnet. Äldre lägen utan melodi får startmelodin.
    static void readPatterns (juce::ValueTree& tree, StepPattern& drums, TonPattern* melody)
    {
        const auto steg = tree.getChildWithName ("Stegverket");
        if (! steg.isValid()) return;
        drums.fromString (steg["pattern"].toString());
        if (melody != nullptr)
        {
            if (steg.hasProperty ("melody")) melody->fromString (steg["melody"].toString());
            else melody->loadDefault();
        }
        tree.removeChild (steg, nullptr);
    }

    static juce::File userFolder()
    {
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        base = base.getChildFile ("Application Support"); // på macOS pekar userApplicationDataDirectory på ~/Library
       #endif
        return base.getChildFile ("KrangBox").getChildFile ("Presets");
    }

    // Läser in listan på nytt: fabrikspresets först, sedan egna i bokstavsordning.
    void refresh()
    {
        entries.clear();
        const auto& factory = presets::factoryPresets();
        for (int i = 0; i < (int) factory.size(); ++i)
            entries.push_back ({ juce::String::fromUTF8 (factory[(size_t) i].name),
                                 juce::String::fromUTF8 (factory[(size_t) i].category), true, i, {} });

        auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*" + fileExtension);
        files.sort();
        for (auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), juce::String::fromUTF8 ("Egna"), false, -1, f });
    }

    const std::vector<Entry>& getEntries() const { return entries; }
    int getNumFactory() const { return (int) presets::factoryPresets().size(); }

    // Namnet sparas i läget så att det följer med DAW-projektet.
    juce::String getCurrentName() const { return state.state.getProperty ("presetName", "Init Basmålen").toString(); }

    int getCurrentIndex() const
    {
        const auto name = getCurrentName();
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].name == name) return i;
        return -1;
    }

    bool load (int index)
    {
        if (index < 0 || index >= (int) entries.size())
            return false;
        const auto& e = entries[(size_t) index];
        if (e.factory) loadFactory (presets::factoryPresets()[(size_t) e.factoryIndex]);
        else if (! loadFile (e.file)) return false;
        state.state.setProperty ("presetName", e.name, nullptr);
        return true;
    }

    void loadNext (int delta)
    {
        if (entries.empty()) return;
        const int n = (int) entries.size();
        load (((juce::jmax (0, getCurrentIndex()) + delta) % n + n) % n);
    }

    // Sparar nuvarande ljud (alla parametrar, Stegverkets mönster och melodin) som eget preset.
    bool saveUser (const juce::String& rawName)
    {
        auto name = juce::File::createLegalFileName (rawName.trim());
        if (name.isEmpty()) return false;
        // Fabrikspresets kan inte skrivas över: samma namn får ett tillägg.
        for (const auto& e : entries)
            if (e.factory && e.name == name)
                name << " (eget)";
        userFolder().createDirectory();
        auto copy = state.copyState();
        copy.setProperty ("presetName", name, nullptr);
        writePatterns (copy, pattern, ton);
        auto xml = copy.createXml();
        if (xml == nullptr || ! xml->writeTo (userFolder().getChildFile (name + fileExtension)))
            return false;
        state.state.setProperty ("presetName", name, nullptr);
        refresh();
        return true;
    }

private:
    void setParam (juce::RangedAudioParameter& p, float normalised)
    {
        p.beginChangeGesture();
        p.setValueNotifyingHost (normalised);
        p.endChangeGesture();
    }

    void loadFactory (const presets::FactoryPreset& preset)
    {
        // Allt till standard först, så att presets inte ärver från det förra ljudet.
        for (auto* param : state.processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                setParam (*p, p->getDefaultValue());

        for (const auto& [paramId, value] : preset.values)
            if (auto* p = state.getParameter (paramId))
                setParam (*p, p->convertTo0to1 (value));
            else
                jassertfalse; // stavfel i FactoryPresets.h

        if (! preset.pattern.empty())
        {
            pattern.clear();
            for (int lane = 0; lane < juce::jmin ((int) preset.pattern.size(), StepPattern::lanes); ++lane)
            {
                const juce::String row (preset.pattern[(size_t) lane]);
                for (int step = 0; step < StepPattern::maxSteps; ++step)
                    pattern.set (lane, step, row[step % juce::jmax (1, row.length())] - '0');
            }
        }
        if (preset.melody != nullptr && ton != nullptr)
            ton->fromString (juce::String::fromUTF8 (preset.melody));
    }

    bool loadFile (const juce::File& f)
    {
        auto xml = juce::XmlDocument::parse (f);
        if (xml == nullptr || ! xml->hasTagName (state.state.getType()))
            return false;
        auto tree = juce::ValueTree::fromXml (*xml);
        readPatterns (tree, pattern, ton);
        state.replaceState (tree);
        return true;
    }

    juce::AudioProcessorValueTreeState& state;
    StepPattern& pattern;
    TonPattern* ton = nullptr;
    std::vector<Entry> entries;
};

} // namespace krang
