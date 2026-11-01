#!/usr/bin/env python3
"""Builds the embedded sample bank for the 2000s era.

Picks zones from the CC0 libraries listed in sample_specs.py, trims, loops sustained sounds,
encodes them as 8-bit mu-law and writes C++ sources into plugins/RetroNovelFM/samples/.

usage: build_samples.py <dir with vcsl and vsco git clones> <output dir>
"""
import os, re, subprocess, sys
import numpy as np
from sample_specs import SPECS, DRUMS

SRC, OUT = sys.argv[1], sys.argv[2]
os.makedirs(OUT, exist_ok=True)

PC = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
def midi(n):
    m = re.match(r'([A-G])(#?)(-?\d)', n)
    return 12 * (int(m.group(3)) + 1) + PC[m.group(1)] + (1 if m.group(2) else 0)

def ls(repo):
    r = subprocess.run(['git', '-C', os.path.join(SRC, repo), 'ls-tree', '-r', 'HEAD', '--name-only'], capture_output=True, text=True, check=True)
    return [l for l in r.stdout.split('\n') if l]

LISTS = {r: ls(r) for r in ('vcsl', 'vsco')}

# ---------------------------------------------------------------- choose files
plan = []   # (key, repo, path, midi)
for s in SPECS:
    pre = s['folder'] + '/'
    cand = {}
    for f in LISTS[s['repo']]:
        if not f.startswith(pre): continue
        m = re.search(s['rx'], f[len(pre):])
        if not m: continue
        n = 30 + int(m.group(1)) * 5 if s.get('nonote') else midi(m.group(1))
        if s['rng'][0] <= n <= s['rng'][1] and n not in cand: cand[n] = f
    last = -99
    for n in sorted(cand):
        if n - last >= s['spacing']:
            plan.append((s['key'], s['repo'], cand[n], n)); last = n
drumplan = []
for lane, repo, rx in DRUMS:
    f = [x for x in LISTS[repo] if re.search(rx, x)]
    if f: drumplan.append((lane, repo, f[0]))
    else: print('missing drum', lane)

# ---------------------------------------------------------------- fetch blobs (sparse checkout)
for repo in ('vcsl', 'vsco'):
    paths = [p for _, r, p, _ in plan if r == repo] + [p for _, r, p in drumplan if r == repo]
    if not paths: continue
    pat = ['/' + p.replace('[', '\\[').replace(']', '\\]').replace('*', '\\*') for p in paths]
    d = os.path.join(SRC, repo)
    subprocess.run(['git', '-C', d, 'sparse-checkout', 'set', '--no-cone', '--stdin'], input='\n'.join(pat), text=True, check=True)

def load(repo, path, rate):
    r = subprocess.run(['ffmpeg', '-v', 'error', '-i', os.path.join(SRC, repo, path), '-ac', '1', '-ar', str(rate), '-f', 'f32le', '-'], capture_output=True, check=True)
    return np.frombuffer(r.stdout, dtype=np.float32).copy()

def detect_midi(x, rate):
    """autocorrelation pitch estimate on the steady part of a note -> fractional midi number"""
    a = int(0.12 * rate); seg = x[a:a + int(0.35 * rate)].astype(np.float64)
    if len(seg) < 2048: seg = x[:4096].astype(np.float64)
    seg = seg - seg.mean()
    n = len(seg)
    f = np.fft.rfft(seg, 2 * n); ac = np.fft.irfft(f * np.conj(f))[:n]
    ac /= ac[0] + 1e-12
    lo, hi = int(rate / 1500), int(rate / 40)
    region = ac[lo:hi]
    peak = int(np.argmax(region)) + lo
    # prefer the first peak that is almost as strong (avoids octave-down errors)
    for i in range(lo + 1, peak):
        if ac[i] > 0.88 * ac[peak] and ac[i] >= ac[i - 1] and ac[i] >= ac[i + 1]: peak = i; break
    y0, y1, y2 = ac[peak - 1], ac[peak], ac[peak + 1]
    d = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2 + 1e-12)
    f0 = rate / (peak + d)
    return 69 + 12 * np.log2(f0 / 440.0)

def trim_start(x):
    pk = np.max(np.abs(x)) + 1e-9
    idx = np.argmax(np.abs(x) > pk * 0.01)
    return x[max(0, idx - 48):]

def make_loop(x, rate, maxlen):
    """returns (data, loopStart, loopEnd) with a crossfaded seamless loop"""
    n = min(len(x), int(maxlen * rate))
    x = x[:n].astype(np.float64)
    le = int(n * 0.94)
    L = int(min(0.75 * rate, le * 0.45))
    xf = int(min(0.18 * rate, L * 0.5))
    # search loop start that best matches the material before the loop end
    ref = x[le - xf:le]
    best, bestc = le - L, -2
    lo, hi = max(xf + 1, le - int(L * 1.4)), le - int(L * 0.7)
    for ls in range(lo, hi, 7):
        seg = x[ls - xf:ls]
        c = np.dot(seg, ref) / (np.linalg.norm(seg) * np.linalg.norm(ref) + 1e-9)
        if c > bestc: bestc, best = c, ls
    ls = best
    y = x.copy()
    w = np.linspace(0, 1, xf)
    y[le - xf:le] = x[le - xf:le] * np.sqrt(1 - w) + x[ls - xf:ls] * np.sqrt(w)
    return y[:le], ls, le

def oneshot(x, rate, maxlen):
    n = min(len(x), int(maxlen * rate))
    y = x[:n].astype(np.float64)
    f = int(n * 0.15)
    y[n - f:] *= np.linspace(1, 0, f) ** 2
    # trim trailing near-silence
    pk = np.max(np.abs(y)) + 1e-9
    nz = np.where(np.abs(y) > pk * 0.0015)[0]
    end = min(n, (nz[-1] + 64) if len(nz) else n)
    return y[:end]

MU = 255.0
def mulaw(y):
    y = np.clip(y, -1, 1)
    c = np.sign(y) * np.log1p(MU * np.abs(y)) / np.log1p(MU)
    return np.clip(np.round((c + 1) * 127.5), 0, 255).astype(np.uint8)

def emit_cpp(name, data):
    path = os.path.join(OUT, name + '.cpp')
    with open(path, 'w') as f:
        f.write('// generated by tools/build_samples.py from CC0 recordings (VCSL / VSCO-2-CE)\n')
        f.write('#include <cstdint>\nnamespace retrofm {\nextern const uint8_t %s[];\nconst uint8_t %s[%d] = {\n' % (name, name, max(1, len(data))))
        for i in range(0, len(data), 40):
            f.write(','.join(str(int(b)) for b in data[i:i + 40]) + ',\n')
        f.write('};\n}\n')

# ---------------------------------------------------------------- process instruments
index = []   # (key, [(name, len, ls, le, rate, root, gain)])
total = 0
for s in SPECS:
    zones = [(p, n, r) for k, r, p, n in plan if k == s['key']]
    if not zones:
        print('WARNING: no zones for', s['key']); continue
    zl = []
    blob = bytearray()
    for path, n, repo in zones:
        x = trim_start(load(repo, path, s['rate']))
        if s.get('detect'):
            dm = detect_midi(x, s['rate'])
            print('   %s: named %d, detected %.2f' % (path.split('/')[-1], n, dm))
            if np.isfinite(dm) and 24 <= dm <= 100: n = int(round(dm))
        if s['loop']: y, ls_, le_ = make_loop(x, s['rate'], s['maxlen'])
        else:
            y = oneshot(x, s['rate'], s['maxlen']); ls_, le_ = 0, 0
        pk = np.max(np.abs(y)) + 1e-9
        y = y / pk * 0.95
        off = len(blob)
        blob += mulaw(y).tobytes()
        zl.append((off, len(y), ls_, le_, s['rate'], n, float(pk)))
    zl.sort(key=lambda z: z[5])
    name = 'kSmp_' + s['key']
    emit_cpp(name, blob)
    total += len(blob)
    # relative loudness: keep the natural balance between zones (normalised to the loudest)
    mx = max(z[6] for z in zl)
    index.append((s['key'], name, [(o, l, a, b, r, n, g / mx) for o, l, a, b, r, n, g in zl]))
    print('%-9s %2d zones %7.0f KB  notes %s' % (s['key'], len(zl), len(blob) / 1024, [z[5] for z in zl]))

# drums
dblob = bytearray(); dz = []
for lane, repo, path in drumplan:
    x = trim_start(load(repo, path, 32000))
    y = oneshot(x, 32000, 1.6 if lane in ('crash', 'ohat') else 0.9)
    y = y / (np.max(np.abs(y)) + 1e-9) * 0.95
    dz.append((lane, len(dblob), len(y)))
    dblob += mulaw(y).tobytes()
emit_cpp('kSmp_drums', dblob)
total += len(dblob)
print('drums %.0f KB, total %.2f MB' % (len(dblob) / 1024, total / 1048576))

with open(os.path.join(OUT, 'SampleIndex.hpp'), 'w') as f:
    f.write('// generated by tools/build_samples.py - do not edit\n#pragma once\n#include <cstdint>\n\nnamespace retrofm {\n\n')
    f.write('struct SampleZone { uint32_t offset, length, loopStart, loopEnd, rate; uint8_t root; float gain; };\n')
    f.write('struct SampleSet { const char* key; const uint8_t* data; const SampleZone* zones; int count; };\n\n')
    for key, name, zl in index:
        f.write('extern const uint8_t %s[];\n' % name)
    f.write('extern const uint8_t kSmp_drums[];\n\n')
    for key, name, zl in index:
        f.write('static const SampleZone kZ_%s[] = {\n' % key)
        for o, l, a, b, r, n, g in zl: f.write('    {%d, %d, %d, %d, %d, %d, %.4ff},\n' % (o, l, a, b, r, n, g))
        f.write('};\n')
    f.write('\nstatic const SampleSet kSampleSets[] = {\n')
    for key, name, zl in index: f.write('    {"%s", %s, kZ_%s, %d},\n' % (key, name, key, len(zl)))
    f.write('};\nstatic const int kSampleSetCount = %d;\n\n' % len(index))
    lanes = ['kick', 'snare', 'chat', 'ohat', 'clap', 'toml', 'tomh', 'rim', 'crash']
    f.write('// live kit, lane order = DrumLane\nstatic const SampleZone kDrumZones[9] = {\n')
    for ln in lanes:
        z = [d for d in dz if d[0] == ln]
        if z: f.write('    {%d, %d, 0, 0, 32000, 60, 1.0f},\n' % (z[0][1], z[0][2]))
        else: f.write('    {0, 0, 0, 0, 32000, 60, 0.0f},\n')
    f.write('};\n\n} // namespace retrofm\n')
print('done')
