// Stegverkets panel: lamprutnät 8 spår × 16 steg (fyra sidor upp till 64), transport och inställningar.
// Klick växlar av -> slag -> accent -> av. Dra för att måla samma värde över flera steg.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "KrangStyle.h"
#include "../dsp/Stegverket.h"

namespace krang::ui
{

class StepGrid : public juce::Component
{
public:
    StepGrid (StepPattern& p, std::atomic<int>& playing) : pattern (p), playingStep (playing) {}

    void setPage (int newPage) { page = newPage; repaint(); }
    void setPlayablePages (int n) { if (n != playablePages) { playablePages = n; repaint(); } }
    int getPage() const { return page; }

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
            g.setColour (colours::engraving.withAlpha (0.85f));
            g.setFont (panelFont (11.0f));
            g.drawText (VfdDisplay::upper (String::fromUTF8 (drumName (lane))), Rectangle<float> (0, y, labelW - 8.0f, cellH),
                        Justification::centredRight);

            for (int col = 0; col < 16; ++col)
            {
                const int step = page * 16 + col;
                const int v = pattern.get (lane, step);
                auto cell = Rectangle<float> (labelW + col * cellW, y, cellW, cellH).reduced (3.0f, 3.0f);
                const bool isPlaying = step == playing;

                // Lampa: mörk bakelit när av, rörorange glöd när på, ljusare med mässingsring vid accent.
                g.setColour (colours::bakelite);
                g.fillRoundedRectangle (cell, 3.0f);
                if (v > 0)
                {
                    const auto c = v == 2 ? colours::tubeOrange.brighter (0.35f) : colours::tubeOrange.withAlpha (0.8f);
                    g.setColour (c.withAlpha (0.25f));
                    g.fillRoundedRectangle (cell.expanded (1.5f), 4.0f);
                    g.setColour (c);
                    g.fillRoundedRectangle (cell.reduced (2.0f), 2.5f);
                    if (v == 2)
                    {
                        g.setColour (colours::brassLight);
                        g.drawRoundedRectangle (cell.reduced (0.5f), 3.0f, 1.4f);
                    }
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
        const auto hit = cellAt (e.position);
        if (hit.lane < 0) return;
        const int current = pattern.get (hit.lane, hit.step);
        paintValue = e.mods.isShiftDown() ? 2 : (current + 1) % 3;  // Shift-klick = accent direkt
        pattern.set (hit.lane, hit.step, paintValue);
        lastCell = hit;
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto hit = cellAt (e.position);
        if (hit.lane < 0 || (hit.lane == lastCell.lane && hit.step == lastCell.step)) return;
        pattern.set (hit.lane, hit.step, paintValue);
        lastCell = hit;
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
    int page = 0, playablePages = 4, paintValue = 1;
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

        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
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

        g.setColour (colours::engraving.withAlpha (0.55f));
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("Klick: slag / accent / av   Shift-klick: accent   Dra: måla   "
                                            "I en DAW följer Stegverket DAW:ens transport när SPELA är på"),
                    getLocalBounds().removeFromBottom (22).reduced (20, 0), juce::Justification::centredLeft);
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

        r.removeFromBottom (22);
        grid.setBounds (r.reduced (0, 4));
    }

    // Rutnätets placering (för att linjera röstrattar mot spåren i Hammarverket-panelen).
    juce::Rectangle<int> getGridBounds() const { return grid.getBounds(); }
    static constexpr int gridLabelWidth = 112;

    // Öppnar en sida (0–3), som ett klick på sidknappen.
    void showPage (int i) { updatePages(); pages[(size_t) juce::jlimit (0, 3, i)].setToggleState (true, juce::sendNotificationSync); grid.setPage (juce::jlimit (0, 3, i)); }

    juce::TextButton play { "SPELA" };
    juce::Slider tempo, swing;
    juce::ComboBox length;

private:
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
