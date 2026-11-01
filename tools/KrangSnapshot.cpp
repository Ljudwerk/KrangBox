// KrangSnapshot – ritar Krang-stilprovet till PNG utan att starta en DAW.
//
//   KrangSnapshot <fil.png> [skala] [steg-fil.png] [--tid]
//   KrangSnapshot --kaos <fil.png>   (Kaosraden med en hackad våg i Kaosskopet)
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/KrangStyle.h"
#include "ui/StegverketPanel.h"
#include "ui/Panels.h"
#include <iostream>

int main (int argc, char* argv[])
{
    if (argc < 2) { std::cerr << "Användning: KrangSnapshot <fil.png> [skala]\n"; return 1; }
    juce::ScopedJuceInitialiser_GUI gui;

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
