// RetroFMEngine.hpp - 4-operator FM engine (PC-98 / 90s VN / RPG style)
// Pure C++, no framework dependencies: used by the plugin and by tests.
#pragma once
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>
#include "RetroData.hpp"
#include "RetroDrums.hpp"
#include "RetroSampler.hpp"
#include "RetroVocal.hpp"
#include <string>

namespace retrofm {

static constexpr double kTwoPi = 6.283185307179586;

struct Op { float ratio, level, a, d, s, r; };
struct Patch { int alg; float fb; Op op[4]; };

// algorithm, feedback, then op1..op4: ratio, level, attack, decay, sustain, release
static const Patch kPatches[24] = {
    {4,0.f,   {{14,0.12f,0,0.25f,0,0.3f},{1,0.75f,0,1.6f,0,0.4f},{1,0.3f,0,0.9f,0.1f,0.4f},{1,0.75f,0,2.0f,0,0.4f}}},
    {4,0.f,   {{7,0.25f,0,0.3f,0,0.3f},{2,0.6f,0,0.9f,0,0.6f},{3.5f,0.3f,0,0.5f,0,0.5f},{1,0.6f,0,1.2f,0,0.8f}}},
    {5,0.3f,  {{1,0.35f,0.25f,1,0.8f,0.5f},{1,0.5f,0.3f,1,0.9f,0.6f},{1.003f,0.5f,0.35f,1,0.9f,0.6f},{2,0.25f,0.4f,1,0.9f,0.6f}}},
    {4,0.25f, {{1,0.55f,0.06f,0.4f,0.6f,0.2f},{1,0.7f,0.04f,0.5f,0.85f,0.2f},{1,0.4f,0.08f,0.4f,0.5f,0.2f},{1.002f,0.6f,0.05f,0.5f,0.85f,0.2f}}},
    {4,0.f,   {{1,0.6f,0,0.12f,0,0.05f},{1,0.9f,0,0.6f,0.3f,0.1f},{3,0.25f,0,0.08f,0,0.05f},{0.5f,0.6f,0,0.8f,0.4f,0.1f}}},
    {0,0.5f,  {{1,0.3f,0,0.3f,0.1f,0.1f},{1,0.4f,0,0.4f,0.2f,0.1f},{1,0.3f,0,0.5f,0.3f,0.1f},{1,0.9f,0,1.5f,0.6f,0.1f}}},
    {4,0.f,   {{2,0.3f,0,1,0.9f,0.15f},{1,0.6f,0.005f,1,0.9f,0.15f},{2,0.25f,0,1,0.9f,0.15f},{1.004f,0.5f,0.005f,1,0.9f,0.15f}}},
    {6,0.1f,  {{1,0.1f,0.08f,1,0.7f,0.2f},{1,0.6f,0.08f,1,0.9f,0.2f},{2,0.12f,0.1f,1,0.9f,0.2f},{1,0,0,1,0,0.1f}}},
    {4,0.f,   {{2,0.4f,0.03f,1,0.8f,0.15f},{1,0.6f,0.03f,1,0.9f,0.15f},{2,0.15f,0.04f,1,0.8f,0.15f},{1,0.3f,0.04f,1,0.9f,0.15f}}},
    {7,0.1f,  {{0.5f,0.3f,0.005f,1,1,0.05f},{1,0.4f,0.005f,1,1,0.05f},{2,0.3f,0.005f,1,1,0.05f},{3,0.2f,0.005f,1,1,0.05f}}},
    {4,0.f,   {{3,0.15f,0,0.15f,0,0.3f},{1,0.7f,0,1.0f,0,0.5f},{1,0.2f,0,0.3f,0,0.3f},{1.002f,0.6f,0,1.2f,0,0.5f}}},
    {4,0.f,   {{4,0.25f,0,0.04f,0,0.05f},{1,0.8f,0,0.45f,0,0.2f},{10,0.15f,0,0.02f,0,0.05f},{1,0.5f,0,0.3f,0,0.2f}}},
    {4,0.f,   {{4,0.12f,0,0.5f,0,0.5f},{1,0.7f,0,2.5f,0,0.8f},{1,0.15f,0,1,0,0.6f},{4,0.25f,0,0.8f,0,0.5f}}},
    {5,0.15f, {{1,0.2f,0.4f,1,0.8f,0.8f},{1,0.45f,0.5f,1,0.9f,1},{2.005f,0.25f,0.6f,1,0.9f,1},{0.998f,0.45f,0.5f,1,0.9f,1}}},
    {4,0.f,   {{5,0.1f,0,0.4f,0,0.4f},{2,0.6f,0,1.5f,0,0.6f},{1,0.2f,0,0.6f,0,0.4f},{4,0.35f,0,1.0f,0,0.5f}}},
    {4,0.2f,  {{3,0.3f,0,0.2f,0,0.1f},{1,0.7f,0,0.8f,0,0.2f},{5,0.15f,0,0.1f,0,0.1f},{1,0.5f,0,0.6f,0,0.2f}}},
    {4,0.5f,  {{1,0.45f,0,0.6f,0,0.08f},{1,0.6f,0,1.2f,0,0.15f},{4,0.2f,0,0.4f,0,0.08f},{2,0.3f,0,0.9f,0,0.15f}}},
    {1,0.2f,  {{1,0.25f,0.03f,1,0.8f,0.15f},{3,0.2f,0.03f,1,0.8f,0.15f},{1,0.25f,0.03f,1,0.9f,0.15f},{1,0.6f,0.03f,1,0.9f,0.15f}}},
    {4,0.6f,  {{1,0.5f,0,0.8f,0.2f,0.2f},{1,0.6f,0,1.5f,0,0.3f},{3.01f,0.3f,0,1,0.1f,0.2f},{1.005f,0.5f,0,1.5f,0,0.3f}}},
    {4,0.2f,  {{3,0.25f,0,0.25f,0,0.1f},{1,0.7f,0,1.2f,0,0.25f},{1,0.25f,0,0.5f,0,0.2f},{2,0.4f,0,0.9f,0,0.25f}}},
    {5,0.f,   {{0.5f,0.15f,0.8f,1,0.8f,1.2f},{1,0.45f,0.9f,1,0.9f,1.5f},{1.007f,0.45f,1.0f,1,0.9f,1.5f},{0.5f,0.35f,0.9f,1,0.9f,1.5f}}},
    {7,0.f,   {{1,0.6f,0.01f,1,0.9f,0.2f},{2,0.1f,0.01f,1,0.9f,0.2f},{1.005f,0.3f,0.02f,1,0.9f,0.2f},{1,0,0,1,0,0.1f}}},
    {4,0.f,   {{3.5f,0.35f,0,2,0,1},{1,0.6f,0,4,0,1.5f},{1.41f,0.3f,0,2,0,1},{2,0.4f,0,3,0,1.5f}}},
    {4,0.f,   {{6.3f,0.2f,0,0.08f,0,0.05f},{1,0.75f,0,0.7f,0,0.3f},{1,0.1f,0,0.3f,0,0.2f},{5.6f,0.2f,0,0.15f,0,0.1f}}},
};

class Engine {
public:
    static constexpr int NV = 16;
    static constexpr int CHN = 16384;
    static constexpr int RB = 8192;

    Engine() {
        for (int i = 0; i < kParamCount; ++i) params[i] = kParams[i].def;
        // pattern table: 12 presets + 4 user patterns (start as copies of a few presets)
        for (int i = 0; i < kNumPresetPatterns; ++i) pats[i] = kPatterns[i];
        static const int seed[4] = { 0, 5, 7, 11 };
        for (int u = 0; u < 4; ++u) setUserPattern(u, patternToString(kPatterns[seed[u]]).c_str());
        seq.setTable(pats);
        setSampleRate(44100.0);
    }

    // ---- user drum patterns: "lane|lane|...(9)|;len" with chars X x g -
    static std::string patternToString(const DrumPattern& P) {
        std::string s;
        for (int l = 0; l < kDrumLanes; ++l) { s += std::string(P.lane[l]).substr(0, P.len); s += '|'; }
        s += ';'; s += std::to_string(P.len);
        return s;
    }
    void setUserPattern(int u, const char* str) {
        if (u < 0 || u >= 4 || !str) return;
        char (*L)[17] = userLanes[u];
        int len = 16;
        const char* semi = std::strchr(str, ';');
        if (semi) { len = std::atoi(semi + 1); if (len != 12) len = 16; }
        int lane = 0, k = 0;
        for (int l = 0; l < kDrumLanes; ++l) { std::memset(L[l], '-', 16); L[l][16] = 0; }
        for (const char* c = str; *c && c != semi && lane < kDrumLanes; ++c) {
            if (*c == '|') { ++lane; k = 0; continue; }
            if (k < 16) L[lane][k++] = (*c == 'X' || *c == 'x' || *c == 'g') ? *c : '-';
        }
        DrumPattern& P = pats[kNumPresetPatterns + u];
        P.name = kUserPatternNames[u]; P.len = len; P.swing = 0;
        for (int l = 0; l < kDrumLanes; ++l) P.lane[l] = L[l];
    }
    std::string getUserPattern(int u) const { return patternToString(pats[kNumPresetPatterns + u]); }

    // ---- user samples (call from a non-audio thread)
    bool loadUserSample(int k, const char* path) {
        if (k < 0 || k >= kNumUser) return false;
        if (!path || !*path) { sampler.setUser(k, nullptr); userPath[k].clear(); return true; }
        UserSample* u = new UserSample();
        double rate = 44100;
        if (!loadAudioFileMono(path, u->data, rate) || u->data.size() < 8) { delete u; return false; }
        float pk = 0; for (float x : u->data) pk = std::max(pk, std::fabs(x));
        if (pk > 0) for (float& x : u->data) x *= 0.9f / pk;
        u->data.push_back(0); u->data.push_back(0);
        u->zone.data = u->data.data(); u->zone.len = (uint32_t)u->data.size() - 2; u->zone.rate = rate; u->zone.root = 60;
        u->path = path;
        loaded.push_back(std::unique_ptr<UserSample>(u));   // kept alive until the plugin is destroyed
        sampler.setUser(k, u);
        userPath[k] = path;
        return true;
    }
    void setUserRoot(int k, int n) { if (k >= 0 && k < kNumUser) { userRootNote[k] = n; sampler.setUserRoot(k, n); } }
    const std::string& getUserPath(int k) const { return userPath[k]; }
    int getUserRoot(int k) const { return userRootNote[k]; }

    void setSampleRate(double sr) {
        srate = sr;
        static const int tbl[12] = {1116,1188,1277,1356,1139,1211,1300,1379,556,441,579,464};
        for (int i = 0; i < 12; ++i) {
            int L = (int)std::floor(tbl[i] * sr / 44100.0);
            rvLen[i] = L < RB - 1 ? L : RB - 1; rvPos[i] = 0; rvSt[i] = 0;
        }
        std::memset(rvBuf, 0, sizeof(rvBuf));
        std::memset(chb, 0, sizeof(chb));
        cw = 0; cph = vph = 0; hcnt = 0; held = 0;
        for (auto& v : voices) v.active = false;
        drums.setSampleRate(sr); seq.setSampleRate(sr); sampler.setSampleRate(sr); vocal.setSampleRate(sr);
        update();
    }

    void setParam(int i, float v) { if (i >= 0 && i < kParamCount) { params[i] = v; update(); } }
    float getParam(int i) const { return (i >= 0 && i < kParamCount) ? params[i] : 0.f; }

    void midi(const uint8_t* d, uint32_t size) {
        if (size < 1) return;
        const uint8_t st = d[0] & 0xF0;
        const uint8_t m2 = size > 1 ? d[1] : 0, m3 = size > 2 ? d[2] : 0;
        if ((d[0] & 0x0F) == 9) {            // MIDI channel 10 = drums (GM map)
            if (st == 0x90 && m3 > 0) drums.trigger(gmLane(m2), m3 / 127.0f);
            return;
        }
        if (st == 0x90 && m3 > 0) {
            const int inst = (int)std::floor(params[pInstrument] + 0.5f);
            if (params[pVocalMode] > 0.5f) vocal.noteOn(m2 + (int)params[pTranspose], m3);
            else if (inst < kNumFM) noteOn(m2, m3);
            else sampler.noteOn(inst, m2, m3, params[pTranspose]);
        }
        else if (st == 0x80 || (st == 0x90 && m3 == 0)) { noteOff(m2); sampler.noteOff(m2, pedal); vocal.noteOff(m2 + (int)params[pTranspose]); }
        else if (st == 0xE0) { bend = ((m3 * 128 + m2) - 8192) / 8192.0; bendmul = std::pow(2.0, bend * params[pBendRange] / 12.0); }
        else if (st == 0xB0) {
            if (m2 == 1) modwheel = m3 / 127.0;
            else if (m2 == 64) { bool p = m3 >= 64; if (pedal && !p) pedalUpAll(); pedal = p; }
            else if (m2 == 120 || m2 == 123) { allOff(); sampler.allOff(); vocal.allOff(); }
        }
    }

    // call once per audio block with host transport info
    void transport(bool playing, bool posValid, double posSteps, double bpm) {
        if (playing && !wasPlaying) vocal.rewind();      // lyrics restart with the song
        wasPlaying = playing;
        seq.transport((int)params[pDrumMode], playing, posValid, posSteps, bpm);
    }

    // values for the UI
    float outActivity() const { return (float)activity; }
    float outDrumStep() const { return (float)seq.currentStep(); }
    float outBeat() const { return drums.pulse; }
    float outMouth() const { return vocal.mouthOpen(); }
    float outVowel() const { return (float)vocal.vowel(); }
    float outSyll() const { return (float)vocal.currentSyllable(); }

    // ---- vocal lyrics (call from a non-audio thread)
    void setLyrics(const char* text) { vocal.setLyrics(text); }
    const std::string& getLyrics() const { return vocal.lyrics(); }
    void rewindLyrics() { vocal.rewind(); }

    void process(float* outL, float* outR, uint32_t n) {
        for (uint32_t s = 0; s < n; ++s) { float l, r; tick(l, r); outL[s] = l; outR[s] = r; }
    }

    void tick(float& outL, float& outR) {
        vph += vrateInc; if (vph >= 1) vph -= 1;
        const double pmul = bendmul * std::pow(2.0, std::sin(kTwoPi * vph) * (params[pVibDepth] + modwheel * 40) / 1200.0);
        double mix = 0, act = 0;
        float sL = 0, sR = 0;
        act += sampler.tick(pmul, sL, sR) * 0.8;
        const float vox = vocal.tick(bendmul);
        sL += vox * 0.75f; sR += vox * 0.75f;
        act += vocal.mouthOpen();
        for (auto& v : voices) {
            if (!v.active) continue;
            const double base = v.inc * pmul;
            for (int k = 0; k < 4; ++k) { v.ph[k] += base * ratio[k]; v.ph[k] -= std::floor(v.ph[k]); }
            double e[4];
            for (int k = 0; k < 4; ++k) e[k] = env(v, k);
            double a[4];
            for (int k = 0; k < 4; ++k) a[k] = e[k] * el[k] * (car[k] ? v.vc : v.vm);
            const double o1 = std::sin(kTwoPi * (v.ph[0] + fbk * (v.fb0 + v.fb1) * 0.5)) * a[0];
            v.fb1 = v.fb0; v.fb0 = o1;
            double o2, o3, o4, out = 0;
            auto S = [&](int k, double m) { return std::sin(kTwoPi * (v.ph[k] + m)) * a[k]; };
            switch (alg) {
                case 0: o2 = S(1,o1); o3 = S(2,o2); o4 = S(3,o3); out = o4; break;
                case 1: o2 = S(1,0); o3 = S(2,o1+o2); o4 = S(3,o3); out = o4; break;
                case 2: o2 = S(1,0); o3 = S(2,o2); o4 = S(3,o1+o3); out = o4; break;
                case 3: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,o2+o3); out = o4; break;
                case 4: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,o3); out = o2+o4; break;
                case 5: o2 = S(1,o1); o3 = S(2,o1); o4 = S(3,o1); out = o2+o3+o4; break;
                case 6: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,0); out = o2+o3+o4; break;
                default: o2 = S(1,0); o3 = S(2,0); o4 = S(3,0); out = o1+o2+o3+o4; break;
            }
            mix += out;
            act += e[3] * v.vc;
            double emax = e[3];
            for (int k = 0; k < 3; ++k) if (car[k] && e[k] > emax) emax = e[k];
            if (v.stage[3] >= 2 && emax < 0.0001) v.active = false;
        }

        activity += ((act > 1 ? 1 : act) - activity) * 0.0005;
        double dry = mix * norm * gain + (sL + sR) * 0.5 * gain * 1.6;
        const double side = (sL - sR) * 0.5 * gain * 1.6;
        if (lofi > 0) {
            if (++hcnt >= hold) { hcnt = 0; held = std::floor(dry * q + 0.5) / q; }
            dry = held;
        }
        chb[cw] = (float)dry;
        cph += 0.7 / srate; if (cph >= 1) cph -= 1;
        const double tapL = chread((0.012 + 0.003 * std::sin(kTwoPi * cph)) * srate);
        const double tapR = chread((0.012 + 0.003 * std::cos(kTwoPi * cph)) * srate);
        if (++cw >= CHN) cw = 0;
        const double chm = params[pChorus];
        const double L = dry * (1 - 0.4 * chm) + tapL * 0.7 * chm + side;
        const double R = dry * (1 - 0.4 * chm) + tapR * 0.7 * chm - side;

        seq.tick(drums, params[pDrumFill] > 0.5f);
        const double dr = drums.tick() * drumGain;

        const double rin = ((L + R) * 0.5 + dr * 0.35) * 0.015;
        double cl = comb(0,rin) + comb(1,rin) + comb(2,rin) + comb(3,rin);
        double cr = comb(4,rin) + comb(5,rin) + comb(6,rin) + comb(7,rin);
        cl = apass(9, apass(8, cl));
        cr = apass(11, apass(10, cr));
        const double rv = params[pReverb];
        outL = (float)softclip(L + dr + cl * 3 * rv);
        outR = (float)softclip(R + dr + cr * 3 * rv);
    }

private:
    struct Voice {
        bool active = false, released = false, held = false;
        int note = 0, stage[4] = {0,0,0,0};
        double inc = 0, vc = 1, vm = 1, ph[4] = {0,0,0,0}, e[4] = {0,0,0,0};
        double atk[4] = {0,0,0,0}, dec[4] = {0,0,0,0}, rel[4] = {0,0,0,0};
        double fb0 = 0, fb1 = 0;
        uint32_t age = 0;
    };

    double env(Voice& v, int k) {
        double e = v.e[k];
        if (v.stage[k] == 1) { e += v.atk[k]; if (e >= 1) { e = 1; v.stage[k] = 2; } }
        if (v.stage[k] == 2) { const double s = patch->op[k].s; e = s + (e - s) * v.dec[k]; }
        if (v.stage[k] == 3) e *= v.rel[k];
        v.e[k] = e;
        return e;
    }

    void noteOn(int n, int vel) {
        Voice* best = nullptr;
        for (auto& v : voices) if (!v.active) { best = &v; break; }
        if (!best) { uint32_t ba = 0xFFFFFFFFu; for (auto& v : voices) if (v.age < ba) { ba = v.age; best = &v; } }
        Voice& v = *best;
        const double nn = n + params[pTranspose];
        v.active = true; v.released = false; v.held = false; v.note = n;
        v.inc = 440.0 * std::pow(2.0, (nn - 69) / 12.0) / srate;
        const double x = vel / 127.0, vs = params[pVelSens];
        v.vc = (1 - vs) + vs * x * x;
        v.vm = (1 - vs * 0.6) + vs * 0.6 * x;
        v.age = ++serial; v.fb0 = v.fb1 = 0;
        double ks = std::pow(2.0, -(nn - 60) / 24.0); ks = ks < 0.3 ? 0.3 : (ks > 3 ? 3 : ks);
        for (int k = 0; k < 4; ++k) {
            const Op& o = patch->op[k];
            v.ph[k] = 0; v.e[k] = 0; v.stage[k] = 1;
            v.atk[k] = o.a <= 0.001f ? 1.0 : 1.0 / (o.a * srate);
            v.dec[k] = std::exp(-1.0 / (std::fmax(o.d * ks, 0.002) * srate));
            v.rel[k] = std::exp(-1.0 / (std::fmax(o.r * params[pRelease], 0.003) * srate));
        }
    }
    void release(Voice& v) { v.released = true; v.held = false; for (int k = 0; k < 4; ++k) v.stage[k] = 3; }
    void noteOff(int n) { for (auto& v : voices) if (v.active && v.note == n && !v.released) { if (pedal) v.held = true; else release(v); } }
    void pedalUpAll() { pedalUp(); sampler.pedalUp(); }
    void pedalUp() { for (auto& v : voices) if (v.active && v.held) release(v); }
    void allOff() { pedal = false; for (auto& v : voices) if (v.active) release(v); }

    double chread(double d) const {
        double rp = cw - d; if (rp < 0) rp += CHN;
        int i0 = (int)rp; const double fr = rp - i0; int i1 = i0 + 1; if (i1 >= CHN) i1 = 0;
        return chb[i0] * (1 - fr) + chb[i1] * fr;
    }
    double comb(int i, double x) {
        float* b = rvBuf[i]; const double y = b[rvPos[i]];
        rvSt[i] = y * (1 - 0.25) + rvSt[i] * 0.25;
        b[rvPos[i]] = (float)(x + rvSt[i] * 0.84);
        if (++rvPos[i] >= rvLen[i]) rvPos[i] = 0;
        return y;
    }
    double apass(int i, double x) {
        float* b = rvBuf[i]; const double y = b[rvPos[i]];
        b[rvPos[i]] = (float)(x + y * 0.5);
        if (++rvPos[i] >= rvLen[i]) rvPos[i] = 0;
        return y - x;
    }

    void update() {
        int pn = (int)std::floor(params[pInstrument] + 0.5f); pn = pn < 0 ? 0 : (pn > 23 ? 23 : pn);
        patch = &kPatches[pn];
        alg = patch->alg; fbk = patch->fb;
        car[0] = alg == 7; car[1] = alg >= 4; car[2] = alg >= 5; car[3] = true;
        for (int k = 0; k < 4; ++k) { ratio[k] = patch->op[k].ratio; el[k] = patch->op[k].level * (car[k] ? 1.0 : params[pBrightness]); }
        norm = 1.0 / std::sqrt((double)(car[0] + car[1] + car[2] + car[3]));
        gain = std::pow(10.0, params[pVolume] / 20.0);
        lofi = params[pLofi]; hold = 1 + (int)std::floor(lofi * 7); q = std::pow(2.0, 15 - lofi * 10);
        vrateInc = params[pVibRate] / srate;
        bendmul = std::pow(2.0, bend * params[pBendRange] / 12.0);
        seq.setPattern((int)std::floor(params[pDrumPattern] + 0.5f));
        sampler.setRelease(params[pRelease]);
        {
            VoiceParams vp;
            vp.gender = params[pVGender]; vp.formant = params[pVFormant]; vp.breath = params[pVBreath];
            vp.vib = params[pVVib]; vp.vibRate = params[pVVibRate]; vp.bright = params[pVBright]; vp.glide = params[pVGlide];
            vocal.setParams(vp);
            vocal.setRelease(params[pRelease]);
            if (params[pVocalMode] < 0.5f) vocal.allOff();
        }
        sampler.setBrightness(params[pBrightness]);
        drums.setKit((int)std::floor(params[pDrumKit] + 0.5f));
        drumGain = std::pow(10.0, params[pDrumVolume] / 20.0);
    }

    static double softclip(double x) {
        const double a = std::fabs(x);
        if (a <= 0.8) return x;
        const double y = 0.8 + 0.2 * std::tanh((a - 0.8) / 0.2);
        return x < 0 ? -y : y;
    }

    static int gmLane(int n) {
        switch (n) {
            case 35: case 36: return dKick;
            case 38: case 40: return dSnare;
            case 37: return dRim;
            case 39: return dClap;
            case 42: case 44: return dCHat;
            case 46: return dOHat;
            case 41: case 43: case 45: return dTomL;
            case 47: case 48: case 50: return dTomH;
            case 49: case 51: case 52: case 55: case 57: return dCrash;
            default: return -1;
        }
    }

    double srate = 44100;
    float params[kParamCount];
    const Patch* patch = &kPatches[0];
    int alg = 4; double fbk = 0; bool car[4] = {false,true,false,true};
    double ratio[4] = {1,1,1,1}, el[4] = {0,0,0,0}, norm = 1, gain = 1;
    double lofi = 0, q = 32768, held = 0; int hold = 1, hcnt = 0;
    double vph = 0, vrateInc = 0, cph = 0, bend = 0, bendmul = 1, modwheel = 0;
    bool pedal = false; uint32_t serial = 0;
    Sampler sampler;
    VocalSynth vocal;
    bool wasPlaying = false;
    DrumPattern pats[kNumPatterns];
    char userLanes[4][kDrumLanes][17];
    std::string userPath[kNumUser];
    int userRootNote[kNumUser] = {60, 60, 60, 60};
    std::vector<std::unique_ptr<UserSample>> loaded;
    DrumSynth drums; DrumSequencer seq; double drumGain = 0.5, activity = 0;
    Voice voices[NV];
    float chb[CHN]; int cw = 0;
    float rvBuf[12][RB]; int rvLen[12], rvPos[12]; double rvSt[12];
};

} // namespace retrofm
