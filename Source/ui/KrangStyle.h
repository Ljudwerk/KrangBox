// KrangBox frontpanel (KrangTopPanel). Stilen i övrigt – palett, LookAndFeel, rattar, VFD, visarmätare,
// radiorör, spaken, modulramar – ligger i krang-libs (krang/stil/krang/KrangStyle.h).
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/stil/krang/KrangStyle.h>
#include "Jugend.h"

namespace krang::ui
{

class KrangTopPanel : public juce::Component
{
public:
    KrangTopPanel()
    {
        addAndMakeVisible (display);
        chamber.setColour (juce::ComboBox::backgroundColourId, colours::bakelite);
        chamber.setColour (juce::ComboBox::textColourId, colours::engraving);
        chamber.setColour (juce::ComboBox::outlineColourId, colours::brass);
        chamber.setColour (juce::ComboBox::arrowColourId, colours::brass);
        chamber.setTooltip ("Kammare: vilken ljudmotor som spelar");
        addAndMakeVisible (chamber);

        for (auto* c : { &presetBox })
        {
            c->setColour (juce::ComboBox::backgroundColourId, colours::bakelite);
            c->setColour (juce::ComboBox::textColourId, colours::vfdText);
            c->setColour (juce::ComboBox::outlineColourId, colours::brass);
            c->setColour (juce::ComboBox::arrowColourId, colours::brass);
            addAndMakeVisible (c);
        }
        presetBox.setTextWhenNothingSelected ("(eget ljud)");
        for (auto* b : { &prevPreset, &nextPreset, &savePreset })
        {
            b->setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b->setColour (juce::TextButton::textColourOffId, colours::engraving);
            addAndMakeVisible (b);
        }
        prevPreset.setTooltip (juce::String::fromUTF8 ("Föregående preset"));
        nextPreset.setTooltip (juce::String::fromUTF8 ("Nästa preset"));
        savePreset.setTooltip ("Spara ljudet som eget preset");
        for (auto* k : { &drive, &katedral, &master })
            addAndMakeVisible (k);
        // Stämningen: system, grundton och blå toner, på rad bredvid märkskylten.
        for (auto* c : std::initializer_list<juce::Component*> { &tuning, &tuningRoot, &blue })
            addAndMakeVisible (c);
        tuning.setTooltip (juce::String::fromUTF8 ("Stämning för alla melodiska kammare: liksvävande, ren stämning, pytagoreisk, medeltonig eller naturtoner"));
        tuningRoot.setTooltip (juce::String::fromUTF8 ("Grundtonen som stämningen räknas från. Bordunen = Bordunorgelns bordunton"));
        blue.setSliderStyle (juce::Slider::LinearHorizontal);
        blue.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        blue.setPopupDisplayEnabled (true, true, nullptr);
        blue.setColour (juce::Slider::trackColourId, colours::brass);
        blue.setColour (juce::Slider::thumbColourId, colours::brassLight);
        blue.setColour (juce::Slider::backgroundColourId, colours::bakelite);
        blue.setTooltip (juce::String::fromUTF8 ("Blå toner: tersen och septiman dras mot det neutrala läget mellan moll och dur, som i folkmusik"));
        addAndMakeVisible (meter);
        addAndMakeVisible (lever);
        addAndMakeVisible (tube);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.45f);

        // Mässingslist längst ned och kopparrör som binder ihop rattarna.
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r.removeFromBottom (6.0f).expanded (2.0f, 0.0f), 0.0f, 3.0f);

        if (! knobRow.isEmpty())
        {
            const float y = (float) drive.getBounds().getCentreY() + 8.0f;
            juce::ColourGradient pipe (colours::copper.brighter (0.4f), 0, y - 4.0f, colours::copper.darker (0.6f), 0, y + 4.0f, false);
            g.setGradientFill (pipe);
            g.fillRect (juce::Rectangle<float> ((float) knobRow.getX(), y - 4.0f, (float) knobRow.getWidth(), 8.0f));
        }

        // Märkskylt.
        auto plate = juce::Rectangle<float> (20.0f, 14.0f, 300.0f, 26.0f);
        g.setColour (colours::engraving);
        g.setFont (panelFont (17.0f));
        g.drawText ("DR. KRANGS PATENTERADE LJUDMASKIN", plate.withWidth ((float) getWidth() - 40.0f), juce::Justification::centredLeft);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("PRESET", prevPreset.getBounds().translated (-60, 0).withWidth (54), juce::Justification::centredRight);
        g.drawText (juce::String::fromUTF8 ("STÄMNING"), tuning.getBounds().translated (-66, 0).withWidth (62), juce::Justification::centredRight);
        g.drawText (juce::String::fromUTF8 ("BLÅ"), blue.getBounds().translated (-30, 0).withWidth (26), juce::Justification::centredRight);

        for (auto p : { juce::Point<float> (10.0f, 10.0f), { (float) getWidth() - 10.0f, 10.0f },
                        { 10.0f, (float) getHeight() - 16.0f }, { (float) getWidth() - 10.0f, (float) getHeight() - 16.0f } })
            drawRivet (g, p, 4.0f);
    }

    void resized() override
    {
        background.invalidate();
        // Presetrad uppe till höger: PRESET < [lista] > SPARA
        auto bar = juce::Rectangle<int> (getWidth() - 20 - 400, 14, 400, 24);
        bar.removeFromLeft (58); // plats för etiketten PRESET
        prevPreset.setBounds (bar.removeFromLeft (24));
        bar.removeFromLeft (3);
        savePreset.setBounds (bar.removeFromRight (64));
        bar.removeFromRight (6);
        nextPreset.setBounds (bar.removeFromRight (24));
        bar.removeFromRight (3);
        presetBox.setBounds (bar);

        // Stämningsraden till vänster om presetraden.
        auto tune = juce::Rectangle<int> (getWidth() - 20 - 400 - 8 - 286, 14, 286, 24);
        tuning.setBounds (tune.removeFromLeft (112));
        tune.removeFromLeft (4);
        tuningRoot.setBounds (tune.removeFromLeft (92));
        tune.removeFromLeft (30);
        blue.setBounds (tune);

        auto r = getLocalBounds().reduced (20).withTrimmedTop (30).withTrimmedBottom (2);

        auto left = r.removeFromLeft (250).withSizeKeepingCentre (250, 132);
        display.setBounds (left.removeFromTop (96));
        left.removeFromTop (8);
        chamber.setBounds (left);
        r.removeFromLeft (16);
        meter.setBounds (r.removeFromRight (210).withSizeKeepingCentre (210, 130));
        r.removeFromRight (14);
        tube.setBounds (r.removeFromRight (40).withSizeKeepingCentre (40, 92));
        r.removeFromRight (6);
        lever.setBounds (r.removeFromRight (104).withSizeKeepingCentre (104, 168));
        r.removeFromRight (10);

        knobRow = r;
        const int w = r.getWidth() / 3;
        for (auto* k : { &drive, &katedral, &master })
            k->setBounds (r.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (w, 130), 150));
    }

    VfdDisplay display;
    juce::ComboBox chamber; // kammarväljare: Basmålen, Hammarverket, ...

    // Presetväljare
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "SPARA" };
    juce::ComboBox tuning, tuningRoot;   // Stämning och dess grundton
    juce::Slider blue;                   // Blå toner
    KrangKnob drive { "Krangdrive" }, katedral { "Katedral" }, master { "Master" };
    NeedleMeter meter;
    KrangLever lever;
    TubeGlow tube;

private:
    juce::Rectangle<int> knobRow;
    textures::BackgroundCache background;
};

} // namespace krang::ui
