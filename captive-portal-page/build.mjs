import * as esbuild from "esbuild";
import { minify } from "html-minifier";
import { readFile, writeFile } from "node:fs/promises";

const built = await esbuild.build({
    entryPoints: ["src/main.ts"],
    bundle: true,
    write: false,
    platform: "browser",
    minify: true,
    treeShaking: true
});

const js = new TextDecoder().decode(built.outputFiles[0].contents);
const html = await readFile("./page.html", { encoding: "utf-8" });
const css = await readFile("./style.css", { encoding: "utf-8" });

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
await writeFile("./captive_portal_index.h", `\
#include <pgmspace.h>
// captive_portal_index.h built at ${new Date().toISOString()}
#ifndef __CAPTIVE_PORTAL_INDEX
#define __CAPTIVE_PORTAL_INDEX

const char index_html[] PROGMEM = R"====(
${final}
)====";

#endif
`);
