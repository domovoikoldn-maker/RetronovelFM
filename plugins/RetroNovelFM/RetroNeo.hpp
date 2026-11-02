// RetroNeo.hpp - "REIWA" era: modern synths heard in 2020s anime games, gacha RPGs and VN openings
// (future bass chords, kawaii leads, plucks, city pop keys, 808 / wobble / reese basses, synthwave,
//  hyperpop, J-core hoover, vocal chops, erhu and shamisen for "wa"-style themes).
// Virtual-analog voice: unison oscillators -> state-variable filter with envelope + tempo-synced LFO
// -> amp envelope -> drive / crush -> sidechain-style pump locked to the beat.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include "RetroData.hpp"
#include "RetroSampler.hpp"

namespace retrofm {

enum NeoOsc { noSaw, noSquare, noSine, noFM, noSample, noVox, noPluck, noCowbell };
enum NeoFilter { nfLP, nfBP, nfHP };

struct NeoPatch {
    int osc = noSaw; int uni = 1; float detune = 0, spread = 0.6f;
    float osc2Semi = 0, osc2Lvl = 0, sub = 0, noise = 0, pw = 0.5f;
    float fmRatio = 1, fmIndex = 0, fmDecay = 1, fm2Ratio = 14, fm2Index = 0, fm2Decay = 0.05f;
    int filt = nfLP; float cutoff = 18000, reso = 0, envAmt = 0, fA = 0.001f, fD = 0.3f, fS = 1, keyTrack = 0.5f;
    float lfoAmt = 0, lfoRate = 1; int lfoSync = 0;      // lfoSync > 0: one LFO cycle every N sixteenths
    float a = 0.002f, d = 1, s = 1, r = 0.3f;
    float pitchEnv = 0, pitchDec = 0.05f;
    float glide = 0; int mono = 0;
    float vib = 0, vibDelay = 0.3f, vibRate = 5.5f;
    float pump = 0, drive = 0, crush = 0, level = 0.5f;
    int formant = -1;                                       // noVox: -1 cycle vowels, else fixed formant set
    const char* sample = nullptr;
    float bodyMix = 0;                                      // noPluck: body resonance
};

static const NeoPatch* neoPatches()
{
    static NeoPatch P[kNumNeo];
    static bool init = false;
    if (init) return P;
    init = true;
    NeoPatch* p;
    // 0 Future Chord: wide supersaw stack, filter envelope, pumping
    p = &P[0];  p->osc = noSaw; p->uni = 7; p->detune = 22; p->spread = 0.9f; p->cutoff = 1800; p->envAmt = 1.6f; p->fD = 0.55f; p->fS = 0.45f; p->reso = 0.15f;
                p->a = 0.004f; p->d = 1.2f; p->s = 0.85f; p->r = 0.3f; p->pump = 0.75f; p->level = 0.32f; p->sub = 0.15f;
    // 1 Kawaii Lead: bright pulse, chirp, delayed vibrato, glide
    p = &P[1];  p->osc = noSquare; p->uni = 2; p->detune = 7; p->pw = 0.32f; p->cutoff = 6500; p->reso = 0.1f; p->pitchEnv = 4; p->pitchDec = 0.018f;
                p->vib = 28; p->vibDelay = 0.22f; p->vibRate = 6.3f; p->glide = 0.045f; p->mono = 1; p->r = 0.12f; p->level = 0.42f;
    // 2 Future Pluck
    p = &P[2];  p->osc = noSaw; p->uni = 5; p->detune = 18; p->cutoff = 380; p->envAmt = 5.2f; p->fA = 0.001f; p->fD = 0.16f; p->fS = 0; p->reso = 0.2f;
                p->a = 0.001f; p->d = 0.38f; p->s = 0; p->r = 0.22f; p->pump = 0.35f; p->level = 0.62f;
    // 3 Crystal Pluck: FM bell-pluck
    p = &P[3];  p->osc = noFM; p->uni = 2; p->detune = 6; p->fmRatio = 4; p->fmIndex = 3.2f; p->fmDecay = 0.12f; p->fm2Ratio = 7.1f; p->fm2Index = 0.8f; p->fm2Decay = 0.04f;
                p->a = 0.001f; p->d = 0.7f; p->s = 0; p->r = 0.45f; p->level = 0.5f;
    // 4 Bell Pad
    p = &P[4];  p->osc = noFM; p->uni = 2; p->detune = 7; p->fmRatio = 3; p->fmIndex = 1.6f; p->fmDecay = 1.4f; p->fm2Ratio = 0.5f; p->fm2Index = 0.3f; p->fm2Decay = 4;
                p->a = 0.25f; p->d = 2; p->s = 0.7f; p->r = 1.8f; p->level = 0.36f; p->lfoAmt = 0; p->vib = 4; p->vibDelay = 0.5f; p->vibRate = 4.5f;
    // 5 City Pop EP: DX-style tine piano
    p = &P[5];  p->osc = noFM; p->uni = 2; p->detune = 5; p->fmRatio = 1; p->fmIndex = 2.0f; p->fmDecay = 0.9f; p->fm2Ratio = 14; p->fm2Index = 0.9f; p->fm2Decay = 0.035f;
                p->a = 0.001f; p->d = 2.6f; p->s = 0.12f; p->r = 0.35f; p->level = 0.5f;
    // 6 Lo-Fi Keys: worn EP with tape wow
    p = &P[6];  *p = P[5]; p->cutoff = 2100; p->keyTrack = 0.2f; p->vib = 14; p->vibDelay = 0; p->vibRate = 0.55f; p->noise = 0.025f; p->crush = 0.15f; p->level = 0.6f; p->fmIndex = 1.5f;
    // 7 Vapor Pad
    p = &P[7];  p->osc = noSaw; p->uni = 5; p->detune = 18; p->osc2Semi = 12; p->osc2Lvl = 0.25f; p->cutoff = 1300; p->reso = 0.12f; p->lfoAmt = 0.35f; p->lfoRate = 0.18f;
                p->a = 0.8f; p->d = 2; p->s = 1; p->r = 2.4f; p->vib = 9; p->vibDelay = 0; p->vibRate = 0.3f; p->level = 0.42f;
    // 8 Shimmer Pad
    p = &P[8];  p->osc = noSaw; p->uni = 3; p->detune = 10; p->osc2Semi = 12; p->osc2Lvl = 0.55f; p->cutoff = 5200; p->lfoAmt = 0.4f; p->lfoRate = 0.22f;
                p->a = 1.1f; p->d = 2; p->s = 1; p->r = 3.0f; p->level = 0.38f; p->filt = nfLP;
    // 9 Synthwave Lead
    p = &P[9];  p->osc = noSaw; p->uni = 2; p->detune = 11; p->osc2Semi = -12; p->osc2Lvl = 0.4f; p->cutoff = 2800; p->reso = 0.3f; p->envAmt = 1.2f; p->fD = 0.35f; p->fS = 0.6f;
                p->vib = 22; p->vibDelay = 0.3f; p->vibRate = 5.4f; p->glide = 0.05f; p->mono = 1; p->r = 0.25f; p->level = 0.4f;
    // 10 Synthwave Pad (brassy)
    p = &P[10]; p->osc = noSaw; p->uni = 4; p->detune = 14; p->cutoff = 700; p->envAmt = 2.0f; p->fA = 0.25f; p->fD = 0.8f; p->fS = 0.5f; p->reso = 0.1f;
                p->a = 0.12f; p->d = 1; p->s = 1; p->r = 0.8f; p->level = 0.32f;
    // 11 Hyperpop Lead: detuned square, pitch scoop, bit crush, drive
    p = &P[11]; p->osc = noSquare; p->uni = 3; p->detune = 26; p->pw = 0.5f; p->pitchEnv = 12; p->pitchDec = 0.03f; p->crush = 0.55f; p->drive = 2.2f;
                p->cutoff = 9000; p->glide = 0.02f; p->mono = 1; p->r = 0.1f; p->level = 0.32f;
    // 12 Supersaw Lead
    p = &P[12]; p->osc = noSaw; p->uni = 7; p->detune = 25; p->spread = 0.8f; p->cutoff = 9000; p->vib = 12; p->vibDelay = 0.25f; p->vibRate = 5.8f;
                p->glide = 0.03f; p->mono = 1; p->r = 0.2f; p->level = 0.45f;
    // 13 J-Core Hoover
    p = &P[13]; p->osc = noSaw; p->uni = 5; p->detune = 42; p->osc2Semi = -12; p->osc2Lvl = 0.6f; p->pitchEnv = 5; p->pitchDec = 0.12f; p->cutoff = 4200; p->reso = 0.2f;
                p->glide = 0.08f; p->mono = 1; p->drive = 1.4f; p->r = 0.2f; p->level = 0.4f;
    // 14 Vocal Chop
    p = &P[14]; p->osc = noVox; p->a = 0.004f; p->d = 0.26f; p->s = 0; p->r = 0.12f; p->pitchEnv = 1; p->pitchDec = 0.04f; p->level = 1.9f; p->noise = 0.04f; p->pump = 0.2f;
    // 15 Wobble Bass: tempo-synced filter wobble (1/8)
    p = &P[15]; p->osc = noSaw; p->uni = 3; p->detune = 12; p->sub = 0.6f; p->cutoff = 260; p->reso = 0.55f; p->lfoAmt = 3.6f; p->lfoSync = 2; p->keyTrack = 0.3f;
                p->glide = 0.03f; p->mono = 1; p->drive = 1.6f; p->r = 0.08f; p->level = 0.45f;
    // 16 Reese Bass
    p = &P[16]; p->osc = noSaw; p->uni = 2; p->detune = 26; p->sub = 0.55f; p->cutoff = 900; p->reso = 0.2f; p->lfoAmt = 0.45f; p->lfoRate = 0.15f; p->keyTrack = 0.3f;
                p->mono = 1; p->glide = 0.02f; p->drive = 1.2f; p->r = 0.12f; p->level = 0.45f;
    // 17 808 Glide: long sine bass with a punch and slides between overlapping notes
    p = &P[17]; p->osc = noSine; p->pitchEnv = 12; p->pitchDec = 0.035f; p->a = 0.001f; p->d = 1.6f; p->s = 0.35f; p->r = 0.25f; p->glide = 0.12f; p->mono = 1;
                p->drive = 2.4f; p->level = 0.8f; p->keyTrack = 0;
    // 18 Slap Funk Bass (FM)
    p = &P[18]; p->osc = noFM; p->fmRatio = 1; p->fmIndex = 3.4f; p->fmDecay = 0.07f; p->fm2Ratio = 3; p->fm2Index = 0.6f; p->fm2Decay = 0.03f; p->sub = 0.4f;
                p->cutoff = 3200; p->a = 0.001f; p->d = 0.6f; p->s = 0.35f; p->r = 0.08f; p->level = 0.7f;
    // 19 Acid 303: resonant filter sweep, accent by velocity, slides
    p = &P[19]; p->osc = noSaw; p->cutoff = 260; p->reso = 0.86f; p->envAmt = 3.8f; p->fA = 0.001f; p->fD = 0.2f; p->fS = 0; p->keyTrack = 0.4f;
                p->a = 0.001f; p->d = 0.5f; p->s = 0.75f; p->r = 0.06f; p->glide = 0.06f; p->mono = 1; p->drive = 1.3f; p->level = 0.5f;
    // 20 Phonk Cowbell (808 style: two detuned squares through a band-pass)
    p = &P[20]; p->osc = noCowbell; p->filt = nfBP; p->cutoff = 2400; p->reso = 0.35f; p->keyTrack = 1; p->a = 0.0005f; p->d = 0.33f; p->s = 0; p->r = 0.1f; p->drive = 1.8f; p->level = 0.9f;
    // 21 Wah Funk Guitar: real clean guitar through an envelope wah
    p = &P[21]; p->osc = noSample; p->sample = "egtr"; p->filt = nfBP; p->cutoff = 420; p->reso = 0.72f; p->envAmt = 3.1f; p->fA = 0.006f; p->fD = 0.22f; p->fS = 0.15f; p->keyTrack = 0.2f;
                p->a = 0.001f; p->d = 0.9f; p->s = 0.25f; p->r = 0.06f; p->drive = 0.9f; p->level = 2.6f;
    // 22 Erhu: bowed two-string fiddle with nasal body, slides and expressive vibrato
    p = &P[22]; p->osc = noVox; p->formant = 5; p->noise = 0.05f; p->a = 0.07f; p->d = 1; p->s = 1; p->r = 0.18f; p->vib = 32; p->vibDelay = 0.2f; p->vibRate = 5.6f;
                p->glide = 0.09f; p->mono = 1; p->level = 1.6f;
    // 23 Shamisen: plucked string with buzzing bridge (sawari) and a hard plectrum
    p = &P[23]; p->osc = noPluck; p->a = 0.0005f; p->d = 1.2f; p->s = 0; p->r = 0.15f; p->bodyMix = 0.5f; p->level = 1.7f;
    return P;
}

class NeoSynth {
public:
    static constexpr int NV = 16;
    static constexpr int KS = 4096;

    NeoSynth() : bank(SampleBank::get()) { neoPatches(); ks.assign(NV * KS, 0.f); }

    void setSampleRate(double sr) { srate = sr; for (auto& v : voices) v.active = false; nstack = 0; }
    void setRelease(float r) { relMul = r; }
    void setBrightness(float b) { bright = std::pow(2.0f, (b - 1.f) * 2.f); }
    void setVelSens(float s) { velSens = s; }

    void noteOn(int inst, int note, int vel, double transpose, float gainMul = 1.f) {
        if (!isNeo(inst)) return;
        const int pi = inst - kFirstNeo;
        const NeoPatch& P = neoPatches()[pi];
        const double n = note + transpose;
        const double f = 440.0 * std::pow(2.0, (n - 69) / 12.0);
        if (P.mono) {
            // last-note priority with glide; legato keeps the envelope running
            pushStack(note);
            for (auto& v : voices)
                if (v.active && v.mono && !v.released && v.patch == pi) {
                    v.note = note; v.target = f; v.vel = vel / 127.f;
                    if (P.glide <= 0) v.freq = f;
                    return;
                }
            Voice& v = alloc();
            start(v, pi, note, vel, f, gainMul);
            if (lastMonoFreq > 0 && P.glide > 0 && lastMonoPatch == pi && nstack > 1) v.freq = lastMonoFreq;
            v.mono = true;
            return;
        }
        Voice& v = alloc();
        start(v, pi, note, vel, f, gainMul);
    }

    void noteOff(int note, bool pedal, double transpose) {
        // mono voices: fall back to the previous held note
        popStack(note);
        for (auto& v : voices) {
            if (!v.active || v.released || v.note != note) continue;
            if (v.mono && nstack > 0) {
                const int nn = stack[nstack - 1];
                v.note = nn; v.target = 440.0 * std::pow(2.0, (nn + transpose - 69) / 12.0);
                if (neoPatches()[v.patch].glide <= 0) v.freq = v.target;
                continue;
            }
            if (pedal) v.held = true; else release(v);
        }
    }
    void pedalUp() { for (auto& v : voices) if (v.active && v.held) release(v); }
    void allOff() { nstack = 0; for (auto& v : voices) if (v.active) release(v); }

    // clock: song position in 16ths (for synced LFO and pump); spp = sixteenths per sample
    float tick(double pmul, double clock, float& outL, float& outR) {
        float act = 0;
        const double beatPh = clock / 4.0 - std::floor(clock / 4.0);
        const double pumpShape = std::pow(std::max(0.0, 1.0 - beatPh / 0.42), 2.0);
        for (auto& v : voices) {
            if (!v.active) continue;
            const NeoPatch& P = neoPatches()[v.patch];
            // envelopes
            if (v.stage == 1) { v.env += (float)v.atk; if (v.env >= 1) { v.env = 1; v.stage = 2; } }
            else if (v.stage == 2) v.env = P.s + (v.env - P.s) * (float)v.dec;
            else { v.env *= (float)v.rel; if (v.env < 0.0001f) { v.active = false; continue; } }
            if (v.stage == 2 && P.s <= 0 && v.env < 0.0001f) { v.active = false; continue; }
            if (v.fstage == 1) { v.fenv += (float)v.fatk; if (v.fenv >= 1) { v.fenv = 1; v.fstage = 2; } }
            else v.fenv = P.fS + (v.fenv - P.fS) * (float)v.fdec;
            v.t += 1.0 / srate;

            // pitch: glide, pitch envelope, delayed vibrato, bend
            if (P.glide > 0) v.freq += (v.target - v.freq) * glideCoef(P.glide);
            else v.freq = v.target;
            double pm = pmul;
            if (P.pitchEnv != 0) pm *= std::pow(2.0, P.pitchEnv * std::exp(-v.t / P.pitchDec) / 12.0);
            if (P.vib > 0) {
                v.vph += P.vibRate / srate; if (v.vph >= 1) v.vph -= 1;
                const double ramp = std::min(1.0, std::max(0.0, (v.t - P.vibDelay) / 0.35));
                pm *= std::pow(2.0, std::sin(6.2831853 * v.vph) * P.vib * ramp / 1200.0);
            }
            const double f = v.freq * pm;
            if (P.mono) { lastMonoFreq = v.freq; lastMonoPatch = v.patch; }

            // oscillators
            float l = 0, r = 0;
            oscillate(v, P, f, l, r);

            // filter
            double lfo = 0;
            if (P.lfoAmt > 0) {
                if (P.lfoSync > 0) { const double ph = clock / P.lfoSync; lfo = -std::cos(6.2831853 * (ph - std::floor(ph))); }
                else { v.lph += P.lfoRate / srate; if (v.lph >= 1) v.lph -= 1; lfo = std::sin(6.2831853 * v.lph); }
            }
            if (P.cutoff < 17500 || P.filt != nfLP || P.envAmt > 0 || P.lfoAmt > 0) {
                const double velAmt = 0.5 + 0.5 * v.vel;
                double fc = P.cutoff * std::pow(2.0, P.envAmt * v.fenv * velAmt + P.lfoAmt * lfo * 0.5 + P.keyTrack * (v.note - 60) / 12.0) * bright;
                fc = std::max(25.0, std::min(fc, srate * 0.45));
                const double g = std::tan(3.14159265 * fc / srate);
                const double k = 2.0 - 1.96 * P.reso;
                const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
                l = svf(v.fs[0], v.fs[1], l, a1, a2, a3, k, P.filt);
                r = svf(v.fs[2], v.fs[3], r, a1, a2, a3, k, P.filt);
            }
            if (P.drive > 0) { const float dg = 1.f + P.drive; const float nrm = 1.f / std::tanh(dg * 0.6f); l = std::tanh(l * dg * 0.6f) * nrm * 0.8f; r = std::tanh(r * dg * 0.6f) * nrm * 0.8f; }
            if (P.crush > 0) {
                const int hold = 1 + (int)(P.crush * 5);
                if (++v.crushCnt >= hold) { v.crushCnt = 0; const float q = std::pow(2.f, 9.f - 4.f * P.crush); v.chL = std::floor(l * q + 0.5f) / q; v.chR = std::floor(r * q + 0.5f) / q; }
                l = v.chL; r = v.chR;
            }
            float a = v.env * v.amp;
            if (P.pump > 0) a *= (float)(1.0 - P.pump * 0.85 * pumpShape);
            outL += l * a; outR += r * a;
            act += v.env;
        }
        return act;
    }

private:
    struct Voice {
        bool active = false, released = false, held = false, mono = false;
        int note = 0, patch = 0, stage = 0, fstage = 0, vowel = 0, crushCnt = 0;
        uint32_t age = 0;
        double freq = 440, target = 440, t = 0, vph = 0, lph = 0;
        double ph[8] = {0}, ph2[8] = {0}, subPh = 0, fmPh = 0, fm2Ph = 0, cbPh[2] = {0, 0};
        float pan[8] = {0};
        float env = 0, fenv = 0, vel = 1, amp = 1, chL = 0, chR = 0;
        double atk = 1, dec = 1, rel = 1, fatk = 1, fdec = 1;
        float fs[4] = {0, 0, 0, 0}, fm[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
        // sample osc
        const Zone* z = nullptr; double spos = 0, sinc = 0;
        // pluck
        double ksLen = 100; int ksPos = 0; float ksPrev = 0, body[2] = {0, 0};
    };

    static inline double blep(double p, double dt) {
        if (p < dt) { const double t = p / dt; return t + t - t * t - 1.0; }
        if (p > 1.0 - dt) { const double t = (p - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }
    static inline float svf(float& ic1, float& ic2, float v0, double a1, double a2, double a3, double k, int mode) {
        const double v3 = v0 - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = (float)(2 * v1 - ic1); ic2 = (float)(2 * v2 - ic2);
        if (mode == nfBP) return (float)(v1 * k);
        if (mode == nfHP) return (float)(v0 - k * v1 - v2);
        return (float)v2;
    }
    double glideCoef(float sec) const { return 1.0 - std::exp(-1.0 / (sec * 0.5 * srate)); }
    float noise() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 8388608.f - 1.f; }

    void oscillate(Voice& v, const NeoPatch& P, double f, float& l, float& r) {
        const double dt = f / srate;
        switch (P.osc) {
        case noSaw:
        case noSquare: {
            const int n = P.uni;
            for (int k = 0; k < n; ++k) {
                const double det = n == 1 ? 1.0 : std::pow(2.0, ((k - (n - 1) * 0.5) / ((n - 1) * 0.5)) * P.detune / 1200.0);
                const double d = dt * det;
                v.ph[k] += d; if (v.ph[k] >= 1) v.ph[k] -= 1;
                double o;
                if (P.osc == noSaw) o = (2 * v.ph[k] - 1) - blep(v.ph[k], d);
                else { o = v.ph[k] < P.pw ? 1.0 : -1.0; o += blep(v.ph[k], d); double q = v.ph[k] + 1.0 - P.pw; q -= std::floor(q); o -= blep(q, d); }
                if (P.osc2Lvl > 0) {
                    const double d2 = d * std::pow(2.0, P.osc2Semi / 12.0);
                    v.ph2[k] += d2; if (v.ph2[k] >= 1) v.ph2[k] -= 1;
                    o += ((2 * v.ph2[k] - 1) - blep(v.ph2[k], d2)) * P.osc2Lvl;
                }
                const float pn = v.pan[k];
                l += (float)o * (1.f - pn); r += (float)o * (1.f + pn);
            }
            const float nrm = 0.55f / std::sqrt((float)n);
            l *= nrm; r *= nrm;
            break;
        }
        case noSine: {
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            const float o = (float)std::sin(6.2831853 * v.ph[0]) * 0.8f;
            l = r = o;
            break;
        }
        case noFM: {
            const int n = std::max(1, std::min(2, P.uni));
            const double mi = P.fmIndex * std::exp(-v.t / P.fmDecay) * (0.5 + 0.5 * v.vel);
            const double mi2 = P.fm2Index * std::exp(-v.t / P.fm2Decay) * v.vel;
            for (int k = 0; k < n; ++k) {
                const double det = n == 1 ? 1.0 : std::pow(2.0, (k ? 1 : -1) * P.detune / 1200.0);
                v.ph[k] += dt * det; if (v.ph[k] >= 1) v.ph[k] -= 1;
                v.ph2[k] += dt * det * P.fmRatio; if (v.ph2[k] >= 1) v.ph2[k] -= 1;
                v.ph2[k + 4] += dt * det * P.fm2Ratio; if (v.ph2[k + 4] >= 1) v.ph2[k + 4] -= 1;
                const double mod = std::sin(6.2831853 * v.ph2[k]) * mi + std::sin(6.2831853 * v.ph2[k + 4]) * mi2;
                const float o = (float)std::sin(6.2831853 * v.ph[k] + mod) * 0.6f;
                const float pn = n == 1 ? 0.f : (k ? 0.5f : -0.5f);
                l += o * (1.f - pn); r += o * (1.f + pn);
            }
            if (n == 2) { l *= 0.7f; r *= 0.7f; }
            break;
        }
        case noSample: {
            if (!v.z) { l = r = 0; break; }
            const Zone& z = *v.z;
            if (v.spos >= z.len - 2) { l = r = 0; break; }
            const uint32_t i = (uint32_t)v.spos; const float fr = (float)(v.spos - i);
            const float o = z.data[i] * (1 - fr) + z.data[i + 1] * fr;
            v.spos += v.sinc * (f / v.freq);
            if (z.loopEnd > z.loopStart && v.spos >= z.loopEnd) v.spos -= (z.loopEnd - z.loopStart);
            l = r = o * z.gain;
            break;
        }
        case noVox: {
            // saw through two formant resonators (vowel per note, or a fixed instrument body)
            v.ph[0] += dt; if (v.ph[0] >= 1) v.ph[0] -= 1;
            const double saw = (2 * v.ph[0] - 1) - blep(v.ph[0], dt) + noise() * P.noise * 6.0;
            static const float F[6][2] = { {800, 1200}, {420, 2000}, {300, 2500}, {470, 860}, {330, 760}, {1050, 2650} };
            const int vw = P.formant >= 0 ? P.formant : v.vowel;
            float o = 0;
            for (int k = 0; k < 2; ++k) {
                const double fc = std::min((double)F[vw][k], srate * 0.4);
                const double g = std::tan(3.14159265 * fc / srate), kk = k ? 0.22 : 0.16;
                const double a1 = 1.0 / (1.0 + g * (g + kk)), a2 = g * a1, a3 = g * a2;
                o += svf(v.fm[k][0], v.fm[k][1], (float)saw, a1, a2, a3, kk, nfBP) * (k ? 0.55f : 1.f);
            }
            if (P.formant == 5) o = o * 0.7f + (float)saw * 0.12f;    // erhu: keep some raw bow
            l = r = o * 0.5f;
            break;
        }
        case noPluck: {
            float* buf = &ks[(&v - voices) * KS];
            const int N = (int)v.ksLen;
            // sawari: the bridge buzz shortens the string a little when it swings wide
            const double fracLen = v.ksLen - N;
            const int i0 = v.ksPos, i1 = (i0 + 1) % N;
            const float y = buf[i0] * (float)(1 - fracLen) + buf[i1] * (float)fracLen;
            float nv = (y + v.ksPrev) * 0.5f * (v.released ? 0.9f : 0.9965f);
            if (std::fabs(nv) > 0.18f) nv = nv * 0.97f + (nv > 0 ? 0.0045f : -0.0045f);
            v.ksPrev = y; buf[i0] = nv;
            v.ksPos = (v.ksPos + 1) % N;
            // body: two resonances of the skin-covered drum body
            v.body[0] += (y - v.body[0]) * 0.18f;
            v.body[1] += (v.body[0] - v.body[1]) * 0.18f;
            const float o = y * (1.f - P.bodyMix * 0.5f) + (v.body[0] - v.body[1]) * P.bodyMix * 2.2f;
            l = r = o * 1.1f;
            break;
        }
        case noCowbell: {
            for (int k = 0; k < 2; ++k) {
                const double d = dt * (k ? 1.48 : 1.0);
                v.cbPh[k] += d; if (v.cbPh[k] >= 1) v.cbPh[k] -= 1;
                double o = v.cbPh[k] < 0.5 ? 1.0 : -1.0; o += blep(v.cbPh[k], d); double q = v.cbPh[k] + 0.5; q -= std::floor(q); o -= blep(q, d);
                l += (float)o * 0.5f;
            }
            r = l;
            break;
        }
        }
        if (P.sub > 0) {
            v.subPh += dt * 0.5; if (v.subPh >= 1) v.subPh -= 1;
            const float s = (float)std::sin(6.2831853 * v.subPh) * P.sub;
            l += s; r += s;
        }
        if (P.noise > 0 && P.osc != noVox) { const float nz = noise() * P.noise; l += nz; r += nz; }
    }

    Voice& alloc() {
        for (auto& v : voices) if (!v.active) return v;
        Voice* best = &voices[0]; uint32_t ba = 0xFFFFFFFFu;
        for (auto& v : voices) if (v.age < ba) { ba = v.age; best = &v; }
        return *best;
    }
    void start(Voice& v, int pi, int note, int vel, double f, float gainMul) {
        const NeoPatch& P = neoPatches()[pi];
        v.active = true; v.released = false; v.held = false; v.mono = false;
        v.note = note; v.patch = pi; v.age = ++serial;
        v.freq = v.target = f; v.t = 0; v.vph = 0; v.crushCnt = 0; v.chL = v.chR = 0;
        v.vel = vel / 127.f;
        const float x = v.vel;
        v.amp = ((1 - velSens) + velSens * (0.25f + 0.75f * x * x)) * P.level * gainMul;
        v.env = 0; v.stage = 1;
        v.atk = P.a <= 0.0005f ? 1.0 : 1.0 / (P.a * srate);
        v.dec = std::exp(-1.0 / (std::max(0.01f, P.d) * srate));
        v.rel = std::exp(-1.0 / (std::max(0.01f, P.r * relMul) * srate));
        v.fenv = 0; v.fstage = 1;
        v.fatk = P.fA <= 0.0005f ? 1.0 : 1.0 / (P.fA * srate);
        v.fdec = std::exp(-1.0 / (std::max(0.005f, P.fD) * srate));
        std::memset(v.fs, 0, sizeof(v.fs)); std::memset(v.fm, 0, sizeof(v.fm));
        for (int k = 0; k < 8; ++k) {
            rng = rng * 1664525u + 1013904223u; v.ph[k] = (rng >> 8) / 16777216.0;
            rng = rng * 1664525u + 1013904223u; v.ph2[k] = (rng >> 8) / 16777216.0;
            v.pan[k] = P.uni <= 1 ? 0.f : P.spread * ((k - (P.uni - 1) * 0.5f) / ((P.uni - 1) * 0.5f)) * ((k & 1) ? -1.f : 1.f);
        }
        if (P.osc == noFM || P.osc == noSine) { for (int k = 0; k < 8; ++k) { v.ph[k] = 0; v.ph2[k] = 0; } }
        v.subPh = 0; v.cbPh[0] = v.cbPh[1] = 0;
        v.vowel = (vowelCounter++) % 5;
        v.z = nullptr;
        if (P.osc == noSample && P.sample) {
            const int set = bank.find(P.sample);
            if (set >= 0) {
                const double n = 69 + 12 * std::log2(f / 440.0);
                double bd = 1e9;
                for (const Zone& z : bank.set(set)) { const double d = std::fabs(z.root - n); if (d < bd) { bd = d; v.z = &z; } }
                if (v.z) { v.spos = 0; v.sinc = std::pow(2.0, (n - v.z->root) / 12.0) * v.z->rate / srate; }
            }
        }
        if (P.osc == noPluck) {
            float* buf = &ks[(&v - voices) * KS];
            v.ksLen = std::min((double)KS - 2, srate / f);
            const int N = (int)v.ksLen + 2;
            float hp = 0, prev = 0;
            for (int i = 0; i < N; ++i) {
                // hard plectrum (bachi): bright, almost white excitation, plus a sharp click
                const float nz = noise();
                hp = nz - prev; prev = nz;
                buf[i] = (nz * 0.55f + hp * 0.45f) * (0.6f + 0.4f * x);
            }
            v.ksPos = 0; v.ksPrev = 0; v.body[0] = v.body[1] = 0;
        }
    }
    void release(Voice& v) { v.released = true; v.held = false; v.stage = 3; }
    void pushStack(int n) { popStack(n); if (nstack < 16) stack[nstack++] = n; }
    void popStack(int n) { int j = 0; for (int i = 0; i < nstack; ++i) if (stack[i] != n) stack[j++] = stack[i]; nstack = j; }

    const SampleBank& bank;
    Voice voices[NV];
    std::vector<float> ks;
    double srate = 44100, lastMonoFreq = 0;
    int lastMonoPatch = -1;
    float relMul = 1, bright = 1, velSens = 1;
    int stack[16]; int nstack = 0, vowelCounter = 0;
    uint32_t serial = 0, rng = 0x13579bdfu;
};

} // namespace retrofm
