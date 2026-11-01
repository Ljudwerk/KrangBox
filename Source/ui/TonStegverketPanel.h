// Tonstegverkets panel: melodier i 16 steg per sida (fyra sidor upp till 64).
// Överst en liten pianorulle över två oktaver: klicka för att sätta tonen, klicka på en tänd ton för att
// släcka den, dra för att rita en slinga, mushjulet flyttar en halvton, Shift-klick ger accent.
// Under: längd (klick ökar, Shift-klick minskar), glid och accent per steg.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Panels.h"
#include "../dsp/TonStegverket.h"

namespace krang::ui
{

class TonGrid : public juce::Component
{
public:
    TonGrid (TonPattern& p, std::atomic<int>& playing) : pattern (p), playingStep (playing) {}

    void setPage (int newPage) { page = newPage; repaint(); }
    void setPlayablePages (int n) { if (n != playablePages) { playablePages = n; repaint(); } }
    int getPage() const { return page; }
    void setRoot (int note) { if (note != root) { root = note; repaint(); } }
    void setScale (TonScale s) { if (s != scale) { scale = s; repaint(); } }
    void setTranspose (int t) { if (t != transpose) { transpose = t; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const auto L = layout();
        const int playing = playingStep.load();

        // Radnamn.
        g.setFont (panelFont (10.0f));
        g.setColour (colours::brass);
        g.drawText ("TON", L.labels.withHeight (L.roll.getHeight()), Justification::centredRight);
        g.drawText (u8 ("LÄNGD"), L.labels.withY (L.lenRow.getY()).withHeight (L.lenRow.getHeight()), Justification::centredRight);
        g.drawText ("GLID", L.labels.withY (L.slideRow.getY()).withHeight (L.slideRow.getHeight()), Justification::centredRight);
        g.drawText ("ACCENT", L.labels.withY (L.accentRow.getY()).withHeight (L.accentRow.getHeight()), Justification::centredRight);

        // Pianorullen: mörk botten, skalans toner något ljusare, grundtonerna med mässingslinje.
        g.setColour (colours::bakelite);
        g.fillRoundedRectangle (L.roll, 4.0f);
        for (int semis = -12; semis <= 12; ++semis)
        {
            const auto row = rowRect (L, semis);
            if (inScale (scale, semis))
            {
                g.setColour (colours::chassisHigh.withAlpha (0.75f));
                g.fillRect (row.reduced (0, 0.5f));
            }
            if (semis % 12 == 0)
            {
                g.setColour (colours::brassDark.withAlpha (0.9f));
                g.drawHorizontalLine ((int) row.getCentreY(), L.roll.getX() + 2.0f, L.roll.getRight() - 2.0f);
            }
        }
        // Taktslag: var fjärde kolumn.
        for (int c = 4; c < 16; c += 4)
        {
            g.setColour (colours::brass.withAlpha (0.35f));
            const float x = L.roll.getX() + c * L.cellW;
            g.drawVerticalLine ((int) x, L.roll.getY() + 2.0f, L.accentRow.getBottom());
        }
        // Grundtonen och oktaverna ovanför och under, till vänster i rullen.
        g.setFont (panelFont (9.0f, false));
        g.setColour (colours::engraving.withAlpha (0.55f));
        for (int semis : { -12, 0, 12 })
            g.drawText (MidiMessage::getMidiNoteName (root + transpose + semis, true, true, 4),
                        rowRect (L, semis).withWidth (28.0f).translated (3.0f, 0).expanded (0, 4.0f), Justification::centredLeft);

        // Stegen.
        Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (L.roll.toNearestInt().getUnion (L.accentRow.toNearestInt()));
        for (int col = 0; col < 16; ++col)
        {
            const int step = page * 16 + col;
            const auto s = pattern.get (step);
            const float x = L.roll.getX() + col * L.cellW;

            if (step == playing)
            {
                g.setColour (colours::vfdText.withAlpha (0.12f));
                g.fillRect (Rectangle<float> (x, L.roll.getY(), L.cellW, L.accentRow.getBottom() - L.roll.getY()));
            }

            if (s.gate > 0)
            {
                const auto row = rowRect (L, s.semis);
                const float w = jmax (6.0f, L.cellW * (float) tonLengths[(size_t) s.length] - 3.0f);
                auto bar = Rectangle<float> (x + 2.0f, row.getY() - 1.0f, w, row.getHeight() + 2.0f);
                const auto c = s.gate == 2 ? colours::tubeOrange.brighter (0.35f) : colours::tubeOrange.withAlpha (0.85f);
                g.setColour (c.withAlpha (0.25f));
                g.fillRoundedRectangle (bar.expanded (2.0f), 3.0f);
                g.setColour (c);
                g.fillRoundedRectangle (bar, 2.0f);
                if (s.gate == 2)
                {
                    g.setColour (colours::brassLight);
                    g.drawRoundedRectangle (bar.expanded (1.0f), 2.5f, 1.2f);
                }

                // Glid: en kurva från tonens slut till nästa ton.
                const auto next = pattern.get ((step + 1) % TonPattern::maxSteps);
                if (s.slide && next.gate > 0)
                {
                    const auto nr = rowRect (L, next.semis);
                    Path p;
                    p.startNewSubPath (bar.getRight(), bar.getCentreY());
                    p.cubicTo (x + L.cellW, bar.getCentreY(), x + L.cellW, nr.getCentreY(), x + L.cellW + 4.0f, nr.getCentreY());
                    g.setColour (colours::vfdText.withAlpha (0.85f));
                    g.strokePath (p, PathStrokeType (1.6f));
                }

                // Tonnamnet över stapeln.
                g.setColour (colours::engraving.withAlpha (0.9f));
                g.setFont (panelFont (9.0f));
                const auto label = MidiMessage::getMidiNoteName (root + transpose + s.semis, true, true, 4);
                const float ly = s.semis >= 10 ? bar.getBottom() + 1.0f : bar.getY() - 12.0f;
                g.drawText (label, Rectangle<float> (x, ly, L.cellW, 11.0f), Justification::centred);
            }

            // Längd, glid och accent.
            auto lenCell = Rectangle<float> (x, L.lenRow.getY(), L.cellW, L.lenRow.getHeight()).reduced (3.0f, 2.0f);
            g.setColour (colours::bakelite);
            g.fillRoundedRectangle (lenCell, 3.0f);
            g.setColour (s.gate > 0 ? colours::vfdText : colours::vfdText.withAlpha (0.3f));
            g.setFont (panelFont (11.0f, false));
            g.drawText (String::fromUTF8 (tonLengthName (s.length)), lenCell, Justification::centred);

            drawLamp (g, Rectangle<float> (x, L.slideRow.getY(), L.cellW, L.slideRow.getHeight()), s.slide, colours::vfdText);
            drawLamp (g, Rectangle<float> (x, L.accentRow.getY(), L.cellW, L.accentRow.getHeight()), s.gate == 2, colours::tubeOrange);
        }

        // Sidan ligger utanför slingan: matta lampor och en förklaring.
        if (page >= playablePages)
        {
            auto r = getLocalBounds().toFloat().withTrimmedLeft (labelW);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (r);
            g.setColour (colours::engraving.withAlpha (0.85f));
            g.setFont (panelFont (12.0f));
            g.drawText (juce::String::fromUTF8 ("SIDAN SPELAS INTE – ÖKA STEG TILL ") + juce::String ((page + 1) * 16) + juce::String::fromUTF8 (" FÖR ATT HÖRA DEN"),
                        r, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto L = layout();
        const int col = columnAt (L, e.position.x);
        if (col < 0) return;
        const int step = page * 16 + col;
        auto s = pattern.get (step);
        lastCol = col;
        dragMode = Drag::None;

        if (L.roll.contains (e.position))
        {
            const int semis = semisAt (L, e.position.y);
            if (e.mods.isShiftDown())                 { s.gate = s.gate == 2 ? 1 : 2; s.semis = s.gate > 0 ? s.semis : semis; }
            else if (s.gate > 0 && s.semis == semis)  { s.gate = 0; dragMode = Drag::Erase; }
            else                                      { s.gate = s.gate == 0 ? 1 : s.gate; s.semis = semis; dragMode = Drag::Draw; }
        }
        else if (L.lenRow.contains (e.position))
            s.length = (s.length + (e.mods.isShiftDown() || e.mods.isPopupMenu() ? 7 : 1)) % 8;
        else if (L.slideRow.contains (e.position))
            s.slide = ! s.slide;
        else if (L.accentRow.contains (e.position))
            s.gate = s.gate == 2 ? 1 : 2;
        else return;

        pattern.set (step, s);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragMode == Drag::None) return;
        const auto L = layout();
        const int col = columnAt (L, e.position.x);
        if (col < 0) return;
        const int step = page * 16 + col;
        auto s = pattern.get (step);
        if (dragMode == Drag::Erase)
        {
            if (col == lastCol) return;
            s.gate = 0;
        }
        else
        {
            const int semis = semisAt (L, juce::jlimit (L.roll.getY(), L.roll.getBottom() - 1.0f, e.position.y));
            if (s.gate > 0 && s.semis == semis) return;
            s.gate = s.gate == 0 ? 1 : s.gate;
            s.semis = semis;
        }
        lastCol = col;
        pattern.set (step, s);
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const auto L = layout();
        const int col = columnAt (L, e.position.x);
        if (col < 0 || std::abs (w.deltaY) < 0.01f) return;
        const int step = page * 16 + col;
        auto s = pattern.get (step);
        const int dir = (w.deltaY > 0) != w.isReversed ? 1 : -1;
        if (L.lenRow.contains (e.position)) s.length = juce::jlimit (0, 7, s.length + dir);
        else if (s.gate > 0)                s.semis = juce::jlimit (-12, 12, s.semis + dir);
        else return;
        pattern.set (step, s);
        repaint();
    }

private:
    enum class Drag { None, Draw, Erase };
    struct Layout { juce::Rectangle<float> labels, roll, lenRow, slideRow, accentRow; float cellW, rowH; };

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    Layout layout() const
    {
        auto r = getLocalBounds().toFloat();
        Layout L;
        L.labels = r.removeFromLeft (labelW).withTrimmedRight (8.0f);
        L.accentRow = r.removeFromBottom (20.0f);
        L.slideRow = r.removeFromBottom (20.0f);
        L.lenRow = r.removeFromBottom (24.0f);
        r.removeFromBottom (4.0f);
        L.roll = r;
        L.labels = L.labels.withY (r.getY());
        L.cellW = r.getWidth() / 16.0f;
        L.rowH = (r.getHeight() - 8.0f) / 25.0f;
        return L;
    }

    juce::Rectangle<float> rowRect (const Layout& L, int semis) const
    {
        return { L.roll.getX(), L.roll.getY() + 4.0f + (float) (12 - semis) * L.rowH, L.roll.getWidth(), L.rowH };
    }

    int semisAt (const Layout& L, float y) const
    {
        return juce::jlimit (-12, 12, 12 - (int) std::floor ((y - L.roll.getY() - 4.0f) / L.rowH));
    }

    static int columnAt (const Layout& L, float x)
    {
        if (x < L.roll.getX() || x >= L.roll.getRight()) return -1;
        return juce::jlimit (0, 15, (int) ((x - L.roll.getX()) / L.cellW));
    }

    static void drawLamp (juce::Graphics& g, juce::Rectangle<float> cell, bool on, juce::Colour c)
    {
        const float d = juce::jmin (cell.getHeight() - 6.0f, 12.0f);
        auto bulb = cell.withSizeKeepingCentre (d, d);
        g.setColour (colours::bakelite);
        g.fillEllipse (bulb.expanded (1.5f));
        g.setColour (on ? c : c.withAlpha (0.12f));
        g.fillEllipse (bulb);
        if (on)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (bulb.expanded (3.0f));
        }
    }

    static constexpr float labelW = 64.0f;
    TonPattern& pattern;
    std::atomic<int>& playingStep;
    int page = 0, playablePages = 4, root = 36, transpose = 0, lastCol = -1;
    TonScale scale = TonScale::Moll;
    Drag dragMode = Drag::None;
};

class TonStegverketPanel : public juce::Component, private juce::Timer
{
public:
    TonStegverketPanel (TonPattern& p, std::atomic<int>& playing, std::atomic<int>& transposed)
        : pattern (p), playingStep (playing), transposeShown (transposed), grid (p, playing)
    {
        play.setClickingTogglesState (true);
        play.setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
        play.setColour (juce::TextButton::buttonOnColourId, colours::tubeOrange.darker (0.3f));
        play.setColour (juce::TextButton::textColourOffId, colours::engraving);
        play.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        play.onStateChange = [this] { play.setButtonText (play.getToggleState() ? "STOPP" : "SPELA"); };

        for (auto* s : { &tempo, &swing, &root })
        {
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
            s->setColour (juce::Slider::trackColourId, colours::brass);
            s->setColour (juce::Slider::thumbColourId, colours::brassLight);
            s->setColour (juce::Slider::backgroundColourId, colours::bakelite);
            s->setColour (juce::Slider::textBoxTextColourId, colours::engraving);
            s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        }
        root.onValueChange = [this] { grid.setRoot ((int) root.getValue()); };
        scale.onChange = [this] { grid.setScale ((TonScale) juce::jmax (0, scale.getSelectedItemIndex())); };
        length.onChange = [this] { updatePages(); };

        for (int i = 0; i < 4; ++i)
        {
            auto& b = pages[(size_t) i];
            b.setButtonText (juce::String (i + 1));
            b.setRadioGroupId (4712);
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b.setColour (juce::TextButton::buttonOnColourId, colours::brass);
            b.setColour (juce::TextButton::textColourOffId, colours::engraving);
            b.setColour (juce::TextButton::textColourOnId, colours::bakelite);
            b.onClick = [this, i] { grid.setPage (i); };
            addAndMakeVisible (b);
        }
        pages[0].setToggleState (true, juce::dontSendNotification);

        for (auto* b : { &randomise, &clear, &standard })
        {
            b->setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b->setColour (juce::TextButton::textColourOffId, colours::engraving);
        }
        randomise.setTooltip (u8 ("Doktor Krang komponerar: en ny slinga i vald skala"));
        randomise.onClick = [this]
        {
            pattern.randomise ((TonScale) juce::jmax (0, scale.getSelectedItemIndex()), 16 * juce::jmax (1, length.getSelectedItemIndex() + 1), random);
            grid.repaint();
        };
        clear.onClick    = [this] { pattern.clear(); grid.repaint(); };
        standard.onClick = [this] { pattern.loadDefault(); grid.repaint(); };
        keys.setTooltip (u8 ("Transponera: en tangent flyttar hela slingan (grundtonen = ingen flytt). Spela med: tangenterna spelar ovanpå."));

        for (auto* c : std::initializer_list<juce::Component*> { &play, &tempo, &swing, &length, &rate, &root, &keys, &scale,
                                                                  &randomise, &clear, &standard, &grid })
            addAndMakeVisible (c);

        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });

        // Hur långt tangenterna har flyttat slingan.
        const int t = transposeShown.load();
        g.setFont (panelFont (11.0f));
        g.setColour (t != 0 ? colours::vfdText : colours::engraving.withAlpha (0.4f));
        g.drawText (t == 0 ? u8 ("OTRANSPONERAD") : "TRANSPONERAD " + juce::String (t > 0 ? "+" : "") + juce::String (t),
                    transposeLabel, juce::Justification::centredLeft);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        drawModuleFrame (g, gridFrame.toFloat(), u8 ("Stegverket · toner"));
        drawModuleFrame (g, sideFrame.toFloat(), "Slingan");

        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        for (auto& [label, comp] : { std::pair<juce::String, juce::Component*> { "TEMPO", &tempo }, { "GUNG %", &swing },
                                     { "STEG", &length }, { "SIDA", &pages[0] }, { "TAKT", &rate },
                                     { "GRUNDTON", &root }, { "TANGENTER", &keys }, { "SKALA", &scale } })
            g.drawText (label, comp->getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);

        g.setColour (colours::engraving.withAlpha (0.55f));
        g.drawText (u8 ("Klick: sätt ton / släck   Dra: rita   Hjul: halvton   Shift: accent   "
                        "I en DAW följer Stegverket DAW:ens transport när SPELA är på"),
                    hintArea, juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        auto r = getLocalBounds().reduced (12, 8);
        sideFrame = r.removeFromRight (200);
        r.removeFromRight (12);
        gridFrame = r;

        auto in = gridFrame.withTrimmedTop (28).reduced (12, 6);
        hintArea = in.removeFromBottom (16);
        auto top = in.removeFromTop (40).withTrimmedTop (14).withTrimmedBottom (2);
        play.setBounds (top.removeFromLeft (72));
        top.removeFromLeft (12);
        tempo.setBounds (top.removeFromLeft (150));
        top.removeFromLeft (12);
        swing.setBounds (top.removeFromLeft (130));
        top.removeFromLeft (12);
        length.setBounds (top.removeFromLeft (62));
        top.removeFromLeft (12);
        for (auto& b : pages) { b.setBounds (top.removeFromLeft (26)); top.removeFromLeft (3); }
        top.removeFromLeft (10);
        rate.setBounds (top.removeFromLeft (100));
        top.removeFromLeft (12);
        transposeLabel = top;
        in.removeFromTop (8);
        grid.setBounds (in);

        auto side = sideFrame.withTrimmedTop (30).reduced (12, 4);
        auto row = [&side] (int h) { side.removeFromTop (14); return side.removeFromTop (h); };
        root.setBounds (row (24));
        side.removeFromTop (4);
        keys.setBounds (row (24));
        side.removeFromTop (4);
        scale.setBounds (row (24));
        side.removeFromTop (12);
        randomise.setBounds (side.removeFromTop (26));
        side.removeFromTop (6);
        auto buttons = side.removeFromTop (26);
        clear.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 3));
        buttons.removeFromLeft (6);
        standard.setBounds (buttons);
    }

    // Öppnar en sida (0–3), som ett klick på sidknappen.
    void showPage (int i) { updatePages(); pages[(size_t) juce::jlimit (0, 3, i)].setToggleState (true, juce::sendNotificationSync); grid.setPage (juce::jlimit (0, 3, i)); }

    juce::TextButton play { "SPELA" };
    juce::Slider tempo, swing, root;
    juce::ComboBox length, rate, keys, scale;

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    // Alla fyra sidor går att öppna och redigera; sidor bortom STEG spelas inte och visas matta,
    // så att man kan förbereda en längre slinga innan man ökar STEG.
    void updatePages()
    {
        const int numPages = juce::jmax (1, length.getSelectedItemIndex() + 1);
        if (numPages == shownPages) return;
        shownPages = numPages;
        for (int i = 0; i < 4; ++i)
        {
            auto& b = pages[(size_t) i];
            const bool inLoop = i < numPages;
            b.setColour (juce::TextButton::textColourOffId, inLoop ? colours::engraving : colours::engraving.withAlpha (0.35f));
            b.setColour (juce::TextButton::buttonColourId, inLoop ? colours::chassisHigh : colours::bakelite);
            b.setTooltip (inLoop ? juce::String() : juce::String::fromUTF8 ("Sidan ligger utanför slingan – öka STEG för att höra den"));
        }
        grid.setPlayablePages (numPages);
    }

    void timerCallback() override
    {
        if (! isShowing()) return;
        updatePages();

        // Lampan följer uppspelningen; mönstret kan ändras av ett preset eller av DAW:en.
        juce::uint64 h = 1469598103934665603ull;
        for (int i = 0; i < TonPattern::maxSteps; ++i)
        {
            const auto s = pattern.get (i);
            h = (h ^ (juce::uint64) (s.gate + 4 * (s.semis + 12) + 128 * s.length + 1024 * (s.slide ? 1 : 0))) * 1099511628211ull;
        }
        const int step = playingStep.load(), t = transposeShown.load();
        if (step != lastStep || h != lastHash)
        {
            lastStep = step;
            lastHash = h;
            grid.repaint();
        }
        if (t != lastTranspose)
        {
            lastTranspose = t;
            grid.setTranspose (t);
            repaint (transposeLabel);
        }
    }

    TonPattern& pattern;
    std::atomic<int>& playingStep;
    std::atomic<int>& transposeShown;
    TonGrid grid;
    std::array<juce::TextButton, 4> pages;
    juce::TextButton randomise { "SLUMPA" }, clear { "RENSA" }, standard { "STANDARD" };
    juce::Rectangle<int> gridFrame, sideFrame, hintArea, transposeLabel;
    int lastStep = -2, shownPages = -1, lastTranspose = 0;
    juce::uint64 lastHash = 0;
    juce::Random random;
    textures::BackgroundCache background;
};

} // namespace krang::ui
