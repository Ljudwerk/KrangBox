// KrangSnapshot – ritar Krang-stilprovet till PNG utan att starta en DAW.
//
//   KrangSnapshot <fil.png> [skala] [steg-fil.png] [--tid]
//   KrangSnapshot --kaos <fil.png>   (Kaosraden med en hackad våg i Kaosskopet)
//   KrangSnapshot --modulation <fil.png>   (Modulationsraden med tre kopplingar och tända lampor)
//   KrangSnapshot --tonsteg <fil.png> [skala] [sida]   (Tonstegverket med startmelodin; med sida: STEG 16 och den sidan öppen)
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/KrangStyle.h"
#include "ui/StegverketPanel.h"
#include "ui/Panels.h"
#include "ui/ModulationPanel.h"
#include "ui/TonStegverketPanel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>

namespace
{
struct SnapshotHost : juce::AudioProcessor
{
    SnapshotHost() : AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo())) {}
    const juce::String getName() const override { return "Snapshot"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    juce::AudioProcessorValueTreeState state { *this, nullptr, "KrangBox", krang::params::createLayout() };
};

// Kopplar en panels kontroller till parametrarna, som editorn gör.
struct Attach
{
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    void slider (juce::AudioProcessorValueTreeState& st, const juce::String& pid, juce::Slider& s) { sliders.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, pid, s)); }
    void combo (juce::AudioProcessorValueTreeState& st, const juce::String& pid, juce::ComboBox& c)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (pid))) c.addItemList (choice->choices, 1);
        combos.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, pid, c));
    }
};
}

int main (int argc, char* argv[])
{
    if (argc < 2) { std::cerr << "Användning: KrangSnapshot <fil.png> [skala]\n"; return 1; }
    juce::ScopedJuceInitialiser_GUI gui;

    // KrangSnapshot --stilprov-textur <fil.png>: Basmålen-modulen med dagens texturer och med stilprovets åldrade ytor.
    if (juce::String (argv[1]) == "--stilprov-textur" && argc > 2)
    {
        krang::ui::KrangLookAndFeel lf;
        auto render = [&] (bool agedLook)
        {
            krang::ui::textures::useAgedLook() = agedLook;
            krang::ui::BasmalenPanel panel;
            panel.setLookAndFeel (&lf);
            panel.setSize (1200, 320);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            auto img = panel.createComponentSnapshot (panel.getLocalBounds(), true, 1.5f);
            std::cout << (agedLook ? "stilprov" : "idag") << ": " << juce::Time::getMillisecondCounterHiRes() - start << " ms\n";
            panel.setLookAndFeel (nullptr);
            return img;
        };
        const auto before = render (false), after = render (true);
        juce::Image sheet (juce::Image::ARGB, before.getWidth(), before.getHeight() * 2 + 60, true);
        {
            juce::Graphics g (sheet);
            g.fillAll (juce::Colour (0xff101010));
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
            g.drawText (juce::String::fromUTF8 ("IDAG: upprepad textur"), 16, 0, 800, 30, juce::Justification::centredLeft);
            g.drawImageAt (before, 0, 30);
            g.drawText (juce::String::fromUTF8 ("STILPROV: unika ytor med ljus, slitage, repor och rinnande rost"), 16, before.getHeight() + 30, 1400, 30,
                        juce::Justification::centredLeft);
            g.drawImageAt (after, 0, before.getHeight() + 60);
        }
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (sheet, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--tonsteg" && argc > 2)
    {
        SnapshotHost host;
        if (auto* p = host.state.getParameter ("seqRun")) p->setValueNotifyingHost (1.0f);
        if (auto* p = host.state.getParameter ("seqLength")) p->setValueNotifyingHost (p->convertTo0to1 (argc > 4 ? 0 : 1));
        krang::TonPattern melody;
        melody.loadDefault();
        std::atomic<int> playing { 6 }, transposed { 5 };
        krang::ui::KrangLookAndFeel lf;
        krang::ui::TonStegverketPanel panel (melody, playing, transposed);
        panel.setLookAndFeel (&lf);
        panel.setSize (1200, 320);
        Attach a;
        a.combo (host.state, "seqLength", panel.length);
        a.slider (host.state, "seqTempo", panel.tempo);
        a.slider (host.state, "seqSwing", panel.swing);
        a.slider (host.state, "tsRoot", panel.root);
        a.combo (host.state, "tsRate", panel.rate);
        a.combo (host.state, "tsKeys", panel.keys);
        a.combo (host.state, "tsScale", panel.scale);
        panel.play.setToggleState (true, juce::sendNotificationSync);
        panel.root.textFromValueFunction = [] (double v) { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 4); };
        panel.root.updateText();
        if (argc > 4) panel.showPage (juce::String (argv[4]).getIntValue() - 1);   // t.ex. sida 3 med STEG 16
        const float scale = argc > 3 ? juce::String (argv[3]).getFloatValue() : 1.0f;
        const auto img = panel.createComponentSnapshot (panel.getLocalBounds(), true, scale);
        a = {};
        panel.setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--modulation" && argc > 2)
    {
        SnapshotHost host;
        auto set = [&host] (const juce::String& pid, float v)
        {
            if (auto* p = host.state.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (v));
        };
        set ("lfo1Shape", 0); set ("lfo1Sync", 5); set ("lfo2Shape", 5); set ("lfo2Rate", 0.4f);
        set ("env1A", 0.02f); set ("env1D", 0.6f); set ("env1S", 0.3f); set ("env1R", 1.2f);
        set ("mod1Src", 1); set ("mod1Dst", 1); set ("mod1Amt", 0.45f);
        set ("mod2Src", 3); set ("mod2Dst", 3); set ("mod2Amt", -0.6f);
        set ("mod3Src", 2); set ("mod3Dst", 9); set ("mod3Amt", 0.3f);

        krang::ui::KrangLookAndFeel lf;
        krang::ui::ModulationPanel mod;
        mod.setLookAndFeel (&lf);
        mod.setSize (1200, 320);
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> ca;
        const auto b = mod.bindings();
        for (auto& [slider, pid] : b.sliders) sa.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (host.state, pid, *slider));
        for (auto& [combo, pid] : b.combos)
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (host.state.getParameter (pid)))
                combo->addItemList (choice->choices, 1);
            ca.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (host.state, pid, *combo));
        }
        mod.setSourceValues ({ 0.8f, -0.3f, 0.7f, 0.0f, 0.0f, 0.0f, 0.8f });
        const float scale = argc > 3 ? juce::String (argv[3]).getFloatValue() : 1.0f;
        const auto img = mod.createComponentSnapshot (mod.getLocalBounds(), true, scale);
        sa.clear(); ca.clear();
        mod.setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        std::cout << "aktiva kopplingar: " << mod.activeCount() << "\n";
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--kaos" && argc > 2)
    {
        krang::ui::KrangLookAndFeel lf;
        krang::ui::KaosPanel kaos;
        kaos.setLookAndFeel (&lf);
        kaos.setSize (1200, 130);
        // En sågtand på 110 Hz där en bit hackas och en bit söndras, som Kaosmotorn gör.
        std::vector<float> wave (4096);
        for (size_t i = 0; i < wave.size(); ++i)
        {
            const double t = (double) i / 48000.0;
            float x = (float) (2.0 * std::fmod (t * 110.0, 1.0) - 1.0) * 0.6f;
            if (i > 1800 && i < 2600) x = wave[1000 + (i - 1800) % 200];
            if (i > 2600 && i < 3400) x = std::round (wave[(i / 12) * 12 - 1200] * 4.0f) / 4.0f;
            wave[i] = x;
        }
        kaos.scope.setSamples (wave.data() + 1000, 3000);
        kaos.lamp.flash();
        const auto img = kaos.createComponentSnapshot (kaos.getLocalBounds(), true, 1.0f);
        kaos.setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 2.0f;

    krang::ui::KrangLookAndFeel lf;
    krang::ui::KrangTopPanel panel;
    panel.setLookAndFeel (&lf);
    panel.setSize (900, 230);

    panel.katedral.slider.setRange (0.0, 100.0);
    panel.katedral.slider.setValue (15.0);
    panel.katedral.slider.setTextValueSuffix (" %");
    panel.katedral.slider.setNumDecimalPlacesToDisplay (0);
    panel.master.slider.setRange (-48.0, 6.0);
    panel.master.slider.setValue (0.0);
    panel.master.slider.setTextValueSuffix (" dB");
    panel.master.slider.setNumDecimalPlacesToDisplay (1);
    panel.drive.slider.setRange (0.0, 48.0);
    panel.drive.slider.setValue (18.0);
    panel.drive.slider.setTextValueSuffix (" dB");
    panel.drive.slider.setNumDecimalPlacesToDisplay (1);

    panel.display.setLines (juce::String::fromUTF8 ("Ångvisslan"), "DRIVE  18.0 DB");
    panel.meter.setLevelImmediate (krang::ui::dbToGain (-4.0f));

    const auto image = panel.createComponentSnapshot (panel.getLocalBounds(), true, scale);
    panel.setLookAndFeel (nullptr);

    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream)) { std::cerr << "Kunde inte skriva " << argv[1] << "\n"; return 2; }
    std::cout << "Skrev " << out.getFullPathName() << " (" << image.getWidth() << "x" << image.getHeight() << ")\n";

    // Valfritt: Stegverkets panel med startmönstret och steg 4 tänt.
    if (argc > 3)
    {
        krang::StepPattern pattern;
        pattern.loadDefault();
        std::atomic<int> playing { 4 };
        krang::ui::StegverketPanel steg (pattern, playing);
        steg.length.addItemList ({ "16", "32", "48", "64" }, 1);
        steg.length.setSelectedId (2);
        steg.tempo.setRange (40.0, 240.0, 1.0);
        steg.tempo.setValue (96.0);
        steg.swing.setRange (0.0, 50.0, 1.0);
        steg.swing.setValue (12.0);
        steg.setSize (900, 300);
        const auto img = steg.createComponentSnapshot (steg.getLocalBounds(), true, scale);
        juce::File f2 = juce::File::getCurrentWorkingDirectory().getChildFile (argv[3]);
        f2.deleteFile();
        juce::FileOutputStream s2 (f2);
        if (! s2.openedOk() || ! png.writeImageToStream (img, s2)) return 2;
        std::cout << "Skrev " << f2.getFullPathName() << "\n";
    }
    // Rittid: hur lång tid en hel omritning av panelerna tar (mjukvarurendering, designstorlek).
    if (argc > 4 && juce::String (argv[4]) == "--tid")
    {
        {
            // Första öppningen: alla plåtar räknas fram (åldrade ytor), därefter ligger de i cachen.
            krang::ui::KrangTopPanel top; top.setLookAndFeel (&lf); top.setSize (1200, 230);
            krang::ui::BasmalenPanel b; b.setLookAndFeel (&lf); b.setSize (1200, 320);
            krang::ui::AngledningenPanel a; a.setLookAndFeel (&lf); a.setSize (1200, 190);
            krang::ui::KaosPanel k; k.setLookAndFeel (&lf); k.setSize (1200, 130);
            juce::Image canvas (juce::Image::ARGB, 1200, 970, true);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            { juce::Graphics g (canvas); top.paintEntireComponent (g, true); b.paintEntireComponent (g, true);
              a.paintEntireComponent (g, true); k.paintEntireComponent (g, true); }
            std::cout << "Forsta oppningen (alla platar framraknade): " << juce::Time::getMillisecondCounterHiRes() - t0 << " ms\n";
            for (auto* c : std::initializer_list<juce::Component*> { &top, &b, &a, &k }) c->setLookAndFeel (nullptr);
        }
        krang::ui::BasmalenPanel bas;   bas.setLookAndFeel (&lf); bas.setSize (1200, 320);
        krang::ui::AngledningenPanel ang; ang.setLookAndFeel (&lf); ang.setSize (1200, 190);
        panel.setLookAndFeel (&lf); panel.setSize (1200, 230);
        juce::Image canvas (juce::Image::ARGB, 1200, 850, true);
        { juce::Graphics g (canvas); panel.paintEntireComponent (g, true); bas.paintEntireComponent (g, true); } // värm upp texturerna
        constexpr int runs = 30;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < runs; ++i)
        {
            juce::Graphics g (canvas);
            panel.paintEntireComponent (g, true);
            g.setOrigin (0, 230); bas.paintEntireComponent (g, true);
            g.setOrigin (0, 320); ang.paintEntireComponent (g, true);
        }
        std::cout << "Hel omritning: " << (juce::Time::getMillisecondCounterHiRes() - start) / runs << " ms\n";

        // Kaosskopet ritas om 30 gånger i sekunden; det måste vara billigt.
        krang::ui::Kaosskop scope;
        scope.setSize (640, 90);
        std::vector<float> wave (4096);
        for (size_t i = 0; i < wave.size(); ++i) wave[i] = (float) std::sin ((double) i * 0.03);
        const auto s0 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < runs; ++i)
        {
            scope.setSamples (wave.data(), (int) wave.size());
            juce::Graphics g (canvas);
            scope.paintEntireComponent (g, true);
        }
        std::cout << "Kaosskopet: " << (juce::Time::getMillisecondCounterHiRes() - s0) / runs << " ms per bild\n";
        bas.setLookAndFeel (nullptr); ang.setLookAndFeel (nullptr); panel.setLookAndFeel (nullptr);
    }
    return 0;
}
