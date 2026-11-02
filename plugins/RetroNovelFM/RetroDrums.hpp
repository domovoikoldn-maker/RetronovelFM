// RetroDrums.hpp - synthesized drum kit + step sequencer synced to host tempo
#pragma once
#include <cmath>
#include <cstdint>
#include "RetroData.hpp"
#include "RetroSampler.hpp"

namespace retrofm {

class DrumSynth {
public:
    void setSampleRate(double sr) { srate = sr; for (auto& v : v_) v.active = false; }
    void setKit(int k) { kit = k < 0 ? 0 : (k > kNumKits - 1 ? kNumKits - 1 : k); }

    void trigger(int lane, float vel) {
        if (lane < 0 || lane >= kDrumLanes || vel <= 0) return;
        if (lane == dCHat) v_[dOHat].active = false;   // closed hat chokes open hat
        Voice& v = v_[lane];
        v.active = true; v.t = 0; v.vel = vel; v.ph = 0; v.ph2 = 0;
        v.lp = v.bp = 0; v.hpz = 0; v.spos = 0; v.zd = nullptr; v.zl = 0; v.zg = 1.f;
        // real kits (and the Eurobeat kit's cymbals / snare layer) play recorded hits
        const int slot = kitSampleSlot(kit);
        const bool euroSynth = kit == 7 && (lane == dKick || lane == dClap || lane == dTomL || lane == dTomH || lane == dRim);
        if (slot >= 0 && !euroSynth) {
            const int layer = vel >= 0.6f ? 1 : 0;
            const int rr = (rrCount[lane]++) & 1;
            const SampleBank& B = SampleBank::get();
            v.zd = B.kitData(slot, lane, layer, rr, v.zl);
            // keep dynamics inside a layer: hard layer covers 0.6..1, soft layer 0..0.6
            v.zg = layer ? (0.75f + 0.25f * (vel - 0.6f) / 0.4f) : (0.45f + 0.55f * vel / 0.6f);
            if (kit == 7) v.zg *= lane == dSnare ? 0.8f : 0.9f;
        }
        pulse = 1.0f;
    }

    // returns mono drum sample
    float tick() {
        double out = 0;
        const double dt = 1.0 / srate;
        for (int lane = 0; lane < kDrumLanes; ++lane) {
            Voice& v = v_[lane];
            if (!v.active) continue;
            const double t = v.t;
            double s = 0, amp = 1;
            if (v.zd) {      // recorded hit (real kits)
                const uint32_t i = (uint32_t)v.spos;
                if (i + 1 >= v.zl) { v.zd = nullptr; if (!(kit == 7 && lane == dSnare)) { v.active = false; continue; } }
                else {
                    const float f = (float)(v.spos - i);
                    const double smp = (v.zd[i] * (1 - f) + v.zd[i + 1] * f) * v.zg * kitGain(kit, lane);
                    v.spos += 32000.0 / srate;
                    if (!(kit == 7 && lane == dSnare)) { out += smp; v.t += dt; continue; }
                    out += smp;   // Eurobeat snare: recorded snare + synthetic body below
                }
            }
            if (kit == 3) {   // orchestral kit: CC0 recordings
                const Zone& z = SampleBank::get().drum(lane);
                const uint32_t i = (uint32_t)v.spos;
                if (!z.data || i + 1 >= z.len) { v.active = false; continue; }
                const float f = (float)(v.spos - i);
                s = (z.data[i] * (1 - f) + z.data[i + 1] * f) * liveGain(lane);
                v.spos += z.rate / srate;
                out += s * v.vel;
                v.t += dt;
                continue;
            }
            switch (lane) {
            case dKick: {
                const double f0 = kit == 1 ? 120 : (kit == 7 ? 240 : 170), f1 = kit == 1 ? 44 : (kit == 7 ? 50 : 52);
                const double pt = kit == 1 ? 0.045 : (kit == 7 ? 0.028 : 0.03), at = kit == 1 ? 0.55 : (kit == 7 ? 0.42 : 0.22);
                const double f = f1 + (f0 - f1) * std::exp(-t / pt);
                v.ph += f * dt; if (v.ph >= 1) v.ph -= 1;
                amp = std::exp(-t / at);
                s = std::sin(6.283185307 * v.ph);
                if (kit != 1) s += (kit == 7 ? 1.3 : 0.6) * std::exp(-t / 0.003) * noise();   // click
                s = std::tanh(s * (kit == 2 ? 2.5 : (kit == 7 ? 2.2 : 1.4)));
                if (amp < 0.0005) v.active = false;
                break; }
            case dSnare: {
                const double ft = kit == 1 ? 180 : 210;
                v.ph += ft * dt; if (v.ph >= 1) v.ph -= 1;
                const double tone = std::sin(6.283185307 * v.ph) * std::exp(-t / 0.06);
                const double n = hp(v, noise(), 0.3) * std::exp(-t / (kit == 1 ? 0.18 : (kit == 7 ? 0.16 : 0.13))) * (kit == 7 ? 0.55 : 1.0);
                s = tone * 0.6 + n * 0.9;
                if (t > 0.9) v.active = false;
                break; }
            case dCHat:
            case dOHat: {
                double n = metal(v) * 0.5 + noise() * 0.5;
                n = hp(v, n, 0.6);
                amp = std::exp(-t / (lane == dCHat ? (kit == 1 ? 0.03 : 0.045) : 0.32));
                s = n * 0.75;
                if (amp < 0.0005) v.active = false;
                break; }
            case dClap: {
                const double n = bp(v, noise(), 1200, 0.5);
                double e = 0;
                for (int b = 0; b < 3; ++b) { const double tb = t - b * 0.011; if (tb >= 0 && tb < 0.011) e = std::fmax(e, std::exp(-tb / 0.004)); }
                if (t >= 0.022) e = std::fmax(e, std::exp(-(t - 0.022) / 0.11));
                s = n * e * 2.2; amp = 1;
                if (t > 0.8) v.active = false;
                break; }
            case dTomL:
            case dTomH: {
                const double f1 = lane == dTomL ? 105 : 165, f = f1 * (1 + 0.6 * std::exp(-t / 0.05));
                v.ph += f * dt; if (v.ph >= 1) v.ph -= 1;
                amp = std::exp(-t / 0.28);
                s = std::sin(6.283185307 * v.ph) + 0.15 * noise() * std::exp(-t / 0.01);
                if (amp < 0.0005) v.active = false;
                break; }
            case dRim: {
                v.ph += 1650 * dt; if (v.ph >= 1) v.ph -= 1;
                v.ph2 += 420 * dt; if (v.ph2 >= 1) v.ph2 -= 1;
                amp = std::exp(-t / 0.018);
                s = (std::sin(6.283185307 * v.ph) * 0.6 + std::sin(6.283185307 * v.ph2) * 0.6);
                if (amp < 0.0005) v.active = false;
                break; }
            case dCrash: {
                double n = metal(v) * 0.6 + noise() * 0.6;
                n = hp(v, n, 0.45);
                amp = std::exp(-t / 1.1) * (1 - std::exp(-t / 0.002));
                s = n * 0.55;
                if (t > 4) v.active = false;
                break; }
            }
            out += s * amp * v.vel * laneGain(lane);
            v.t += dt;
        }
        // 8-bit kit: crush
        static const double kitMaster[kNumKits] = { 1.0, 0.65, 0.6, 1.15, 1.75, 1.45, 2.1, 0.62, 2.0 };
        out *= kitMaster[kit];
        if (kit == 4) out = std::tanh(out * 1.35) * 0.9;    // rock: glue
        if (kit == 7) out = std::tanh(out * 1.25) * 0.92;   // eurobeat: punchy bus
        if (kit == 8) {                                     // lo-fi: dull and slightly crushed
            lofiLp += (out - lofiLp) * 0.28; out = lofiLp;
            if (++hc >= 2) { hc = 0; held = std::floor(out * 96 + 0.5) / 96; }
            out = held;
        }
        if (kit == 2) {
            if (++hc >= 3) { hc = 0; held = std::floor(out * 24 + 0.5) / 24; }
            out = held;
        }
        pulse *= 0.9997f;
        return (float)out;
    }

    float pulse = 0;

private:
    static float liveGain(int l) { static const float g[kDrumLanes] = { 1.0f, 0.75f, 0.45f, 0.4f, 0.55f, 0.7f, 0.65f, 0.5f, 0.4f }; return g[l]; }
    static double kitGain(int kit, int lane) {
        // per kit balance for recorded kits (kick, snare, chat, ohat, clap, toml, tomh, rim, crash)
        static const double g[kNumKits][kDrumLanes] = {
            {1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1},
            {1.10, 0.85, 0.40, 0.42, 0.80, 0.75, 0.70, 0.55, 0.55},   // rock
            {1.05, 0.80, 0.45, 0.42, 0.75, 0.70, 0.65, 0.60, 0.50},   // pop
            {0.90, 0.85, 0.55, 0.50, 0.70, 0.70, 0.65, 0.60, 0.55},   // brush
            {1.00, 0.70, 0.45, 0.45, 1.00, 1.00, 1.00, 1.00, 0.50},   // eurobeat (cymbals / snare layer)
            {1.10, 0.85, 0.50, 0.45, 0.80, 0.75, 0.70, 0.60, 0.50},   // lo-fi
        };
        return g[kit][lane];
    }
    double lofiLp = 0;
    int rrCount[kDrumLanes] = {0,0,0,0,0,0,0,0,0};
    struct Voice { const float* zd = nullptr; uint32_t zl = 0; float zg = 1.f; double spos = 0; bool active = false; double t = 0, ph = 0, ph2 = 0, lp = 0, bp = 0, hpz = 0, mph[6] = {0,0,0,0,0,0}; float vel = 0; };

    static double laneGain(int lane) {
        static const double g[kDrumLanes] = { 0.95, 0.7, 1.1, 1.0, 0.38, 0.6, 0.55, 0.45, 0.85 };
        return g[lane];
    }
    double noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (rng / 4294967295.0) * 2 - 1; }
    // one-pole high-pass, c = lowpass coefficient (bigger = higher cutoff)
    static double hp(Voice& v, double x, double c) { v.lp += c * (x - v.lp); return x - v.lp; }
    // state-variable band-pass
    double bp(Voice& v, double x, double fc, double q) {
        const double f = 2 * std::sin(3.14159265 * fc / srate);
        const double hpv = x - v.lp - q * v.bp;
        v.bp += f * hpv; v.lp += f * v.bp;
        return v.bp;
    }
    // 808-style metallic square cluster
    double metal(Voice& v) {
        static const double fr[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
        double s = 0;
        for (int i = 0; i < 6; ++i) { v.mph[i] += fr[i] * 1.7 / srate; if (v.mph[i] >= 1) v.mph[i] -= 1; s += v.mph[i] < 0.5 ? 1 : -1; }
        return s / 6.0;
    }

    double srate = 44100;
    int kit = 0, hc = 0;
    double held = 0;
    uint32_t rng = 0x12345678u;
    Voice v_[kDrumLanes];
};

// Step sequencer: 16th-note steps, follows host position (or free-runs)
class DrumSequencer {
public:
    void setSampleRate(double sr) { srate = sr; resync = true; }
    void setPattern(int p) {
        p = p < 0 ? 0 : (p > kNumPatterns - 1 ? kNumPatterns - 1 : p);
        if (p != pat) { pat = p; resync = true; }
    }
    void setTable(const DrumPattern* t) { table = t; }

    // called once per block before ticking
    // mode: 0 off, 1 sync to host, 2 always
    void transport(int mode_, bool playing, bool posValid, double posSteps, double bpm) {
        mode = mode_;
        stepsPerSample = (bpm > 0 ? bpm : 120.0) / 60.0 * 4.0 / srate;
        if (mode == 0) { running = false; return; }
        if (playing && posValid) {
            if (!running || resync || std::fabs(posSteps - pos) > 0.05) { pos = posSteps; resyncNext(); }
            running = true; free = false;
        } else if (mode == 2) {
            if (!running || !free) { pos = 0; resyncNext(); free = true; }
            running = true;
        } else {
            running = false;
        }
    }

    // advance one sample; fires hits into the drum synth
    void tick(DrumSynth& d, bool fills) {
        if (!running) return;
        pos += stepsPerSample;
        const DrumPattern& P = table[pat];
        if (resync) resyncNext();
        while (trigTime(next, P) <= pos) {
            fire(d, next, P, fills);
            ++next;
        }
    }

    int currentStep() const { return running ? lastStep : -1; }

private:
    // a step is a 16th note, or an 8th-note triplet for shuffle / jazz patterns (pos is in 16ths)
    static double stepLen(const DrumPattern& P) { return P.triplet ? 4.0 / 3.0 : 1.0; }
    static double trigTime(int64_t s, const DrumPattern& P) { return ((double)s + ((s & 1) ? P.swing : 0.0)) * stepLen(P); }
    void resyncNext() {
        const DrumPattern& P = table[pat];
        next = (int64_t)std::ceil(pos / stepLen(P) - 0.5);
        if (next < 0) next = 0;
        while (trigTime(next, P) < pos - 1e-9) ++next;
        // keep the bar aligned: step numbers count from the start of the song in this grid
        resync = false;
    }
    void fire(DrumSynth& d, int64_t s, const DrumPattern& P, bool fills) {
        const int len = P.len < 1 ? 16 : P.len;
        const int64_t bar = s / len;
        const int st = (int)(s % len);
        lastStep = st;
        const bool fillBar = fills && (bar % 4 == 3) && st >= len - 4;
        for (int lane = 0; lane < kDrumLanes; ++lane) {
            if (fillBar && lane != dKick && lane != dCrash) continue;
            const float v = stepVel(P.lane[lane][st]);
            if (v > 0) d.trigger(lane, v);
        }
        if (fillBar) {
            const int k = st - (len - 4);
            static const int fl[4] = { dSnare, dSnare, dTomH, dTomL };
            static const float fv[4] = { 1.0f, 0.75f, 1.0f, 1.0f };
            d.trigger(fl[k], fv[k]);
            if (k == 1) d.trigger(dTomH, 0.5f);
        }
        if (fills && st == 0 && bar > 0 && bar % 4 == 0) d.trigger(dCrash, 0.9f);
    }

    double srate = 44100, pos = 0, stepsPerSample = 0;
    int64_t next = 0;
    const DrumPattern* table = kPatterns;
    int pat = 0, mode = 0, lastStep = -1;
    bool running = false, free = false, resync = true;
};

} // namespace retrofm
