import * as esbuild from "esbuild";
import { minify } from "html-minifier";
import { readFile, writeFile } from "node:fs/promises";
import { join } from 'node:path';

const [, , dir] = process.argv;
if (!dir) throw new Error("build.mjs: No dir supplied.");
const path = (...args) => join(dir, ...args);

const built = await esbuild.build({
    entryPoints: [path('src', 'main.ts')],
    bundle: true,
    write: false,
    platform: "browser",
    minify: true,
    treeShaking: true
});

const js = new TextDecoder().decode(built.outputFiles[0].contents);
const html = await readFile(path("page.html"), { encoding: "utf-8" });
const css = await readFile(path("style.css"), { encoding: "utf-8" });

const final = minify(
    html.replace('<script src="main.js"></script>', `<script>${js}</script>`)
        .replace(
            '<link rel="stylesheet" href="./style.css">',
            `<style>${css}</style>`,
        ),
    {
        minifyCSS: true,
        minifyJS: false,
        collapseWhitespace: true,
        conservativeCollapse: true,
        preserveLineBreaks: true,
    },
);

await writeFile("./index.html", final);
await writeFile("./src/captive_portal_index.h", `\
#include <pgmspace.h>
// captive_portal_index.h built at ${new Date().toISOString()}
#ifndef __CAPTIVE_PORTAL_INDEX
#define __CAPTIVE_PORTAL_INDEX

const char index_html[] = R"====(
${final}
)====";

#endif
`);
