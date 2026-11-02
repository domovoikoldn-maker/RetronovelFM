// RetroData.hpp - shared tables: parameters, instruments, drum patterns, characters
#pragma once
#include <cstdint>

namespace retrofm {

enum Param {
    pInstrument = 0, pVolume, pBrightness, pRelease, pVibDepth, pVibRate,
    pLofi, pChorus, pReverb, pBendRange, pTranspose, pVelSens,
    pDrumMode, pDrumPattern, pDrumKit, pDrumVolume, pDrumFill,
    // vocal (singing) mode
    pVocalMode, pIdol, pVGender, pVFormant, pVBreath, pVVib, pVVibRate, pVBright, pVGlide,
    pScene, pDrumGenre,
    // outputs (DSP -> UI)
    pOutActivity, pOutDrumStep, pOutBeat, pOutMouth, pOutVowel, pOutSyll,
    kParamCount
};

struct ParamInfo { const char* name; const char* symbol; const char* unit; float def, min, max; bool integer; bool output; };

static const ParamInfo kParams[kParamCount] = {
    {"Instrument",       "instrument", "",      0,    0,   77, true,  false},
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
    {"Drum Pattern",     "drumpattern","",      0,    0,   41, true,  false},
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
    {"Drum Style",       "drumgenre",  "",      0,    0,    8, true,  false},
    {"Activity",         "activity",   "",      0,    0,    1, false, true},
    {"Drum Step",        "drumstep",   "",     -1,   -1,   15, true,  true},
    {"Beat",             "beat",       "",      0,    0,    1, false, true},
    {"Mouth",            "mouth",      "",      0,    0,    1, false, true},
    {"Vowel",            "vowel",      "",      0,    0,    5, true,  true},
    {"Syllable",         "syllable",   "",     -1,   -1, 1023, true,  true},
};

// eras: 0..23 PC-98 FM, 24..53 Windows XP / 2000s samples, 54..57 user sample slots, 58..77 rock & battle
static constexpr int kNumFM = 24, kNumXP = 30, kNumUser = 4, kNumRock = 20;
static constexpr int kFirstXP = kNumFM, kFirstUser = kNumFM + kNumXP, kFirstRock = kFirstUser + kNumUser;
static constexpr int kNumInstruments = kNumFM + kNumXP + kNumUser + kNumRock;
enum Era { eraPC98 = 0, eraXP, eraUser, eraRock };
static inline int eraOf(int inst) { return inst < kFirstXP ? eraPC98 : (inst < kFirstUser ? eraXP : (inst < kFirstRock ? eraUser : eraRock)); }

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
    "Epic Toms","Chip Square"
};

static const char* const kDrumModeNames[3] = { "Off", "Sync to DAW", "Always" };
static constexpr int kNumKits = 9;
// 0..2 synthesized, 3 orchestral (VCSL), 4..6 + 8 real acoustic kits, 7 Eurobeat (909-style + real cymbals/snare)
static const char* const kDrumKitNames[kNumKits] = { "PC-98 FM", "Analog 808", "8-bit Lo-Fi", "Orchestral", "Rock (live)", "Pop (live)", "Brush Jazz", "Eurobeat", "Lo-Fi (live)" };
static const char* const kDrumKitShort[kNumKits] = { "PC-98", "808", "8-BIT", "ORCH", "ROCK", "POP", "BRUSH", "EURO", "LO-FI" };
static inline int kitSampleSlot(int kit) { return kit == 4 ? 0 : (kit == 5 ? 1 : (kit == 6 ? 2 : (kit == 8 ? 3 : (kit == 7 ? 1 : -1)))); }
static const char* const kUserPatternNames[4] = { "User 1", "User 2", "User 3", "User 4" };
// pattern index space: 0..15 presets, 16..19 user patterns (kept here for older projects), 20..41 more presets
static constexpr int kFirstUserPattern = 16, kNumUserPatterns = 4, kNumPatterns = 42, kNumPresetTable = 38;
static inline bool isUserPattern(int p) { return p >= kFirstUserPattern && p < kFirstUserPattern + kNumUserPatterns; }
static inline int presetSlot(int p) { return p < kFirstUserPattern ? p : p - kNumUserPatterns; }

// ---------------------------------------------------------------- drum styles (genre -> kit + patterns)
struct DrumGenre { const char* name; int kit; int pats[6]; };
static constexpr int kNumGenres = 9;
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
static constexpr int kNumScenes = 26;
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
};

static inline float stepVel(char c) { return c == 'X' ? 1.0f : (c == 'x' ? 0.72f : (c == 'g' ? 0.35f : 0.0f)); }

} // namespace retrofm
