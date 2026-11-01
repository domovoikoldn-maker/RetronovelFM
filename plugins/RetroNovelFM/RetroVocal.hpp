// RetroVocal.hpp - formant singing synthesizer ("vocal" mode)
// Monophonic singer: glottal pulse source -> 5 cascade formants (Klatt resonators),
// plus a noise branch for consonants. Each MIDI note sings the next syllable of the lyrics.
// Lyrics: romaji ("sa ku ra"), Cyrillic ("ля-ля") ; "-" holds the previous syllable.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace retrofm {

// ------------------------------------------------------------------ phonetics
enum VowelId { vA = 0, vI, vU, vE, vO, vN, kNumVowels };
enum ConsType { ctNone = 0, ctPlosive, ctFric, ctAffric, ctNasal, ctFlap, ctGlide, ctAspir };

struct ConsDef {
    const char* key; ConsType type; bool voiced;
    float closeMs, burstMs, fricMs;      // timeline
    float nf1, nb1, ng1, nf2, nb2, ng2;  // noise bands (Hz, bandwidth, gain)
    float locus2;                        // F2 locus at the vowel onset (0 = none)
    int glideFrom;                       // for glides / nasals: vowel-like start (VowelId) or -1
};

static const ConsDef kCons[] = {
    // key   type        vcd   close burst fric   nf1   nb1  ng1   nf2   nb2  ng2   locus2  glide
    {"k",  ctPlosive,  false, 38,  12,  28,  2300,  700, 1.0f, 3600, 900, 0.4f, 2000, -1},
    {"g",  ctPlosive,  true,  30,   8,   0,  2200,  700, 0.6f, 3400, 900, 0.3f, 2000, -1},
    {"t",  ctPlosive,  false, 36,  10,  24,  4200, 1200, 1.0f, 6000, 1800, 0.5f, 1750, -1},
    {"d",  ctPlosive,  true,  28,   7,   0,  4000, 1200, 0.6f, 5800, 1800, 0.3f, 1750, -1},
    {"p",  ctPlosive,  false, 40,  10,  26,   900,  700, 0.8f, 2500, 1500, 0.3f,  900, -1},
    {"b",  ctPlosive,  true,  30,   7,   0,   900,  700, 0.5f, 2500, 1500, 0.2f,  900, -1},
    {"s",  ctFric,     false,  0,   0, 105,  6200, 1800, 1.0f, 8000, 2000, 0.3f, 1700, -1},
    {"z",  ctFric,     true,   0,   0,  75,  6200, 2200, 0.7f, 8200, 2500, 0.4f, 1700, -1},
    {"sh", ctFric,     false,  0,   0, 105,  3100, 1400, 1.0f, 5200, 2000, 0.6f, 2100, -1},
    {"j",  ctAffric,   true,  22,   0,  60,  3000, 1400, 0.7f, 5000, 2000, 0.4f, 2100, -1},
    {"ch", ctAffric,   false, 30,   0,  70,  3100, 1400, 1.0f, 5200, 2000, 0.5f, 2100, -1},
    {"ts", ctAffric,   false, 30,   0,  65,  6300, 2200, 1.0f, 8300, 2500, 0.6f, 1700, -1},
    {"f",  ctFric,     false,  0,   0,  90,  1600, 3000, 0.45f, 4500, 3000, 0.35f, 1000, -1},
    {"v",  ctFric,     true,   0,   0,  60,  1600, 3000, 0.3f, 4500, 3000, 0.2f, 1000, -1},
    {"h",  ctAspir,    false,  0,   0,  75,     0,    0, 0.0f,    0,    0, 0.0f,    0, -1},
    {"n",  ctNasal,    true,   0,   0,  65,     0,    0, 0.0f,    0,    0, 0.0f, 1650, -1},
    {"m",  ctNasal,    true,   0,   0,  70,     0,    0, 0.0f,    0,    0, 0.0f, 1000, -1},
    {"r",  ctFlap,     true,   0,   0,  28,     0,    0, 0.0f,    0,    0, 0.0f, 1500, -1},
    {"l",  ctFlap,     true,   0,   0,  45,     0,    0, 0.0f,    0,    0, 0.0f, 1300, -1},
    {"y",  ctGlide,    true,   0,   0,  60,     0,    0, 0.0f,    0,    0, 0.0f,    0, vI},
    {"w",  ctGlide,    true,   0,   0,  60,     0,    0, 0.0f,    0,    0, 0.0f,    0, vU},
};
static constexpr int kNumCons = sizeof(kCons) / sizeof(kCons[0]);

// formants (Hz) and bandwidths for a tenor-like (male) and soprano-like (female) singer
static const float kFormM[kNumVowels][5] = {
    {650, 1080, 2650, 2900, 3250}, {290, 1870, 2800, 3250, 3540}, {350, 1100, 2400, 2900, 3300},
    {400, 1700, 2600, 3200, 3580}, {400,  800, 2600, 2800, 3000}, {280, 1300, 2300, 2900, 3300}};
static const float kFormF[kNumVowels][5] = {
    {800, 1150, 2900, 3900, 4950}, {270, 2140, 2950, 3900, 4950}, {330, 1300, 2700, 3800, 4950},
    {350, 2000, 2800, 3600, 4950}, {450,  800, 2830, 3800, 4950}, {290, 1500, 2600, 3700, 4900}};
static const float kBw[kNumVowels][5] = {
    {80, 90, 120, 130, 140}, {50, 90, 100, 120, 120}, {50, 80, 120, 150, 180},
    {60, 90, 110, 130, 140}, {60, 80, 100, 130, 135}, {110, 200, 250, 300, 300}};
static const float kMouth[kNumVowels] = { 1.0f, 0.35f, 0.3f, 0.6f, 0.7f, 0.05f };

struct Syllable { int cons = -1; bool yglide = false; int vowel = vA; bool codaN = false; bool hold = false; char text[16] = {0}; };

// ------------------------------------------------------------------ lyrics parser
static inline void appendLatin(uint32_t cp, std::string& out)
{
    // Cyrillic -> romaji-like letters
    switch (cp) {
    case 0x430: out += "a"; break;  case 0x431: out += "b"; break;  case 0x432: out += "v"; break;
    case 0x433: out += "g"; break;  case 0x434: out += "d"; break;  case 0x435: out += "e"; break;
    case 0x451: out += "yo"; break; case 0x436: out += "j"; break;  case 0x437: out += "z"; break;
    case 0x438: out += "i"; break;  case 0x439: out += "y"; break;  case 0x43A: out += "k"; break;
    case 0x43B: out += "l"; break;  case 0x43C: out += "m"; break;  case 0x43D: out += "n"; break;
    case 0x43E: out += "o"; break;  case 0x43F: out += "p"; break;  case 0x440: out += "r"; break;
    case 0x441: out += "s"; break;  case 0x442: out += "t"; break;  case 0x443: out += "u"; break;
    case 0x444: out += "f"; break;  case 0x445: out += "h"; break;  case 0x446: out += "ts"; break;
    case 0x447: out += "ch"; break; case 0x448: out += "sh"; break; case 0x449: out += "sh"; break;
    case 0x44B: out += "i"; break;  case 0x44D: out += "e"; break;  case 0x44E: out += "yu"; break;
    case 0x44F: out += "ya"; break; default: break;   // soft/hard signs and others are skipped
    }
}

static inline bool parseSyllable(const std::string& tok, Syllable& S)
{
    std::strncpy(S.text, tok.c_str(), sizeof(S.text) - 1);
    // decode UTF-8, lowercase, transliterate
    std::string s;
    for (size_t i = 0; i < tok.size();) {
        const unsigned char c = (unsigned char)tok[i];
        uint32_t cp; int n;
        if (c < 0x80) { cp = c; n = 1; } else if ((c >> 5) == 6 && i + 1 < tok.size()) { cp = ((c & 31) << 6) | (tok[i + 1] & 63); n = 2; }
        else if ((c >> 4) == 14 && i + 2 < tok.size()) { cp = ((c & 15) << 12) | ((tok[i + 1] & 63) << 6) | (tok[i + 2] & 63); n = 3; }
        else { cp = 0; n = 1; }
        i += n;
        if (cp >= 'A' && cp <= 'Z') cp += 32;
        if (cp >= 0x410 && cp <= 0x42F) cp += 32;   // Cyrillic upper -> lower
        if (cp == 0x401) cp = 0x451;
        if (cp < 128) { if (cp >= 'a' && cp <= 'z') s += (char)cp; }
        else appendLatin(cp, s);
    }
    if (s.empty()) return false;
    size_t p = 0;
    S.cons = -1; S.yglide = false; S.codaN = false;
    auto isV = [](char c) { return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o'; };
    // consonant: longest key match
    int best = -1; size_t blen = 0;
    for (int k = 0; k < kNumCons; ++k) {
        const size_t L = std::strlen(kCons[k].key);
        if (L > blen && s.compare(0, L, kCons[k].key) == 0) { best = k; blen = L; }
    }
    if (best >= 0 && !(std::strcmp(kCons[best].key, "n") == 0 && (s.size() == 1 || (!isV(s[1]) && s[1] != 'y')))) {
        S.cons = best; p = blen;
        if (p < s.size() && s[p] == 'y' && std::strcmp(kCons[best].key, "y") != 0 && p + 1 < s.size() && isV(s[p + 1])) { S.yglide = true; ++p; }
    }
    // vowel (skip extra consonants of a cluster, e.g. "vme", "ste")
    size_t q = p;
    while (q < s.size() && !isV(s[q])) ++q;
    if (q < s.size()) p = q;
    else while (p < s.size() && s[p] != 'n' && s[p] != 'm') ++p;
    if (p < s.size() && isV(s[p])) {
        const char v = s[p++];
        S.vowel = v == 'a' ? vA : v == 'i' ? vI : v == 'u' ? vU : v == 'e' ? vE : vO;
    } else if (p < s.size() || (S.cons < 0)) {
        S.vowel = vN; if (p < s.size()) ++p;     // syllabic n / m
    } else S.vowel = vU;                          // lone consonant: devoiced "u"
    // coda
    for (; p < s.size(); ++p) if (s[p] == 'n' || s[p] == 'm') { S.codaN = true; break; }
    return true;
}

static inline void parseLyrics(const char* text, std::vector<Syllable>& out)
{
    out.clear();
    std::string word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        bool onlyDash = true;
        for (char c : word) if (c != '-' && c != '_') onlyDash = false;
        if (onlyDash) { Syllable h; h.hold = true; std::strcpy(h.text, "-"); out.push_back(h); word.clear(); return; }
        std::string part;
        for (size_t i = 0; i <= word.size(); ++i) {
            if (i == word.size() || word[i] == '-' || word[i] == '_') {
                Syllable S;
                if (!part.empty() && parseSyllable(part, S)) out.push_back(S);
                part.clear();
            } else part += word[i];
        }
        word.clear();
    };
    for (const char* c = text; c && *c; ++c) {
        if (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r' || *c == ',' || *c == '.') flushWord();
        else word += *c;
    }
    flushWord();
    if (out.size() > 1024) out.resize(1024);
}

// ------------------------------------------------------------------ singer
struct VoiceParams {
    float gender = 1.f;    // 0 male tract .. 1 female tract
    float formant = 0.f;   // -1..1 extra formant shift ("voice size")
    float breath = 0.25f;  // aspiration
    float vib = 30.f;      // vibrato depth (cents)
    float vibRate = 5.5f;
    float bright = 0.6f;
    float glide = 60.f;    // portamento (ms)
};

class VocalSynth {
public:
    VocalSynth() {
        for (int b = 0; b < 2; ++b) buf[b].reserve(1024);
        setLyrics("la la la la la la la la");
    }

    void setSampleRate(double sr) { srate = sr; reset(); }
    void reset() { active = false; nheld = 0; env = 0; std::memset(y1, 0, sizeof(y1)); std::memset(y2, 0, sizeof(y2)); }
    void setParams(const VoiceParams& p) { vp = p; }
    void setRelease(float r) { relMul = r; }

    // non-realtime: parse new lyrics into the inactive buffer, then publish
    void setLyrics(const char* text) {
        const int w = 1 - cur.load();
        parseLyrics(text ? text : "", buf[w]);
        lyricText = text ? text : "";
        cur.store(w);
        syl = 0;
    }
    const std::string& lyrics() const { return lyricText; }
    void rewind() { syl = 0; }

    void noteOn(int note, int vel) {
        const std::vector<Syllable>& L = buf[cur.load()];
        // monophonic: remember held notes for legato
        if (nheld < 16) held[nheld++] = note;
        const bool legato = active && gate;
        targetHz = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        velocity = vel / 127.f;
        Syllable S;
        int idx = -1;
        if (!L.empty()) { idx = syl % (int)L.size(); S = L[idx]; syl = idx + 1; }
        curSyl = idx;
        if (S.hold && legato) return;          // "-" : just glide to the new pitch
        if (S.hold) { S = sy; S.cons = -1; }
        sy = S;
        t = 0; gate = true;
        legatoStart = legato;
        if (!active) {
            f0 = targetHz * std::pow(2.0, -45.0 / 1200.0);     // small scoop into the note
            active = true; env = 0;
            for (int k = 0; k < 5; ++k) { F[k] = vowelF(sy.vowel, k); B[k] = kBw[sy.vowel][k]; }
            vibT = 0;
        }
        consDur = 0;
        if (sy.cons >= 0) {
            const ConsDef& c = kCons[sy.cons];
            const float sc = legato ? 0.7f : 1.f;
            consDur = (c.closeMs + c.burstMs + c.fricMs) * 0.001f * sc;
            if (sy.yglide) consDur += 0.04f;
        }
        onsetSet = false;
    }

    void noteOff(int note) {
        int j = 0;
        for (int i = 0; i < nheld; ++i) if (held[i] != note) held[j++] = held[i];
        nheld = j;
        if (nheld > 0) { targetHz = 440.0 * std::pow(2.0, (held[nheld - 1] - 69) / 12.0); return; }
        gate = false; relT = 0;
    }
    void allOff() { nheld = 0; gate = false; relT = 0; }

    float tick(double pmul) {
        if (!active) { mouth += (0 - mouth) * 0.002f; return 0.f; }
        const double dt = 1.0 / srate;
        t += dt; vibT += dt;
        // ---- pitch: portamento + vibrato + jitter
        const double gl = std::max(0.004, vp.glide * 0.001 * (legatoStart ? 1.0 : 0.35));
        f0 += (targetHz - f0) * (1.0 - std::exp(-dt / gl));
        const double vibAmt = vp.vib * std::min(1.0, std::max(0.0, (vibT - 0.25) / 0.4));
        vibPh += vp.vibRate * dt; if (vibPh >= 1) vibPh -= 1;
        rng = rng * 1664525u + 1013904223u;
        jit += (((rng >> 9) / 4194304.0 - 1.0) * 0.5 - jit) * 0.0008;
        const double hz = f0 * pmul * std::pow(2.0, (std::sin(6.2831853 * vibPh) * vibAmt + jit * 8.0) / 1200.0);

        // ---- articulation
        float av = 1.f, ah = 0.f, af = 0.f;
        float tF[5], tB[5];
        for (int k = 0; k < 5; ++k) { tF[k] = vowelF(sy.vowel, k); tB[k] = kBw[sy.vowel][k]; }
        float smooth = 0.025f;
        const ConsDef* c = sy.cons >= 0 ? &kCons[sy.cons] : nullptr;
        bool inCons = c && t < consDur;
        if (inCons) {
            const float ms = (float)t * 1000.f / (legatoStart ? 0.7f : 1.f);
            switch (c->type) {
            case ctPlosive:
                if (ms < c->closeMs) { av = c->voiced ? 0.12f : 0.f; tF[0] = 200; }
                else if (ms < c->closeMs + c->burstMs) { av = c->voiced ? 0.3f : 0.f; af = 1.f; }
                else { av = 0.f; ah = 0.55f; }
                break;
            case ctFric:
            {
                const float r = std::min(1.f, ms / 20.f);
                av = c->voiced ? 0.35f : 0.f; af = r;
                break;
            }
            case ctAffric:
                if (ms < c->closeMs) { av = c->voiced ? 0.12f : 0.f; }
                else { av = c->voiced ? 0.3f : 0.f; af = 1.f; }
                break;
            case ctAspir: av = 0.f; ah = 1.f; break;
            case ctNasal:
                av = 0.55f;
                tF[0] = 280; tF[1] = c->key[0] == 'm' ? 1100.f : 1600.f; tF[2] = 2500; tF[3] = 3300; tF[4] = 4200;
                tB[0] = 120; tB[1] = 250; tB[2] = 300; tB[3] = 300; tB[4] = 300;
                smooth = 0.008f;
                break;
            case ctFlap:
                av = 0.45f; tF[0] = 320; tF[1] = c->locus2; tF[2] = c->key[0] == 'l' ? 2900.f : 2200.f;
                smooth = 0.006f;
                break;
            case ctGlide:
                for (int k = 0; k < 5; ++k) tF[k] = vowelF(c->glideFrom, k);
                smooth = 0.012f;
                break;
            default: break;
            }
            if (sy.yglide && consDur - t < 0.05) { for (int k = 0; k < 5; ++k) tF[k] = vowelF(vI, k); av = std::max(av, 0.7f); }
        } else if (c && !onsetSet) {
            // vowel onset: start formants at the consonant locus, then glide
            onsetSet = true;
            if (c->locus2 > 0 && c->type != ctNasal && c->type != ctFlap) { F[1] = 0.5f * (F[1] + c->locus2); F[0] = std::min(F[0], 350.f); }
        }
        // released note with a final "n": move to the nasal
        if (!gate && sy.codaN && relT < 0.08) {
            tF[0] = 280; tF[1] = 1600; tF[2] = 2500; tB[0] = 120; tB[1] = 250; av = 0.6f;
        }
        // high notes: singers raise the first formant above the pitch
        if (tF[0] < hz * 1.1) tF[0] = (float)std::min(hz * 1.1, 1100.0);

        const float k = 1.f - std::exp(-(float)dt / smooth);
        for (int i = 0; i < 5; ++i) { F[i] += (tF[i] - F[i]) * k; B[i] += (tB[i] - B[i]) * k; }

        // ---- envelope
        if (gate) env += (1.f - env) * (1.f - std::exp(-(float)dt / 0.02f));
        else {
            relT += dt;
            env *= std::exp(-(float)dt / (0.09f * relMul + 0.01f));
            if (env < 0.0003f) { active = false; return 0.f; }
        }
        avS += (av - avS) * 0.004f; ahS += (ah - ahS) * 0.01f; afS += (af - afS) * 0.02f;

        // ---- glottal source (Rosenberg pulse, differentiated)
        ph += hz * dt; if (ph >= 1) ph -= 1;
        const double oq = 0.6 - 0.15 * vp.bright, tp = oq * 0.8, tn = oq - tp;
        double g;
        if (ph < tp) g = 0.5 * (1 - std::cos(3.14159265 * ph / tp));
        else if (ph < oq) g = std::cos(1.5707963 * (ph - tp) / tn);
        else g = 0;
        const double dg = (g - gPrev) * srate / std::max(80.0, hz) * 0.05;
        gPrev = g;
        rng = rng * 1664525u + 1013904223u;
        const double nz = (rng >> 8) / 8388608.0 - 1.0;
        double src = dg * avS + nz * (vp.breath * 0.18 * g * avS + ahS * 0.35);
        // spectral tilt (brightness)
        const double tc = 0.25 + 0.75 * vp.bright * (0.6 + 0.4 * velocity);
        tilt += (src - tilt) * tc; src = tilt;

        // ---- cascade formants (Klatt resonators), coefficients refreshed every 16 samples
        if ((++coefCount & 15) == 0 || !coefInit) { updateCoefs(); coefInit = true; }
        double x = src;
        for (int i = 0; i < 5; ++i) {
            const double y = A[i] * x + Bc[i] * y1[i] + Cc[i] * y2[i];
            y2[i] = y1[i]; y1[i] = y; x = y;
        }
        double out = x * 0.9;

        // ---- consonant noise branch
        if (afS > 0.001f && c) {
            out += bandNoise(nz, *c) * afS * 0.5;
        }
        mouth += ((inCons && c && (c->key[0] == 'm' || c->key[0] == 'p' || c->key[0] == 'b') ? 0.f : kMouth[sy.vowel] * env) - mouth) * 0.003f;
        return (float)(out * env * (0.55 + 0.45 * velocity) * 0.32 * std::pow(std::max(1.0, hz / 200.0), -0.45));
    }

    float mouthOpen() const { return mouth; }
    int vowel() const { return sy.vowel; }
    int currentSyllable() const { return active ? curSyl : -1; }
    bool isActive() const { return active; }

private:
    float vowelF(int v, int k) const {
        const float g = vp.gender;
        float f = kFormM[v][k] * (1 - g) + kFormF[v][k] * g;
        return f * std::pow(2.f, vp.formant * 0.25f);
    }
    void updateCoefs() {
        for (int i = 0; i < 5; ++i) {
            const double f = std::min((double)F[i], srate * 0.45), bw = B[i];
            const double r = std::exp(-3.14159265 * bw / srate);
            Cc[i] = -r * r;
            Bc[i] = 2 * r * std::cos(6.2831853 * f / srate);
            A[i] = 1 - Bc[i] - Cc[i];
        }
    }
    double bandNoise(double nz, const ConsDef& c) {
        double o = 0;
        const double fs[2] = { c.nf1, c.nf2 }, bs[2] = { c.nb1, c.nb2 }, gs[2] = { c.ng1, c.ng2 };
        for (int b = 0; b < 2; ++b) {
            if (gs[b] <= 0) continue;
            const double fc = 2 * std::sin(3.14159265 * std::min(fs[b], srate * 0.24) / srate);
            const double q = std::min(1.5, bs[b] / std::max(100.0, fs[b]) * 1.4);
            const double hp = nz - nl[b] - q * nb[b];
            nb[b] += fc * hp; nl[b] += fc * nb[b];
            o += nb[b] * gs[b];
        }
        return o;
    }

    double srate = 44100;
    VoiceParams vp;
    float relMul = 1;
    std::vector<Syllable> buf[2];
    std::atomic<int> cur{0};
    std::string lyricText;
    int syl = 0, curSyl = -1;
    Syllable sy;
    bool active = false, gate = false, legatoStart = false, onsetSet = false, coefInit = false;
    int held[16]; int nheld = 0;
    double t = 0, consDur = 0, relT = 0, vibT = 0, vibPh = 0, jit = 0;
    double f0 = 220, targetHz = 220, ph = 0, gPrev = 0, tilt = 0;
    float velocity = 1, env = 0, avS = 0, ahS = 0, afS = 0, mouth = 0;
    float F[5] = {800, 1150, 2900, 3900, 4950}, B[5] = {80, 90, 120, 130, 140};
    double A[5], Bc[5], Cc[5], y1[5], y2[5];
    double nl[2] = {0, 0}, nb[2] = {0, 0};
    uint32_t rng = 0x9e3779b9u, coefCount = 0;
};

} // namespace retrofm
