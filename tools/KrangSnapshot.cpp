// KrangSnapshot – ritar Krang-stilprovet till PNG utan att starta en DAW.
//
//   KrangSnapshot <fil.png> [skala] [steg-fil.png]
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/KrangStyle.h"
#include "ui/StegverketPanel.h"
#include <iostream>

int main (int argc, char* argv[])
{
    if (argc < 2) { std::cerr << "Användning: KrangSnapshot <fil.png> [skala]\n"; return 1; }
    juce::ScopedJuceInitialiser_GUI gui;
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
    return 0;
}
