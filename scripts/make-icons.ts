import { join } from "jsr:@std/path";
import sharp from "sharp";

const d = Deno.cwd();
const iconsDir = join(d, 'icons');

const outputFile = Deno.args[1];
if (!outputFile) throw new Error("No output file specified, bailing");

const icons: Record<string, number[]> = {};
const drawables: string[] = [];

for await (const file of Deno.readDir(iconsDir)) {
    if (!file.isFile || !file.name.toLowerCase().endsWith('png')) continue;
    const bytes = await Deno.readFile(join(d, 'icons', file.name));
    const cleaned = file.name.replace(/^D_/, "").replace(/\.png$/, '');
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

const output = [
    '#include <pgmspace.h>',
    '',
    '#ifndef __ICON_H',
    '#define __ICON_H',
    '',
    'typedef enum icon_id_t {',
    ...Object.keys(icons).map((key, idx) => {
        const suffix = idx === 0 ? " = 1" : "";
        const delimiter = idx === Object.keys(icons).length - 1 ? "" : ",";
        return `    ${key}${suffix}${delimiter}`;
    }),
    '} IconId;',
    '',
    'const int DRAWABLE_ICONS[] PROGMEM = {',
    ...drawables.map((drawable, idx) => {
        const delimiter = idx === drawables.length - 1 ? "" : ",";
        return `    ${drawable}${delimiter}`;
    }),
    '};',
    '',
    '#define DRAWABLE_LENGTH (sizeof(DRAWABLE_ICONS)/sizeof(DRAWABLE_ICONS[0]))',
    '',
    `const byte ICONS[${Object.keys(icons).length}][8] PROGMEM = {`,
    ...Object.entries(icons).map(([key, value], idx) => {
        const delimiter = idx === Object.entries(icons).length - 1 ? "" : ",";
        return `    {${value.join(", ")}}${delimiter} // ${key}`;
    }),
    '};',
    '',
    '#define ICONS_LENGTH (sizeof(ICONS)/sizeof(ICONS[0]))',
    '#define ICON(id) ICONS[id - 1]',
    '',
    '#endif'
];

await Deno.writeTextFile(outputFile, output.join('\n'));