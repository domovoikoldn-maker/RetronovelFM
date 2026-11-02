// RetroTools.hpp - UI-side tools: .rnfm presets (JSON), preset packs, Standard MIDI Files, Random Song
//
// Why JSON for .rnfm: it is plain UTF-8 text, so presets can be read, diffed, shared in chats and fixed by
// hand; unknown keys are ignored and missing keys keep their defaults, so presets stay compatible when
// new knobs are added in later versions; a whole pack is just a list of presets. Presets are tiny
// (a few KB), so a binary format would save nothing worth the loss of readability.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include "RetroData.hpp"
#include "RetroBridge.hpp"

namespace retrofm {

FILE* retroFopenUtf8(const char* path, const char* mode);   // RetroWinKeys.cpp

// ====================================================================== tiny JSON
struct JVal {
    enum Type { Null, Bool, Num, Str, Arr, Obj } t = Null;
    double n = 0; bool b = false; std::string s;
    std::vector<JVal> a;
    std::vector<std::pair<std::string, JVal>> o;
    const JVal* get(const char* k) const { for (auto& kv : o) if (kv.first == k) return &kv.second; return nullptr; }
    double num(const char* k, double d) const { const JVal* v = get(k); return v && v->t == Num ? v->n : d; }
    std::string str(const char* k, const char* d = "") const { const JVal* v = get(k); return v && v->t == Str ? v->s : std::string(d); }
};

class JParser {
public:
    explicit JParser(const std::string& txt) : buf(txt), p(buf.c_str()), e(buf.c_str() + buf.size()) {}
    bool parse(JVal& out) { ws(); if (!value(out, 0)) return false; ws(); return true; }
private:
    std::string buf;
    const char* p; const char* e;
    void ws() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p; }
    static void utf8(std::string& s, uint32_t cp) {
        if (cp < 0x80) s += (char)cp;
        else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 63)); }
        else if (cp < 0x10000) { s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 63)); s += (char)(0x80 | (cp & 63)); }
        else { s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 63)); s += (char)(0x80 | ((cp >> 6) & 63)); s += (char)(0x80 | (cp & 63)); }
    }
    bool hex4(uint32_t& v) {
        if (e - p < 4) return false;
        v = 0;
        for (int i = 0; i < 4; ++i) { const char c = *p++; v <<= 4; if (c >= '0' && c <= '9') v |= c - '0'; else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10; else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10; else return false; }
        return true;
    }
    bool string(std::string& s) {
        if (p >= e || *p != '"') return false;
        ++p;
        while (p < e && *p != '"') {
            if (*p == '\\') {
                ++p; if (p >= e) return false;
                const char c = *p++;
                switch (c) {
                case 'n': s += '\n'; break; case 't': s += '\t'; break; case 'r': s += '\r'; break;
                case 'b': s += '\b'; break; case 'f': s += '\f'; break;
                case 'u': {
                    uint32_t cp; if (!hex4(cp)) return false;
                    if (cp >= 0xD800 && cp < 0xDC00 && e - p >= 6 && p[0] == '\\' && p[1] == 'u') { p += 2; uint32_t lo; if (!hex4(lo)) return false; cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); }
                    utf8(s, cp); break;
                }
                default: s += c; break;
                }
            } else s += *p++;
        }
        if (p >= e) return false;
        ++p;
        return true;
    }
    bool value(JVal& v, int depth) {
        if (depth > 32 || p >= e) return false;
        ws();
        if (*p == '{') {
            v.t = JVal::Obj; ++p; ws();
            if (p < e && *p == '}') { ++p; return true; }
            while (p < e) {
                ws(); std::string k; if (!string(k)) return false;
                ws(); if (p >= e || *p != ':') return false; ++p;
                JVal c; if (!value(c, depth + 1)) return false;
                v.o.push_back(std::make_pair(k, c));
                ws(); if (p < e && *p == ',') { ++p; continue; }
                if (p < e && *p == '}') { ++p; return true; }
                return false;
            }
            return false;
        }
        if (*p == '[') {
            v.t = JVal::Arr; ++p; ws();
            if (p < e && *p == ']') { ++p; return true; }
            while (p < e) {
                JVal c; if (!value(c, depth + 1)) return false;
                v.a.push_back(c);
                ws(); if (p < e && *p == ',') { ++p; continue; }
                if (p < e && *p == ']') { ++p; return true; }
                return false;
            }
            return false;
        }
        if (*p == '"') { v.t = JVal::Str; return string(v.s); }
        if (e - p >= 4 && !std::strncmp(p, "true", 4)) { v.t = JVal::Bool; v.b = true; p += 4; return true; }
        if (e - p >= 5 && !std::strncmp(p, "false", 5)) { v.t = JVal::Bool; v.b = false; p += 5; return true; }
        if (e - p >= 4 && !std::strncmp(p, "null", 4)) { v.t = JVal::Null; p += 4; return true; }
        char* end = nullptr;
        v.n = std::strtod(p, &end);
        if (!end || end == p) return false;
        v.t = JVal::Num; p = end;
        return true;
    }
};

static inline std::string jsonEscape(const std::string& s) {
    std::string o = "\"";
    for (unsigned char c : s) {
        if (c == '"') o += "\\\""; else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n"; else if (c == '\r') o += "\\r"; else if (c == '\t') o += "\\t";
        else if (c < 0x20) { char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
        else o += (char)c;
    }
    return o + "\"";
}

// ====================================================================== presets
struct PresetData {
    std::string name;
    float params[kParamCount];
    bool has[kParamCount];
    std::string lyrics, pats[4], song, smp[4];
    int root[4] = {60, 60, 60, 60};
    bool valid = false;
    PresetData() { for (int i = 0; i < kParamCount; ++i) { params[i] = kParams[i].def; has[i] = false; } }
};

static inline const char* eraName(int era) {
    static const char* const n[5] = { "PC-98", "WIN XP", "MY SAMPLES", "ROCK", "REIWA" };
    return era >= 0 && era < 5 ? n[era] : "?";
}

static inline std::string presetToJson(const PresetData& P, const char* indent = "  ") {
    std::string s = "{\n";
    std::string in(indent);
    s += in + "\"format\": \"RetroNovelFM preset\",\n";
    s += in + "\"version\": 8,\n";
    s += in + "\"name\": " + jsonEscape(P.name) + ",\n";
    const int inst = (int)std::lround(P.params[pInstrument]);
    if (inst >= 0 && inst < kNumInstruments) {
        s += in + "\"era\": " + jsonEscape(eraName(eraOf(inst))) + ",\n";
        s += in + "\"instrumentName\": " + jsonEscape(kInstrumentNames[inst]) + ",\n";
    }
    s += in + "\"params\": {";
    bool first = true;
    for (int i = 0; i < kParamCount; ++i) {
        if (kParams[i].output) continue;
        char b[96];
        std::snprintf(b, sizeof(b), "%s\n%s  \"%s\": %.6g", first ? "" : ",", indent, kParams[i].symbol, (double)P.params[i]);
        s += b; first = false;
    }
    s += "\n" + in + "},\n";
    s += in + "\"lyrics\": " + jsonEscape(P.lyrics) + ",\n";
    s += in + "\"userPatterns\": [";
    for (int u = 0; u < 4; ++u) s += (u ? ", " : "") + jsonEscape(P.pats[u]);
    s += "],\n";
    s += in + "\"song\": " + jsonEscape(P.song) + ",\n";
    s += in + "\"samples\": [";
    for (int u = 0; u < 4; ++u) { char b[32]; std::snprintf(b, sizeof(b), ", \"root\": %d}", P.root[u]); s += (u ? ", " : "") + std::string("{\"path\": ") + jsonEscape(P.smp[u]) + b; }
    s += "]\n";
    return s + std::string(indent).substr(2) + "}";
}

static inline bool presetFromJson(const JVal& v, PresetData& P) {
    if (v.t != JVal::Obj) return false;
    const std::string fmt = v.str("format");
    if (fmt != "RetroNovelFM preset") return false;
    P = PresetData();
    P.name = v.str("name", "Preset");
    if (const JVal* pr = v.get("params"))
        for (int i = 0; i < kParamCount; ++i) {
            if (kParams[i].output) continue;
            const JVal* x = pr->get(kParams[i].symbol);
            if (x && x->t == JVal::Num) {
                float f = (float)x->n;
                f = f < kParams[i].min ? kParams[i].min : (f > kParams[i].max ? kParams[i].max : f);
                P.params[i] = f; P.has[i] = true;
            }
        }
    P.lyrics = v.str("lyrics");
    if (const JVal* up = v.get("userPatterns")) for (int u = 0; u < 4 && u < (int)up->a.size(); ++u) if (up->a[u].t == JVal::Str) P.pats[u] = up->a[u].s;
    P.song = v.str("song");
    if (const JVal* sm = v.get("samples"))
        for (int u = 0; u < 4 && u < (int)sm->a.size(); ++u) { P.smp[u] = sm->a[u].str("path"); P.root[u] = (int)sm->a[u].num("root", 60); }
    P.valid = true;
    return true;
}

// pack: {"format": "RetroNovelFM pack", "presets": [preset | null, ...]}
static inline std::string packToJson(const std::vector<PresetData>& v, const char* name, bool keepEmpty) {
    std::string s = "{\n  \"format\": \"RetroNovelFM pack\",\n  \"version\": 8,\n  \"name\": " + jsonEscape(name) + ",\n  \"presets\": [\n";
    bool first = true;
    for (const PresetData& P : v) {
        if (!P.valid && !keepEmpty) continue;
        s += first ? "    " : ",\n    ";
        s += P.valid ? presetToJson(P, "      ") : std::string("null");
        first = false;
    }
    return s + "\n  ]\n}\n";
}
static inline int packFromJson(const JVal& v, std::vector<PresetData>& out) {
    out.clear();
    if (v.t != JVal::Obj) return -1;
    if (v.str("format") == "RetroNovelFM preset") { PresetData P; if (presetFromJson(v, P)) out.push_back(P); return (int)out.size(); }
    if (v.str("format") != "RetroNovelFM pack") return -1;
    const JVal* a = v.get("presets");
    if (!a || a->t != JVal::Arr) return -1;
    for (const JVal& x : a->a) { PresetData P; if (x.t == JVal::Obj) presetFromJson(x, P); out.push_back(P); }
    return (int)out.size();
}

static inline bool readTextFile(const char* path, std::string& out) {
    FILE* f = retroFopenUtf8(path, "rb");
    if (!f) return false;
    out.clear();
    char buf[8192]; size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) { out.append(buf, n); if (out.size() > (16u << 20)) break; }
    std::fclose(f);
    if (out.size() >= 3 && (unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF) out.erase(0, 3);
    return true;
}
static inline bool writeTextFile(const char* path, const std::string& s) {
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) return false;
    const bool ok = std::fwrite(s.data(), 1, s.size(), f) == s.size();
    std::fclose(f);
    return ok;
}
static inline std::string withExt(const char* path, const char* ext) {
    std::string p(path);
    const size_t sl = p.find_last_of("/\\");
    const size_t dot = p.find_last_of('.');
    if (dot == std::string::npos || (sl != std::string::npos && dot < sl)) p += ext;
    return p;
}

// ====================================================================== Standard MIDI File (type 1, 480 ppq)
struct MidiNote { double t, dur; int note, vel; std::string lyric; };
struct MidiTrack { std::string name; int channel = 0; std::vector<MidiNote> notes; };

static inline bool writeMidiFile(const char* path, const std::vector<MidiTrack>& tracks, double bpm, int num, int den) {
    auto vlq = [](std::vector<uint8_t>& b, uint32_t v) {
        uint8_t tmp[5]; int n = 0;
        tmp[n++] = v & 0x7F;
        while (v >>= 7) tmp[n++] = 0x80 | (v & 0x7F);
        while (n) b.push_back(tmp[--n]);
    };
    auto meta = [&](std::vector<uint8_t>& b, uint32_t dt, uint8_t type, const std::string& data) {
        vlq(b, dt); b.push_back(0xFF); b.push_back(type); vlq(b, (uint32_t)data.size()); b.insert(b.end(), data.begin(), data.end());
    };
    std::vector<std::vector<uint8_t>> chunks;
    {   // conductor track
        std::vector<uint8_t> b;
        meta(b, 0, 0x03, "RetroNovelFM");
        const uint32_t us = (uint32_t)std::lround(60000000.0 / (bpm > 0 ? bpm : 120.0));
        std::string tempo; tempo += (char)((us >> 16) & 255); tempo += (char)((us >> 8) & 255); tempo += (char)(us & 255);
        meta(b, 0, 0x51, tempo);
        int dp = 0; while ((1 << dp) < den) ++dp;
        std::string ts; ts += (char)num; ts += (char)dp; ts += (char)24; ts += (char)8;
        meta(b, 0, 0x58, ts);
        meta(b, 0, 0x2F, "");
        chunks.push_back(b);
    }
    for (const MidiTrack& T : tracks) {
        struct Ev { uint32_t tick; int order; uint8_t st, d1, d2; std::string lyric; };
        std::vector<Ev> ev;
        for (const MidiNote& n : T.notes) {
            const uint32_t on = (uint32_t)std::max(0L, std::lround(n.t * 120.0));
            const uint32_t off = std::max(on + 1, (uint32_t)std::lround((n.t + n.dur) * 120.0));
            if (!n.lyric.empty()) ev.push_back(Ev{on, 1, 0, 0, 0, n.lyric});
            ev.push_back(Ev{on, 2, (uint8_t)(0x90 | (T.channel & 15)), (uint8_t)std::max(0, std::min(127, n.note)), (uint8_t)std::max(1, std::min(127, n.vel)), std::string()});
            ev.push_back(Ev{off, 0, (uint8_t)(0x80 | (T.channel & 15)), (uint8_t)std::max(0, std::min(127, n.note)), 0, std::string()});
        }
        std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.tick != b.tick ? a.tick < b.tick : a.order < b.order; });
        std::vector<uint8_t> b;
        meta(b, 0, 0x03, T.name);
        uint32_t last = 0;
        for (const Ev& x : ev) {
            const uint32_t dt = x.tick - last; last = x.tick;
            if (x.order == 1) meta(b, dt, 0x05, x.lyric);
            else { vlq(b, dt); b.push_back(x.st); b.push_back(x.d1); b.push_back(x.d2); }
        }
        meta(b, 0, 0x2F, "");
        chunks.push_back(b);
    }
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) return false;
    auto be32 = [&](uint32_t v) { uint8_t c[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v}; std::fwrite(c, 1, 4, f); };
    auto be16 = [&](uint16_t v) { uint8_t c[2] = {(uint8_t)(v >> 8), (uint8_t)v}; std::fwrite(c, 1, 2, f); };
    std::fwrite("MThd", 1, 4, f); be32(6); be16(1); be16((uint16_t)chunks.size()); be16(480);
    for (auto& c : chunks) { std::fwrite("MTrk", 1, 4, f); be32((uint32_t)c.size()); std::fwrite(c.data(), 1, c.size(), f); }
    std::fclose(f);
    return true;
}

// recorded take -> notes (pairs on/off). barSteps: shift so the take starts on a bar line
static inline void takeToNotes(const std::vector<RecEvent>& ev, int barSteps, std::vector<MidiNote>& out, bool& vocal) {
    out.clear(); vocal = false;
    if (ev.empty()) return;
    double t0 = ev[0].t;
    for (const RecEvent& e : ev) t0 = std::min(t0, e.t);
    const double shift = std::floor(t0 / barSteps + 1e-6) * barSteps;
    int openIdx[128]; for (int& x : openIdx) x = -1;
    for (const RecEvent& e : ev) {
        if (e.on) {
            if (openIdx[e.note] >= 0) { MidiNote& m = out[openIdx[e.note]]; m.dur = std::max(0.05, e.t - shift - m.t); }
            MidiNote m; m.t = e.t - shift; m.dur = 1; m.note = e.note; m.vel = e.vel;
            if (e.vocal) { vocal = true; m.lyric = std::to_string((int)e.syl); }
            openIdx[e.note] = (int)out.size();
            out.push_back(m);
        } else if (openIdx[e.note] >= 0) {
            MidiNote& m = out[openIdx[e.note]];
            m.dur = std::max(0.05, e.t - shift - m.t);
            openIdx[e.note] = -1;
        }
    }
}

// ====================================================================== Random Song
struct SongRecipe {
    const char* name; int era;
    int mel[6], back[4], bass[3];
    int backStyle;      // 0 sustain, 1 off-beat stabs, 2 broken chords (8ths)
    int bassStyle;      // 0 whole notes, 1 8th pulse, 2 octave jumps, 3 syncopated
    int genre; int pats[3];
    int scale; int chordMode;   // chord mode for the backing: 1 triad, 2 7th, 3 9th
    int progs[3];
    int bpm; float vocalChance;
};
// progressions: degrees of the scale (0 = I/i). The famous J-pop "royal road" is IV-V-iii-vi.
struct Progression { const char* name; int n; int deg[8]; };
static const Progression kProgressions[] = {
    {"Royal road IV-V-iii-vi", 4, {3, 4, 2, 5}},
    {"Komuro vi-IV-V-I",       4, {5, 3, 4, 0}},
    {"Canon I-V-vi-iii-IV-I-IV-V", 8, {0, 4, 5, 2, 3, 0, 3, 4}},
    {"Axis I-V-vi-IV",         4, {0, 4, 5, 3}},
    {"City pop IV-iii-vi-I",   4, {3, 2, 5, 0}},
    {"Epic i-VI-VII-i",        4, {0, 5, 6, 0}},
    {"Dark i-iv-VII-III",      4, {0, 3, 6, 2}},
    {"Eurobeat VI-VII-i-i",    4, {5, 6, 0, 0}},
    {"Horror i-ii-i-vii",      4, {0, 1, 0, 6}},
    {"Ballad I-vi-IV-V",       4, {0, 5, 3, 4}},
};
static const SongRecipe kRecipes[] = {
    // name                  era      melody                       backing              bass         bk bs genre pats        scale chord progs      bpm vocal
    {"Школьный день",        eraXP,   {24, 33, 84, 36, -1},        {25, 45, 32, -1},     {29, 31, -1}, 2, 3, 0, {0, 2, 0},     0, 1, {0, 3, 2},   112, 0.25f},
    {"Признание",            eraXP,   {27, 24, 33, -1},            {32, 46, 25, -1},     {31, -1},     0, 0, 3, {29, 29, 34},  0, 2, {0, 9, 2},    78, 0.3f},
    {"Битва с боссом",       eraRock, {60, 69, 66, -1},            {71, 67, -1},         {64, -1},     0, 1, 2, {24, 13, 26},  1, 1, {5, 6, 5},   165, 0.15f},
    {"Евробит-гонка",        eraReiwa,{97, 66, 86, -1},            {67, 85, -1},         {64, 5, -1},  1, 2, 1, {20, 21, 23},  1, 1, {7, 5, 6},   155, 0.35f},
    {"Песня айдола",         eraReiwa,{86, 97, 88, -1},            {85, 90, 88, -1},     {103, 64, -1},1, 3, 3, {27, 28, 27},  0, 2, {0, 1, 3},   140, 1.0f},
    {"Lo-Fi вечер",          eraReiwa,{88, 99, 90, -1},            {91, -1},             {103, 64, -1},0, 3, 7, {11, 38, 37},  6, 2, {4, 4, 0},    82, 0.2f},
    {"Future bass закат",    eraReiwa,{86, 88, 99, 97, -1},        {85, -1},             {102, -1},    1, 0, 9, {42, 43, 42},  0, 3, {1, 0, 1},   150, 0.4f},
    {"Хоррор: шкатулка",     eraXP,   {78, 53, 48, -1},            {80, 49, -1},         {-1},         0, 0, 0, {3, 3, 3},     4, 1, {8, 8, 6},    70, 0.1f},
    {"Приключение PC-98",    eraPC98, {6, 7, 3, 21, -1},           {0, 20, -1},          {5, 4, -1},   2, 1, 0, {2, 1, 0},     0, 1, {2, 3, 0},   132, 0.2f},
    {"City Pop ночь",        eraReiwa,{94, 86, -1},                {90, -1},             {103, -1},    1, 3, 9, {45, 45, 45},  0, 2, {4, 0, 9},   112, 0.35f},
    {"Ва-фэнтези",           eraReiwa,{107, 108, 15, -1},          {89, 92, -1},         {101, 103, -1},0, 0, 6, {15, 39, 15}, 3, 1, {5, 6, 5},   120, 0.2f},
    {"Чиптюн-квест",         eraPC98, {77, 6, -1},                 {68, -1},             {5, -1},      0, 1, 8, {41, 41, 1},   0, 1, {3, 2, 0},   150, 0.15f},
};
static constexpr int kNumRecipes = (int)(sizeof(kRecipes) / sizeof(kRecipes[0]));

struct GeneratedSong {
    std::string title, recipe, progression, text, lyrics;
    int era = 0, inst = 0, back = -1, bass = -1, genre = 0, pattern = 0, kit = 0, key = 0, scale = 0, bpm = 120;
    bool vocal = false; int idol = 0;
};

class SongRng {
public:
    explicit SongRng(uint32_t s) : x(s ? s : 0x1234567u) {}
    uint32_t next() { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; }
    int range(int n) { return n <= 1 ? 0 : (int)(next() % (uint32_t)n); }
    float uni() { return (next() & 0xffffff) / 16777216.f; }
    template <int N> int pick(const int (&a)[N]) { int c = 0; while (c < N && a[c] >= 0) ++c; return c ? a[range(c)] : -1; }
private:
    uint32_t x;
};

static inline GeneratedSong generateSong(uint32_t seed, int forceRecipe = -1) {
    SongRng R(seed * 2654435761u + 12345u);
    for (int i = 0; i < 4; ++i) R.next();
    GeneratedSong G;
    const SongRecipe& C = kRecipes[forceRecipe >= 0 && forceRecipe < kNumRecipes ? forceRecipe : R.range(kNumRecipes)];
    G.recipe = C.name; G.era = C.era;
    G.inst = R.pick(C.mel); G.back = R.pick(C.back); G.bass = R.pick(C.bass);
    for (int tries = 0; tries < 6 && G.back == G.inst; ++tries) G.back = R.pick(C.back);
    G.genre = C.genre; G.pattern = C.pats[R.range(3)]; G.kit = kGenres[C.genre].kit;
    G.scale = C.scale; G.bpm = C.bpm;
    G.key = R.range(12);
    const Progression& PR = kProgressions[C.progs[R.range(3)]];
    G.progression = PR.name;
    G.vocal = R.uni() < C.vocalChance;
    G.idol = R.range(kNumIdols);
    const ScaleDef& S = kScales[G.scale];
    const int bars = 8, barLen = 16, loop = bars * barLen;
    struct GenEv { int t, dur, note, vel, track; };
    std::vector<GenEv> ev;
    auto add = [&](int t, int d, int n, int v, int tr) { if (t >= 0 && t < loop && d > 0 && n > 0 && n < 128) ev.push_back(GenEv{t, d, n, v, tr}); };
    // chord roots: degree of the parent scale around the key, backing octave around C3..B3
    auto chordRoot = [&](int deg, int octBase) { return octBase + G.key + S.parent[deg % 7]; };
    for (int b = 0; b < bars; ++b) {
        const int deg = PR.deg[b % PR.n];
        int root = chordRoot(deg, 48); if (root > 59) root -= 12;
        int ch[5]; const int nc = buildChord(root, G.key, G.scale, C.chordMode, ch);
        const int t0 = b * barLen;
        if (G.back >= 0) {
            if (C.backStyle == 0) for (int i = 0; i < nc; ++i) add(t0, barLen, ch[i], 68, 1);
            else if (C.backStyle == 1) { for (int k = 0; k < 4; ++k) for (int i = 0; i < nc; ++i) add(t0 + k * 4 + 2, 2, ch[i], 72, 1); }
            else { static const int ord[8] = {0, 2, 1, 2, 0, 2, 1, 2}; for (int k = 0; k < 8; ++k) add(t0 + k * 2, 2, ch[ord[k] % nc] + (k == 4 ? 12 : 0), 64, 1); }
        }
        if (G.bass >= 0) {
            int bn = root - 12; while (bn < 33) bn += 12; while (bn > 47) bn -= 12;
            if (C.bassStyle == 0) add(t0, barLen, bn, 96, 2);
            else if (C.bassStyle == 1) for (int k = 0; k < 8; ++k) add(t0 + k * 2, 2, bn, k % 2 ? 84 : 100, 2);
            else if (C.bassStyle == 2) for (int k = 0; k < 8; ++k) add(t0 + k * 2, 2, bn + (k % 2 ? 12 : 0), k % 2 ? 90 : 100, 2);
            else { static const int st[4] = {0, 6, 8, 14}, du[4] = {5, 2, 5, 2}; for (int k = 0; k < 4; ++k) add(t0 + st[k], du[k], bn + (k == 3 ? 7 : 0), 96, 2); }
        }
    }
    // ---- melody: motif A (bars 1-2), A' (3-4), B (5-6), A'' (7-8) ending on the tonic
    static const int rhythms[][8][2] = {
        {{0, 2}, {2, 2}, {4, 4}, {8, 2}, {10, 2}, {12, 4}, {-1, 0}},
        {{0, 4}, {4, 2}, {6, 2}, {8, 6}, {14, 2}, {-1, 0}},
        {{0, 3}, {3, 3}, {6, 2}, {8, 4}, {12, 4}, {-1, 0}},
        {{2, 2}, {4, 2}, {6, 2}, {8, 2}, {10, 6}, {-1, 0}},
        {{0, 2}, {2, 2}, {4, 2}, {6, 2}, {8, 4}, {12, 2}, {14, 2}, {-1, 0}},
        {{0, 6}, {6, 2}, {8, 8}, {-1, 0}},
    };
    const int nr = (int)(sizeof(rhythms) / sizeof(rhythms[0]));
    const bool slow = C.bpm < 95;
    int rA[2] = { R.range(nr), R.range(nr) }, rB[2] = { R.range(nr), R.range(nr) };
    if (slow) { rA[1] = 5; rB[1] = 5; }
    // melody scale notes in range
    std::vector<int> mel;
    const int lo = G.vocal ? 60 : 62, hi = G.vocal ? 79 : 84;
    for (int n = lo; n <= hi; ++n) { const int pc = ((n - G.key) % 12 + 12) % 12; for (int i = 0; i < S.n; ++i) if (S.deg[i] == pc) { mel.push_back(n); break; } }
    auto nearestIdx = [&](int n) { int bi = 0, bd = 999; for (int i = 0; i < (int)mel.size(); ++i) { const int d = std::abs(mel[i] - n); if (d < bd) { bd = d; bi = i; } } return bi; };
    int cur = nearestIdx(lo + 7 + G.key % 5);
    std::vector<int> motifPitch;
    int lyricsCount = 0;
    for (int b = 0; b < bars; ++b) {
        const int section = b / 2;   // 0 A, 1 A', 2 B, 3 A''
        const int rid = (section == 2) ? rB[b % 2] : rA[b % 2];
        const int deg = PR.deg[b % PR.n];
        int root = chordRoot(deg, 60); int ch[5]; buildChord(root, G.key, G.scale, 1, ch);
        int k = 0;
        for (; k < 8 && rhythms[rid][k][0] >= 0; ++k) {
            const int t = b * barLen + rhythms[rid][k][0];
            int d = rhythms[rid][k][1];
            const bool strong = (t % 8) == 0;
            const bool lastNote = b == bars - 1 && (k + 1 >= 8 || rhythms[rid][k + 1][0] < 0);
            int target;
            if (lastNote) {
                // home: the tonic near the current pitch
                int best = mel[cur], bd = 999;
                for (int n : mel) if (((n - G.key) % 12 + 12) % 12 == 0 && std::abs(n - mel[cur]) < bd) { bd = std::abs(n - mel[cur]); best = n; }
                cur = nearestIdx(best); d = std::max(d, 4);
            } else if (section == 1 && (b % 2) == 0 && k < (int)motifPitch.size()) {
                // A': same shape, shifted along the scale
                cur = std::max(0, std::min((int)mel.size() - 1, nearestIdx(motifPitch[k]) + (R.range(2) ? 1 : -1)));
            } else if (strong) {
                // chord tone close to where we are
                int best = cur, bd = 999;
                for (int i = 0; i < (int)mel.size(); ++i) {
                    const int pc = mel[i] % 12; bool ct = false;
                    for (int c = 0; c < 3; ++c) if (ch[c] % 12 == pc) ct = true;
                    if (ct && std::abs(i - cur) < bd) { bd = std::abs(i - cur); best = i; }
                }
                cur = best;
            } else {
                // stepwise, sometimes a small leap; drift back to the middle
                const int r = R.range(10);
                int step = r < 4 ? 1 : (r < 8 ? -1 : (r == 8 ? 2 : -2));
                if (cur < 2) step = std::abs(step);
                if (cur > (int)mel.size() - 3) step = -std::abs(step);
                cur = std::max(0, std::min((int)mel.size() - 1, cur + step));
            }
            target = mel[cur];
            if (section == 0 && (b % 2) == 0) motifPitch.push_back(target);
            const int gap = G.vocal ? 0 : (C.bpm > 140 ? 1 : 0);
            add(t, std::max(1, d - gap), target, strong ? 104 : 92, 0);
            ++lyricsCount;
        }
    }
    // ---- text
    char head[64]; std::snprintf(head, sizeof(head), "song1;%d;%d;%d;", loop, G.back, G.bass);
    G.text = head;
    for (const GenEv& e : ev) { char b[48]; std::snprintf(b, sizeof(b), "%d,%d,%d,%d,%d;", e.t, e.dur, e.note, e.vel, e.track); G.text += b; }
    // ---- lyrics: anime-style romaji words, one syllable per melody note
    static const char* const words[] = {
        "sa ku ra", "yu me", "hi ka ri", "so ra", "ki mi", "a i", "na mi da", "mi ra i", "ho shi", "ko ko ro",
        "tsu ba sa", "ka ze", "ha ru", "na tsu", "to mo da chi", "e ga o", "ki se ki", "a shi ta", "u ta", "ne ga i",
        "yo ru", "tsu ki", "ha na", "mi chi", "ka ga ya ku", "to bi ta tsu", "da ki shi me te", "wa su re na i" };
    const int nw = (int)(sizeof(words) / sizeof(words[0]));
    int syl = 0; std::string ly;
    while (syl < lyricsCount) {
        const char* w = words[R.range(nw)];
        int cnt = 1; for (const char* c = w; *c; ++c) if (*c == ' ') ++cnt;
        if (!ly.empty()) ly += ' ';
        ly += w; syl += cnt;
    }
    G.lyrics = ly;
    // ---- title
    static const char* const t1[] = { "Sakura", "Neon", "Mirai", "Hoshi", "Kaze", "Yume", "Natsu", "Tsuki", "Hikari", "Sora", "Kokoro", "Ame" };
    static const char* const t2[] = { "Drive", "Memories", "Days", "Story", "Signal", "Melody", "Road", "Wish", "Night", "Chime", "Parade", "Heart" };
    G.title = std::string(t1[R.range(12)]) + " " + t2[R.range(12)];
    return G;
}

// song events of a generated song text (for MIDI export)
static inline void songToTracks(const char* text, const std::vector<std::string>& syllables, bool vocal, std::vector<MidiTrack>& tracks) {
    tracks.clear();
    MidiTrack mel, ch, bs;
    mel.name = vocal ? "Vocal" : "Melody"; mel.channel = 0;
    ch.name = "Chords"; ch.channel = 1;
    bs.name = "Bass"; bs.channel = 2;
    if (!text || std::strncmp(text, "song1;", 6)) return;
    const char* c = text + 6;
    for (int k = 0; k < 3 && c; ++k) { c = std::strchr(c, ';'); if (c) ++c; }
    int sylIdx = 0;
    std::vector<MidiNote> m0;
    while (c && *c) {
        int t, d, n, v, tr;
        if (std::sscanf(c, "%d,%d,%d,%d,%d", &t, &d, &n, &v, &tr) != 5) break;
        MidiNote m; m.t = t; m.dur = d; m.note = n; m.vel = v;
        if (tr == 0) m0.push_back(m); else if (tr == 1) ch.notes.push_back(m); else bs.notes.push_back(m);
        c = std::strchr(c, ';'); if (c) ++c;
    }
    std::stable_sort(m0.begin(), m0.end(), [](const MidiNote& a, const MidiNote& b) { return a.t < b.t; });
    if (vocal && !syllables.empty()) for (MidiNote& m : m0) m.lyric = syllables[(sylIdx++) % syllables.size()];
    mel.notes = m0;
    tracks.push_back(mel);
    if (!ch.notes.empty()) tracks.push_back(ch);
    if (!bs.notes.empty()) tracks.push_back(bs);
}

} // namespace retrofm
