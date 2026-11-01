// RetroData.hpp - shared tables: parameters, instruments, drum patterns, characters
#pragma once
#include <cstdint>

namespace retrofm {

enum Param {
    pInstrument = 0, pVolume, pBrightness, pRelease, pVibDepth, pVibRate,
    pLofi, pChorus, pReverb, pBendRange, pTranspose, pVelSens,
    pDrumMode, pDrumPattern, pDrumKit, pDrumVolume, pDrumFill,
    // outputs (DSP -> UI)
    pOutActivity, pOutDrumStep, pOutBeat,
    kParamCount
};

struct ParamInfo { const char* name; const char* symbol; const char* unit; float def, min, max; bool integer; bool output; };

static const ParamInfo kParams[kParamCount] = {
    {"Instrument",       "instrument", "",      0,    0,   23, true,  false},
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
    {"Drum Pattern",     "drumpattern","",      0,    0,   11, true,  false},
    {"Drum Kit",         "drumkit",    "",      0,    0,    2, true,  false},
    {"Drum Volume",      "drumvolume", "dB",   -9,  -40,    6, false, false},
    {"Drum Fills",       "drumfill",   "",      1,    0,    1, true,  false},
    {"Activity",         "activity",   "",      0,    0,    1, false, true},
    {"Drum Step",        "drumstep",   "",     -1,   -1,   15, true,  true},
    {"Beat",             "beat",       "",      0,    0,    1, false, true},
};

static const char* const kInstrumentNames[24] = {
    "FM E.Piano","Music Box","FM Strings","FM Brass","Slap Bass","Synth Bass",
    "PC-98 Lead","Flute","Clarinet","Organ","Harp","Marimba",
    "Vibraphone","Choir Pad","Celesta","Koto","Harpsichord","Oboe",
    "Sitar","Steel Guitar","Warm Pad","Sine Lead","Tubular Bell","Kalimba"
};

static const char* const kDrumModeNames[3] = { "Off", "Sync to DAW", "Always" };
static const char* const kDrumKitNames[3]  = { "PC-98 FM", "Analog 808", "8-bit Lo-Fi" };

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
