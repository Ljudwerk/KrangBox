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

// --- Kören i schaktet ----------------------------------------------------------------

class KorenPanel : public juce::Component
{
public:
    KorenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &voices, &singers, &spread, &vowel, &vibrato, &breath,
                                                                 &attack, &release, &speaker })
            addAndMakeVisible (c);
        voices.onChange = [this] { repaint (hornArea); };
        vowel.slider.onValueChange = [this] { repaint (hornArea); };
        singers.slider.onValueChange = [this] { repaint (hornArea); };
        speaker.slider.onValueChange = [this] { repaint (hornArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintHorn (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("RÖSTER"), voices.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto choir = r.removeFromLeft (290);  r.removeFromLeft (12);
        auto voice = r.removeFromLeft (310);  r.removeFromLeft (12);
        auto breathF = r.removeFromLeft (210); r.removeFromLeft (12);
        auto horn = r;
        frames = { { choir, juce::String::fromUTF8 ("Kören") }, { voice, juce::String::fromUTF8 ("Rösten") },
                   { breathF, "Andetaget" }, { horn, juce::String::fromUTF8 ("Högtalaren") } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        voices.setBounds (choir.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (choir), { &singers, &spread }, 100);
        layoutRow (knobRow (voice), { &vowel, &vibrato, &breath }, 96);
        layoutRow (knobRow (breathF), { &attack, &release }, 96);
        auto h = knobRow (horn);
        speaker.setBounds (h.removeFromLeft (100));
        hornArea = horn.withTrimmedTop (34).reduced (10, 10).withTrimmedLeft (106);
    }

    Bindings bindings()
    {
        return { { { &singers.slider, "koSingers" }, { &spread.slider, "koSpread" }, { &vowel.slider, "koVowel" },
                   { &vibrato.slider, "koVibrato" }, { &breath.slider, "koBreath" }, { &attack.slider, "koAttack" },
                   { &release.slider, "koRelease" }, { &speaker.slider, "koSpeaker" } },
                 { { &voices, "koVoices" } } };
    }

private:
    // Högtalaren: en tratt i mässing, vokalen i grönt glas och en lampa för varje sångare.
    void paintHorn (juce::Graphics& g)
    {
        using namespace juce;
        if (hornArea.isEmpty()) return;
        auto a = hornArea.toFloat();
        const bool kulning = voices.getSelectedItemIndex() == 3;
        const int n = kulning ? 1 : roundToInt (singers.slider.getValue());
        const float broken = (float) speaker.slider.valueToProportionOfLength (speaker.slider.getValue());

        // Vokalen i ett litet VFD-fönster.
        auto glass = a.removeFromTop (54.0f).withSizeKeepingCentre (54.0f, 50.0f);
        drawBrassBezel (g, glass, 6.0f, 3.0f);
        g.setColour (colours::vfdGlass);
        g.fillRoundedRectangle (glass.reduced (4.0f), 3.0f);
        static const char* vowels[] = { "A", "E", "I", "O", "U" };
        const String v (vowels[jlimit (0, 4, roundToInt (vowel.slider.valueToProportionOfLength (vowel.slider.getValue()) * 4.0))]);
        g.setColour (colours::vfdText.withAlpha (0.25f));
        g.setFont (panelFont (34.0f));
        g.drawText (v, glass.translated (0.0f, 1.0f), Justification::centred);
        g.setColour (colours::vfdText);
        g.setFont (panelFont (30.0f));
        g.drawText (v, glass, Justification::centred);

        // Tratten.
        a.removeFromTop (8.0f);
        auto lamps = a.removeFromBottom (16.0f);
        const float cx = a.getCentreX(), cy = a.getCentreY();
        Path horn;
        horn.startNewSubPath (cx - 8.0f, cy - 6.0f);
        horn.lineTo (cx - a.getWidth() * 0.42f, cy - a.getHeight() * 0.45f);
        horn.lineTo (cx - a.getWidth() * 0.42f, cy + a.getHeight() * 0.45f);
        horn.lineTo (cx - 8.0f, cy + 6.0f);
        horn.closeSubPath();
        ColourGradient brass (colours::brassLight, cx - a.getWidth() * 0.4f, cy - a.getHeight() * 0.4f, colours::brassDark, cx, cy + 10.0f, false);
        g.setGradientFill (brass);
        g.fillPath (horn);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.strokePath (horn, PathStrokeType (1.0f));
        g.setColour (Colour (0xff2a241e));
        g.fillRoundedRectangle (cx - 8.0f, cy - 12.0f, 26.0f, 24.0f, 4.0f); // drivsteget
        drawRivet (g, { cx + 5.0f, cy }, 4.0f);
        if (broken > 0.05f) // revor i tratten
        {
            g.setColour (Colours::black.withAlpha (0.8f));
            g.drawLine (cx - a.getWidth() * 0.30f, cy - a.getHeight() * 0.30f,
                        cx - a.getWidth() * 0.30f + 14.0f * broken, cy - a.getHeight() * 0.30f + 22.0f * broken, 1.4f);
            g.drawLine (cx - a.getWidth() * 0.36f, cy + a.getHeight() * 0.20f,
                        cx - a.getWidth() * 0.36f + 18.0f * broken, cy + a.getHeight() * 0.20f - 10.0f * broken, 1.2f);
            textures::fill (g, horn.getBounds(), textures::rust(), 0.35f * broken, 0.0f);
        }

        // En lampa per sångare.
        const float step = 14.0f, x0 = lamps.getCentreX() - step * 3.5f;
        for (int i = 0; i < 8; ++i)
        {
            const auto c = Point<float> (x0 + step * (float) i, lamps.getCentreY());
            g.setColour (i < n ? colours::tubeOrange : Colour (0xff3a2a1a));
            g.fillEllipse (c.x - 4.0f, c.y - 4.0f, 8.0f, 8.0f);
            if (i < n)
            {
                g.setColour (colours::tubeOrange.withAlpha (0.3f));
                g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
            }
        }
    }

    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    juce::ComboBox voices;
    KrangKnob singers { u8 ("Sångare") }, spread { "Spridning" }, vowel { "Vokal" }, vibrato { "Vibrato" },
              breath { "Andning" }, attack { "Insats" }, release { "Avklang" }, speaker { "Trasig" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> hornArea;
    textures::BackgroundCache background;
};

// --- Radiomasten ------------------------------------------------------------------

class RadiomastenPanel : public juce::Component
{
public:
    RadiomastenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &mode, &signal, &bandwidth, &noise, &drift, &fading, &storm,
                                                                 &attack, &release })
            addAndMakeVisible (c);
        storm.slider.onValueChange = [this] { repaint (dialArea); };
    }

    // Nålen pekar på den senast spelade tonen (anropas från editorns timer).
    void setTunedNote (int note)
    {
        if (note != tunedNote) { tunedNote = note; repaint (dialArea); }
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintDial (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText (juce::String::fromUTF8 ("SÄNDNING"), mode.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto rx = r.removeFromLeft (290);    r.removeFromLeft (12);
        auto ether = r.removeFromLeft (400); r.removeFromLeft (12);
        auto tone = r.removeFromLeft (210);  r.removeFromLeft (12);
        auto dial = r;
        frames = { { rx, "Mottagaren" }, { ether, "Etern" }, { tone, "Tonen" }, { dial, "Radioskalan" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        mode.setBounds (rx.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (rx), { &signal, &bandwidth }, 100);
        layoutRow (knobRow (ether), { &noise, &drift, &fading, &storm }, 92);
        layoutRow (knobRow (tone), { &attack, &release }, 96);
        dialArea = dial.withTrimmedTop (32).reduced (10, 8);
    }

    Bindings bindings()
    {
        return { { { &signal.slider, "raSignal" }, { &bandwidth.slider, "raBandwidth" }, { &noise.slider, "raNoise" },
                   { &drift.slider, "raDrift" }, { &fading.slider, "raFading" }, { &storm.slider, "raStorm" },
                   { &attack.slider, "raAttack" }, { &release.slider, "raRelease" } },
                 { { &mode, "raMode" } } };
    }

private:
    // Radioskalan: en rund, glödande skala med stationsnamn, och en nål som ställs in på tonen man spelar.
    void paintDial (juce::Graphics& g)
    {
        using namespace juce;
        if (dialArea.isEmpty()) return;
        const auto a = dialArea.toFloat();
        const float size = jmin (a.getWidth(), a.getHeight());
        auto face = a.withSizeKeepingCentre (size, size);
        drawBrassBezel (g, face, size * 0.5f, 5.0f);
        auto glass = face.reduced (6.0f);
        ColourGradient glow (Colour (0xffe9c27a), glass.getCentreX(), glass.getCentreY(), Colour (0xff6e4a1c), glass.getX(), glass.getY(), true);
        g.setGradientFill (glow);
        g.fillEllipse (glass);

        const float cx = glass.getCentreX(), cy = glass.getCentreY() + size * 0.08f, rad = size * 0.40f;
        const float a0 = degreesToRadians (-130.0f), a1 = degreesToRadians (130.0f);

        // Streck för varje oktav (C1–C7) och mellanstreck.
        g.setColour (Colour (0xff3a2510));
        for (int i = 0; i <= 24; ++i)
        {
            const float ang = a0 + (a1 - a0) * (float) i / 24.0f;
            const float len = (i % 4 == 0) ? 9.0f : 4.0f;
            g.drawLine (cx + std::sin (ang) * rad, cy - std::cos (ang) * rad,
                        cx + std::sin (ang) * (rad - len), cy - std::cos (ang) * (rad - len), i % 4 == 0 ? 1.4f : 0.8f);
        }
        // Stationerna.
        static const char* stations[] = { "MOTALA", "HÖRBY", "MOSKVA", "LUXEMBURG", "ÅNGVIK", "KALUNDBORG" };
        g.setFont (panelFont (jmax (7.0f, size * 0.038f)));
        for (int i = 0; i < 6; ++i)
        {
            const float ang = a0 + (a1 - a0) * ((float) i + 0.5f) / 6.0f;
            const float tr = rad - 22.0f;
            Graphics::ScopedSaveState s (g);
            g.addTransform (AffineTransform::rotation (ang, cx, cy));
            g.drawText (String::fromUTF8 (stations[i]), Rectangle<float> (cx - 40.0f, cy - tr - 6.0f, 80.0f, 12.0f), Justification::centred);
        }

        // Nålen: C1 (24) till C7 (96).
        const float t = tunedNote < 0 ? 0.5f : jlimit (0.0f, 1.0f, (float) (tunedNote - 24) / 72.0f);
        const float ang = a0 + (a1 - a0) * t;
        g.setColour (colours::meterRed);
        g.drawLine (cx, cy, cx + std::sin (ang) * (rad + 2.0f), cy - std::cos (ang) * (rad + 2.0f), 2.0f);
        drawRivet (g, { cx, cy }, 6.0f);

        // Storm: blixtrande ljus över glaset.
        const float st = (float) storm.slider.valueToProportionOfLength (storm.slider.getValue());
        if (st > 0.02f)
        {
            g.setColour (Colours::white.withAlpha (0.08f * st));
            g.fillEllipse (glass);
        }
        ColourGradient refl (Colours::white.withAlpha (0.12f), glass.getX(), glass.getY(), Colours::transparentWhite, glass.getX(), glass.getCentreY(), false);
        g.setGradientFill (refl);
        g.fillEllipse (glass.withHeight (glass.getHeight() * 0.5f).reduced (glass.getWidth() * 0.12f, 2.0f));
    }


    juce::ComboBox mode;
    KrangKnob signal { "Signal" }, bandwidth { "Bandbredd" }, noise { "Brus" }, drift { "Drift" }, fading { "Fading" },
              storm { "Storm" }, attack { "Insats" }, release { "Avklang" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> dialArea;
    int tunedNote = -1;
    textures::BackgroundCache background;
};

// --- Lurblåsaren ------------------------------------------------------------------

class LurblasarenPanel : public juce::Component
{
public:
    LurblasarenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &natural, &key, &breath, &tension, &rasp,
                                                                 &noise, &vibrato, &attack, &release })
            addAndMakeVisible (c);
        type.onChange = [this] { repaint (hornArea); };
        rasp.slider.onValueChange = [this] { repaint (hornArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintInstrument (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("INSTRUMENT", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("NATURTONER", natural.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("LURENS TON", key.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto inst = r.removeFromLeft (250);   r.removeFromLeft (12);
        auto mouth = r.removeFromLeft (310);  r.removeFromLeft (12);
        auto air = r.removeFromLeft (400);    r.removeFromLeft (12);
        auto vis = r;
        frames = { { inst, "Instrumentet" }, { mouth, "Munstycket" }, { air, "Andetaget" }, { vis, juce::String::fromUTF8 ("Lurstället") } };

        auto c = inst.withTrimmedTop (28).reduced (20, 0);
        c.removeFromTop (30); type.setBounds (c.removeFromTop (26));
        c.removeFromTop (34); natural.setBounds (c.removeFromTop (26));
        c.removeFromTop (34); key.setBounds (c.removeFromTop (26));

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        layoutRow (knobRow (mouth), { &breath, &tension, &rasp }, 96);
        layoutRow (knobRow (air), { &noise, &vibrato, &attack, &release }, 92);
        hornArea = vis.withTrimmedTop (34).reduced (12, 10);
    }

    Bindings bindings()
    {
        return { { { &breath.slider, "luBreath" }, { &tension.slider, "luTension" }, { &rasp.slider, "luRasp" },
                   { &noise.slider, "luNoise" }, { &vibrato.slider, "luVibrato" }, { &attack.slider, "luAttack" },
                   { &release.slider, "luRelease" } },
                 { { &type, "luType" }, { &natural, "luNatural" }, { &key, "luKey" } } };
    }

private:
    // Lurstället: det valda instrumentet, stående – lång lur, krökt horn, skalmeja med hål eller ångvissla.
    void paintInstrument (juce::Graphics& g)
    {
        using namespace juce;
        if (hornArea.isEmpty()) return;
        const auto a = hornArea.toFloat();
        const int t = jmax (0, type.getSelectedItemIndex());
        const float raspAmount = (float) rasp.slider.valueToProportionOfLength (rasp.slider.getValue());
        const float cx = a.getCentreX();
        Path p;
        Colour body = Colour (0xff9a6a3a);                               // näver
        if (t == 0) // näverlur: lång, smal och konisk
        {
            p.startNewSubPath (cx - 2.5f, a.getY());
            p.lineTo (cx + 2.5f, a.getY());
            p.lineTo (cx + 13.0f, a.getBottom());
            p.lineTo (cx - 13.0f, a.getBottom());
            p.closeSubPath();
        }
        else if (t == 1) // bockhorn: krökt horn
        {
            p.startNewSubPath (cx - 30.0f, a.getY() + 10.0f);
            p.quadraticTo (cx + 40.0f, a.getY() + a.getHeight() * 0.3f, cx + 10.0f, a.getBottom() - 10.0f);
            p.lineTo (cx - 14.0f, a.getBottom() - 18.0f);
            p.quadraticTo (cx + 18.0f, a.getY() + a.getHeight() * 0.35f, cx - 32.0f, a.getY() + 16.0f);
            p.closeSubPath();
            body = Colour (0xffd9c7a0);                                 // horn
        }
        else if (t == 2) // skalmeja: trärör med klocka och fingerhål
        {
            p.addRoundedRectangle (cx - 6.0f, a.getY(), 12.0f, a.getHeight() * 0.8f, 3.0f);
            p.startNewSubPath (cx - 6.0f, a.getY() + a.getHeight() * 0.78f);
            p.lineTo (cx + 6.0f, a.getY() + a.getHeight() * 0.78f);
            p.lineTo (cx + 20.0f, a.getBottom());
            p.lineTo (cx - 20.0f, a.getBottom());
            p.closeSubPath();
            body = Colour (0xff6b4423);
        }
        else // ångvissla: mässingsvissla på ett rör
        {
            p.addRoundedRectangle (cx - 16.0f, a.getY() + 10.0f, 32.0f, a.getHeight() * 0.45f, 6.0f);
            p.addRectangle (cx - 4.0f, a.getY() + 10.0f + a.getHeight() * 0.45f, 8.0f, a.getHeight() * 0.5f);
            body = colours::brass;
        }
        ColourGradient grad (body.brighter (0.35f), a.getX(), a.getY(), body.darker (0.4f), a.getRight(), a.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (p);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.strokePath (p, PathStrokeType (1.0f));

        if (t == 0) // näverns lindningar
        {
            g.setColour (Colour (0xff3a2614).withAlpha (0.7f));
            for (int i = 1; i < 8; ++i)
            {
                const float y = a.getY() + a.getHeight() * (float) i / 8.0f;
                const float w = 2.5f + 10.5f * (float) i / 8.0f;
                g.drawLine (cx - w, y, cx + w, y, 1.2f);
            }
        }
        if (t == 2) // fingerhål
        {
            g.setColour (Colours::black.withAlpha (0.8f));
            for (int i = 0; i < 6; ++i)
                g.fillEllipse (cx - 2.5f, a.getY() + 14.0f + (float) i * a.getHeight() * 0.1f, 5.0f, 5.0f);
        }
        if (t == 3) // ånga
        {
            g.setColour (Colours::white.withAlpha (0.18f));
            for (int i = 0; i < 4; ++i)
                g.fillEllipse (cx - 26.0f + (float) i * 16.0f, a.getY() - 4.0f - (float) (i % 2) * 6.0f, 18.0f, 14.0f);
        }
        if (raspAmount > 0.05f)
            textures::fill (g, p.getBounds(), textures::rust(), 0.3f * raspAmount, 0.0f);
    }

    juce::ComboBox type, natural, key;
    KrangKnob breath { u8 ("Blåstryck") }, tension { u8 ("Spänning") }, rasp { "Rasp" },
              noise { "Andning" }, vibrato { "Vibrato" }, attack { "Insats" }, release { "Avklang" };
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> hornArea;
    textures::BackgroundCache background;
};

// --- Lutverket --------------------------------------------------------------------

class LutverketPanel : public juce::Component
{
public:
    LutverketPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &pos, &hard, &decay, &bright, &course, &damper, &buzz, &body })
            addAndMakeVisible (c);
        type.onChange = [this] { repaint (bodyArea); };
        pos.slider.onValueChange = [this] { repaint (bodyArea); };
        buzz.slider.onValueChange = [this] { repaint (bodyArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintInstrument (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("INSTRUMENT", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto pluckF = r.removeFromLeft (300);  r.removeFromLeft (12);
        auto strF = r.removeFromLeft (400);    r.removeFromLeft (12);
        auto boxF = r.removeFromLeft (210);    r.removeFromLeft (12);
        auto vis = r;
        frames = { { pluckF, juce::String::fromUTF8 ("Knäppet") }, { strF, juce::String::fromUTF8 ("Strängarna") },
                   { boxF, juce::String::fromUTF8 ("Lådan") }, { vis, "Instrumentet" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        type.setBounds (pluckF.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (pluckF), { &pos, &hard }, 100);
        layoutRow (knobRow (strF), { &decay, &bright, &course, &damper }, 92);
        layoutRow (knobRow (boxF), { &buzz, &body }, 96);
        bodyArea = vis.withTrimmedTop (34).reduced (12, 10);
    }

    Bindings bindings()
    {
        return { { { &pos.slider, "ltPos" }, { &hard.slider, "ltHard" }, { &decay.slider, "ltDecay" }, { &bright.slider, "ltBright" },
                   { &course.slider, "ltCourse" }, { &damper.slider, "ltDamper" }, { &buzz.slider, "ltBuzz" }, { &body.slider, "ltBody" } },
                 { { &type, "ltType" } } };
    }

private:
    // Instrumentet ovanifrån: kantele, luta, cittra eller rostharpa, med strängarna och en markering där man knäpper.
    void paintInstrument (juce::Graphics& g)
    {
        using namespace juce;
        if (bodyArea.isEmpty()) return;
        const auto a = bodyArea.toFloat();
        const int t = jmax (0, type.getSelectedItemIndex());
        const float rustAmount = (float) buzz.slider.valueToProportionOfLength (buzz.slider.getValue());
        const float pluck = (float) pos.slider.valueToProportionOfLength (pos.slider.getValue());
        const float cx = a.getCentreX();

        Path shape;
        Colour wood = Colour (0xff8a5a2b);
        float top = a.getY() + 6.0f, bottom = a.getBottom() - 6.0f;
        int strings = 5;
        if (t == 0)      // kantele: trapets
        {
            shape.startNewSubPath (cx - 22.0f, top); shape.lineTo (cx + 22.0f, top);
            shape.lineTo (cx + 46.0f, bottom); shape.lineTo (cx - 46.0f, bottom); shape.closeSubPath();
            strings = 5;
        }
        else if (t == 1) // luta: päronformad kropp och hals
        {
            shape.addRoundedRectangle (cx - 7.0f, top, 14.0f, (bottom - top) * 0.45f, 3.0f);
            shape.addEllipse (cx - 44.0f, top + (bottom - top) * 0.40f, 88.0f, (bottom - top) * 0.60f);
            wood = Colour (0xffa0702f);
            strings = 6;
        }
        else if (t == 2) // cittra: platt låda
        {
            shape.addRoundedRectangle (cx - 48.0f, top + 10.0f, 96.0f, bottom - top - 20.0f, 8.0f);
            wood = Colour (0xff6e4424);
            strings = 9;
        }
        else             // rostharpa: en järnram
        {
            shape.addRoundedRectangle (cx - 40.0f, top, 80.0f, bottom - top, 4.0f);
            wood = Colour (0xff5a4a40);
            strings = 6;
        }
        ColourGradient grad (wood.brighter (0.35f), a.getX(), a.getY(), wood.darker (0.45f), a.getRight(), a.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (shape);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.strokePath (shape, PathStrokeType (1.0f));
        if (t == 1) // rosett
        {
            g.setColour (Colours::black.withAlpha (0.6f));
            g.drawEllipse (cx - 12.0f, top + (bottom - top) * 0.62f, 24.0f, 24.0f, 1.2f);
        }

        // Strängarna, och var man knäpper.
        const auto b = shape.getBounds();
        const float sTop = b.getY() + 4.0f, sBottom = b.getBottom() - 6.0f;
        const float width = (t == 1 ? 12.0f : b.getWidth() * 0.6f);
        for (int i = 0; i < strings; ++i)
        {
            const float x = cx - width * 0.5f + width * ((float) i + 0.5f) / (float) strings;
            const auto metal = (t == 1 ? Colour (0xffe8dcc4) : Colour (0xffc9ccd0)).interpolatedWith (Colour (0xff8a4a22), rustAmount);
            g.setColour (metal.withAlpha (0.9f));
            g.drawLine (x, sTop, x, sBottom, t == 1 ? 0.8f : 1.0f);
        }
        const float py = sBottom - (sBottom - sTop) * (0.05f + 0.9f * pluck) * 0.5f;
        g.setColour (colours::tubeOrange);
        g.drawLine (cx - width * 0.5f - 6.0f, py, cx + width * 0.5f + 6.0f, py, 2.0f);
        if (rustAmount > 0.05f)
            textures::fill (g, b, textures::rust(), 0.35f * rustAmount, 0.0f);
    }

    juce::ComboBox type;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    KrangKnob pos { u8 ("Knäpp") }, hard { u8 ("Hårdhet") }, decay { "Klingtid" }, bright { "Glans" }, course { u8 ("Kör") },
              damper { u8 ("Dämpare") }, buzz { "Surr" }, body { "Botten" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> bodyArea;
    textures::BackgroundCache background;
};

// --- Vattenpumpen ----------------------------------------------------------------

class VattenpumpenPanel : public juce::Component
{
public:
    VattenpumpenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &pressure, &speed, &water, &vibrato, &glide, &rust, &attack, &release })
            addAndMakeVisible (c);
        type.onChange = [this] { repaint (visArea); };
        water.slider.onValueChange = [this] { repaint (visArea); };
        rust.slider.onValueChange = [this] { repaint (visArea); };
        pressure.slider.onValueChange = [this] { repaint (visArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintInstrument (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("METALL", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto bowF = r.removeFromLeft (300);   r.removeFromLeft (12);
        auto metF = r.removeFromLeft (400);   r.removeFromLeft (12);
        auto toneF = r.removeFromLeft (210);  r.removeFromLeft (12);
        auto vis = r;
        frames = { { bowF, juce::String::fromUTF8 ("Stråken") }, { metF, "Metallen" }, { toneF, "Tonen" }, { vis, "Pumphuset" } };

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        type.setBounds (bowF.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        layoutRow (knobRow (bowF), { &pressure, &speed }, 100);
        layoutRow (knobRow (metF), { &water, &vibrato, &glide, &rust }, 92);
        layoutRow (knobRow (toneF), { &attack, &release }, 96);
        visArea = vis.withTrimmedTop (34).reduced (12, 10);
    }

    Bindings bindings()
    {
        return { { { &pressure.slider, "vpPressure" }, { &speed.slider, "vpSpeed" }, { &water.slider, "vpWater" },
                   { &vibrato.slider, "vpVibrato" }, { &glide.slider, "vpGlide" }, { &rust.slider, "vpRust" },
                   { &attack.slider, "vpAttack" }, { &release.slider, "vpRelease" } },
                 { { &type, "vpType" } } };
    }

private:
    // Pumphuset: vattenfonen med sina stavar, sågen, plåten eller cymbalen – och stråken som drar över metallen.
    void paintInstrument (juce::Graphics& g)
    {
        using namespace juce;
        if (visArea.isEmpty()) return;
        const auto a = visArea.toFloat();
        const int t = jmax (0, type.getSelectedItemIndex());
        const float waterAmt = (float) water.slider.valueToProportionOfLength (water.slider.getValue());
        const float rustAmt = (float) rust.slider.valueToProportionOfLength (rust.slider.getValue());
        const float press = (float) pressure.slider.valueToProportionOfLength (pressure.slider.getValue());
        const float cx = a.getCentreX(), cy = a.getCentreY();
        const auto steel = Colour (0xffb9bdc2).interpolatedWith (Colour (0xff8a4a22), rustAmt * 0.7f);
        Rectangle<float> bounds;

        if (t == 0) // vattenfon: rund behållare med stavar runt kanten
        {
            const float rx = jmin (a.getWidth(), a.getHeight()) * 0.34f;
            for (int i = 0; i < 9; ++i)
            {
                const float ang = MathConstants<float>::pi * (0.15f + 0.7f * (float) i / 8.0f);
                const float len = rx * (0.7f + 0.5f * (float) ((i * 5) % 4) / 3.0f);
                const float x = cx - std::cos (ang) * rx;
                g.setColour (steel);
                g.drawLine (x, cy, x - std::cos (ang) * len * 0.3f, cy - len, 2.0f);
            }
            auto tank = Rectangle<float> (cx - rx, cy - rx * 0.25f, rx * 2.0f, rx * 1.1f);
            g.setColour (steel.darker (0.2f));
            g.fillEllipse (tank);
            // Vattnet i behållaren, som skvalpar mer med Vatten.
            Graphics::ScopedSaveState clip (g);
            Path tankShape;
            tankShape.addEllipse (tank.reduced (3.0f));
            g.reduceClipRegion (tankShape);
            g.setColour (Colour (0xff3a6e8a).withAlpha (0.8f));
            Path w;
            const float wy = tank.getCentreY() + tank.getHeight() * 0.05f;
            w.startNewSubPath (tank.getX() + 6.0f, wy);
            for (float x = tank.getX() + 6.0f; x <= tank.getRight() - 6.0f; x += 4.0f)
                w.lineTo (x, wy + std::sin (x * 0.15f) * 3.0f * waterAmt);
            w.lineTo (tank.getRight() - 10.0f, tank.getBottom() - 8.0f);
            w.lineTo (tank.getX() + 10.0f, tank.getBottom() - 8.0f);
            w.closeSubPath();
            g.fillPath (w);
            bounds = tank.withTop (cy - rx); // klippningen släpps när blocket tar slut
        }
        else if (t == 1) // såg: ett krökt blad med handtag
        {
            Path blade;
            blade.startNewSubPath (cx - a.getWidth() * 0.35f, cy + 30.0f);
            blade.quadraticTo (cx, cy - 50.0f, cx + a.getWidth() * 0.35f, cy - 10.0f);
            blade.lineTo (cx + a.getWidth() * 0.35f, cy + 6.0f);
            blade.quadraticTo (cx, cy - 26.0f, cx - a.getWidth() * 0.35f, cy + 44.0f);
            blade.closeSubPath();
            g.setColour (steel);
            g.fillPath (blade);
            g.setColour (Colour (0xff6b4423));
            g.fillRoundedRectangle (cx - a.getWidth() * 0.45f, cy + 26.0f, 26.0f, 22.0f, 5.0f);
            bounds = blade.getBounds();
        }
        else if (t == 2) // plåt i två kedjor
        {
            bounds = Rectangle<float> (cx - a.getWidth() * 0.35f, a.getY() + 24.0f, a.getWidth() * 0.7f, a.getHeight() * 0.7f);
            g.setColour (colours::brassDark);
            g.drawLine (bounds.getX() + 10.0f, a.getY(), bounds.getX() + 10.0f, bounds.getY(), 1.5f);
            g.drawLine (bounds.getRight() - 10.0f, a.getY(), bounds.getRight() - 10.0f, bounds.getY(), 1.5f);
            ColourGradient grad (steel.brighter (0.2f), bounds.getX(), bounds.getY(), steel.darker (0.4f), bounds.getRight(), bounds.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRect (bounds);
        }
        else // cymbal
        {
            bounds = Rectangle<float> (cx - a.getWidth() * 0.38f, cy - 18.0f, a.getWidth() * 0.76f, 36.0f);
            ColourGradient grad (colours::brassLight, bounds.getX(), bounds.getY(), colours::brassDark, bounds.getRight(), bounds.getBottom(), false);
            g.setGradientFill (grad);
            g.fillEllipse (bounds);
            g.setColour (colours::brassDark);
            g.fillEllipse (bounds.withSizeKeepingCentre (14.0f, 7.0f));
        }
        if (rustAmt > 0.05f)
            textures::fill (g, bounds, textures::rust(), 0.35f * rustAmt, 0.0f);

        // Stråken: trä och tagel, mot metallens kant; mer tryck ger en tydligare böjd stråke.
        const float bx = bounds.getRight() - 8.0f, by = bounds.getY() + 6.0f;
        g.setColour (Colour (0xff4a2c14));
        g.drawLine (bx - 50.0f, by - 30.0f, bx + 20.0f, by + 40.0f, 3.0f);
        g.setColour (Colour (0xffeee6d0).withAlpha (0.9f));
        g.drawLine (bx - 46.0f + 4.0f * press, by - 26.0f, bx + 22.0f + 4.0f * press, by + 34.0f, 1.2f);
    }

    juce::ComboBox type;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    KrangKnob pressure { "Tryck" }, speed { "Fart" }, water { "Vatten" }, vibrato { "Vibrato" }, glide { "Glid" },
              rust { "Skrot" }, attack { "Insats" }, release { "Avklang" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> visArea;
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
