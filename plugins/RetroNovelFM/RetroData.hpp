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
    // outputs (DSP -> UI)
    pOutActivity, pOutDrumStep, pOutBeat, pOutMouth, pOutVowel, pOutSyll,
    kParamCount
};

struct ParamInfo { const char* name; const char* symbol; const char* unit; float def, min, max; bool integer; bool output; };

static const ParamInfo kParams[kParamCount] = {
    {"Instrument",       "instrument", "",      0,    0,   57, true,  false},
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
    {"Drum Pattern",     "drumpattern","",      0,    0,   15, true,  false},
    {"Drum Kit",         "drumkit",    "",      0,    0,    3, true,  false},
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
    {"Activity",         "activity",   "",      0,    0,    1, false, true},
    {"Drum Step",        "drumstep",   "",     -1,   -1,   15, true,  true},
    {"Beat",             "beat",       "",      0,    0,    1, false, true},
    {"Mouth",            "mouth",      "",      0,    0,    1, false, true},
    {"Vowel",            "vowel",      "",      0,    0,    5, true,  true},
    {"Syllable",         "syllable",   "",     -1,   -1, 1023, true,  true},
};

// eras: 0..23 PC-98 FM, 24..53 Windows XP / 2000s samples, 54..57 user sample slots
static constexpr int kNumFM = 24, kNumXP = 30, kNumUser = 4;
static constexpr int kFirstXP = kNumFM, kFirstUser = kNumFM + kNumXP;
static constexpr int kNumInstruments = kNumFM + kNumXP + kNumUser;
enum Era { eraPC98 = 0, eraXP, eraUser };
static inline int eraOf(int inst) { return inst < kFirstXP ? eraPC98 : (inst < kFirstUser ? eraXP : eraUser); }

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
    "My Sample 1","My Sample 2","My Sample 3","My Sample 4"
};

static const char* const kDrumModeNames[3] = { "Off", "Sync to DAW", "Always" };
static const char* const kDrumKitNames[4]  = { "PC-98 FM", "Analog 808", "8-bit Lo-Fi", "Live 2000s" };
static const char* const kUserPatternNames[4] = { "User 1", "User 2", "User 3", "User 4" };
static constexpr int kNumPresetPatterns = 12, kNumPatterns = 16;

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
struct DrumPattern { const char* name; int len; float swing; const char* lane[kDrumLanes]; };

static const DrumPattern kPatterns[12] = {
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
};

static inline float stepVel(char c) { return c == 'X' ? 1.0f : (c == 'x' ? 0.72f : (c == 'g' ? 0.35f : 0.0f)); }

} // namespace retrofm
