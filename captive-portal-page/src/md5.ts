const hs = [...Array(16)].map((_, i) => i.toString(16));
const hex2s = (hs.join("") + hs.reverse().join("")).match(/../g);
const H = new Uint32Array(Uint8Array.from(hex2s, v => parseInt(v, 16)).buffer);
const K = Uint32Array.from(
    Array(64), (_, i) => Math.floor(Math.abs(Math.sin(i + 1)) * (2 ** 32)));
const S = [[7, 12, 17, 22], [5, 9, 14, 20], [4, 11, 16, 23], [6, 10, 15, 21]];
const F = [
    (b: number, c: number, d: number) => ((b & c) | ((~b >>> 0) & d)) >>> 0,
    (b: number, c: number, d: number) => ((d & b) | ((~d >>> 0) & c)) >>> 0,
    (b: number, c: number, d: number) => (b ^ c ^ d) >>> 0,
    (b: number, c: number, d: number) => (c ^ (b | (~d >>> 0))) >>> 0,
];
const J = [
    (i: any) => i,
    (i: number) => (5 * i + 1) % 16,
    (i: number) => (3 * i + 5) % 16,
    (i: number) => (7 * i) % 16,
];
function rotl(v: number, n: number) {
    return ((v << n) | (v >>> (32 - n))) >>> 0;
}

// from: https://gist.github.com/bellbind/74c0543b48cf65f053ac136a0cad40c2
export function md5(buffer: ArrayBufferView | ArrayBuffer) {
    const u8a = ArrayBuffer.isView(buffer) ?
        new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength) :
        new Uint8Array(buffer);
    const total = Math.ceil((u8a.length + 9) / 64) * 64;
    const chunks = new Uint8Array(total);
    chunks.set(u8a);
    chunks.fill(0, u8a.length);
    chunks[u8a.length] = 0x80;
    const lenbuf = new Uint32Array(chunks.buffer, total - 8);
    const low = u8a.length % (1 << 29);
    const high = (u8a.length - low) / (1 << 29);
    lenbuf[0] = low << 3;
    lenbuf[1] = high;

    const hash = H.slice();
    for (let offs = 0; offs < total; offs += 64) {
        const w = new Uint32Array(chunks.buffer, offs, 16);
        let [a, b, c, d] = hash;
        for (let s = 0; s < 4; s++) {
            for (let i = s * 16, end = i + 16; i < end; i++) {
                const t = a + F[s](b, c, d) + K[i] + w[J[s](i)];
                const na = (b + rotl(t >>> 0, S[s][i % 4])) >>> 0;
                [a, b, c, d] = [d, na, b, c];
            }
        }
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d;
    }
    return hash.buffer;
}

