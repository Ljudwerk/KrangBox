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
    const char* melody = nullptr;     // Tonstegverkets melodi (se TonPattern::fromString); nullptr = orörd
};

// Index för valparametrar, så att listan nedan går att läsa.
namespace mode
{
    constexpr float basmalen = 0, hammarverket = 1, bordunorgeln = 2, spolslingan = 3;
    constexpr float arpUpp = 1, arpNed = 2, arpUppNed = 3, arpSlump = 4, arpSomSpelat = 5, arpAckord = 6, arpTvars = 7, arpTrappa = 8;
    constexpr float fasvridaren = 1;
    constexpr float angorgeln = 11, speldosan = 12, automaten = 13, krigstuban = 14;
    constexpr float massingTratt = 0, ljudtratt = 1, megafon = 2;
    constexpr float liksvavande = 0, renStamning = 1, pytagoreisk = 2, medeltonig = 3, naturtoner = 4, grundBordunen = 12;
    constexpr float kyrkorgel = 0, kalliope = 1, harmonium = 2, positiv = 3;
    constexpr float vanligDosa = 0, kyrkspeldosa = 1, glasdosa = 2;
    constexpr float ordKrang = 0, ordMaskin = 1, ordHjalp = 2, ordAnga = 3, ordVarning = 4, ordRost = 5, ordNollEtt = 6,
                    ordFolket = 7, ordJa = 8, ordNej = 9, ordManniska = 10, ordDoktorKrang = 11, ordTangenterna = 12;
    constexpr float sangfageln = 1, urverket = 2;
    constexpr float kanarie = 0, naktergal = 1, larka = 2, trast = 3;
    constexpr float fickur = 0, pendelur = 1, gokur = 2, klockspelsur = 3;
    constexpr float pdSag = 0, pdFyrkant = 1, pdPuls = 2, pdDubbelsinus = 3, pdResSag = 4, pdResTriangel = 5, pdResTrapets = 6;
    constexpr float takt8 = 0, takt16 = 1, takt16T = 2, takt32 = 3;
    constexpr float tonC = 0, tonD = 2, tonE = 4, tonG = 7, tonA = 9;
    constexpr float ror = 0, fuzz = 1, vikning = 2, bitkross = 3;
    constexpr float filterAv = 0, lagpass = 1, hogpass = 2, formant = 3, kam = 4;
    constexpr float hall = 0, platta = 1, rostigTank = 2;
    constexpr float stalpiano = 4;
    constexpr float pianoLage = 0, hackbrade = 1, preparerat = 2;
    constexpr float klockgjuteriet = 5, koren = 6, radiomasten = 7, lurblasaren = 8, lutverket = 9, vattenpumpen = 10;
    constexpr float vattenfon = 0, sag = 1, plat = 2, cymbal = 3;
    constexpr float kantele = 0, luta = 1, cittra = 2, rostharpa = 3;
    constexpr float naverlur = 0, bockhorn = 1, skalmeja = 2, angvissla = 3;
    constexpr float kortvag = 0, morse = 1, vindLedningar = 2, interferens = 3;
    constexpr float mansroster = 0, kvinnoroster = 1, blandadKor = 2, kulning = 3;
    constexpr float kyrkklocka = 0, skalla = 1, gong = 2, rals = 3, skalklocka = 4;
    constexpr float stal = 0, massing = 1, koppar = 2, jarn = 3, silke = 4, djurhud = 5;
    constexpr float kaosFri = 0, kaos4 = 1, kaos8 = 2, kaos16 = 3, kaos32 = 4;
    constexpr float ekoFri = 0, eko4 = 1, eko8 = 2, eko8p = 3, eko16 = 4, eko4T = 5;
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

        { "Slitet band", "Bas",
          { { "chamber", basmalen }, { "osc1Level", 0.8f }, { "osc2Level", 0.4f }, { "osc2Fine", 7.0f }, { "subLevel", 0.6f },
            { "cutoff", 420.0f }, { "resonance", 0.4f }, { "envAmount", 3.0f }, { "fltD", 0.3f }, { "fltS", 0.2f },
            { "distMode", ror }, { "distDrive", 16.0f },
            { "tpWow", 0.45f }, { "tpFlutter", 0.35f }, { "tpAge", 0.6f },
            { "ecSync", eko8p }, { "ecFeedback", 0.35f }, { "ecDark", 0.6f }, { "ecMix", 0.25f } }, {} },

        { "Hackande maskin", "Bas",
          { { "chamber", basmalen }, { "osc1Level", 0.9f }, { "osc2Level", 0.5f }, { "osc2Semi", 12.0f }, { "subLevel", 0.6f },
            { "cutoff", 500.0f }, { "resonance", 0.55f }, { "envAmount", 3.5f }, { "fltD", 0.2f }, { "fltS", 0.1f },
            { "distMode", fuzz }, { "distDrive", 18.0f },
            { "kaAmount", 0.35f }, { "kaSync", kaos16 }, { "kaDrift", 0.2f }, { "kaStutter", 0.8f }, { "kaReverse", 0.2f },
            { "kaCrush", 0.4f } }, {} },

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

        { "Gammal rulle", "Drön och folk",
          { { "chamber", bordunorgeln }, { "boTimbre", 0.4f }, { "boDrone", 0.5f }, { "boSymp", 0.5f },
            { "distMix", 0.2f }, { "distDrive", 8.0f },
            { "tpWow", 0.8f }, { "tpFlutter", 0.5f }, { "tpAge", 0.85f }, { "tpDropout", 0.5f },
            { "ecSync", ekoFri }, { "ecTime", 620.0f }, { "ecFeedback", 0.55f }, { "ecDark", 0.75f }, { "ecMix", 0.3f },
            { "rvbMode", hall }, { "rvbMix", 0.25f } }, {} },

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

        { "Ekon från ödemarken", "Leads och slingor",
          { { "chamber", spolslingan }, { "spArp", arpUppNed }, { "spArpRate", takt8 }, { "spArpOct", 2.0f },
            { "spRatio", 3.0f }, { "spIndex", 3.5f }, { "spDecay", 0.2f }, { "spRelease", 0.15f }, { "seqTempo", 96.0f },
            { "distDrive", 8.0f }, { "tpWow", 0.25f }, { "tpAge", 0.4f },
            { "ecSync", eko8p }, { "ecFeedback", 0.7f }, { "ecDark", 0.55f }, { "ecMix", 0.4f },
            { "rvbMode", platta }, { "rvbMix", 0.15f } }, {} },

        { "Radion i bunkern", "Leads och slingor",
          { { "chamber", spolslingan }, { "spRatio", 1.5f }, { "spIndex", 4.0f }, { "spDecay", 0.5f }, { "spHarp", 0.3f },
            { "spGrain", 0.25f }, { "distMode", bitkross }, { "distBits", 7.0f }, { "distMix", 0.4f },
            { "kaAmount", 0.45f }, { "kaSync", kaos8 }, { "kaDrift", 0.6f }, { "kaStutter", 0.4f }, { "kaReverse", 0.6f },
            { "kaCrush", 0.3f }, { "tpAge", 0.5f }, { "rvbMode", rostigTank }, { "rvbMix", 0.25f } }, {} },

        // ------------------------------------------------------------------ Fasvridaren
        // Spolslingan med fasdistorsion i CZ-stil, och Slingans nya lägen.
        { "Init Fasvridaren", "Fasvridaren",
          { { "chamber", spolslingan }, { "master", 2.0f }, { "spEngine", fasvridaren }, { "spPdAmount", 0.7f }, { "spDecay", 0.5f },
            { "distMix", 0.2f }, { "rvbMix", 0.2f } }, {} },

        { "Resonant spole", "Fasvridaren",
          { { "chamber", spolslingan }, { "master", 4.0f }, { "spEngine", fasvridaren }, { "spPdWave", pdResSag }, { "spPdAmount", 0.9f }, { "spDecay", 0.25f },
            { "spRelease", 0.25f }, { "distMix", 0.3f }, { "distDrive", 12.0f }, { "ecSync", eko8p }, { "ecFeedback", 0.35f }, { "ecMix", 0.2f },
            { "rvbMix", 0.2f } }, {} },

        { "Svävande stråkar", "Fasvridaren",
          { { "chamber", spolslingan }, { "master", 6.0f }, { "spEngine", fasvridaren }, { "spPdWave", pdSag }, { "spPdAmount", 0.45f }, { "spDecay", 2.0f },
            { "spPdDetune", 14.0f }, { "spAttack", 0.6f }, { "spRelease", 1.8f }, { "distMix", 0.0f }, { "tpWow", 0.25f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.85f } }, {} },

        { "Trappan i tornet", "Fasvridaren",
          { { "chamber", spolslingan }, { "spEngine", fasvridaren }, { "spPdWave", pdResTrapets }, { "spPdAmount", 0.6f }, { "spDecay", 0.15f },
            { "spArp", arpTrappa }, { "spArpRate", takt16 }, { "spArpOct", 2.0f }, { "spArpGate", 0.4f }, { "spRelease", 0.15f },
            { "ecSync", eko8 }, { "ecFeedback", 0.3f }, { "ecMix", 0.2f }, { "rvbMix", 0.25f } }, {} },

        { "Ackordpumpen", "Fasvridaren",
          { { "chamber", spolslingan }, { "spEngine", fasvridaren }, { "spPdWave", pdFyrkant }, { "spPdAmount", 0.8f }, { "spDecay", 0.12f },
            { "spPdDetune", 8.0f }, { "spArp", arpAckord }, { "spArpRate", takt8 }, { "spArpGate", 0.3f }, { "spArpLatch", 1.0f },
            { "spRelease", 0.1f }, { "distDrive", 14.0f }, { "distMix", 0.4f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Pulsande ruin", "Fasvridaren",
          { { "chamber", spolslingan }, { "spEngine", fasvridaren }, { "spPdWave", pdPuls }, { "spPdAmount", 0.75f }, { "spDecay", 0.3f },
            { "spArp", arpTvars }, { "spArpRate", takt16T }, { "spArpOct", 2.0f }, { "spArpGate", 0.6f }, { "spGrain", 0.25f },
            { "kaAmount", 0.2f }, { "kaSync", kaos16 }, { "kaStutter", 0.8f }, { "rvbMix", 0.25f } }, {} },

        // ------------------------------------------------------------------ Stålpiano
        { "Init Stålpiano", "Stålpiano",
          { { "chamber", stalpiano }, { "distMix", 0.15f }, { "distDrive", 8.0f }, { "rvbMix", 0.2f } }, {} },

        { "Missionshuset", "Stålpiano",
          { { "chamber", stalpiano }, { "stHard", 0.4f }, { "stDetune", 7.0f }, { "stRust", 0.3f }, { "stBody", 0.6f },
            { "distMix", 0.2f }, { "distDrive", 10.0f }, { "tpWow", 0.25f }, { "tpAge", 0.45f },
            { "rvbMode", hall }, { "rvbMix", 0.35f }, { "rvbSize", 0.8f } }, {} },

        { "Hackbrädet", "Stålpiano",
          { { "chamber", stalpiano }, { "master", 1.5f }, { "stMode", hackbrade }, { "stHard", 0.7f }, { "stPos", 0.1f }, { "stDetune", 5.0f },
            { "stDecay", 6.0f }, { "stBody", 0.55f }, { "distMix", 0.1f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Skruvat piano", "Stålpiano",
          { { "chamber", stalpiano }, { "stMode", preparerat }, { "stHard", 0.6f }, { "stStiff", 0.6f }, { "stRust", 0.2f },
            { "distMode", ror }, { "distDrive", 12.0f }, { "distMix", 0.3f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.25f } }, {} },

        { "Stålregn", "Stålpiano",
          { { "chamber", stalpiano }, { "stHard", 0.9f }, { "stStiff", 0.9f }, { "stPos", 0.08f }, { "stDecay", 12.0f },
            { "distMix", 0.1f }, { "ecSync", eko8p }, { "ecFeedback", 0.5f }, { "ecDark", 0.5f }, { "ecMix", 0.3f },
            { "kaAmount", 0.15f }, { "kaStutter", 0.2f }, { "kaReverse", 0.8f }, { "kaCrush", 0.0f }, { "kaDrift", 0.1f },
            { "rvbMode", hall }, { "rvbMix", 0.4f } }, {} },

        { "Kedjepianot", "Stålpiano",
          { { "chamber", stalpiano }, { "stMode", preparerat }, { "stRust", 0.8f }, { "stDetune", 10.0f }, { "stDamper", 0.4f },
            { "distMode", bitkross }, { "distBits", 6.0f }, { "distDown", 4.0f }, { "distMix", 0.4f },
            { "kaAmount", 0.25f }, { "kaStutter", 0.8f }, { "kaReverse", 0.2f }, { "kaCrush", 0.3f } }, {} },

        { "Hudharpan", "Stålpiano",
          { { "chamber", stalpiano }, { "master", 3.0f }, { "stMaterial", djurhud }, { "stMode", hackbrade }, { "stHard", 0.6f },
            { "stDecay", 16.0f }, { "stBody", 0.8f }, { "stDetune", 6.0f }, { "distMix", 0.15f },
            { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Silkesqin", "Stålpiano",
          { { "chamber", stalpiano }, { "master", 2.0f }, { "stMaterial", silke }, { "stHard", 0.3f }, { "stPos", 0.2f }, { "stDecay", 14.0f },
            { "stDamper", 0.6f }, { "stBody", 0.6f }, { "distMix", 0.0f }, { "ecSync", eko8p }, { "ecMix", 0.2f },
            { "ecFeedback", 0.35f }, { "rvbMode", platta }, { "rvbMix", 0.3f } }, {} },

        { "Kopparharpan", "Stålpiano",
          { { "chamber", stalpiano }, { "stMaterial", koppar }, { "stMode", hackbrade }, { "stHard", 0.55f },
            { "stDetune", 4.0f }, { "distMix", 0.1f }, { "tpAge", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.25f } }, {} },

        // ------------------------------------------------------------------ Klockor
        { "Init Klockgjuteriet", "Klockor",
          { { "chamber", klockgjuteriet }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.25f } }, {} },

        { "Kyrkklockan i dimman", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", kyrkklocka }, { "klDecay", 18.0f }, { "klHum", 0.8f },
            { "distMix", 0.0f }, { "tpAge", 0.25f }, { "tpWow", 0.15f }, { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } }, {} },

        { "Fäbodskällan", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", skalla }, { "klHard", 0.7f }, { "klDecay", 6.0f }, { "master", 2.0f }, { "klBeat", 8.0f },
            { "distMix", 0.1f }, { "rvbMode", platta }, { "rvbMix", 0.15f } }, {} },

        { "Operagongen", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", gong }, { "klSweep", 0.6f }, { "klHard", 0.4f }, { "klDecay", 14.0f },
            { "distMix", 0.05f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Rälsen i tunneln", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", rals }, { "klHard", 0.9f }, { "klDecay", 12.0f },
            { "distMix", 0.15f }, { "ecSync", 1.0f }, { "ecFeedback", 0.5f }, { "ecDark", 0.6f }, { "ecMix", 0.3f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Sprucken kyrkklocka", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", kyrkklocka }, { "klCrack", 0.6f }, { "klBeat", 12.0f },
            { "distMode", bitkross }, { "distBits", 7.0f }, { "distMix", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Sjungande skålen", "Klockor",
          { { "chamber", klockgjuteriet }, { "klType", skalklocka }, { "klHard", 0.2f }, { "klBeat", 8.0f }, { "klDecay", 30.0f },
            { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Kör
        { "Init Kören", "Kör",
          { { "chamber", koren }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.3f } }, {} },

        { "Psalm i schaktet", "Kör",
          { { "chamber", koren }, { "koVoices", blandadKor }, { "koVowel", vokalO }, { "koSingers", 8.0f }, { "koSpread", 10.0f },
            { "koVibrato", 0.35f }, { "koAttack", 0.8f }, { "koRelease", 2.0f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 0.9f } }, {} },

        { "Kulning över dalen", "Kör",
          { { "chamber", koren }, { "master", 5.0f }, { "koVoices", kulning }, { "koVowel", vokalA }, { "koVibrato", 0.15f }, { "koBreath", 0.15f },
            { "koAttack", 0.08f }, { "koRelease", 0.6f }, { "distMix", 0.0f },
            { "ecSync", 1.0f }, { "ecFeedback", 0.4f }, { "ecDark", 0.6f }, { "ecMix", 0.25f }, { "rvbMode", hall }, { "rvbMix", 0.45f } }, {} },

        { "Munkarna i gruvan", "Kör",
          { { "chamber", koren }, { "koVoices", mansroster }, { "koVowel", vokalU }, { "koSingers", 8.0f }, { "koSpread", 5.0f },
            { "koVibrato", 0.1f }, { "koAttack", 1.2f }, { "koRelease", 2.5f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbDark", 0.7f } }, {} },

        { "Högtalarkören", "Kör",
          { { "chamber", koren }, { "koVoices", kvinnoroster }, { "koVowel", vokalE }, { "koSpeaker", 0.8f },
            { "distMix", 0.1f }, { "tpAge", 0.5f }, { "tpDropout", 0.3f }, { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Spökkören", "Kör",
          { { "chamber", koren }, { "koVoices", blandadKor }, { "koVowel", vokalI }, { "koSpread", 25.0f }, { "koBreath", 0.7f },
            { "koVibrato", 0.6f }, { "koAttack", 2.0f }, { "koRelease", 3.0f }, { "distMix", 0.0f }, { "tpWow", 0.4f },
            { "ecSync", 3.0f }, { "ecFeedback", 0.55f }, { "ecMix", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.5f } }, {} },

        // ------------------------------------------------------------------ Radio
        { "Init Radiomasten", "Radio",
          { { "chamber", radiomasten }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.2f } }, {} },

        { "Nattsändning från Moskva", "Radio",
          { { "chamber", radiomasten }, { "raMode", kortvag }, { "raFading", 0.6f }, { "raDrift", 0.4f }, { "raNoise", 0.5f },
            { "raBandwidth", 0.3f }, { "distMix", 0.0f }, { "tpAge", 0.4f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "SOS från skeppet", "Radio",
          { { "chamber", radiomasten }, { "raMode", morse }, { "raSignal", 0.8f }, { "raNoise", 0.35f }, { "raFading", 0.4f },
            { "raStorm", 0.3f }, { "distMix", 0.0f }, { "ecSync", 2.0f }, { "ecFeedback", 0.35f }, { "ecMix", 0.2f },
            { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Telegraftrådarna", "Radio",
          { { "chamber", radiomasten }, { "master", 2.0f }, { "raMode", vindLedningar }, { "raSignal", 0.7f }, { "raNoise", 0.2f }, { "raStorm", 0.1f },
            { "raAttack", 1.5f }, { "raRelease", 3.0f }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.4f } }, {} },

        { "Fjärrskrivaren", "Radio",
          { { "chamber", radiomasten }, { "raMode", interferens }, { "raBandwidth", 0.4f }, { "raDrift", 0.5f },
            { "distMode", bitkross }, { "distBits", 6.0f }, { "distMix", 0.3f }, { "rvbMix", 0.1f } }, {} },

        { "Åska över masten", "Radio",
          { { "chamber", radiomasten }, { "raMode", kortvag }, { "raStorm", 0.9f }, { "raFading", 0.8f }, { "raNoise", 0.6f },
            { "distMix", 0.1f }, { "kaAmount", 0.2f }, { "kaStutter", 0.2f }, { "kaReverse", 0.1f }, { "kaCrush", 0.8f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Blås
        { "Init Lurblåsaren", "Blås",
          { { "chamber", lurblasaren }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.25f } }, {} },

        { "Vallåten", "Blås",
          { { "chamber", lurblasaren }, { "luType", naverlur }, { "luNatural", 1.0f }, { "luKey", 2.0f }, { "luBreath", 0.6f },
            { "luVibrato", 0.15f }, { "luAttack", 0.15f }, { "distMix", 0.0f },
            { "ecSync", 1.0f }, { "ecFeedback", 0.3f }, { "ecMix", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } }, {} },

        { "Bockhornet", "Blås",
          { { "chamber", lurblasaren }, { "master", 2.0f }, { "luType", bockhorn }, { "luRasp", 0.4f }, { "luBreath", 0.8f }, { "luNoise", 0.3f },
            { "distMix", 0.1f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Skalmejan i gränden", "Blås",
          { { "chamber", lurblasaren }, { "luType", skalmeja }, { "luTension", 0.6f }, { "luVibrato", 0.25f }, { "luRasp", 0.2f },
            { "distMix", 0.15f }, { "tpAge", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.25f } }, {} },

        { "Ångvisslan i hamnen", "Blås",
          { { "chamber", lurblasaren }, { "luType", angvissla }, { "luBreath", 0.9f }, { "luTension", 0.35f }, { "luNoise", 0.4f },
            { "luAttack", 0.3f }, { "luRelease", 0.8f }, { "distMix", 0.0f },
            { "ecSync", 2.0f }, { "ecFeedback", 0.45f }, { "ecDark", 0.6f }, { "ecMix", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.5f } }, {} },

        { "Rostiga luren", "Blås",
          { { "chamber", lurblasaren }, { "luType", naverlur }, { "luRasp", 0.8f }, { "luBreath", 0.9f }, { "luNoise", 0.5f },
            { "distMode", fuzz }, { "distDrive", 14.0f }, { "distMix", 0.4f }, { "tpWow", 0.3f }, { "tpAge", 0.5f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Strängar
        { "Init Lutverket", "Strängar",
          { { "chamber", lutverket }, { "master", 2.5f }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.2f } }, {} },

        { "Kantelen vid elden", "Strängar",
          { { "chamber", lutverket }, { "master", 2.0f }, { "ltType", kantele }, { "ltDecay", 8.0f }, { "ltDamper", 1.5f }, { "ltPos", 0.15f },
            { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Lutan i tornet", "Strängar",
          { { "chamber", lutverket }, { "master", 3.5f }, { "ltType", luta }, { "ltDecay", 9.0f }, { "ltHard", 0.35f }, { "ltBright", 0.45f }, { "ltCourse", 5.0f },
            { "ltBody", 0.7f }, { "distMix", 0.0f }, { "tpAge", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.35f } }, {} },

        { "Cittrans regn", "Strängar",
          { { "chamber", lutverket }, { "ltType", cittra }, { "ltDecay", 10.0f }, { "ltDamper", 2.0f }, { "ltCourse", 7.0f },
            { "distMix", 0.0f }, { "ecSync", 3.0f }, { "ecFeedback", 0.45f }, { "ecMix", 0.25f }, { "rvbMode", platta }, { "rvbMix", 0.3f } }, {} },

        { "Rostharpan", "Strängar",
          { { "chamber", lutverket }, { "master", 5.0f }, { "ltType", rostharpa }, { "ltDecay", 9.0f }, { "ltBuzz", 0.5f }, { "ltHard", 0.7f }, { "ltCourse", 9.0f },
            { "distMode", ror }, { "distDrive", 12.0f }, { "distMix", 0.3f }, { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Plockslinga i ruinen", "Strängar",
          { { "chamber", lutverket }, { "master", 2.5f }, { "ltType", kantele }, { "ltHard", 0.8f }, { "ltDecay", 6.0f }, { "ltBuzz", 0.2f },
            { "distMix", 0.15f }, { "kaAmount", 0.25f }, { "kaStutter", 0.7f }, { "kaReverse", 0.4f }, { "kaCrush", 0.0f },
            { "ecSync", 2.0f }, { "ecMix", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.25f } }, {} },

        // ------------------------------------------------------------------ Stråkad metall
        { "Init Vattenpumpen", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "distMix", 0.1f }, { "distDrive", 8.0f }, { "rvbMix", 0.3f } }, {} },

        { "Vattenfonen i tunneln", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "master", 2.0f }, { "vpType", vattenfon }, { "vpWater", 0.7f }, { "vpPressure", 0.4f }, { "vpAttack", 1.0f },
            { "vpRelease", 3.0f }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 0.9f } }, {} },

        { "Sjungande sågen", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "vpType", sag }, { "vpGlide", 0.4f }, { "vpVibrato", 0.5f }, { "vpPressure", 0.3f },
            { "vpAttack", 0.4f }, { "vpRelease", 1.2f }, { "distMix", 0.0f }, { "rvbMode", platta }, { "rvbMix", 0.35f } }, {} },

        { "Plåtåskan", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "vpType", plat }, { "vpPressure", 0.7f }, { "vpRust", 0.4f }, { "vpAttack", 0.6f },
            { "vpRelease", 2.5f }, { "distMix", 0.15f }, { "rvbMode", rostigTank }, { "rvbMix", 0.35f } }, {} },

        { "Cymbalens skrik", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "vpType", cymbal }, { "vpPressure", 0.85f }, { "vpSpeed", 0.9f }, { "vpAttack", 1.5f },
            { "vpRelease", 4.0f }, { "distMix", 0.0f }, { "ecSync", 1.0f }, { "ecFeedback", 0.4f }, { "ecMix", 0.2f },
            { "rvbMode", hall }, { "rvbMix", 0.45f } }, {} },

        { "Pumpstationen", "Stråkad metall",
          { { "chamber", vattenpumpen }, { "vpType", vattenfon }, { "vpWater", 1.0f }, { "vpRust", 0.5f }, { "vpPressure", 0.6f },
            { "distMode", fuzz }, { "distDrive", 10.0f }, { "distMix", 0.25f }, { "kaAmount", 0.15f }, { "kaStutter", 0.3f },
            { "kaReverse", 0.7f }, { "kaCrush", 0.0f }, { "tpWow", 0.4f }, { "rvbMode", hall }, { "rvbMix", 0.4f } }, {} },

        // ------------------------------------------------------------------ Orgel
        { "Init Ångorgeln", "Orgel", { { "chamber", angorgeln }, { "distMix", 0.0f }, { "rvbMix", 0.3f } }, {} },

        { "Katedralen i ruinen", "Orgel",
          { { "chamber", angorgeln }, { "agR16", 0.8f }, { "agR8", 1.0f }, { "agR4", 0.8f }, { "agR223", 0.4f }, { "agR2", 0.6f },
            { "agPressure", 0.85f }, { "agChiff", 0.5f }, { "agLeak", 0.15f }, { "agRelease", 0.6f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 1.0f } }, {} },

        { "Karusellen", "Orgel",
          { { "chamber", angorgeln }, { "agType", kalliope }, { "agR16", 0.0f }, { "agR8", 1.0f }, { "agR4", 0.7f }, { "agR2", 0.0f },
            { "agPressure", 0.6f }, { "agChiff", 0.6f }, { "agLeak", 0.35f }, { "agTrem", 0.3f }, { "distMix", 0.15f },
            { "tpWow", 0.2f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Väckelsemötet", "Orgel",
          { { "chamber", angorgeln }, { "master", 5.0f }, { "agType", harmonium }, { "agR16", 0.3f }, { "agR8", 1.0f }, { "agR4", 0.5f }, { "agR2", 0.0f },
            { "agTrem", 0.4f }, { "agChiff", 0.2f }, { "agAttack", 0.08f }, { "agRelease", 0.3f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Positivet i kapellet", "Orgel",
          { { "chamber", angorgeln }, { "master", 2.0f }, { "agType", positiv }, { "agR16", 0.0f }, { "agR8", 1.0f }, { "agR4", 0.6f },
            { "agR223", 0.5f }, { "agR2", 0.0f }, { "agChiff", 0.7f }, { "agPressure", 0.9f }, { "agLeak", 0.05f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.35f }, { "rvbSize", 0.7f } }, {} },

        { "Sviktande bälg", "Orgel",
          { { "chamber", angorgeln }, { "agR16", 0.6f }, { "agR8", 1.0f }, { "agR4", 0.6f }, { "agR2", 0.4f }, { "agPressure", 0.2f },
            { "agLeak", 0.6f }, { "agTrem", 0.15f }, { "agRelease", 0.8f }, { "tpWow", 0.3f }, { "tpAge", 0.5f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.35f } }, {} },

        // ------------------------------------------------------------------ Speldosor
        { "Init Speldosan", "Speldosor", { { "chamber", speldosan }, { "distMix", 0.0f }, { "rvbMix", 0.25f } }, {} },

        { "Vaggvisa i ruinen", "Speldosor",
          { { "chamber", speldosan }, { "sdBox", 0.8f }, { "sdSpring", 0.35f }, { "sdWorks", 0.3f }, { "distMix", 0.0f },
            { "tpWow", 0.15f }, { "tpAge", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.35f } }, {} },

        { "Kyrkspeldosan", "Speldosor",
          { { "chamber", speldosan }, { "sdType", kyrkspeldosa }, { "sdDecay", 1.6f }, { "sdBox", 0.6f }, { "sdHard", 0.4f },
            { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } }, {} },

        { "Glasdosan", "Speldosor",
          { { "chamber", speldosan }, { "sdType", glasdosa }, { "sdHard", 0.3f }, { "sdBox", 0.3f }, { "sdWorks", 0.1f },
            { "distMix", 0.0f }, { "ecSync", eko8p }, { "ecFeedback", 0.35f }, { "ecDark", 0.5f }, { "ecMix", 0.2f },
            { "rvbMode", hall }, { "rvbMix", 0.35f } }, {} },

        { "Fjädern tar slut", "Speldosor",
          { { "chamber", speldosan }, { "sdSpring", 1.0f }, { "sdRust", 0.55f }, { "sdWorks", 0.7f }, { "sdBox", 0.7f },
            { "tpWow", 0.4f }, { "tpAge", 0.6f }, { "tpDropout", 0.2f }, { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Speldosans vals", "Speldosor",
          { { "chamber", speldosan }, { "sdBox", 0.6f }, { "sdSpring", 0.25f }, { "sdWorks", 0.35f }, { "tsRoot", 72.0f },
            { "tsScale", 1.0f }, { "seqTempo", 84.0f }, { "tsRate", 0.0f }, { "seqLength", 1.0f }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.3f } },
          {}, "0 4 7 . 7 . 12 11 . 7 . 9 . 7 5 4 2 5 9 . 9 . 11 12 . 7 . 4 . 2 0 ." },

        // ------------------------------------------------------------------ Automaten
        { "Init Automaten", "Automaten", { { "chamber", automaten }, { "master", 3.0f }, { "atRepeat", 1.0f }, { "distMix", 0.2f }, { "rvbMix", 0.2f } }, {} },

        { "Varningsrösten", "Automaten",
          { { "chamber", automaten }, { "atWord", ordVarning }, { "atVoice", 0.5f }, { "atMonotone", 1.0f }, { "atRepeat", 1.0f },
            { "atTempo", 0.6f }, { "distDrive", 14.0f }, { "distMix", 0.5f }, { "fxMode", hogpass }, { "fxCutoff", 300.0f },
            { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Doktor Krang talar", "Automaten",
          { { "chamber", automaten }, { "atWord", ordDoktorKrang }, { "atVoice", 0.25f }, { "atMonotone", 0.2f }, { "atTempo", 0.4f },
            { "distMix", 0.1f }, { "rvbMode", hall }, { "rvbMix", 0.35f } }, {} },

        { "Den trasiga roboten", "Automaten",
          { { "chamber", automaten }, { "atWord", ordHjalp }, { "atVoice", 0.55f }, { "atBroken", 0.85f }, { "atRepeat", 1.0f },
            { "distMode", bitkross }, { "distBits", 6.0f }, { "distMix", 0.5f }, { "kaAmount", 0.3f }, { "kaSync", kaos16 },
            { "kaStutter", 1.0f }, { "rvbMix", 0.2f } }, {} },

        { "Viskande maskin", "Automaten",
          { { "chamber", automaten }, { "master", 3.0f }, { "atWord", ordManniska }, { "atVoice", 1.0f }, { "atMonotone", 0.5f },
            { "atTempo", 0.3f }, { "distMix", 0.0f }, { "ecSync", eko4 }, { "ecFeedback", 0.5f }, { "ecMix", 0.3f },
            { "rvbMode", hall }, { "rvbMix", 0.5f } }, {} },

        { "Talande tangenter", "Automaten",
          { { "chamber", automaten }, { "master", 4.0f }, { "atWord", ordTangenterna }, { "atVoice", 0.45f }, { "atMonotone", 0.7f }, { "atTempo", 0.65f },
            { "distMix", 0.2f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        // Doktor Krangs leksaker: mekaniska sångfåglar och urverk som tickar och sjunger.
        { "Sångfågeln i buren", "Automaten",
          { { "chamber", automaten }, { "master", 3.0f }, { "atFigure", sangfageln }, { "atBird", kanarie }, { "atVoice", 0.3f }, { "distMix", 0.0f },
            { "rvbMode", platta }, { "rvbMix", 0.25f } }, {} },

        { "Näktergalen i ruinen", "Automaten",
          { { "chamber", automaten }, { "atFigure", sangfageln }, { "atBird", naktergal }, { "atVoice", 0.25f }, { "atMonotone", 0.4f },
            { "distMix", 0.0f }, { "ecSync", eko8p }, { "ecFeedback", 0.3f }, { "ecDark", 0.6f }, { "ecMix", 0.15f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } }, {} },

        { "Lärkan över fabriken", "Automaten",
          { { "chamber", automaten }, { "atFigure", sangfageln }, { "atBird", larka }, { "atTempo", 0.6f }, { "atBroken", 0.2f },
            { "distMix", 0.1f }, { "tpWow", 0.3f }, { "tpAge", 0.3f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        { "Trasig trast", "Automaten",
          { { "chamber", automaten }, { "atFigure", sangfageln }, { "atBird", trast }, { "atBroken", 0.65f }, { "atVoice", 0.6f },
            { "distMode", bitkross }, { "distBits", 7.0f }, { "distMix", 0.35f }, { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Pendeluret i hallen", "Automaten",
          { { "chamber", automaten }, { "master", 4.0f }, { "atFigure", urverket }, { "atClock", pendelur }, { "atVoice", 0.6f },
            { "atMonotone", 0.9f }, { "atTempo", 0.55f }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.4f }, { "rvbSize", 0.9f } }, {} },

        { "Gökuret", "Automaten",
          { { "chamber", automaten }, { "master", 4.0f }, { "atFigure", urverket }, { "atClock", gokur }, { "atVoice", 0.7f },
            { "atTempo", 0.6f }, { "distMix", 0.0f }, { "rvbMode", platta }, { "rvbMix", 0.25f } }, {} },

        { "Fickuret", "Automaten",
          { { "chamber", automaten }, { "master", 5.0f }, { "atFigure", urverket }, { "atClock", fickur }, { "atVoice", 0.5f },
            { "atTempo", 0.5f }, { "distMix", 0.0f }, { "rvbMode", platta }, { "rvbMix", 0.2f } }, {} },

        { "Klockspelsuret", "Automaten",
          { { "chamber", automaten }, { "master", 4.0f }, { "atFigure", urverket }, { "atClock", klockspelsur }, { "atVoice", 0.75f },
            { "atBroken", 0.15f }, { "distMix", 0.0f }, { "tpWow", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Krigstuban
        { "Init Krigstuban", "Krigstuban", { { "chamber", krigstuban }, { "master", 3.0f }, { "distMix", 0.0f }, { "rvbMix", 0.25f } }, {} },

        { "Domedagens BRAAM", "Krigstuban",
          { { "chamber", krigstuban }, { "ktEnsemble", 1.0f }, { "ktPressure", 0.95f }, { "ktGrowl", 0.3f }, { "ktSwell", 0.35f },
            { "ktFall", 0.4f }, { "ktRumble", 0.8f }, { "ktRelease", 2.0f }, { "distDrive", 10.0f }, { "distMix", 0.3f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 1.0f } }, {} },

        { "Ljudtrattarna", "Krigstuban",
          { { "chamber", krigstuban }, { "ktBell", ljudtratt }, { "ktEnsemble", 0.6f }, { "ktPressure", 0.55f }, { "ktSwell", 0.5f },
            { "ktRumble", 0.4f }, { "ktRelease", 1.5f }, { "distMix", 0.0f }, { "ecSync", eko4 }, { "ecFeedback", 0.4f },
            { "ecDark", 0.7f }, { "ecMix", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.5f }, { "rvbSize", 0.95f } }, {} },

        { "Kapitulationen", "Krigstuban",
          { { "chamber", krigstuban }, { "master", 4.0f }, { "ktEnsemble", 0.3f }, { "ktPressure", 0.4f }, { "ktSwell", 0.7f }, { "ktFall", 1.0f },
            { "ktRelease", 2.5f }, { "ktRumble", 0.2f }, { "distMix", 0.0f }, { "tpWow", 0.3f }, { "tpAge", 0.4f },
            { "rvbMode", hall }, { "rvbMix", 0.4f } }, {} },

        { "Megafonens order", "Krigstuban",
          { { "chamber", krigstuban }, { "ktBell", megafon }, { "ktEnsemble", 0.4f }, { "ktPressure", 0.9f }, { "ktFlutter", 0.3f },
            { "ktSwell", 0.0f }, { "ktRelease", 0.3f }, { "distDrive", 14.0f }, { "distMix", 0.4f }, { "rvbMode", rostigTank }, { "rvbMix", 0.3f } }, {} },

        { "Morrande bas", "Krigstuban",
          { { "chamber", krigstuban }, { "ktEnsemble", 0.2f }, { "ktPressure", 0.7f }, { "ktGrowl", 0.8f }, { "ktFlutter", 0.2f },
            { "ktSwell", 0.05f }, { "ktFall", 0.2f }, { "ktRelease", 0.3f }, { "ktRumble", 0.5f }, { "distMix", 0.2f },
            { "rvbMix", 0.15f } }, {} },

        { "Marschen i ruinerna", "Krigstuban",
          { { "chamber", krigstuban }, { "master", 6.0f }, { "ktRumble", 0.5f }, { "ktEnsemble", 0.45f }, { "ktPressure", 0.75f }, { "ktSwell", 0.0f }, { "ktFall", 0.15f },
            { "ktRelease", 0.2f }, { "tsRoot", 33.0f }, { "seqTempo", 96.0f }, { "tsRate", 1.0f },
            { "distMix", 0.15f }, { "rvbMode", hall }, { "rvbMix", 0.3f } },
          {}, "0a/2 . 0/2 . 0/2 . 3/2 . 5a/5 . 3/2 . 0/2 . -5/2 ." },

        // ------------------------------------------------------------------ Gramofon och megafon
        { "Gammal 78:a", "Gramofon och megafon",
          { { "chamber", stalpiano }, { "master", 4.0f }, { "grCrackle", 0.5f }, { "grHiss", 0.45f }, { "grWow", 0.35f }, { "grAge", 0.75f },
            { "distMix", 0.0f }, { "rvbMode", platta }, { "rvbMix", 0.15f } }, {} },

        { "Skivan som hakar sig", "Gramofon och megafon",
          { { "chamber", speldosan }, { "sdBox", 0.6f }, { "grCrackle", 0.4f }, { "grHiss", 0.3f }, { "grWow", 0.25f }, { "grAge", 0.5f },
            { "grSkip", 0.6f }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.25f } }, {} },

        { "Kalliopen på grammofon", "Gramofon och megafon",
          { { "chamber", angorgeln }, { "master", 3.0f }, { "agType", kalliope }, { "agR16", 0.0f }, { "agR8", 1.0f }, { "agR4", 0.6f }, { "agR2", 0.0f },
            { "agLeak", 0.25f }, { "grCrackle", 0.55f }, { "grHiss", 0.5f }, { "grWow", 0.5f }, { "grAge", 0.85f }, { "distMix", 0.0f },
            { "rvbMix", 0.1f } }, {} },

        { "Megafonrösten", "Gramofon och megafon",
          { { "chamber", automaten }, { "atWord", ordDoktorKrang }, { "atVoice", 0.35f }, { "atRepeat", 1.0f }, { "mgAmount", 1.0f },
            { "mgHorn", 0.6f }, { "mgCrack", 0.5f }, { "mgHum", 0.5f }, { "distMix", 0.0f }, { "ecSync", eko8p }, { "ecFeedback", 0.4f },
            { "ecMix", 0.25f }, { "rvbMode", hall }, { "rvbMix", 0.4f } }, {} },

        { "Propagandaorkestern", "Gramofon och megafon",
          { { "chamber", krigstuban }, { "ktEnsemble", 0.7f }, { "ktPressure", 0.7f }, { "mgAmount", 0.8f }, { "mgHorn", 0.35f },
            { "mgCrack", 0.6f }, { "mgHum", 0.3f }, { "grCrackle", 0.3f }, { "grWow", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.45f } }, {} },

        // ------------------------------------------------------------------ Folkstämningar
        // Stämningen (tuSystem, tuRoot, tuBlue) gäller alla melodiska kammare.
        { "Nyckelharpa i ren stämning", "Folkstämningar",
          { { "chamber", bordunorgeln }, { "boTimbre", 1.0f }, { "boBow", 0.5f }, { "boDrone", 0.5f }, { "boDroneNote", tonD },
            { "boSymp", 0.6f }, { "tuSystem", renStamning }, { "tuRoot", grundBordunen }, { "distMix", 0.0f },
            { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },
        { "Blå kulning", "Folkstämningar",
          { { "chamber", koren }, { "master", 5.0f }, { "koVoices", kulning }, { "koVowel", vokalA }, { "koVibrato", 0.15f }, { "koBreath", 0.15f },
            { "koAttack", 0.08f }, { "koRelease", 0.6f }, { "tuRoot", tonD }, { "tuBlue", 0.8f }, { "distMix", 0.0f },
            { "ecSync", 1.0f }, { "ecFeedback", 0.4f }, { "ecDark", 0.6f }, { "ecMix", 0.25f }, { "rvbMode", hall }, { "rvbMix", 0.45f } }, {} },
        { "Näverlur i naturtoner", "Folkstämningar",
          { { "chamber", lurblasaren }, { "luType", naverlur }, { "luKey", 2.0f }, { "luBreath", 0.5f }, { "luVibrato", 0.1f },
            { "luAttack", 0.12f }, { "tuSystem", naturtoner }, { "tuRoot", tonD }, { "distMix", 0.0f },
            { "ecSync", 1.0f }, { "ecFeedback", 0.35f }, { "ecMix", 0.2f }, { "rvbMode", hall }, { "rvbMix", 0.4f }, { "rvbSize", 0.9f } }, {} },
        { "Medeltonig kyrkorgel", "Folkstämningar",
          { { "chamber", angorgeln }, { "agR16", 0.5f }, { "agR8", 1.0f }, { "agR4", 0.7f }, { "agR2", 0.4f },
            { "agPressure", 0.9f }, { "agChiff", 0.5f }, { "agLeak", 0.05f }, { "agRelease", 0.5f },
            { "tuSystem", medeltonig }, { "tuRoot", tonC }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 1.0f } }, {} },
        { "Pytagoreisk kantele", "Folkstämningar",
          { { "chamber", lutverket }, { "master", 2.0f }, { "ltType", kantele }, { "ltDecay", 8.0f }, { "ltDamper", 1.5f }, { "ltPos", 0.15f },
            { "tuSystem", pytagoreisk }, { "tuRoot", tonD }, { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },
        { "Hackbräde i blått", "Folkstämningar",
          { { "chamber", stalpiano }, { "master", 1.5f }, { "stMode", hackbrade }, { "tuSystem", renStamning }, { "tuRoot", tonG }, { "tuBlue", 0.5f },
            { "distMix", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.3f } }, {} },

        // ------------------------------------------------------------------ Modulation
        // Ljud som bygger på Modulationsraden. LFO-form: 0 sinus, 1 triangel, 2 fyrkant, 3 sågtand, 4 slump, 5 glid.
        // Takt: 0 fri, 3 en takt, 4 1/2, 5 1/4, 6 1/8, 7 1/16. Källa: 1 LFO 1, 2 LFO 2, 3 env 1, 4 env 2, 5 modhjul,
        // 6 aftertouch, 7 anslag. Mål: 1 Klang, 2 Krangdrive, 3 filterfrekvens, 4 filtervokal, 5 volym, 6 eko,
        // 7 katedral, 8 kaos, 9 bandwow, 10 Krangspaken.
        { "Andande bas", "Modulation",
          { { "chamber", basmalen }, { "lfo1Shape", 0 }, { "lfo1Sync", 4 },
            { "mod1Src", 1 }, { "mod1Dst", 1 }, { "mod1Amt", 0.35f },
            { "mod2Src", 7 }, { "mod2Dst", 2 }, { "mod2Amt", 0.3f } }, {} },

        { "Pumpande bordun", "Modulation",
          { { "chamber", bordunorgeln }, { "master", 2.0f }, { "lfo1Shape", 3 }, { "lfo1Sync", 5 },
            { "mod1Src", 1 }, { "mod1Dst", 5 }, { "mod1Amt", -0.3f },
            { "lfo2Shape", 0 }, { "lfo2Rate", 0.08f }, { "mod2Src", 2 }, { "mod2Dst", 1 }, { "mod2Amt", 0.5f },
            { "rvbMix", 0.3f } }, {} },

        { "Kören som andas", "Modulation",
          { { "chamber", koren }, { "lfo1Shape", 5 }, { "lfo1Rate", 0.15f },
            { "mod1Src", 1 }, { "mod1Dst", 1 }, { "mod1Amt", 0.7f },
            { "lfo2Shape", 0 }, { "lfo2Rate", 0.05f }, { "mod2Src", 2 }, { "mod2Dst", 7 }, { "mod2Amt", 0.25f },
            { "rvbMode", hall }, { "rvbMix", 0.35f }, { "rvbSize", 0.85f } }, {} },

        { "Stormen i modhjulet", "Modulation",
          { { "chamber", radiomasten },
            { "mod1Src", 5 }, { "mod1Dst", 10 }, { "mod1Amt", 0.9f },
            { "mod2Src", 6 }, { "mod2Dst", 2 }, { "mod2Amt", 0.6f },
            { "lfo1Shape", 4 }, { "lfo1Sync", 7 }, { "mod3Src", 1 }, { "mod3Dst", 9 }, { "mod3Amt", 0.25f } }, {} },

        { "Hårda slag, mjuka slag", "Modulation",
          { { "chamber", stalpiano }, { "master", 1.0f },
            { "mod1Src", 7 }, { "mod1Dst", 1 }, { "mod1Amt", 0.7f },
            { "env1A", 0.001f }, { "env1D", 1.5f }, { "env1S", 0.0f }, { "env1R", 1.0f },
            { "mod2Src", 3 }, { "mod2Dst", 6 }, { "mod2Amt", -0.3f }, { "ecSync", eko16 }, { "ecMix", 0.35f },
            { "rvbMix", 0.25f } }, {} },

        { "Vokalslingan", "Modulation",
          { { "chamber", spolslingan }, { "fxMode", formant }, { "fxVowel", 0.4f },
            { "lfo1Shape", 1 }, { "lfo1Sync", 3 }, { "mod1Src", 1 }, { "mod1Dst", 1 }, { "mod1Amt", 0.4f },
            { "lfo2Shape", 4 }, { "lfo2Sync", 7 }, { "mod2Src", 2 }, { "mod2Dst", 4 }, { "mod2Amt", 0.35f } }, {} },

        // ------------------------------------------------------------------ Slingor (med melodi)
        // Melodier för Tonstegverket: tryck SPELA under STEGVERKET (eller starta DAW:en). Tangenterna transponerar.
        { "Acidslinga i pannrummet", "Slingor (med melodi)",
          { { "chamber", basmalen }, { "seqTempo", 124.0f }, { "glide", 70.0f }, { "cutoff", 380.0f }, { "resonance", 0.8f },
            { "envAmount", 0.7f }, { "fltD", 0.18f }, { "ampS", 0.8f }, { "distMode", ror }, { "distDrive", 20.0f }, { "distMix", 0.7f },
            { "ecSync", eko8p }, { "ecFeedback", 0.35f }, { "ecMix", 0.18f } },
          {}, "0a 0~ 12 0 . 0 10~ 12 0a . 3 0 7~ 5 3a 0" },

        { "Speldosan i ruinen", "Slingor (med melodi)",
          { { "chamber", klockgjuteriet }, { "klType", skalklocka }, { "klDecay", 3.0f }, { "tsRoot", 60.0f }, { "tsScale", 4.0f },
            { "seqTempo", 92.0f }, { "tpWow", 0.35f }, { "tpAge", 0.4f }, { "rvbMode", hall }, { "rvbMix", 0.35f } },
          {}, "0 7 12 . 4 . 9 7 . 12 . 4 2 . 0 ." },

        { "Kantelevisan", "Slingor (med melodi)",
          { { "chamber", lutverket }, { "master", 2.0f }, { "ltType", kantele }, { "ltDecay", 6.0f }, { "tsRoot", 50.0f }, { "tsScale", 2.0f },
            { "seqTempo", 84.0f }, { "tsRate", 0.0f }, { "rvbMode", hall }, { "rvbMix", 0.3f } },
          {}, "0 . 3 5 7a . 5 3 2 . 0 . -2 . 0/5 ." },

        { "Kulningsslingan", "Slingor (med melodi)",
          { { "chamber", koren }, { "koVoices", kulning }, { "tsRoot", 62.0f }, { "tsScale", 2.0f }, { "seqTempo", 70.0f },
            { "rvbMode", hall }, { "rvbMix", 0.45f }, { "rvbSize", 0.9f } },
          {}, "0~/7 . . . 7~/7 . . . 5~/5 . . 3~/3 . 0/7 . ." },

        { "Sågen i schaktet", "Slingor (med melodi)",
          { { "chamber", vattenpumpen }, { "vpType", sag }, { "vpGlide", 0.25f }, { "vpVibrato", 0.4f }, { "tsRoot", 52.0f },
            { "seqTempo", 76.0f }, { "tsRate", 0.0f }, { "distMix", 0.0f }, { "rvbMode", platta }, { "rvbMix", 0.35f } },
          {}, "0~/5 . 7~/5 . 5~/3 3~ 2/5 . 0~/5 . -2~/5 . 0/7 . . ." },

        { "Psalmodikon", "Slingor (med melodi)",
          { { "chamber", bordunorgeln }, { "tsRoot", 50.0f }, { "tsRate", 0.0f }, { "seqTempo", 66.0f }, { "tsScale", 2.0f },
            { "rvbMode", hall }, { "rvbMix", 0.4f } },
          {}, "0/7 . . . 2/5 . 3/5 . 2/7 . . . 0~/7 . . ." },

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

        { "Ekande smedja", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 100.0f }, { "distDrive", 10.0f },
            { "tpAge", 0.5f }, { "tpFlutter", 0.3f },
            { "ecSync", eko16 }, { "ecFeedback", 0.5f }, { "ecDark", 0.65f }, { "ecMix", 0.3f } },
          { "2000000020100000", "0000000000000000", "0000200000002000", "0000000000000000",
            "0000000000000000", "1010101010101010", "0000000000000000", "0000000000000000" } },

        { "Trasig trummaskin", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 118.0f }, { "distMode", bitkross }, { "distBits", 6.0f },
            { "distMix", 0.5f }, { "hvRost", 0.4f },
            { "kaAmount", 0.4f }, { "kaSync", kaos16 }, { "kaDrift", 0.0f }, { "kaStutter", 1.0f }, { "kaReverse", 0.4f },
            { "kaCrush", 0.5f } },

          { "2000001020000000", "0000000000000010", "0000200000002000", "0000000000000000",
            "0000000000000000", "1011101110111011", "0000000000000000", "0000001000000100" } },

        { "Fabrikshallen", "Trummor",
          { { "chamber", hammarverket }, { "hv0Sound", 11.0f }, { "hv1Sound", 8.0f }, { "hv2Sound", 2.0f }, { "hv3Sound", 10.0f }, { "hv4Sound", 9.0f }, { "hv5Sound", 5.0f }, { "hv6Sound", 12.0f }, { "hv7Sound", 13.0f }, { "seqTempo", 118.0f }, { "distDrive", 10.0f }, { "rvbMode", rostigTank }, { "rvbMix", 0.22f } },
          { "2000001020100000",
            "0000000000001{t+5}001{c2}",
            "0000200000002001{r1}",
            "0001{c2}000000010000",
            "00000000000001{t-5}1{t-3}0",
            "10101011{r1}10101011{c1}",
            "0000000000000010",
            "0000000001{c1}000000" } },
        { "Folkdans med bjällror", "Trummor",
          { { "chamber", hammarverket }, { "hv0Sound", 15.0f }, { "hv1Sound", 14.0f }, { "hv2Sound", 4.0f }, { "hv3Sound", 3.0f }, { "hv4Sound", 9.0f }, { "hv5Sound", 7.0f }, { "hv6Sound", 6.0f }, { "hv7Sound", 10.0f }, { "seqTempo", 104.0f }, { "seqSwing", 0.18f }, { "rvbMode", hall }, { "rvbMix", 0.25f } },
          { "2000001000200000",
            "1001{c1}001001{c1}001001{c2}",
            "0000200000002000",
            "0000000000001000",
            "000000000000001{t+3}1",
            "101101101101101{r1}1",
            "0000000000000000",
            "0000000000000001{c3}" } },
        { "Ruinens maskineri", "Trummor",
          { { "chamber", hammarverket }, { "hv0Sound", 11.0f }, { "hv1Sound", 13.0f }, { "hv2Sound", 12.0f }, { "hv3Sound", 8.0f }, { "hv4Sound", 9.0f }, { "hv5Sound", 10.0f }, { "hv6Sound", 6.0f }, { "hv7Sound", 15.0f }, { "seqTempo", 96.0f }, { "tpWow", 0.3f }, { "tpAge", 0.4f }, { "ecSync", eko8p }, { "ecFeedback", 0.45f }, { "ecMix", 0.2f }, { "rvbMix", 0.25f } },
          { "2001{c2}0010002001{c3}00",
            "000000000000001{c2t-7}0",
            "00001001{c3}00001{d6}000",
            "01{c2t+2}000001{c3}01{c2t-3}000000",
            "000000000001{t-5c1}0001{c2}",
            "1{c1}01{c2}01{c1}01{c3}01{c1}01{c2}01{c1r2}01{c3}0",
            "0000000000000000",
            "000000001{c2}0000000" } },
        { "Ratchetsmedjan", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 124.0f }, { "distDrive", 12.0f }, { "distMix", 0.5f }, { "rvbMode", platta }, { "rvbMix", 0.15f } },
          { "200010002000101{r1}0",
            "0000000000000001{r3c1}",
            "0000200000002001{r2}",
            "0000000000000000",
            "001{t+7}0001{t+5}0001{t+3}0001{t}0",
            "101{r1}0101{r2}0101{r1}0101{r3}0",
            "01{d2}0001{d2}0001{d5}0001{d2}00",
            "0000000000000000" } },
        { "Slumpens trummaskin", "Trummor",
          { { "chamber", hammarverket }, { "seqTempo", 110.0f }, { "hvRost", 0.5f }, { "distMode", bitkross }, { "distBits", 8.0f }, { "distMix", 0.3f } },
          { "2001{c2}001{c1}0201{c2}001{c3}00",
            "00000000000001{c3}01{c3}",
            "00002001{c3}0000201{c2}0",
            "0000000000001{c1}000",
            "01{c3}0001{c3}0001{c3}00001{c2}1{c2}",
            "11{c2}1{c1}1{c2}11{c2}1{c1}1{c2}11{c2}1{c1}1{c2r1}11{c2}1{c1}1{c3r2}",
            "0000000000000000",
            "0001{c2}000001{c2}000000" } },
    };
    return list;
}

} // namespace krang::presets
