import { select, selectAll, nu, html, escape } from "campfire.js";
import cfa from "cf-alert";
import { md5 } from "./md5.ts";

interface ScanResult {
    ssid: string,
    rssi: number,
    security: string,
    band: string
};

// workaround for https://www.esp8266.com/viewtopic.php?f=34&t=4398
const S = "succ";
const U = "ess" + "fully";
const SU = S + U;

function humanReadableRssi(rssi: number) {
    const minRSSI = -100; // Considered as no signal
    const maxRSSI = -50;  // Considered as excellent signal

    if (rssi <= minRSSI) {
        return 0;
    } else if (rssi >= maxRSSI) {
        return 100;
    } else {
        return Math.round(((rssi - minRSSI) / (maxRSSI - minRSSI)) * 100);
    }
}

const fetchScanResults = async () => {
    const res = await fetch('/scan');
    const err = () => cfa.message('Error fetching Wi-Fi scan results. Please enter details manually.');
    if (!res.ok) await err();
    try {
        const txt = await res.text();
        console.log(`received: ${txt}`);
        const parsed = await JSON.parse(txt);
        return (parsed?.scan_results || []) as ScanResult[];
    }
    catch {
        await err();
    }
    return [];
}

window.addEventListener("DOMContentLoaded", async () => {
    const fields = selectAll("input[id^=secret-token]") as HTMLInputElement[];
    const ssidField = select("#wifi-ssid") as HTMLInputElement;
    const pskField = select("#wifi-psk") as HTMLInputElement;
    const usernameField = select("#username") as HTMLInputElement;
    const firmwareField = select("#firmware-file") as HTMLInputElement;
    const firmwareSubmit = select("#firmware-submit") as HTMLButtonElement;

    const setSecret = (
        str: string | undefined,
        interactive = true,
        field?: HTMLInputElement,
        e?: ClipboardEvent,
    ) => {
        const log = interactive ? console.warn : cfa.message;
        if (!str) return;
        if (!str.includes(" ")) return;
        const split = str.split(" ");
        if (split.length !== 4) {
            if (field) field.value = "";
            return log(
                "Invalid secret. The secret phrase must have four words.",
                "Error",
            );
        }

        if (split.some((i) => i.length < 5 || i.length > 11)) {
            if (field) field.value = "";
            return log(
                "The parts of the secret should be between 5 and 11 characters long.",
            );
        }

        e?.preventDefault();

        split.forEach((item, i) => fields[i].value = item);
    };

    try {
        const handle = (res: Response) => {
            if (!res.ok) return "";
            return res.json().then((val) =>
                val.message === "ok" ? val.value.trim() as string : ""
            );
        };
        const secret = await fetch("/secret").then(handle);
        const psk = await fetch("/psk").then(handle);
        const ssid = await fetch("/ssid").then(handle);
        const username = await fetch("/username").then(handle);

        if (secret) setSecret(secret);
        if (psk) pskField.value = psk;
        if (ssid) ssidField.value = ssid;
        if (username) usernameField.value = username;
    } catch { }

    const networks = await fetchScanResults().then(r => r.toSorted((a, b) => b.rssi - a.rssi));

    if (!networks.length) {
        select('.networks-container-label')?.setHTMLUnsafe('No available networks');
    }
    select('section.networks')?.append(...networks.map(network => {
        const [elt] = nu('article.network', {
            raw: true,
            on: {
                click: (e) => {
                    ssidField.value = network.ssid;
                }
            },
            c: html`
            <header>
                <span class="hidden">Network name</span>
                <strong>${network.ssid}</strong>
                <em>(${network.band})</em>
            </header>
            <div class="network-meta">
                <span class="hidden">Network security</span>
                    <span class="network-security">🔒 ${network.security}</span>
            </div>
            <div class=network-meta>
                <span class="hidden">Network strength</span>
                <span class="network-strength">📶 ${humanReadableRssi(network.rssi)}%</span>
            </div>
            `
        });
        return elt;
    }));

    fields.forEach((field, i) => {
        field.addEventListener("input", (e) => {
            const t = field.value.endsWith(" ");
            field.value = field.value.trim();
            if (t) {
                e.preventDefault();
                if (i < 3) fields[i + 1].focus();
            }
        });

        field.addEventListener("keyup", (e) => {
            if (e.key === "Backspace" && !field.value.trim()) {
                if (i > 0) fields[i - 1].focus();
            }
        });

        field.addEventListener("paste", (e) => {
            const data = e.clipboardData?.getData("text/plain").trim();
            setSecret(data, true, field, e);
        });
    });

    const setSecretBtn = select("#secret-token-submit");
    const setWifiBtn = select("#wifi-submit");

    const post = async (route: string, value: string) => {
        const res = await fetch(route, {
            method: "POST",
            body: new URLSearchParams({ value }).toString(),
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
        });

        if (!res.ok) {
            await cfa.message('Error communicating with device: ' + res.statusText, 'Error');
            throw new Error;
        }
    }

    setSecretBtn?.addEventListener("click", async () => {
        if (!usernameField.value.trim()) {
            return cfa.message("You must enter your username.");
        }

        if (fields.some((field) => !field.value.trim())) {
            return cfa.message("All secret fields must be filled.");
        }
        if (
            fields.some((field) =>
                field.value.length < 5 || field.value.length > 11
            )
        ) {
            return cfa.message(
                "Secret parts must be between 5 and 11 characters long.",
            );
        }

        try {
            await post(
                "/secret",
                fields.map((field) => field.value.trim()).join(" "),
            );
            await post(
                '/username',
                usernameField.value.trim()
            )
            await cfa.message(`Secret changed ${SU}.`);
        }
        catch { }
    });

    setWifiBtn?.addEventListener("click", async () => {
        if (!ssidField.value.trim() || !pskField.value.trim()) {
            return cfa.message("Enter both the network name and password.");
        }
        try {
            await post("/psk", pskField.value.trim());
            await post("/ssid", ssidField.value.trim());
            await cfa.message(`Details changed ${SU}.`);
        }
        catch { }
    });

    const updateFirmwareLabel = () => {
        const fileLabel = select("#firmware-form .selected-file") as HTMLDivElement;

        if (!firmwareField.files?.length) {
            if (!fileLabel.classList.contains('unselected')) fileLabel.classList.add("unselected");
            fileLabel.textContent = "No file selected.";
            return;
        }

        fileLabel.classList.remove('unselected');
        const file = firmwareField.files[0];
        fileLabel.textContent = escape(file.name);
    }

    updateFirmwareLabel();
    firmwareField.addEventListener('change', () => updateFirmwareLabel());

    const getMd5 = (file: File) => {
        return new Promise<string>((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (event) => {
                const fileBuf = event.target.result;
                if (typeof fileBuf === 'string') throw new Error("got string but was expecting file");
                const hashBuf = md5(fileBuf);
                resolve(Array.from(new Uint8Array(hashBuf))
                    .map((b) => b.toString(16).padStart(2, "0"))
                    .join(""));
            }
            reader.onerror = reject;
            reader.readAsArrayBuffer(file);
        })
    }

    firmwareSubmit.addEventListener('click', async () => {
        if (!firmwareField.files?.length) {
            return await cfa.message("No file selected. Please select a firmware (.bin) file and press update again.", "Error");
        }
        const file = firmwareField.files[0];
        const hash = await getMd5(file);

        const body = new FormData();
        body.append("MD5", hash);
        body.append("update", file, file.name);

        try {
            const res = await fetch("/update", { method: "POST", body });
            if (res.ok) {
                await cfa.message("update started. Please wait for the device to reboot.");
                firmwareField.value = '';
            }
            else {
                await cfa.fatal("update failed: " + await res.text());
            }
        }
        catch (e) {
            await cfa.fatal("Error upgrading: " + e);
        }
    })
});
