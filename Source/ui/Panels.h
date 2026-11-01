// Krang-panelen (fas 2): modulerna under frontpanelen.
//   BasmalenPanel      – oscillatorer, rörelse, filter, två envelopes (faders)
//   HammarverketPanel  – Stegverket + en rad röstrattar per trumspår, Rost
//   AngledningenPanel  – Krangdistortion -> Ångfilter -> Bandmaskinen -> Ekokammaren -> Katedralreverb,
//                        sammankopplade med kopparrör
//   KaosPanel          – Kaosmotorn (slump och glitch, med kaoslampa) och Kaosskopet (oscilloskop på utgången)
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
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
    }

    void resized() override
    {
        background.invalidate();
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
    textures::BackgroundCache background;
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

// --- Bordunorgeln ---------------------------------------------------------------

class BordunorgelnPanel : public juce::Component
{
public:
    BordunorgelnPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &timbre, &bow, &drift, &drone, &droneNote, &symp, &attack, &release })
            addAndMakeVisible (c);
        droneNote.onChange = [this] { repaint (stringsArea); };
        symp.slider.onValueChange = [this] { repaint (stringsArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintStrings (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("BORDUNTON", droneNote.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto klang = r.removeFromLeft (360);   r.removeFromLeft (12);
        auto bordun = r.removeFromLeft (250);  r.removeFromLeft (12);
        auto env = r.removeFromRight (200);    r.removeFromRight (12);
        auto res = r;
        frames = { { klang, juce::String::fromUTF8 ("Klang") }, { bordun, "Bordun" },
                   { res, juce::String::fromUTF8 ("Resonanssträngar") }, { env, "Envelope" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28).reduced (6, 4).withSizeKeepingCentre (f.getWidth() - 12, 140); };
        layoutRow (knobRow (klang), { &timbre, &bow, &drift }, 100);

        auto b = knobRow (bordun);
        drone.setBounds (b.removeFromLeft (110));
        droneNote.setBounds (b.withSizeKeepingCentre (90, 26));

        auto rr = res.withTrimmedTop (28).reduced (12, 8);
        symp.setBounds (rr.removeFromLeft (110).withSizeKeepingCentre (110, 140));
        stringsArea = rr.reduced (10, 20);

        layoutRow (env.withTrimmedTop (28).reduced (6, 8), { &attack, &release }, 70);
    }

    Bindings bindings()
    {
        return { { { &timbre.slider, "boTimbre" }, { &bow.slider, "boBow" }, { &drift.slider, "boDrift" },
                   { &drone.slider, "boDrone" }, { &symp.slider, "boSymp" },
                   { &attack.slider, "boAttack" }, { &release.slider, "boRelease" } },
                 { { &droneNote, "boDroneNote" } } };
    }

private:
    // Sex resonanssträngar, stämda efter bordunen (samma intervall som i DSP:n), med stämskruvar i mässing.
    void paintStrings (juce::Graphics& g)
    {
        if (stringsArea.isEmpty()) return;
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "H" };
        static constexpr int intervals[6] = { 0, 7, 12, 15, 17, 19 };
        const int root = juce::jmax (0, droneNote.getSelectedItemIndex());
        const auto a = stringsArea.toFloat();
        const float glow = (float) symp.slider.valueToProportionOfLength (symp.slider.getValue());
        for (int k = 0; k < 6; ++k)
        {
            const float y = a.getY() + a.getHeight() * ((float) k + 0.5f) / 6.0f;
            drawRivet (g, { a.getX() + 30.0f, y }, 4.0f);
            drawRivet (g, { a.getRight() - 6.0f, y }, 3.0f);
            g.setColour (colours::brassLight.withAlpha (0.35f + 0.5f * glow));
            g.drawLine (a.getX() + 34.0f, y, a.getRight() - 9.0f, y, 0.8f + 0.25f * (float) (5 - k));
            g.setColour (colours::engraving.withAlpha (0.8f));
            g.setFont (panelFont (10.0f));
            g.drawText (names[(root + intervals[k]) % 12], juce::Rectangle<float> (a.getX(), y - 7.0f, 22.0f, 14.0f),
                        juce::Justification::centredRight);
        }
    }

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    KrangKnob timbre { u8 ("Klangfärg") }, bow { u8 ("Stråke") }, drift { "Drift" },
              drone { "Bordun" }, symp { "Resonans" };
    juce::ComboBox droneNote;
    KrangFader attack { "ATTACK" }, release { "RELEASE" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> stringsArea;
    textures::BackgroundCache background;
};

// --- Stålpiano ------------------------------------------------------------------

class StalpianoPanel : public juce::Component
{
public:
    StalpianoPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &mode, &material, &hard, &pos, &stiff, &detune, &decay, &damper, &body, &rust })
            addAndMakeVisible (c);
        mode.onChange = [this] { repaint (stringsArea); };
        material.onChange = [this] { repaint (stringsArea); };
        pos.slider.onValueChange = [this] { repaint (stringsArea); };
        rust.slider.onValueChange = [this] { repaint (stringsArea); };
        detune.slider.onValueChange = [this] { repaint (stringsArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintStrings (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("LÄGE"), mode.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("MATERIAL", material.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto hammer = r.removeFromLeft (310);  r.removeFromLeft (12);
        auto str    = r.removeFromLeft (420);  r.removeFromLeft (12);
        auto bot    = r.removeFromRight (220); r.removeFromRight (12);
        auto vis    = r;
        frames = { { hammer, "Hammaren" }, { str, juce::String::fromUTF8 ("Strängar") },
                   { vis, juce::String::fromUTF8 ("Strängbädden") }, { bot, "Klangbotten" } };

        // Rattarna ligger på samma höjd i alla moduler; Hammaren har lägesväljaren ovanför.
        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        mode.setBounds (hammer.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        material.setBounds (str.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (hammer), { &hard, &pos }, 100);
        layoutRow (knobRow (str), { &stiff, &detune, &decay, &damper }, 96);
        layoutRow (knobRow (bot), { &body, &rust }, 96);
        stringsArea = vis.withTrimmedTop (34).reduced (14, 12);
    }

    Bindings bindings()
    {
        return { { { &hard.slider, "stHard" }, { &pos.slider, "stPos" }, { &stiff.slider, "stStiff" },
                   { &detune.slider, "stDetune" }, { &decay.slider, "stDecay" }, { &damper.slider, "stDamper" },
                   { &body.slider, "stBody" }, { &rust.slider, "stRust" } },
                 { { &mode, "stMode" }, { &material, "stMaterial" } } };
    }

private:
    // Strängbädden: nio stålsträngar i tre kor, hammaren vid anslagspunkten, rost och skruvar efter inställningarna.
    void paintStrings (juce::Graphics& g)
    {
        using namespace juce;
        if (stringsArea.isEmpty()) return;
        const auto a = stringsArea.toFloat();
        const int m = jmax (0, mode.getSelectedItemIndex());
        const int mat = jmax (0, material.getSelectedItemIndex());
        // Strängarnas färg och tjocklek efter material: stål, mässing, koppar, järn, silke, djurhud.
        static const Colour matColour[6] = { Colour (0xffc9ccd0), Colour (0xffd9b45a), Colour (0xffc0703e),
                                             Colour (0xff5d5652), Colour (0xffeee3c8), Colour (0xff8d6238) };
        static constexpr float matWidth[6] = { 1.0f, 1.1f, 1.5f, 1.2f, 1.1f, 2.4f };
        const float rustAmount = (float) rust.slider.valueToProportionOfLength (rust.slider.getValue());
        const float spread = (float) detune.slider.valueToProportionOfLength (detune.slider.getValue());
        const float hammerX = a.getX() + 18.0f + (a.getWidth() - 36.0f)
                            * (float) pos.slider.valueToProportionOfLength (pos.slider.getValue());

        // Stall och stämskruvar.
        g.setColour (colours::brassDark);
        g.fillRect (a.getX() + 8.0f, a.getY(), 5.0f, a.getHeight());
        g.fillRect (a.getRight() - 13.0f, a.getY(), 5.0f, a.getHeight());

        constexpr int courses = 4, perCourse = 3;
        for (int c = 0; c < courses; ++c)
            for (int k = 0; k < perCourse; ++k)
            {
                const float y = a.getY() + 10.0f + (a.getHeight() - 20.0f) * ((float) c + 0.18f + 0.32f * (float) k) / (float) courses;
                drawRivet (g, { a.getRight() - 4.0f, y }, 2.5f);
                const auto steel = matColour[mat].interpolatedWith (Colour (0xff8a4a22), rustAmount * (0.6f + 0.4f * (float) ((c + k) % 2)));
                g.setColour (steel.withAlpha (0.85f));
                // Ostämda strängar ritas en aning sneda mot varandra.
                const float tilt = (float) (k - 1) * spread * 2.5f;
                const float width = matWidth[mat] * (1.0f + 0.35f * (float) (courses - c));
                if (mat == 5)
                {
                    // Djurhud: tjock, ojämn och lite vågig.
                    Path p;
                    p.startNewSubPath (a.getX() + 10.0f, y);
                    for (float x = a.getX() + 10.0f; x <= a.getRight() - 6.0f; x += 6.0f)
                    {
                        const float t = (x - a.getX()) / a.getWidth();
                        p.lineTo (x, y + tilt * t + 0.8f * std::sin (x * 0.21f + (float) (c * 3 + k)));
                    }
                    g.strokePath (p, PathStrokeType (width));
                }
                else
                {
                    g.drawLine (a.getX() + 10.0f, y, a.getRight() - 6.0f, y + tilt, width);
                    if (mat == 2) // koppar är lindad: ritas med tvärstreck
                    {
                        g.setColour (steel.darker (0.5f).withAlpha (0.6f));
                        for (float x = a.getX() + 12.0f; x < a.getRight() - 8.0f; x += 3.0f)
                            g.drawVerticalLine ((int) x, y - width * 0.5f, y + width * 0.5f);
                    }
                }
            }

        if (m == 2) // Preparerat: skruvar mellan strängarna
            for (int c = 0; c < courses; ++c)
            {
                const float x = a.getX() + a.getWidth() * (0.55f + 0.09f * (float) ((c * 7) % 4));
                const float y = a.getY() + 10.0f + (a.getHeight() - 20.0f) * ((float) c + 0.5f) / (float) courses;
                g.setColour (colours::brassLight);
                g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
                g.setColour (colours::bakelite);
                g.drawLine (x - 2.5f, y, x + 2.5f, y, 1.0f);
            }

        if (m == 1) // Hackbräde: två klubbor
        {
            for (int i = 0; i < 2; ++i)
            {
                const float y = a.getY() + a.getHeight() * (0.3f + 0.4f * (float) i);
                g.setColour (Colour (0xff6b4a2a));
                g.drawLine (hammerX, y, hammerX + 44.0f, y + 26.0f, 3.0f);
                g.setColour (Colour (0xffd9c39a));
                g.fillEllipse (hammerX - 5.0f, y - 5.0f, 10.0f, 10.0f);
            }
        }
        else // pianohammare med filt
        {
            auto h = Rectangle<float> (hammerX - 6.0f, a.getY() - 4.0f, 12.0f, a.getHeight() + 8.0f);
            g.setColour (Colour (0xff5a4632));
            g.fillRoundedRectangle (h, 4.0f);
            g.setColour (Colour (0xffd9cfb8).withAlpha (0.9f));
            g.fillRoundedRectangle (h.reduced (3.0f, 6.0f), 3.0f);
        }

        if (rustAmount > 0.01f)
            textures::fill (g, a, textures::rust(), rustAmount * 0.35f, 3.0f);
    }

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    juce::ComboBox mode, material;
    KrangKnob hard { u8 ("Hårdhet") }, pos { "Anslag" }, stiff { "Styvhet" }, detune { u8 ("Svävning") },
              decay { "Klingtid" }, damper { u8 ("Dämpare") }, body { "Botten" }, rust { "Rost" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> stringsArea;
    textures::BackgroundCache background;
};

// --- Klockgjuteriet ----------------------------------------------------------------

class KlockgjuterietPanel : public juce::Component
{
public:
    KlockgjuterietPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &hard, &hum, &beat, &crack, &sweep, &decay, &release })
            addAndMakeVisible (c);
        type.onChange = [this] { repaint (bellArea); };
        crack.slider.onValueChange = [this] { repaint (bellArea); };
        sweep.slider.setDoubleClickReturnValue (true, 0.0);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintBell (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("KLOCKTYP", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto bell = r.removeFromLeft (300);  r.removeFromLeft (12);
        auto cast = r.removeFromLeft (320);  r.removeFromLeft (12);
        auto klang = r.removeFromLeft (220); r.removeFromLeft (12);
        auto vis = r;
        frames = { { bell, "Klockan" }, { cast, "Gjutningen" }, { klang, "Klang" }, { vis, "Klockstapeln" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        type.setBounds (bell.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (bell), { &hard, &hum }, 100);
        layoutRow (knobRow (cast), { &beat, &crack, &sweep }, 100);
        layoutRow (knobRow (klang), { &decay, &release }, 100);
        bellArea = vis.withTrimmedTop (34).reduced (14, 12);
    }

    Bindings bindings()
    {
        return { { { &hard.slider, "klHard" }, { &hum.slider, "klHum" }, { &beat.slider, "klBeat" }, { &crack.slider, "klCrack" },
                   { &sweep.slider, "klSweep" }, { &decay.slider, "klDecay" }, { &release.slider, "klRelease" } },
                 { { &type, "klType" } } };
    }

private:
    // Klockstapeln: en bjälke med klockan som är vald, i mässing, med sprickan inritad.
    void paintBell (juce::Graphics& g)
    {
        using namespace juce;
        if (bellArea.isEmpty()) return;
        const auto a = bellArea.toFloat();
        const int t = jmax (0, type.getSelectedItemIndex());
        const float crackAmount = (float) crack.slider.valueToProportionOfLength (crack.slider.getValue());

        // Bjälken.
        auto beam = a.withHeight (10.0f);
        g.setColour (Colour (0xff4a3826));
        g.fillRoundedRectangle (beam, 2.0f);
        drawRivet (g, { beam.getX() + 8.0f, beam.getCentreY() }, 3.0f);
        drawRivet (g, { beam.getRight() - 8.0f, beam.getCentreY() }, 3.0f);

        const float cx = a.getCentreX(), top = beam.getBottom();
        const float h = a.getHeight() - 16.0f, w = jmin (a.getWidth() * 0.8f, h * 1.1f);
        Path shape;
        switch (t)
        {
            case 0: // kyrkklocka
                shape.startNewSubPath (cx - w * 0.18f, top + 8.0f);
                shape.cubicTo (cx - w * 0.22f, top + h * 0.45f, cx - w * 0.30f, top + h * 0.75f, cx - w * 0.48f, top + h * 0.92f);
                shape.lineTo (cx + w * 0.48f, top + h * 0.92f);
                shape.cubicTo (cx + w * 0.30f, top + h * 0.75f, cx + w * 0.22f, top + h * 0.45f, cx + w * 0.18f, top + 8.0f);
                shape.closeSubPath();
                break;
            case 1: // skälla
                shape.startNewSubPath (cx - w * 0.16f, top + 10.0f);
                shape.lineTo (cx + w * 0.16f, top + 10.0f);
                shape.lineTo (cx + w * 0.26f, top + h * 0.7f);
                shape.lineTo (cx - w * 0.26f, top + h * 0.7f);
                shape.closeSubPath();
                break;
            case 2: // gong: en skiva i snören
                shape.addEllipse (cx - w * 0.4f, top + 16.0f, w * 0.8f, w * 0.8f);
                break;
            case 3: // räls: en balk som hänger i två snören
                shape.addRoundedRectangle (cx - w * 0.45f, top + h * 0.5f, w * 0.9f, 14.0f, 2.0f);
                break;
            default: // skålklocka på en kudde
                shape.startNewSubPath (cx - w * 0.38f, top + h * 0.5f);
                shape.cubicTo (cx - w * 0.36f, top + h * 0.85f, cx + w * 0.36f, top + h * 0.85f, cx + w * 0.38f, top + h * 0.5f);
                shape.closeSubPath();
                break;
        }

        // Upphängning.
        g.setColour (colours::brassDark);
        if (t == 2 || t == 3)
        {
            const auto b = shape.getBounds();
            g.drawLine (b.getX() + b.getWidth() * 0.2f, top, b.getX() + b.getWidth() * 0.2f, b.getY() + 4.0f, 1.5f);
            g.drawLine (b.getRight() - b.getWidth() * 0.2f, top, b.getRight() - b.getWidth() * 0.2f, b.getY() + 4.0f, 1.5f);
        }
        else if (t != 4)
            g.fillRect (cx - 4.0f, top, 8.0f, 12.0f);

        const auto b = shape.getBounds();
        ColourGradient brass (colours::brassLight, b.getX(), b.getY(), colours::brassDark, b.getRight(), b.getBottom(), false);
        brass.addColour (0.45, colours::brass);
        g.setGradientFill (brass);
        g.fillPath (shape);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.strokePath (shape, PathStrokeType (1.2f));
        if (t == 2) // gongens knopp och ringar
        {
            g.setColour (colours::brassDark.withAlpha (0.7f));
            g.drawEllipse (b.reduced (b.getWidth() * 0.18f), 1.0f);
            g.fillEllipse (b.withSizeKeepingCentre (b.getWidth() * 0.2f, b.getHeight() * 0.2f));
        }
        textures::fill (g, b, textures::rust(), 0.15f + 0.4f * crackAmount, 0.0f);

        // Sprickan: en sicksack som blir längre ju större sprickan är.
        if (crackAmount > 0.02f)
        {
            Path c;
            float x = b.getX() + b.getWidth() * 0.62f, y = b.getBottom() - 2.0f;
            c.startNewSubPath (x, y);
            const int steps = 2 + (int) (crackAmount * 8.0f);
            for (int i = 0; i < steps; ++i)
            {
                x += (i % 2 == 0 ? -1.0f : 1.0f) * b.getWidth() * 0.05f;
                y -= b.getHeight() * 0.07f;
                c.lineTo (x, y);
            }
            g.setColour (Colours::black.withAlpha (0.85f));
            g.strokePath (c, PathStrokeType (1.4f));
        }
    }

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    juce::ComboBox type;
    KrangKnob hard { u8 ("Kläpp") }, hum { "Brum" }, beat { u8 ("Svävning") }, crack { "Spricka" }, sweep { "Svep" },
              decay { "Klingtid" }, release { u8 ("Släpp") };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> bellArea;
    textures::BackgroundCache background;
};

// --- Spolslingan ----------------------------------------------------------------

class SpolslinganPanel : public juce::Component
{
public:
    SpolslinganPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &ratio, &index, &decay, &harp, &mouth, &grain, &grainMs, &spread,
                                                                 &arpMode, &arpRate, &arpOct, &attack, &release })
            addAndMakeVisible (c);
        decay.labelSize = 11.0f;
        arpOct.labelSize = 11.0f;
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
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("RIKTNING", arpMode.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("TAKT", arpRate.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto fm    = r.removeFromLeft (300); r.removeFromLeft (12);
        auto mun   = r.removeFromLeft (210); r.removeFromLeft (12);
        auto korn  = r.removeFromLeft (300); r.removeFromLeft (12);
        auto env   = r.removeFromRight (130); r.removeFromRight (12);
        auto slinga = r;
        frames = { { fm, "FM-spolen" }, { mun, "Munharpa" }, { korn, "Radiokorn" }, { slinga, "Slingan" }, { env, "Envelope" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28).reduced (6, 4).withSizeKeepingCentre (f.getWidth() - 12, 140); };
        layoutRow (knobRow (fm),   { &ratio, &index, &decay }, 92);
        layoutRow (knobRow (mun),  { &harp, &mouth }, 92);
        layoutRow (knobRow (korn), { &grain, &grainMs, &spread }, 92);

        auto sl = slinga.withTrimmedTop (28).reduced (12, 8);
        auto combos = sl.removeFromLeft (90);
        arpMode.setBounds (combos.withSizeKeepingCentre (88, 26).translated (0, -34));
        arpRate.setBounds (combos.withSizeKeepingCentre (88, 26).translated (0, 34));
        sl.removeFromLeft (4);
        arpOct.setBounds (sl.withSizeKeepingCentre (juce::jmin (sl.getWidth(), 92), 140));

        layoutRow (env.withTrimmedTop (28).reduced (6, 8), { &attack, &release }, 56);
    }

    Bindings bindings()
    {
        return { { { &ratio.slider, "spRatio" }, { &index.slider, "spIndex" }, { &decay.slider, "spDecay" },
                   { &harp.slider, "spHarp" }, { &mouth.slider, "spMouth" }, { &grain.slider, "spGrain" },
                   { &grainMs.slider, "spGrainMs" }, { &spread.slider, "spSpread" }, { &arpOct.slider, "spArpOct" },
                   { &attack.slider, "spAttack" }, { &release.slider, "spRelease" } },
                 { { &arpMode, "spArp" }, { &arpRate, "spArpRate" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    KrangKnob ratio { "Ratio" }, index { "Index" }, decay { "Avklingning" },
              harp { "Munharpa" }, mouth { u8 ("Munhåla") },
              grain { "Korn" }, grainMs { "Storlek" }, spread { "Spridning" },
              arpOct { "Oktaver" };
    juce::ComboBox arpMode, arpRate;
    KrangFader attack { "ATTACK" }, release { "RELEASE" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    textures::BackgroundCache background;
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
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
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
        background.invalidate();
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
    textures::BackgroundCache background;
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
        for (auto* c : std::initializer_list<juce::Component*> { &distMode, &fxMode, &ecSync, &rvbMode })
            addAndMakeVisible (c);
        for (auto* k : allKnobs())
        {
            addAndMakeVisible (k);
            k->labelSize = 10.0f;
        }
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        const float y = (float) modules[0].getCentreY() + 14.0f;
        for (int i = 0; i + 1 < numModules; ++i)
            drawCopperPipe (g, (float) modules[(size_t) i].getRight(), (float) modules[(size_t) i + 1].getX(), y);

        const char* titles[numModules] = { "Krangdistortion", "Ångfilter", "Bandmaskinen", "Ekokammaren", "Katedralreverb" };
        for (int i = 0; i < numModules; ++i)
            drawModuleFrame (g, modules[(size_t) i].toFloat(), juce::String::fromUTF8 (titles[i]));

        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("ÅNGLEDNINGEN"), getLocalBounds().removeFromBottom (14).reduced (14, 0),
                    juce::Justification::centredRight);
    }

    void resized() override
    {
        background.invalidate();
        auto r = getLocalBounds().reduced (10, 8).withTrimmedBottom (8);
        // Alla rattar lika stora: modulens bredd följer antalet rattar, reverben får det som blir över.
        const int pipe = 10;
        const int numKnobs[numModules] = { 4, 3, 4, 4, 2 };
        for (int i = 0; i < numModules; ++i)
        {
            const int w = i == numModules - 1 ? r.getWidth() : numKnobs[i] * knobWidth + 6;
            modules[(size_t) i] = r.removeFromLeft (w);
            r.removeFromLeft (pipe);
        }

        auto place = [this] (juce::Rectangle<int> frame, juce::ComboBox* mode, std::initializer_list<juce::Component*> knobs)
        {
            auto in = frame.withTrimmedTop (28).reduced (6, 2);
            auto modeRow = in.removeFromTop (24);
            if (mode != nullptr)
                mode->setBounds (modeRow.withSizeKeepingCentre (juce::jmin (124, modeRow.getWidth() - 8), 22));
            in.removeFromTop (4);
            layoutRow (in, knobs, knobWidth);
        };
        place (modules[0], &distMode, { &distMix, &distBits, &distDown, &distOut });
        place (modules[1], &fxMode,   { &fxCutoff, &fxReso, &fxVowel });
        place (modules[2], nullptr,   { &tpWow, &tpFlutter, &tpAge, &tpDropout });
        place (modules[3], &ecSync,   { &ecTime, &ecFeedback, &ecDark, &ecMix });
        place (modules[4], &rvbMode,  { &rvbSize, &rvbDark });
    }

    Bindings bindings()
    {
        return { { { &distMix.slider, "distMix" }, { &distBits.slider, "distBits" }, { &distDown.slider, "distDown" },
                   { &distOut.slider, "distOut" }, { &fxCutoff.slider, "fxCutoff" }, { &fxReso.slider, "fxReso" },
                   { &fxVowel.slider, "fxVowel" },
                   { &tpWow.slider, "tpWow" }, { &tpFlutter.slider, "tpFlutter" }, { &tpAge.slider, "tpAge" },
                   { &tpDropout.slider, "tpDropout" },
                   { &ecTime.slider, "ecTime" }, { &ecFeedback.slider, "ecFeedback" }, { &ecDark.slider, "ecDark" },
                   { &ecMix.slider, "ecMix" },
                   { &rvbSize.slider, "rvbSize" }, { &rvbDark.slider, "rvbDark" } },
                 { { &distMode, "distMode" }, { &fxMode, "fxMode" }, { &ecSync, "ecSync" }, { &rvbMode, "rvbMode" } } };
    }

private:
    static constexpr int numModules = 5, knobWidth = 63;
    textures::BackgroundCache background;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    std::vector<KrangKnob*> allKnobs() { return { &distMix, &distBits, &distDown, &distOut, &fxCutoff, &fxReso, &fxVowel,
                                                  &tpWow, &tpFlutter, &tpAge, &tpDropout,
                                                  &ecTime, &ecFeedback, &ecDark, &ecMix, &rvbSize, &rvbDark }; }

    juce::ComboBox distMode, fxMode, ecSync, rvbMode;
    KrangKnob distMix { "Mix" }, distBits { "Bitar" }, distDown { "Sampl." }, distOut { "Ut" },
              fxCutoff { "Frekvens" }, fxReso { "Resonans" }, fxVowel { "Vokal" },
              tpWow { "Wow" }, tpFlutter { "Flutter" }, tpAge { u8 ("Ålder") }, tpDropout { "Avbrott" },
              ecTime { "Tid" }, ecFeedback { u8 ("Återk.") }, ecDark { u8 ("Mörker") }, ecMix { "Mix" },
              rvbSize { "Storlek" }, rvbDark { u8 ("Mörker") };
    std::array<juce::Rectangle<int>, numModules> modules;
};

// --- Kaosraden: Kaosmotorn och Kaosskopet ----------------------------------------------

// Oscilloskop i grönt glas: visar utgången, triggat på uppåtgående nollgenomgång så att tonen står still.
class Kaosskop : public juce::Component
{
public:
    static constexpr int numPoints = 1024;

    // Anropas från gränssnittets timer med de senaste samplen, äldst först.
    void setSamples (const float* data, int n)
    {
        // Leta efter en nollgenomgång uppåt i första halvan, så att vågen inte rullar.
        int start = 0;
        for (int i = 1; i < n - numPoints; ++i)
            if (data[i - 1] <= 0.0f && data[i] > 0.0f) { start = i; break; }
        const int count = juce::jmin (numPoints, n - start);
        for (int i = 0; i < count; ++i) points[(size_t) i] = data[start + i];
        for (int i = count; i < numPoints; ++i) points[(size_t) i] = 0.0f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        drawBrassBezel (g, r, 8.0f, 5.0f);
        auto glass = r.reduced (6.0f);
        g.setColour (colours::vfdGlass);
        g.fillRoundedRectangle (glass, 4.0f);

        // Rutnät som på ett gammalt mätinstrument.
        g.setColour (colours::vfdText.withAlpha (0.10f));
        for (int i = 1; i < 10; ++i)
            g.drawVerticalLine ((int) (glass.getX() + glass.getWidth() * (float) i / 10.0f), glass.getY() + 3.0f, glass.getBottom() - 3.0f);
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine ((int) (glass.getY() + glass.getHeight() * (float) i / 4.0f), glass.getX() + 3.0f, glass.getRight() - 3.0f);

        // Vågen med glöd: bred och svag under, smal och ljus över.
        Path wave;
        const float mid = glass.getCentreY(), amp = glass.getHeight() * 0.46f;
        for (int i = 0; i < numPoints; ++i)
        {
            const float x = glass.getX() + 4.0f + (glass.getWidth() - 8.0f) * (float) i / (float) (numPoints - 1);
            const float y = mid - jlimit (-1.0f, 1.0f, points[(size_t) i]) * amp;
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }
        g.setColour (colours::vfdText.withAlpha (0.18f));
        g.strokePath (wave, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (colours::vfdText.withAlpha (0.9f));
        g.strokePath (wave, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));

        ColourGradient refl (Colours::white.withAlpha (0.06f), glass.getX(), glass.getY(),
                             Colours::transparentWhite, glass.getX(), glass.getCentreY(), false);
        g.setGradientFill (refl);
        g.fillRoundedRectangle (glass.withHeight (glass.getHeight() * 0.45f), 4.0f);
    }

private:
    std::array<float, numPoints> points {};
};

// Kaoslampan: en röd glödlampa som blixtrar till när Kaosmotorn slår till.
class KaosLamp : public juce::Component, public juce::SettableTooltipClient
{
public:
    void flash() { level = 1.0f; repaint(); }
    void decay()
    {
        if (level <= 0.0f) return;
        level = juce::jmax (0.0f, level - 0.12f);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        auto bulb = r.withSizeKeepingCentre (jmin (r.getWidth(), r.getHeight()), jmin (r.getWidth(), r.getHeight())).reduced (2.0f);
        drawBrassBezel (g, bulb.expanded (2.0f), bulb.getWidth() * 0.5f + 2.0f, 3.0f);
        const auto off = Colour (0xff3a0d0a), on = Colour (0xffff3b24);
        g.setColour (off.interpolatedWith (on, level));
        g.fillEllipse (bulb);
        if (level > 0.0f)
        {
            g.setColour (on.withAlpha (0.35f * level));
            g.fillEllipse (bulb.expanded (4.0f * level));
        }
        g.setColour (Colours::white.withAlpha (0.25f));
        g.fillEllipse (bulb.reduced (bulb.getWidth() * 0.3f).translated (-bulb.getWidth() * 0.12f, -bulb.getHeight() * 0.12f));
    }

private:
    float level = 0.0f;
};

class KaosPanel : public juce::Component
{
public:
    KaosPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &sync, &lamp, &scope })
            addAndMakeVisible (c);
        for (auto* k : knobs())
        {
            addAndMakeVisible (k);
            k->labelSize = 10.0f;
        }
        lamp.setTooltip (juce::String::fromUTF8 ("Kaoslampan blixtrar när Kaosmotorn slår till"));
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        drawModuleFrame (g, motor.toFloat(), "Kaosmotorn");
        drawModuleFrame (g, scopeFrame.toFloat(), "Kaosskopet");
        g.setColour (colours::engraving);
        g.setFont (panelFont (10.0f));
        g.drawText ("TAKT", syncLabel, juce::Justification::centred);
    }

    void resized() override
    {
        background.invalidate();
        auto r = getLocalBounds().reduced (10, 6);
        motor = r.removeFromLeft (64 + 6 * 63 + 24);
        r.removeFromLeft (12);
        scopeFrame = r;

        auto in = motor.withTrimmedTop (26).reduced (8, 2);
        auto left = in.removeFromLeft (72);
        syncLabel = left.removeFromTop (16);
        sync.setBounds (left.removeFromTop (24).reduced (0, 1));
        lamp.setBounds (left.withSizeKeepingCentre (30, 30));
        in.removeFromLeft (8);
        layoutRow (in, { &amount, &rate, &drift, &stutter, &reverse, &crush }, 63);

        scope.setBounds (scopeFrame.withTrimmedTop (26).reduced (10, 4));
    }

    Bindings bindings()
    {
        return { { { &amount.slider, "kaAmount" }, { &rate.slider, "kaRate" }, { &drift.slider, "kaDrift" },
                   { &stutter.slider, "kaStutter" }, { &reverse.slider, "kaReverse" }, { &crush.slider, "kaCrush" } },
                 { { &sync, "kaSync" } } };
    }

    KaosLamp lamp;
    Kaosskop scope;

private:
    textures::BackgroundCache background;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    std::vector<KrangKnob*> knobs() { return { &amount, &rate, &drift, &stutter, &reverse, &crush }; }

    juce::ComboBox sync;
    KrangKnob amount { "Kaos" }, rate { "Fri takt" }, drift { "Drift" }, stutter { "Hack" },
              reverse { u8 ("Bakåt") }, crush { u8 ("Söndring") };
    juce::Rectangle<int> motor, scopeFrame, syncLabel;
};

} // namespace krang::ui
