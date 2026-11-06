// Stegverkets panel: lamprutnät 8 spår × 16 steg (fyra sidor upp till 64), transport och inställningar.
// Läget SLAG: klick växlar av -> slag -> accent -> av, dra för att måla. CHANS och RATCH: klick bläddrar
// (Shift bakåt), dra för att måla. TON och KLANG: dra uppåt/nedåt eller använd mushjulet för parameterlås.
// Dubbelklick tar bort. Klick på ett spårnamn byter spårets ljud.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "KrangStyle.h"
#include "../dsp/Stegverket.h"
#include <functional>

namespace krang::ui
{

// Vad ett klick i rutnätet ändrar.
enum class StepEdit { Slag = 0, Chans, Ratchet, Ton, Klang };

class StepGrid : public juce::Component
{
public:
    StepGrid (StepPattern& p, std::atomic<int>& playing) : pattern (p), playingStep (playing)
    {
        for (int l = 0; l < StepPattern::lanes; ++l)
            laneNames[(size_t) l] = juce::String::fromUTF8 (drumName (l));
    }

    void setPage (int newPage) { page = newPage; repaint(); }
    void setPlayablePages (int n) { if (n != playablePages) { playablePages = n; repaint(); } }
    int getPage() const { return page; }
    void setMode (StepEdit m) { mode = m; repaint(); }

    // Spårens namn (ljudet som spåret spelar); ett klick på namnet anropar onLaneClicked.
    void setLaneName (int lane, const juce::String& name)
    {
        if (laneNames[(size_t) lane] != name) { laneNames[(size_t) lane] = name; repaint(); }
    }
    std::function<void (int lane, juce::Rectangle<int> labelArea)> onLaneClicked;

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const auto area = getLocalBounds().toFloat();
        const float cellW = (area.getWidth() - labelW) / 16.0f;
        const float cellH = area.getHeight() / (float) StepPattern::lanes;
        const int playing = playingStep.load();

        // Fält för var fjärde steg (ett slag), så att man ser takten.
        for (int beat = 0; beat < 4; ++beat)
        {
            auto r = Rectangle<float> (labelW + beat * 4 * cellW, 0, 4 * cellW, area.getHeight());
            g.setColour ((beat % 2 == 0 ? colours::chassisHigh : colours::chassis).withAlpha (0.9f));
            g.fillRect (r.reduced (1.0f, 0));
        }

        for (int lane = 0; lane < StepPattern::lanes; ++lane)
        {
            const float y = lane * cellH;
            const auto label = Rectangle<float> (0, y, labelW - 8.0f, cellH);
            g.setColour (colours::engraving.withAlpha (0.85f));
            g.setFont (panelFont (11.0f));
            g.drawText (VfdDisplay::upper (laneNames[(size_t) lane]), label.withTrimmedRight (12.0f), Justification::centredRight);
            // En liten pil: namnet går att klicka på för att byta ljud.
            Path arrow;
            const float ax = label.getRight() - 6.0f, ay = label.getCentreY();
            arrow.addTriangle (ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 2.5f);
            g.setColour (colours::brass.withAlpha (0.8f));
            g.fillPath (arrow);

            for (int col = 0; col < 16; ++col)
            {
                const int step = page * 16 + col;
                const auto st = pattern.getStep (lane, step);
                const int v = st.gate;
                auto cell = Rectangle<float> (labelW + col * cellW, y, cellW, cellH).reduced (3.0f, 3.0f);
                const bool isPlaying = step == playing;
                const bool dimmed = mode != StepEdit::Slag && v == 0;

                // Lampa: mörk bakelit när av, rörorange glöd när på, ljusare med mässingsring vid accent.
                g.setColour (colours::bakelite);
                g.fillRoundedRectangle (cell, 3.0f);
                if (v > 0)
                {
                    const float strength = mode == StepEdit::Slag ? drumChanceValues[(size_t) st.chance] : 0.45f;
                    const auto c = v == 2 ? colours::tubeOrange.brighter (0.35f) : colours::tubeOrange.withAlpha (0.8f);
                    g.setColour (c.withAlpha (0.25f * strength));
                    g.fillRoundedRectangle (cell.expanded (1.5f), 4.0f);
                    g.setColour (c.withMultipliedAlpha (0.35f + 0.65f * strength));
                    g.fillRoundedRectangle (cell.reduced (2.0f), 2.5f);
                    if (v == 2)
                    {
                        g.setColour (colours::brassLight);
                        g.drawRoundedRectangle (cell.reduced (0.5f), 3.0f, 1.4f);
                    }
                }

                if (v > 0 && mode == StepEdit::Slag)
                {
                    // Små märken: ratchet som streck överst, lås som en mässingsprick.
                    g.setColour (colours::bakelite.withAlpha (0.85f));
                    for (int r = 0; r < st.ratchet; ++r)
                        g.fillRect (Rectangle<float> (cell.getX() + 4.0f + r * 5.0f, cell.getY() + 3.0f, 3.0f, 3.0f));
                    if (st.tuneLocked || st.decay > 0)
                    {
                        g.setColour (colours::brassLight);
                        g.fillEllipse (Rectangle<float> (5.0f, 5.0f).withCentre ({ cell.getRight() - 5.0f, cell.getBottom() - 5.0f }));
                    }
                }
                else if (v > 0)
                {
                    // Värdet för det valda läget som text i lampan.
                    String text;
                    bool set = true;
                    switch (mode)
                    {
                        case StepEdit::Chans:   text = String (roundToInt (drumChanceValues[(size_t) st.chance] * 100.0f)) + "%"; set = st.chance != 0; break;
                        case StepEdit::Ratchet: text = String::fromUTF8 ("×") + String (st.ratchet + 1); set = st.ratchet != 0; break;
                        case StepEdit::Ton:     text = st.tuneLocked ? (st.tune > 0 ? "+" : "") + String (st.tune) : String::fromUTF8 ("·"); set = st.tuneLocked; break;
                        case StepEdit::Klang:   text = st.decay > 0 ? String::fromUTF8 ("×") + String (lockDecayValues[(size_t) st.decay], st.decay < 4 ? 2 : 1).trimCharactersAtEnd ("0").trimCharactersAtEnd (".")
                                                                    : String::fromUTF8 ("·");
                                                set = st.decay > 0; break;
                        case StepEdit::Slag: break;
                    }
                    g.setColour (set ? Colours::white : colours::engraving.withAlpha (0.6f));
                    g.setFont (panelFont (jmin (12.0f, cellW * 0.32f)));
                    g.drawText (text, cell, Justification::centred);
                }
                if (dimmed)
                {
                    g.setColour (Colours::black.withAlpha (0.25f));
                    g.fillRoundedRectangle (cell, 3.0f);
                }
                if (isPlaying)
                {
                    g.setColour (colours::vfdText.withAlpha (v > 0 ? 0.9f : 0.35f));
                    g.drawRoundedRectangle (cell.expanded (1.0f), 3.5f, 1.6f);
                }
            }
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
        if (e.position.x < labelW)
        {
            const float cellH = (float) getHeight() / (float) StepPattern::lanes;
            const int lane = juce::jlimit (0, StepPattern::lanes - 1, (int) (e.position.y / cellH));
            if (onLaneClicked) onLaneClicked (lane, juce::Rectangle<int> (0, (int) (lane * cellH), (int) labelW, (int) cellH));
            return;
        }
        const auto hit = cellAt (e.position);
        if (hit.lane < 0) return;
        auto st = pattern.getStep (hit.lane, hit.step);
        lastCell = hit;
        const bool back = e.mods.isShiftDown();

        switch (mode)
        {
            case StepEdit::Slag:
                paintValue = back ? 2 : (st.gate + 1) % 3;    // Shift-klick = accent direkt
                st.gate = paintValue;
                break;
            case StepEdit::Chans:
                if (st.gate == 0) return;
                paintValue = (st.chance + (back ? 3 : 1)) % 4;
                st.chance = paintValue;
                break;
            case StepEdit::Ratchet:
                if (st.gate == 0) return;
                paintValue = (st.ratchet + (back ? 3 : 1)) % 4;
                st.ratchet = paintValue;
                break;
            case StepEdit::Ton:
                if (st.gate == 0) return;
                if (! st.tuneLocked) { st.tuneLocked = true; st.tune = 0; }
                dragStartValue = st.tune;
                break;
            case StepEdit::Klang:
                if (st.gate == 0) return;
                if (st.decay == 0) st.decay = 4;
                dragStartValue = st.decay;
                break;
        }
        pattern.setStep (hit.lane, hit.step, st);
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (lastCell.lane < 0) return;
        if (mode == StepEdit::Ton || mode == StepEdit::Klang)
        {
            // Dra uppåt eller nedåt för att ändra låset.
            auto st = pattern.getStep (lastCell.lane, lastCell.step);
            if (st.gate == 0) return;
            const int delta = -e.getDistanceFromDragStartY() / (mode == StepEdit::Ton ? 6 : 12);
            if (mode == StepEdit::Ton) { st.tune = juce::jlimit (-12, 12, dragStartValue + delta); st.tuneLocked = true; }
            else                       st.decay = juce::jlimit (1, 7, dragStartValue + delta);
            pattern.setStep (lastCell.lane, lastCell.step, st);
            repaint();
            return;
        }
        const auto hit = cellAt (e.position);
        if (hit.lane < 0 || (hit.lane == lastCell.lane && hit.step == lastCell.step)) return;
        auto st = pattern.getStep (hit.lane, hit.step);
        lastCell = hit;
        if (mode == StepEdit::Slag) st.gate = paintValue;
        else if (st.gate == 0) return;
        else if (mode == StepEdit::Chans) st.chance = paintValue;
        else if (mode == StepEdit::Ratchet) st.ratchet = paintValue;
        pattern.setStep (hit.lane, hit.step, st);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { lastCell = {}; }

    // Dubbelklick tar bort låset (eller chansen/ratcheten) i det valda läget.
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (mode == StepEdit::Slag) return;
        const auto hit = cellAt (e.position);
        if (hit.lane < 0) return;
        auto st = pattern.getStep (hit.lane, hit.step);
        if (mode == StepEdit::Ton)          { st.tuneLocked = false; st.tune = 0; }
        else if (mode == StepEdit::Klang)   st.decay = 0;
        else if (mode == StepEdit::Chans)   st.chance = 0;
        else if (mode == StepEdit::Ratchet) st.ratchet = 0;
        pattern.setStep (hit.lane, hit.step, st);
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (mode != StepEdit::Ton && mode != StepEdit::Klang) return;
        const auto hit = cellAt (e.position);
        if (hit.lane < 0 || std::abs (w.deltaY) < 0.01f) return;
        auto st = pattern.getStep (hit.lane, hit.step);
        if (st.gate == 0) return;
        const int dir = (w.deltaY > 0) != w.isReversed ? 1 : -1;
        if (mode == StepEdit::Ton) { st.tune = juce::jlimit (-12, 12, (st.tuneLocked ? st.tune : 0) + dir); st.tuneLocked = true; }
        else                       st.decay = juce::jlimit (1, 7, (st.decay == 0 ? 4 : st.decay) + dir);
        pattern.setStep (hit.lane, hit.step, st);
        repaint();
    }

private:
    struct Cell { int lane = -1, step = -1; };

    Cell cellAt (juce::Point<float> p) const
    {
        const float cellW = ((float) getWidth() - labelW) / 16.0f;
        const float cellH = (float) getHeight() / (float) StepPattern::lanes;
        if (p.x < labelW) return {};
        const int col = (int) ((p.x - labelW) / cellW), lane = (int) (p.y / cellH);
        if (col < 0 || col >= 16 || lane < 0 || lane >= StepPattern::lanes) return {};
        return { lane, page * 16 + col };
    }

    static constexpr float labelW = 112.0f;
    StepPattern& pattern;
    std::atomic<int>& playingStep;
    std::array<juce::String, (size_t) StepPattern::lanes> laneNames;
    StepEdit mode = StepEdit::Slag;
    int page = 0, playablePages = 4, paintValue = 1, dragStartValue = 0;
    Cell lastCell;
};

class StegverketPanel : public juce::Component, private juce::Timer
{
public:
    StegverketPanel (StepPattern& p, std::atomic<int>& playing)
        : pattern (p), playingStep (playing), grid (p, playing)
    {
        play.setClickingTogglesState (true);
        play.setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
        play.setColour (juce::TextButton::buttonOnColourId, colours::tubeOrange.darker (0.3f));
        play.setColour (juce::TextButton::textColourOffId, colours::engraving);
        play.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        play.onStateChange = [this] { play.setButtonText (play.getToggleState() ? "STOPP" : "SPELA"); };

        for (auto* s : { &tempo, &swing })
        {
            s->setSliderStyle (juce::Slider::LinearHorizontal);
            s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
            s->setColour (juce::Slider::trackColourId, colours::brass);
            s->setColour (juce::Slider::thumbColourId, colours::brassLight);
            s->setColour (juce::Slider::backgroundColourId, colours::bakelite);
            s->setColour (juce::Slider::textBoxTextColourId, colours::engraving);
            s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        }
        length.setColour (juce::ComboBox::backgroundColourId, colours::bakelite);
        length.setColour (juce::ComboBox::textColourId, colours::engraving);
        length.setColour (juce::ComboBox::outlineColourId, colours::brass);
        length.onChange = [this] { updatePages(); };

        for (int i = 0; i < 4; ++i)
        {
            auto& b = pages[(size_t) i];
            b.setButtonText (juce::String (i + 1));
            b.setRadioGroupId (4711);
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b.setColour (juce::TextButton::buttonOnColourId, colours::brass);
            b.setColour (juce::TextButton::textColourOffId, colours::engraving);
            b.setColour (juce::TextButton::textColourOnId, colours::bakelite);
            b.onClick = [this, i] { grid.setPage (i); };
            addAndMakeVisible (b);
        }
        pages[0].setToggleState (true, juce::dontSendNotification);

        for (auto* b : { &clear, &standard })
        {
            b->setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b->setColour (juce::TextButton::textColourOffId, colours::engraving);
        }
        clear.onClick    = [this] { pattern.clear(); grid.repaint(); };
        standard.onClick = [this] { pattern.loadDefault(); grid.repaint(); };

        for (auto* c : std::initializer_list<juce::Component*> { &play, &tempo, &swing, &length, &clear, &standard, &grid })
            addAndMakeVisible (c);

        // Redigeringslägen: vad ett klick i rutnätet ändrar.
        static const char* modeNames[] = { "SLAG", "CHANS", "RATCH", "TON", "KLANG" };
        static const char* modeTips[] = { "Klick: slag, accent, av. Dra för att måla.",
                                          "Hur ofta steget slår: 100, 75, 50 eller 25 %.",
                                          "Ratchet: 1–4 snabba slag inom steget.",
                                          "Parameterlås: stämning för just det här slaget.",
                                          "Parameterlås: klangens längd för just det här slaget." };
        for (int i = 0; i < 5; ++i)
        {
            auto& b = modes[(size_t) i];
            b.setButtonText (modeNames[i]);
            b.setRadioGroupId (4713);
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
            b.setColour (juce::TextButton::buttonOnColourId, colours::brass);
            b.setColour (juce::TextButton::textColourOffId, colours::engraving);
            b.setColour (juce::TextButton::textColourOnId, colours::bakelite);
            b.setTooltip (juce::String::fromUTF8 (modeTips[i]));
            b.onClick = [this, i] { grid.setMode ((StepEdit) i); mode = i; repaint (hintArea); };
            addAndMakeVisible (b);
        }
        modes[0].setToggleState (true, juce::dontSendNotification);

        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });

        // Hjälptexten följer redigeringsläget.
        static const char* hints[] = {
            "Klick: slag / accent / av   Shift: accent   Dra: måla   Klick på spårnamnet: byt ljud",
            "Klick: 75 / 50 / 25 / 100 %   Shift: bakåt   Dra: måla   Dubbelklick: 100 %",
            "Klick: ×2 / ×3 / ×4 / ×1   Shift: bakåt   Dra: måla   Dubbelklick: ×1",
            "Dra upp/ned eller hjul: ±12 halvtoner för slaget   Dubbelklick: ta bort låset",
            "Dra upp/ned eller hjul: kortare eller längre klang   Dubbelklick: ta bort låset" };
        g.setColour (colours::engraving.withAlpha (0.55f));
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 (hints[juce::jlimit (0, 4, mode)]), hintArea, juce::Justification::centredLeft);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r.removeFromBottom (4.0f).expanded (2.0f, 0.0f), 0.0f, 2.0f);

        g.setColour (colours::engraving);
        g.setFont (panelFont (15.0f));
        g.drawText ("STEGVERKET", juce::Rectangle<int> (20, 10, 160, 26), juce::Justification::centredLeft);

        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        for (auto& [label, comp] : { std::pair<const char*, juce::Component*> { "TEMPO", &tempo }, { "GUNG %", &swing },
                                     { "STEG", &length }, { "SIDA", &pages[0] } })
            g.drawText (label, comp->getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        auto r = getLocalBounds().reduced (20, 0);
        auto top = r.removeFromTop (60).withTrimmedTop (24).withTrimmedBottom (6);
        top.removeFromLeft (120);
        play.setBounds (top.removeFromLeft (80));
        top.removeFromLeft (12);
        tempo.setBounds (top.removeFromLeft (140));
        top.removeFromLeft (12);
        swing.setBounds (top.removeFromLeft (120));
        top.removeFromLeft (12);
        length.setBounds (top.removeFromLeft (60));
        top.removeFromLeft (12);
        for (auto& b : pages) { b.setBounds (top.removeFromLeft (26)); top.removeFromLeft (3); }
        top.removeFromLeft (10);
        standard.setBounds (top.removeFromRight (78));
        top.removeFromRight (6);
        clear.setBounds (top.removeFromRight (60));

        auto bottom = r.removeFromBottom (24).reduced (0, 2);
        bottom.removeFromLeft (gridLabelWidth - 60);
        for (auto& b : modes) { b.setBounds (bottom.removeFromLeft (56)); bottom.removeFromLeft (3); }
        bottom.removeFromLeft (10);
        hintArea = bottom;
        grid.setBounds (r.reduced (0, 2));
    }

    // Rutnätets placering (för att linjera röstrattar mot spåren i Hammarverket-panelen).
    juce::Rectangle<int> getGridBounds() const { return grid.getBounds(); }
    static constexpr int gridLabelWidth = 112;

    // Öppnar en sida (0–3), som ett klick på sidknappen.
    void showPage (int i) { updatePages(); pages[(size_t) juce::jlimit (0, 3, i)].setToggleState (true, juce::sendNotificationSync); grid.setPage (juce::jlimit (0, 3, i)); }

    StepGrid& getGrid() { return grid; }
    // Väljer redigeringsläge (0 SLAG … 4 KLANG), som ett klick på knappen.
    void setMode (int i) { modes[(size_t) juce::jlimit (0, 4, i)].setToggleState (true, juce::sendNotificationSync); grid.setMode ((StepEdit) juce::jlimit (0, 4, i)); mode = juce::jlimit (0, 4, i); }

    juce::TextButton play { "SPELA" };
    juce::Slider tempo, swing;
    juce::ComboBox length;

private:
    std::array<juce::TextButton, 5> modes;
    juce::Rectangle<int> hintArea;
    int mode = 0;

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
        updatePages(); // längden kan ändras från DAW-automation eller ett preset

        // Lampan följer uppspelningen.
        const int step = playingStep.load();
        if (step != lastStep)
        {
            lastStep = step;
            grid.repaint();
        }
    }

    StepPattern& pattern;
    std::atomic<int>& playingStep;
    StepGrid grid;
    std::array<juce::TextButton, 4> pages;
    juce::TextButton clear { "RENSA" }, standard { "STANDARD" };
    int lastStep = -2, shownPages = -1;
    textures::BackgroundCache background;
};

} // namespace krang::ui
