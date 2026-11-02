#!/usr/bin/env python3
"""Builds the real (sampled) drum kits from CC0 libraries.

  Big Rusty Drums  (Karoryfer, CC0)   https://github.com/sfzinstruments/karoryfer.big-rusty-drums
  Virtuosity Drums (Versilian, CC0)   https://github.com/sfzinstruments/virtuosity_drums

Every kit has 9 lanes (kick, snare, closed hat, open hat, clap/rimshot, low tom, high tom, rim, crash),
2 velocity layers (soft / hard) and up to 2 round robins. Close and overhead mics are mixed to mono.

usage: build_drumkits.py <dir with the library clones> <output dir>
"""
import os, re, subprocess, sys
import numpy as np

SRC, OUT = sys.argv[1], sys.argv[2]
RATE = 32000
RUSTY, VIRT = 'karoryfer.big-rusty-drums', 'virtuosity_drums'
LANES = ['kick', 'snare', 'chat', 'ohat', 'clap', 'toml', 'tomh', 'rim', 'crash']

# lane -> (repo, [(directory regex, file-name regex with vl/rr groups, mic gain)], max seconds, round robins)
def R(d, f, g): return (d, f, g)
KITS = [
    ('rock', RUSTY, {
        'kick':  ([R(r'kick_24/kick/kick', r'k_vl(\d+)_rr(\d+)', 1.0), R(r'kick_24/kick/oh', r'k_vl(\d+)_rr(\d+)', 0.5)], 0.7, 1),
        'snare': ([R(r'snare_14/center/top', r'sn_center_vl(\d+)_rr(\d+)', 1.0), R(r'snare_14/center/oh', r'sn_center_vl(\d+)_rr(\d+)', 0.6), R(r'snare_14/center/btm', r'sn_center_vl(\d+)_rr(\d+)', 0.25)], 0.8, 2),
        'chat':  ([R(r'hihat_14/cl/cl', r'ht_cl_vl(\d+)_rr(\d+)', 1.0), R(r'hihat_14/cl/oh', r'ht_cl_vl(\d+)_rr(\d+)', 0.5)], 0.35, 2),
        'ohat':  ([R(r'hihat_14/open/cl', r'ht_open_vl(\d+)_rr(\d+)', 1.0), R(r'hihat_14/open/oh', r'ht_open_vl(\d+)_rr(\d+)', 0.5)], 1.3, 1),
        'clap':  ([R(r'snare_14/rimshot/top', r'sn_rims_vl(\d+)_rr(\d+)', 1.0), R(r'snare_14/rimshot/oh', r'sn_rims_vl(\d+)_rr(\d+)', 0.6)], 0.9, 1),
        'toml':  ([R(r'tom_22/center/cl', r't22_vl(\d+)_rr(\d+)', 1.0), R(r'tom_22/center/oh', r't22_vl(\d+)_rr(\d+)', 0.5)], 1.2, 1),
        'tomh':  ([R(r'tom_14/center/cl', r't14_vl(\d+)_rr(\d+)', 1.0), R(r'tom_14/center/oh', r't14_vl(\d+)_rr(\d+)', 0.5)], 1.0, 1),
        'rim':   ([R(r'snare_14/sidestick/top', r'sn_ss_vl(\d+)_rr(\d+)', 1.0), R(r'snare_14/sidestick/oh', r'sn_ss_vl(\d+)_rr(\d+)', 0.5)], 0.4, 1),
        'crash': ([R(r'crash_17/cr/cl', r'cr_vl(\d+)_rr(\d+)', 1.0), R(r'crash_17/cr/oh', r'cr_vl(\d+)_rr(\d+)', 0.7)], 2.4, 1),
    }),
    ('pop', VIRT, {
        'kick':  ([R(r'kickmic/kick', r'kickmic_kick_snoff_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/kick', r'oh_kick_snoff_vl(\d+)(?:_rr(\d+))?', 0.6)], 0.7, 1),
        'snare': ([R(r'snaremic/snare', r'snaremic_snare_center_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/snare', r'oh_snare_center_vl(\d+)(?:_rr(\d+))?', 0.7)], 0.8, 2),
        'chat':  ([R(r'oh/hh', r'oh_hh_closed_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'mid/hh', r'mid_hh_closed_vl(\d+)(?:_rr(\d+))?', 0.5)], 0.35, 2),
        'ohat':  ([R(r'oh/hh', r'oh_hh_open_vl(\d+)(?:_rr(\d+))?', 1.0)], 1.3, 1),
        'clap':  ([R(r'snaremic/snare', r'snaremic_snare_rimshot_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/snare', r'oh_snare_rimshot_vl(\d+)(?:_rr(\d+))?', 0.7)], 0.9, 1),
        'toml':  ([R(r'mid/ltom', r'mid_ltom_center_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/ltom', r'oh_ltom_center_vl(\d+)(?:_rr(\d+))?', 0.6)], 1.2, 1),
        'tomh':  ([R(r'mid/htom', r'mid_htom_center_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/htom', r'oh_htom_center_vl(\d+)(?:_rr(\d+))?', 0.6)], 1.0, 1),
        'rim':   ([R(r'snaremic/snare', r'snaremic_snare_crossstick_vl(\d+)(?:_rr(\d+))?', 1.0), R(r'oh/snare', r'oh_snare_crossstick_vl(\d+)(?:_rr(\d+))?', 0.5)], 0.4, 1),
        'crash': ([R(r'oh/crash', r'oh_crash_crash_vl(\d+)(?:_rr(\d+))?', 1.0)], 2.4, 1),
    }),
    ('brush', RUSTY, {
        'kick':  ([R(r'kick_24/kick/kick', r'k_vl(\d+)_rr(\d+)', 1.0), R(r'kick_24/kick/oh', r'k_vl(\d+)_rr(\d+)', 0.5)], 0.7, 1, (0.15, 0.5)),
        'snare': ([R(r'snare_14/brush/top', r'sn_b_vl(\d+)_rr(\d+)', 1.0), R(r'snare_14/brush/oh', r'sn_b_vl(\d+)_rr(\d+)', 0.7)], 0.7, 2),
        'chat':  ([R(r'hihat_14/cl_brush/cl', r'ht_cl_b_vl(\d+)_rr(\d+)', 1.0), R(r'hihat_14/cl_brush/oh', r'ht_cl_b_vl(\d+)_rr(\d+)', 0.6)], 0.4, 2),
        'ohat':  ([R(r'hihat_14/open_brush/cl', r'ht_open_b_vl(\d+)_rr(\d+)', 1.0), R(r'hihat_14/open_brush/oh', r'ht_open_b_vl(\d+)_rr(\d+)', 0.6)], 1.2, 1),
        'clap':  ([R(r'snare_14/bstir/top', r'sn_stir_dl(\d+)_rr(\d+)', 1.0), R(r'snare_14/bstir/oh', r'sn_stir_dl(\d+)_rr(\d+)', 0.6)], 0.9, 1),
        'toml':  ([R(r'tom_18/brush/cl', r't18_b_vl(\d+)_rr(\d+)', 1.0), R(r'tom_18/brush/oh', r't18_b_vl(\d+)_rr(\d+)', 0.6)], 1.0, 1),
        'tomh':  ([R(r'tom_14/brush/cl', r't14_b_vl(\d+)_rr(\d+)', 1.0), R(r'tom_14/brush/oh', r't14_b_vl(\d+)_rr(\d+)', 0.6)], 0.9, 1),
        'rim':   ([R(r'snare_14/sidestick/top', r'sn_ss_vl(\d+)_rr(\d+)', 1.0), R(r'snare_14/sidestick/oh', r'sn_ss_vl(\d+)_rr(\d+)', 0.5)], 0.4, 1, (0.3, 0.6)),
        'crash': ([R(r'ride_22/brush/cl', r'rd_b_vl(\d+)_rr(\d+)', 1.0), R(r'ride_22/brush/oh', r'rd_b_vl(\d+)_rr(\d+)', 0.7)], 2.0, 1),
    }),
    ('lofi', VIRT, {
        'kick':  ([R(r'lofi/kick', r'lofi_kick_snoff_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.7, 1),
        'snare': ([R(r'lofi/snare', r'lofi_snare_center_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.7, 2),
        'chat':  ([R(r'lofi/hh', r'lofi_hh_closed_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.3, 2),
        'ohat':  ([R(r'lofi/hh', r'lofi_hh_half_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.9, 1),
        'clap':  ([R(r'lofi/snare', r'lofi_snare_rimshot_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.7, 1),
        'toml':  ([R(r'lofi/ltom', r'lofi_ltom_center_vl(\d+)(?:_rr(\d+))?', 1.0)], 1.0, 1),
        'tomh':  ([R(r'lofi/htom', r'lofi_htom_center_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.9, 1),
        'rim':   ([R(r'lofi/snare', r'lofi_snare_crossstick_vl(\d+)(?:_rr(\d+))?', 1.0)], 0.35, 1),
        'crash': ([R(r'lofi/crash', r'lofi_crash_crash_vl(\d+)(?:_rr(\d+))?', 1.0)], 2.0, 1),
    }),
]

def ls(repo):
    r = subprocess.run(['git', '-C', os.path.join(SRC, repo), 'ls-tree', '-r', 'HEAD', '--name-only'], capture_output=True, text=True, check=True)
    return [l for l in r.stdout.split('\n') if l]
LISTS = {r: ls(r) for r in (RUSTY, VIRT)}

def find(repo, d, f):
    """{(vl, rr): path} for files in Samples/<d>/ matching f"""
    out = {}
    pre = re.compile(r'^Samples/' + d + r'/' + f + r'\.(flac|wav)$')
    for p in LISTS[repo]:
        m = pre.match(p)
        if m: out[(int(m.group(1)), int(m.group(2) or 1))] = p
    return out

# ---------------------------------------------------------------- pick the files
plan = []      # (kit, lane, layer, rr, [(repo, path, gain)])
need = {RUSTY: set(), VIRT: set()}
for kit, repo, lanes in KITS:
    for lane in LANES:
        spec = lanes[lane]
        mics, maxlen, nrr = spec[0], spec[1], spec[2]
        velpos = spec[3] if len(spec) > 3 else (0.35, 0.85)
        found = [find(repo, d, f) for d, f, g in mics]
        if not found[0]:
            print('!! missing', kit, lane, mics[0][0]); continue
        vls = sorted(set(v for v, r in found[0]))
        for layer, pos in enumerate(velpos):
            vl = vls[min(len(vls) - 1, int(round(pos * (len(vls) - 1))))]
            rrs = sorted(r for v, r in found[0] if v == vl)
            for rr_i, rr in enumerate(rrs[:nrr]):
                parts = []
                for (d, f, g), fm in zip(mics, found):
                    p = fm.get((vl, rr))
                    if p: parts.append((repo, p, g)); need[repo].add(p)
                plan.append((kit, lane, layer, rr_i, parts, maxlen))

for repo, paths in need.items():
    if not paths: continue
    pat = '\n'.join('/' + p for p in sorted(paths))
    from fetch_blobs import materialize
    materialize(os.path.join(SRC, repo), sorted(paths))

def load(repo, path):
    r = subprocess.run(['ffmpeg', '-v', 'error', '-i', os.path.join(SRC, repo, path), '-ac', '1', '-ar', str(RATE), '-f', 'f32le', '-'], capture_output=True, check=True)
    return np.frombuffer(r.stdout, dtype=np.float32).astype(np.float64)

MU = 255.0
def mulaw(y):
    y = np.clip(y, -1, 1)
    c = np.sign(y) * np.log1p(MU * np.abs(y)) / np.log1p(MU)
    return np.clip(np.round((c + 1) * 127.5), 0, 255).astype(np.uint8)

blob = bytearray()
zones = {}     # (kit, lane, layer, rr) -> (offset, length, peak)
for kit, lane, layer, rr, parts, maxlen in plan:
    sigs = [load(repo, p) * g for repo, p, g in parts]
    n = max(len(s) for s in sigs)
    x = np.zeros(n)
    for s in sigs: x[:len(s)] += s
    pk = np.max(np.abs(x)) + 1e-9
    i0 = max(0, int(np.argmax(np.abs(x) > pk * 0.02)) - 16)
    x = x[i0:i0 + int(maxlen * RATE)]
    f = int(len(x) * 0.2)
    x[len(x) - f:] *= np.linspace(1, 0, f) ** 2
    nz = np.where(np.abs(x) > pk * 0.002)[0]
    x = x[:min(len(x), (nz[-1] + 32) if len(nz) else len(x))]
    zones[(kit, lane, layer, rr)] = (x, pk)

# normalise per lane so the hard layer peaks at 0.95 and the soft layer keeps its natural level
KITNAMES = [k[0] for k in KITS]
data_index = []
for kit in KITNAMES:
    for lane in LANES:
        keys = [k for k in zones if k[0] == kit and k[1] == lane]
        if not keys: continue
        top = max(np.max(np.abs(zones[k][0])) for k in keys) + 1e-9
        for k in sorted(keys):
            x = zones[k][0] / top * 0.95
            off = len(blob)
            blob += mulaw(x).tobytes()
            data_index.append((k, off, len(x)))
print('drum kits: %.0f KB' % (len(blob) / 1024))

os.makedirs(OUT, exist_ok=True)
with open(os.path.join(OUT, 'kDrumKits.cpp'), 'w') as f:
    f.write('// generated by tools/build_drumkits.py from CC0 recordings (Big Rusty Drums, Virtuosity Drums)\n#include <cstdint>\nnamespace retrofm {\n')
    f.write('extern const uint8_t kSmp_drumkits[];\nconst uint8_t kSmp_drumkits[%d] = {\n' % len(blob))
    for i in range(0, len(blob), 40): f.write(','.join(str(b) for b in blob[i:i + 40]) + ',\n')
    f.write('};\n}\n')
with open(os.path.join(OUT, 'DrumKitIndex.hpp'), 'w') as f:
    f.write('// generated by tools/build_drumkits.py - do not edit\n#pragma once\n#include <cstdint>\n\nnamespace retrofm {\n\n')
    f.write('extern const uint8_t kSmp_drumkits[];\n')
    f.write('static const int kNumSampleKits = %d;   // %s\n' % (len(KITNAMES), ', '.join(KITNAMES)))
    f.write('// [kit][lane][layer][rr] -> offset, length (32 kHz mu-law); length 0 = missing\n')
    f.write('static const uint32_t kDrumKitZones[%d][9][2][2][2] = {\n' % len(KITNAMES))
    for kit in KITNAMES:
        f.write('  { // %s\n' % kit)
        for lane in LANES:
            row = []
            for layer in range(2):
                rrs = []
                for rr in range(2):
                    e = [d for d in data_index if d[0] == (kit, lane, layer, rr)]
                    if not e: e = [d for d in data_index if d[0] == (kit, lane, layer, 0)]
                    rrs.append('{%d, %d}' % (e[0][1], e[0][2]) if e else '{0, 0}')
                row.append('{' + ', '.join(rrs) + '}')
            f.write('    {' + ', '.join(row) + '},  // %s\n' % lane)
        f.write('  },\n')
    f.write('};\n\n} // namespace retrofm\n')
print('done')
