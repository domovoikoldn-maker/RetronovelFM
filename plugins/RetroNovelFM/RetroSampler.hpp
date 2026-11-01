// RetroSampler.hpp - sample-based instruments of the "Windows XP / 2000s visual novel" era
// Embedded recordings are CC0 (VCSL, VSCO-2-CE). Choir and nylon guitar are synthesized.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "RetroData.hpp"
#include "samples/SampleIndex.hpp"

namespace retrofm {

struct Zone { const float* data = nullptr; uint32_t len = 0, loopStart = 0, loopEnd = 0; double rate = 44100; int root = 60; float gain = 1; };

// ------------------------------------------------------------------ embedded bank (decoded once)
class SampleBank {
public:
    static const SampleBank& get() { static SampleBank b; return b; }

    int find(const char* key) const {
        for (int i = 0; i < kSampleSetCount; ++i) if (std::strcmp(kSampleSets[i].key, key) == 0) return i;
        return -1;
    }
    const std::vector<Zone>& set(int i) const { return sets[i]; }
    const Zone& drum(int lane) const { return drums[lane]; }

private:
    SampleBank() {
        float lut[256];
        for (int b = 0; b < 256; ++b) {
            const double c = b / 127.5 - 1.0;
            const double y = (std::pow(256.0, std::fabs(c)) - 1.0) / 255.0;
            lut[b] = (float)(c < 0 ? -y : y);
        }
        data.resize(kSampleSetCount);
        sets.resize(kSampleSetCount);
        for (int i = 0; i < kSampleSetCount; ++i) {
            const SampleSet& S = kSampleSets[i];
            uint32_t total = 0;
            for (int z = 0; z < S.count; ++z) total = std::max(total, S.zones[z].offset + S.zones[z].length);
            data[i].resize(total + 4);
            for (uint32_t k = 0; k < total; ++k) data[i][k] = lut[S.data[k]];
            for (int z = 0; z < S.count; ++z) {
                const SampleZone& sz = S.zones[z];
                Zone zz;
                zz.data = data[i].data() + sz.offset; zz.len = sz.length;
                zz.loopStart = sz.loopStart; zz.loopEnd = sz.loopEnd; zz.rate = sz.rate; zz.root = sz.root;
                zz.gain = 1.0f / std::sqrt(std::max(0.05f, sz.gain)) * sz.gain;   // soften level differences
                sets[i].push_back(zz);
            }
        }
        uint32_t dtotal = 0;
        for (int l = 0; l < 9; ++l) dtotal = std::max(dtotal, kDrumZones[l].offset + kDrumZones[l].length);
        drumData.resize(dtotal + 4);
        for (uint32_t k = 0; k < dtotal; ++k) drumData[k] = lut[kSmp_drums[k]];
        for (int l = 0; l < 9; ++l) {
            drums[l].data = drumData.data() + kDrumZones[l].offset; drums[l].len = kDrumZones[l].length;
            drums[l].rate = 32000; drums[l].gain = kDrumZones[l].gain;
        }
    }
    std::vector<std::vector<float>> data;
    std::vector<std::vector<Zone>> sets;
    std::vector<float> drumData;
    Zone drums[9];
};

// ------------------------------------------------------------------ user samples
struct UserSample {
    std::vector<float> data;
    Zone zone;
    std::string path;
};
// implemented in RetroFileLoad.cpp (WAV + OGG, UTF-8 paths on Windows)
bool loadAudioFileMono(const char* path, std::vector<float>& out, double& rate);

// ------------------------------------------------------------------ instrument definitions
enum SrcType { srcSample, srcChoir, srcGuitar };
struct Layer { const char* set; int lo, hi; float gain, pitch; };
struct XPInst {
    SrcType src;
    Layer layer[3];
    float a, d, s, r;       // envelope (d = decay time constant towards s)
    float cutoff, velCut;   // low-pass (Hz) and how much velocity opens it
    float detune;           // cents between two unison copies (0 = mono)
    float vib;              // built-in vibrato (cents)
    bool reverse;
    float crush;            // bit depth (0 = off)
    float randDetune;       // random cents per note
    float octLayer;         // extra layer one octave below
    float level;
};

#define L1(s)            {{s, 0, 127, 1, 0}, {nullptr,0,0,0,0}, {nullptr,0,0,0,0}}
#define L1P(s, p)        {{s, 0, 127, 1, p}, {nullptr,0,0,0,0}, {nullptr,0,0,0,0}}
#define ENS              {{"cello", 0, 52, 1.0f, 0}, {"viola", 53, 64, 1.0f, 0}, {"strings", 65, 127, 1.0f, 0}}
#define ENSP(p)          {{"ctrem", 0, 56, 1.0f, p}, {"trem", 57, 127, 1.0f, p}, {nullptr,0,0,0,0}}

static const XPInst kXP[kNumXP] = {
    // src        layers               a      d    s    r     cutoff  vc    det  vib  rev   crush rnd  oct   level
    {srcSample, L1("piano"),          0.001f, 9,  1,  0.35f, 20000, 0.6f,  0,   0, false, 0,  0,   0,    1.0f},  // Grand Piano
    {srcSample, L1("piano"),          0.008f, 9,  1,  0.6f,   2600, 0.4f,  0,   0, false, 0,  0,   0,    1.25f}, // Soft Piano
    {srcSample, L1P("glock", 12),     0.0f, 1.1f, 0,  0.3f,   9000, 0.3f,  0,   0, false, 0,  6,   0,    1.3f},  // Music Box
    {srcSample, ENS,                  0.08f,  1,  1,  0.5f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Strings
    {srcSample, ENS,                  0.7f,   1,  1,  1.2f,   6000, 0.2f,  0,   0, false, 0,  0,   0,    1.0f},  // Slow Strings
    {srcSample, {{"cpizz", 0, 56, 1, 0}, {"pizz", 57, 127, 1, 0}, {nullptr,0,0,0,0}},
                                      0.0f,   1,  1,  0.2f,  20000, 0.5f,  0,   0, false, 0,  0,   0,    1.4f},  // Pizzicato
    {srcSample, ENSP(0),              0.05f,  1,  1,  0.4f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Tremolo
    {srcSample, L1("cello"),          0.1f,   1,  1,  0.5f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Cello
    {srcSample, L1("harp"),           0.0f,   9,  1,  0.8f,  20000, 0.5f,  0,   0, false, 0,  0,   0,    1.3f},  // Harp
    {srcSample, L1("flute"),          0.05f,  1,  1,  0.25f, 20000, 0.3f,  0,   0, false, 0,  0,   0,    1.5f},  // Flute
    {srcSample, L1("clarinet"),       0.03f,  1,  1,  0.2f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Clarinet
    {srcSample, L1("oboe"),           0.03f,  1,  1,  0.2f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Oboe
    {srcSample, L1("recorder"),       0.03f,  1,  1,  0.2f,  20000, 0.3f,  0,   0, false, 0,  0,   0,    1.3f},  // Recorder
    {srcSample, L1("ocarina"),        0.04f,  1,  1,  0.25f, 20000, 0.3f,  0,   0, false, 0,  0,   0,    1.0f},  // Ocarina
    {srcSample, L1("glock"),          0.0f,   9,  1,  0.8f,  20000, 0.4f,  0,   0, false, 0,  0,   0,    0.8f},  // Glockenspiel
    {srcSample, L1("vibes"),          0.0f,   9,  1,  1.0f,  20000, 0.4f,  0,   0, false, 0,  0,   0,    1.0f},  // Vibraphone
    {srcSample, L1("chimes"),         0.0f,   9,  1,  1.5f,  20000, 0.4f,  0,   0, false, 0,  0,   0,    0.9f},  // Hand Bells
    {srcSample, L1("kalimba"),        0.0f,   9,  1,  0.6f,  20000, 0.4f,  0,   0, false, 0,  0,   0,    1.3f},  // Kalimba
    {srcSample, L1("zither"),         0.0f,   9,  1,  0.6f,  20000, 0.4f,  0,   0, false, 0,  0,   0,    1.0f},  // Zither
    {srcSample, L1("harpsi"),         0.0f,   9,  1,  0.15f, 20000, 0.2f,  0,   0, false, 0,  0,   0,    1.4f},  // Harpsichord
    {srcChoir,  L1(nullptr),          0.25f,  1,  1,  0.8f,   5000, 0.2f,  0,   0, false, 0,  0,   0,    1.0f},  // Choir Aah
    {srcGuitar, L1(nullptr),          0.0f,   9,  1,  0.3f,  20000, 0.5f,  0,   0, false, 0,  0,   0,    1.0f},  // Nylon Guitar
    {srcSample, L1("bowed"),          0.5f,   1,  1,  1.5f,  20000, 0.2f,  8,   0, false, 0,  0,   0,    1.0f},  // Glass Pad
    {srcSample, ENS,                  0.9f,   1,  1,  1.8f,   4500, 0.1f, 12,   0, false, 0,  0,   0,    1.0f},  // Dream Pad
    {srcSample, L1("piano"),          0.004f, 9,  1,  1.5f,   1500, 0.3f,  6,   0, false, 0,  0,   0.7f, 1.0f},  // Dark Piano
    {srcSample, ENSP(-12),            1.5f,   1,  1,  2.5f,   1400, 0.0f, 15,  25, false, 0,  0,   0,    1.3f},  // Horror Drone
    {srcChoir,  L1(nullptr),          0.8f,   1,  1,  2.0f,   2500, 0.0f, 25,  40, false, 0,  0,   0,    1.0f},  // Eerie Choir
    {srcSample, L1P("glock", 12),     0.0f, 1.2f, 0,  0.4f,  20000, 0.3f,  0,   0, false, 6, 45,   0,    0.8f},  // Broken Box
    {srcSample, L1("piano"),          0.02f,  1,  1,  0.3f,   8000, 0.3f,  0,   0, true,  0,  0,   0,    1.2f},  // Reverse Piano
    {srcSample, L1P("chimes", -12),   0.25f,  9,  1,  2.0f,   3000, 0.2f, 10,   0, false, 0,  0,   0,    1.1f},  // Ghost Bells
};
#undef L1
#undef L1P
#undef ENS
#undef ENSP

static const XPInst kUserDef = {srcSample, {{nullptr,0,0,0,0},{nullptr,0,0,0,0},{nullptr,0,0,0,0}},
                                0.002f, 9, 1, 0.25f, 20000, 0.0f, 0, 0, false, 0, 0, 0, 1.0f};

// ------------------------------------------------------------------ voices
class Sampler {
public:
    static constexpr int NV = 24;
    static constexpr int KS = 8192;   // Karplus-Strong buffer per voice

    Sampler() : bank(SampleBank::get()) {
        for (int i = 0; i < kNumXP; ++i)
            for (int l = 0; l < 3; ++l) setIdx[i][l] = kXP[i].layer[l].set ? bank.find(kXP[i].layer[l].set) : -1;
        ks.assign(NV * KS, 0.f);
        for (auto& u : user) u.store(nullptr);
        for (int k = 0; k < kNumUser; ++k) userRoot[k] = 60;
    }

    void setSampleRate(double sr) { srate = sr; for (auto& v : voices) v.active = false; }
    void setRelease(float r) { relMul = r; }
    void setBrightness(float b) { bright = std::pow(2.0f, (b - 1.f) * 2.f); }
    void setUser(int k, UserSample* s) { user[k].store(s); }
    void setUserRoot(int k, int n) { userRoot[k] = n; }

    void noteOn(int inst, int note, int vel, double transpose) {
        Voice* best = nullptr;
        for (auto& v : voices) if (!v.active) { best = &v; break; }
        if (!best) { uint32_t ba = 0xFFFFFFFFu; for (auto& v : voices) if (v.age < ba) { ba = v.age; best = &v; } }
        Voice& v = *best;
        std::memset(v.play, 0, sizeof(v.play));
        v.np = 0; v.src = srcSample;
        const double x = vel / 127.0;
        v.vel = (float)x;
        const double n = note + transpose;
        if (inst >= kFirstUser) {
            const int k = inst - kFirstUser;
            UserSample* u = user[k].load();
            if (!u || u->zone.len < 4) return;
            u->zone.root = userRoot[k];
            addPlay(v, &u->zone, n, 0, 1.f, 0.f, false);
            v.def = &kUserDef;
        } else {
            const XPInst& I = kXP[inst - kFirstXP];
            v.def = &I;
            v.src = I.src;
            double rnd = 0;
            if (I.randDetune > 0) { rng = rng * 1664525u + 1013904223u; rnd = ((rng >> 8) / 16777216.0 * 2 - 1) * I.randDetune / 100.0; }
            if (I.src == srcSample) {
                for (int l = 0; l < 3; ++l) {
                    const Layer& L = I.layer[l];
                    if (!L.set || setIdx[inst - kFirstXP][l] < 0 || note < L.lo || note > L.hi) continue;
                    const Zone* z = pick(setIdx[inst - kFirstXP][l], n + L.pitch);
                    if (!z) continue;
                    if (I.detune > 0) {
                        addPlay(v, z, n + L.pitch + rnd - I.detune / 200.0, 0, L.gain * 0.75f, -0.6f, I.reverse);
                        addPlay(v, z, n + L.pitch + rnd + I.detune / 200.0, 0, L.gain * 0.75f, 0.6f, I.reverse);
                    } else addPlay(v, z, n + L.pitch + rnd, 0, L.gain, 0.f, I.reverse);
                    if (I.octLayer > 0) {
                        const Zone* zo = pick(setIdx[inst - kFirstXP][l], n + L.pitch - 12);
                        if (zo) addPlay(v, zo, n + L.pitch - 12, 0, L.gain * I.octLayer, 0.f, I.reverse);
                    }
                }
                if (v.np == 0) return;
            } else if (I.src == srcChoir) {
                v.freq = 440.0 * std::pow(2.0, (n - 69) / 12.0);
                for (int k = 0; k < 3; ++k) v.ph[k] = (k * 0.31);
                std::memset(v.fs, 0, sizeof(v.fs));
                v.cdet = I.detune > 0 ? I.detune : 9.f;
            } else {
                // Karplus-Strong pluck
                v.freq = 440.0 * std::pow(2.0, (n - 69) / 12.0);
                float* buf = &ks[(&v - voices) * KS];
                v.ksLen = std::min((double)KS - 2, srate / v.freq);
                const int N = (int)v.ksLen + 2;
                float lp = 0;
                for (int i = 0; i < N; ++i) {
                    rng = rng * 1664525u + 1013904223u;
                    const float nz = (rng >> 8) / 8388608.f - 1.f;
                    lp += (nz - lp) * (0.35f + 0.5f * (float)x);
                    buf[i] = lp * 0.9f;
                }
                v.ksPos = 0; v.ksPrev = 0; v.ksDamp = 0.996f;
                v.body = 0;
            }
        }
        const XPInst& D = *v.def;
        v.active = true; v.released = false; v.held = false; v.note = note;
        v.age = ++serial;
        v.env = 0; v.stage = 1;
        v.atk = D.a <= 0.0005f ? 1.0 : 1.0 / (D.a * srate);
        v.dec = std::exp(-1.0 / (std::max(0.01f, D.d) * srate));
        v.rel = std::exp(-1.0 / (std::max(0.01f, D.r * relMul) * srate));
        v.amp = (float)((0.35 + 0.65 * x * x) * D.level);
        const double cut = std::min(18000.0, D.cutoff * (1.0 - D.velCut + D.velCut * x) * bright);
        v.lpc = (float)(1.0 - std::exp(-2.0 * 3.14159265 * std::max(60.0, cut) / srate));
        v.lpOn = cut < 17000;
        std::memset(v.lp, 0, sizeof(v.lp));
        v.vph = 0;
    }

    void noteOff(int note, bool pedal) {
        for (auto& v : voices) if (v.active && v.note == note && !v.released) { if (pedal) v.held = true; else release(v); }
    }
    void pedalUp() { for (auto& v : voices) if (v.active && v.held) release(v); }
    void allOff() { for (auto& v : voices) if (v.active) release(v); }

    // pmul: global pitch multiplier (bend + vibrato). adds into L/R, returns activity
    float tick(double pmul, float& outL, float& outR) {
        float act = 0;
        for (auto& v : voices) {
            if (!v.active) continue;
            const XPInst& D = *v.def;
            // envelope
            if (v.stage == 1) { v.env += (float)v.atk; if (v.env >= 1) { v.env = 1; v.stage = 2; } }
            else if (v.stage == 2) v.env = D.s + (v.env - D.s) * (float)v.dec;
            else { v.env *= (float)v.rel; if (v.env < 0.0001f) { v.active = false; continue; } }
            if (v.stage == 2 && D.s <= 0 && v.env < 0.0001f) { v.active = false; continue; }

            double pm = pmul;
            if (D.vib > 0) { v.vph += 5.2 / srate; if (v.vph >= 1) v.vph -= 1; pm *= std::pow(2.0, std::sin(6.2831853 * v.vph) * D.vib / 1200.0); }

            float l = 0, r = 0;
            if (v.src == srcSample) {
                int alive = 0;
                for (int p = 0; p < v.np; ++p) {
                    Play& P = v.play[p];
                    if (!P.z) continue;
                    const Zone& z = *P.z;
                    double rp = P.rev ? P.revStart - P.pos : P.pos;
                    if (rp < 0 || rp >= z.len - 1) { P.z = nullptr; continue; }
                    const float s = interp(z.data, z.len, rp) * P.gain;
                    l += s * (1.f - P.pan) * 0.5f * 2.f * 0.70710678f;
                    r += s * (1.f + P.pan) * 0.5f * 2.f * 0.70710678f;
                    P.pos += P.inc * pm;
                    if (!P.rev && z.loopEnd > z.loopStart && P.pos >= z.loopEnd) P.pos -= (z.loopEnd - z.loopStart);
                    ++alive;
                }
                if (!alive) { v.active = false; continue; }
            } else if (v.src == srcChoir) {
                const double f = v.freq * pm;
                double saw = 0;
                for (int k = 0; k < 3; ++k) {
                    const double det = std::pow(2.0, (k - 1) * v.cdet / 1200.0);
                    v.ph[k] += f * det / srate; if (v.ph[k] >= 1) v.ph[k] -= 1;
                    saw += (v.ph[k] * 2 - 1);
                }
                rng = rng * 1664525u + 1013904223u;
                const double nz = ((rng >> 8) / 8388608.0 - 1.0) * 0.08;
                const double in = saw / 3.0 + nz;
                static const double F[3] = { 760, 1180, 2650 }, G[3] = { 1.0, 0.55, 0.18 };
                double o = 0;
                for (int k = 0; k < 3; ++k) {
                    const double fc = 2 * std::sin(3.14159265 * std::min(F[k], srate * 0.24) / srate);
                    double& lo = v.fs[k][0]; double& bp = v.fs[k][1];
                    const double hp = in - lo - 0.12 * bp;
                    bp += fc * hp; lo += fc * bp;
                    o += bp * G[k];
                }
                l = r = (float)(o * 0.32);
            } else {
                float* buf = &ks[(&v - voices) * KS];
                const int N = (int)v.ksLen;
                const float fr = (float)(v.ksLen - N);
                const int i0 = v.ksPos, i1 = (v.ksPos + 1) % N;
                const float y = buf[i0] * (1 - fr) + buf[i1] * fr;
                const float damp = v.released ? 0.94f : v.ksDamp;
                const float nv = (y + v.ksPrev) * 0.5f * damp;
                v.ksPrev = y;
                buf[i0] = nv;
                v.ksPos = (v.ksPos + 1) % N;
                v.body += (y - v.body) * 0.25f;
                l = r = (y * 0.6f + v.body * 0.5f) * 1.2f;
                (void)pm;
            }
            // filter (2 x one-pole)
            if (v.lpOn) {
                v.lp[0] += (l - v.lp[0]) * v.lpc; v.lp[1] += (v.lp[0] - v.lp[1]) * v.lpc; l = v.lp[1];
                v.lp[2] += (r - v.lp[2]) * v.lpc; v.lp[3] += (v.lp[2] - v.lp[3]) * v.lpc; r = v.lp[3];
            }
            if (D.crush > 0) {
                const float q = std::pow(2.f, D.crush - 1);
                l = std::floor(l * q + 0.5f) / q; r = std::floor(r * q + 0.5f) / q;
            }
            const float a = v.env * v.amp;
            outL += l * a; outR += r * a;
            act += v.env;
        }
        return act;
    }

private:
    struct Play { const Zone* z; double pos, inc, revStart; float gain, pan; bool rev; };
    struct Voice {
        bool active = false, released = false, held = false, lpOn = false;
        int note = 0, stage = 0, np = 0; uint32_t age = 0;
        SrcType src = srcSample;
        const XPInst* def = nullptr;
        Play play[6];
        float env = 0, amp = 1, vel = 1, lpc = 1, lp[4] = {0,0,0,0};
        double atk = 1, dec = 1, rel = 1, vph = 0;
        // synth sources
        double freq = 440, ph[3] = {0,0,0}, fs[3][2] = {{0,0},{0,0},{0,0}}; float cdet = 9;
        double ksLen = 100; int ksPos = 0; float ksPrev = 0, ksDamp = 0.996f, body = 0;
    };

    void release(Voice& v) { v.released = true; v.held = false; v.stage = 3; }

    const Zone* pick(int set, double note) const {
        const std::vector<Zone>& zs = bank.set(set);
        const Zone* best = nullptr; double bd = 1e9;
        for (const Zone& z : zs) { const double d = std::fabs(z.root - note); if (d < bd) { bd = d; best = &z; } }
        return best;
    }
    void addPlay(Voice& v, const Zone* z, double note, double, float gain, float pan, bool rev) {
        if (v.np >= 6) return;
        Play& P = v.play[v.np++];
        P.z = z; P.pos = 0; P.gain = gain * z->gain; P.pan = pan; P.rev = rev;
        P.inc = std::pow(2.0, (note - z->root) / 12.0) * z->rate / srate;
        P.revStart = std::min((double)z->len - 2, 0.9 * z->rate);
    }
    static inline float interp(const float* d, uint32_t len, double pos) {
        const uint32_t i = (uint32_t)pos; const float f = (float)(pos - i);
        const float xm1 = d[i > 0 ? i - 1 : 0], x0 = d[i], x1 = d[i + 1 < len ? i + 1 : len - 1], x2 = d[i + 2 < len ? i + 2 : len - 1];
        const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

    const SampleBank& bank;
    int setIdx[kNumXP][3];
    Voice voices[NV];
    std::vector<float> ks;
    std::atomic<UserSample*> user[kNumUser];
    int userRoot[kNumUser];
    double srate = 44100;
    float relMul = 1, bright = 1;
    uint32_t serial = 0, rng = 0x2468ace1u;

};

} // namespace retrofm
