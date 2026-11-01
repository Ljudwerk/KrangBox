#!/usr/bin/env python3
# Bygger testkompositioner som Reaper-projekt (.RPP), med KrangBox förinställt på varje spår.
#
#   python3 tools/reaper/make_reaper_projects.py <KrangRender> <utkatalog> [--render]
#
# Varje komposition blir:
#   <namn>.RPP        – öppnas i Reaper; ett spår per KrangBox, med MIDI och presetet inbyggt i spårets FX
#   <namn>.json       – samma komposition för KrangRender --project (offline-rendering utan DAW)
#   <namn>.wav        – (med --render) förhandslyssning renderad offline, att jämföra med Reapers rendering
#
# Pluginets tillstånd hämtas med "KrangRender --state", så att spåren laddar exakt samma XML som pluginet
# själv sparar. VST3-blocket i .RPP-filen följer Reapers format (se make_vst3_chunk nedan).
#
# Copyright (C) 2026 KrangBox-teamet. Licens: AGPLv3, se LICENSE.
import base64, json, os, struct, subprocess, sys, tempfile, uuid

PPQ = 960  # Reapers MIDI-upplösning: tick per fjärdedel

# --- Noter -----------------------------------------------------------------------------

NOTE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'H': 11, 'B': 10}

def n(name):
    """'D3' -> 50 (C4 = 60)."""
    pitch, octave = name[:-1], int(name[-1])
    return 12 * (octave + 1) + NOTE[pitch]

def chord(names, start, length, vel=0.7):
    return [[start, length, n(x), vel] for x in names.split()]

def seq(names, start, step, length=None, vel=0.75):
    """En melodi i jämna steg: '.' är paus och '-' förlänger föregående ton ett steg.
    length = fast tonlängd i slag (staccato); annars hålls tonen hela sitt steg."""
    out, t = [], start
    for token in names.split():
        if token == '-':
            if out and length is None:
                out[-1][1] += step
        elif token != '.':
            out.append([t, length if length is not None else step, n(token), vel])
        t += step
    return out

# --- Kompositionerna -------------------------------------------------------------------------

def kammartest():
    """Alla elva kammare i tur och ordning, fyra takter var, med sitt grundpreset: lyssningstestet för grind 3."""
    tracks, beat = [], 0
    def add(name, preset, notes, **ov):
        tracks.append({'name': name, 'preset': preset, 'notes': notes, 'overrides': ov})

    add('Basmålen', 'Init Basmålen', seq('D2 D2 A2 C3 D2 . F2 G2 D2 D2 A2 C3 D3 C3 A2 G2', 0, 1.0, 0.8))
    drums = []
    for bar in range(4):
        b = 16 + bar * 4
        drums += [[b, 0.5, 36, 0.9], [b + 2.5, 0.5, 36, 0.7], [b + 1, 0.5, 38, 0.8], [b + 3, 0.5, 38, 0.9]]
        drums += [[b + i * 0.5, 0.25, 42, 0.5 + 0.2 * (i % 2 == 0)] for i in range(8)]
        drums += [[b + 3.5, 0.25, 75, 0.6]]
    add('Hammarverket', 'Init Hammarverket', drums)
    add('Bordunorgeln', 'Bordunorgeln', chord('D3 A3', 32, 8) + chord('F3 C4', 40, 8))
    add('Spolslingan', 'Spolslingan', seq('A4 C5 D5 F5 E5 D5 C5 A4 G4 A4 C5 D5 A4 - - .', 48, 1.0, 0.9))
    add('Stålpiano', 'Init Stålpiano', chord('D3 A3 D4 F4', 64, 4) + chord('C3 G3 C4 E4', 68, 4)
        + seq('A4 G4 F4 E4 D4 . A4 .', 72, 1.0, 0.9))
    add('Klockgjuteriet', 'Init Klockgjuteriet', seq('D4 A3 F4 D4 . . A3 D3', 80, 2.0, 1.5))
    add('Kören', 'Init Kören', chord('D3 F3 A3 D4', 96, 8) + chord('C3 E3 G3 C4', 104, 8))
    add('Radiomasten', 'Init Radiomasten', chord('D4', 112, 6) + chord('A4', 118, 10))
    add('Lurblåsaren', 'Init Lurblåsaren', seq('D4 F4 A4 - G4 F4 E4 D4 - - C4 D4 - - - .', 128, 1.0))
    add('Lutverket', 'Init Lutverket', seq('D3 A3 D4 F4 A4 F4 D4 A3 C3 G3 C4 E4 G4 E4 C4 G3', 144, 1.0, 1.5))
    add('Vattenpumpen', 'Init Vattenpumpen', chord('D3', 160, 7) + chord('A3', 168, 7))
    return {'title': 'KrangBox – kammartest', 'tempo': 100, 'lengthBeats': 180, 'masterDb': -2, 'tracks': tracks}

def ruinens_psalm():
    """72 BPM, D-dorisk: kör, bordun, kyrkklockor, en näverlur i naturtoner och en avlägsen radiostation."""
    prog = [('D3 F3 A3 D4', 'D2'), ('G2 D3 G3 H3', 'G2'), ('C3 G3 C4 E4', 'C2'), ('A2 E3 A3 C4', 'A2'),
            ('D3 F3 A3 D4', 'D2'), ('G2 D3 G3 H3', 'G2'), ('A2 E3 A3 C4', 'A2'), ('D3 A3 D4 F4', 'D2')]
    koren, bordun, klockor = [], [], []
    for i, (c, root) in enumerate(prog):
        koren += chord(c, i * 8, 8, 0.65)
    bordun += chord('D3 A3', 0, 32, 0.6) + chord('D3 A3', 32, 32, 0.6)
    for bar in range(16):
        if bar % 4 == 0:
            klockor += [[bar * 4, 3, n('D4'), 0.8], [bar * 4 + 2, 2, n('A3'), 0.5]]
    klockor += [[60, 4, n('D3'), 0.9], [61, 3, n('A3'), 0.7], [62, 2, n('D4'), 0.6]]
    # Näverluren i D spelar naturtoner: A3 D4 F#4 A4 C5 D5 (tredje till åttonde deltonen).
    lur = seq('A4 - D5 - A4 F#4 A4 - - - . . . . . .', 16, 0.5, None, 0.8) + \
          seq('D5 - - C5 A4 - F#4 - A4 - - - D4 - - -', 48, 0.5, None, 0.75)
    radio = [[0, 64, n('D5'), 0.5]]
    return {'title': 'Ruinens psalm', 'tempo': 72, 'lengthBeats': 64, 'tracks': [
        {'name': 'Kör', 'preset': 'Psalm i schaktet', 'notes': koren},
        {'name': 'Bordun', 'preset': 'Nyckelharpsdrön', 'notes': bordun, 'gainDb': -3},
        {'name': 'Klockor', 'preset': 'Kyrkklockan i dimman', 'notes': klockor, 'gainDb': -2},
        {'name': 'Näverlur', 'preset': 'Vallåten', 'notes': lur},
        {'name': 'Radio', 'preset': 'Nattsändning från Moskva', 'notes': radio, 'gainDb': -9},
    ]}

def maskinhallen():
    """118 BPM: Stegverket synkat till DAW:en, acidbas, arpeggio från Slingan, skruvat piano och en åskplåt."""
    bass = []
    line = 'D2 D2 D3 D2 F2 D2 C3 D2 D2 D2 A2 D2 G2 D2 F2 C2'
    for bar in range(2, 16):
        bass += seq(line, bar * 4, 0.25, 0.2, 0.8)
    arp = []
    for i, c in enumerate(['D3 F3 A3', 'C3 E3 G3', 'D3 F3 A3', 'G2 H2 D3']):
        if i >= 1:
            arp += chord(c, 16 + (i - 1) * 16, 16, 0.75)
    piano = []
    for bar in range(8, 16):
        piano += chord('D4 F4 A4 C5', bar * 4, 0.5, 0.8) + chord('D4 F4 A4 C5', bar * 4 + 1.5, 0.25, 0.6)
    plat = chord('D2', 0, 15, 0.7) + chord('D2', 48, 15, 0.8)
    return {'title': 'Maskinhallen', 'tempo': 118, 'lengthBeats': 64, 'tracks': [
        {'name': 'Stegverket', 'preset': 'Smedjegång', 'notes': [], 'overrides': {'seqRun': 1}},
        {'name': 'Acidbas', 'preset': 'Kopparrörsacid', 'notes': bass, 'gainDb': -2},
        {'name': 'Slingan', 'preset': 'Ångslinga', 'notes': arp, 'gainDb': -4},
        {'name': 'Skruvat piano', 'preset': 'Skruvat piano', 'notes': piano, 'gainDb': -4},
        {'name': 'Åskplåt', 'preset': 'Plåtåskan', 'notes': plat, 'gainDb': -6},
    ]}

def folkdans():
    """104 BPM: folkdans med Hammarverket via MIDI, kantele, skalmeja, bas och munharpa."""
    drums = []
    for bar in range(16):
        b = bar * 4
        drums += [[b, 0.5, 36, 1.0], [b + 2, 0.5, 36, 0.8], [b + 1, 0.5, 41, 0.8], [b + 3, 0.5, 41, 0.9], [b + 3.5, 0.25, 41, 0.6]]
        drums += [[b + i * 0.5, 0.25, 75, 0.45 + 0.25 * (i % 2 == 0)] for i in range(8)]
        if bar % 4 == 3:
            drums += [[b + 3.75, 0.25, 39, 0.7]]
    kantele = []
    pattern = {'Dm': 'D3 A3 D4 F4 A4 F4 D4 A3', 'C': 'C3 G3 C4 E4 G4 E4 C4 G3', 'G': 'G2 D3 G3 H3 D4 H3 G3 D3'}
    for bar, ch in enumerate(['Dm', 'Dm', 'C', 'Dm', 'Dm', 'G', 'C', 'Dm'] * 2):
        kantele += seq(pattern[ch], bar * 4, 0.5, 1.0, 0.65)
    tune = ('A4 - D5 C5 A4 G4 F4 G4 A4 - - - D4 E4 F4 G4 A4 - C5 A4 G4 - E4 - D4 - - - . . . . '
            'A4 C5 D5 - C5 A4 G4 E4 F4 G4 A4 - G4 F4 E4 C4 D4 - F4 E4 D4 - C4 - D4 - - - . . . .')
    skalmeja = seq(tune, 16, 0.5, None, 0.8)
    bass = []
    for bar, root in enumerate(['D2', 'D2', 'C2', 'D2', 'D2', 'G2', 'C2', 'D2'] * 2):
        bass += [[bar * 4, 1.5, n(root), 0.85], [bar * 4 + 2, 1.0, n(root) + 7, 0.7]]
    munharpa = []
    for bar in range(8, 16):
        munharpa += [[bar * 4 + i, 0.5, n('D3'), 0.6 + 0.2 * (i == 0)] for i in range(4)]
    return {'title': 'Folkdans i ruinerna', 'tempo': 104, 'lengthBeats': 64, 'tracks': [
        {'name': 'Trummor', 'preset': 'Folkdans i ruinerna', 'notes': drums},
        {'name': 'Kantele', 'preset': 'Kantelen vid elden', 'notes': kantele, 'gainDb': -3},
        {'name': 'Skalmeja', 'preset': 'Skalmejan i gränden', 'notes': skalmeja, 'gainDb': -2},
        {'name': 'Bas', 'preset': 'Smedjans bas', 'notes': bass, 'gainDb': -3},
        {'name': 'Munharpa', 'preset': 'Munharpa i maskinhallen', 'notes': munharpa, 'gainDb': -6},
    ]}

def tonverket():
    """110 BPM: allt spelas av Stegverket i takt med Reaper. Noterna på spåren flyttar slingorna
    (Tonstegverkets läge Transponera) i en ackordföljd: C, C, B♭, A♭."""
    def transpose(root):
        return [[bar * 4, 0.25, root + shift, 0.8] for bar, shift in enumerate([0, 0, 0, 0, -2, -2, -4, -4] * 2)]
    return {'title': 'Tonverket i natten', 'tempo': 110, 'lengthBeats': 64, 'tracks': [
        {'name': 'Stegverket', 'preset': 'Smedjegång', 'notes': [], 'overrides': {'seqRun': 1}},
        {'name': 'Acidslinga', 'preset': 'Acidslinga i pannrummet', 'notes': transpose(36), 'overrides': {'seqRun': 1}, 'gainDb': -2},
        {'name': 'Speldosa', 'preset': 'Speldosan i ruinen', 'notes': transpose(60), 'overrides': {'seqRun': 1}, 'gainDb': -7},
        {'name': 'Kulning', 'preset': 'Kulningsslingan', 'notes': transpose(60), 'overrides': {'seqRun': 1, 'tsRoot': 60}, 'gainDb': -5},
    ]}

COMPOSITIONS = {
    '01-kammartest': kammartest,
    '02-ruinens-psalm': ruinens_psalm,
    '03-maskinhallen': maskinhallen,
    '04-folkdans-i-ruinerna': folkdans,
    '05-tonverket-i-natten': tonverket,
}

# --- Reaper-formatet ---------------------------------------------------------------------

def make_vst3_chunk(state_xml, program_name):
    """Reapers VST3-block för KrangBox: [huvud][komponenttillstånd + kontrollertillstånd][programnamn],
    där komponenttillståndet är JUCE:s VST3-tillstånd: 'VC2!' + längd + XML + JUCE:s privata data."""
    xml = state_xml.encode('utf-8')
    private = bytes.fromhex('0000000000000000004a554345507269766174654461746100010142797061737300010103001d00000000000000'
                            '4a5543455072697661746544617461')
    component = b'VC2!' + struct.pack('<I', len(xml)) + xml + private
    controller = bytes(8)
    body = struct.pack('<II', len(component), 1) + component + controller
    header = struct.pack('<11I', 737024205, 4276969198, 0, 2, 1, 0, 2, 0, len(body), 1, 0)
    name = b'\x00' + program_name.encode('utf-8') + bytes(5)
    lines = [base64.b64encode(header).decode()]
    for i in range(0, len(body), 96):
        lines.append(base64.b64encode(body[i:i + 96]).decode())
    lines.append(base64.b64encode(name).decode())
    return lines

def midi_events(notes, length_beats):
    """Noter -> Reapers 'E <delta> <status> <data1> <data2>' i tick (960 per fjärdedel), sorterade."""
    events = []
    for start, length, note, vel in notes:
        on = int(round(start * PPQ)); off = int(round((start + length) * PPQ)) - 1
        events.append((on, 1, 0x90, note, max(1, min(127, int(round(vel * 127))))))
        events.append((max(on + 1, off), 0, 0x80, note, 0))
    events.sort()
    out, last = [], 0
    for t, _, status, d1, d2 in events:
        out.append('E %d %02x %02x %02x' % (t - last, status, d1, d2))
        last = t
    end = int(length_beats * PPQ)
    out.append('E %d b0 7b 00' % max(0, end - last))  # alla toner av vid slutet
    return out

def guid():
    return '{' + str(uuid.uuid4()).upper() + '}'

def make_rpp(comp, states):
    tempo, beats = comp['tempo'], comp['lengthBeats']
    seconds = beats * 60.0 / tempo
    out = ['<REAPER_PROJECT 0.1 "7.84/macOS-arm64" 0 0',
           '  <NOTES 0 2', '    |%s – byggd av tools/reaper/make_reaper_projects.py' % comp['title'], '  >',
           '  TEMPO %g 4 4 0' % tempo, '  SAMPLERATE 48000 0 0', '  LOOP 0',
           '  MASTER_VOLUME %.6f 0 -1 -1 1' % (10 ** (comp.get('masterDb', -6) / 20.0)),
           '  SELECTION 0 %g' % seconds, '  CURSOR 0']
    for track, (state_xml, program) in zip(comp['tracks'], states):
        gain = 10 ** (track.get('gainDb', 0) / 20.0)
        out += ['  <TRACK %s' % guid(), '    NAME "%s"' % track['name'], '    VOLPAN %.6f 0 -1 -1 1' % gain,
                '    MUTESOLO 0 0 0', '    NCHAN 2', '    FX 1', '    MAINSEND 1 0',
                '    <FXCHAIN', '      SHOW 0', '      LASTSEL 0', '      DOCKED 0', '      BYPASS 0 0 0',
                '      <VST "VST3i: KrangBox (Doktor Krang)" KrangBox.vst3 0 "" 737024205{ABCDEF019182FAEB4B726E674B627831} ""']
        out += ['        ' + l for l in make_vst3_chunk(state_xml, program)]
        out += ['      >', '      FLOATPOS 0 0 0 0', '      FXID %s' % guid(), '      WAK 0 0', '    >']
        if track['notes']:
            out += ['    <ITEM', '      POSITION 0', '      LENGTH %.6f' % seconds, '      LOOP 0',
                    '      NAME "%s"' % track['name'], '      IGUID %s' % guid(), '      <SOURCE MIDI',
                    '        HASDATA 1 %d QN' % PPQ]
            out += ['        ' + e for e in midi_events(track['notes'], beats)]
            out += ['        GUID %s' % guid(), '      >', '    >']
        out += ['  >']
    out += ['>']
    return '\n'.join(out) + '\n'

def main():
    if len(sys.argv) < 3:
        print(__doc__ or 'make_reaper_projects.py <KrangRender> <utkatalog> [--render]')
        sys.exit(1)
    krangrender, outdir = sys.argv[1], sys.argv[2]
    render = '--render' in sys.argv[3:]
    os.makedirs(outdir, exist_ok=True)
    failures = 0
    for name, build in COMPOSITIONS.items():
        comp = build()
        states = []
        with tempfile.TemporaryDirectory() as tmp:
            for track in comp['tracks']:
                path = os.path.join(tmp, 'state.xml')
                args = [krangrender, '--state', track['preset'], path]
                args += ['%s=%s' % (k, v) for k, v in track.get('overrides', {}).items()]
                subprocess.run(args, check=True)
                states.append((open(path, encoding='utf-8').read().strip(), track['preset']))
        with open(os.path.join(outdir, name + '.RPP'), 'w', encoding='utf-8') as f:
            f.write(make_rpp(comp, states))
        spec_path = os.path.join(outdir, name + '.json')
        with open(spec_path, 'w', encoding='utf-8') as f:
            json.dump(comp, f, ensure_ascii=False, indent=1)
        print('%s: %d spår, %d slag i %g BPM' % (name, len(comp['tracks']), comp['lengthBeats'], comp['tempo']))
        if render:
            result = subprocess.run([krangrender, '--project', spec_path, os.path.join(outdir, name + '.wav')])
            failures += 1 if result.returncode != 0 else 0
    sys.exit(1 if failures else 0)

if __name__ == '__main__':
    main()
