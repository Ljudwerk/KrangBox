// Krang-stilprov (fas 0): rattar, VFD-display och visarmätare.
// Allt ritas som vektorer så att det skalar; genererade texturer läggs på i fas 2.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
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

// --- Ratt med graverad etikett ----------------------------------------------
class KrangKnob : public juce::Component
{
public:
    explicit KrangKnob (const juce::String& labelText) : label (labelText.toUpperCase())
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (colours::engraving);
        g.setFont (panelFont (12.5f));
        g.drawText (label, getLocalBounds().removeFromTop (18), juce::Justification::centred);
    }

    void resized() override { slider.setBounds (getLocalBounds().withTrimmedTop (18)); }

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

        ColourGradient paper (colours::meterFace, face.getX(), face.getY(),
                              colours::meterFace.darker (0.18f), face.getX(), face.getBottom(), false);
        g.setGradientFill (paper);
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
        for (auto* k : { &cutoff, &resonance, &drive })
            addAndMakeVisible (k);
        addAndMakeVisible (meter);
        buildBrushedMetal (1, 1);
    }

    void paint (juce::Graphics& g) override
    {
        g.drawImageAt (metal, 0, 0);

        // Mässingslist längst ned och kopparrör som binder ihop rattarna.
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r.removeFromBottom (6.0f).expanded (2.0f, 0.0f), 0.0f, 3.0f);

        if (! knobRow.isEmpty())
        {
            const float y = (float) cutoff.getBounds().getCentreY() + 8.0f;
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
        buildBrushedMetal (getWidth(), getHeight());
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
        r.removeFromRight (16);

        knobRow = r;
        const int w = r.getWidth() / 3;
        for (auto* k : { &cutoff, &resonance, &drive })
            k->setBounds (r.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (w, 130), 150));
    }

    VfdDisplay display;
    juce::ComboBox chamber; // kammarväljare: Basmålen, Hammarverket, ...

    // Presetväljare
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "SPARA" };
    KrangKnob cutoff { "Cutoff" }, resonance { "Resonans" }, drive { "Krangdrive" };
    NeedleMeter meter;

private:
    // Borstad mörk metall: tusentals tunna horisontella streck med slumpad ljushet.
    void buildBrushedMetal (int w, int h)
    {
        metal = juce::Image (juce::Image::RGB, juce::jmax (1, w), juce::jmax (1, h), true);
        juce::Graphics g (metal);
        juce::ColourGradient base (colours::chassisHigh, 0, 0, colours::chassis, 0, (float) h, false);
        g.setGradientFill (base);
        g.fillAll();
        juce::Random rnd (0x4b42); // fast frö: samma mönster varje gång
        for (int i = 0; i < h * 3; ++i)
        {
            const float y = rnd.nextFloat() * (float) h;
            const float x0 = rnd.nextFloat() * (float) w * 0.6f;
            const float len = (float) w * (0.2f + rnd.nextFloat() * 0.6f);
            g.setColour (juce::Colours::white.withAlpha (rnd.nextFloat() * 0.035f));
            g.drawHorizontalLine ((int) y, x0, x0 + len);
        }
    }

    juce::Image metal;
    juce::Rectangle<int> knobRow;
};

} // namespace krang::ui
