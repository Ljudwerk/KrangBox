// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

namespace
{
    // Höjder i designstorlek (bredd 1200).
    constexpr int topHeight = 230, chamberHeight = 320, angledningHeight = 190, kaosHeight = 130, keyboardHeight = 100;
    static_assert (topHeight + chamberHeight + angledningHeight + kaosHeight + keyboardHeight == KrangBoxEditor::designHeight);
}

KrangBoxEditor::KrangBoxEditor (KrangBoxProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      hammarverket (p.pattern, p.stegverket.currentStep),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace krang::ui;
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (content);
    for (auto* c : std::initializer_list<juce::Component*> { &top, &basmalen, &angledningen, &kaos, &keyboard,
                                                             &octaveDown, &octaveUp, &keyboardHint })
        content.addAndMakeVisible (c);
    content.addChildComponent (hammarverket);
    content.addChildComponent (bordunorgeln);
    content.addChildComponent (spolslingan);
    content.addChildComponent (stalpiano);
    content.addChildComponent (klockor);
    content.addChildComponent (koren);
    content.addChildComponent (radio);
    content.addChildComponent (lur);
    content.addChildComponent (lut);

    bindAll();

    // Presets
    fillPresetBox();
    top.presetBox.onChange = [this]
    {
        const int index = top.presetBox.getSelectedId() - 1;
        if (index >= 0 && processor.presets.load (index))
            hammarverket.repaint();
    };
    top.prevPreset.onClick = [this] { processor.presets.loadNext (-1); hammarverket.repaint(); };
    top.nextPreset.onClick = [this] { processor.presets.loadNext (+1); hammarverket.repaint(); };
    top.savePreset.onClick = [this] { showSaveDialog(); };

    // Keyboard: C1–C7, datorns tangentbord (A W S E D ...) spelar från C2 – bra för bas.
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (24);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setVelocity (0.8f, true);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, colours::meterFace);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, colours::bakelite);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, colours::brassDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::tubeOrange.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::brass.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setWantsKeyboardFocus (true);
    octaveDown.onClick = [this] { shiftOctave (-1); };
    octaveUp.onClick   = [this] { shiftOctave (+1); };
    keyboardHint.setColour (juce::Label::textColourId, colours::brass);
    keyboardHint.setFont (panelFont (11.0f, false));
    keyboardHint.setJustificationType (juce::Justification::centredLeft);
    shiftOctave (0);

    layoutContent();

    // Proportionell storleksändring, 60–160 % av designstorleken; startar på 85 % så att den ryms på en 13-tums skärm.
    setResizable (true, true);
    setResizeLimits (designWidth * 6 / 10, designHeight * 6 / 10, designWidth * 16 / 10, designHeight * 16 / 10);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designWidth / designHeight);
    setSize (designWidth * 85 / 100, designHeight * 85 / 100);

    startTimerHz (30);
    timerCallback();
}

KrangBoxEditor::~KrangBoxEditor()
{
    setLookAndFeel (nullptr);
}

// --- Parametrar --------------------------------------------------------------

void KrangBoxEditor::bindAll()
{
    namespace id = krang::params::id;

    // Frontpanelen: kammare och de tre stora rattarna, som gäller alla kamrar.
    bindCombo (top.chamber, id::chamber);
    bindSlider (top.drive.slider, id::distDrive);
    bindSlider (top.katedral.slider, id::rvbMix);
    bindSlider (top.master.slider, id::master);
    bindSlider (top.lever.slider, id::krang);

    for (auto bindings : { basmalen.bindings(), hammarverket.bindings(), bordunorgeln.bindings(), spolslingan.bindings(), stalpiano.bindings(), klockor.bindings(), koren.bindings(), radio.bindings(), lur.bindings(), lut.bindings(), angledningen.bindings(), kaos.bindings() })
    {
        for (auto& [slider, paramId] : bindings.sliders) bindSlider (*slider, paramId);
        for (auto& [combo, paramId] : bindings.combos)   bindCombo (*combo, paramId);
    }

    // Stegverket
    auto& steg = hammarverket.steg;
    bindCombo (steg.length, id::seqLength);
    bindSlider (steg.tempo, id::seqTempo);
    bindSlider (steg.swing, id::seqSwing);
    seqRunAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.state, id::seqRun, steg.play);
}

void KrangBoxEditor::bindSlider (juce::Slider& slider, const juce::String& paramId)
{
    auto* param = processor.state.getParameter (paramId);
    jassert (param != nullptr);
    if (param == nullptr) return;

    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.state, paramId, slider));

    // Visa värdet med enhet ("400 Hz", "0.250 s"), och låt displayen följa den ratt man vrider på.
    slider.textFromValueFunction = [param] (double v)
    {
        auto text = param->getText (param->convertTo0to1 ((float) v), 0);
        const auto unit = param->getLabel();
        return unit.isEmpty() ? text : text + " " + unit;
    };
    slider.updateText();
    slider.onDragStart = [this, paramId] { lastParam = paramId; };
}

void KrangBoxEditor::bindCombo (juce::ComboBox& combo, const juce::String& paramId)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.state.getParameter (paramId)))
    {
        combo.clear (juce::dontSendNotification);
        combo.addItemList (choice->choices, 1);
        comboAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.state, paramId, combo));
        // Behåll panelens egen onChange (t.ex. strängbädden som ritas om) och visa parametern i displayen.
        combo.onChange = [this, paramId, previous = combo.onChange]
        {
            lastParam = paramId;
            if (previous) previous();
        };
    }
}

// --- Layout --------------------------------------------------------------------

void KrangBoxEditor::paint (juce::Graphics& g)
{
    g.fillAll (krang::ui::colours::chassis); // syns bara under skalningen; innehållet täcker allt
}

void KrangBoxEditor::layoutContent()
{
    content.setBounds (0, 0, designWidth, designHeight);
    auto r = content.getLocalBounds();
    top.setBounds (r.removeFromTop (topHeight));
    const auto chamberArea = r.removeFromTop (chamberHeight);
    basmalen.setBounds (chamberArea);
    hammarverket.setBounds (chamberArea);
    bordunorgeln.setBounds (chamberArea);
    spolslingan.setBounds (chamberArea);
    stalpiano.setBounds (chamberArea);
    klockor.setBounds (chamberArea);
    koren.setBounds (chamberArea);
    radio.setBounds (chamberArea);
    lur.setBounds (chamberArea);
    lut.setBounds (chamberArea);
    angledningen.setBounds (r.removeFromTop (angledningHeight));
    kaos.setBounds (r.removeFromTop (kaosHeight));

    auto kb = r.reduced (12, 6);
    auto bar = kb.removeFromTop (26);
    octaveDown.setBounds (bar.removeFromLeft (64));
    bar.removeFromLeft (6);
    octaveUp.setBounds (bar.removeFromLeft (64));
    bar.removeFromLeft (12);
    keyboardHint.setBounds (bar);
    kb.removeFromTop (4);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 43.0f); // C1–C7 = 43 vita tangenter, fyller hela bredden
}

void KrangBoxEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    content.setTransform (juce::AffineTransform::scale (scale));
}

// --- Keyboard, display och presets ------------------------------------------------

void KrangBoxEditor::shiftOctave (int delta)
{
    keyOctave = juce::jlimit (1, 7, keyOctave + delta);
    keyboard.setKeyPressBaseOctave (keyOctave);
    const auto lowest = juce::MidiMessage::getMidiNoteName (keyOctave * 12, true, true, 4);
    keyboardHint.setText (juce::String::fromUTF8 ("Spela med musen, eller klicka på keyboardet och använd datorns tangenter "
                                                  "A W S E D F T G Y H U J K (från ") + lowest + ").",
                          juce::dontSendNotification);
}

void KrangBoxEditor::showParameter (const juce::String& paramId)
{
    if (auto* param = processor.state.getParameter (paramId))
        top.display.setLines (processor.presets.getCurrentName(),
                              param->getName (24) + "  " + param->getCurrentValueAsText() + " " + param->getLabel());
}

void KrangBoxEditor::fillPresetBox()
{
    auto& box = top.presetBox;
    box.clear (juce::dontSendNotification);
    juce::String category;
    const auto& entries = processor.presets.getEntries();
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        if (entries[(size_t) i].category != category)
        {
            category = entries[(size_t) i].category;
            box.addSectionHeading (category);
        }
        box.addItem (entries[(size_t) i].name, i + 1);
    }
    box.setSelectedId (processor.presets.getCurrentIndex() + 1, juce::dontSendNotification);
    shownPreset = processor.presets.getCurrentName();
}

void KrangBoxEditor::showSaveDialog()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Spara preset", juce::String::fromUTF8 ("Namn på ljudet:"),
                                                      juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor ("name", processor.presets.getCurrentName());
    saveDialog->addButton ("SPARA", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("AVBRYT", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
        {
            const auto name = saveDialog->getTextEditorContents ("name");
            if (processor.presets.saveUser (name))
                fillPresetBox();
            else
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Spara preset",
                                                        juce::String::fromUTF8 ("Kunde inte spara presetet. Kontrollera namnet."));
        }
        saveDialog.reset();
    }), false);

    if (auto* editor = saveDialog->getTextEditor ("name"))
    {
        editor->grabKeyboardFocus();
        editor->selectAll();
    }
}

void KrangBoxEditor::timerCallback()
{
    top.meter.setLevel (processor.outputPeak.exchange (0.0f));

    // Röret glöder med drive och Krangspaken.
    const float drive = processor.state.getRawParameterValue (krang::params::id::distDrive)->load() / 48.0f;
    const float spak  = processor.state.getRawParameterValue (krang::params::id::krang)->load();
    top.tube.setGlow (0.15f + 0.6f * drive + 0.5f * spak);

    // Kaosskopet och kaoslampan.
    {
        auto& tap = processor.scope;
        const int w = tap.writeIndex.load (std::memory_order_acquire);
        constexpr int n = KrangBoxProcessor::ScopeTap::size;
        for (int i = 0; i < n; ++i)
            scopeCopy[(size_t) i] = tap.samples[(size_t) ((w + i) & (n - 1))].load (std::memory_order_relaxed);
        kaos.scope.setSamples (scopeCopy.data(), n);

        const auto events = processor.kaosEvents.load (std::memory_order_relaxed);
        if (events != shownKaosEvents) { shownKaosEvents = events; kaos.lamp.flash(); }
        else kaos.lamp.decay();
    }

    radio.setTunedNote (processor.lastNote.load (std::memory_order_relaxed));

    // Kammarmodulen följer kammarväljaren.
    const int chamber = (int) processor.state.getRawParameterValue (krang::params::id::chamber)->load();
    if (chamber != shownChamber)
    {
        shownChamber = chamber;
        basmalen.setVisible (chamber == 0);
        hammarverket.setVisible (chamber == 1);
        bordunorgeln.setVisible (chamber == 2);
        spolslingan.setVisible (chamber == 3);
        stalpiano.setVisible (chamber == 4);
        klockor.setVisible (chamber == 5);
        koren.setVisible (chamber == 6);
        radio.setVisible (chamber == 7);
        lur.setVisible (chamber == 8);
        lut.setVisible (chamber == 9);
    }

    // Presetbyte från DAW:en (programlistan) eller nästa/föregående: följ med i listan.
    if (processor.presets.getCurrentName() != shownPreset)
    {
        shownPreset = processor.presets.getCurrentName();
        top.presetBox.setSelectedId (processor.presets.getCurrentIndex() + 1, juce::dontSendNotification);
        hammarverket.repaint();
    }

    showParameter (lastParam);
}
