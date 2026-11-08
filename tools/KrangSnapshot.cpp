// KrangSnapshot – ritar Krang-stilprovet till PNG utan att starta en DAW.
//
//   KrangSnapshot <fil.png> [skala] [steg-fil.png] [--tid]
//   KrangSnapshot --kaos <fil.png>   (Kaosavdelningen med en hackad våg i Krangskopet)
//   KrangSnapshot --modulation <fil.png>   (Modulationsraden med tre kopplingar och tända lampor)
//   KrangSnapshot --kammare <orgel|speldosa|automat|fagel|ur|tuba> <fil.png> [tratt 0–2]   (de tre nyaste kamrarna; fagel och ur = Automatens leksaker)
//   KrangSnapshot --spolslingan <fil.png> [pd]   (Spolslingan med FM-spolen eller Fasvridaren)
//   KrangSnapshot --hammarverket <fil.png> [läge 0–4]   (Hammarverket med kit, chans, ratchets och lås)
//   KrangSnapshot --tonsteg <fil.png> [skala] [sida]   (Tonstegverket med startmelodin; med sida: STEG 16 och den sidan öppen)
//   KrangSnapshot --klicktest   (hela panelen: drar varje ratt och fader, dubbelklickar, stegar varje lista, trycker
//                                knappar, spakar och tangenter i alla kammare och vyer, och kontrollerar parametrarna; OK/FEL)
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/KrangStyle.h"
#include "ui/StegverketPanel.h"
#include "ui/Panels.h"
#include "ui/ModulationPanel.h"
#include "ui/TonStegverketPanel.h"
#include "ui/ValueFormat.h"
#include "ui/Jugend.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>
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
    void slider (juce::AudioProcessorValueTreeState& st, const juce::String& pid, juce::Slider& s)
    {
        sliders.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, pid, s));
        if (auto* param = st.getParameter (pid))
        {
            s.textFromValueFunction = [param] (double v) { return krang::ui::formatValue (*param, (float) v); };
            s.updateText();
        }
    }
    void combo (juce::AudioProcessorValueTreeState& st, const juce::String& pid, juce::ComboBox& c)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (pid))) { c.clear (juce::dontSendNotification); c.addItemList (choice->choices, 1); }
        combos.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, pid, c));
    }
};
}


// --- Klicktestet ---------------------------------------------------------------------------------
// Öppnar den riktiga panelen med den riktiga processorn och klickar sig igenom allt, som en människa
// skulle göra: i varje kammare och i vyerna STEGVERKET, MODULATION och KAOS. Varje ratt och fader dras
// och dubbelklickas, varje lista stegas genom alla sina val (och det som då visas testas också), varje
// knapp och spak trycks, och keyboardet spelas. Till sist: alla parametrar som ska nås från panelen.
namespace klick
{
    struct Recorder : juce::AudioProcessorParameter::Listener
    {
        std::set<int> changed, ever;
        void parameterValueChanged (int index, float) override { changed.insert (index); ever.insert (index); }
        void parameterGestureChanged (int, bool) override {}
    };

    juce::MouseEvent event (juce::Component& c, juce::Point<float> pos, juce::Point<float> downPos, int clicks, bool dragged)
    {
        auto src = juce::Desktop::getInstance().getMainMouseSource();
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent (src, pos, juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier), 1.0f,
                                 juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                                 juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY,
                                 &c, &c, now, downPos, now, clicks, dragged);
    }

    bool showing (juce::Component& c)
    {
        // Panelen ligger inte på skärmen här: allt under editorn räknas, editorn själv är roten.
        for (auto* p = &c; p != nullptr && dynamic_cast<juce::AudioProcessorEditor*> (p) == nullptr; p = p->getParentComponent())
            if (! p->isVisible()) return false;
        return true;
    }

    template <typename T>
    void collect (juce::Component& root, std::vector<T*>& out, bool includeHidden = false)
    {
        for (auto* child : root.getChildren())
        {
            if (auto* t = dynamic_cast<T*> (child))
                if (includeHidden || showing (*t)) out.push_back (t);
            collect (*child, out, includeHidden);
        }
    }

    // Var reglaget sitter: närmaste panel med namn (klassnamnet) och läget i den.
    juce::String where (juce::Component& c)
    {
        juce::Component* panel = c.getParentComponent();
        while (panel != nullptr && panel->getParentComponent() != nullptr
               && dynamic_cast<juce::AudioProcessorEditor*> (panel->getParentComponent()->getParentComponent()) == nullptr)
            panel = panel->getParentComponent();
        juce::String cls = panel != nullptr ? juce::String (typeid (*panel).name()) : juce::String ("?");
        cls = cls.fromLastOccurrenceOf ("ui", false, false).trimCharactersAtStart ("0123456789").upToFirstOccurrenceOf ("E", false, false);
        const auto pos = panel != nullptr ? panel->getLocalPoint (&c, c.getLocalBounds().getCentre()) : c.getPosition();
        return cls + " @" + juce::String (pos.x) + "," + juce::String (pos.y);
    }
}

int runKlicktest()
{
    using namespace juce;
    std::cout << std::unitbuf;
    KrangBoxProcessor processor;
    processor.prepareToPlay (48000.0, 256);
    std::unique_ptr<AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (1200, 840);
    auto& params = processor.getParameters();
    klick::Recorder rec;
    for (auto* p : params) p->addListener (&rec);
    auto pid = [&] (int i) { if (auto* r = dynamic_cast<RangedAudioParameter*> (params[i])) return r->getParameterID(); return String (i); };

    int ok = 0, fel = 0;
    auto check = [&] (bool good, const String& what)
    {
        (good ? ok : fel)++;
        if (! good) std::cout << what << "  FEL\n";
    };

    std::set<Component*> tested;
    std::set<String> sliderParams, comboParams;

    auto testSlider = [&] (Slider& s)
    {
        if (! s.isEnabled()) return;
        s.setPopupDisplayEnabled (false, false, nullptr);   // värderutan är ett eget fönster; här finns ingen skärm
        const auto b = s.getLocalBounds().toFloat();
        const auto c = b.getCentre();
        rec.changed.clear();
        const double before = s.getValue();
        s.mouseDown (klick::event (s, c, c, 1, false));
        for (int k = 1; k <= 6; ++k)
        {
            const auto p = c + Point<float> (8.0f * (float) k, -10.0f * (float) k);
            s.mouseDrag (klick::event (s, p, c, 1, true));
        }
        s.mouseUp (klick::event (s, c + Point<float> (48.0f, -60.0f), c, 1, true));
        bool moved = std::abs (s.getValue() - before) > 1.0e-9;
        if (! moved)   // stod redan i toppen: dra åt andra hållet
        {
            s.mouseDown (klick::event (s, c, c, 1, false));
            for (int k = 1; k <= 6; ++k) s.mouseDrag (klick::event (s, c + Point<float> (-8.0f * (float) k, 10.0f * (float) k), c, 1, true));
            s.mouseUp (klick::event (s, c, c, 1, true));
            moved = std::abs (s.getValue() - before) > 1.0e-9;
        }
        const auto name = klick::where (s);
        check (moved, "ratt " + name + ": rör sig inte när man drar");
        check (rec.changed.size() == 1, "ratt " + name + ": ändrar " + String ((int) rec.changed.size()) + " parametrar (ska vara 1)");
        if (rec.changed.size() != 1) return;
        const int index = *rec.changed.begin();
        sliderParams.insert (pid (index));
        // Dubbelklick ska ge parameterns standardvärde.
        s.mouseDoubleClick (klick::event (s, c, c, 2, false));
        auto* rp = dynamic_cast<RangedAudioParameter*> (params[index]);
        const bool atDefault = rp != nullptr && std::abs (rp->getValue() - rp->getDefaultValue()) < 0.002f;
        check (atDefault, "ratt " + name + " (" + pid (index) + "): dubbelklick ger inte standardvärdet");
    };

    auto testToggle = [&] (Component& t)
    {
        rec.changed.clear();
        const auto b = t.getLocalBounds().toFloat();
        for (float y : { b.getBottom() - 3.0f, b.getY() + 3.0f })   // nedre läget och tillbaka till det övre
        {
            const Point<float> p (b.getCentreX(), y);
            t.mouseDown (klick::event (t, p, p, 1, false));
            t.mouseUp (klick::event (t, p, p, 1, false));
        }
        check (! rec.changed.empty(), "spak " + klick::where (t) + ": ändrar ingen parameter");
        for (int i : rec.changed) comboParams.insert (pid (i));
    };

    std::function<void (Component&)> sweep;
    auto testCombo = [&] (ComboBox& cb, bool hidden)
    {
        const int n = cb.getNumItems();
        if (n < 2) return;
        const int original = cb.getSelectedItemIndex();
        rec.changed.clear();
        std::set<int> touched;
        for (int i = 0; i < n; ++i)
        {
            cb.setSelectedItemIndex (i, sendNotificationSync);
            for (int c : rec.changed) touched.insert (c);
            rec.changed.clear();
            if (! hidden) sweep (*editor);   // det som visas med det här valet (t.ex. Fasvridarens rattar)
        }
        cb.setSelectedItemIndex (jmax (0, original), sendNotificationSync);
        rec.changed.clear();
        const auto name = klick::where (cb) + " (" + cb.getItemText (0) + " …)";
        check (! touched.empty(), "lista " + name + ": ändrar ingen parameter");
        for (int i : touched) comboParams.insert (pid (i));
    };

    sweep = [&] (Component& root)
    {
        for (int pass = 0; pass < 3; ++pass)
        {
            std::vector<Slider*> sliders;   klick::collect (root, sliders);
            std::vector<ComboBox*> combos;  klick::collect (root, combos, true);
            std::vector<krang::ui::KrangToggle*> toggles; klick::collect (root, toggles);
            bool any = false;
            for (auto* s : sliders)
                if (tested.insert (s).second) { testSlider (*s); any = true; }
            for (auto* t : toggles)
                if (tested.insert (t).second) { testToggle (*t); any = true; }
            for (auto* cb : combos)
            {
                // Dolda listor räknas bara när föräldern syns: de nås via en meny (Hammarverkets ljud per spår).
                const bool hidden = ! klick::showing (*cb);
                if (hidden && (cb->getParentComponent() == nullptr || ! klick::showing (*cb->getParentComponent()))) continue;
                if (tested.insert (cb).second) { testCombo (*cb, hidden); any = true; }
            }
            if (! any) break;
        }
    };

    // Vyknapparna och kammarväljaren hittas efter text och innehåll, inte efter namn i koden.
    std::vector<TextButton*> buttons; klick::collect (*editor, buttons, true);
    auto button = [&] (const String& text) -> TextButton*
    {
        for (auto* b : buttons) if (b->getButtonText().startsWith (text)) return b;
        return nullptr;
    };
    std::vector<ComboBox*> allCombos; klick::collect (*editor, allCombos, true);
    ComboBox* chamberBox = nullptr;
    for (auto* cb : allCombos) if (cb->getNumItems() >= 15 && cb->getItemText (14) == "Krigstuban") chamberBox = cb;
    check (chamberBox != nullptr, "kammarväljaren hittas inte");
    if (chamberBox == nullptr) return 2;
    tested.insert (chamberBox);
    auto* kammare = button ("KAMMARE");
    // Ett knapptryck som en människa: växlar läget (för av/på- och vyknappar) och skickar klicket direkt.
    auto press = [] (Button& b)
    {
        if (b.getClickingTogglesState()) b.setToggleState (! b.getToggleState() || b.getRadioGroupId() != 0, sendNotificationSync);
        else b.triggerClick();
    };
    for (const char* v : { "KAMMARE", "STEGVERKET", "MODULATION", "KAOS" })
        check (button (v) != nullptr, String ("vyknappen ") + v + " hittas inte");

    for (int chamber = 0; chamber < 15; ++chamber)
    {
        chamberBox->setSelectedItemIndex (chamber, sendNotificationSync);
        if (kammare != nullptr) press (*kammare);
        for (int i = 0; i < 3; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
        const int before = ok + fel;
        sweep (*editor);
        // STEGVERKET hör till de melodiska kamrarna, så den testas i Basmålen.
        if (chamber == 0)
            if (auto* b = button ("STEGVERKET")) { press (*b); MessageManager::getInstance()->runDispatchLoopUntil (40); sweep (*editor); }
        std::cout << "kammare " << chamberBox->getItemText (chamber) << ": " << (ok + fel - before) << " kontroller\n";
    }
    for (const char* v : { "MODULATION", "KAOS" })
        if (auto* b = button (v))
        {
            press (*b);
            MessageManager::getInstance()->runDispatchLoopUntil (40);
            const int before = ok + fel;
            sweep (*editor);
            std::cout << "vyn " << v << ": " << (ok + fel - before) << " kontroller\n";
        }
    if (kammare != nullptr) press (*kammare);

    // Knapparna med av/på (SPELA i Stegverken).
    for (auto* b : buttons)
        if (b->getClickingTogglesState() && b->getRadioGroupId() == 0)
        {
            rec.changed.clear();
            press (*b); press (*b);
            check (! rec.changed.empty(), "knappen " + b->getButtonText() + ": ändrar ingen parameter");
            for (int i : rec.changed) comboParams.insert (pid (i));
        }

    // Keyboardet: det ska finnas, spela på processorns tangentbordsläge, och en ton därifrån ska höras.
    // (Att klicka på tangenterna kräver en skärm och provas på Macen.)
    {
        std::vector<MidiKeyboardComponent*> kbs; klick::collect (*editor, kbs);
        check (kbs.size() == 1, "keyboardet hittas inte");
        if (auto* chamberParam = processor.state.getParameter ("chamber")) chamberParam->setValueNotifyingHost (0.0f);
        AudioBuffer<float> buf (2, 256);
        MidiBuffer midi;
        float peak = 0.0f;
        processor.keyboardState.noteOn (1, 48, 0.8f);
        for (int b = 0; b < 40; ++b) { buf.clear(); midi.clear(); processor.processBlock (buf, midi); peak = jmax (peak, buf.getMagnitude (0, 256)); }
        processor.keyboardState.noteOff (1, 48, 0.0f);
        check (peak > 0.01f, "keyboardet: en ton från tangentbordet hörs inte");
    }

    // Täckningen: alla parametrar ska gå att nå från panelen (grind 3).
    StringArray missing;
    for (int i = 0; i < params.size(); ++i)
        if (rec.ever.count (i) == 0) missing.add (pid (i));
    check (missing.isEmpty(), "parametrar som inte nås från panelen: " + missing.joinIntoString (", "));
    std::cout << "\nreglage provade: " << (int) tested.size() << ", parametrar nådda: " << (int) rec.ever.size()
              << " av " << params.size() << "\n";
    std::cout << "KrangSnapshot --klicktest: " << (ok + fel) << " kontroller, " << ok << " OK, " << fel << " FEL\n";
    for (auto* p : params) p->removeListener (&rec);
    editor.reset();
    return fel == 0 ? 0 : 2;
}

int main (int argc, char* argv[])
{
    if (argc < 2) { std::cerr << "Användning: KrangSnapshot <fil.png> [skala]\n"; return 1; }
    juce::ScopedJuceInitialiser_GUI gui;

    if (juce::String (argv[1]) == "--klicktest")
        return runKlicktest();

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

    // KrangSnapshot --kammare <orgel|speldosa|automat> <fil.png>: en av de tre nyaste kamrarnas paneler.
    // KrangSnapshot --jugend <fil.png>: alla jugendillustrationer i alla lägen, i panelernas storlekar.
    if (juce::String (argv[1]) == "--jugend" && argc > 2)
    {
        namespace J = krang::ui::jugend;
        juce::Image img (juce::Image::RGB, 1420, 1330, true);
        juce::Graphics g (img);
        g.fillAll (juce::Colour (0xff1d1c1b));
        auto cell = [] (int row, int col, float w, float h) { return juce::Rectangle<float> (10.0f + col * 282.0f, 10.0f + row * 264.0f, w, h); };
        for (int t = 0; t < 4; ++t) J::lur (g, cell (0, t, 156, 246), t, t == 3 ? 0.6f : 0.0f);
        for (int t = 0; t < 4; ++t) J::lutInstrument (g, cell (1, t, 180, 246), t, t == 3 ? 0.6f : 0.1f, 0.3f);
        for (int t = 0; t < 5; ++t) J::bell (g, cell (2, t, 272, 242), t, t == 0 ? 0.5f : 0.0f);
        for (int t = 0; t < 4; ++t) J::pump (g, cell (3, t, 206, 246), t, 0.6f, t == 2 ? 0.5f : 0.0f, 0.5f);
        J::tuba (g, cell (4, 0, 264, 256), 0, 1, 0.3f);
        J::tuba (g, cell (4, 1, 264, 256), 0, 3, 0.8f);
        J::tuba (g, cell (4, 2, 264, 256), 1, 2, 0.5f);
        J::tuba (g, cell (4, 3, 264, 256), 2, 1, 0.5f);
        J::tuba (g, cell (4, 4, 264, 256), 2, 4, 1.0f);
        if (argc > 3)   // de nya: Basmålen, Bordunorgeln och Spolslingan
        {
            g.fillAll (juce::Colour (0xff1d1c1b));
            J::mill (g, cell (0, 0, 140, 280), 0.6f);
            J::coil (g, cell (0, 1, 110, 272), false, 0.8f);
            J::coil (g, cell (0, 2, 110, 272), true, 0.2f);
            for (int t = 0; t < 5; ++t) J::bordunInstrument (g, cell (1 + t / 3, t % 3, 170, 256), t, 0.6f);
            J::musicBox (g, juce::Rectangle<float> (300, 10, 450, 256), 0, 0.1f, 0.2f);
            J::musicBox (g, juce::Rectangle<float> (300, 280, 450, 256), 1, 0.7f, 0.8f);
            J::musicBox (g, juce::Rectangle<float> (860, 10, 450, 256), 2, 0.0f, 0.5f);
        }
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--kammare" && argc > 3)
    {
        SnapshotHost host;
        krang::ui::KrangLookAndFeel lf;
        const juce::String which (argv[2]);
        std::unique_ptr<juce::Component> panel;
        krang::ui::Bindings b;
        if (which == "orgel")         { auto* p = new krang::ui::AngorgelnPanel(); b = p->bindings(); panel.reset (p); }
        else if (which == "speldosa") { auto* p = new krang::ui::SpeldosanPanel(); b = p->bindings(); panel.reset (p); }
        else if (which == "tuba")     { auto* p = new krang::ui::KrigstubanPanel(); b = p->bindings(); panel.reset (p); }
        else if (which == "kor")      { auto* p = new krang::ui::KorenPanel(); b = p->bindings(); panel.reset (p); }
        else                          { auto* p = new krang::ui::AutomatenPanel(); b = p->bindings(); panel.reset (p); }
        const float fig = which == "fagel" ? 1.0f : which == "ur" ? 2.0f : 0.0f;
        for (auto [pid, value] : { std::pair<const char*, float> { "agLeak", 0.5f }, { "sdRust", 0.4f }, { "sdSpring", 0.7f }, { "atBroken", 0.5f },
                                   { "atFigure", fig }, { "atBird", 1.0f }, { "atClock", 2.0f },
                                   { "ktEnsemble", 0.8f }, { "koSpeaker", 0.6f }, { "ktPressure", 0.8f }, { "ktBell", (float) (argc > 4 ? juce::String (argv[4]).getIntValue() : 0) } })
            if (auto* p = host.state.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value));
        panel->setLookAndFeel (&lf);
        panel->setSize (1200, 320);
        Attach a;
        for (auto& [slider, pid] : b.sliders) a.slider (host.state, pid, *slider);
        for (auto& [combo, pid] : b.combos) a.combo (host.state, pid, *combo);
        if (auto* ap = dynamic_cast<krang::ui::AutomatenPanel*> (panel.get()))
            ap->setSpokenWord (fig > 0.0f ? 100 + (int) fig : 11);
        const auto img = panel->createComponentSnapshot (panel->getLocalBounds(), true, 1.0f);
        a = {};
        panel->setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[3]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--spolslingan" && argc > 2)
    {
        SnapshotHost host;
        const bool pd = argc > 3 && juce::String (argv[3]) == "pd";
        for (auto [pid, value] : { std::pair<const char*, float> { "chamber", 3.0f }, { "spEngine", pd ? 1.0f : 0.0f },
                                   { "spPdWave", 4.0f }, { "spArp", 8.0f }, { "spArpLatch", 1.0f }, { "spPdDetune", 12.0f } })
            if (auto* p = host.state.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value));
        krang::ui::KrangLookAndFeel lf;
        krang::ui::SpolslinganPanel panel;
        panel.setLookAndFeel (&lf);
        panel.setSize (1200, 320);
        Attach a;
        const auto b = panel.bindings();
        for (auto& [slider, pid] : b.sliders) a.slider (host.state, pid, *slider);
        for (auto& [combo, pid] : b.combos) a.combo (host.state, pid, *combo);
        const auto img = panel.createComponentSnapshot (panel.getLocalBounds(), true, 1.0f);
        a = {};
        panel.setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
    }

    if (juce::String (argv[1]) == "--hammarverket" && argc > 2)
    {
        SnapshotHost host;
        krang::StepPattern pattern;
        pattern.fromString ("2000002000200000/0000000000000100/00002{r1}000000020{c2}00/0000000000002000/0001{t-5}0001{t+7}0000001{d6}/1010101{r3}1{c1}01{c2}01{c3}00/0000000000000010/0000010001010000");
        std::atomic<int> playing { 5 };
        const int mode = argc > 3 ? juce::String (argv[3]).getIntValue() : 0;
        for (auto [pid, value] : { std::pair<const char*, float> { "hv1Sound", 8.0f }, { "hv3Sound", 10.0f }, { "hv4Sound", 9.0f } })
            if (auto* p = host.state.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value));
        krang::ui::KrangLookAndFeel lf;
        krang::ui::HammarverketPanel panel (pattern, playing);
        panel.setLookAndFeel (&lf);
        panel.setSize (1200, 320);
        Attach a;
        const auto b = panel.bindings();
        for (auto& [slider, pid] : b.sliders) a.slider (host.state, pid, *slider);
        for (auto& [combo, pid] : b.combos) a.combo (host.state, pid, *combo);
        a.combo (host.state, "seqLength", panel.steg.length);
        panel.steg.setMode (mode);
        const auto img = panel.createComponentSnapshot (panel.getLocalBounds(), true, 1.0f);
        a = {};
        panel.setLookAndFeel (nullptr);
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream st (f);
        juce::PNGImageFormat png;
        return st.openedOk() && png.writeImageToStream (img, st) ? 0 : 2;
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
        kaos.setSize (1200, 320);
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

        // Krangskopet ritas om 30 gånger i sekunden; det måste vara billigt.
        krang::ui::Krangskop scope;
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
        std::cout << "Krangskopet: " << (juce::Time::getMillisecondCounterHiRes() - s0) / runs << " ms per bild\n";
        bas.setLookAndFeel (nullptr); ang.setLookAndFeel (nullptr); panel.setLookAndFeel (nullptr);
    }
    return 0;
}
