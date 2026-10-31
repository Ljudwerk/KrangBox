// Fabrikspresets för MVP:n – första utkast, ska finslipas av Doktor Krang och Sara efter lyssningstestet.
//
// Varje preset börjar från parametrarnas standardvärden och listar bara det som skiljer sig.
// Värden anges i parameterns egen enhet (Hz, dB, sekunder, 0..1); val anges som index.
// Trumpresets kan ha ett 16-stegsmönster per spår ('0' av, '1' slag, '2' accent), upprepat till 64 steg.
// Spårordning: Ångslaget, Skrotplåten, Smedjan, Klappern, Ramtrumman, Ventilen, Ångröret, Träskedarna.
//
// Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <initializer_list>
#include <utility>
#include <vector>

namespace krang::presets
{

struct FactoryPreset
{
    const char* name;      // UTF-8
    const char* category;  // UTF-8
    std::vector<std::pair<const char*, float>> values;
    std::vector<const char*> pattern; // tomt = mönstret lämnas orört; annars 8 rader à 16 tecken
};

// Index för valparametrar, så att listan nedan går att läsa.
namespace mode
{
    constexpr float basmalen = 0, hammarverket = 1, bordunorgeln = 2, spolslingan = 3;
    constexpr float arpUpp = 1, arpNed = 2, arpUppNed = 3, arpSlump = 4;
    constexpr float takt8 = 0, takt16 = 1, takt16T = 2, takt32 = 3;
    constexpr float tonC = 0, tonD = 2, tonE = 4, tonG = 7, tonA = 9;
    constexpr float ror = 0, fuzz = 1, vikning = 2, bitkross = 3;
    constexpr float filterAv = 0, lagpass = 1, hogpass = 2, formant = 3, kam = 4;
    constexpr float hall = 0, platta = 1, rostigTank = 2;
    constexpr float vokalA = 0.0f, vokalE = 0.25f, vokalI = 0.5f, vokalO = 0.75f, vokalU = 1.0f;
}

inline const std::vector<FactoryPreset>& factoryPresets()
{
    using namespace mode;
    static const std::vector<FactoryPreset> list =
    {
        // ------------------------------------------------------------------ Bas
        { "Init Basmålen", "Bas", { { "chamber", basmalen } }, {} },

        { "Ångvisslan", "Bas",
          { { "chamber", basmalen }, { "osc1Level", 1.0f }, { "osc2Level", 0.0f }, { "subLevel", 0.5f },
            { "glide", 220.0f }, { "cutoff", 300.0f }, { "resonance", 0.6f }, { "envAmount", 4.0f },
            { "distMode", ror }, { "distDrive", 24.0f } }, {} },

        { "Smedjans bas", "Bas",
          { { "chamber", basmalen }, { "osc1Level", 0.3f }, { "osc2Level", 0.9f }, { "osc2Semi", 0.0f }, { "osc2Fine", 4.0f },
            { "subLevel", 0.8f }, { "ampD", 0.35f }, { "ampS", 0.0f }, { "ampR", 0.08f },
            { "cutoff", 180.0f }, { "envAmount", 5.0f }, { "fltD", 0.22f }, { "fltS", 0.0f },
            { "distMode", fuzz }, { "distDrive", 20.0f } }, {} },

        { "Rostig sub", "Bas",
          { { "chamber", basmalen }, { "master", 4.0f }, { "osc1Level", 0.2f }, { "osc2Level", 0.0f }, { "subLevel", 1.0f },
            { "cutoff", 600.0f }, { "envAmount", 1.5f }, { "distMode", bitkross }, { "distDrive", 12.0f },
            { "distBits", 5.0f }, { "distDown", 6.0f }, { "distMix", 0.5f } }, {} },

        { "Kopparrörsacid", "Bas",
          { { "chamber", basmalen }, { "osc1Level", 1.0f }, { "osc2Level", 0.0f }, { "subLevel", 0.2f },
            { "glide", 80.0f }, { "cutoff", 250.0f }, { "resonance", 0.85f }, { "filterDrive", 2.5f },
            { "envAmount", 5.0f }, { "fltD", 0.18f }, { "fltS", 0.0f }, { "ampS", 0.7f },
            { "distMode", ror }, { "distDrive", 30.0f } }, {} },

        { "Vikt vågbas", "Bas",
          { { "chamber", basmalen }, { "osc2Semi", 0.0f }, { "osc2Fine", 5.0f }, { "cutoff", 1200.0f },
            { "envAmount", 2.0f }, { "distMode", vikning }, { "distDrive", 20.0f } }, {} },

        { "Domedagsbas", "Bas",
          { { "chamber", basmalen }, { "osc2Semi", -12.0f }, { "subLevel", 0.8f }, { "cutoff", 500.0f },
            { "distMode", fuzz }, { "distDrive", 40.0f }, { "distOut", -9.0f },
            { "fxMode", lagpass }, { "fxCutoff", 900.0f }, { "fxReso", 0.5f },
            { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Tyst maskin", "Bas",
          { { "chamber", basmalen }, { "distOut", 0.0f }, { "master", 2.0f }, { "osc1Level", 0.6f }, { "osc2Level", 0.3f }, { "cutoff", 800.0f },
            { "resonance", 0.25f }, { "envAmount", 1.5f }, { "distMix", 0.15f }, { "distDrive", 10.0f },
            { "rvbMode", platta }, { "rvbMix", 0.25f } }, {} },

        // ------------------------------------------------------------------ Drön och folk
        { "Nyckelharpsdrön", "Drön och folk",
          { { "chamber", basmalen }, { "osc1Level", 0.8f }, { "osc2Level", 0.6f }, { "osc2Semi", 7.0f }, { "osc2Fine", 12.0f },
            { "subLevel", 0.3f }, { "drift", 0.8f }, { "ampA", 0.4f }, { "ampS", 1.0f }, { "ampR", 1.2f },
            { "cutoff", 1500.0f }, { "envAmount", 0.5f }, { "distDrive", 12.0f }, { "distMix", 0.6f },
            { "fxMode", formant }, { "fxVowel", vokalE }, { "fxReso", 0.5f },
            { "rvbMode", hall }, { "rvbMix", 0.35f }, { "rvbSize", 0.8f } }, {} },

        { "Kulning i ångan", "Drön och folk",
          { { "chamber", basmalen }, { "osc1Level", 0.9f }, { "osc2Level", 0.2f }, { "osc2Semi", 12.0f },
            { "glide", 150.0f }, { "drift", 0.5f }, { "ampA", 0.08f }, { "ampS", 0.9f }, { "ampR", 0.6f },
            { "cutoff", 2000.0f }, { "envAmount", 1.0f }, { "distMix", 0.5f },
            { "fxMode", formant }, { "fxVowel", vokalO }, { "fxReso", 0.7f },
            { "rvbMode", hall }, { "rvbMix", 0.4f }, { "rvbSize", 0.9f }, { "rvbDark", 0.6f } }, {} },

        { "Kamröret", "Drön och folk",
          { { "chamber", basmalen }, { "osc1Level", 1.0f }, { "osc2Level", 0.0f }, { "ampA", 0.02f }, { "ampS", 0.9f },
            { "cutoff", 1400.0f }, { "envAmount", 1.0f }, { "distDrive", 14.0f },
            { "fxMode", kam }, { "fxCutoff", 110.0f }, { "fxReso", 0.8f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Väderkvarnen", "Drön och folk",
          { { "chamber", basmalen }, { "master", 4.0f }, { "osc1Level", 0.5f }, { "osc2Level", 0.7f }, { "osc2Semi", -5.0f }, { "osc2Fine", -9.0f },
            { "subLevel", 0.4f }, { "drift", 1.0f }, { "ampA", 1.2f }, { "ampS", 1.0f }, { "ampR", 2.5f },
            { "cutoff", 700.0f }, { "resonance", 0.6f }, { "envAmount", 2.0f }, { "fltA", 1.5f }, { "fltD", 2.0f }, { "fltS", 0.4f },
            { "distMode", vikning }, { "distDrive", 10.0f }, { "distMix", 0.5f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 1.0f } }, {} },

        { "Bordunorgeln", "Drön och folk", { { "chamber", bordunorgeln }, { "distMix", 0.25f }, { "rvbMix", 0.25f } }, {} },

        { "Nyckelharpan i ruinen", "Drön och folk",
          { { "chamber", bordunorgeln }, { "boTimbre", 1.0f }, { "boBow", 0.5f }, { "boDrone", 0.4f }, { "boDroneNote", tonD },
            { "boSymp", 0.8f }, { "boDrift", 0.3f }, { "boAttack", 0.15f }, { "boRelease", 1.5f },
            { "distDrive", 8.0f }, { "distMix", 0.35f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Säckpipan från Ångvik", "Drön och folk",
          { { "chamber", bordunorgeln }, { "boTimbre", 0.5f }, { "boBow", 0.25f }, { "boDrone", 0.8f }, { "boDroneNote", tonA },
            { "boSymp", 0.2f }, { "boAttack", 0.05f }, { "boRelease", 0.4f },
            { "distMode", fuzz }, { "distDrive", 10.0f }, { "distMix", 0.4f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Vallflöjt i dimman", "Drön och folk",
          { { "chamber", bordunorgeln }, { "boTimbre", 0.05f }, { "boBow", 0.6f }, { "boDrone", 0.2f }, { "boDroneNote", tonG },
            { "boSymp", 0.5f }, { "boDrift", 0.6f }, { "boAttack", 0.3f }, { "boRelease", 2.5f }, { "distMix", 0.1f },
            { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 0.9f }, { "rvbDark", 0.6f } }, {} },

        { "Rostorgeln", "Drön och folk",
          { { "chamber", bordunorgeln }, { "boTimbre", 0.7f }, { "boDrone", 0.7f }, { "boDroneNote", tonC }, { "boSymp", 0.6f },
            { "boDrift", 0.8f }, { "distMode", bitkross }, { "distBits", 6.0f }, { "distDown", 3.0f }, { "distMix", 0.5f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.35f } }, {} },

        // ------------------------------------------------------------------ Leads och slingor
        { "Spolslingan", "Leads och slingor", { { "chamber", spolslingan }, { "distMix", 0.3f }, { "rvbMix", 0.2f } }, {} },

        { "Munharpa i maskinhallen", "Leads och slingor",
          { { "chamber", spolslingan }, { "spHarp", 0.9f }, { "spMouth", 0.55f }, { "spIndex", 1.0f }, { "spRelease", 0.2f },
            { "distDrive", 10.0f }, { "distMix", 0.4f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Kulning över ödemarken", "Leads och slingor",
          { { "chamber", spolslingan }, { "spRatio", 1.0f }, { "spIndex", 1.2f }, { "spDecay", 1.5f }, { "spAttack", 0.08f },
            { "spRelease", 1.2f }, { "distMix", 0.2f }, { "fxMode", formant }, { "fxVowel", vokalO }, { "fxReso", 0.5f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } }, {} },

        { "Trasig radio", "Leads och slingor",
          { { "chamber", spolslingan }, { "master", 4.0f }, { "spRatio", 3.5f }, { "spIndex", 4.0f }, { "spGrain", 0.8f }, { "spGrainMs", 60.0f },
            { "spSpread", 0.7f }, { "distMode", bitkross }, { "distBits", 7.0f }, { "distMix", 0.3f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.2f } }, {} },

        { "Ångslinga", "Leads och slingor",
          { { "chamber", spolslingan }, { "spArp", arpUpp }, { "spArpRate", takt16 }, { "spArpOct", 2.0f },
            { "spRatio", 2.0f }, { "spIndex", 5.0f }, { "spDecay", 0.12f }, { "spRelease", 0.1f }, { "seqTempo", 112.0f },
            { "distDrive", 14.0f }, { "rvbMode", platta }, { "rvbMix", 0.15f } }, {} },

        { "Spolpuls i ruinen", "Leads och slingor",
          { { "chamber", spolslingan }, { "spArp", arpSlump }, { "spArpRate", takt16T }, { "spArpOct", 3.0f },
            { "spRatio", 1.41f }, { "spIndex", 6.0f }, { "spDecay", 0.2f }, { "spRelease", 0.15f }, { "spGrain", 0.3f },
            { "seqTempo", 90.0f }, { "distMix", 0.5f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Trummor
        { "Init Hammarverket", "Trummor", { { "chamber", hammarverket } },
          { "2000001000100000", "0000000000000100", "0000100000002000", "0000000000001000",
            "0001000100000001", "1010100010100000", "0000000000000010", "0000010001010000" } },

        { "Smedjegång", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 96.0f }, { "distDrive", 12.0f }, { "hvRost", 0.3f },
            { "rvbMode", platta }, { "rvbMix", 0.12f } },
          { "2000001000100000", "0000000000000100", "0000100000002000", "0000000000001000",
            "0001000100000001", "1010100010100000", "0000000000000010", "0000010001010000" } },

        { "Rostig fabrik", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 118.0f }, { "hvRost", 0.9f }, { "hv1Level", 1.0f },
            { "distMode", bitkross }, { "distDrive", 10.0f }, { "distBits", 6.0f }, { "distDown", 4.0f }, { "distMix", 0.6f },
            { "rvbMode", hall }, { "rvbMix", 0.2f } },
          { "2000100020001000", "0010001000100010", "0000200000002000", "0000000000000000",
            "0000000000000000", "1111111111111111", "0000000000000000", "0000000000000000" } },

        { "Folkdans i ruinerna", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 104.0f }, { "seqSwing", 0.2f }, { "distDrive", 6.0f },
            { "hv4Level", 1.0f }, { "hv7Level", 0.9f }, { "rvbMode", hall }, { "rvbMix", 0.3f } },
          { "2000000020000000", "0000000000000000", "0000000000000000", "0000000000000000",
            "2001001002001001", "0000000000000000", "0000000000000000", "0010010010010010" } },

        { "Ångslagets puls", "Trummor",
          { { "chamber", hammarverket }, { "master", 2.0f }, { "seqTempo", 128.0f }, { "distMode", fuzz }, { "distDrive", 10.0f },
            { "distMix", 0.4f }, { "hv0Decay", 0.8f }, { "rvbMode", platta }, { "rvbMix", 0.1f } },
          { "2000200020002000", "0000000000000000", "0000000000000000", "0000100000001000",
            "0000000000000000", "1000100010001000", "0010001000100010", "0000000000000000" } },

        { "Bitkrossad marsch", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 100.0f }, { "distMode", bitkross }, { "distDrive", 8.0f },
            { "distBits", 4.0f }, { "distDown", 8.0f }, { "hv2Decay", 0.6f } },
          { "2000000020000000", "0000000000000000", "2011201120112111", "0000000000000000",
            "0000000000000000", "0000000000000000", "0000000000000000", "0000000000000000" } },

        { "Katedralens slagverk", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 70.0f }, { "hv4Tune", -5.0f }, { "hv4Decay", 1.8f },
            { "hv1Decay", 2.0f }, { "distDrive", 8.0f }, { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 0.9f } },
          { "2000000000000000", "0000000000100000", "0000000000000000", "0000000000000000",
            "0000000200000000", "0000000000000000", "0000000000000000", "0000000000000000" } },

        { "Träskedsbrygga", "Trummor",
          { { "chamber", hammarverket }, { "master", 3.0f }, { "seqTempo", 110.0f }, { "seqSwing", 0.3f }, { "distMix", 0.3f },
            { "hv7Level", 1.0f }, { "rvbMode", rostigTank }, { "rvbMix", 0.25f } },
          { "0000000000000000", "0000000000000000", "0000000000000000", "0000100000001000",
            "1000001000100000", "0000000000000000", "0000000000000000", "1011101110111011" } },

        { "Ventilsystemet", "Trummor",
          { { "chamber", hammarverket }, { "distOut", 0.0f }, { "master", 3.0f }, { "seqTempo", 122.0f }, { "hv5Decay", 1.3f }, { "hv5Level", 0.55f }, { "distDrive", 9.0f },
            { "hv0Level", 1.0f }, { "hv2Level", 1.0f }, { "fxMode", hogpass }, { "fxCutoff", 90.0f }, { "fxReso", 0.4f }, { "rvbMode", platta }, { "rvbMix", 0.2f } },
          { "2000001000200010", "0000000000000000", "0000100000001000", "0000000000000000",
            "0000000000000000", "2121212121212101", "0000000000000010", "0000000000000000" } },
    };
    return list;
}

} // namespace krang::presets
