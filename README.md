# KrangBox

*Dr. Krangs Patenterade Ljudmaskin* – en trasig synthbox från en dystopisk framtid, byggd av en uppfinnare ur en Jules Verne-roman, med klangfärger från gammal folkmusik.

Instrument-plugin (VST3, AU, CLAP) och fristående app för macOS. Allt ljud är syntetiskt.

**Status: fas 0 – prototyp.** Basmålen (bas) och Krangdistortion är spelbara via MIDI i en DAW, med ett enkelt grått testgränssnitt.

## Bygga

Kräver macOS 11+, Xcode och CMake 3.22+. JUCE 8 och clap-juce-extensions hämtas automatiskt.

```sh
cmake -B build -G Xcode
cmake --build build --config Release --target KrangBox_All
```

Byggena hamnar i `build/KrangBox_artefacts/Release/`. Kopiera `KrangBox.vst3` till `~/Library/Audio/Plug-Ins/VST3/`, `KrangBox.component` till `~/Library/Audio/Plug-Ins/Components/` och `KrangBox.clap` till `~/Library/Audio/Plug-Ins/CLAP/`.

## Lyssningstest

```sh
cmake --build build --config Release --target KrangRender
./build/KrangRender_artefacts/Release/KrangRender renders
```

Renderar en basslinga i D-dorisk genom alla fyra distortionslägen (Rör, Fuzz, Vikning, Bitkross) till WAV och kontrollerar NaN, toppnivå och DC.

## Struktur

| Sökväg | Innehåll | Ansvarig |
| --- | --- | --- |
| `Source/dsp/Basmalen.h` | Basmotor: 2 osc + sub, ladder-filter, envelopes, glid, drift | Sara |
| `Source/dsp/KrangDistortion.h` | Rör, Fuzz, Vikning (4x översamplat), Bitkross | Sara / Justine |
| `Source/dsp/KrangEngine.h` | Kopplar ihop kammare, effekter, DC-spärr och limiter | Justine |
| `Source/Parameters.h` | Parameterträdet – ID:n får aldrig döpas om | Justine |
| `Source/PluginEditor.*` | Testgränssnitt, ersätts av Krang-panelen i fas 2 | Philip |
| `tools/KrangRender.cpp` | Offline-rendering för lyssningstest och CI | Doktor Krang |
| `.github/workflows/` | macOS-bygge, renderingstest och pluginval | Justine |

## Licens

GNU Affero General Public License v3.0 – se [LICENSE](LICENSE). KrangBox använder JUCE under dess AGPLv3-licens.
