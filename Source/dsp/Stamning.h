// Stämningen: hur tangenterna stäms i alla melodiska kammare (fas 3d).
//
//   Liksvävande  – vanliga tolv lika steg, som ett piano
//   Ren stämning – rena intervall räknade från grundtonen (5-gräns: ren ters 5/4, ren kvint 3/2 …):
//                  ackord i grundtonarten står stilla utan svävning, men långt bort från grundtonen skär det
//   Pytagoreisk  – allt byggt av rena kvinter (3/2): ljusa, höga terser, som medeltida sång och fiol i kvintstämning
//   Medeltonig   – kvartskommamedeltonig: rena terser och lite smalare kvinter, som gamla orglar
//   Naturtoner   – tonerna tagna ur övertonsserien ovanför grundtonen (16:e–31:a övertonen), som näverlurens och
//                  bordunens egna toner: naturseptiman (7/4) och den höga elvan (11/8)
// Ovanpå stämningen lägger Blå toner folkmusikens neutrala intervall: tersen och septiman dras mot mitten mellan moll
// och dur (som i svenska låtar och kulning), kvarten höjs en aning och sexten dras mot mitten.
// Grundtonen kan följa Bordunorgelns bordunton, så att skalan byggs på bordunen.
//
// Alla kammare frågar Stamning::hz() i stället för att räkna 440 · 2^((not − 69)/12) själva. Stämningen ändrar bara
// tonhöjden per tonklass: grundtonen ligger alltid kvar där den ligger i liksvävande stämning.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <array>
#include <cmath>

namespace krang
{

enum class TuningSystem { Liksvavande = 0, Ren, Pytagoreisk, Medeltonig, Naturtoner };

struct StamningParams
{
    TuningSystem system = TuningSystem::Liksvavande;
    int root = 0;          // tonklass 0 = C … 11 = H
    float blue = 0.0f;     // blå toner 0..1
    bool operator== (const StamningParams& o) const { return system == o.system && root == o.root && blue == o.blue; }
    bool operator!= (const StamningParams& o) const { return ! (*this == o); }
};

class Stamning
{
public:
    Stamning() { offsets.fill (0.0); }

    // En delad liksvävande stämning för kammare som inte fått någon egen (t.ex. i testverktyg).
    static const Stamning& neutral() { static const Stamning s; return s; }

    void setParams (const StamningParams& p)
    {
        if (p == params && ready) return;
        params = p;
        ready = true;
        ++version;
        const auto cents = systemCents (p.system);
        // Blå toner: neutral ters (350) och septima (1050), höjd kvart, neutral sext. I cent per steg från grundtonen.
        static constexpr double blueShift[12] = { 0, 0, 0, 50, -50, 30, 0, 0, 25, -25, 50, -50 };
        for (int pc = 0; pc < 12; ++pc)
        {
            const int step = ((pc - p.root) % 12 + 12) % 12;
            double c = cents[(size_t) step] - 100.0 * step;
            // De blå tonerna glider mot sitt neutrala läge (räknat från den liksvävande tonen), oavsett system.
            if (blueShift[step] != 0.0) c += (double) p.blue * (blueShift[step] - c);
            offsets[(size_t) pc] = c / 100.0;   // i halvtoner
        }
    }

    const StamningParams& getParams() const { return params; }
    // Ökar varje gång stämningen ändras, så att kammare med fast stämda delar (resonanssträngar) vet när de ska stämma om.
    int getVersion() const { return version; }

    // Avvikelsen från liksvävande i halvtoner för en (möjligen bråkdels-) MIDI-not, glidande mellan tangenterna.
    double offset (double note) const
    {
        const double fl = std::floor (note);
        const double frac = note - fl;
        const auto a = offsets[(size_t) (((int) fl % 12 + 12) % 12)];
        if (frac < 1.0e-9) return a;
        const auto b = offsets[(size_t) (((int) fl + 1) % 12 + 12) % 12];
        return a + (b - a) * frac;
    }

    double hz (double note) const { return 440.0 * std::exp2 ((note + offset (note) - 69.0) / 12.0); }

    static std::array<double, 12> systemCents (TuningSystem s)
    {
        auto ratios = [] (std::initializer_list<double> r)
        {
            std::array<double, 12> c {};
            int i = 0;
            for (double x : r) c[(size_t) i++] = 1200.0 * std::log2 (x);
            return c;
        };
        switch (s)
        {
            case TuningSystem::Ren:
                return ratios ({ 1.0, 16.0 / 15, 9.0 / 8, 6.0 / 5, 5.0 / 4, 4.0 / 3, 45.0 / 32, 3.0 / 2, 8.0 / 5, 5.0 / 3, 9.0 / 5, 15.0 / 8 });
            case TuningSystem::Pytagoreisk:
                return ratios ({ 1.0, 256.0 / 243, 9.0 / 8, 32.0 / 27, 81.0 / 64, 4.0 / 3, 729.0 / 512, 3.0 / 2, 128.0 / 81, 27.0 / 16, 16.0 / 9, 243.0 / 128 });
            case TuningSystem::Naturtoner:
                return ratios ({ 1.0, 17.0 / 16, 9.0 / 8, 19.0 / 16, 5.0 / 4, 21.0 / 16, 11.0 / 8, 3.0 / 2, 13.0 / 8, 27.0 / 16, 7.0 / 4, 15.0 / 8 });
            case TuningSystem::Medeltonig:
            {
                // Kvintkedjan från Ess till Giss med kvinter som är en kvarts syntoniskt komma smala (ca 696,6 cent).
                const double fifth = 1200.0 * std::log2 (1.5) - 1200.0 * std::log2 (81.0 / 80) / 4.0;
                std::array<double, 12> c {};
                for (int k = -3; k <= 8; ++k)
                {
                    double v = std::fmod (k * fifth, 1200.0);
                    if (v < 0) v += 1200.0;
                    const int step = ((k * 7) % 12 + 12) % 12;
                    c[(size_t) step] = v;
                }
                return c;
            }
            case TuningSystem::Liksvavande:
            default:
            {
                std::array<double, 12> c {};
                for (int i = 0; i < 12; ++i) c[(size_t) i] = 100.0 * i;
                return c;
            }
        }
    }

private:
    StamningParams params;
    bool ready = false;
    int version = 0;
    std::array<double, 12> offsets {};
};

} // namespace krang
