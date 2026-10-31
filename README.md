# KrangBox

*Dr. Krangs Patenterade Ljudmaskin* – en trasig synthbox från en dystopisk framtid, byggd av en uppfinnare ur en Jules Verne-roman, med klangfärger från gammal folkmusik.

Instrument-plugin (VST3, AU, CLAP) och fristående app för macOS. Allt ljud är syntetiskt.

**Status: fas 3 pågår.** Fyra kammare: Basmålen (bas), Hammarverket (trummaskin med Stegverket), Bordunorgeln (drön och folkklanger) och Spolslingan (leads och slingor med arpeggiatorn Slingan). Alla parametrar sitter på Krang-panelen, som skalas proportionellt mellan 60 och 160 %.

![Krang-panelen med Basmålen](docs/krangpanelen_basmalen.png)

## Krang-panelen

Panelen ritas i en fast designstorlek (1200 × 850) och skalas när du drar i fönstrets hörn. Uppifrån:

- **Frontpanelen:** display, kammarväljare, presets och tre stora rattar som gäller båda kamrarna: **Krangdrive** (distortionens drive), **Katedral** (reverbens mix) och **Master**. Bredvid sitter **Krangspaken** och ett radiorör som glöder starkare ju hårdare maskinen drivs. VU-mätaren visar utnivån.

### Krangspaken

En makrospak som drar hela maskinen mot sönderfall på en gång: mer drive och full distortion-mix, grövre bitkross, mer drift (tonen glider ur stämning), högre filterresonans, mer rost i trummorna och mer och mörkare reverb. Nivån kompenseras så att spaken förstör ljudet utan att bara höja volymen. Spaken är en vanlig parameter (*Krangspaken*, 0–100 %) och kan automatiseras i DAW:en; dubbelklick sätter den i botten.
- **Kammaren:** med Basmålen vald visas oscillatorer, rörelse, filter och två envelopes (faders). Med Hammarverket vald visas Stegverket och, i linje med varje spår, rösternas Stämning, Klang och Nivå, samt Rost.
- **Ångledningen:** Krangdistortion → Ångfilter → Katedralreverb, sammankopplade med kopparrör. Läget väljs i listan i varje modul.
- **Keyboardet** längst ned.

Displayen visar presetets namn och den parameter du senast rörde. Dubbelklick på en ratt eller fader återställer standardvärdet; små rattar och faders visar värdet medan du drar.

![Krang-panelen med Hammarverket](docs/krangpanelen_hammarverket.png)

### Texturer

Alla texturer genereras i kod (`Source/ui/Textures.h`) med fasta frön, i dubbel upplösning så att de håller vid 160 % skala: borstad chassimetall, modulplåt med nötta partier, rost och smuts, åldrat mätarpapper med en kaffefläck, korn i VFD-glaset och svarvspår i rattarnas mässing. Statiska bakgrunder ritas en gång och sparas som bild, så en hel omritning tar ungefär 20 ms (mjukvarurendering) och vanliga uppdateringar bara en bråkdel av det.

**Byta mot genererade bilder (Philip):** varje textur är en funktion (`chassisMetal()`, `plate()`, `rust()`, `meterPaper()`) som returnerar en bild. Lägg en sömlös PNG, gärna 512 × 512 eller större, i `Resources/` och låt funktionen returnera den i stället; resten av gränssnittet ändras inte. `KrangSnapshot … --tid` mäter rittiden efteråt.

## Bygga lokalt

### Det här behöver du

| Vad | Version | Installera |
| --- | --- | --- |
| Mac | Apple Silicon eller Intel | – |
| macOS | 14 Sonoma eller senare för att bygga (pluginet körs på macOS 11+) | – |
| Xcode | 15 eller senare | App Store, starta en gång och godkänn licensen |
| Xcodes kommandoradsverktyg | – | `xcode-select --install` |
| CMake | 3.22 eller senare | `brew install cmake` |
| Git | – | ingår i kommandoradsverktygen |
| Ninja (valfritt) | – | `brew install ninja`, snabbare bygge under utveckling |
| pluginval (valfritt) | – | [github.com/Tracktion/pluginval](https://github.com/Tracktion/pluginval/releases) |

Homebrew installeras från [brew.sh](https://brew.sh). Första gången du konfigurerar hämtas JUCE 8 och clap-juce-extensions från GitHub, så du behöver internet. Har du redan JUCE lokalt kan du peka på det i stället:

```sh
cmake -B build -G Xcode -DFETCHCONTENT_SOURCE_DIR_JUCE=/sökväg/till/JUCE
```

### Bygga allt (Release, universal binary)

```sh
git clone https://github.com/mikkeschiren/KrangBox.git
cd KrangBox
cmake -B build -G Xcode
cmake --build build --config Release --target KrangBox_All
```

`KrangBox_All` bygger VST3, AU, CLAP och den fristående appen. Byggena hamnar i `build/KrangBox_artefacts/Release/`.

### Snabbare bygge under utveckling

Bygg bara för din egen processor och med debug-information:

```sh
cmake -B build-dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-dev --target KrangBox_Standalone
open build-dev/KrangBox_artefacts/Debug/Standalone/KrangBox.app
```

Byt `arm64` mot `x86_64` på en Intel-Mac.

### Byggmål

| Mål | Bygger |
| --- | --- |
| `KrangBox_All` | Alla format nedan |
| `KrangBox_VST3` | `KrangBox.vst3` |
| `KrangBox_AU` | `KrangBox.component` |
| `KrangBox_CLAP` | `KrangBox.clap` |
| `KrangBox_Standalone` | `KrangBox.app` |
| `KrangRender` | Verktyget för lyssningstest (se nedan) |
| `KrangSnapshot` | Ritar stilprovet till PNG: `KrangSnapshot docs/stilprov.png 2 docs/stegverket.png --tid` (med `--tid`: mäter rittiden) |

### Installera i din DAW

```sh
cp -R build/KrangBox_artefacts/Release/VST3/KrangBox.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/KrangBox_artefacts/Release/AU/KrangBox.component ~/Library/Audio/Plug-Ins/Components/
mkdir -p ~/Library/Audio/Plug-Ins/CLAP
cp -R build/KrangBox_artefacts/Release/CLAP/KrangBox.clap ~/Library/Audio/Plug-Ins/CLAP/
```

Om Logic inte hittar AU-versionen: kör `killall -9 AudioComponentRegistrar` och starta om Logic. Lokala byggen är inte signerade; macOS kan kräva att du godkänner dem under Systeminställningar > Integritet och säkerhet.

### Felsökning

**`No CMAKE_C_COMPILER could be found` med `-G Xcode`:** Xcode-generatorn kräver hela Xcode, inte bara kommandoradsverktygen. Kontrollera med `xcode-select -p`. Visar den `/Library/Developer/CommandLineTools`, peka om till Xcode och töm byggkatalogen:

```sh
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
xcodebuild -runFirstLaunch
rm -rf build
cmake -B build -G Xcode
```

Utan Xcode installerat går det att bygga med bara kommandoradsverktygen och Ninja:

```sh
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KrangBox_All
```

Byggena hamnar då i `build/KrangBox_artefacts/Release/` på samma sätt.

### Validera med pluginval

```sh
pluginval --strictness-level 5 --validate build/KrangBox_artefacts/Release/VST3/KrangBox.vst3
```

## Lyssningstest

```sh
cmake --build build --config Release --target KrangRender
./build/KrangRender_artefacts/Release/KrangRender renders
```

Renderar en basslinga i D-dorisk genom alla fyra distortionslägen (Rör, Fuzz, Vikning, Bitkross), en trumslinga från Hammarverket (torr, Rör och Bitkross), varje trumröst för sig, varje filter- och reverbläge (med hög resonans och stort rum, som stresstest) Stegverkets startmönster och alla fabrikspresets till WAV. Testar också Stegverkets timing mot en simulerad DAW, den interna klockan och gung, på sample-nivå, att alla fabrikspresets ligger mellan −16 och −7 dBFS RMS, och att tre presets förblir stabila med Krangspaken i topp, att Slingan spelar tonerna i rätt ordning och på rätt sample, och att Bordunorgeln med åtta röster håller sig under 25 % av en processorkärna. Kontrollerar NaN, toppnivå, DC och att Basmålens lägen ligger inom 1,5 dB från varandra i nivå.

## Hammarverket

Välj **Hammarverket** i kammarväljaren under displayen på frontpanelen. Åtta helt syntetiska trumröster, var och en med Stämning, Klang (längd) och Nivå, plus *Rost* som lägger skrap och brus på alla röster.

| MIDI-not | Röst | Vad det är | Datortangent (från C2) |
| --- | --- | --- | --- |
| 36 (C2) | Ångslaget | Bastrumma: sinus med tonhöjdsfall, klick och mättning | A |
| 37 (C#2) | Skrotplåten | Rostig plåt: ringmodulerade oharmoniska toner och skrap | W |
| 38 (D2) | Smedjan | Virvel: skinn, sejare och en klang av städ | S |
| 39 (D#2) | Klappern | Handklapp: fyra brusskurar och en svans | E |
| 41 (F2) | Ramtrumman | Folklig ramtrumma: membranets grundton och överton | F |
| 42 (F#2) | Ventilen | Stängd hihat; stryper Ångröret | T |
| 46 (A#2) | Ångröret | Öppen hihat | U |
| 75 (D#5) | Träskedarna | Skedspel: två resonanta träklick | – (musen) |

Noterna 35, 40, 43, 44, 45 och 76 går också till närmaste röst, enligt General MIDI.

## Stegverket

När Hammarverket är valt visas Stegverket under frontpanelen: ett rutnät med ett spår per trumröst och 16 steg per sida.

![Stegverket](docs/stegverket.png)

- **Klick** på en lampa växlar av, slag och accent. **Shift-klick** ger accent direkt. **Dra** för att måla samma värde över flera steg.
- **SPELA** startar och stoppar. I den fristående appen går Stegverket på egen klocka med **Tempo**. I en DAW följer det DAW:ens tempo och position när SPELA är på och DAW:en spelar.
- **Gung** låter varannan 16-del släpa, upp till 100 % (en halv 16-del).
- **Steg** sätter längden till 16, 32, 48 eller 64 steg; **Sida** 1–4 väljer vilka 16 steg som visas.
- **RENSA** tömmer mönstret och **STANDARD** laddar startmönstret. Mönstret sparas med DAW-projektet.

## Bordunorgeln

Välj **Bordunorgeln** i kammarväljaren. En polyfon drönmotor (8 röster) med bandbegränsade vågtabeller:

| Kontroll | Vad den gör |
| --- | --- |
| Klangfärg | Glider från vallflöjt (mjuk, udda deltoner) via säckpipa (nasal rörbladston) till nyckelharpa (stråkton med kroppsresonanser) |
| Stråke | Smalbandigt brus kring tredje deltonen, som stråke eller andning |
| Drift | Långsam ostämdhet per röst |
| Bordun + Bordunton | En grundton och kvint i stora oktaven som ligger kvar så länge du håller en tangent |
| Resonans (resonanssträngar) | Sex sympatiska strängar, fysiskt modellerade och stämda efter bordunen (grundton, kvint, oktav, ters, kvart, kvint), som klingar med – som understrängarna på en nyckelharpa |
| Attack, Release | Envelope för alla röster |

Åtta röster använder ungefär 3 % av en processorkärna (mätt i Linux-bygget). Fem presets finns under *Drön och folk*: Bordunorgeln, Nyckelharpan i ruinen, Säckpipan från Ångvik, Vallflöjt i dimman och Rostorgeln.

![Krang-panelen med Bordunorgeln](docs/krangpanelen_bordunorgeln.png)

## Spolslingan

Välj **Spolslingan** i kammarväljaren. En polyfon motor (8 röster) för leads, arpeggion och slingor:

| Modul | Kontroller | Vad den gör |
| --- | --- | --- |
| FM-spolen | Ratio, Index, Avklingning | En modulator styr en bärvåg; indexet klingar av efter anslaget så att tonen plinkar från ljus till mörk |
| Munharpa | Munharpa, Munhåla | Ett surrande pulståg (lamellen) genom två resonanser som sveper över deltonerna, som munhålan på en munharpa |
| Radiokorn | Korn, Storlek, Spridning | Granulär effekt: korta korn med slumpad position, tonhöjd och riktning, och ibland tystnad – en trasig radio |
| Slingan | Riktning, Takt, Oktaver | Arpeggiator: Upp, Ned, Upp/ned eller Slump i 1/8, 1/16, 1/16-trioler eller 1/32, över 1–3 oktaver |
| Envelope | Attack, Release | |

Slingan följer DAW:ens tempo och position när DAW:en spelar; annars går den på egen klocka i Stegverkets tempo från första tangenttrycket. Sex presets finns under *Leads och slingor*: Spolslingan, Munharpa i maskinhallen, Kulning över ödemarken, Trasig radio, Ångslinga och Spolpuls i ruinen.

![Krang-panelen med Spolslingan](docs/krangpanelen_spolslingan.png)

## Ångledningen (effekter)

Signalvägen är: kammare → **Krangdistortion** → **Ångfilter** → **Katedralreverb** → limiter. Allt fram till reverben är mono; reverben gör ljudet stereo.

| Effekt | Lägen | Inställningar |
| --- | --- | --- |
| Krangdistortion | Rör, Fuzz, Vikning, Bitkross | Drive, Mix, Bitar, Nedsampling, Ut |
| Ångfilter | Av, Lågpass, Högpass, Formant, Kam | Frekvens, Resonans, Vokal (A–U, för Formant) |
| Katedralreverb | Hall, Platta, Rostig tank (fjäderreverb) | Mix, Storlek, Mörker |

Kam ger en metallisk ton på vald frekvens, som ljud genom ett långt rör. Formant låter ljudet "säga" vokaler. Utgången har ett tak på −0,3 dBFS.

## Presets

Uppe till höger på frontpanelen: `<` och `>` bläddrar, listan visar alla presets i kategorier och **SPARA** sparar nuvarande ljud som eget preset. Ett eget preset innehåller alla inställningar och Stegverkets mönster. DAW:ens programlista visar fabrikspresets.

| Kategori | Presets |
| --- | --- |
| Bas | Init Basmålen, Ångvisslan, Smedjans bas, Rostig sub, Kopparrörsacid, Vikt vågbas, Domedagsbas, Tyst maskin |
| Drön och folk | Nyckelharpsdrön, Kulning i ångan, Kamröret, Väderkvarnen, Bordunorgeln, Nyckelharpan i ruinen, Säckpipan från Ångvik, Vallflöjt i dimman, Rostorgeln |
| Leads och slingor | Spolslingan, Munharpa i maskinhallen, Kulning över ödemarken, Trasig radio, Ångslinga, Spolpuls i ruinen |
| Trummor (med mönster) | Init Hammarverket, Smedjegång, Rostig fabrik, Folkdans i ruinerna, Ångslagets puls, Bitkrossad marsch, Katedralens slagverk, Träskedsbrygga, Ventilsystemet |

Fabrikspresets finns i `Source/presets/FactoryPresets.h`: varje preset listar bara det som skiljer sig från standardvärdena. Egna presets sparas som `.krang`-filer (XML) i `~/Library/Application Support/KrangBox/Presets/` och kan delas mellan datorer. Ett eget preset kan inte få samma namn som ett fabrikspreset; det får då tillägget "(eget)".

## Provspela

Öppna den fristående appen (`KrangBox.app`) eller pluginet i en DAW. Längst ned finns ett virtuellt keyboard (C1–C7): spela med musen, eller klicka på det och använd datorns tangenter `A W S E D F T G Y H U J K`. `OKT -` och `OKT +` flyttar datortangenterna en oktav; de startar på C2, bra för bas. I den fristående appen väljer du ljudkort och MIDI-ingång under *Options*.

## Struktur

| Sökväg | Innehåll | Ansvarig |
| --- | --- | --- |
| `Source/dsp/Basmalen.h` | Basmotor: 2 osc + sub, ladder-filter, envelopes, glid, drift | Sara |
| `Source/dsp/Hammarverket.h` | Trummaskin: åtta syntetiska röster och MIDI-mappning | Sara |
| `Source/dsp/Bordunorgeln.h` | Drönmotor: vågtabeller, stråkbrus, bordun, resonanssträngar | Sara |
| `Source/dsp/Spolslingan.h` | Leads och slingor: FM, munharpa, radiokorn | Sara |
| `Source/dsp/Slingan.h` | Arpeggiator med värdsynk och egen klocka | Sara |
| `Source/dsp/Stegverket.h` | Stegsekvenser: mönster, värdsynk, intern klocka och gung | Sara |
| `Source/dsp/KrangDistortion.h` | Rör, Fuzz, Vikning (4x översamplat), Bitkross | Sara / Justine |
| `Source/dsp/Angledningen.h` | Ångfilter och Katedralreverb (mono in, stereo ut) | Justine |
| `Source/dsp/KrangEngine.h` | Kopplar ihop kammare, effekter, DC-spärr och limiter | Justine |
| `Source/Parameters.h` | Parameterträdet – ID:n får aldrig döpas om | Justine |
| `Source/ui/KrangStyle.h` | Krang-stil: LookAndFeel, ratt, fader, modulram, kopparrör, VFD-display, VU-mätare, frontpanel | Philip |
| `Source/ui/Textures.h` | Genererade texturer och bakgrundscache | Philip |
| `Source/ui/Panels.h` | Modulerna: Basmålen, Hammarverket (röster + Stegverket), Ångledningen | Philip |
| `Source/ui/StegverketPanel.h` | Stegverkets lamprutnät och transport | Philip |
| `Source/presets/` | Fabrikspresets och PresetManager (spara, ladda, DAW-program) | Doktor Krang |
| `Source/PluginEditor.*` | Krang-panelen: skalning, koppling av alla parametrar, presets, keyboard | Philip |
| `tools/KrangRender.cpp` | Offline-rendering för lyssningstest och CI | Doktor Krang |
| `tools/KrangSnapshot.cpp` | Ritar stilprovet till PNG för granskning utan DAW | Philip |
| `docs/` | Bilder på Krang-panelen, stilprovet och Stegverket | – |
| `.github/workflows/` | macOS-bygge, renderingstest och pluginval | Justine |

## Licens

GNU Affero General Public License v3.0 – se [LICENSE](LICENSE). KrangBox använder JUCE under dess AGPLv3-licens.
