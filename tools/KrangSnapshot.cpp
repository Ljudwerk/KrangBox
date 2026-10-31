// KrangSnapshot – ritar Krang-stilprovet till PNG utan att starta en DAW.
//
//   KrangSnapshot <fil.png> [skala]
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/KrangStyle.h"
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

    panel.cutoff.slider.setRange (20.0, 18000.0);
    panel.cutoff.slider.setSkewFactor (0.25);
    panel.cutoff.slider.setValue (400.0);
    panel.cutoff.slider.setTextValueSuffix (" Hz");
    panel.cutoff.slider.setNumDecimalPlacesToDisplay (0);
    panel.resonance.slider.setRange (0.0, 1.0);
    panel.resonance.slider.setValue (0.45);
    panel.resonance.slider.setNumDecimalPlacesToDisplay (2);
    panel.drive.slider.setRange (0.0, 48.0);
    panel.drive.slider.setValue (18.0);
    panel.drive.slider.setTextValueSuffix (" dB");
    panel.drive.slider.setNumDecimalPlacesToDisplay (1);

    panel.display.setLines ("BASMALEN  ROR", "CUTOFF  400 HZ");
    panel.meter.setLevelImmediate (krang::ui::dbToGain (-4.0f));

    const auto image = panel.createComponentSnapshot (panel.getLocalBounds(), true, scale);
    panel.setLookAndFeel (nullptr);

    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream)) { std::cerr << "Kunde inte skriva " << argv[1] << "\n"; return 2; }
    std::cout << "Skrev " << out.getFullPathName() << " (" << image.getWidth() << "x" << image.getHeight() << ")\n";
    return 0;
}
