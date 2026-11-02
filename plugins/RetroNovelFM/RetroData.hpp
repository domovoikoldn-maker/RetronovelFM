// RetroData.hpp - shared tables: parameters, instruments, drum patterns, characters
#pragma once
#include <cstdint>
#include <cstdlib>

namespace retrofm {

enum Param {
    pInstrument = 0, pVolume, pBrightness, pRelease, pVibDepth, pVibRate,
    pLofi, pChorus, pReverb, pBendRange, pTranspose, pVelSens,
    pDrumMode, pDrumPattern, pDrumKit, pDrumVolume, pDrumFill,
    // vocal (singing) mode
    pVocalMode, pIdol, pVGender, pVFormant, pVBreath, pVVib, pVVibRate, pVBright, pVGlide,
    pScene, pDrumGenre,
    // v8: meter, groove, harmony helper, song player
    pMeter, pSwing, pHumanize, pKey, pScale, pChord, pScaleLock, pSongPlay,
    // outputs (DSP -> UI)
    pOutActivity, pOutDrumStep, pOutBeat, pOutMouth, pOutVowel, pOutSyll,
    kParamCount
};

struct ParamInfo { const char* name; const char* symbol; const char* unit; float def, min, max; bool integer; bool output; };

static const ParamInfo kParams[kParamCount] = {
    {"Instrument",       "instrument", "",      0,    0,  108, true,  false},
    {"Volume",           "volume",     "dB",   -8,  -40,    6, false, false},
    {"Brightness",       "brightness", "",      1,    0,    2, false, false},
    {"Release x",        "release",    "",      1, 0.1f,    4, false, false},
    {"Vibrato Depth",    "vibdepth",   "cents", 0,    0,   60, false, false},
    {"Vibrato Rate",     "vibrate",    "Hz",  5.5f, 0.5f,  10, false, false},
    {"Lo-Fi",            "lofi",       "",      0,    0,    1, false, false},
    {"Chorus",           "chorus",     "",   0.3f,    0,    1, false, false},
    {"Reverb",           "reverb",     "",  0.25f,    0,    1, false, false},
    {"Pitch Bend Range", "bendrange",  "st",    2,    0,   12, true,  false},
    {"Transpose",        "transpose",  "st",    0,  -24,   24, true,  false},
    {"Velocity Sens",    "velsens",    "",      1,    0,    1, false, false},
    {"Drums",            "drummode",   "",      0,    0,    2, true,  false},
    {"Drum Pattern",     "drumpattern","",      0,    0,   47, true,  false},
    {"Drum Kit",         "drumkit",    "",      0,    0,    8, true,  false},
    {"Drum Volume",      "drumvolume", "dB",   -9,  -40,    6, false, false},
    {"Drum Fills",       "drumfill",   "",      1,    0,    1, true,  false},
    {"Vocal Mode",       "vocalmode",  "",      0,    0,    1, true,  false},
    {"Idol",             "idol",       "",      0,    0,    5, true,  false},
    {"Voice Gender",     "vgender",    "",   0.9f,    0,    1, false, false},
    {"Voice Size",       "vformant",   "",   0.1f,   -1,    1, false, false},
    {"Breath",           "vbreath",    "",   0.3f,    0,    1, false, false},
    {"Vocal Vibrato",    "vvib",       "cents", 30,   0,  100, false, false},
    {"Vibrato Speed",    "vvibrate",   "Hz",  5.6f,   3,    8, false, false},
    {"Voice Bright",     "vbright",    "",   0.6f,    0,    1, false, false},
    {"Glide",            "vglide",     "ms",   60,    0,  300, false, false},
    {"Scene",            "scene",      "",      0,    0,   26, true,  false},
    {"Drum Style",       "drumgenre",  "",      0,    0,    9, true,  false},
    {"Time Signature",   "meter",      "",      0,    0,    8, true,  false},
    {"Drum Swing",       "swing",      "",      0,    0,    1, false, false},
    {"Drum Humanize",    "humanize",   "",      0,    0,    1, false, false},
    {"Key",              "key",        "",      0,    0,   11, true,  false},
    {"Scale",            "scale",      "",      0,    0,    7, true,  false},
    {"Chord Helper",     "chord",      "",      0,    0,    5, true,  false},
    {"Scale Lock",       "scalelock",  "",      0,    0,    1, true,  false},
    {"Song Player",      "songplay",   "",      0,    0,    1, true,  false},
    {"Activity",         "activity",   "",      0,    0,    1, false, true},
    {"Drum Step",        "drumstep",   "",     -1,   -1,   23, true,  true},
    {"Beat",             "beat",       "",      0,    0,    1, false, true},
    {"Mouth",            "mouth",      "",      0,    0,    1, false, true},
    {"Vowel",            "vowel",      "",      0,    0,    5, true,  true},
    {"Syllable",         "syllable",   "",     -1,   -1, 1023, true,  true},
};

// eras: 0..23 PC-98 FM, 24..53 Windows XP / 2000s samples, 54..57 user sample slots, 58..77 rock & battle,
//       78..84 more 2000s (horror & school), 85..108 REIWA (2020s synths)
static constexpr int kNumFM = 24, kNumXP = 30, kNumUser = 4, kNumRock = 20, kNumXP2 = 7, kNumNeo = 24;
static constexpr int kFirstXP = kNumFM, kFirstUser = kNumFM + kNumXP, kFirstRock = kFirstUser + kNumUser;
static constexpr int kFirstXP2 = kFirstRock + kNumRock, kFirstNeo = kFirstXP2 + kNumXP2;
static constexpr int kNumInstruments = kFirstNeo + kNumNeo;
enum Era { eraPC98 = 0, eraXP, eraUser, eraRock, eraReiwa };
static inline int eraOf(int inst) {
    if (inst < kFirstXP) return eraPC98;
    if (inst < kFirstUser) return eraXP;
    if (inst < kFirstRock) return eraUser;
    if (inst < kFirstXP2) return eraRock;
    if (inst < kFirstNeo) return eraXP;
    return eraReiwa;
}
static inline bool isNeo(int inst) { return inst >= kFirstNeo && inst < kNumInstruments; }

static const char* const kInstrumentNames[kNumInstruments] = {
    // PC-98 / 90s FM
    "FM E.Piano","Music Box","FM Strings","FM Brass","Slap Bass","Synth Bass",
    "PC-98 Lead","Flute","Clarinet","Organ","Harp","Marimba",
    "Vibraphone","Choir Pad","Celesta","Koto","Harpsichord","Oboe",
    "Sitar","Steel Guitar","Warm Pad","Sine Lead","Tubular Bell","Kalimba",
    // Windows XP / 2000s VN (sampled)
    "Grand Piano","Soft Piano","Music Box","Strings","Slow Strings","Pizzicato",
    "Tremolo Str.","Cello","Harp","Flute","Clarinet","Oboe",
    "Recorder","Ocarina","Glockenspiel","Vibraphone","Hand Bells","Kalimba",
    "Zither","Harpsichord","Choir Aah","Nylon Guitar","Glass Pad","Dream Pad",
    "Dark Piano","Horror Drone","Eerie Choir","Broken Box","Reverse Piano","Ghost Bells",
    // user
    "My Sample 1","My Sample 2","My Sample 3","My Sample 4",
    // rock & battle
    "Power Chords","Dist Guitar","Lead Guitar","Clean Guitar","Palm Mute","Chug 16th",
    "Bass Guitar","Rock Organ","Synth Lead","Supersaw Pad","Battle Arp","Brass Section",
    "Brass Stabs","Str Ostinato","Timpani","Taiko","Orchestra Hit","Battle Choir",
    "Epic Toms","Chip Square",
    // 2000s extras: horror & school
    "Cursed Box","Rev. Cymbal","Detuned Pad","School Chime","School Bell","Class Piano","Xylophone",
    // REIWA (2020s)
    "Future Chord","Kawaii Lead","Future Pluck","Crystal Pluck","Bell Pad","City Pop EP",
    "Lo-Fi Keys","Vapor Pad","Shimmer Pad","Synthwave Lead","Synthwave Pad","Hyperpop Lead",
    "Supersaw Lead","J-Core Hoover","Vocal Chop","Wobble Bass","Reese Bass","808 Glide",
    "Slap Funk Bass","Acid 303","Phonk Cowbell","Wah Funk Gtr","Erhu","Shamisen"
};

static const char* const kDrumModeNames[3] = { "Off", "Sync to DAW", "Always" };
static constexpr int kNumKits = 9;
// 0..2 synthesized, 3 orchestral (VCSL), 4..6 + 8 real acoustic kits, 7 Eurobeat (909-style + real cymbals/snare)
static const char* const kDrumKitNames[kNumKits] = { "PC-98 FM", "Analog 808", "8-bit Lo-Fi", "Orchestral", "Rock (live)", "Pop (live)", "Brush Jazz", "Eurobeat", "Lo-Fi (live)" };
static const char* const kDrumKitShort[kNumKits] = { "PC-98", "808", "8-BIT", "ORCH", "ROCK", "POP", "BRUSH", "EURO", "LO-FI" };
static inline int kitSampleSlot(int kit) { return kit == 4 ? 0 : (kit == 5 ? 1 : (kit == 6 ? 2 : (kit == 8 ? 3 : (kit == 7 ? 1 : -1)))); }
static const char* const kUserPatternNames[4] = { "User 1", "User 2", "User 3", "User 4" };
// pattern index space: 0..15 presets, 16..19 user patterns (kept here for older projects), 20..47 more presets
static constexpr int kFirstUserPattern = 16, kNumUserPatterns = 4, kNumPatterns = 48, kNumPresetTable = 44;
static constexpr int kMaxSteps = 24;   // longest bar (12/8 in 16ths)
static inline bool isUserPattern(int p) { return p >= kFirstUserPattern && p < kFirstUserPattern + kNumUserPatterns; }
static inline int presetSlot(int p) { return p < kFirstUserPattern ? p : p - kNumUserPatterns; }

// ---------------------------------------------------------------- drum styles (genre -> kit + patterns)
struct DrumGenre { const char* name; int kit; int pats[6]; };
static constexpr int kNumGenres = 10;
static const DrumGenre kGenres[kNumGenres] = {
    {"VN/RPG",     0, { 0,  2,  3,  6, 10,  1}},
    {"EUROBEAT",   7, {20, 21, 22, 23,  5, 36}},
    {"J-ROCK",     4, {24, 12, 25, 26, 13, 14}},
    {"POP/ANISON", 5, {27, 28, 29, 30, 31, 32}},
    {"JAZZ/CAFE",  6, {33,  4, 34, 35, 11, 30}},
    {"ELECTRO",    1, { 8,  9,  7, 36, 37, 32}},
    {"EPIC",       3, {15, 39, 40, 14,  1, 10}},
    {"LO-FI",      8, {11, 37, 38,  3, 29,  4}},
    {"CHIPTUNE",   2, {41,  1,  2,  0,  9,  5}},
    {"REIWA",      1, {42, 43, 44, 45, 46, 47}},
};

// ---------------------------------------------------------------- time signatures
// 0 = the pattern's own bar. "/4" meters use a straight 16th grid, "/8" meters group eighths by three.
static constexpr int kNumMeters = 9;
struct MeterDef { const char* name; int steps; int compound; int beatSteps; };
static const MeterDef kMeters[kNumMeters] = {
    {"AUTO", 0, 0, 4}, {"2/4", 8, 0, 4}, {"3/4", 12, 0, 4}, {"4/4", 16, 0, 4}, {"5/4", 20, 0, 4},
    {"6/8", 12, 1, 6}, {"7/8", 14, 0, 4}, {"9/8", 18, 1, 6}, {"12/8", 24, 1, 6},
};

// ---------------------------------------------------------------- harmony helper
static const char* const kKeyNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static constexpr int kNumScales = 8;
struct ScaleDef { const char* name; const char* ru; int n; int deg[7]; int parent[7]; };
// deg = notes of the scale; parent = 7-note scale used to build chords on those notes
static const ScaleDef kScales[kNumScales] = {
    {"Major",      "мажор",            7, {0, 2, 4, 5, 7, 9, 11}, {0, 2, 4, 5, 7, 9, 11}},
    {"Minor",      "минор",            7, {0, 2, 3, 5, 7, 8, 10}, {0, 2, 3, 5, 7, 8, 10}},
    {"Pentatonic", "мажорная пентатоника", 5, {0, 2, 4, 7, 9},    {0, 2, 4, 5, 7, 9, 11}},
    {"Minor Pent", "минорная пентатоника", 5, {0, 3, 5, 7, 10},   {0, 2, 3, 5, 7, 8, 10}},
    {"Japan In",   "японский лад Ин",  5, {0, 1, 5, 7, 8},        {0, 1, 3, 5, 7, 8, 10}},
    {"Ryukyu",     "окинавский лад",   5, {0, 4, 5, 7, 11},       {0, 2, 4, 5, 7, 9, 11}},
    {"Dorian",     "дорийский",        7, {0, 2, 3, 5, 7, 9, 10}, {0, 2, 3, 5, 7, 9, 10}},
    {"Harm Minor", "гармонический минор", 7, {0, 2, 3, 5, 7, 8, 11}, {0, 2, 3, 5, 7, 8, 11}},
};
static constexpr int kNumChordModes = 6;
static const char* const kChordModeNames[kNumChordModes] = { "OFF", "TRIAD", "7TH", "9TH", "POWER", "SUS4" };

// snap a note down to the nearest note of the scale
static inline int snapToScale(int note, int key, int scale) {
    const ScaleDef& S = kScales[scale < 0 || scale >= kNumScales ? 0 : scale];
    for (int d = 0; d < 12; ++d) {
        const int n = note - d;
        const int pc = ((n - key) % 12 + 12) % 12;
        for (int i = 0; i < S.n; ++i) if (S.deg[i] == pc) return n;
    }
    return note;
}
// diatonic chord built on 'root' (already in the scale). returns the number of notes written to out
static inline int buildChord(int root, int key, int scale, int mode, int* out) {
    const ScaleDef& S = kScales[scale < 0 || scale >= kNumScales ? 0 : scale];
    if (mode == 4) { out[0] = root; out[1] = root + 7; out[2] = root + 12; return 3; }
    const int pc = ((root - key) % 12 + 12) % 12;
    // nearest degree of the parent scale
    int deg = 0, best = 99;
    for (int i = 0; i < 7; ++i) { const int d = std::abs(S.parent[i] - pc); if (d < best) { best = d; deg = i; } }
    auto tone = [&](int k) { const int i = deg + k; return root - S.parent[deg] + S.parent[i % 7] + 12 * (i / 7); };
    if (mode == 5) { out[0] = root; out[1] = tone(3); out[2] = tone(4); return 3; }
    int n = 0;
    out[n++] = root; out[n++] = tone(2); out[n++] = tone(4);
    if (mode == 2 || mode == 3) out[n++] = tone(6);
    if (mode == 3) out[n++] = tone(8);
    return n;
}

// ---------------------------------------------------------------- vocal character types
struct VoiceType { const char* name; const char* ru; float gender, formant, breath, vib, vibRate, bright, glide; };
static constexpr int kNumVoiceTypes = 6;
static const VoiceType kVoiceTypes[kNumVoiceTypes] = {
    // name          ru                 gender formant breath vib  rate  bright glide   (gender < 0 = keep)
    {"Tsundere",     "цундэрэ",          0.95f,  0.25f, 0.15f,  18, 6.8f, 0.85f,  25},
    {"Yandere",      "яндэрэ",           0.95f,  0.20f, 0.55f,  45, 4.2f, 0.35f, 160},
    {"Teacher",      "учительница",      0.85f, -0.15f, 0.20f,  25, 5.2f, 0.55f,  70},
    {"Little Sister","младшая сестра",   1.00f,  0.55f, 0.40f,  15, 6.5f, 0.80f,  30},
    {"Whisper",      "тихий голос",     -1.00f,  0.10f, 0.85f,  10, 5.0f, 0.25f,  90},
    {"Rock Vocal",   "рок-вокал",       -1.00f, -0.05f, 0.08f,  55, 5.8f, 0.95f,  40},
};

// ---------------------------------------------------------------- scene presets
// up to 3 layered parts (instrument, key range, level). Bass parts usually take the left hand.
struct ScenePart { int inst, lo, hi; float gain; };
struct ScenePreset {
    const char* name; const char* category; const char* desc;
    ScenePart part[3];
    int drumMode, drumPattern, drumKit; float drumVol;
    float reverb, chorus, lofi, bright;
};
static constexpr int kNumScenes = 32;
static const ScenePreset kScenes[kNumScenes] = {
    // name                       category       description
    {"Школьный день",           "Повседневность", "лёгкое пианино, флейта, ксилофон",     {{25, 36, 127, 1.0f}, {33, 60, 127, 0.55f}, {11, 60, 127, 0.45f}}, 1, 0, 5, -12, 0.25f, 0.2f, 0, 1.0f},
    {"Утро перед уроками",      "Повседневность", "рояль, пиццикато, кларнет",           {{24, 36, 127, 0.9f}, {29, 48, 127, 0.6f}, {34, 60, 127, 0.5f}}, 1, 2, 5, -12, 0.22f, 0.15f, 0, 1.0f},
    {"Клубная комната",         "Повседневность", "гитара, блокфлейта, калимба",         {{45, 36, 127, 1.0f}, {36, 60, 127, 0.5f}, {41, 60, 127, 0.45f}}, 1, 4, 6, -14, 0.2f, 0.2f, 0, 1.0f},
    {"Кафе после уроков",       "Повседневность", "вибрафон, FM-пиано, пиццикато",      {{39, 48, 127, 0.9f}, {0, 36, 127, 0.6f}, {29, 36, 59, 0.6f}}, 1, 4, 6, -14, 0.28f, 0.3f, 0, 1.0f},
    {"Летний фестиваль",        "Повседневность", "кото, флейта, колокольчики",          {{15, 36, 127, 1.0f}, {33, 60, 127, 0.5f}, {40, 72, 127, 0.4f}}, 1, 27, 5, -11, 0.25f, 0.2f, 0, 1.0f},
    {"Комедия",                 "Повседневность", "пиццикато, калимба, кларнет",         {{29, 36, 127, 1.0f}, {41, 60, 127, 0.5f}, {34, 48, 127, 0.45f}}, 1, 2, 5, -12, 0.15f, 0.1f, 0, 1.0f},
    {"Признание",               "Романтика",      "скрипки, арфа, мягкое пианино",       {{27, 48, 127, 0.8f}, {32, 36, 127, 0.7f}, {25, 36, 127, 0.8f}}, 0, 29, 5, -16, 0.45f, 0.25f, 0, 0.9f},
    {"Закат на крыше",          "Романтика",      "медленные струнные, рояль, пэд",      {{28, 48, 127, 0.7f}, {24, 36, 127, 0.9f}, {46, 36, 127, 0.4f}}, 1, 29, 5, -16, 0.4f, 0.25f, 0, 0.9f},
    {"Первое свидание",         "Романтика",      "шкатулка, гитара, пэд мечты",         {{26, 60, 127, 0.7f}, {45, 36, 127, 0.9f}, {47, 36, 127, 0.35f}}, 1, 34, 6, -14, 0.35f, 0.3f, 0.15f, 0.9f},
    {"Ночь, луна, тайна",       "Мистика",        "тёмный пэд, шкатулка, хор",           {{46, 36, 127, 0.6f}, {26, 60, 127, 0.7f}, {44, 36, 127, 0.5f}}, 0, 3, 3, -16, 0.5f, 0.3f, 0, 0.8f},
    {"Старый особняк",          "Мистика",        "клавесин, тремоло, призрачные колокола", {{43, 36, 127, 0.9f}, {30, 36, 127, 0.5f}, {53, 60, 127, 0.4f}}, 0, 3, 3, -16, 0.45f, 0.2f, 0, 0.9f},
    {"Хоррор: за дверью",       "Мистика",        "гул, жуткий хор, сломанная шкатулка", {{49, 0, 127, 0.7f}, {50, 36, 127, 0.5f}, {51, 60, 127, 0.6f}}, 0, 3, 3, -16, 0.55f, 0.2f, 0.1f, 0.7f},
    {"Детектив",                "Мистика",        "пиццикато, кларнет, тёмное пианино",  {{29, 36, 127, 0.9f}, {34, 60, 127, 0.5f}, {48, 36, 59, 0.6f}}, 1, 33, 6, -14, 0.3f, 0.15f, 0.1f, 0.9f},
    {"Прощание на станции",     "Грусть",         "рояль, виолончель, струнные",         {{24, 48, 127, 1.0f}, {31, 36, 59, 0.7f}, {28, 48, 127, 0.45f}}, 0, 29, 5, -16, 0.42f, 0.2f, 0, 0.9f},
    {"Дождь и воспоминания",    "Грусть",         "мягкое пианино, обратное пианино, пэд", {{25, 36, 127, 1.0f}, {52, 60, 127, 0.4f}, {47, 36, 127, 0.35f}}, 1, 11, 8, -16, 0.45f, 0.3f, 0.2f, 0.85f},
    {"Битва с боссом",          "Битва",          "рок-гитара, струнное остинато, бас",  {{58, 48, 127, 0.8f}, {71, 48, 127, 0.6f}, {64, 0, 47, 1.0f}}, 1, 13, 4, -8, 0.2f, 0.1f, 0, 1.1f},
    {"Финальный босс",          "Битва",          "медь, боевой хор, литавры",           {{69, 48, 127, 0.9f}, {75, 48, 127, 0.5f}, {72, 0, 47, 1.0f}}, 1, 39, 3, -8, 0.3f, 0.1f, 0, 1.1f},
    {"Погоня",                  "Битва",          "чаг 16-х, арпеджио, бас",             {{63, 48, 127, 0.7f}, {68, 60, 127, 0.45f}, {64, 0, 47, 1.0f}}, 1, 25, 4, -9, 0.15f, 0.1f, 0, 1.1f},
    {"Случайная битва (RPG)",   "Битва",          "синт-лид, оркестровый удар, бас",     {{66, 60, 127, 0.8f}, {74, 48, 59, 0.6f}, {64, 0, 47, 1.0f}}, 1, 24, 4, -9, 0.2f, 0.15f, 0, 1.1f},
    {"Кульминация",             "Битва",          "соло-гитара, суперсо, тайко",         {{60, 60, 127, 0.75f}, {67, 48, 127, 0.4f}, {73, 0, 47, 1.0f}}, 1, 14, 4, -8, 0.3f, 0.2f, 0, 1.1f},
    {"Чиптюн-бой",              "Битва",          "квадрат, арпеджио, синт-бас",         {{77, 60, 127, 0.8f}, {68, 48, 59, 0.45f}, {5, 0, 47, 1.0f}}, 1, 41, 2, -10, 0.12f, 0.1f, 0.35f, 1.0f},
    {"Восточный поединок",      "Битва",          "кото, тайко, эпические томы",         {{15, 48, 127, 1.0f}, {73, 0, 47, 1.0f}, {76, 36, 47, 0.6f}}, 1, 15, 3, -9, 0.3f, 0.15f, 0, 1.0f},
    {"Eurobeat-гонка",          "Битва",          "суперсо, синт-лид, синт-бас",         {{67, 48, 127, 0.5f}, {66, 60, 127, 0.7f}, {5, 0, 47, 1.0f}}, 1, 21, 7, -9, 0.2f, 0.3f, 0, 1.1f},
    {"Подземелье",              "Приключение",    "орган, жуткий хор, литавры",          {{9, 36, 127, 0.8f}, {50, 48, 127, 0.4f}, {72, 0, 47, 0.9f}}, 0, 3, 3, -14, 0.5f, 0.2f, 0, 0.9f},
    {"Город 90-х (PC-98)",      "Приключение",    "PC-98 лид, чиптюн, бас-гитара",       {{6, 60, 127, 0.9f}, {77, 48, 59, 0.4f}, {64, 0, 47, 0.9f}}, 1, 2, 0, -11, 0.2f, 0.25f, 0.3f, 1.0f},
    {"Эндинг / титры",          "Финал",          "рояль, струнные, хор",                {{24, 48, 127, 1.0f}, {27, 36, 127, 0.55f}, {44, 36, 127, 0.4f}}, 1, 29, 5, -16, 0.45f, 0.25f, 0, 0.95f},
    // v8: horror, school and REIWA (2020s)
    {"Проклятая шкатулка",      "Хоррор",         "шкатулка, расстроенный пэд, гул",     {{78, 60, 127, 0.8f}, {80, 36, 127, 0.5f}, {49, 0, 47, 0.6f}}, 0, 3, 3, -16, 0.6f, 0.2f, 0.15f, 0.7f},
    {"Звонок с урока",          "Школа",          "колокол-чайм, ксилофон, пианино класса", {{81, 60, 127, 0.8f}, {84, 72, 127, 0.5f}, {83, 36, 71, 0.8f}}, 1, 0, 5, -12, 0.3f, 0.15f, 0, 1.0f},
    {"Future Bass закат",       "REIWA",          "аккорды future bass, кристальный плак, 808", {{85, 48, 127, 0.7f}, {88, 72, 127, 0.45f}, {102, 0, 47, 1.0f}}, 1, 42, 1, -9, 0.3f, 0.2f, 0, 1.05f},
    {"Lo-Fi комната",           "REIWA",          "lo-fi клавиши, вейпор-пэд, слэп-бас", {{91, 48, 127, 0.9f}, {92, 48, 127, 0.35f}, {103, 0, 47, 0.9f}}, 1, 11, 8, -12, 0.35f, 0.25f, 0.25f, 0.9f},
    {"Неоновый город",          "REIWA",          "City Pop EP, синтвейв-лид, бас",      {{90, 48, 127, 0.9f}, {94, 72, 127, 0.5f}, {103, 0, 47, 0.9f}}, 1, 45, 1, -11, 0.3f, 0.3f, 0, 1.0f},
    {"Гача-фестиваль",          "REIWA",          "эрху, сямисэн, future pluck",         {{107, 60, 127, 0.8f}, {108, 48, 127, 0.6f}, {87, 36, 59, 0.5f}}, 1, 15, 3, -10, 0.35f, 0.2f, 0, 1.0f},
};

// ---------------------------------------------------------------- vocal idols (voice presets)
struct IdolVoice { const char* name; float gender, formant, breath, vib, vibRate, bright, glide; int octave; };
static constexpr int kNumIdols = 6;
static const IdolVoice kIdolVoices[kNumIdols] = {
    // name      gender formant breath vib  rate  bright glide octave(suggested)
    {"Sora",     0.95f,  0.15f, 0.20f,  35, 5.8f, 0.80f,  50, 5},   // bright pop
    {"Mei",      1.00f,  0.35f, 0.45f,  20, 6.2f, 0.65f,  40, 5},   // sweet, breathy, high
    {"Reina",    0.80f, -0.10f, 0.25f,  45, 5.2f, 0.55f,  90, 4},   // cool alto
    {"Haru",     0.90f,  0.05f, 0.40f,  25, 5.0f, 0.45f, 110, 4},   // warm ballad
    {"Hayato",   0.05f, -0.10f, 0.15f,  40, 5.4f, 0.75f,  60, 3},   // rock tenor
    {"Yuu",      0.30f,  0.10f, 0.35f,  25, 5.6f, 0.55f,  80, 3},   // soft boy voice
};
static const char* const kLyricPresets[6][2] = {
    {"La la", "la la la la la la la la ra ra ra ra"},
    {"A I U E O", "a i u e o ka ki ku ke ko sa shi su se so"},
    {"Sakura", "sa ku ra sa ku ra ya yo i no so ra wa mi wa ta su ka gi ri"},
    {"Yume", "yu me no na ka de ki mi wo ma tsu - hi ka ri no mi chi"},
    {"По-русски", "мы по-ём вме-сте до-ро-га све-тла-я"},
    {"Doo-wop", "du bi du ba shu bi du ba da ba da"},
};

// ---------------------------------------------------------------- drum patterns
// lanes: Kick, Snare, Closed hat, Open hat, Clap, Low tom, High tom, Rim, Crash
enum DrumLane { dKick = 0, dSnare, dCHat, dOHat, dClap, dTomL, dTomH, dRim, dCrash, kDrumLanes };
static const char* const kLaneNames[kDrumLanes] = { "KICK","SNARE","C.HAT","O.HAT","CLAP","TOM L","TOM H","RIM","CRASH" };

// 'X' = accent, 'x' = normal, 'g' = ghost, '-' = rest
struct DrumPattern { const char* name; int len; float swing; const char* lane[kDrumLanes]; int triplet; };

static const DrumPattern kPatterns[kNumPresetTable] = {
    {"Daily Life", 16, 0.f, {
        "X-----x-X-------", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Battle!", 16, 0.f, {
        "X--x--x-X--x--x-", "----X-------X--x", "xxxxxxxxxxxxxxxx", "----------------",
        "----------------", "----------------", "----------------", "----------------", "X---------------"}},
    {"Town Theme", 16, 0.10f, {
        "X-----X---X-----", "----X-------X---", "x-g-x-g-x-g-x-g-", "----------------",
        "----------------", "----------------", "----------------", "------x-------x-", "----------------"}},
    {"Sad Scene", 16, 0.f, {
        "X---------X-----", "--------X-------", "x---x---x---x---", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Bossa Cafe", 16, 0.f, {
        "X--xX--xX--xX--x", "----------------", "xgxgxgxgxgxgxgxg", "----------------",
        "----------------", "----------------", "----------------", "X--X--X---X--X--", "----------------"}},
    {"Eurobeat", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "x---x---x---x---", "--x---x---x---x-",
        "----X-------X---", "----------------", "----------------", "----------------", "----------------"}},
    {"Waltz", 12, 0.f, {
        "X-----------", "------------", "x-g-x-g-x-g-", "------------",
        "------------", "------------", "------------", "----x---x---", "------------"}},
    {"Breakbeat", 16, 0.f, {
        "X-X-------XX----", "----X--g-g--X--g", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Four on Floor", 16, 0.f, {
        "X---X---X---X---", "----------------", "x---x---x---x---", "--X---X---X---X-",
        "----X-------X---", "----------------", "----------------", "----------------", "----------------"}},
    {"Drum'n'Bass", 16, 0.f, {
        "X---------X-----", "----X--g----X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"March", 16, 0.f, {
        "X-------X-------", "X-xxX-x-X-xxXxxx", "----------------", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Lo-Fi Chill", 16, 0.16f, {
        "X-----gX--X-----", "----X-------X---", "x-xgx-x-x-xgx-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Rock 8th", 16, 0.f, {
        "X---x---X-x-x---", "----X-------X---", "x-x-x-x-x-x-x-x-", "--------------x-",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Boss Battle", 16, 0.f, {
        "XxXxXxXxXxXxXxXx", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "X-------X-------"}},
    {"Climax", 16, 0.f, {
        "X--x--x-X--x--x-", "----X-------X-X-", "x-x-x-x-x-x-x---", "--------------X-",
        "----------------", "------------x-xX", "----------x-x---", "----------------", "X---------------"}},
    {"Taiko March", 16, 0.f, {
        "X--X--X-X-X-X---", "--------------X-", "----------------", "----------------",
        "----------------", "x---x---x-xx--x-", "--x---x-----x---", "x-x-x-x-x-x-x-x-", "----------------"}},
    // ---------------- index 20.. (after the 4 user slots)
    {"Euro Verse", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "x-x---x-x-x---x-", "--X---X---X---X-",
        "----X-------X---", "----------------", "----------------", "----------------", "----------------"}},
    {"Euro Chorus", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "xxx-xxx-xxx-xxx-", "--X---X---X---X-",
        "----X-------X--x", "----------------", "----------------", "----------------", "X---------------"}},
    {"Euro Break", 16, 0.f, {
        "X-------X-x-----", "----X-------X-xx", "x-x-x-x-x-x-x-x-", "----------------",
        "----X-------X---", "------------x-x-", "-------------x-x", "----------------", "----------------"}},
    {"Para Para", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "x-x-x-x-x-x-x-x-", "--X---X---X---X-",
        "----X--x----X---", "----------------", "----------------", "---x---x---x--x-", "----------------"}},
    {"J-Rock", 16, 0.f, {
        "X-----xX--X---x-", "----X-------X---", "x-x-x-x-x-x-x-x-", "--------------x-",
        "----------------", "----------------", "----------------", "----------------", "X---------------"}},
    {"Punk", 16, 0.f, {
        "X---X---X---X---", "--X---X---X---X-", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "X---------------"}},
    {"Metal", 16, 0.f, {
        "XxXxXxXxXxXxXxXx", "----X-------X---", "x---x---x---x---", "----------------",
        "----------------", "----------------", "----------------", "----------------", "X-------x-------"}},
    {"Anison", 16, 0.f, {
        "X-----x-X-----x-", "----X--g----X-gx", "x-x-x-x-x-x-x-x-", "------x-------x-",
        "----------------", "----------------", "----------------", "----------------", "X---------------"}},
    {"Pop", 16, 0.f, {
        "X-----x---X-----", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Ballad", 16, 0.f, {
        "X-------x-X-----", "--------X-------", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----x-------x---", "----------------"}},
    {"Shuffle", 12, 0.f, {
        "X-----X-----", "---X-----X--", "x-xx-xx-xx-x", "------------",
        "------------", "------------", "------------", "------------", "------------"}, 1},
    {"Funk", 16, 0.f, {
        "X-x---x---X--x--", "----X--g-g--X--g", "xgxgxgxgxgxgxgxg", "--------------x-",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Disco", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "x---x---x---x---", "--X---X---X---X-",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Jazz Swing", 12, 0.f, {
        "g-----g-----", "-----g-----g", "x--x-xx--x-x", "------------",
        "------------", "------------", "------------", "---x-----x--", "------------"}, 1},
    {"Brush Ballad", 16, 0.f, {
        "X-------g-------", "g-g-X-g-g-g-X-g-", "x---x---x---x---", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Jazz Waltz", 9, 0.f, {
        "g--------", "-----g---", "x--x-xx-x", "---------",
        "---------", "---------", "---------", "---x--x--", "---------"}, 1},
    {"Trance", 16, 0.f, {
        "X---X---X---X---", "----------------", "x-xxx-xxx-xxx-xx", "--X---X---X---X-",
        "----X-------X---", "----------------", "----------------", "----------------", "X---------------"}},
    {"Hip-hop", 16, 0.12f, {
        "X------X--X-----", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Boom Bap", 16, 0.18f, {
        "X--x------X--x--", "----X-------X---", "x-x-x-x-x-x-x-xg", "--------------x-",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    {"Epic Battle", 16, 0.f, {
        "X---X---X---X---", "----X-------X---", "----------------", "----------------",
        "----------------", "X--X--X-X--X--X-", "--x---x---x-x-xx", "----------------", "X---------------"}},
    {"Trailer Build", 16, 0.f, {
        "X-------X-------", "------------xxXX", "----------------", "----------------",
        "----------------", "X---X---X-X-X-XX", "----------------", "----------------", "X---------------"}},
    {"Chip Run", 16, 0.f, {
        "X---X-X-X---X-X-", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----------------", "----------------", "----------------", "----------------", "----------------"}},
    // ---------------- REIWA (index 42..47)
    {"Future Bass", 16, 0.f, {
        "X-----x---X-----", "--------X-------", "x-x-x-x-x-x-xxxx", "----------------",
        "--------X-------", "----------------", "----------------", "----------------", "X---------------"}},
    {"Trap", 16, 0.f, {
        "X------X--X-----", "--------X-------", "x-x-xxx-x-x-xxxx", "----------------",
        "--------X-------", "----------------", "----------------", "----------------", "----------------"}},
    {"Phonk", 16, 0.f, {
        "X--X--X---X--x--", "----X-------X---", "x-x-x-x-x-x-x-x-", "----------------",
        "----X-------X---", "----------------", "----------------", "--x---x---x-x---", "----------------"}},
    {"City Pop", 16, 0.08f, {
        "X-----x---X-----", "----X-------X---", "x-xgx-xgx-xgx-xg", "------------x---",
        "----------------", "----------------", "----------------", "---x------x-----", "----------------"}},
    {"Hyperpop", 16, 0.f, {
        "X---X---X---X-X-", "----X-------X---", "xxxxxxxxxxxxxxxx", "----------------",
        "----X-------X--X", "----------------", "----------------", "----------------", "X---------------"}},
    {"J-Core", 16, 0.f, {
        "X---X---X---X-XX", "----X-------X---", "x-x-x-x-x-x-x-x-", "--X---X---X---X-",
        "----X-------X---", "----------------", "----------------", "----------------", "X---------------"}},
};


// ---------------------------------------------------------------- pattern adapted to a time signature
// The drum machine always plays this: the preset/user pattern stretched or cut to the chosen meter.
struct EffPattern {
    int len = 16;          // steps in a bar
    int triplet = 0;       // 1 = steps are 8th-note triplets (only for AUTO on triplet patterns)
    float swing = 0;       // built-in swing of the pattern
    int beatLen = 4;       // steps per beat (for fills and the editor grid)
    int fillLen = 4;       // steps of the fill at the end of every 4th bar
    char swingMask[kMaxSteps];   // 0 = on the grid, 1 = swung 16th, 2 = swung 8th
    char lane[kDrumLanes][kMaxSteps + 1];
};

static inline void effectivePattern(const char* const* lanes, int srcLen, int srcTrip, float srcSwing, int meter, EffPattern& E)
{
    if (srcLen < 1) srcLen = 16;
    if (srcLen > kMaxSteps) srcLen = kMaxSteps;
    for (int l = 0; l < kDrumLanes; ++l) { for (int k = 0; k < kMaxSteps; ++k) E.lane[l][k] = '-'; E.lane[l][kMaxSteps] = 0; }
    for (int k = 0; k < kMaxSteps; ++k) E.swingMask[k] = 0;
    auto srcAt = [&](int lane, int i) -> char {
        if (i < 0 || i >= srcLen || !lanes[lane]) return '-';
        for (int q = 0; q <= i; ++q) if (!lanes[lane][q]) return '-';   // shorter strings
        return lanes[lane][i];
    };
    if (meter <= 0 || meter >= kNumMeters) {
        E.len = srcLen; E.triplet = srcTrip; E.swing = srcSwing;
        E.beatLen = srcTrip ? 3 : 4; E.fillLen = E.beatLen;
        for (int l = 0; l < kDrumLanes; ++l) for (int k = 0; k < srcLen; ++k) E.lane[l][k] = srcAt(l, k);
    } else {
        const MeterDef& M = kMeters[meter];
        const int N = M.steps;
        int tb[12], ts[12], nb = 0;
        if (M.compound) for (int st = 0; st < N; st += 6) { tb[nb] = st; ts[nb++] = 6; }
        else if (N == 14) { tb[0] = 0; ts[0] = 4; tb[1] = 4; ts[1] = 4; tb[2] = 8; ts[2] = 6; nb = 3; }   // 7/8 = 2+2+3
        else for (int st = 0; st < N; st += 4) { tb[nb] = st; ts[nb++] = 4; }
        const int sub = srcTrip ? 3 : 4;
        const int nsb = srcLen / sub > 0 ? (srcLen + sub - 1) / sub : 1;
        for (int j = 0; j < nb; ++j) {
            // beats past the end of the source repeat its 3rd and 4th beat (5/4 = K S K S | K ...)
            const int sb = j < nsb ? j : (nsb >= 2 ? nsb - 2 + ((j - nsb) % 2) : 0);
            for (int l = 0; l < kDrumLanes; ++l) {
                auto src = [&](int k) -> char { return k < sub ? srcAt(l, sb * sub + k) : '-'; };
                char* o = E.lane[l] + tb[j];
                if (ts[j] == 4) {
                    if (!srcTrip) for (int k = 0; k < 4; ++k) o[k] = src(k);
                    else { o[0] = src(0); o[1] = src(1); o[2] = src(2); }
                } else {
                    if (srcTrip) for (int k = 0; k < 3; ++k) o[k * 2] = src(k);
                    else {
                        char e1 = src(1), e2 = src(2) != '-' ? src(2) : src(3);
                        if (l == dCHat && e1 == '-' && src(0) != '-' && e2 != '-') e1 = 'g';
                        o[0] = src(0); o[2] = e1; o[4] = e2;
                    }
                }
            }
            // swing positions inside straight beats
            if (ts[j] == 4) { E.swingMask[tb[j] + 1] = 1; E.swingMask[tb[j] + 2] = 2; E.swingMask[tb[j] + 3] = 1; }
        }
        E.len = N; E.triplet = 0;
        E.swing = M.compound ? 0.f : (srcTrip ? 0.3333f : srcSwing);
        E.beatLen = M.compound ? 6 : 4;
        E.fillLen = ts[nb - 1];
    }
    if (meter <= 0 || meter >= kNumMeters) {
        if (!E.triplet) for (int st = 0; st < E.len; ++st) E.swingMask[st] = (st % 4 == 2) ? 2 : ((st & 1) ? 1 : 0);
    }
    // swing the 16ths only when the groove really has 16ths, otherwise swing the 8ths
    bool has16 = false;
    for (int l = 0; l < kDrumLanes && !has16; ++l) if (l != dCrash) for (int st = 0; st < E.len; ++st) if (E.swingMask[st] == 1 && E.lane[l][st] != '-') { has16 = true; break; }
    for (int st = 0; st < E.len; ++st) {
        if (has16) E.swingMask[st] = E.swingMask[st] == 0 ? 0 : ((st & 1) ? 1 : 0);
        else if (E.swingMask[st] == 1) E.swingMask[st] = 0;
    }
}

static inline float stepVel(char c) { return c == 'X' ? 1.0f : (c == 'x' ? 0.72f : (c == 'g' ? 0.35f : 0.0f)); }

} // namespace retrofm
