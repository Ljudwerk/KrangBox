// Krang-stilprov (fas 0): rattar, VFD-display och visarmätare.
// Allt ritas som vektorer så att det skalar; genererade texturer läggs på i fas 2.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Textures.h"
#include <cmath>

namespace krang::ui
{

// Små dB-hjälpare, så att gränssnittet inte behöver juce_audio_basics.
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float gainToDb (float g, float floorDb = -40.0f) { return g > 0.0f ? juce::jmax (floorDb, 20.0f * std::log10 (g)) : floorDb; }

// --- Palett ----------------------------------------------------------------
namespace colours
{
    inline const juce::Colour chassis     { 0xff1d1c1b };
    inline const juce::Colour chassisHigh { 0xff2c2a28 };
    inline const juce::Colour brass       { 0xffb5874a };
    inline const juce::Colour brassLight  { 0xffe2c07f };
    inline const juce::Colour brassDark   { 0xff6e4f26 };
    inline const juce::Colour copper      { 0xffa8552e };
    inline const juce::Colour bakelite    { 0xff141210 };
    inline const juce::Colour engraving   { 0xffe8dcc4 };
    inline const juce::Colour tubeOrange  { 0xffff8a2a };
    inline const juce::Colour vfdGlass    { 0xff07110e };
    inline const juce::Colour vfdText     { 0xff6dffc8 };
    inline const juce::Colour meterFace   { 0xfff1e6c8 };
    inline const juce::Colour meterRed    { 0xffb3261e };
}

inline juce::Font panelFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)
                           .withKerningFactor (0.12f));
}

// Mässingsbricka med nit – används i hörn och runt instrument.
inline void drawRivet (juce::Graphics& g, juce::Point<float> c, float r)
{
    juce::ColourGradient grad (colours::brassLight, c.x - r * 0.5f, c.y - r * 0.5f,
                               colours::brassDark, c.x + r, c.y + r, true);
    g.setGradientFill (grad);
    g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 0.8f);
}

// Mässingsram med fasad kant.
inline void drawBrassBezel (juce::Graphics& g, juce::Rectangle<float> r, float corner, float width)
{
    juce::ColourGradient grad (colours::brassLight, r.getX(), r.getY(),
                               colours::brassDark, r.getRight(), r.getBottom(), false);
    grad.addColour (0.5, colours::brass);
    g.setGradientFill (grad);
    juce::Path p;
    p.addRoundedRectangle (r, corner);
    p.addRoundedRectangle (r.reduced (width), juce::jmax (0.0f, corner - width));
    p.setUsingNonZeroWinding (false);
    g.fillPath (p);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (width), juce::jmax (0.0f, corner - width), 1.0f);
}

// --- LookAndFeel ----------------------------------------------------------
class KrangLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KrangLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, colours::engraving);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, colours::engraving);
        setColour (juce::ComboBox::backgroundColourId, colours::bakelite);
        setColour (juce::ComboBox::textColourId, colours::engraving);
        setColour (juce::ComboBox::outlineColourId, colours::brass);
        setColour (juce::ComboBox::arrowColourId, colours::brass);
        setColour (juce::PopupMenu::backgroundColourId, colours::chassisHigh);
        setColour (juce::PopupMenu::textColourId, colours::engraving);
        setColour (juce::PopupMenu::headerTextColourId, colours::brass);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::tubeOrange.withAlpha (0.6f));
        setColour (juce::TextButton::buttonColourId, colours::chassisHigh);
        setColour (juce::TextButton::textColourOffId, colours::engraving);
        setColour (juce::TooltipWindow::backgroundColourId, colours::bakelite);
        setColour (juce::TooltipWindow::textColourId, colours::vfdText);
        setColour (juce::BubbleComponent::backgroundColourId, colours::bakelite);
        setColour (juce::BubbleComponent::outlineColourId, colours::brass);
    }

    // Bakelitratt i mässingsring, med graverad skala och rörorange värdebåge.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace juce;
        const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float size = jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float rScale = size * 0.5f;
        const float rRing  = size * 0.40f;
        const float rCap   = size * 0.31f;
        const float angle  = startAngle + pos * (endAngle - startAngle);

        // Graverad skala: 11 streck, varannan längre.
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float r0 = rScale * (i % 5 == 0 ? 0.84f : 0.89f);
            const auto p0 = c.getPointOnCircumference (r0, a);
            const auto p1 = c.getPointOnCircumference (rScale * 0.98f, a);
            g.setColour (colours::engraving.withAlpha (i % 5 == 0 ? 0.9f : 0.5f));
            g.drawLine ({ p0, p1 }, i % 5 == 0 ? 1.6f : 1.0f);
        }

        // Värdebåge med glöd.
        Path arc;
        arc.addCentredArc (c.x, c.y, rScale * 0.80f, rScale * 0.80f, 0.0f, startAngle, angle, true);
        g.setColour (colours::tubeOrange.withAlpha (0.25f));
        g.strokePath (arc, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (colours::tubeOrange);
        g.strokePath (arc, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // Skugga.
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (Rectangle<float> (rRing * 2, rRing * 2).withCentre (c.translated (0, size * 0.03f)));

        // Mässingsring.
        ColourGradient ring (colours::brassLight, c.x - rRing, c.y - rRing,
                             colours::brassDark, c.x + rRing, c.y + rRing, false);
        ring.addColour (0.45, colours::brass);
        g.setGradientFill (ring);
        g.fillEllipse (Rectangle<float> (rRing * 2, rRing * 2).withCentre (c));

        // Svarvspår i mässingen: tunna koncentriska ringar med varierande ljushet.
        for (int i = 0; i < 14; ++i)
        {
            const float rr = rCap + (rRing - rCap) * ((float) i + 0.5f) / 14.0f;
            const float shade = textures::detail::hash (i, (int) rRing, 5);
            g.setColour ((shade > 0.5f ? Colours::white : Colours::black).withAlpha (0.05f + 0.07f * std::abs (shade - 0.5f)));
            g.drawEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c), 0.7f);
        }

        // Räfflad kant: tunna mörka streck runt ringen.
        g.setColour (colours::brassDark.withAlpha (0.55f));
        for (int i = 0; i < 48; ++i)
        {
            const float a = (float) i / 48.0f * MathConstants<float>::twoPi;
            g.drawLine ({ c.getPointOnCircumference (rCap + 2.0f, a), c.getPointOnCircumference (rRing - 1.0f, a) }, 0.7f);
        }

        // Bakelitlock med blank reflex.
        ColourGradient cap (colours::chassisHigh, c.x, c.y - rCap, colours::bakelite, c.x, c.y + rCap, false);
        g.setGradientFill (cap);
        g.fillEllipse (Rectangle<float> (rCap * 2, rCap * 2).withCentre (c));
        ColourGradient shine (Colours::white.withAlpha (0.18f), c.x, c.y - rCap,
                              Colours::transparentWhite, c.x, c.y, false);
        g.setGradientFill (shine);
        g.fillEllipse (Rectangle<float> (rCap * 1.6f, rCap * 1.1f).withCentre (c.translated (0, -rCap * 0.35f)));

        // Visarstreck.
        g.setColour (colours::engraving);
        g.drawLine ({ c.getPointOnCircumference (rCap * 0.25f, angle), c.getPointOnCircumference (rCap * 0.92f, angle) }, 2.4f);
    }

    // Fader för envelopes: mörkt spår i mässingsram, bakelitgrepp med vit linje.
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s);
            return;
        }
        using namespace juce;
        const float cx = (float) x + (float) w * 0.5f;
        auto slot = Rectangle<float> (cx - 3.0f, (float) y, 6.0f, (float) h);
        g.setColour (colours::bakelite);
        g.fillRoundedRectangle (slot, 3.0f);
        g.setColour (colours::brassDark);
        g.drawRoundedRectangle (slot, 3.0f, 1.0f);
        g.setColour (colours::tubeOrange.withAlpha (0.8f));
        g.fillRoundedRectangle (slot.withTop (pos).reduced (1.5f, 0), 2.0f);

        auto cap = Rectangle<float> ((float) juce::jmin (w - 4, 26), 14.0f).withCentre ({ cx, pos });
        ColourGradient grad (colours::chassisHigh.brighter (0.2f), 0, cap.getY(), colours::bakelite, 0, cap.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (cap, 2.5f);
        g.setColour (colours::brass);
        g.drawRoundedRectangle (cap, 2.5f, 1.0f);
        g.setColour (colours::engraving);
        g.drawHorizontalLine ((int) cap.getCentreY(), cap.getX() + 3.0f, cap.getRight() - 3.0f);
    }

    int getSliderThumbRadius (juce::Slider& s) override
    {
        return s.getSliderStyle() == juce::Slider::LinearVertical ? 7 : LookAndFeel_V4::getSliderThumbRadius (s);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (panelFont (12.0f, false));
        l->setColour (juce::Label::textColourId, colours::engraving.withAlpha (0.8f));
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        return l;
    }
};

// Versaler som klarar å, ä, ö oavsett system.
inline juce::String VfdDisplay_upper (const juce::String& s)
{
    return s.toUpperCase()
            .replace (juce::String::fromUTF8 ("å"), juce::String::fromUTF8 ("Å"))
            .replace (juce::String::fromUTF8 ("ä"), juce::String::fromUTF8 ("Ä"))
            .replace (juce::String::fromUTF8 ("ö"), juce::String::fromUTF8 ("Ö"));
}

// Modulram: graverad rubrik, tunn mässingslist och nitar i hörnen.
inline void drawModuleFrame (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    if (textures::useAgedLook())
    {
        // Stilprovet: en unik, åldrad plåt med relief, slitna kanter, repor och rost som rinner från skruvarna.
        const int seed = textures::aged::seedFor (r);
        const auto img = textures::aged::makePlate ((int) r.getWidth(), (int) r.getHeight(), seed, 66.0f,
                                                    juce::Colour (0xff8c8270), 1.0f);
        {
            juce::Graphics::ScopedSaveState s (g);
            juce::Path clip;
            clip.addRoundedRectangle (r, 6.0f);
            g.reduceClipRegion (clip);
            g.drawImage (img, r);
        }
        // Plåten ligger infälld: skugga upptill och till vänster, ljus kant nedtill och till höger.
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawLine (r.getX() + 6.0f, r.getY() + 1.0f, r.getRight() - 6.0f, r.getY() + 1.0f, 1.5f);
        g.drawLine (r.getX() + 1.0f, r.getY() + 6.0f, r.getX() + 1.0f, r.getBottom() - 6.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (r.getX() + 6.0f, r.getBottom() - 1.5f, r.getRight() - 6.0f, r.getBottom() - 1.5f, 1.0f);
        g.setColour (colours::brassDark.darker (0.3f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.2f);
        // Graverad titel: mörk fördjupning med ljus underkant, i stället för tryckt text.
        const auto titleArea = r.withHeight (24.0f).reduced (12.0f, 0);
        g.setFont (panelFont (12.0f));
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawText (VfdDisplay_upper (title), titleArea.translated (0.0f, -1.0f), juce::Justification::centredLeft);
        g.setColour (colours::engraving.withAlpha (0.85f));
        g.drawText (VfdDisplay_upper (title), titleArea, juce::Justification::centredLeft);
        g.setColour (colours::brass.withAlpha (0.25f));
        g.drawHorizontalLine ((int) r.getY() + 24, r.getX() + 10.0f, r.getRight() - 10.0f);
        juce::Random rnd (seed);
        for (auto p : { r.getTopLeft().translated (6, 6), r.getTopRight().translated (-6, 6),
                        r.getBottomLeft().translated (6, -6), r.getBottomRight().translated (-6, -6) })
            textures::aged::drawScrew (g, p, 3.2f, rnd.nextFloat() * 3.14159f);
        return;
    }
    textures::fill (g, r, textures::plate(), 1.0f, 6.0f);
    textures::fill (g, r, textures::rust(), 0.25f, 6.0f);
    // Insänkt plåt: mörk kant upptill, ljus nedtill.
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 6.0f, r.getRight() - 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) r.getBottom() - 2, r.getX() + 6.0f, r.getRight() - 6.0f);
    g.setColour (colours::brassDark);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.2f);
    g.setColour (colours::brass.withAlpha (0.35f));
    g.drawHorizontalLine ((int) r.getY() + 24, r.getX() + 10.0f, r.getRight() - 10.0f);
    g.setColour (colours::engraving);
    g.setFont (panelFont (12.0f));
    g.drawText (VfdDisplay_upper (title), r.withHeight (24.0f).reduced (12.0f, 0), juce::Justification::centredLeft);
    for (auto p : { r.getTopLeft().translated (6, 6), r.getTopRight().translated (-6, 6),
                    r.getBottomLeft().translated (6, -6), r.getBottomRight().translated (-6, -6) })
        drawRivet (g, p, 2.5f);
}

// Kopparrör mellan två moduler (horisontellt), med flänsar.
inline void drawCopperPipe (juce::Graphics& g, float x0, float x1, float y)
{
    juce::ColourGradient pipe (colours::copper.brighter (0.4f), 0, y - 5.0f, colours::copper.darker (0.6f), 0, y + 5.0f, false);
    g.setGradientFill (pipe);
    g.fillRect (juce::Rectangle<float> (x0, y - 5.0f, x1 - x0, 10.0f));
    g.setColour (colours::brassDark);
    for (float x : { x0 + 2.0f, x1 - 6.0f })
        g.fillRoundedRectangle (x, y - 8.0f, 4.0f, 16.0f, 1.5f);
}

// --- Ratt med graverad etikett ----------------------------------------------
class KrangKnob : public juce::Component
{
public:
    explicit KrangKnob (const juce::String& labelText, bool showValue = true) : label (VfdDisplay_upper (labelText))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxTextColourId, colours::engraving.withAlpha (0.85f));
        if (showValue) slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        else           slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        if (label.isEmpty()) return;
        g.setColour (colours::engraving);
        g.setFont (panelFont (labelSize));
        g.drawText (label, getLocalBounds().removeFromTop (18), juce::Justification::centred);
    }

    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (label.isEmpty() ? 0 : 18)); }

    float labelSize = 12.5f;

    juce::Slider slider;

private:
    juce::String label;
};

// --- VFD-display ------------------------------------------------------------
class VfdDisplay : public juce::Component
{
public:
    void setLines (const juce::String& top, const juce::String& bottom)
    {
        const auto t = upper (top), b = upper (bottom);
        if (t != line1 || b != line2) { line1 = t; line2 = b; repaint(); }
    }

    // VFD:n visar bara versaler. toUpperCase() missar å, ä, ö på vissa system.
    static juce::String upper (const juce::String& s)
    {
        return s.toUpperCase()
                .replace (juce::String::fromUTF8 ("å"), juce::String::fromUTF8 ("Å"))
                .replace (juce::String::fromUTF8 ("ä"), juce::String::fromUTF8 ("Ä"))
                .replace (juce::String::fromUTF8 ("ö"), juce::String::fromUTF8 ("Ö"));
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r, 8.0f, 5.0f);
        auto glass = r.reduced (6.0f);

        g.setColour (colours::vfdGlass);
        g.fillRoundedRectangle (glass, 4.0f);
        textures::fill (g, glass, textures::plate(), 0.10f, 4.0f); // korn i glaset

        // Raster av glödtrådar och pixelrutnät, som på ett riktigt VFD-glas.
        g.setColour (colours::vfdText.withAlpha (0.05f));
        for (float x = glass.getX() + 2.0f; x < glass.getRight(); x += 3.0f)
            g.drawVerticalLine ((int) x, glass.getY(), glass.getBottom());
        g.setColour (colours::vfdText.withAlpha (0.08f));
        for (float y = glass.getY() + glass.getHeight() * 0.25f; y < glass.getBottom(); y += glass.getHeight() * 0.5f)
            g.drawHorizontalLine ((int) y, glass.getX() + 4.0f, glass.getRight() - 4.0f);

        auto text = glass.reduced (14.0f, 10.0f);
        drawGlowText (g, line1, text.removeFromTop (text.getHeight() * 0.5f), 22.0f);
        drawGlowText (g, line2, text, 17.0f);

        // Reflex i glaset.
        ColourGradient refl (Colours::white.withAlpha (0.07f), glass.getX(), glass.getY(),
                             Colours::transparentWhite, glass.getX(), glass.getCentreY(), false);
        g.setGradientFill (refl);
        g.fillRoundedRectangle (glass.withHeight (glass.getHeight() * 0.45f), 4.0f);
    }

private:
    static void drawGlowText (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size)
    {
        auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
        g.setFont (font);
        // Glöd: samma text några gånger med låg opacitet och liten förskjutning.
        g.setColour (colours::vfdText.withAlpha (0.12f));
        for (auto d : { juce::Point<float> (-1.5f, 0), { 1.5f, 0 }, { 0, -1.5f }, { 0, 1.5f } })
            g.drawText (s, r.translated (d.x, d.y), juce::Justification::centredLeft);
        g.setColour (colours::vfdText);
        g.drawText (s, r, juce::Justification::centredLeft);
    }

    juce::String line1 { "KRANGBOX" }, line2;
};

// --- Visarmätare (VU) ----------------------------------------------------------
class NeedleMeter : public juce::Component, private juce::Timer
{
public:
    NeedleMeter() { startTimerHz (30); }

    // Anropas från UI-tråden med senaste toppnivå (linjär).
    void setLevel (float linear) { target = linear; }
    // För stilprov och test: sätt visaren direkt utan tröghet.
    void setLevelImmediate (float linear) { target = linear; shown = toNorm (linear); repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r, 8.0f, 5.0f);
        auto face = r.reduced (6.0f);

        textures::fill (g, face, textures::meterPaper(), 1.0f, 4.0f);
        ColourGradient shade (Colours::transparentBlack, face.getCentreX(), face.getY(),
                              Colours::black.withAlpha (0.18f), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (shade);
        g.fillRoundedRectangle (face, 4.0f);

        const Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.18f);
        const float radius = face.getHeight() * 0.80f;
        const float a0 = -0.72f, a1 = 0.72f; // radianer från lodrätt

        // Röd zon från 0 VU och uppåt.
        Path red;
        red.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, angleFor (toNorm (1.0f), a0, a1), a1, true);
        g.setColour (colours::meterRed);
        g.strokePath (red, PathStrokeType (5.0f));

        Path scale;
        scale.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, a0, a1, true);
        g.setColour (Colours::black.withAlpha (0.8f));
        g.strokePath (scale, PathStrokeType (1.2f));

        // Streck och siffror: -20, -10, -5, -3, 0, +3 dB.
        g.setFont (panelFont (11.0f));
        for (int db : { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 })
        {
            const float a = angleFor (toNorm (dbToGain ((float) db)), a0, a1);
            const bool major = db == -20 || db == -10 || db == -5 || db == 0 || db == 3;
            const auto p0 = pivot.getPointOnCircumference (radius, a);
            const auto p1 = pivot.getPointOnCircumference (radius + (major ? 9.0f : 5.0f), a);
            g.setColour (db >= 0 ? colours::meterRed : Colours::black);
            g.drawLine ({ p0, p1 }, major ? 1.6f : 1.0f);
            if (major)
            {
                const auto tp = pivot.getPointOnCircumference (radius + 18.0f, a);
                g.drawText (db > 0 ? "+" + String (db) : String (db),
                            Rectangle<float> (30.0f, 14.0f).withCentre (tp), Justification::centred);
            }
        }

        g.setColour (Colours::black.withAlpha (0.75f));
        g.setFont (panelFont (15.0f));
        g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.50f).withHeight (20.0f), Justification::centred);
        g.setFont (panelFont (8.5f, false));
        face.removeFromBottom (face.getHeight() * 0.16f); // plats för navkåpan
        g.drawText (String::fromUTF8 ("KRANG & SON  ·  MANOMETERVERKEN"), face.removeFromBottom (14.0f), Justification::centred);

        // Visare med skugga.
        const float a = angleFor (shown, a0, a1);
        const auto tip = pivot.getPointOnCircumference (radius + 6.0f, a);
        const auto base = pivot.getPointOnCircumference (face.getHeight() * 0.32f, a);
        g.saveState();
        g.reduceClipRegion (getLocalBounds().reduced (6));
        g.setColour (Colours::black.withAlpha (0.2f));
        g.drawLine ({ base.translated (2, 2), tip.translated (2, 2) }, 1.6f);
        g.setColour (Colours::black);
        g.drawLine ({ base, tip }, 1.6f);
        g.restoreState();

        // Navkåpa längst ned döljer visarens fäste, som på en riktig VU.
        auto full = getLocalBounds().toFloat().reduced (6.0f);
        auto hub = full.removeFromBottom (full.getHeight() * 0.16f);
        ColourGradient hubGrad (colours::chassisHigh, hub.getX(), hub.getY(), colours::bakelite, hub.getX(), hub.getBottom(), false);
        g.setGradientFill (hubGrad);
        g.fillRect (hub);
        g.setColour (colours::brass);
        g.drawHorizontalLine ((int) hub.getY(), hub.getX(), hub.getRight());
    }

private:
    // dB -> 0..1 längs skalan (-20 dB till +3 dB).
    static float toNorm (float linear)
    {
        const float db = gainToDb (linear);
        return juce::jlimit (0.0f, 1.0f, (db + 20.0f) / 23.0f);
    }
    static float angleFor (float norm, float a0, float a1) { return a0 + norm * (a1 - a0); }

    void timerCallback() override
    {
        // Mekanisk tröghet: ca 300 ms stigtid som en äkta VU.
        const float t = toNorm (target);
        shown += (t - shown) * 0.25f;
        target *= 0.85f;
        repaint();
    }

    float target = 0.0f, shown = 0.0f;
};

// --- Toppanelen: display, tre rattar och mätare ---------------------------------
// --- Radiorör som glöder ------------------------------------------------------
// Glöden följer hur hårt maskinen drivs (Krangdrive och Krangspaken), med ett lätt flimmer.
class TubeGlow : public juce::Component, private juce::Timer
{
public:
    TubeGlow() { startTimerHz (30); }

    void setGlow (float g) { target = juce::jlimit (0.0f, 1.0f, g); }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        const float w = r.getWidth(), h = r.getHeight();

        // Sockel i bakelit med mässingsring.
        auto base = r.removeFromBottom (h * 0.18f);
        g.setColour (colours::bakelite);
        g.fillRoundedRectangle (base, 3.0f);
        g.setColour (colours::brass);
        g.drawRoundedRectangle (base, 3.0f, 1.0f);

        // Glaskolv: rundad topp.
        Path bulb;
        bulb.addRoundedRectangle (r.getX() + w * 0.1f, r.getY(), w * 0.8f, r.getHeight() + 2.0f, w * 0.4f, w * 0.4f,
                                  true, true, false, false);

        // Glöd bakom glaset.
        const float glow = shown;
        ColourGradient halo (colours::tubeOrange.withAlpha (0.55f * glow), r.getCentreX(), r.getCentreY() + h * 0.1f,
                             colours::tubeOrange.withAlpha (0.0f), r.getCentreX(), r.getY() - h * 0.1f, true);
        g.setGradientFill (halo);
        g.fillEllipse (r.expanded (w * 0.4f, h * 0.15f));

        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillPath (bulb);

        // Anod (plåtcylinder) och glödtråd.
        auto plate = Rectangle<float> (w * 0.36f, h * 0.38f).withCentre ({ r.getCentreX(), r.getCentreY() + h * 0.05f });
        g.setColour (Colour (0xff3a3632));
        g.fillRect (plate);
        g.setColour (Colour (0xff5a5550));
        g.drawRect (plate, 1.0f);
        const auto filament = colours::tubeOrange.interpolatedWith (Colour (0xffffe2a8), glow * 0.6f);
        g.setColour (filament.withAlpha (0.25f + 0.75f * glow));
        for (float dx : { -0.06f, 0.06f })
            g.drawLine (r.getCentreX() + w * dx, plate.getY() - h * 0.08f, r.getCentreX() + w * dx, plate.getBottom() + h * 0.04f,
                        1.2f + 1.5f * glow);

        // Reflex i glaset och kontur.
        g.setColour (Colours::white.withAlpha (0.18f));
        g.drawLine (r.getX() + w * 0.25f, r.getY() + h * 0.12f, r.getX() + w * 0.25f, r.getY() + h * 0.55f, 2.0f);
        g.setColour (Colours::white.withAlpha (0.25f));
        g.strokePath (bulb, PathStrokeType (1.0f));
    }

private:
    void timerCallback() override
    {
        // Snabb uppvärmning, långsammare avsvalning, och lite flimmer när röret är varmt.
        const float k = target > shown ? 0.25f : 0.06f;
        float next = shown + (target - shown) * k;
        next *= 1.0f - 0.06f * random.nextFloat() * next;
        if (std::abs (next - shown) > 0.002f) { shown = next; repaint(); }
    }

    float target = 0.0f, shown = 0.0f;
    juce::Random random;
};

// --- Krangspaken -------------------------------------------------------------------
// En tung spak på en mässingsaxel: dra uppåt för att förstöra allt. Använder en vanlig Slider
// (osynlig) för värde, automation och dubbelklick, och ritar spaken ovanpå.
class KrangLever : public juce::Component
{
public:
    KrangLever()
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setAlpha (0.0f);  // osynlig men tar emot musen
        slider.setDoubleClickReturnValue (true, 0.0);
        slider.setPopupDisplayEnabled (true, true, nullptr);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        g.setColour (colours::engraving);
        g.setFont (panelFont (11.0f));
        g.drawText ("KRANGSPAKEN", r.removeFromTop (16.0f), Justification::centred);

        auto area = r.reduced (4.0f, 4.0f);
        const float norm = (float) slider.valueToProportionOfLength (slider.getValue());

        // Spår med skala: grönt -> rött mot toppen.
        auto slot = Rectangle<float> (12.0f, area.getHeight() - 30.0f).withCentre ({ area.getCentreX(), area.getCentreY() - 6.0f });
        g.setColour (colours::bakelite);
        g.fillRoundedRectangle (slot.expanded (5.0f, 3.0f), 6.0f);
        drawBrassBezel (g, slot.expanded (5.0f, 3.0f), 6.0f, 2.0f);
        for (int i = 0; i <= 5; ++i)
        {
            const float y = slot.getBottom() - slot.getHeight() * (float) i / 5.0f;
            g.setColour (Colour (0xff4a8a3a).interpolatedWith (colours::meterRed, (float) i / 5.0f));
            g.drawHorizontalLine ((int) y, slot.getRight() + 6.0f, slot.getRight() + 13.0f);
            g.drawHorizontalLine ((int) y, slot.getX() - 13.0f, slot.getX() - 6.0f);
        }

        // Spakarm från axeln längst ned upp till handtaget.
        const Point<float> pivot (slot.getCentreX(), slot.getBottom() + 14.0f);
        const float handleY = slot.getBottom() - norm * slot.getHeight();
        const Point<float> handle (slot.getCentreX(), handleY);
        g.setColour (Colour (0xff8c8a86));
        g.drawLine ({ pivot, handle }, 5.0f);
        g.setColour (Colours::white.withAlpha (0.3f));
        g.drawLine ({ pivot.translated (-1.5f, 0), handle.translated (-1.5f, 0) }, 1.0f);
        drawRivet (g, pivot, 7.0f);

        // Handtag: röd kula som glöder mer ju högre spaken står.
        auto ball = Rectangle<float> (24.0f, 24.0f).withCentre (handle);
        g.setColour (colours::meterRed.withAlpha (0.35f * norm));
        g.fillEllipse (ball.expanded (6.0f * norm));
        ColourGradient grad (colours::meterRed.brighter (0.6f), ball.getX() + 6.0f, ball.getY() + 5.0f,
                             colours::meterRed.darker (0.6f), ball.getRight(), ball.getBottom(), true);
        g.setGradientFill (grad);
        g.fillEllipse (ball);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawEllipse (ball, 1.0f);
    }

    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (16)); }

    juce::Slider slider;
};

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
    KrangKnob drive { "Krangdrive" }, katedral { "Katedral" }, master { "Master" };
    NeedleMeter meter;
    KrangLever lever;
    TubeGlow tube;

private:
    juce::Rectangle<int> knobRow;
    textures::BackgroundCache background;
};

} // namespace krang::ui
