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
#include "RetroNeo.hpp"
#include "RetroBridge.hpp"
#include <atomic>
#include <cstdio>
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

// ------------------------------------------------------------------ MIDI recorder (what was played, for "Export MIDI")
class Recorder {
public:
    static constexpr int CAP = 8192;
    void clear() { gen.fetch_add(1); count.store(0, std::memory_order_release); }
    void add(const RecEvent& e) {
        if (paused.load(std::memory_order_acquire)) return;
        const int c = count.load(std::memory_order_relaxed);
        if (c >= CAP) return;
        ev[c] = e;
        count.store(c + 1, std::memory_order_release);
    }
    int size() const { return count.load(std::memory_order_acquire); }
    // consistent copy for another thread
    void snapshot(std::vector<RecEvent>& out) const {
        for (int tries = 0; tries < 4; ++tries) {
            const uint32_t g = gen.load();
            const int n = count.load(std::memory_order_acquire);
            out.assign(ev, ev + n);
            if (gen.load() == g) return;
        }
    }
    std::string serialize(double bpm) const {
        std::vector<RecEvent> v; snapshot(v);
        std::string s;
        char b[96];
        std::snprintf(b, sizeof(b), "take1;%.3f;", bpm); s += b;
        for (const RecEvent& e : v) { std::snprintf(b, sizeof(b), "%.4f,%d,%d,%d,%d,%d;", e.t, e.on, e.note, e.vel, e.vocal, e.syl); s += b; }
        return s;
    }
    void deserialize(const char* str) {
        paused.store(true);
        count.store(0); gen.fetch_add(1);
        if (str && !std::strncmp(str, "take1;", 6)) {
            const char* c = std::strchr(str + 6, ';');
            int n = 0;
            while (c && *c && n < CAP) {
                ++c;
                RecEvent e; int on, note, vel, vo, sy;
                if (std::sscanf(c, "%lf,%d,%d,%d,%d,%d", &e.t, &on, &note, &vel, &vo, &sy) != 6) break;
                e.on = (uint8_t)on; e.note = (uint8_t)note; e.vel = (uint8_t)vel; e.vocal = (uint8_t)vo; e.syl = (int16_t)sy;
                ev[n++] = e;
                c = std::strchr(c, ';');
            }
            count.store(n, std::memory_order_release);
        }
        paused.store(false);
    }
private:
    RecEvent ev[CAP];
    std::atomic<int> count { 0 };
    std::atomic<uint32_t> gen { 0 };
    std::atomic<bool> paused { false };
};

// ------------------------------------------------------------------ generated song ("Random Song")
// text: "song1;<loop steps>;<backing inst>;<bass inst>;t,dur,note,vel,track;..."   (times in 16ths)
struct SongEvent { int t, dur, note, vel, track; };
struct SongData {
    int loop = 0, back = -1, bass = -1;
    std::vector<SongEvent> ev;
    std::string text;
    bool parse(const char* str) {
        ev.clear(); loop = 0; back = bass = -1; text = str ? str : "";
        if (!str || std::strncmp(str, "song1;", 6)) return false;
        if (std::sscanf(str + 6, "%d;%d;%d;", &loop, &back, &bass) != 3) return false;
        const char* c = str + 6;
        for (int k = 0; k < 3 && c; ++k) { c = std::strchr(c, ';'); if (c) ++c; }
        while (c && *c) {
            SongEvent e;
            if (std::sscanf(c, "%d,%d,%d,%d,%d", &e.t, &e.dur, &e.note, &e.vel, &e.track) != 5) break;
            if (e.t >= 0 && e.t < loop && e.dur > 0 && e.note >= 0 && e.note < 128) ev.push_back(e);
            c = std::strchr(c, ';'); if (c) ++c;
        }
        return loop > 0 && !ev.empty();
    }
};

class Engine {
public:
    static constexpr int NV = 16;
    static constexpr int CHN = 16384;
    static constexpr int RB = 8192;

    Engine() {
        for (int i = 0; i < kParamCount; ++i) params[i] = kParams[i].def;
        for (int i = 0; i < kNumPatterns; ++i) if (!isUserPattern(i)) pats[i] = kPatterns[presetSlot(i)];
        static const int seed[4] = { 0, 5, 7, 11 };
        for (int u = 0; u < 4; ++u) setUserPattern(u, patternToString(kPatterns[seed[u]]).c_str());
        for (int k = 0; k < 128; ++k) chordN[k] = 0;
        setSampleRate(44100.0);
    }

    // ---- user drum patterns: "lane|lane|...(9)|;len" with chars X x g -   (len 1..24)
    static std::string patternToString(const DrumPattern& P) {
        std::string s;
        for (int l = 0; l < kDrumLanes; ++l) { s += std::string(P.lane[l]).substr(0, P.len); s += '|'; }
        s += ';'; s += std::to_string(P.len);
        return s;
    }
    void setUserPattern(int u, const char* str) {
        if (u < 0 || u >= 4 || !str) return;
        char (*L)[kMaxSteps + 1] = userLanes[u];
        int len = 16;
        const char* semi = std::strchr(str, ';');
        if (semi) { len = std::atoi(semi + 1); if (len < 1 || len > kMaxSteps) len = 16; }
        int lane = 0, k = 0;
        for (int l = 0; l < kDrumLanes; ++l) { std::memset(L[l], '-', kMaxSteps); L[l][kMaxSteps] = 0; }
        for (const char* c = str; *c && c != semi && lane < kDrumLanes; ++c) {
            if (*c == '|') { ++lane; k = 0; continue; }
            if (k < kMaxSteps) L[lane][k++] = (*c == 'X' || *c == 'x' || *c == 'g') ? *c : '-';
        }
        DrumPattern& P = pats[kFirstUserPattern + u];
        P.name = kUserPatternNames[u]; P.len = len; P.swing = 0; P.triplet = 0;
        for (int l = 0; l < kDrumLanes; ++l) P.lane[l] = L[l];
        ++userVersion;
        rebuildPattern();
    }
    std::string getUserPattern(int u) const { return patternToString(pats[kFirstUserPattern + u]); }

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

    // ---- generated song (non-audio thread): double buffered
    void setSong(const char* text) {
        const int w = songIdx.load() ^ 1;
        songBuf[w].parse(text);
        songIdx.store(w);
        songResync = true;
    }
    const std::string& getSong() const { return songBuf[songIdx.load()].text; }

    // ---- recorded take
    Recorder& recorder() { return rec; }
    double tempo() const { return lastBpm; }

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
        drums.setSampleRate(sr); seq.setSampleRate(sr); sampler.setSampleRate(sr); vocal.setSampleRate(sr); neo.setSampleRate(sr);
        update();
    }

    void setParam(int i, float v) {
        if (i < 0 || i >= kParamCount) return;
        const float old = params[i];
        params[i] = v;
        if (i == pSongPlay && (v > 0.5f) != (old > 0.5f)) songResync = true;
        update();
    }
    float getParam(int i) const { return (i >= 0 && i < kParamCount) ? params[i] : 0.f; }

    void midi(const uint8_t* d, uint32_t size) {
        if (size < 1) return;
        const uint8_t st = d[0] & 0xF0;
        const uint8_t m2 = size > 1 ? d[1] : 0, m3 = size > 2 ? d[2] : 0;
        if ((d[0] & 0x0F) == 9) {            // MIDI channel 10 = drums (GM map)
            if (st == 0x90 && m3 > 0) drums.trigger(gmLane(m2), m3 / 127.0f);
            return;
        }
        if (st == 0x90 && m3 > 0) keyOn(m2, m3);
        else if (st == 0x80 || (st == 0x90 && m3 == 0)) keyOff(m2);
        else if (st == 0xE0) { bend = ((m3 * 128 + m2) - 8192) / 8192.0; bendmul = std::pow(2.0, bend * params[pBendRange] / 12.0); }
        else if (st == 0xB0) {
            if (m2 == 1) modwheel = m3 / 127.0;
            else if (m2 == 64) { bool p = m3 >= 64; if (pedal && !p) pedalUpAll(); pedal = p; }
            else if (m2 == 120 || m2 == 123) { allNotesOff(); }
        }
    }

    // a key from the keyboard / DAW: scale lock and one-key chords happen here
    void keyOn(int key, int vel) {
        if (chordN[key] > 0) keyOff(key);
        const bool vox = params[pVocalMode] > 0.5f;
        const int k = (int)params[pKey], sc = (int)params[pScale], mode = (int)params[pChord];
        int base = key;
        if (params[pScaleLock] > 0.5f) base = snapToScale(key, k, sc);
        int out[5]; int n = 1; out[0] = base;
        if (mode > 0 && !vox) n = buildChord(base, k, sc, mode, out);
        chordN[key] = 0;
        for (int i = 0; i < n; ++i) {
            if (out[i] < 0 || out[i] > 127) continue;
            const int v = i == 0 ? vel : std::max(1, vel * 85 / 100);
            startNote(out[i], v, -1);
            chordNotes[key][chordN[key]++] = (uint8_t)out[i];
        }
    }
    void keyOff(int key) {
        for (int i = 0; i < chordN[key]; ++i) stopNote(chordNotes[key][i], true);
        chordN[key] = 0;
    }

    // track -1 = live key, 0 = song melody, 1 = song chords, 2 = song bass
    void startNote(int note, int vel, int track) {
        const int inst = (int)std::floor(params[pInstrument] + 0.5f);
        const int scene = (int)std::floor(params[pScene] + 0.5f);
        const bool vox = params[pVocalMode] > 0.5f;
        const SongData& S = songBuf[songIdx.load()];
        if (track == 1 && S.back >= 0) { playInst(S.back, note, vel, 0.7f); return; }
        if (track == 2 && S.bass >= 0) { playInst(S.bass, note, vel, 0.9f); return; }
        if (vox && track <= 0) {
            vocal.noteOn(note + (int)params[pTranspose], vel);
            if (track < 0) rec.add(RecEvent{clock, 1, (uint8_t)note, (uint8_t)vel, 1, (int16_t)vocal.currentSyllable()});
            return;
        }
        if (track < 0) rec.add(RecEvent{clock, 1, (uint8_t)note, (uint8_t)vel, 0, -1});
        if (scene > 0 && scene <= kNumScenes) {
            const ScenePreset& SP = kScenes[scene - 1];
            for (int k = 0; k < 3; ++k) {
                const ScenePart& P = SP.part[k];
                if (P.inst < 0 || note < P.lo || note > P.hi) continue;
                playInst(P.inst, note, vel, P.gain);
            }
        }
        else playInst(inst, note, vel, 1.f);
    }
    void stopNote(int note, bool record) {
        noteOff(note); sampler.noteOff(note, pedal); neo.noteOff(note, pedal, params[pTranspose]);
        vocal.noteOff(note + (int)params[pTranspose]); rhythmOff(note);
        if (record) rec.add(RecEvent{clock, 0, (uint8_t)note, 0, (uint8_t)(params[pVocalMode] > 0.5f ? 1 : 0), -1});
    }
    void allNotesOff() {
        allOff(); sampler.allOff(); vocal.allOff(); neo.allOff(); nr = 0;
        for (int k = 0; k < 128; ++k) chordN[k] = 0;
        nSongOn = 0;
    }

    // play one instrument (FM / sampled / rock / REIWA); rhythmic instruments are retriggered in tempo
    void playInst(int inst, int note, int vel, float gain) {
        if (inst < 0 || inst >= kNumInstruments) return;
        if (inst < kNumFM) { noteOn(note, vel, gain, inst); return; }
        if (isNeo(inst)) { neo.noteOn(inst, note, vel, params[pTranspose], gain); return; }
        if (inst >= kFirstRock && inst < kFirstXP2 && instDef(inst).rhythm > 0) {
            if (nr < 32) {
                bool first = true;
                for (int i = 0; i < nr; ++i) if (rn[i].inst == inst) first = false;
                rn[nr].inst = inst; rn[nr].note = note; rn[nr].vel = vel; rn[nr].gain = gain; ++nr;
                if (!instDef(inst).arp) sampler.noteOn(inst, note, vel, params[pTranspose], gain);
                else if (first) { arpIdx = 0; arpNote = note; sampler.noteOn(inst, note, vel, params[pTranspose], gain); }
            }
            return;
        }
        sampler.noteOn(inst, note, vel, params[pTranspose], gain);
    }
    void rhythmOff(int note) {
        int j = 0;
        for (int i = 0; i < nr; ++i) if (rn[i].note != note) rn[j++] = rn[i];
        nr = j;
        if (arpNote >= 0) { bool alive = false; for (int i = 0; i < nr; ++i) if (instDef(rn[i].inst).arp) alive = true; if (!alive) { sampler.noteOff(arpNote, false); arpNote = -1; } }
    }
    // called every sample: 16th-note clock from the host (or free-running)
    void rhythmTick() {
        if (nr == 0) return;
        rpos += rStepsPerSample;
        const int64_t step = (int64_t)std::floor(rpos);
        if (step == rLastStep) return;
        rLastStep = step;
        for (int i = 0; i < nr; ++i) {
            const RNote& R = rn[i];
            const XPInst& D = instDef(R.inst);
            if (D.arp) continue;
            if (D.rhythm == 8 && (step & 1)) continue;
            sampler.noteOff(R.note, false);
            sampler.noteOn(R.inst, R.note, R.vel, params[pTranspose], R.gain);
        }
        // arpeggio: walk up the held notes, then one octave higher
        int list[32], vels[32], cnt = 0, ai = -1; float g = 1.f;
        for (int i = 0; i < nr; ++i) if (instDef(rn[i].inst).arp) { list[cnt] = rn[i].note; vels[cnt++] = rn[i].vel; ai = rn[i].inst; g = rn[i].gain; }
        if (cnt > 0) {
            for (int a = 1; a < cnt; ++a) for (int b = a; b > 0 && list[b] < list[b - 1]; --b) { std::swap(list[b], list[b - 1]); std::swap(vels[b], vels[b - 1]); }
            const int total = cnt * 2;
            arpIdx = (arpIdx + 1) % total;
            const int n = list[arpIdx % cnt] + (arpIdx >= cnt ? 12 : 0);
            if (arpNote >= 0) sampler.noteOff(arpNote, false);
            arpNote = n;
            sampler.noteOn(ai, n, vels[arpIdx % cnt], params[pTranspose], g);
        }
    }

    // call once per audio block with host transport info
    void transport(bool playing, bool posValid, double posSteps, double bpm) {
        if (playing && !wasPlaying) { vocal.rewind(); rec.clear(); songResync = true; }
        wasPlaying = playing;
        lastBpm = bpm > 0 ? bpm : 120.0;
        seq.transport((int)params[pDrumMode], playing, posValid, posSteps, bpm);
        rStepsPerSample = lastBpm / 60.0 * 4.0 / srate;
        if (playing && posValid && std::fabs(posSteps - rpos) > 0.25) rpos = posSteps;
        hostRunning = playing && posValid;
        if (hostRunning) { if (std::fabs(posSteps - clock) > 0.02) { clock = posSteps; songResync = true; } }
        else if (seq.isRunning() && seq.isFree()) { if (std::fabs(seq.position() - clock) > 0.05) { clock = seq.position(); songResync = true; } }
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
        clock += rStepsPerSample;
        songTick();
        vph += vrateInc; if (vph >= 1) vph -= 1;
        const double pmul = bendmul * std::pow(2.0, std::sin(kTwoPi * vph) * (params[pVibDepth] + modwheel * 40) / 1200.0);
        double mix = 0, act = 0;
        rhythmTick();
        float sL = 0, sR = 0;
        act += sampler.tick(pmul, sL, sR) * 0.8;
        act += neo.tick(pmul, clock, sL, sR) * 0.8;
        const float vox = vocal.tick(bendmul);
        sL += vox * 0.75f; sR += vox * 0.75f;
        act += vocal.mouthOpen();
        for (auto& v : voices) {
            if (!v.active) continue;
            const PatchCache& pc = pcache[v.patch];
            const double base = v.inc * pmul;
            for (int k = 0; k < 4; ++k) { v.ph[k] += base * pc.ratio[k]; v.ph[k] -= std::floor(v.ph[k]); }
            double e[4];
            for (int k = 0; k < 4; ++k) e[k] = env(v, k);
            double a[4];
            for (int k = 0; k < 4; ++k) a[k] = e[k] * pc.el[k] * (pc.car[k] ? v.vc : v.vm);
            const double o1 = std::sin(kTwoPi * (v.ph[0] + pc.fb * (v.fb0 + v.fb1) * 0.5)) * a[0];
            v.fb1 = v.fb0; v.fb0 = o1;
            double o2, o3, o4, out = 0;
            auto S = [&](int k, double m) { return std::sin(kTwoPi * (v.ph[k] + m)) * a[k]; };
            switch (pc.alg) {
                case 0: o2 = S(1,o1); o3 = S(2,o2); o4 = S(3,o3); out = o4; break;
                case 1: o2 = S(1,0); o3 = S(2,o1+o2); o4 = S(3,o3); out = o4; break;
                case 2: o2 = S(1,0); o3 = S(2,o2); o4 = S(3,o1+o3); out = o4; break;
                case 3: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,o2+o3); out = o4; break;
                case 4: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,o3); out = o2+o4; break;
                case 5: o2 = S(1,o1); o3 = S(2,o1); o4 = S(3,o1); out = o2+o3+o4; break;
                case 6: o2 = S(1,o1); o3 = S(2,0); o4 = S(3,0); out = o2+o3+o4; break;
                default: o2 = S(1,0); o3 = S(2,0); o4 = S(3,0); out = o1+o2+o3+o4; break;
            }
            mix += out * pc.norm;
            act += e[3] * v.vc;
            double emax = e[3];
            for (int k = 0; k < 3; ++k) if (pc.car[k] && e[k] > emax) emax = e[k];
            if (v.stage[3] >= 2 && emax < 0.0001) v.active = false;
        }

        activity += ((act > 1 ? 1 : act) - activity) * 0.0005;
        double dry = mix * gain + (sL + sR) * 0.5 * gain * 1.6;
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
        int note = 0, stage[4] = {0,0,0,0}, patch = 0;
        double inc = 0, vc = 1, vm = 1, ph[4] = {0,0,0,0}, e[4] = {0,0,0,0};
        double atk[4] = {0,0,0,0}, dec[4] = {0,0,0,0}, rel[4] = {0,0,0,0};
        double fb0 = 0, fb1 = 0;
        uint32_t age = 0;
    };
    struct PatchCache { int alg = 4; double fb = 0, ratio[4] = {1,1,1,1}, el[4] = {0,0,0,0}, norm = 1; bool car[4] = {false,true,false,true}; };

    // ---- song player: fires the generated song in sync with the drum clock
    void songTick() {
        const SongData& S = songBuf[songIdx.load()];
        const bool on = params[pSongPlay] > 0.5f && S.loop > 0 && (hostRunning || seq.isRunning() || freeSong);
        if (!on) {
            if (nSongOn > 0) { for (int i = 0; i < nSongOn; ++i) songOff(songOn[i]); nSongOn = 0; }
            lastSongStep = INT64_MIN;
            return;
        }
        const int64_t step = (int64_t)std::floor(clock + 1e-9);
        if (songResync || lastSongStep == INT64_MIN || step < lastSongStep || step - lastSongStep > 4) {
            for (int i = 0; i < nSongOn; ++i) songOff(songOn[i]);
            nSongOn = 0; songResync = false;
            lastSongStep = step - 1;
        }
        while (lastSongStep < step) {
            ++lastSongStep;
            const int k = (int)(((lastSongStep % S.loop) + S.loop) % S.loop);
            // note-offs first (repeated notes)
            int j = 0;
            for (int i = 0; i < nSongOn; ++i) { if (--songOn[i].left <= 0) songOff(songOn[i]); else songOn[j++] = songOn[i]; }
            nSongOn = j;
            for (const SongEvent& e : S.ev) {
                if (e.t != k) continue;
                startNote(e.note, e.vel, e.track);
                if (nSongOn < 64) { songOn[nSongOn].note = e.note; songOn[nSongOn].track = e.track; songOn[nSongOn].left = e.dur; ++nSongOn; }
            }
        }
    }
    struct SongOn { int note, track, left; };
    void songOff(const SongOn& o) { stopNote(o.note, false); (void)o.track; }

    double env(Voice& v, int k) {
        const Patch& P = kPatches[v.patch];
        double e = v.e[k];
        if (v.stage[k] == 1) { e += v.atk[k]; if (e >= 1) { e = 1; v.stage[k] = 2; } }
        if (v.stage[k] == 2) { const double s = P.op[k].s; e = s + (e - s) * v.dec[k]; }
        if (v.stage[k] == 3) e *= v.rel[k];
        v.e[k] = e;
        return e;
    }

    void noteOn(int n, int vel, float gainMul, int patchIdx) {
        Voice* best = nullptr;
        for (auto& v : voices) if (!v.active) { best = &v; break; }
        if (!best) { uint32_t ba = 0xFFFFFFFFu; for (auto& v : voices) if (v.age < ba) { ba = v.age; best = &v; } }
        Voice& v = *best;
        const double nn = n + params[pTranspose];
        v.patch = patchIdx < 0 ? 0 : (patchIdx > 23 ? 23 : patchIdx);
        const Patch& P = kPatches[v.patch];
        v.active = true; v.released = false; v.held = false; v.note = n;
        v.inc = 440.0 * std::pow(2.0, (nn - 69) / 12.0) / srate;
        const double x = vel / 127.0, vs = params[pVelSens];
        v.vc = ((1 - vs) + vs * x * x) * gainMul;
        v.vm = (1 - vs * 0.6) + vs * 0.6 * x;
        v.age = ++serial; v.fb0 = v.fb1 = 0;
        double ks = std::pow(2.0, -(nn - 60) / 24.0); ks = ks < 0.3 ? 0.3 : (ks > 3 ? 3 : ks);
        for (int k = 0; k < 4; ++k) {
            const Op& o = P.op[k];
            v.ph[k] = 0; v.e[k] = 0; v.stage[k] = 1;
            v.atk[k] = o.a <= 0.001f ? 1.0 : 1.0 / (o.a * srate);
            v.dec[k] = std::exp(-1.0 / (std::fmax(o.d * ks, 0.002) * srate));
            v.rel[k] = std::exp(-1.0 / (std::fmax(o.r * params[pRelease], 0.003) * srate));
        }
    }
    void release(Voice& v) { v.released = true; v.held = false; for (int k = 0; k < 4; ++k) v.stage[k] = 3; }
    void noteOff(int n) { for (auto& v : voices) if (v.active && v.note == n && !v.released) { if (pedal) v.held = true; else release(v); } }
    void pedalUpAll() { pedalUp(); sampler.pedalUp(); neo.pedalUp(); }
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

    void rebuildPattern() {
        const int pat = std::max(0, std::min(kNumPatterns - 1, (int)std::floor(params[pDrumPattern] + 0.5f)));
        const int meter = std::max(0, std::min(kNumMeters - 1, (int)std::floor(params[pMeter] + 0.5f)));
        if (pat == effPat && meter == effMeter && userVersion == effUser) return;
        effPat = pat; effMeter = meter; effUser = userVersion;
        const int w = (effIdx + 1) % 3;
        const DrumPattern& P = pats[pat];
        effectivePattern(P.lane, P.len, P.triplet, P.swing, meter, effBuf[w]);
        effIdx = w;
        seq.setPattern(&effBuf[w]);
    }

    void update() {
        for (int pn = 0; pn < kNumFM; ++pn) {
            const Patch& P = kPatches[pn];
            PatchCache& c = pcache[pn];
            c.alg = P.alg; c.fb = P.fb;
            c.car[0] = c.alg == 7; c.car[1] = c.alg >= 4; c.car[2] = c.alg >= 5; c.car[3] = true;
            for (int k = 0; k < 4; ++k) { c.ratio[k] = P.op[k].ratio; c.el[k] = P.op[k].level * (c.car[k] ? 1.0 : params[pBrightness]); }
            c.norm = 1.0 / std::sqrt((double)(c.car[0] + c.car[1] + c.car[2] + c.car[3]));
        }
        gain = std::pow(10.0, params[pVolume] / 20.0);
        lofi = params[pLofi]; hold = 1 + (int)std::floor(lofi * 7); q = std::pow(2.0, 15 - lofi * 10);
        vrateInc = params[pVibRate] / srate;
        bendmul = std::pow(2.0, bend * params[pBendRange] / 12.0);
        rebuildPattern();
        seq.setGroove(params[pSwing], params[pHumanize]);
        sampler.setRelease(params[pRelease]);
        neo.setRelease(params[pRelease]);
        neo.setBrightness(params[pBrightness]);
        neo.setVelSens(params[pVelSens]);
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
        // the song preview runs on its own clock when neither the DAW nor the drums are running
        freeSong = true;
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
    PatchCache pcache[kNumFM];
    double gain = 1;
    double lofi = 0, q = 32768, held = 0; int hold = 1, hcnt = 0;
    double vph = 0, vrateInc = 0, cph = 0, bend = 0, bendmul = 1, modwheel = 0;
    bool pedal = false; uint32_t serial = 0;
    Sampler sampler;
    NeoSynth neo;
    struct RNote { int inst, note, vel; float gain; };
    RNote rn[32]; int nr = 0, arpIdx = 0, arpNote = -1;
    double rpos = 0, rStepsPerSample = 0; int64_t rLastStep = -1;
    VocalSynth vocal;
    bool wasPlaying = false, hostRunning = false, freeSong = true;
    DrumPattern pats[kNumPatterns];
    char userLanes[4][kDrumLanes][kMaxSteps + 1];
    int userVersion = 0;
    EffPattern effBuf[3]; int effIdx = 0, effPat = -1, effMeter = -1, effUser = -1;
    std::string userPath[kNumUser];
    int userRootNote[kNumUser] = {60, 60, 60, 60};
    std::vector<std::unique_ptr<UserSample>> loaded;
    DrumSynth drums; DrumSequencer seq; double drumGain = 0.5, activity = 0;
    // harmony helper: which notes each key started
    uint8_t chordNotes[128][5]; int chordN[128];
    // song + recorder
    SongData songBuf[2]; std::atomic<int> songIdx { 0 }; bool songResync = true;
    SongOn songOn[64]; int nSongOn = 0; int64_t lastSongStep = INT64_MIN;
    double clock = 0, lastBpm = 120;
    Recorder rec;
    Voice voices[NV];
    float chb[CHN]; int cw = 0;
    float rvBuf[12][RB]; int rvLen[12], rvPos[12]; double rvSt[12];
};

} // namespace retrofm
