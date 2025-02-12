import { join } from "jsr:@std/path";
import sharp from "sharp";

const d = Deno.cwd();
const iconsDir = join(d, 'icons');

const outputFile = Deno.args[1];
if (!outputFile) throw new Error("No output file specified, bailing");

const output = [
    '#include <pgmspace.h>',
    '#include <string.h>',
    '',
    '#ifndef __ICON_H',
    '#define __ICON_H',
    '',
    'typedef enum icon_id_t {',
];

const icons: Record<string, number[]> = {};
const drawables: string[] = [];

for await (const file of Deno.readDir(iconsDir)) {
    if (!file.isFile || !file.name.toLowerCase().endsWith('png')) continue;
    const bytes = await Deno.readFile(join(d, 'icons', file.name));
    const cleaned = file.name.replace(/^D_/, "").replace(/\.png$/, '').toUpperCase();
    if (file.name.startsWith("D_")) {
        drawables.push(cleaned);
    }
    const arr = [];
    const raw = await sharp(bytes)
        .resize(8, 8)
        .threshold(128)
        .toColourspace('b-w')
        .raw()
        .toBuffer();

    for (let y = 0; y < 8; y++) {
        let byte = 0;
        for (let x = 0; x < 8; x++) {
            const pixel = raw[y * 8 + x];
            if (pixel === 255) { // White pixel
                byte |= 1 << (7 - x);
            }
        }
        arr.push(byte);
    }

    icons[cleaned] = arr;
}

output.push(
    ...Object.keys(icons).map((key, idx) => {
        const suffix = idx === 0 ? " = 1" : "";
        const delimiter = idx === Object.keys(icons).length - 1 ? "" : ",";
        return `    ${key}${suffix}${delimiter}`;
    }),
    '} IconId;',
    '',
    'const char ICON_NAMES[][20] = {',
    '    "",', // Invalid entry
    '    ' + Object.keys(icons).map(key => `"${key}"`).join(',\n    '),
    '};',
    '',
    'const IconId DRAWABLE_ICONS[] = {',
    ...drawables.map((drawable, idx) => {
        const delimiter = idx === drawables.length - 1 ? "" : ",";
        return `    ${drawable}${delimiter}`;
    }),
    '};',
    '',
    '#define DRAWABLE_LENGTH (sizeof(DRAWABLE_ICONS)/sizeof(DRAWABLE_ICONS[0]))',
    '',
    `const byte ICONS[${Object.keys(icons).length}][8] = {`,
    ...Object.entries(icons).map(([key, value], idx) => {
        const delimiter = idx === Object.entries(icons).length - 1 ? "" : ",";
        return `    {${value.join(", ")}}${delimiter} // ${key}`;
    }),
    '};',
    '',
    '#define ICONS_LENGTH (sizeof(ICONS)/sizeof(ICONS[0]))',
    '#define ICON(id) ICONS[id - 1]',
    '',
    `int icon_idx(const char *name) {`,
    '    for (int i = 1; i <= ICONS_LENGTH; i++) {',
    '        if (strcmp_P(name, ICON_NAMES[i]) == 0) {',
    '            return i;',
    '        }',
    '    }',
    '    return 0;',
    '}',
    '',
    'const byte* get_icon_by_name(const char *name) {',
    '    int idx = icon_idx(name);',
    '    if (idx == 0) return ICON(EXCLAMATION);',
    '    return ICON(idx);',
    '}',
    '',
    'const char * icon_name(IconId id) {',
    '    for (int i = 0; i < ICONS_LENGTH; i++) {',
    '        if (i + 1 == id) return ICON_NAMES[i + 1];',
    '    }',
    '    return NULL;',
    '}',
    '',
    '#endif'
);

await Deno.writeTextFile(outputFile, output.join('\n'));
