// Modulationsraden i gränssnittet: två LFO:er, två modulationsenvelopes och matrisen med sex kopplingar.
// Panelen ligger på samma plats som kammarmodulen och visas med knappen MODULATION i keyboardraden.
// Lamporna följer källorna i realtid, och varje koppling har en tunn visare som visar hur mycket den
// skjuter på sitt mål just nu.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Panels.h"
#include "../Parameters.h"

namespace krang::ui
{

// En liten bärnstenslampa som lyser med en modulationskälla (0..1).
class ModLamp : public juce::Component
{
public:
    void setLevel (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (std::abs (v - level) < 0.01f) return;
        level = v;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        const float d = jmin (r.getWidth(), r.getHeight());
        auto bulb = r.withSizeKeepingCentre (d, d).reduced (3.0f);
        drawBrassBezel (g, bulb.expanded (2.0f), bulb.getWidth() * 0.5f + 2.0f, 2.5f);
        const auto off = Colour (0xff2e1a06), on = Colour (0xffffb02e);
        g.setColour (off.interpolatedWith (on, level));
        g.fillEllipse (bulb);
        g.setColour (on.withAlpha (0.3f * level));
        g.fillEllipse (bulb.expanded (3.0f * level));
        g.setColour (Colours::white.withAlpha (0.22f));
        g.fillEllipse (bulb.reduced (bulb.getWidth() * 0.32f).translated (-bulb.getWidth() * 0.12f, -bulb.getHeight() * 0.12f));
    }

private:
    float level = 0.0f;
};

// Mängdreglaget i matrisen: noll i mitten, kopparfyllning åt plus eller minus,
// och en tunn grön visare för vad kopplingen gör just nu.
class AmountBar : public juce::Slider
{
public:
    AmountBar()
    {
        setSliderStyle (juce::Slider::LinearHorizontal);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setPopupDisplayEnabled (true, true, nullptr);
        setDoubleClickReturnValue (true, 0.0);
        setSliderSnapsToMousePosition (false); // dra relativt – greppet hoppar inte dit man klickar
        setMouseDragSensitivity (220);
    }

    void setLive (float v)
    {
        v = juce::jlimit (-1.0f, 1.0f, v);
        if (std::abs (v - live) < 0.005f) return;
        live = v;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat().reduced (8.0f, 0.0f);
        // Värdet i procent till höger.
        const auto textArea = r.removeFromRight (44.0f);
        r.removeFromRight (6.0f);
        {
            const int pct = roundToInt (getValue() * 100.0);
            g.setColour (pct == 0 ? colours::engraving.withAlpha (0.45f) : colours::engraving);
            g.setFont (panelFont (12.0f, false));
            g.drawText ((pct > 0 ? "+" : "") + String (pct) + " %", textArea, Justification::centredRight);
        }
        auto slot = r.withSizeKeepingCentre (r.getWidth(), 8.0f);
        g.setColour (colours::bakelite);
        g.fillRoundedRectangle (slot, 4.0f);
        g.setColour (colours::brassDark);
        g.drawRoundedRectangle (slot, 4.0f, 1.0f);

        const float cx = slot.getCentreX();
        const float amount = (float) getValue();
        const float x = cx + amount * slot.getWidth() * 0.5f;
        g.setColour (colours::tubeOrange.withAlpha (0.85f));
        g.fillRoundedRectangle (Rectangle<float>::leftTopRightBottom (jmin (cx, x), slot.getY() + 1.5f, jmax (cx, x), slot.getBottom() - 1.5f), 2.5f);

        // Nollstrecket.
        g.setColour (colours::engraving.withAlpha (0.7f));
        g.drawVerticalLine ((int) cx, r.getY() + 3.0f, r.getBottom() - 3.0f);

        // Vad kopplingen gör nu.
        if (std::abs (live) > 0.002f)
        {
            const float lx = cx + live * slot.getWidth() * 0.5f;
            g.setColour (Colour (0xff7dff9a).withAlpha (0.9f));
            g.fillRect (Rectangle<float> (lx - 1.0f, slot.getY() - 4.0f, 2.0f, slot.getHeight() + 8.0f));
        }

        // Greppet.
        auto cap = Rectangle<float> (12.0f, jmin (22.0f, r.getHeight() - 2.0f)).withCentre ({ x, r.getCentreY() });
        ColourGradient grad (colours::chassisHigh.brighter (0.2f), 0, cap.getY(), colours::bakelite, 0, cap.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (cap, 2.5f);
        g.setColour (colours::brass);
        g.drawRoundedRectangle (cap, 2.5f, 1.0f);
        g.setColour (colours::engraving);
        g.drawVerticalLine ((int) cap.getCentreX(), cap.getY() + 3.0f, cap.getBottom() - 3.0f);
    }

private:
    float live = 0.0f;
};

class ModulationPanel : public juce::Component
{
public:
    ModulationPanel()
    {
        for (auto& l : lfos)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &l.shape, &l.sync, &l.retrig, &l.rate, &l.lamp })
                addAndMakeVisible (c);
            l.rate.labelSize = 11.0f;
            l.shape.setTooltip ("Formen: sinus, triangel, fyrkant, " + u8 ("sågtand") + ", slump (nytt värde varje varv) eller glid (slump som glider)");
            l.sync.setTooltip (u8 ("Fri går på hastighetsratten; annars följer LFO:n takten i DAW:en eller i Stegverkets tempo"));
            l.retrig.setTooltip (u8 ("Vid ton: LFO:n börjar om från början på varje ny ton"));
            // Synkad LFO: hastighetsratten gör inget, så den blir matt.
            l.sync.onChange = [&l] { l.rate.setAlpha (l.sync.getSelectedItemIndex() > 0 ? 0.4f : 1.0f); };
        }
        for (auto& e : envs)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &e.a, &e.d, &e.s, &e.r, &e.lamp })
                addAndMakeVisible (c);
        }
        for (auto& row : rows)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &row.src, &row.dst, &row.amount })
                addAndMakeVisible (c);
            row.src.setTooltip (u8 ("Källan: vad som ska röra målet"));
            row.dst.setTooltip (u8 ("Målet. Klang är det som gör just den här kammaren ljusare eller skarpare."));
            row.amount.setTooltip (u8 ("Hur mycket, och åt vilket håll. Dubbelklicka för noll."));
        }
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);

        g.setFont (panelFont (10.0f));
        g.setColour (colours::brass);
        for (auto& l : lfos)
        {
            g.drawText ("FORM", l.shape.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
            g.drawText ("TAKT", l.sync.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
            g.drawText ("OMSTART", l.retrig.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
        }

        // Matrisens rubriker, radnummer och pilar.
        g.drawText (u8 ("KÄLLA"), headSrc, juce::Justification::centredLeft);
        g.drawText (u8 ("MÅL"), headDst, juce::Justification::centredLeft);
        g.drawText (u8 ("MÄNGD"), headAmt, juce::Justification::centred);
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto& row = rows[i];
            const auto src = row.src.getBounds();
            g.setColour (colours::engraving);
            g.setFont (panelFont (13.0f));
            g.drawText (juce::String ((int) i + 1), src.withX (src.getX() - 26).withWidth (20), juce::Justification::centredRight);
            const float y = (float) src.getCentreY();
            const float x0 = (float) src.getRight() + 5.0f, x1 = (float) row.dst.getX() - 5.0f;
            g.setColour (colours::brass);
            g.drawArrow ({ x0, y, x1, y }, 1.6f, 7.0f, 7.0f);
        }
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto lfoCol = r.removeFromLeft (330); r.removeFromLeft (12);
        auto envCol = r.removeFromLeft (250); r.removeFromLeft (12);
        auto matrix = r;
        const int half = (lfoCol.getHeight() - 10) / 2;

        for (int i = 0; i < 2; ++i)
        {
            auto f = i == 0 ? lfoCol.removeFromTop (half) : lfoCol.withTrimmedTop (10);
            frames.push_back ({ f, "LFO " + juce::String (i + 1) });
            auto in = f.withTrimmedTop (30).reduced (12, 6);
            auto& l = lfos[(size_t) i];
            l.lamp.setBounds (in.removeFromRight (30).withSizeKeepingCentre (26, 26));
            in.removeFromRight (4);
            l.rate.setBounds (in.removeFromRight (86));
            in.removeFromRight (8);
            auto combos = in.withTrimmedTop (12);
            l.shape.setBounds (combos.removeFromTop (24));
            combos.removeFromTop (16);
            auto bottom = combos.removeFromTop (24);
            l.sync.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 - 4));
            bottom.removeFromLeft (8);
            l.retrig.setBounds (bottom);
        }

        for (int i = 0; i < 2; ++i)
        {
            auto f = i == 0 ? envCol.removeFromTop (half) : envCol.withTrimmedTop (10);
            frames.push_back ({ f, "Envelope " + juce::String (i + 1) });
            auto in = f.withTrimmedTop (28).reduced (10, 4);
            auto& e = envs[(size_t) i];
            e.lamp.setBounds (in.removeFromRight (30).withSizeKeepingCentre (26, 26));
            layoutRow (in, { &e.a, &e.d, &e.s, &e.r }, 44);
        }

        frames.push_back ({ matrix, "Matrisen" });
        auto in = matrix.withTrimmedTop (30).reduced (14, 4);
        auto head = in.removeFromTop (14);
        head.removeFromLeft (28);
        const int comboW = 140, gap = 30;
        headSrc = head.removeFromLeft (comboW);
        head.removeFromLeft (gap);
        headDst = head.removeFromLeft (comboW);
        head.removeFromLeft (12);
        headAmt = head;
        const int rowH = in.getHeight() / (int) rows.size();
        for (auto& row : rows)
        {
            auto line = in.removeFromTop (rowH).withSizeKeepingCentre (in.getWidth(), 26);
            line.removeFromLeft (28);
            row.src.setBounds (line.removeFromLeft (comboW));
            line.removeFromLeft (gap);
            row.dst.setBounds (line.removeFromLeft (comboW));
            line.removeFromLeft (12);
            row.amount.setBounds (line);
        }
    }

    Bindings bindings()
    {
        namespace id = krang::params::id;
        Bindings b;
        for (int i = 0; i < 2; ++i)
        {
            auto& l = lfos[(size_t) i];
            b.sliders.push_back ({ &l.rate.slider, id::lfo (i + 1, "Rate") });
            b.combos.push_back ({ &l.shape, id::lfo (i + 1, "Shape") });
            b.combos.push_back ({ &l.sync, id::lfo (i + 1, "Sync") });
            b.combos.push_back ({ &l.retrig, id::lfo (i + 1, "Retrig") });
            auto& e = envs[(size_t) i];
            b.sliders.push_back ({ &e.a.slider, id::menv (i + 1, "A") });
            b.sliders.push_back ({ &e.d.slider, id::menv (i + 1, "D") });
            b.sliders.push_back ({ &e.s.slider, id::menv (i + 1, "S") });
            b.sliders.push_back ({ &e.r.slider, id::menv (i + 1, "R") });
        }
        for (int i = 0; i < (int) rows.size(); ++i)
        {
            auto& row = rows[(size_t) i];
            b.combos.push_back ({ &row.src, id::slot (i + 1, "Src") });
            b.combos.push_back ({ &row.dst, id::slot (i + 1, "Dst") });
            b.sliders.push_back ({ &row.amount, id::slot (i + 1, "Amt") });
        }
        return b;
    }

    // Källornas värden från processorn: LFO 1–2 (−1..1), env 1–2, modhjul, aftertouch, anslag (0..1).
    void setSourceValues (const std::array<float, 7>& v)
    {
        lfos[0].lamp.setLevel (0.5f + 0.5f * v[0]);
        lfos[1].lamp.setLevel (0.5f + 0.5f * v[1]);
        envs[0].lamp.setLevel (v[2]);
        envs[1].lamp.setLevel (v[3]);
        for (auto& row : rows)
        {
            const int src = row.src.getSelectedItemIndex();
            const bool connected = src > 0 && row.dst.getSelectedItemIndex() > 0;
            row.amount.setLive (connected ? v[(size_t) juce::jlimit (0, 6, src - 1)] * (float) row.amount.getValue() : 0.0f);
        }
    }

    // Antal kopplingar som gör något (visas på knappen MODULATION).
    int activeCount() const
    {
        int n = 0;
        for (auto& row : rows)
            if (row.src.getSelectedItemIndex() > 0 && row.dst.getSelectedItemIndex() > 0 && std::abs (row.amount.getValue()) > 0.0005)
                ++n;
        return n;
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    struct Lfo
    {
        juce::ComboBox shape, sync, retrig;
        KrangKnob rate { "Hastighet" };
        ModLamp lamp;
    };
    struct Env
    {
        KrangFader a { "A" }, d { "D" }, s { "S" }, r { "R" };
        ModLamp lamp;
    };
    struct Row
    {
        juce::ComboBox src, dst;
        AmountBar amount;
    };

    std::array<Lfo, 2> lfos;
    std::array<Env, 2> envs;
    std::array<Row, 6> rows;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> headSrc, headDst, headAmt;
    textures::BackgroundCache background;
};

} // namespace krang::ui
