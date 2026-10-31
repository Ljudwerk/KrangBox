// Krang-panelen (fas 2): modulerna under frontpanelen.
//   BasmalenPanel      – oscillatorer, rörelse, filter, två envelopes (faders)
//   HammarverketPanel  – Stegverket + en rad röstrattar per trumspår, Rost
//   AngledningenPanel  – Krangdistortion -> Ångfilter -> Katedralreverb, sammankopplade med kopparrör
// Panelerna äger bara kontrollerna; editorn kopplar dem till parametrarna via bindings().
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "KrangStyle.h"
#include "StegverketPanel.h"
#include "../dsp/Hammarverket.h"

namespace krang::ui
{

// Vilken kontroll som hör till vilken parameter.
struct Bindings
{
    std::vector<std::pair<juce::Slider*, juce::String>> sliders;
    std::vector<std::pair<juce::ComboBox*, juce::String>> combos;
};

// En vertikal fader med etikett, för envelopes.
class KrangFader : public juce::Component
{
public:
    explicit KrangFader (const juce::String& labelText) : label (labelText)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setPopupDisplayEnabled (true, true, nullptr);
        slider.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (slider);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (colours::engraving);
        g.setFont (panelFont (11.0f));
        g.drawText (label, getLocalBounds().removeFromBottom (16), juce::Justification::centred);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (18).reduced (0, 2)); }

    juce::Slider slider;

private:
    juce::String label;
};

// Lägger ut en rad komponenter jämnt i en rektangel.
inline void layoutRow (juce::Rectangle<int> r, std::initializer_list<juce::Component*> items, int itemWidth)
{
    const int n = (int) items.size();
    if (n == 0) return;
    const int gap = juce::jmax (0, (r.getWidth() - n * itemWidth) / n);
    int x = r.getX() + gap / 2;
    for (auto* c : items)
    {
        c->setBounds (x, r.getY(), itemWidth, r.getHeight());
        x += itemWidth + gap;
    }
}

// --- Basmålen -------------------------------------------------------------------

class BasmalenPanel : public juce::Component
{
public:
    BasmalenPanel()
    {
        for (auto* k : knobs()) addAndMakeVisible (k);
        for (auto* f : faders()) addAndMakeVisible (f);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colours::chassis);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
    }

    void resized() override
    {
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto right = r.removeFromRight (400);
        r.removeFromRight (12);

        // Vänster: två rader rattar.
        auto row1 = r.removeFromTop (r.getHeight() / 2 - 4);
        r.removeFromTop (8);
        auto osc = row1.removeFromLeft (row1.getWidth() * 5 / 7 - 6);
        row1.removeFromLeft (12);
        addFrame (osc, "Oscillatorer");
        addFrame (row1, juce::String::fromUTF8 ("Rörelse"));
        layoutRow (inner (osc), { &saw, &square, &semi, &fine, &sub }, 84);
        layoutRow (inner (row1), { &glide, &drift }, 84);

        addFrame (r, "Filter");
        layoutRow (inner (r), { &cutoff, &resonance, &filterDrive, &envAmount, &keyTrack }, 110);

        // Höger: två envelopes med faders.
        auto ampEnv = right.removeFromTop (right.getHeight() / 2 - 4);
        right.removeFromTop (8);
        addFrame (ampEnv, "Amp-envelope");
        addFrame (right, "Filter-envelope");
        layoutRow (inner (ampEnv), { &ampA, &ampD, &ampS, &ampR }, 60);
        layoutRow (inner (right), { &fltA, &fltD, &fltS, &fltR }, 60);
    }

    Bindings bindings()
    {
        return { { { &saw.slider, "osc1Level" }, { &square.slider, "osc2Level" }, { &semi.slider, "osc2Semi" },
                   { &fine.slider, "osc2Fine" }, { &sub.slider, "subLevel" }, { &glide.slider, "glide" },
                   { &drift.slider, "drift" }, { &cutoff.slider, "cutoff" }, { &resonance.slider, "resonance" },
                   { &filterDrive.slider, "filterDrive" }, { &envAmount.slider, "envAmount" }, { &keyTrack.slider, "keyTrack" },
                   { &ampA.slider, "ampA" }, { &ampD.slider, "ampD" }, { &ampS.slider, "ampS" }, { &ampR.slider, "ampR" },
                   { &fltA.slider, "fltA" }, { &fltD.slider, "fltD" }, { &fltS.slider, "fltS" }, { &fltR.slider, "fltR" } },
                 {} };
    }

private:
    static juce::Rectangle<int> inner (juce::Rectangle<int> frame) { return frame.withTrimmedTop (28).reduced (6, 4); }
    void addFrame (juce::Rectangle<int> r, const juce::String& title) { frames.push_back ({ r, title }); }

    std::vector<KrangKnob*> knobs() { return { &saw, &square, &semi, &fine, &sub, &glide, &drift,
                                               &cutoff, &resonance, &filterDrive, &envAmount, &keyTrack }; }
    std::vector<KrangFader*> faders() { return { &ampA, &ampD, &ampS, &ampR, &fltA, &fltD, &fltS, &fltR }; }

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    KrangKnob saw { u8 ("Sågtand") }, square { "Fyrkant" }, semi { "Halvton" }, fine { "Fin" }, sub { "Sub" },
              glide { "Glid" }, drift { "Drift" },
              cutoff { "Cutoff" }, resonance { "Resonans" }, filterDrive { "Drive" }, envAmount { u8 ("Env-mängd") },
              keyTrack { u8 ("Tangentfölj") };
    KrangFader ampA { "A" }, ampD { "D" }, ampS { "S" }, ampR { "R" },
               fltA { "A" }, fltD { "D" }, fltS { "S" }, fltR { "R" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
};

// --- Hammarverket ---------------------------------------------------------------

class HammarverketPanel : public juce::Component
{
public:
    HammarverketPanel (StepPattern& pattern, std::atomic<int>& playing) : steg (pattern, playing)
    {
        addAndMakeVisible (steg);
        addAndMakeVisible (rost);
        for (int d = 0; d < numDrums; ++d)
            for (auto* k : { &tune[(size_t) d], &decay[(size_t) d], &level[(size_t) d] })
            {
                k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                k->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
                k->setPopupDisplayEnabled (true, true, nullptr);
                k->setDoubleClickReturnValue (true, 0.0);
                k->setTooltip (juce::String::fromUTF8 (drumName (d)));
                addAndMakeVisible (k);
            }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colours::chassis);
        drawModuleFrame (g, voicesFrame.toFloat(), juce::String::fromUTF8 ("Röster"));
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        for (int c = 0; c < 3; ++c)
        {
            static const char* heads[] = { "STÄMNING", "KLANG", "NIVÅ" };
            g.drawText (juce::String::fromUTF8 (heads[c]), juce::Rectangle<int> (colX[c] - 30, headY, 60, 14),
                        juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        voicesFrame = r.removeFromRight (300).reduced (6, 10);
        steg.setBounds (r);

        // Linjera röstrattarna med Stegverkets spår.
        const auto grid = steg.getGridBounds().translated (steg.getX(), steg.getY());
        const float rowH = (float) grid.getHeight() / (float) numDrums;
        const int knob = juce::jmin (30, (int) rowH);
        colX[0] = voicesFrame.getX() + 60;
        colX[1] = voicesFrame.getX() + 130;
        colX[2] = voicesFrame.getX() + 200;
        headY = grid.getY() - 16;
        for (int d = 0; d < numDrums; ++d)
        {
            const int cy = grid.getY() + (int) ((float) d * rowH + rowH * 0.5f);
            tune[(size_t) d].setBounds  (juce::Rectangle<int> (knob, knob).withCentre ({ colX[0], cy }));
            decay[(size_t) d].setBounds (juce::Rectangle<int> (knob, knob).withCentre ({ colX[1], cy }));
            level[(size_t) d].setBounds (juce::Rectangle<int> (knob, knob).withCentre ({ colX[2], cy }));
        }
        rost.setBounds (juce::Rectangle<int> (70, 76).withCentre ({ voicesFrame.getRight() - 38, voicesFrame.getY() + 54 })
                            .withY (voicesFrame.getY() + 4));
    }

    Bindings bindings()
    {
        Bindings b;
        b.sliders.push_back ({ &rost.slider, "hvRost" });
        for (int d = 0; d < numDrums; ++d)
        {
            b.sliders.push_back ({ &tune[(size_t) d],  "hv" + juce::String (d) + "Tune" });
            b.sliders.push_back ({ &decay[(size_t) d], "hv" + juce::String (d) + "Decay" });
            b.sliders.push_back ({ &level[(size_t) d], "hv" + juce::String (d) + "Level" });
        }
        return b;
    }

    StegverketPanel steg;

private:
    KrangKnob rost { "Rost", false };
    std::array<juce::Slider, (size_t) numDrums> tune, decay, level;
    juce::Rectangle<int> voicesFrame;
    int colX[3] {}, headY = 0;
};

// --- Ångledningen ---------------------------------------------------------------

class AngledningenPanel : public juce::Component
{
public:
    AngledningenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &distMode, &distMix, &distBits, &distDown, &distOut,
                                                                 &fxMode, &fxCutoff, &fxReso, &fxVowel,
                                                                 &rvbMode, &rvbSize, &rvbDark })
            addAndMakeVisible (c);
        for (auto* k : { &distMix, &distBits, &distDown, &distOut, &fxCutoff, &fxReso, &fxVowel, &rvbSize, &rvbDark })
            k->labelSize = 11.0f;
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (colours::chassis);
        const float y = (float) dist.getCentreY() + 10.0f;
        drawCopperPipe (g, (float) dist.getRight(), (float) filter.getX(), y);
        drawCopperPipe (g, (float) filter.getRight(), (float) reverb.getX(), y);
        drawModuleFrame (g, dist.toFloat(), "Krangdistortion");
        drawModuleFrame (g, filter.toFloat(), juce::String::fromUTF8 ("Ångfilter"));
        drawModuleFrame (g, reverb.toFloat(), "Katedralreverb");

        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("ÅNGLEDNINGEN"), getLocalBounds().removeFromBottom (14).reduced (14, 0),
                    juce::Justification::centredRight);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 8).withTrimmedBottom (8);
        const int pipe = 34;
        const int total = r.getWidth() - 2 * pipe;
        dist   = r.removeFromLeft (total * 5 / 12);   r.removeFromLeft (pipe);
        filter = r.removeFromLeft (total * 4 / 12);   r.removeFromLeft (pipe);
        reverb = r;

        auto place = [] (juce::Rectangle<int> frame, juce::ComboBox& mode, std::initializer_list<juce::Component*> knobs)
        {
            auto in = frame.withTrimmedTop (28).reduced (8, 4);
            auto modeRow = in.removeFromLeft (110);
            mode.setBounds (modeRow.withSizeKeepingCentre (104, 24).withY (in.getY() + 24));
            layoutRow (in, knobs, 80);
        };
        place (dist,   distMode, { &distMix, &distBits, &distDown, &distOut });
        place (filter, fxMode,   { &fxCutoff, &fxReso, &fxVowel });
        place (reverb, rvbMode,  { &rvbSize, &rvbDark });
    }

    Bindings bindings()
    {
        return { { { &distMix.slider, "distMix" }, { &distBits.slider, "distBits" }, { &distDown.slider, "distDown" },
                   { &distOut.slider, "distOut" }, { &fxCutoff.slider, "fxCutoff" }, { &fxReso.slider, "fxReso" },
                   { &fxVowel.slider, "fxVowel" }, { &rvbSize.slider, "rvbSize" }, { &rvbDark.slider, "rvbDark" } },
                 { { &distMode, "distMode" }, { &fxMode, "fxMode" }, { &rvbMode, "rvbMode" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    juce::ComboBox distMode, fxMode, rvbMode;
    KrangKnob distMix { "Mix" }, distBits { "Bitar" }, distDown { "Nedsampl." }, distOut { "Ut" },
              fxCutoff { "Frekvens" }, fxReso { "Resonans" }, fxVowel { "Vokal" },
              rvbSize { "Storlek" }, rvbDark { u8 ("Mörker") };
    juce::Rectangle<int> dist, filter, reverb;
};

} // namespace krang::ui
