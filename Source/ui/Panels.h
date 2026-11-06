// Krang-panelen (fas 2): modulerna under frontpanelen.
//   BasmalenPanel      – oscillatorer, rörelse, filter, två envelopes (faders)
//   HammarverketPanel  – Stegverket + en rad röstrattar per trumspår, Rost
//   AngledningenPanel  – Krangdistortion -> Ångfilter -> Bandmaskinen -> Ekokammaren -> Katedralreverb,
//                        sammankopplade med kopparrör
//   KaosPanel          – kaosavdelningen: Kaosmotorn (med kaoslampa), Megafonen, Gramofonen och Krangskopet (oscilloskop på utgången)
// Panelerna äger bara kontrollerna; editorn kopplar dem till parametrarna via bindings().
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "KrangStyle.h"
#include "Jugend.h"
#include "StegverketPanel.h"
#include "../dsp/Hammarverket.h"
#include "../dsp/Automaten.h"

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
        filterDrive.slider.onValueChange = [this] { repaint (millArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        // Kvarnen som mal basen: mer mjöl ju hårdare filtret drivs.
        if (! millArea.isEmpty())
            jugend::mill (g, millArea.toFloat(), (float) filterDrive.slider.valueToProportionOfLength (filterDrive.slider.getValue()));
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
        auto mill = r.removeFromRight (150);
        r.removeFromRight (12);
        addFrame (mill, "Kvarnen");
        millArea = mill.withTrimmedTop (30).reduced (8, 6);
        auto right = r.removeFromRight (280);
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
    juce::Rectangle<int> millArea;

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
        timbre.slider.onValueChange = [this] { repaint (instrumentArea); };
        drone.slider.onValueChange = [this] { repaint (instrumentArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintStrings (g);
        // Instrumentet som klangfärgen härmar: vallflöjt, pipa, säckpipa, pipa och harpa eller nyckelharpa.
        if (! instrumentArea.isEmpty())
        {
            const float t = (float) timbre.slider.getValue();
            jugend::bordunInstrument (g, instrumentArea.toFloat(), juce::jlimit (0, 4, (int) (t * 5.0f)),
                                      (float) drone.slider.valueToProportionOfLength (drone.slider.getValue()));
        }
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
        auto klang = r.removeFromLeft (330);   r.removeFromLeft (12);
        auto bordun = r.removeFromLeft (170);  r.removeFromLeft (12);
        auto vis = r.removeFromRight (170);    r.removeFromRight (12);
        auto env = r.removeFromRight (150);    r.removeFromRight (12);
        auto res = r;
        frames = { { klang, juce::String::fromUTF8 ("Klang") }, { bordun, "Bordun" },
                   { res, juce::String::fromUTF8 ("Resonanssträngar") }, { env, "Envelope" }, { vis, "Spelmannen" } };
        instrumentArea = vis.withTrimmedTop (30).reduced (8, 6);

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28).reduced (6, 4).withSizeKeepingCentre (f.getWidth() - 12, 140); };
        layoutRow (knobRow (klang), { &timbre, &bow, &drift }, 100);

        droneNote.setBounds (bordun.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (90, 26));
        drone.setBounds (bordun.withTrimmedTop (28 + 60).withHeight (150).withSizeKeepingCentre (110, 150));

        auto rr = res.withTrimmedTop (28).reduced (8, 8);
        symp.setBounds (rr.removeFromLeft (96).withSizeKeepingCentre (96, 140));
        stringsArea = rr.reduced (4, 20);

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
    juce::Rectangle<int> stringsArea, instrumentArea;
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
        stringsArea = vis.withTrimmedTop (30).reduced (8, 6);
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
        // Strängbädden på en jugendplakett: en klangbotten i trä under strängarna.
        const auto a = jugend::plaqueFor (g, stringsArea.toFloat()).reduced (10.0f, 8.0f);
        {
            Path board;
            board.addRoundedRectangle (a.expanded (6.0f, 4.0f), 8.0f);
            jugend::fillInk (g, board, jugend::pal::wood.withAlpha (0.92f));
        }
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
        bellArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &hard.slider, "klHard" }, { &hum.slider, "klHum" }, { &beat.slider, "klBeat" }, { &crack.slider, "klCrack" },
                   { &sweep.slider, "klSweep" }, { &decay.slider, "klDecay" }, { &release.slider, "klRelease" } },
                 { { &type, "klType" } } };
    }

private:
    // Klockstapeln: den valda klockan på en jugendplakett, med sprickan inritad.
    void paintBell (juce::Graphics& g)
    {
        if (bellArea.isEmpty()) return;
        jugend::bell (g, bellArea.toFloat(), juce::jmax (0, type.getSelectedItemIndex()),
                      (float) crack.slider.valueToProportionOfLength (crack.slider.getValue()));
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

// En liten tvålägesspak (vippströmbrytare) med ett läge upptill och ett nedtill. Den bär en osynlig
// ComboBox så att den binds till en valparameter med två val precis som listorna.
class KrangToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    KrangToggle (const juce::String& titleText, const juce::String& upText, const juce::String& downText)
        : title (titleText), up (upText), down (downText)
    {
        choice.addItemList ({ up, down }, 1);
        choice.setSelectedItemIndex (1, juce::dontSendNotification);
        choice.onChange = [this] { repaint(); };
        addChildComponent (choice);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    bool isDown() const { return choice.getSelectedItemIndex() == 1; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto r = getLocalBounds().toFloat();
        int target = isDown() ? 0 : 1;
        if (e.position.y < r.getY() + r.getHeight() * 0.42f) target = 0;        // klick på övre läget
        else if (e.position.y > r.getY() + r.getHeight() * 0.62f) target = 1;   // klick på nedre läget
        choice.setSelectedItemIndex (target, juce::sendNotificationSync);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setFont (panelFont (9.5f));
        g.setColour (colours::engraving);
        g.drawText (title, r.removeFromTop (14.0f), juce::Justification::centred);
        const auto upRow = r.removeFromTop (13.0f), downRow = r.removeFromBottom (13.0f);
        g.setColour (isDown() ? colours::engraving.withAlpha (0.45f) : colours::tubeOrange);
        g.drawText (up, upRow, juce::Justification::centred);
        g.setColour (isDown() ? colours::tubeOrange : colours::engraving.withAlpha (0.45f));
        g.drawText (down, downRow, juce::Justification::centred);

        // Brickan med sin mässingsring, och spaken som pekar mot valt läge.
        const auto c = r.getCentre();
        const float plate = juce::jmin (r.getWidth(), r.getHeight()) * 0.34f;
        juce::ColourGradient ring (colours::brassLight, c.x - plate, c.y - plate, colours::brassDark, c.x + plate, c.y + plate, false);
        g.setGradientFill (ring);
        g.fillEllipse (juce::Rectangle<float> (plate * 2.0f, plate * 2.0f).withCentre (c));
        g.setColour (colours::bakelite);
        g.fillEllipse (juce::Rectangle<float> (plate * 1.3f, plate * 1.3f).withCentre (c));
        const float dir = isDown() ? 1.0f : -1.0f;
        const juce::Point<float> tip { c.x, c.y + dir * (r.getHeight() * 0.5f - 2.0f) };
        g.setColour (colours::brassDark);
        g.drawLine ({ c, tip }, 5.0f);
        g.setColour (colours::brassLight);
        g.drawLine ({ c.translated (-0.8f, 0.0f), tip.translated (-0.8f, 0.0f) }, 2.0f);
        juce::ColourGradient knob (colours::brassLight, tip.x - 3.0f, tip.y - 3.0f, colours::brassDark, tip.x + 4.0f, tip.y + 4.0f, false);
        g.setGradientFill (knob);
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (tip));
    }

    juce::ComboBox choice;

private:
    juce::String title, up, down;
};

class KorenPanel : public juce::Component
{
public:
    KorenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &voices, &singers, &spread, &vowel, &vibrato, &breath,
                                                                 &attack, &release, &speaker, &speakerGate })
            addAndMakeVisible (c);
        speakerGate.setTooltip (juce::String::fromUTF8 ("Spelar: högtalaren sprakar hela tiden (Alltid) eller bara när kören sjunger (Vid ljud)."));
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
        auto hornIn = horn.withTrimmedTop (34).reduced (10, 10).withTrimmedLeft (106);
        speakerGate.setBounds (hornIn.removeFromRight (60).withY (h.getY() + 4).withHeight (84));
        hornArea = hornIn;
    }

    Bindings bindings()
    {
        return { { { &singers.slider, "koSingers" }, { &spread.slider, "koSpread" }, { &vowel.slider, "koVowel" },
                   { &vibrato.slider, "koVibrato" }, { &breath.slider, "koBreath" }, { &attack.slider, "koAttack" },
                   { &release.slider, "koRelease" }, { &speaker.slider, "koSpeaker" } },
                 { { &voices, "koVoices" }, { &speakerGate.choice, "koSpeakerGate" } } };
    }

private:
    // Högtalaren: en tratt i mässing, vokalen i grönt glas och en lampa för varje sångare.
    void paintHorn (juce::Graphics& g)
    {
        using namespace juce;
        if (hornArea.isEmpty()) return;
        auto a = jugend::plaqueFor (g, hornArea.toFloat(), 0.55f).reduced (4.0f, 2.0f);   // på en jugendplakett
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
    KrangToggle speakerGate { "SPELAR", "ALLTID", "VID LJUD" };
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
        hornArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &breath.slider, "luBreath" }, { &tension.slider, "luTension" }, { &rasp.slider, "luRasp" },
                   { &noise.slider, "luNoise" }, { &vibrato.slider, "luVibrato" }, { &attack.slider, "luAttack" },
                   { &release.slider, "luRelease" } },
                 { { &type, "luType" }, { &natural, "luNatural" }, { &key, "luKey" } } };
    }

private:
    // Lurstället: det valda instrumentet på en jugendplakett (se Jugend.h).
    void paintInstrument (juce::Graphics& g)
    {
        if (hornArea.isEmpty()) return;
        jugend::lur (g, hornArea.toFloat(), juce::jmax (0, type.getSelectedItemIndex()),
                     (float) rasp.slider.valueToProportionOfLength (rasp.slider.getValue()));
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
        bodyArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &pos.slider, "ltPos" }, { &hard.slider, "ltHard" }, { &decay.slider, "ltDecay" }, { &bright.slider, "ltBright" },
                   { &course.slider, "ltCourse" }, { &damper.slider, "ltDamper" }, { &buzz.slider, "ltBuzz" }, { &body.slider, "ltBody" } },
                 { { &type, "ltType" } } };
    }

private:
    // Instrumentet på en jugendplakett: kantele, luta, cittra eller rostharpa, med strängarna och var man knäpper.
    void paintInstrument (juce::Graphics& g)
    {
        if (bodyArea.isEmpty()) return;
        jugend::lutInstrument (g, bodyArea.toFloat(), juce::jmax (0, type.getSelectedItemIndex()),
                               (float) buzz.slider.valueToProportionOfLength (buzz.slider.getValue()),
                               (float) pos.slider.valueToProportionOfLength (pos.slider.getValue()));
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
        visArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &pressure.slider, "vpPressure" }, { &speed.slider, "vpSpeed" }, { &water.slider, "vpWater" },
                   { &vibrato.slider, "vpVibrato" }, { &glide.slider, "vpGlide" }, { &rust.slider, "vpRust" },
                   { &attack.slider, "vpAttack" }, { &release.slider, "vpRelease" } },
                 { { &type, "vpType" } } };
    }

private:
    // Pumphuset: vattenfonen, sågen, plåten eller cymbalen på en jugendplakett – och stråken som drar över metallen.
    void paintInstrument (juce::Graphics& g)
    {
        if (visArea.isEmpty()) return;
        auto prop = [] (KrangKnob& k) { return (float) k.slider.valueToProportionOfLength (k.slider.getValue()); };
        jugend::pump (g, visArea.toFloat(), juce::jmax (0, type.getSelectedItemIndex()), prop (water), prop (rust), prop (pressure));
    }

    juce::ComboBox type;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    KrangKnob pressure { "Tryck" }, speed { "Fart" }, water { "Vatten" }, vibrato { "Vibrato" }, glide { "Glid" },
              rust { "Skrot" }, attack { "Insats" }, release { "Avklang" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> visArea;
    textures::BackgroundCache background;
};

// --- Ångorgeln ------------------------------------------------------------------

class AngorgelnPanel : public juce::Component
{
public:
    AngorgelnPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &pressure, &chiff, &leak, &trem, &attack, &release })
            addAndMakeVisible (c);
        for (auto& f : ranks) addAndMakeVisible (f);
        type.onChange = [this] { repaint (visArea); };
        for (auto& f : ranks) f.slider.onValueChange = [this] { repaint (visArea); };
        for (auto* k : { &pressure, &leak, &trem }) k->slider.onValueChange = [this] { repaint (visArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintPipes (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("ORGEL", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto regF = r.removeFromLeft (330);  r.removeFromLeft (12);
        auto steamF = r.removeFromLeft (370); r.removeFromLeft (12);
        auto envF = r.removeFromLeft (136);  r.removeFromLeft (12);
        auto vis = r;
        frames = { { regF, "Registren" }, { steamF, u8 ("Ångan") }, { envF, "Envelope" }, { vis, "Pipverket" } };

        type.setBounds (regF.withTrimmedTop (28 + 16).reduced (14, 0).removeFromTop (24).removeFromLeft (150));
        layoutRow (regF.withTrimmedTop (28 + 50).reduced (8, 8), { &ranks[0], &ranks[1], &ranks[2], &ranks[3], &ranks[4] }, 50);
        layoutRow (steamF.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0), { &pressure, &chiff, &leak, &trem }, 90);
        layoutRow (envF.withTrimmedTop (28).reduced (4, 8), { &attack, &release }, 60);
        visArea = vis.withTrimmedTop (32).reduced (10, 8);
    }

    Bindings bindings()
    {
        return { { { &ranks[0].slider, "agR16" }, { &ranks[1].slider, "agR8" }, { &ranks[2].slider, "agR4" },
                   { &ranks[3].slider, "agR223" }, { &ranks[4].slider, "agR2" }, { &pressure.slider, "agPressure" },
                   { &chiff.slider, "agChiff" }, { &leak.slider, "agLeak" }, { &trem.slider, "agTrem" },
                   { &attack.slider, "agAttack" }, { &release.slider, "agRelease" } },
                 { { &type, "agType" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    float prop (KrangKnob& k) const { return (float) k.slider.valueToProportionOfLength (k.slider.getValue()); }

    // Pipverket: fasaden med fem rader pipor (16' längst, 2' kortast) som lyser när registret är draget,
    // en manometer för ångtrycket och ånga som pyser ur läckorna.
    void paintPipes (juce::Graphics& g)
    {
        using namespace juce;
        if (visArea.isEmpty()) return;
        const auto a = jugend::plaqueFor (g, visArea.toFloat(), 0.45f).reduced (6.0f, 4.0f);   // på en jugendplakett
        const int t = jmax (0, type.getSelectedItemIndex());
        const auto metal = t == 1 ? colours::brassLight : t == 2 ? Colour (0xff8a6a4a) : t == 3 ? Colour (0xffa08060) : Colour (0xffc8ccd0);
        auto pipesArea = a.withTrimmedRight (70.0f);
        const int perRank = 4;
        const float w = pipesArea.getWidth() / (float) (5 * perRank);
        for (int r = 0; r < 5; ++r)
        {
            const float level = (float) ranks[(size_t) r].slider.valueToProportionOfLength (ranks[(size_t) r].slider.getValue());
            for (int k = 0; k < perRank; ++k)
            {
                const int idx = r * perRank + k;
                const float h = pipesArea.getHeight() * (0.95f - 0.13f * (float) r - 0.03f * (float) k);
                auto pipe = Rectangle<float> (pipesArea.getX() + idx * w + 2.0f, pipesArea.getBottom() - h, w - 4.0f, h);
                ColourGradient grad (metal.withMultipliedBrightness (0.45f + 0.55f * level), pipe.getX(), 0,
                                     metal.withMultipliedBrightness (0.25f + 0.35f * level), pipe.getRight(), 0, false);
                grad.addColour (0.35, metal.brighter (0.3f).withMultipliedBrightness (0.5f + 0.5f * level));
                g.setGradientFill (grad);
                g.fillRect (pipe);
                // Munnen (labiet) nära foten.
                g.setColour (colours::bakelite.withAlpha (0.8f));
                g.fillRect (pipe.withY (pipe.getBottom() - h * 0.22f).withHeight (3.0f).reduced (pipe.getWidth() * 0.2f, 0));
                // Ånga ur läckorna.
                if (prop (leak) > 0.05f && (idx * 7) % 5 == 0)
                {
                    g.setColour (Colours::white.withAlpha (0.12f + 0.25f * prop (leak)));
                    for (int puff = 0; puff < 3; ++puff)
                        g.fillEllipse (Rectangle<float> (8.0f + puff * 5.0f, 6.0f + puff * 3.0f)
                                           .withCentre ({ pipe.getCentreX() + puff * 3.0f, pipe.getY() - 6.0f - puff * 7.0f }));
                }
            }
        }
        // Manometern.
        auto gauge = Rectangle<float> (58.0f, 58.0f).withCentre ({ a.getRight() - 34.0f, a.getY() + 40.0f });
        drawBrassBezel (g, gauge, 29.0f, 4.0f);
        g.setColour (colours::meterFace);
        g.fillEllipse (gauge.reduced (5.0f));
        const float p = prop (pressure);
        const float ang = MathConstants<float>::pi * (-0.75f + 1.5f * p);
        const auto c = gauge.getCentre();
        g.setColour (colours::meterRed.withAlpha (0.7f));
        g.drawLine (Line<float> (c.getPointOnCircumference (19.0f, MathConstants<float>::pi * -0.75f),
                                 c.getPointOnCircumference (19.0f, MathConstants<float>::pi * -0.45f)), 3.0f);
        g.setColour (colours::bakelite);
        g.drawLine (Line<float> (c, c.getPointOnCircumference (20.0f, ang)), 1.8f);
        g.setFont (panelFont (8.0f));
        g.drawText ("ATM", gauge.withTrimmedTop (32.0f), Justification::centredTop);
    }

    juce::ComboBox type;
    std::array<KrangFader, 5> ranks { KrangFader ("16'"), KrangFader ("8'"), KrangFader ("4'"),
                                      KrangFader (juce::String::fromUTF8 ("2⅔'")), KrangFader ("2'") };
    KrangKnob pressure { u8 ("Ångtryck") }, chiff { u8 ("Pipblås") }, leak { u8 ("Läckage") }, trem { "Tremulant" };
    KrangFader attack { "ATTACK" }, release { "RELEASE" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> visArea;
    textures::BackgroundCache background;
};

// --- Speldosan ------------------------------------------------------------------

class SpeldosanPanel : public juce::Component
{
public:
    SpeldosanPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &type, &hard, &decay, &box, &spring, &rust, &works })
            addAndMakeVisible (c);
        type.onChange = [this] { repaint (visArea); };
        for (auto* k : { &rust, &spring, &box }) k->slider.onValueChange = [this] { repaint (visArea); };
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintBox (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("DOSA", type.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto combF = r.removeFromLeft (300);  r.removeFromLeft (12);
        auto mechF = r.removeFromLeft (400);  r.removeFromLeft (12);
        auto vis = r;
        frames = { { combF, "Kammen" }, { mechF, "Mekaniken" }, { vis, "Dosan" } };
        type.setBounds (combF.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (170, 26));
        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        layoutRow (knobRow (combF), { &hard, &decay }, 100);
        layoutRow (knobRow (mechF), { &box, &spring, &rust, &works }, 92);
        visArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &hard.slider, "sdHard" }, { &decay.slider, "sdDecay" }, { &box.slider, "sdBox" }, { &spring.slider, "sdSpring" },
                   { &rust.slider, "sdRust" }, { &works.slider, "sdWorks" } },
                 { { &type, "sdType" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    float prop (KrangKnob& k) const { return (float) k.slider.valueToProportionOfLength (k.slider.getValue()); }

    // Dosan: en öppen speldosa på en jugendplakett – locket med intarsia, valsen, kammen och nyckeln (se Jugend.h).
    void paintBox (juce::Graphics& g)
    {
        if (visArea.isEmpty()) return;
        jugend::musicBox (g, visArea.toFloat(), juce::jmax (0, type.getSelectedItemIndex()), prop (rust), prop (spring));
    }

    juce::ComboBox type;
    KrangKnob hard { "Stift" }, decay { "Klingtid" }, box { u8 ("Låda") }, spring { u8 ("Fjäder") }, rust { "Rost" }, works { "Verk" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> visArea;
    textures::BackgroundCache background;
};

// --- Automaten ------------------------------------------------------------------

class AutomatenPanel : public juce::Component, private juce::Timer
{
public:
    AutomatenPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &figure, &word, &bird, &clock, &repeat, &voice, &monotone, &tempo, &broken, &display })
            addAndMakeVisible (c);
        figure.setTooltip (u8 ("Doktor Krangs leksaker: talmaskinen, den mekaniska sångfågeln eller urverket som tickar och sjunger."));
        word.setTooltip (u8 ("Ordet som sägs. Tangenterna: C säger det första ordet, C# det andra … så att du kan spela meningar."));
        bird.setTooltip (u8 ("Fågeln: kanariefågelns rullande drillar, näktergalens omväxling, lärkans kvitter eller trastens upprepade motiv."));
        clock.setTooltip (u8 ("Uret: fickurets repeterverk, pendelurets slag, gökuret eller klockspelsuret. Håll tangenten: uret dras upp och går; släpp: det saktar in och stannar."));
        figure.onChange = [this] { updateFigure(); };
        broken.slider.onValueChange = [this] { repaint (visArea); };
        updateFigure();
        startTimerHz (20);
    }

    // Vad Automaten gör just nu, från processorn: ett ord (0–11), en leksak (100 + figur) eller −1 = tyst.
    void setSpokenWord (int w)
    {
        if (w == spoken) return;
        spoken = w;
        updateDisplay();
        repaint (visArea);
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        const int f = figureIndex();
        if (f == 1)      paintBird (g);
        else if (f == 2) paintClock (g);
        else             paintHead (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("FIGUR", figure.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        const int f = figureIndex();
        g.drawText (f == 1 ? u8 ("FÅGEL") : f == 2 ? juce::String ("UR") : juce::String ("ORD"),
                    word.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
        if (f == 0)
            g.drawText ("UPPREPA", repeat.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto talF = r.removeFromLeft (300);   r.removeFromLeft (12);
        auto voiceF = r.removeFromLeft (400); r.removeFromLeft (12);
        auto vis = r;
        frames = { { talF, "Figuren" }, { voiceF, "Mekaniken" }, { vis, "Automaten" } };
        auto in = talF.withTrimmedTop (28 + 18).reduced (16, 0);
        auto row1 = in.removeFromTop (26);
        figure.setBounds (row1.removeFromLeft (140));
        row1.removeFromLeft (10);
        repeat.setBounds (row1);
        in.removeFromTop (20);
        const auto row2 = in.removeFromTop (26);
        word.setBounds (row2);
        bird.setBounds (row2);
        clock.setBounds (row2);
        in.removeFromTop (14);
        display.setBounds (in.removeFromTop (80));
        layoutRow (voiceF.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0), { &voice, &monotone, &tempo, &broken }, 92);
        visArea = vis.withTrimmedTop (32).reduced (12, 8);
    }

    Bindings bindings()
    {
        return { { { &voice.slider, "atVoice" }, { &monotone.slider, "atMonotone" }, { &tempo.slider, "atTempo" }, { &broken.slider, "atBroken" } },
                 { { &word, "atWord" }, { &repeat, "atRepeat" }, { &figure, "atFigure" }, { &bird, "atBird" }, { &clock, "atClock" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    int figureIndex() const { return juce::jmax (0, figure.getSelectedItemIndex()); }

    // Rattarnas betydelse och vilka listor som syns beror på figuren.
    void updateFigure()
    {
        const int f = figureIndex();
        word.setVisible (f == 0);
        repeat.setVisible (f == 0);
        bird.setVisible (f == 1);
        clock.setVisible (f == 2);
        static const char* labels[3][4] = { { "Röst", "Monoton", "Tempo", "Trasig" },
                                            { "Luft", "Lugn", "Kammar", "Trasig" },
                                            { "Klockor", "Jämn gång", "Takt", "Trasig" } };
        int i = 0;
        for (auto* k : { &voice, &monotone, &tempo, &broken })
            k->setLabel (u8 (labels[f][i++]));
        background.invalidate();
        updateDisplay();
        repaint();
    }

    void updateDisplay()
    {
        const int f = figureIndex();
        if (f == 1)
            display.setLines (u8 ("SÅNGFÅGELN"), spoken >= 100 ? u8 ("SJUNGER") : u8 ("I BUREN"));
        else if (f == 2)
            display.setLines ("URVERKET", spoken >= 100 ? u8 ("TICK TACK") : u8 ("STÅR STILL"));
        else
            display.setLines ("TALMASKINEN", spoken < 0 || spoken >= 100 ? u8 ("VÄNTAR") : VfdDisplay::upper (u8 (automatWord (spoken))));
    }

    void timerCallback() override
    {
        // Leksakerna rör sig medan de spelar: fågeln öppnar näbben, urets visare och pendel går.
        if (spoken >= 100) { ++frame; repaint (visArea); }
    }

    // Talmaskinen: mässingshuvud med galler till mun, ögon som glöder när den talar.
    void paintHead (juce::Graphics& g)
    {
        using namespace juce;
        if (visArea.isEmpty()) return;
        const auto a = jugend::plaqueFor (g, visArea.toFloat(), 0.45f).reduced (6.0f, 4.0f);   // på en jugendplakett
        const bool talking = spoken >= 0 && spoken < 100;
        const float brokenAmt = (float) broken.slider.valueToProportionOfLength (broken.slider.getValue());
        auto head = Rectangle<float> (jmin (a.getWidth() * 0.5f, a.getHeight() * 0.66f), a.getHeight() * 0.76f).withCentre (a.getCentre().translated (0.0f, a.getHeight() * 0.06f));
        ColourGradient grad (colours::brassLight, head.getX(), head.getY(), colours::brassDark, head.getRight(), head.getBottom(), false);
        grad.addColour (0.45, colours::brass);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (head, head.getWidth() * 0.22f);
        g.setColour (colours::bakelite.withAlpha (0.6f));
        g.drawRoundedRectangle (head, head.getWidth() * 0.22f, 2.0f);
        for (int i = 0; i < 6; ++i)
            g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre ({ head.getX() + head.getWidth() * (0.15f + 0.14f * i), head.getY() + 10.0f }));
        for (int e = 0; e < 2; ++e)
        {
            auto eye = Rectangle<float> (head.getWidth() * 0.2f, head.getWidth() * 0.2f)
                           .withCentre ({ head.getX() + head.getWidth() * (e == 0 ? 0.3f : 0.7f), head.getY() + head.getHeight() * 0.33f });
            drawBrassBezel (g, eye.expanded (3.0f), eye.getWidth() * 0.5f + 3.0f, 3.0f);
            const float glow = talking ? (e == 1 && brokenAmt > 0.4f ? 0.45f : 1.0f) : 0.12f;
            g.setColour (colours::tubeOrange.withAlpha (glow));
            g.fillEllipse (eye);
            g.setColour (Colours::white.withAlpha (0.25f * glow));
            g.fillEllipse (eye.reduced (eye.getWidth() * 0.32f).translated (-eye.getWidth() * 0.1f, -eye.getHeight() * 0.1f));
        }
        auto mouth = Rectangle<float> (head.getWidth() * 0.56f, head.getHeight() * 0.18f)
                         .withCentre ({ head.getCentreX(), head.getY() + head.getHeight() * 0.7f });
        g.setColour (talking ? colours::tubeOrange.withAlpha (0.55f) : colours::bakelite);
        g.fillRoundedRectangle (mouth, 4.0f);
        g.setColour (colours::brassDark);
        for (int i = 1; i < 9; ++i)
        {
            const float x = mouth.getX() + mouth.getWidth() * (float) i / 9.0f;
            g.drawLine (x, mouth.getY(), x, mouth.getBottom(), 2.0f);
        }
        g.drawRoundedRectangle (mouth, 4.0f, 2.0f);
        if (brokenAmt > 0.3f)
        {
            g.setColour (colours::bakelite.withAlpha (0.7f));
            Path crack;
            crack.startNewSubPath (head.getRight() - head.getWidth() * 0.2f, head.getY() + 6.0f);
            crack.lineTo (head.getRight() - head.getWidth() * 0.26f, head.getY() + head.getHeight() * 0.15f);
            crack.lineTo (head.getRight() - head.getWidth() * 0.18f, head.getY() + head.getHeight() * 0.24f);
            g.strokePath (crack, PathStrokeType (1.4f));
        }
    }

    // Sångfågeln: en mässingsfågel på en pinne i en fågelbur; näbben öppnar sig när den sjunger.
    void paintBird (juce::Graphics& g)
    {
        using namespace juce;
        if (visArea.isEmpty()) return;
        const auto a = jugend::plaqueFor (g, visArea.toFloat(), 0.45f).reduced (6.0f, 4.0f);   // på en jugendplakett
        const bool singing = spoken >= 100;
        const auto c = a.getCentre();
        const float s = jmin (a.getWidth(), a.getHeight()) * 0.45f;

        // Buren: kupol och galler.
        Path cage;
        cage.addArc (c.x - s, c.y - s * 0.9f, s * 2.0f, s * 1.6f, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, true);
        g.setColour (colours::brassDark);
        g.strokePath (cage, PathStrokeType (2.0f));
        for (int i = -4; i <= 4; ++i)
        {
            const float x = c.x + s * (float) i / 4.4f;
            const float top = c.y - s * 0.1f - std::sqrt (jmax (0.0f, 1.0f - (float) (i * i) / 19.4f)) * s * 0.8f;
            g.drawLine (x, top, x, c.y + s * 0.9f, 1.2f);
        }
        g.fillRoundedRectangle (Rectangle<float> (s * 2.2f, 10.0f).withCentre ({ c.x, c.y + s * 0.92f }), 3.0f);

        // Pinnen och fågeln.
        g.setColour (Colour (0xff5a3519));
        g.fillRoundedRectangle (Rectangle<float> (s * 1.3f, 5.0f).withCentre ({ c.x, c.y + s * 0.38f }), 2.0f);
        const float bob = singing ? std::sin ((float) frame * 1.3f) * 2.0f : 0.0f;
        auto body = Rectangle<float> (s * 0.75f, s * 0.48f).withCentre ({ c.x - s * 0.05f, c.y + s * 0.12f + bob });
        ColourGradient brass (colours::brassLight, body.getX(), body.getY(), colours::brassDark, body.getRight(), body.getBottom(), false);
        g.setGradientFill (brass);
        g.fillEllipse (body);
        Path tail;
        tail.addTriangle (body.getX() + 6.0f, body.getCentreY(), body.getX() - s * 0.35f, body.getBottom() + s * 0.05f, body.getX() - s * 0.3f, body.getBottom() + s * 0.2f);
        g.fillPath (tail);
        auto head = Rectangle<float> (s * 0.34f, s * 0.34f).withCentre ({ body.getRight() - s * 0.08f, body.getY() + 2.0f });
        g.setGradientFill (brass);
        g.fillEllipse (head);
        g.setColour (colours::bakelite);
        g.fillEllipse (Rectangle<float> (5.0f, 5.0f).withCentre ({ head.getCentreX() + 3.0f, head.getCentreY() - 2.0f }));
        // Näbben: öppen när fågeln sjunger.
        const float open = singing ? 0.5f + 0.5f * std::sin ((float) frame * 2.1f) : 0.0f;
        const auto tip = Point<float> (head.getRight() + s * 0.2f, head.getCentreY() + 2.0f);
        Path beak;
        beak.addTriangle (head.getRight() - 3.0f, head.getCentreY() - 3.0f, tip.x, tip.y - 5.0f * open, head.getRight() - 3.0f, head.getCentreY() + 3.0f);
        beak.addTriangle (head.getRight() - 3.0f, head.getCentreY() + 1.0f, tip.x - 2.0f, tip.y + 5.0f * open, head.getRight() - 3.0f, head.getCentreY() + 5.0f);
        g.setColour (colours::tubeOrange.darker (0.2f));
        g.fillPath (beak);
        // Vingen: en gravyr.
        g.setColour (colours::brassDark);
        {
            Path wing;
            wing.addCentredArc (body.getCentreX() - 4.0f, body.getCentreY(), body.getWidth() * 0.3f, body.getHeight() * 0.3f, 0.0f, 0.3f, 2.4f, true);
            g.strokePath (wing, PathStrokeType (1.6f));
        }
        // Noter som flyger ut när den sjunger.
        if (singing)
        {
            g.setColour (colours::vfdText.withAlpha (0.8f));
            for (int k = 0; k < 3; ++k)
            {
                const float t = std::fmod ((float) frame * 0.05f + (float) k / 3.0f, 1.0f);
                g.fillEllipse (Rectangle<float> (5.0f, 4.0f).withCentre ({ tip.x + 10.0f + t * s * 0.6f, tip.y - 8.0f - t * s * 0.5f }));
            }
        }
    }

    // Urverket: urtavla med visare, kugghjul bakom och en gökdörr ovanför.
    void paintClock (juce::Graphics& g)
    {
        using namespace juce;
        if (visArea.isEmpty()) return;
        const auto a = jugend::plaqueFor (g, visArea.toFloat(), 0.45f).reduced (6.0f, 4.0f);   // på en jugendplakett
        const bool running = spoken >= 100;
        const auto c = a.getCentre().translated (0.0f, a.getHeight() * 0.08f);
        const float r = jmin (a.getWidth(), a.getHeight()) * 0.36f;

        // Kugghjul bakom tavlan.
        auto gear = [&] (Point<float> gc, float gr, int teeth, float turn)
        {
            Path p;
            for (int t = 0; t < teeth * 2; ++t)
            {
                const float ang = turn + MathConstants<float>::twoPi * (float) t / (float) (teeth * 2);
                const float rr = (t % 2 == 0) ? gr : gr * 0.82f;
                const auto pt = gc.getPointOnCircumference (rr, ang);
                if (t == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            g.setColour (colours::brassDark);
            g.fillPath (p);
            g.setColour (colours::bakelite.withAlpha (0.6f));
            g.fillEllipse (Rectangle<float> (gr * 0.3f, gr * 0.3f).withCentre (gc));
        };
        const float turn = running ? (float) frame * 0.08f : 0.0f;
        gear (c.translated (-r * 0.95f, r * 0.55f), r * 0.45f, 12, turn);
        gear (c.translated (r * 1.0f, r * 0.45f), r * 0.35f, 9, -turn * 1.33f);

        // Gökdörren.
        auto house = Rectangle<float> (r * 0.9f, r * 0.55f).withCentre ({ c.x, c.y - r * 1.12f });
        g.setColour (Colour (0xff5a3519));
        g.fillRect (house);
        Path roof;
        roof.addTriangle (house.getX() - 6.0f, house.getY(), house.getRight() + 6.0f, house.getY(), house.getCentreX(), house.getY() - r * 0.3f);
        g.fillPath (roof);
        g.setColour (colours::bakelite);
        g.fillRect (house.withSizeKeepingCentre (house.getWidth() * 0.35f, house.getHeight() * 0.7f));

        // Tavlan och visarna.
        drawBrassBezel (g, Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), r, 6.0f);
        g.setColour (colours::meterFace);
        g.fillEllipse (Rectangle<float> (r * 1.84f, r * 1.84f).withCentre (c));
        g.setColour (colours::bakelite);
        for (int h = 0; h < 12; ++h)
        {
            const float ang = MathConstants<float>::twoPi * (float) h / 12.0f;
            g.drawLine (Line<float> (c.getPointOnCircumference (r * 0.78f, ang), c.getPointOnCircumference (r * 0.88f, ang)), h % 3 == 0 ? 2.5f : 1.2f);
        }
        const float sec = running ? (float) (frame % 60) / 60.0f : 0.1f;
        g.drawLine (Line<float> (c, c.getPointOnCircumference (r * 0.5f, MathConstants<float>::twoPi * 0.83f)), 3.5f);
        g.drawLine (Line<float> (c, c.getPointOnCircumference (r * 0.72f, MathConstants<float>::twoPi * 0.05f)), 2.2f);
        g.setColour (colours::meterRed);
        g.drawLine (Line<float> (c, c.getPointOnCircumference (r * 0.8f, MathConstants<float>::twoPi * sec)), 1.2f);
        g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (c));
    }

    juce::ComboBox figure, word, bird, clock, repeat;
    KrangKnob voice { u8 ("Röst") }, monotone { "Monoton" }, tempo { "Tempo" }, broken { "Trasig" };
    VfdDisplay display;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    juce::Rectangle<int> visArea;
    textures::BackgroundCache background;
    int spoken = -2, frame = 0;
};

// --- Krigstuban -----------------------------------------------------------------

class KrigstubanPanel : public juce::Component
{
public:
    KrigstubanPanel()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &bell, &ensemble, &pressure, &growl, &flutter, &swell, &fall, &release, &rumble })
            addAndMakeVisible (c);
        bell.onChange = [this] { repaint (visArea); };
        ensemble.slider.onValueChange = [this] { repaint (visArea); };
        pressure.slider.onValueChange = [this] { repaint (visArea); };
        bell.setTooltip (u8 ("Mässing: en vanlig tubaklocka. Ljudtratt: första världskrigets jättelika lyssnartrattar, ihåligt och avlägset. Megafon: en trasig högtalare."));
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        paintHorns (g);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("TRATT", bell.getBounds().translated (0, -14).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto blowF = r.removeFromLeft (400);  r.removeFromLeft (12);
        auto shapeF = r.removeFromLeft (290); r.removeFromLeft (12);
        auto bellF = r.removeFromLeft (170);  r.removeFromLeft (12);
        auto vis = r;
        frames = { { blowF, u8 ("Blåsarna") }, { shapeF, "Tonen" }, { bellF, "Tratten" }, { vis, "Krigstuban" } };
        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28 + 60).withHeight (150).reduced (6, 0); };
        layoutRow (knobRow (blowF), { &ensemble, &pressure, &growl, &flutter }, 92);
        layoutRow (knobRow (shapeF), { &swell, &fall, &release }, 90);
        bell.setBounds (bellF.withTrimmedTop (28 + 26).withHeight (26).withSizeKeepingCentre (140, 26));
        rumble.setBounds (bellF.withTrimmedTop (28 + 70).withHeight (150).withSizeKeepingCentre (92, 150));
        visArea = vis.withTrimmedTop (30).reduced (8, 6);
    }

    Bindings bindings()
    {
        return { { { &ensemble.slider, "ktEnsemble" }, { &pressure.slider, "ktPressure" }, { &growl.slider, "ktGrowl" },
                   { &flutter.slider, "ktFlutter" }, { &swell.slider, "ktSwell" }, { &fall.slider, "ktFall" },
                   { &release.slider, "ktRelease" }, { &rumble.slider, "ktRumble" } },
                 { { &bell, "ktBell" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    float prop (KrangKnob& k) const { return (float) k.slider.valueToProportionOfLength (k.slider.getValue()); }

    // Krigstuban på en jugendplakett: fler instrument ju större orkestern är – helikoner, lyssnartrattar eller megafoner.
    void paintHorns (juce::Graphics& g)
    {
        if (visArea.isEmpty()) return;
        jugend::tuba (g, visArea.toFloat(), juce::jmax (0, bell.getSelectedItemIndex()),
                      1 + (int) std::round (prop (ensemble) * 3.0f), prop (pressure));
    }

    juce::ComboBox bell;
    KrangKnob ensemble { "Orkester" }, pressure { "Lufttryck" }, growl { "Morr" }, flutter { "Fladder" },
              swell { u8 ("Svällning") }, fall { "Fall" }, release { "Avklang" }, rumble { "Mullret" };
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
                                                                 &arpMode, &arpRate, &arpOct, &arpGate, &arpLatch, &attack, &release,
                                                                 &engine, &pdWave, &pdAmount, &pdDetune })
            addAndMakeVisible (c);
        for (auto* k : { &decay, &arpOct, &arpGate, &grain, &grainMs, &spread })
            k->labelSize = 11.0f;
        engine.onChange = [this] { updateEngine(); };
        arpMode.onChange = [this] { repaint (coilArea); };
        engine.setTooltip (u8 ("FM-spolen: en spole som plinkar. Fasvridaren: fasdistorsion som i 80-talets CZ-syntar."));
        pdWave.setTooltip (u8 ("Fasvridarens våg: sågtand, fyrkant, puls, dubbelsinus eller en av de resonanta vågorna"));
        arpLatch.setTooltip (u8 ("Fäst: slingan fortsätter när du släpper tangenterna, tills du spelar ett nytt ackord"));
        updateEngine();
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
        // Spolen: gnistorna slingrar sig när Slingan går.
        if (! coilArea.isEmpty())
            jugend::coil (g, coilArea.toFloat(), isPd(), arpMode.getSelectedItemIndex() > 0 ? 1.0f : 0.0f);
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        for (auto& [r, title] : frames)
            drawModuleFrame (g, r.toFloat(), title == "FM" ? (isPd() ? u8 ("Fasvridaren") : u8 ("FM-spolen")) : title);
        g.setColour (colours::brass);
        g.setFont (panelFont (10.0f, false));
        g.drawText ("RIKTNING", arpMode.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("TAKT", arpRate.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
        g.drawText (u8 ("FÄST"), arpLatch.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
        g.drawText ("MOTOR", engine.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
        if (isPd())
            g.drawText (u8 ("VÅG"), pdWave.getBounds().translated (0, -13).withHeight (12), juce::Justification::centredLeft);
    }

    void resized() override
    {
        background.invalidate();
        frames.clear();
        auto r = getLocalBounds().reduced (12, 10);
        auto fm    = r.removeFromLeft (280); r.removeFromLeft (10);
        auto mun   = r.removeFromLeft (176); r.removeFromLeft (10);
        auto korn  = r.removeFromLeft (238); r.removeFromLeft (10);
        auto vis   = r.removeFromRight (112); r.removeFromRight (10);
        auto env   = r.removeFromRight (120); r.removeFromRight (10);
        auto slinga = r;
        frames = { { fm, "FM" }, { mun, "Munharpa" }, { korn, "Radiokorn" }, { slinga, "Slingan" }, { env, "Envelope" }, { vis, "Spolen" } };
        coilArea = vis.withTrimmedTop (30).reduced (6, 6);

        // Motorval överst i spolens ram, vågen bredvid (bara för Fasvridaren).
        auto top = fm.withTrimmedTop (28 + 16).reduced (12, 0).removeFromTop (24);
        engine.setBounds (top.removeFromLeft (126));
        top.removeFromLeft (12);
        pdWave.setBounds (top);
        knobArea = fm.withTrimmedTop (28 + 46).reduced (6, 4).withSizeKeepingCentre (fm.getWidth() - 12, 140);
        shownPd = ! isPd();   // tvinga fram en ny placering
        updateEngine();

        auto knobRow = [] (juce::Rectangle<int> f) { return f.withTrimmedTop (28).reduced (6, 4).withSizeKeepingCentre (f.getWidth() - 12, 140); };
        layoutRow (knobRow (mun),  { &harp, &mouth }, 84);
        layoutRow (knobRow (korn), { &grain, &grainMs, &spread }, 80);

        auto sl = slinga.withTrimmedTop (28).reduced (10, 6);
        auto combos = sl.removeFromLeft (96);
        combos.removeFromTop (12);
        arpMode.setBounds (combos.removeFromTop (26));
        combos.removeFromTop (22);
        arpRate.setBounds (combos.removeFromTop (26));
        combos.removeFromTop (22);
        arpLatch.setBounds (combos.removeFromTop (26));
        sl.removeFromLeft (6);
        auto knobs = sl;
        arpOct.setBounds (knobs.removeFromTop (knobs.getHeight() / 2).withSizeKeepingCentre (juce::jmin (knobs.getWidth(), 80), 118));
        arpGate.setBounds (knobs.withSizeKeepingCentre (juce::jmin (knobs.getWidth(), 80), 118));

        layoutRow (env.withTrimmedTop (28).reduced (6, 8), { &attack, &release }, 56);
    }

    Bindings bindings()
    {
        return { { { &ratio.slider, "spRatio" }, { &index.slider, "spIndex" }, { &decay.slider, "spDecay" },
                   { &harp.slider, "spHarp" }, { &mouth.slider, "spMouth" }, { &grain.slider, "spGrain" },
                   { &grainMs.slider, "spGrainMs" }, { &spread.slider, "spSpread" }, { &arpOct.slider, "spArpOct" },
                   { &arpGate.slider, "spArpGate" }, { &attack.slider, "spAttack" }, { &release.slider, "spRelease" },
                   { &pdAmount.slider, "spPdAmount" }, { &pdDetune.slider, "spPdDetune" } },
                 { { &arpMode, "spArp" }, { &arpRate, "spArpRate" }, { &arpLatch, "spArpLatch" },
                   { &engine, "spEngine" }, { &pdWave, "spPdWave" } } };
    }

private:
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    bool isPd() const { return engine.getSelectedItemIndex() == 1; }

    void updateEngine()
    {
        const bool pd = isPd();
        ratio.setVisible (! pd);
        index.setVisible (! pd);
        pdWave.setVisible (pd);
        pdAmount.setVisible (pd);
        pdDetune.setVisible (pd);
        if (pd != shownPd)
        {
            shownPd = pd;
            if (pd) layoutRow (knobArea, { &pdAmount, &decay, &pdDetune }, 92);
            else    layoutRow (knobArea, { &ratio, &index, &decay }, 92);
            background.invalidate();
            repaint();
        }
    }

    KrangKnob ratio { "Ratio" }, index { "Index" }, decay { "Avklingning" },
              harp { "Munharpa" }, mouth { u8 ("Munhåla") },
              grain { "Korn" }, grainMs { "Storlek" }, spread { "Spridning" },
              arpOct { "Oktaver" }, arpGate { u8 ("Tonlängd") },
              pdAmount { "Vridning" }, pdDetune { u8 ("Svävning") };
    juce::ComboBox arpMode, arpRate, arpLatch, engine, pdWave;
    KrangFader attack { "ATTACK" }, release { "RELEASE" };
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> frames;
    textures::BackgroundCache background;
    juce::Rectangle<int> knobArea, coilArea;
    bool shownPd = false;
};

// --- Hammarverket ---------------------------------------------------------------

class HammarverketPanel : public juce::Component
{
public:
    HammarverketPanel (StepPattern& pattern, std::atomic<int>& playing) : steg (pattern, playing)
    {
        addAndMakeVisible (steg);
        addAndMakeVisible (rost);

        // Ljud per spår: osynliga listor som håller parametrarna; man väljer via spårnamnet i rutnätet.
        for (int d = 0; d < numDrums; ++d)
        {
            auto& c = sounds[(size_t) d];
            for (int k = 0; k < numDrumSounds; ++k)
                c.addItem (juce::String::fromUTF8 (drumSoundName (k)), k + 1);
            c.onChange = [this] { soundsChanged(); };
            addChildComponent (c);
        }
        steg.getGrid().onLaneClicked = [this] (int lane, juce::Rectangle<int> area) { chooseSound (lane, area); };

        // Kit: byter ljud på alla åtta spåren på en gång.
        for (int k = 0; k < numDrumKits; ++k)
            kit.addItem (juce::String::fromUTF8 (drumKitName (k)), k + 1);
        kit.setTextWhenNothingSelected ("Eget kit");
        kit.setTooltip (juce::String::fromUTF8 ("Färdiga kit: byter ljud på alla åtta spåren. Klicka på ett spårnamn för att byta ett enskilt ljud."));
        kit.onChange = [this]
        {
            const int k = kit.getSelectedItemIndex();
            if (k < 0 || applyingKit) return;
            applyingKit = true;
            const auto& set = drumKit (k);
            for (int d = 0; d < numDrums; ++d)
                sounds[(size_t) d].setSelectedId (set[(size_t) d] + 1, juce::sendNotificationSync);
            applyingKit = false;
            soundsChanged();
        };
        addAndMakeVisible (kit);
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
        kit.setBounds (voicesFrame.getX() + 84, voicesFrame.getY() + 5, 124, 22);
    }

    Bindings bindings()
    {
        Bindings b;
        for (int d = 0; d < numDrums; ++d)
            b.combos.push_back ({ &sounds[(size_t) d], "hv" + juce::String (d) + "Sound" });
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
    // Spårnamnen och kitlistan följer ljuden (från presets, DAW:en eller ett val i menyn).
    void soundsChanged()
    {
        if (applyingKit) return;
        int match = -1;
        for (int k = 0; k < numDrumKits && match < 0; ++k)
        {
            bool same = true;
            for (int d = 0; d < numDrums; ++d)
                same = same && sounds[(size_t) d].getSelectedItemIndex() == drumKit (k)[(size_t) d];
            if (same) match = k;
        }
        kit.setSelectedId (match + 1, juce::dontSendNotification);
        for (int d = 0; d < numDrums; ++d)
        {
            const int sound = juce::jmax (0, sounds[(size_t) d].getSelectedItemIndex());
            const auto name = juce::String::fromUTF8 (drumSoundName (sound));
            steg.getGrid().setLaneName (d, name);
            for (auto* k : { &tune[(size_t) d], &decay[(size_t) d], &level[(size_t) d] })
                k->setTooltip (name);
        }
    }

    void chooseSound (int lane, juce::Rectangle<int> area)
    {
        juce::PopupMenu menu;
        const int current = sounds[(size_t) lane].getSelectedItemIndex();
        menu.addSectionHeader (juce::String::fromUTF8 ("Ljud för spår ") + juce::String (lane + 1));
        for (int k = 0; k < numDrumSounds; ++k)
        {
            if (k == 8) menu.addSeparator();
            menu.addItem (k + 1, juce::String::fromUTF8 (drumSoundName (k)), true, k == current);
        }
        juce::Component::SafePointer<HammarverketPanel> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&steg.getGrid())
                                .withTargetScreenArea (steg.getGrid().localAreaToGlobal (area)),
                            [safe, lane] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->sounds[(size_t) lane].setSelectedId (result, juce::sendNotificationSync);
                            });
    }

    textures::BackgroundCache background;
    std::array<juce::ComboBox, (size_t) numDrums> sounds;
    juce::ComboBox kit;
    bool applyingKit = false;
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
    }

    void resized() override
    {
        background.invalidate();
        auto r = getLocalBounds().reduced (10, 8).withTrimmedBottom (8);
        // Alla rattar lika stora: modulens bredd följer antalet rattar, reverben får det som blir över.
        const int pipe = 10;
        const int numKnobs[numModules] = { 4, 3, 4, 4, 2 };
        const int extra[numModules] = { 0, 36, 0, 0, 0 };   // Ångfiltrets etiketter (FREKVENS, RESONANS) är långa
        for (int i = 0; i < numModules; ++i)
        {
            const int w = i == numModules - 1 ? r.getWidth() : numKnobs[i] * knobWidth + 6 + extra[i];
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
    static constexpr int numModules = 5, knobWidth = 61;
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

// --- Kaosavdelningen: Kaosmotorn, Megafonen, Gramofonen och Krangskopet ---------------

// Oscilloskop i grönt glas: visar utgången, triggat på uppåtgående nollgenomgång så att tonen står still.
class Krangskop : public juce::Component
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
        for (auto* c : std::initializer_list<juce::Component*> { &sync, &lamp, &scope, &grGate, &mgGate })
            addAndMakeVisible (c);
        for (auto* k : knobs())
        {
            addAndMakeVisible (k);
            k->labelSize = 10.5f;
        }
        lamp.setTooltip (juce::String::fromUTF8 ("Kaoslampan blixtrar när Kaosmotorn slår till"));
        mgAmount.slider.setTooltip (u8 ("Megafonen: en trasig trattalare efter Ångfiltret. På noll är den förbikopplad."));
        grCrackle.slider.setTooltip (u8 ("Gramofonen: en sliten 78-varvsskiva efter Bandmaskinen. Med alla rattar på noll är den förbikopplad."));
        mgGate.setTooltip (u8 ("Spelar: megafonen brummar hela tiden (Alltid) eller bara när ett ljud spelas (Vid ljud)."));
        grGate.setTooltip (u8 ("Spelar: knaster och brus hörs hela tiden (Alltid) eller bara när ett ljud spelas (Vid ljud)."));
    }

    void paint (juce::Graphics& g) override
    {
        background.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { paintBackground (bg); });
    }

    void paintBackground (juce::Graphics& g)
    {
        textures::fillChassis (g, getLocalBounds().toFloat(), 0.25f);
        drawModuleFrame (g, motor.toFloat(), "Kaosmotorn");
        drawModuleFrame (g, megaFrame.toFloat(), "Megafonen");
        drawModuleFrame (g, gramFrame.toFloat(), "Gramofonen");
        drawModuleFrame (g, scopeFrame.toFloat(), "Krangskopet");
        g.setColour (colours::engraving);
        g.setFont (panelFont (10.0f));
        g.drawText ("TAKT", syncLabel, juce::Justification::centred);
    }

    void resized() override
    {
        // Kaosavdelningen har kammarens plats: Kaosmotorn överst, Megafonen och Gramofonen under,
        // och Krangskopet stort till höger.
        background.invalidate();
        auto r = getLocalBounds().reduced (12, 10);
        scopeFrame = r.removeFromRight (360);
        r.removeFromRight (12);
        motor = r.removeFromTop ((r.getHeight() - 12) / 2);
        r.removeFromTop (12);
        constexpr int kw = 70;
        megaFrame = r.removeFromLeft (4 * kw + 14 + 64);
        r.removeFromLeft (12);
        gramFrame = r;

        auto in = motor.withTrimmedTop (28).reduced (10, 2);
        auto left = in.removeFromLeft (90);
        syncLabel = left.removeFromTop (16);
        sync.setBounds (left.removeFromTop (24).reduced (0, 1));
        lamp.setBounds (left.withSizeKeepingCentre (34, 34));
        in.removeFromLeft (10);
        layoutRow (in, { &amount, &rate, &drift, &stutter, &reverse, &crush }, 84);
        auto megaIn = megaFrame.withTrimmedTop (28).reduced (6, 2);
        mgGate.setBounds (megaIn.removeFromLeft (60).withTrimmedBottom (16));
        megaIn.removeFromLeft (4);
        layoutRow (megaIn, { &mgAmount, &mgHorn, &mgCrack, &mgHum }, kw);
        auto gramIn = gramFrame.withTrimmedTop (28).reduced (6, 2);
        grGate.setBounds (gramIn.removeFromLeft (60).withTrimmedBottom (16));
        gramIn.removeFromLeft (4);
        layoutRow (gramIn, { &grCrackle, &grHiss, &grWow, &grAge, &grSkip }, kw);

        scope.setBounds (scopeFrame.withTrimmedTop (30).reduced (14, 12));
    }

    Bindings bindings()
    {
        return { { { &amount.slider, "kaAmount" }, { &rate.slider, "kaRate" }, { &drift.slider, "kaDrift" },
                   { &stutter.slider, "kaStutter" }, { &reverse.slider, "kaReverse" }, { &crush.slider, "kaCrush" },
                   { &mgAmount.slider, "mgAmount" }, { &mgHorn.slider, "mgHorn" }, { &mgCrack.slider, "mgCrack" }, { &mgHum.slider, "mgHum" },
                   { &grCrackle.slider, "grCrackle" }, { &grHiss.slider, "grHiss" }, { &grWow.slider, "grWow" },
                   { &grAge.slider, "grAge" }, { &grSkip.slider, "grSkip" } },
                 { { &sync, "kaSync" }, { &grGate.choice, "grGate" }, { &mgGate.choice, "mgGate" } } };
    }

    KaosLamp lamp;
    Krangskop scope;

private:
    textures::BackgroundCache background;
    static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }
    std::vector<KrangKnob*> knobs() { return { &amount, &rate, &drift, &stutter, &reverse, &crush,
                                               &mgAmount, &mgHorn, &mgCrack, &mgHum,
                                               &grCrackle, &grHiss, &grWow, &grAge, &grSkip }; }

    juce::ComboBox sync;
    KrangToggle grGate { "SPELAR", "ALLTID", "VID LJUD" }, mgGate { "SPELAR", "ALLTID", "VID LJUD" };
    KrangKnob amount { "Kaos" }, rate { "Fri takt" }, drift { "Drift" }, stutter { "Hack" },
              reverse { u8 ("Bakåt") }, crush { u8 ("Sönder") },
              mgAmount { u8 ("Mängd") }, mgHorn { "Tratt" }, mgCrack { "Spricka" }, mgHum { "Brum" },
              grCrackle { "Knaster" }, grHiss { "Ytbrus" }, grWow { "Varvtal" }, grAge { u8 ("Ålder") }, grSkip { "Hopp" };
    juce::Rectangle<int> motor, megaFrame, gramFrame, scopeFrame, syncLabel;
};

} // namespace krang::ui
