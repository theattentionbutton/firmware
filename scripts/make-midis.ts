import { join } from "@std/path";
import MIDI from "npm:@tonejs/midi";
import { Frequency } from "npm:tone";

const d = Deno.cwd();
const midisDir = join(d, 'midis');

const outputFile = Deno.args[1];
if (!outputFile) throw new Error("No output file specified, bailing");

const output = [
    '#include <Arduino.h>',
    '#include <EventEncoderButton.h>',
    '#include <string.h>',
    '#include "constants.h"\n',
    '#ifndef _MIDIS_H',
    '#define _MIDIS_H\n'
]

const tracks = [];

for await (const file of Deno.readDir(midisDir)) {
    if (!file.isFile || !file.name.toLowerCase().endsWith('mid')) continue;
    const midi = new MIDI.Midi(await Deno.readFile(join(midisDir, file.name)));
    const name = file.name.replace(/\.mid$/, '').toUpperCase();
    if (name.length > 16) throw new Error("name too long");
    const track = midi.tracks[0];
    console.log(`Extracting track ${track.name} from ${file.name}`);

    const toMs = (n: number) => Math.round(n * 1000);
    let lastNoteEnd = 0;

    const transformed = track.notes.map(note => {
        const gap = toMs(note.time) - lastNoteEnd;
        lastNoteEnd = toMs(note.time) + toMs(note.duration);
        const freq = Math.round(Frequency(note.name).toFrequency());
        return `{${freq}, ${toMs(note.duration)}, ${gap > 0 ? gap : 0}}`;
    })

    tracks.push({
        name,
        notes: transformed,
    })
}

output.push(...[
    `#define TRACK_COUNT ${tracks.length + 1}`,
    `typedef enum midi_track_idx {`,
    '    INVALID = 0,',
    '    SILENCE,',
    '    ' + tracks.map(itm => itm.name).join(',\n    '),
    '} MidiTrackIdx;\n',
    `const char TRACK_NAMES[][16] = {`,
    '    "INVALID",',
    '    "SILENCE",',
    '    ' + tracks.map(itm => `"${itm.name}"`).join(',\n    '),
    '};',
    `\
typedef struct midi_track_t {
    int length;
    int (*first)[3];
} MidiTrack;
`,
    'int SILENCE_NOTES[][3] = {0, 10000, 0};\n',
    ...tracks.map(track => {
        return `\
int ${track.name}_NOTES[][3] = {${track.notes.join(', ')}};
`
    }),
    'MidiTrack tracks[] = {',
    '    {1, SILENCE_NOTES},',
    '    ' + tracks.map(track => {
        return `{${track.notes.length}, ${track.name}_NOTES}`
    }).join(',\n    '),
    '};',
    `\
MidiTrackIdx track_idx(const char *name) {
    for (int i = 0; i < TRACK_COUNT; i++) {
        if (strcmp(name, TRACK_NAMES[i]) == 0) {
            return (MidiTrackIdx)i;
        }
    }
    return (MidiTrackIdx)0;
}

#define TRACK(x) tracks[x - 1]

void error_beep() {
    for (int i = 0; i < 3; i++) {
        tone(BZ1, 544);
        delay(500);
        noTone(BZ1);
        delay(150);
    }
}

void invalid_tone() {
    for (int i = 0; i < 3; i++) {
        delay(350);
        error_beep();
    }
}

void play_track_by_idx(MidiTrackIdx id, EncoderBtn *button = NULL) {
    if (!id) return invalid_tone();
    long start_pos = button ? button->position() : 0;
    MidiTrack t = TRACK(id);
    if (id == SILENCE) {
        delay((*t.first)[0]);
        return;
    }
    for (int i = 0; i < t.length; i++) {
        int *note = t.first[i];
        delay(note[2]);
        tone(BZ1, note[0]);
        delay((unsigned long)note[1] + 10);
        noTone(BZ1);
        if (button) {
            button->update();
            if (button->position() != start_pos) break;
        }
    }
}

void play_track_by_name(const char *name) {
    play_track_by_idx(track_idx(name));
}\n`,
    '#endif\n'
])

await Deno.writeTextFile(outputFile, output.join('\n'));