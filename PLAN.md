# KrangBox – plan

*Dr. Krangs Patenterade Ljudmaskin. En trasig synthbox från en dystopisk framtid, byggd av en uppfinnare ur en Jules Verne-roman av en Roland-rack, en hög folkinstrument, ångrör, radiorör och kopparspolar.*

**Status:** Fas 3d pågår (Krigstuban, Gramofonen och Megafonen, folkliga stämningar och grafikrundan klara 2 november 2026). Grind 3 är förberedd men inte godkänd: kvar är AU och CLAP i pluginval på Macen, lyssningsrundan i Reaper och Doktor Krangs ja. Planen flyttades hit från Claude-dokumentet 6 november 2026.

Instrumentplugin (VST3, AU, CLAP) och fristående app för macOS, den första i Ljudwerk och förebilden för syskonen. Allt i gränssnittet är på svenska.

*KrangBox är äldre än Ljudwerks faser och grindar och har egna fasnamn (MVP, Krang-panelen, fler kammare). Avvikelser från [styrdokumenten](../ljudwerk-docs/styrning/arbetsgang.md) står i kursiv nedan (6 november 2026).*

## 1. Mål

**KrangBox ska ge producenter av industrial, darkwave, dark folk, techno och film- och spelmusik karaktär direkt ur lådan: trasigt, mättat och bitreducerat ljud med klangfärger från gammal folkmusik – trummor, basar, slingor, drön och texturer.**

Måttstock: Hedningarna, Wardruna, Nine Inch Nails, Swans, Boards of Canada; i gränssnittet tunga 80–90-talssynthar (Jupiter-8, DX7, Korg M1, rackmoduler). Vi kopierar inget; vi lyssnar på dem för att veta när vi är framme.

Det som måste finnas:

- **Helt syntetiskt ljud:** inga samplingar; folkklangerna byggs med fysisk modellering, modalsyntes och formantfilter.
- **Femton kammare** som delar effektkedja och modulation; varje preset väljer en kammare.
- **Krangspaken:** en makrospak som förstör allt på en gång, nivåkompenserad.
- **Spelbar utan DAW:** fristående app med keyboard, datorns tangenter och egen klocka för Stegverken.

### Inte i 1.0

- Flera kammare i lager
- MPE, MIDI learn
- Fritt sorterbar effektkedja (ordningen är fast)
- Windows och Linux

## 2. Instrumentet

```
 MIDI ─▶ STEGVERKEN / SLINGAN ─▶ KAMMARE ─▶ KAOSMOTORN ─▶ KRANGDISTORTION ─▶ ÅNGFILTER ─▶ MEGAFONEN
                                    ▲                                                         │
            STÄMNINGEN ─────────────┘     ┌───────────────────────────────────────────────────┘
            MODULATIONSRADEN ──▶ allt     ▼
                                    BANDMASKINEN ─▶ GRAMOFONEN ─▶ EKOKAMMAREN ─▶ KATEDRALREVERB ─▶ LIMITER (−0,3 dBFS) ─▶ UT
```

### Kamrarna

| # | Kammare | Roll | Teknik |
| --- | --- | --- | --- |
| 0 | Basmålen | Bas | Virtuell analog, 2 osc + sub, ladder-filter, fyra distortionslägen |
| 1 | Hammarverket | Trummaskin, 8 spår | 16 syntetiska trumljud, fyra kit, Stegverket med chans, ratchets och lås |
| 2 | Bordunorgeln | Drön och pads | Vågtabeller (vallflöjt → nyckelharpa), bordun, sex resonanssträngar |
| 3 | Spolslingan | Leads och slingor | FM-spolen eller Fasvridaren (CZ), munharpa, radiokorn, arpeggiatorn Slingan |
| 4 | Stålpiano | Piano, hackbräde, preparerat | Modalsyntes, sex strängmaterial (stål till djurhud), pedal |
| 5 | Klockgjuteriet | Klockor, gonger, metall | Modalsyntes med oharmoniska deltoner |
| 6 | Kören i schaktet | Kör och kulning | Formantsyntes, 1–8 sångare per ton, trasig högtalare |
| 7 | Radiomasten | Brus och textur | Kortvåg, morse, vind i ledningar, interferens |
| 8 | Lurblåsaren | Blås | Vågledare: näverlur, bockhorn, skalmeja, ångvissla |
| 9 | Lutverket | Knäppta strängar | Karplus-Strong: kantele, luta, cittra, rostharpa |
| 10 | Vattenpumpen | Stråkad metall | Vattenfon, såg, plåt, cymbal |
| 11 | Ångorgeln | Orgel | Fem pipregister, kyrkorgel, kalliope, harmonium, positiv; ångtryck |
| 12 | Speldosan | Speldosa | Stålstavar med modalsyntes, låda, fjäder, rost, urverk |
| 13 | Automaten | Doktor Krangs leksaker | Talmaskinen (formantsyntes), Sångfågeln, Urverket |
| 14 | Krigstuban | En mur av mässing | Fysiskt modellerat bleckblås, upp till 8 blåsare per ton, tre trattar |

### Spel och modulation

- **Stegverket** (Hammarverket): 16–64 steg, accent, gung, chans, ratchet, lås för stämning och klang (CC 102/103).
- **Tonstegverket** (övriga kammare): upp till 64 steg med ton, längd, glid och accent; skalor, SLUMPA; transponeras från tangenterna.
- **Slingan:** arpeggiator med åtta lägen, tonlängd och Fäst.
- **Modulationsraden:** två LFO:er, två envelopes, modhjul, aftertouch, anslag och en matris med sex kopplingar.
- **Stämningen:** liksvävande, ren, pytagoreisk, medeltonig, naturtoner; grundton eller bordunen; blå toner.

### Effekterna (Ångledningen och kaosavdelningen)

Krangdistortion (Rör, Fuzz, Vikning, Bitkross), Ångfilter, Bandmaskinen, Ekokammaren, Katedralreverb; Kaosmotorn, Megafonen och Gramofonen (spaken Spelar: Alltid / Vid ljud). Effekt-DSP:n ligger i krang-libs och delas med Ångledningen (ÅL-1889).

### Krangspaken

Drar allt mot sönderfall: drive, bitkross, drift, resonans, rost, bandslitage, självsvängande eko, Kaosmotorn, megafonens spricka, skivans hopp och reverb. Nivån kompenseras.

## 3. Gränssnittet – Krang, verkstaden

### Idén

**En rackmodul från 1987 ombyggd i Krangs verkstad, med jugendplaketter inspirerade av Alphonse Mucha.**

- **Chassit:** åldrad, unik plåt med relief, repor och rost som rinner från skruvarna; mässingslister och nitar.
- **Reglagen:** mässingsringade bakelitrattar, faders, VFD-display, VU-mätare, glödande radiorör, Krangspaken som tung spak.
- **Instrumenten:** varje kammare har en emaljerad plakett med välvd nisch, gyllene gloria och blombårder; motiven följer rattarna.

Färger: svartgrått chassi, mässing och koppar, rörorange; pergament, guld, salvia och rosa i plaketterna.

### Layout (designstorlek 1200 × 840, skalas 60–160 %)

```
┌──────────────────────────────────────────────────────────────────────┐
│ DR. KRANGS PATENTERADE LJUDMASKIN  STÄMNING [▾][▾] BLÅ   PRESET ◀[▾]▶ SPARA │
│ [VFD]  KRANGDRIVE  KATEDRAL  MASTER     KRANGSPAKEN  [rör]  [VU]           │
├──────────────────────────────────────────────────────────────────────┤
│ KAMMAREN – eller STEGVERKET, MODULATION, KAOS (vyknapparna)          │
├──────────────────────────────────────────────────────────────────────┤
│ ÅNGLEDNINGEN: KRANGDISTORTION ─ ÅNGFILTER ─ BANDMASKINEN ─ EKO ─ KATEDRAL │
├──────────────────────────────────────────────────────────────────────┤
│ OKT- OKT+   …                     KAMMARE STEGVERKET MODULATION KAOS │
│ [keyboard C1–C7]                                                     │
└──────────────────────────────────────────────────────────────────────┘
```

### Teknik

- Allt ritas i kod, inga bild- eller typsnittsfiler (`Source/ui/`, Krang-stilen och jugendillustrationerna i krang-libs).
- Bakgrunden cachas; bara mätare, lampor, Krangskopet och Automatens figur ritas om (högst 30 bilder/s).
- `KrangSnapshot` ritar panelen, kamrarna och illustrationerna till PNG och kör klicktestet.

## 4. Kod och struktur

JUCE 8, CMake, clap-juce-extensions, C++20, AGPLv3. Namnrum `krang`. Parameter-ID:n i camelCase utan prefix (`cutoff`, `glide`, `ktBell`) – äldre än Ljudwerks regel och ändras inte.

```
krang-box/
  Source/            PluginProcessor, PluginEditor, Parameters.h
  Source/dsp/        kamrarna, Stegverken, Slingan, Modulation, Stamning, KrangEngine
  Source/ui/         panelerna (tunna omslag runt krang-libs stil)
  Source/presets/    FactoryPresets.h, PresetManager.h (.krang-filer)
  tools/             KrangRender, KrangSnapshot, reaper/make_reaper_projects.py
  Reaper/            testprojekten (tester/) och trummönster (trummönster/)
  docs/              panelbilder
```

| Från / till krang-libs | Ursprung | Fas |
| --- | --- | --- |
| `krang/dsp/KrangDistortion.h`, `Angledningen.h`, `Bandmaskin.h`, `Kaosmotorn.h`, `Gramofonen.h`, `Brusgrind.h` | KrangBox → krang-libs (beslut 4 november 2026) | 3 |
| `krang/stil/krang/KrangStyle.h`, `Textures.h`, `Jugend.h` | KrangBox → krang-libs | 3 |
| `krang/ui/ValueFormat.h` | KrangBox → krang-libs | 3d |

Regler:

- **Parameter-ID:n får aldrig döpas om**; nya läggs sist, och nya val i listor läggs sist.
- Ljudtråden allokerar inget och låser inget (Reader läser förhämtade atomära pekare).
- Varje del får ett testfall i `KrangRender --test` innan den räknas som klar; panelen i `KrangSnapshot --klicktest`.

## 5. Faser och grindar

*KrangBox faser i den ordning de byggdes. Motsvarigheten i Ljudwerk: fas 0–1 här ≈ fas 0–2, fas 2 ≈ fas 3 (panelen), fas 3–3d är fler kammare och fördjupning före fabriksbandet (fas 4).*

### Fas 0 – Prototypen

- Basmålen, Krangdistortion med översampling, stilprovet, testgränssnittet, keyboardet.

**Grind 0:** Blint lyssningstest av distortionslägena (karaktär, folkkänsla, tydliga lägen, spelbarhet, i en mix, teknik).

*Tekniska kriteriet uppfyllt 31 oktober 2026 (lägena inom 1,5 dB). Lyssningsdelen är inte genomförd; referenslistan är inte ifylld.*

### Fas 1 – MVP (Basmålen och Hammarverket)

- Hammarverket, Stegverket, Ångfilter och Katedralreverb, presetsystemet, 21 presets.

*Klar 31 oktober 2026. Ingen formell grind; finslipning efter lyssning återstår.*

### Fas 2 – Krang-panelen

- Egen frontpanel, moduler för alla delar, skalning 60–160 %, genererade texturer, Krangspaken och radioröret.

**Grind 2:** Användartest med producenter.

*Uppskjuten (beslut 1 november 2026).*

### Fas 3 – Fler kammare

- Bordunorgeln, Spolslingan, Bandmaskinen och Ekokammaren, Kaosmotorn och Krangskopet, Stålpiano.

*Klar 1 november 2026.*

### Fas 3b – Sex nya kammare (beslut 1 november 2026)

- Klockgjuteriet, Kören i schaktet, Lurblåsaren, Lutverket, Vattenpumpen, Radiomasten.

*Klar 1 november 2026.*

### Fas 3c – Fördjupning (beslut 1 november 2026)

- Modulationsraden, Tonstegverket, Hammarverket med 16 ljud och kit, Fasvridaren och Slingans nya lägen, Ångorgeln, Speldosan, Automaten med Sångfågeln och Urverket.

*Klar 2 november 2026.*

### Fas 3d – Förbättringar (beslut 2 november 2026)

1. ~~Krigstuban~~ – *klar 2 november 2026*
2. ~~Gramofonen och Megafonen~~ – *klar 2 november 2026; spaken Spelar (önskemål 2 november 2026)*
3. ~~Folkliga stämningar~~ – *klar 2 november 2026*
4. ~~Grafikrundan~~ – *klar 2 november 2026: värdeformat, Mucha-plaketter, nya illustrationer, kaosavdelningen (KAOS), panelen 1200 × 840*
5. Snapshot A/B med morfning
6. Doktor Krangs tärning
7. Sökbar presetlista med favoriter
8. Lyssningsrunda och presetputs

*Pågår. Gjort 6 november 2026: PLAN.md i repot; `KrangRender --test` (465 kontroller, alla går igenom) och `KrangSnapshot --klicktest` (701 kontroller, alla går igenom: 278 reglage i alla femton kammare och vyer, alla 271 parametrar nås från panelen, dubbelklick ger standardvärdet överallt); CI kör båda och hämtar krang-libs.*

**Grind 3:** CPU inom budget, inga krascher i pluginval (VST3, AU, CLAP), alla parametrar nåbara från panelen (klicktestet), lyssningsrundan i Reaper godkänd av Doktor Krang.

*Förberedd: VST3 klarar pluginval nivå 5 på Linux; hela kedjan ca 1,5 % av en kärna, tyngsta kammaren (Krigstuban, 48 blåsare) ca 16 %; klicktestet når alla parametrar (6 november 2026). Kvar: AU och CLAP på Macen, tangentklick och datorns tangenter på Macen, lyssningsrundan.*

### Fas 4 – Release (1.0)

- Fabriksbandet putsat (152 presets i dag), svensk manual med bilder, Reaper-guide, pluginval nivå 5, auval, kodsignering och notarisering.

**Grind 4:** Någon som inte byggt KrangBox gör ett spår med bara fabriksbandet och manualen.

*Släppet väntar på ett Apple Developer ID (beslut 1 november 2026).*

### Fas 5 – Efter 1.0 (kandidater)

- Flera kammare i lager, MIDI learn, MPE, fritt sorterbar effektkedja.

## 6. Risker

| Risk | Motåtgärd |
| --- | --- |
| Omfånget växer (femton kammare) | Fas 3d avslutas med lyssningsrundan; nya kammare kräver beslut |
| CPU i de fysiska modellerna | CPU-test per kammare i `KrangRender --test`, mål under 25 % av en kärna |
| Aliasing i distortion | 2–8× översampling, ADAA i vikningen |
| Kodsignering och notarisering | Apple Developer ID före release |
| Grafiken tar längre tid än DSP | Funktionellt först, skinn sist; allt ritas i kod |
| Ändrad Krang-kod i krang-libs bryter Ångledningen | Kör `ALRender` när krang-libs ändras |

## 7. Beslut

- **Endast macOS**, ingen iPad-version för tillfället (beslut 1 november 2026).
- **Öppen källkod**, AGPLv3 (beslut 1 november 2026).
- **Helt syntetiskt ljud**, inga samplingar; **genererad grafik** (beslut 1 november 2026).
- **Reaper** är DAW:en för test (beslut 1 november 2026).
- **Stegverket för basgångar** väntar till efter MVP:n (beslut 31 oktober 2026); löst genom Tonstegverket.
- **Användartesterna (grind 2) skjuts upp** (beslut 1 november 2026).
- **Fem kammare med Stålpiano**, sex strängmaterial (beslut 1 november 2026); **sex kammare till** i fas 3b (beslut 1 november 2026).
- **Inget Developer ID ännu; vi bygger vidare** före release (beslut 1 november 2026).
- **Hammarverkets MIDI-noter hör till spåren**, inte ljuden; kit är en knapp, inte en parameter (beslut 2 november 2026).
- **Automaten är Doktor Krangs mekaniska leksaker** med talmaskinen som en figur (beslut 2 november 2026).
- **Fas 3d: alla förbättringsförslag in i planen, Krigstuban först** (beslut 2 november 2026).
- **Effekters eget brus kan höras bara när det spelas** – spaken Spelar (önskemål 2 november 2026).
- **Illustrationerna i jugend inspirerad av Alphonse Mucha** (beslut 2 november 2026).
- **Kaosraden blir en egen avdelning** (KAOS); Kaosskopet heter Krangskopet (beslut 2 november 2026).
- **Effekt-DSP:n och Krang-stilen flyttas till krang-libs** och delas med Ångledningen (beslut 4 november 2026).
- **Nya presetkategorier mitt i listan flyttar DAW:ens programnummer**; accepterat före release (beslut 2 november 2026).

Öppna frågor:

- Windows och Linux efter release, eller bara macOS?
- Vem tar emot bidrag och granskar pull requests på GitHub?
- Referenslistan för grind 0 (Doktor Krang fyller i).
