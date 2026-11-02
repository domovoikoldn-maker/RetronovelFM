// Retro Novel FM - visual-novel style UI (NanoVG)
#include "DistrhoUI.hpp"
#include "RetroData.hpp"
#include "RetroCharacters.hpp"
#include "pixelart/PixelArt.hpp"
#include "RetroVocal.hpp"
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

namespace retrofm {
uint32_t winTranslateTyped(uint32_t hostChar);
bool winCtrlDown();
bool winKeyVDown();
bool winLayoutIsCyrillic();
}

START_NAMESPACE_DISTRHO

using namespace retrofm;

static constexpr float kW = 960.f, kH = 660.f;
static constexpr float kPi = 3.14159265f;

static Color hexc(uint32_t h, float a = 1.f) { return Color((int)((h >> 16) & 255), (int)((h >> 8) & 255), (int)(h & 255), a); }
static Color mixc(uint32_t a, uint32_t b, float t, float alpha = 1.f) {
    const float ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
    const float br = (b >> 16) & 255, bg = (b >> 8) & 255, bb = b & 255;
    return Color((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t), (int)(ab + (bb - ab) * t), alpha);
}
static float hash01(int i) { uint32_t x = (uint32_t)i * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return (x & 0xffffff) / 16777215.f; }

struct Box {
    float x = 0, y = 0, w = 0, h = 0;
    bool hit(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

enum CtlType { ctSelect, ctKnob, ctToggle, ctAction, ctEraTab, ctLoad, ctClearSmp, ctRootDn, ctRootUp,
               ctEditOpen, ctEditClose, ctEditUser, ctEditLen, ctEditClear, ctEditCopy, ctCell,
               ctIdol, ctLyricPreset, ctLyricEdit, ctLyricRewind, ctLyricRU, ctLyricPaste,
               ctScenesOpen, ctSceneCard, ctSceneOff, ctScenesClose, ctStoryOpen, ctLesson, ctStoryMenuClose,
               ctGenre, ctGenrePat, ctKitStep };
// control groups: 0 = always, 1..3 = only on that era tab (FM / XP / user), 10 = drum editor overlay
enum CtlGroup { gAlways = 0, gFM = 1, gXP = 2, gUser = 3, gEditor = 10, gVocal = 11, gInst = 12, gRock = 13, gScenes = 14, gStoryMenu = 15 };
enum ActionId { aPrev, aNext, aRandom };

static constexpr int kNumLessons = 5;

struct Control {
    CtlType type; int param; float value; Box b; const char* label; int group;
};

class RetroNovelFMUI : public UI
{
public:
    RetroNovelFMUI()
        : UI((uint)kW, (uint)kH)
    {
        loadSharedResources();
        for (int i = 0; i < kParamCount; ++i) fParams[i] = kParams[i].def;
        fStart = std::chrono::steady_clock::now();

        const double sf = getScaleFactor();
        setGeometryConstraints((uint)(kW * 0.6 * sf), (uint)(kH * 0.6 * sf), true, false);
        if (sf != 1.0) setSize((uint)(kW * sf), (uint)(kH * sf));

        static const int seed[4] = { 0, 5, 7, 11 };
        for (int u = 0; u < 4; ++u) { copyPreset(u, seed[u]); fRoot[u] = 60; }
        fLyrics = kLyricPresets[0][1]; parseLyrics(fLyrics.c_str(), fSyl);
        buildLayout();
       #ifdef RETRO_SCREENSHOT
        if (std::getenv("RETRO_EDITOR")) { fEditor = true; fEditUser = 0; }
        if (const char* e = std::getenv("RETRO_VOCAL")) { fParams[pVocalMode] = 1; fParams[pIdol] = (float)std::atoi(e); fForceVocal = true;
            fLyrics = kLyricPresets[2][1]; parseLyrics(fLyrics.c_str(), fSyl); fParams[pOutSyll] = 3; fParams[pOutMouth] = 0.9f; fParams[pOutVowel] = 0; }
        if (std::getenv("RETRO_SCENES")) fScenesOpen = true;
        if (std::getenv("RETRO_STORYMENU")) fStoryMenu = true;
        if (const char* e = std::getenv("RETRO_STORY")) { fStoryForce = std::atoi(e); }
        if (std::getenv("RETRO_TYPING")) { fEditing = true; fEditBuf = "мы по-ём вме-сте до-ро-га"; }
        if (const char* e = std::getenv("RETRO_EXPORT")) { fExport = true; fExportGirl = std::strcmp(e, "girl") == 0; }
        if (const char* e = std::getenv("RETRO_FRAME")) { const int f = std::atoi(e); fForceBlink = f == 1 ? 1 : 0; fForceMouth = f == 2 ? 1 : 0; }
        if (const char* e = std::getenv("RETRO_BG")) fExportWhite = std::atoi(e) == 1;
        if (const char* e = std::getenv("RETRO_TAB")) fEraTab = std::atoi(e);
        if (std::getenv("RETRO_SMP")) { fSmpPath[0] = "C:\\Music\\vn_piano_C4.wav"; fSmpPath[2] = "/home/me/rpgxp/Audio/SE/bell.ogg"; fRoot[2] = 72; }
        if (const char* e = std::getenv("RETRO_INST")) { const float v = (float)std::atoi(e); parameterChanged(pInstrument, v); setParameterValue(pInstrument, v); fSwitchT = -10; fTypeT = -10; }
        if (const char* e = std::getenv("RETRO_ACT")) fParams[pOutActivity] = (float)std::atof(e);
        if (std::getenv("RETRO_DRUMS")) { fParams[pDrumMode] = 1; fParams[pDrumPattern] = 4; fParams[pOutDrumStep] = 6; }
       #endif
    }

protected:
    // ------------------------------------------------------------------ DSP -> UI
    void parameterChanged(uint32_t index, float value) override
    {
        if (index >= (uint32_t)kParamCount) return;
       #ifdef RETRO_SCREENSHOT
        if (fForceVocal && (index == pVocalMode || index == pIdol || index == pOutSyll || index == pOutMouth || index == pOutVowel)) return;
        if (index == pOutActivity && std::getenv("RETRO_ACT")) return;
        if ((index == pDrumMode || index == pDrumPattern || index == pOutDrumStep) && std::getenv("RETRO_DRUMS")) return;
       #endif
        if (index == pInstrument)
        {
            const int ni = clampi((int)std::lround(value), 0, kNumInstruments - 1);
            const int oi = clampi((int)std::lround(fParams[pInstrument]), 0, kNumInstruments - 1);
            if (ni != oi || !fInited) { fPrevChar = kFlavor[oi].ch; fPrevPixel = eraOf(oi) == eraPC98; fSwitchT = now(); fTypeT = now(); fEraTab = eraOf(ni); }
            fInited = true;
        }
        fParams[index] = value;
        repaint();
    }

    void uiIdle() override { repaint(); }

    // DSP state -> UI (user samples, roots, user drum patterns)
    void stateChanged(const char* key, const char* value) override
    {
        if (key && value && !std::strcmp(key, "lyrics")) { fLyrics = value; parseLyrics(value, fSyl); repaint(); return; }
        if (!key || !value || std::strlen(key) < 4) return;
        const int k = key[std::strlen(key) - 1] - '1';
        if (k < 0 || k > 3) return;
        if (!std::strncmp(key, "smp", 3)) fSmpPath[k] = value;
        else if (!std::strncmp(key, "root", 4)) fRoot[k] = clampi(std::atoi(value), 0, 127);
        else if (!std::strncmp(key, "pat", 3)) parsePattern(k, value);
        repaint();
    }

    // ------------------------------------------------------------------ drawing
    void onNanoDisplay() override
    {
        const float sc = getHeight() / kH;
        fT = now();
        save();
        scale(sc, sc);
       #ifdef RETRO_SCREENSHOT
        if (fStoryForce >= 0 && fStory < 0) { startLesson(fStoryForce / 10); fStoryStep = fStoryForce % 10; storyEnterStep(); fTypeT = -10; fStoryForce = -1; }
       #endif

        // "Live2D" look target: follow the cursor, otherwise wander softly
        {
            float tx, ty;
            if (fT - fMouseT < 4.f && fMouseX >= 0) { tx = (fMouseX - 480.f) / 380.f; ty = (fMouseY - 230.f) / 300.f; }
            else { tx = std::sin(fT * 0.31f) * 0.35f; ty = std::sin(fT * 0.23f) * 0.2f; }
            tx = std::fmax(-1.f, std::fmin(1.f, tx)); ty = std::fmax(-1.f, std::fmin(1.f, ty));
            const float dt = std::fmin(0.1f, std::fmax(0.001f, fT - fLastFrameT));
            const float k = 1.f - std::exp(-dt * 5.f);
            fLX += (tx - fLX) * k; fLY += (ty - fLY) * k;
            fLastFrameT = fT;
            if (fExport) { fLX = fLY = 0; fT = 0; }
           #ifdef RETRO_SCREENSHOT
            if (const char* e = std::getenv("RETRO_LOOK")) { fLX = (float)std::atof(e); fLY = 0.1f; }
           #endif
        }

        const int inst = clampi((int)std::lround(fParams[pInstrument]), 0, kNumInstruments - 1);
        const int ch = kFlavor[inst].ch;
        const CharacterDef& C = kChars[ch];

        // offline export used to build the PC-98 pixel art (tools/build_pixelart.py)
        if (fExport)
        {
            if (fExportGirl)
            {
                beginPath(); rect(0, 0, kW, kH); fillColor(fExportWhite ? Color(255, 255, 255) : Color(0, 0, 0)); fill();
                drawHeroine(C, 480.f, 1.f);
            }
            else drawScene(C, 1.f);
            restore();
            return;
        }
        const float sw = std::fmin(1.f, (fT - fSwitchT) / 0.45f);

        if (fParams[pVocalMode] > 0.5f)
        {
            drawVocalScreen(inst);
            restore();
            return;
        }
        fHeroU = 92.f; fHeroY = 214.f; fDrawingIdol = false;
        // PC-98 era = pixel art, Windows XP era = detailed 2000s style
        const bool pixel = eraOf(inst) == eraPC98;
        fPixelUI = pixel;
        const float ease = 1.f - (1.f - sw) * (1.f - sw);
        // background: crossfade scenes when the heroine (or era) changes
        drawWorldBg(fPrevChar, fPrevPixel, 1.f);
        if (sw < 1.f) { globalAlpha(sw); drawWorldBg(ch, pixel, sw); globalAlpha(1.f); }
        else drawWorldBg(ch, pixel, 1.f);
        // heroine
        if (sw < 1.f && (fPrevChar != ch || fPrevPixel != pixel)) {
            globalAlpha(1.f - ease);
            drawWorldGirl(fPrevChar, fPrevPixel, 480.f - 60.f * ease);
        }
        globalAlpha(ease);
        drawWorldGirl(ch, pixel, 480.f + 60.f * (1.f - ease));
        globalAlpha(1.f);
        if (pixel) drawScanlines();

        drawNotes(C);
        drawTopBar(C);
        drawInstrumentPanel();
        drawSoundPanel(C);
        drawTextbox(C, inst);
        drawDrumPanel(C);
        if (fEditor) drawEditor(C);
        drawOverlays();

        restore();
    }

    // ------------------------------------------------------------------ input
    bool onMouse(const MouseEvent& ev) override
    {
        const float sc = getHeight() / kH;
        const float px = ev.pos.getX() / sc, py = ev.pos.getY() / sc;

        if (!ev.press)
        {
            if (fDrag >= 0) { editParameter(fCtl[fDrag].param, false); fDrag = -1; return true; }
            return false;
        }
        // story mode box buttons come first
        if (fStory >= 0)
        {
            if (fStoryNext.hit(px, py)) { storyAdvance(); repaint(); return true; }
            if (fStoryExit.hit(px, py)) { fStory = -1; repaint(); return true; }
        }
        bool hitEdit = false;
        for (int i = 0; i < fCtlCount; ++i)
            if ((fCtl[i].type == ctLyricEdit || fCtl[i].type == ctLyricRU || fCtl[i].type == ctLyricPaste) && visible(fCtl[i]) && fCtl[i].b.hit(px, py)) hitEdit = true;
        if (fEditing && !hitEdit) commitLyrics();
        for (int i = 0; i < fCtlCount; ++i)
        {
            Control& c = fCtl[i];
            if (!visible(c) || !c.b.hit(px, py)) continue;
            const int cur = clampi((int)std::lround(fParams[pInstrument]), 0, kNumInstruments - 1);
            if (c.type == ctAction && fParams[pVocalMode] > 0.5f)
            {
                const int ci = clampi((int)std::lround(fParams[pIdol]), 0, kNumIdols - 1);
                int n = c.value == aPrev ? (ci + kNumIdols - 1) % kNumIdols : (c.value == aNext ? (ci + 1) % kNumIdols : (int)(hash01((int)(fT * 1000)) * kNumIdols) % kNumIdols);
                selectIdol(n);
                repaint();
                return true;
            }
            switch (c.type)
            {
            case ctIdol: selectIdol((int)c.value); break;
            case ctLyricPreset: fLyrics = kLyricPresets[(int)c.value][1]; setState("lyrics", fLyrics.c_str()); parseLyrics(fLyrics.c_str(), fSyl); fEditing = false; break;
            case ctLyricEdit:
                if (!fEditing) { fEditing = true; fEditBuf = fLyrics; getWindow().focus(); if (winLayoutIsCyrillic()) fAutoRu = true; }
                else if (c.param == 0) commitLyrics();
                break;
            case ctLyricRewind: setState("lyrics", fLyrics.c_str()); break;
            case ctLyricRU: fRuLayout = !fRuLayout; if (fEditing) getWindow().focus(); break;
            case ctLyricPaste: pasteLyrics(); break;
            case ctScenesOpen: fScenesOpen = true; break;
            case ctScenesClose: fScenesOpen = false; break;
            case ctSceneCard: applyScene((int)c.value); fScenesOpen = false; break;
            case ctSceneOff: setParam(pScene, 0.f); fScenesOpen = false; break;
            case ctStoryOpen: fStoryMenu = true; fStory = -1; break;
            case ctStoryMenuClose: fStoryMenu = false; break;
            case ctGenre:
            {
                const int g = (int)c.value;
                setParam(pDrumGenre, (float)g);
                setParam(pDrumKit, (float)kGenres[g].kit);
                setParam(pDrumPattern, (float)kGenres[g].pats[0]);
                fLastPreset = kGenres[g].pats[0];
                break;
            }
            case ctGenrePat:
            {
                const int g = clampi((int)std::lround(fParams[pDrumGenre]), 0, kNumGenres - 1);
                const int pt = kGenres[g].pats[(int)c.value];
                setParam(pDrumPattern, (float)pt); fLastPreset = pt;
                break;
            }
            case ctKitStep:
            {
                const int k = ((int)std::lround(fParams[pDrumKit]) + (int)c.value + kNumKits) % kNumKits;
                setParam(pDrumKit, (float)k);
                break;
            }
            case ctLesson: fStoryMenu = false; startLesson((int)c.value); break;
            case ctSelect:
                if (c.param == pDrumPattern && !isUserPattern((int)c.value)) fLastPreset = (int)c.value;
                if (c.param == pInstrument && fParams[pScene] > 0.5f) setParam(pScene, 0.f);   // picking an instrument leaves the scene
                setParam(c.param, c.value);
                break;
            case ctToggle: setParam(c.param, fParams[c.param] > 0.5f ? 0.f : 1.f); break;
            case ctAction:
            {
                const int lo = fEraTab == eraPC98 ? 0 : (fEraTab == eraXP ? kFirstXP : (fEraTab == eraRock ? kFirstRock : kFirstUser));
                const int cnt = fEraTab == eraPC98 ? kNumFM : (fEraTab == eraXP ? kNumXP : (fEraTab == eraRock ? kNumRock : kNumUser));
                int idx = (cur >= lo && cur < lo + cnt) ? cur - lo : 0;
                if (c.value == aPrev) idx = (idx + cnt - 1) % cnt;
                else if (c.value == aNext) idx = (idx + 1) % cnt;
                else { const int r = (int)(hash01((int)(fT * 1000)) * cnt) % cnt; idx = r == idx ? (r + 1) % cnt : r; }
                setParam(pInstrument, (float)(lo + idx));
                break;
            }
            case ctKnob:
                if (ev.button == kMouseButtonRight) { setParam(c.param, kParams[c.param].def); break; }
                fDrag = i; fDragY = py; fDragV = norm(c.param);
                editParameter(c.param, true);
                break;
            case ctEraTab: fEraTab = (int)c.value; break;
            case ctLoad:
            {
                char key[8]; std::snprintf(key, sizeof(key), "smp%d", (int)c.value + 1);
                setParam(pInstrument, (float)(kFirstUser + (int)c.value));
                requestStateFile(key);
                break;
            }
            case ctClearSmp:
            {
                char key[8]; std::snprintf(key, sizeof(key), "smp%d", (int)c.value + 1);
                fSmpPath[(int)c.value].clear(); setState(key, "");
                break;
            }
            case ctRootDn:
            case ctRootUp:
            {
                const int k = (int)c.value;
                fRoot[k] = clampi(fRoot[k] + (c.type == ctRootUp ? 1 : -1) * ((ev.mod & kModifierShift) ? 12 : 1), 0, 127);
                char key[8], val[8]; std::snprintf(key, sizeof(key), "root%d", k + 1); std::snprintf(val, sizeof(val), "%d", fRoot[k]);
                setState(key, val);
                break;
            }
            case ctEditOpen: { fEditor = true; const int cp = (int)std::lround(fParams[pDrumPattern]); if (isUserPattern(cp)) fEditUser = cp - kFirstUserPattern; else fLastPreset = cp; break; }
            case ctEditClose: fEditor = false; break;
            case ctEditUser: fEditUser = (int)c.value; setParam(pDrumPattern, (float)(kFirstUserPattern + fEditUser)); break;
            case ctEditLen: fULen[fEditUser] = fULen[fEditUser] == 16 ? 12 : 16; commitPattern(fEditUser); break;
            case ctEditClear:
                for (int l = 0; l < kDrumLanes; ++l) std::memset(fUL[fEditUser][l], '-', 16);
                commitPattern(fEditUser); break;
            case ctEditCopy: copyPreset(fEditUser, fLastPreset); commitPattern(fEditUser); break;
            case ctCell:
            {
                const int lane = (int)c.value / 16, st = (int)c.value % 16;
                if (st >= fULen[fEditUser]) break;
                char& ch = fUL[fEditUser][lane][st];
                if (ev.button == kMouseButtonRight) ch = '-';
                else ch = ch == '-' ? 'x' : (ch == 'x' ? 'X' : (ch == 'X' ? 'g' : '-'));
                commitPattern(fEditUser);
                break;
            }
            }
            repaint();
            return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const float sc = getHeight() / kH;
        const float px = ev.pos.getX() / sc, py = ev.pos.getY() / sc;
        fMouseX = px; fMouseY = py; fMouseT = fT;
        if (fDrag >= 0)
        {
            const Control& c = fCtl[fDrag];
            const float fine = (ev.mod & kModifierShift) ? 0.2f : 1.f;
            float v = fDragV + (fDragY - py) / 180.f * fine;
            v = v < 0 ? 0 : (v > 1 ? 1 : v);
            const ParamInfo& p = kParams[c.param];
            float val = p.min + v * (p.max - p.min);
            if (p.integer) val = std::round(val);
            fParams[c.param] = val;
            setParameterValue(c.param, val);
            repaint();
            return true;
        }
        repaint();
        return false;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!ev.press) return fEditing;
        // Ctrl+V: on a Russian layout the host reports the key as 'м' (or nothing at all)
        const bool ctrl = (ev.mod & kModifierControl) || winCtrlDown();
        const bool vKey = ev.key == 'v' || ev.key == 'V' || ev.key == 0x43C || ev.key == 0x41C || ev.key == 22 /* ^V */ || winKeyVDown();
        if (ctrl && vKey && fParams[pVocalMode] > 0.5f)
        {
            if (fT - fLastPasteT > 0.25f) { pasteLyrics(); fLastPasteT = fT; }
            repaint(); return true;
        }
        if (!fEditing) return false;
        if (ev.key == kKeyBackspace) { while (!fEditBuf.empty() && (fEditBuf.back() & 0xC0) == 0x80) fEditBuf.pop_back(); if (!fEditBuf.empty()) fEditBuf.pop_back(); }
        else if (ev.key == kKeyEnter) commitLyrics();
        else if (ev.key == kKeyEscape) fEditing = false;
        else return true;   // printable keys arrive through onCharacterInput
        repaint();
        return true;
    }
    bool onCharacterInput(const CharacterInputEvent& ev) override
    {
        if (!fEditing) return false;
        if ((ev.mod & kModifierControl) || winCtrlDown()) return true;
        uint32_t cp = ev.character;
        // Windows: ask the system which character this key gives in the user's current layout
        if (!(ev.string[0] && (unsigned char)ev.string[0] >= 0x80)) cp = winTranslateTyped(cp);
        // hosts that forward keys (VST) give only the key code; the native window also gives UTF-8 text
        if (ev.string[0] && (unsigned char)ev.string[0] >= 0x80) { if (fEditBuf.size() < 900) fEditBuf += ev.string; repaint(); return true; }
        if (cp >= 32 && cp != 127 && fEditBuf.size() < 900)
        {
            if (fRuLayout && cp < 0x80) cp = ruKey(cp);
            appendUtf8(fEditBuf, cp);
        }
        repaint();
        return true;
    }

    // Latin key -> Cyrillic letter on the same key of the ЙЦУКЕН layout
    static uint32_t ruKey(uint32_t c)
    {
        static const char* const lat = "qwertyuiop[]asdfghjkl;'zxcvbnm,.`";
        static const uint32_t cyr[] = { 0x439,0x446,0x443,0x43A,0x435,0x43D,0x433,0x448,0x449,0x437,0x445,0x44A,
                                        0x444,0x44B,0x432,0x430,0x43F,0x440,0x43E,0x43B,0x434,0x436,0x44D,
                                        0x44F,0x447,0x441,0x43C,0x438,0x442,0x44C,0x431,0x44E,0x451 };
        const bool upper = c >= 'A' && c <= 'Z';
        const uint32_t lc = upper ? c + 32 : c;
        static const char* const latUp = "{}:\"<>~";
        static const uint32_t cyrUp[] = { 0x425, 0x42A, 0x416, 0x42D, 0x411, 0x42E, 0x401 };
        for (int i = 0; latUp[i]; ++i) if ((uint32_t)(unsigned char)latUp[i] == c) return cyrUp[i];
        for (int i = 0; lat[i]; ++i)
            if ((uint32_t)(unsigned char)lat[i] == lc)
            {
                uint32_t r = cyr[i];
                if (upper) r = (r == 0x451) ? 0x401 : r - 32;
                return r;
            }
        return c;
    }
    static void appendUtf8(std::string& s, uint32_t cp)
    {
        if (cp < 0x80) s += (char)cp;
        else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 63)); }
        else { s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 63)); s += (char)(0x80 | (cp & 63)); }
    }
    void pasteLyrics()
    {
        size_t sz = 0;
        const void* data = getClipboard(sz);
        if (!data || sz == 0) return;
        std::string t((const char*)data, sz);
        for (char& ch : t) if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
        while (!t.empty() && t.back() == 0) t.pop_back();
        if (fEditing) { fEditBuf += t; if (fEditBuf.size() > 900) fEditBuf.resize(900); }
        else { fEditBuf = t; if (fEditBuf.size() > 900) fEditBuf.resize(900); commitLyrics(); }
    }

    // ------------------------------------------------------------------ scenes
    void applyScene(int n)
    {
        if (n < 1 || n > kNumScenes) return;
        const ScenePreset& S = kScenes[n - 1];
        setParam(pVocalMode, 0.f);
        setParam(pInstrument, (float)S.part[0].inst);
        setParam(pScene, (float)n);
        setParam(pDrumMode, (float)S.drumMode); setParam(pDrumPattern, (float)S.drumPattern);
        setParam(pDrumKit, (float)S.drumKit); setParam(pDrumVolume, S.drumVol);
        for (int g = 0; g < kNumGenres; ++g) if (kGenres[g].kit == S.drumKit) { setParam(pDrumGenre, (float)g); break; }
        setParam(pReverb, S.reverb); setParam(pChorus, S.chorus); setParam(pLofi, S.lofi); setParam(pBrightness, S.bright);
        fEraTab = eraOf(S.part[0].inst);
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const float sc = getHeight() / kH;
        const float px = ev.pos.getX() / sc, py = ev.pos.getY() / sc;
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != ctKnob || !visible(c) || !c.b.hit(px, py)) continue;
            const ParamInfo& p = kParams[c.param];
            float v = norm(c.param) + (float)ev.delta.getY() * 0.03f;
            v = v < 0 ? 0 : (v > 1 ? 1 : v);
            float val = p.min + v * (p.max - p.min);
            if (p.integer) val = std::round(val);
            setParam(c.param, val);
            return true;
        }
        return false;
    }

private:
    // ================================================================== helpers
    static int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
    float now() const { return std::chrono::duration<float>(std::chrono::steady_clock::now() - fStart).count(); }
    float norm(int p) const { const ParamInfo& i = kParams[p]; return (fParams[p] - i.min) / (i.max - i.min); }
    void setParam(int p, float v)
    {
        editParameter(p, true);
        if (p == pInstrument) parameterChanged(p, v); else fParams[p] = v;
        setParameterValue(p, v);
        editParameter(p, false);
        repaint();
    }
    void addCtl(CtlType t, int param, float value, float x, float y, float w, float h, const char* label = nullptr, int group = gAlways)
    {
        if (fCtlCount >= kMaxCtl) return;
        Control& c = fCtl[fCtlCount++];
        c.type = t; c.param = param; c.value = value; c.b.x = x; c.b.y = y; c.b.w = w; c.b.h = h; c.label = label; c.group = group;
    }
    bool visible(const Control& c) const
    {
        if (fEditor) return c.group == gEditor;
        if (fScenesOpen) return c.group == gScenes;
        if (fStoryMenu) return c.group == gStoryMenu;
        if (c.group == gEditor || c.group == gScenes || c.group == gStoryMenu) return false;
        const bool vocal = fParams[pVocalMode] > 0.5f;
        if (c.group == gVocal) return vocal;
        if (c.group == gInst) return !vocal;
        if (c.group == gFM) return !vocal && fEraTab == eraPC98;
        if (c.group == gXP) return !vocal && fEraTab == eraXP;
        if (c.group == gUser) return !vocal && fEraTab == eraUser;
        if (c.group == gRock) return !vocal && fEraTab == eraRock;
        return true;
    }

    void selectIdol(int n)
    {
        const IdolVoice& V = kIdolVoices[n];
        setParam(pIdol, (float)n);
        setParam(pVGender, V.gender); setParam(pVFormant, V.formant); setParam(pVBreath, V.breath);
        setParam(pVVib, V.vib); setParam(pVVibRate, V.vibRate); setParam(pVBright, V.bright); setParam(pVGlide, V.glide);
        fTypeT = fT;
    }
    void commitLyrics()
    {
        fEditing = false;
        fLyrics = fEditBuf;
        parseLyrics(fLyrics.c_str(), fSyl);
        setState("lyrics", fLyrics.c_str());
    }

    // ---- user drum patterns (same text format as the DSP: 9 lanes joined by '|', then ';len')
    void copyPreset(int u, int pi)
    {
        if (isUserPattern(pi) || pi < 0 || pi >= kNumPatterns) pi = 0;
        const DrumPattern& P = kPatterns[presetSlot(pi)];
        for (int l = 0; l < kDrumLanes; ++l)
        {
            std::memset(fUL[u][l], '-', 16); fUL[u][l][16] = 0;
            for (int k = 0; k < P.len && P.lane[l][k]; ++k) fUL[u][l][k] = P.lane[l][k];
        }
        fULen[u] = P.len;
    }
    void parsePattern(int u, const char* str)
    {
        int lane = 0, k = 0;
        for (int l = 0; l < kDrumLanes; ++l) { std::memset(fUL[u][l], '-', 16); fUL[u][l][16] = 0; }
        const char* semi = std::strchr(str, ';');
        fULen[u] = (semi && std::atoi(semi + 1) == 12) ? 12 : 16;
        for (const char* c = str; *c && c != semi && lane < kDrumLanes; ++c)
        {
            if (*c == '|') { ++lane; k = 0; continue; }
            if (k < 16) fUL[u][lane][k++] = (*c == 'X' || *c == 'x' || *c == 'g') ? *c : '-';
        }
    }
    void commitPattern(int u)
    {
        char buf[256]; int n = 0;
        for (int l = 0; l < kDrumLanes; ++l) { for (int k = 0; k < fULen[u]; ++k) buf[n++] = fUL[u][l][k]; buf[n++] = '|'; }
        n += std::snprintf(buf + n, sizeof(buf) - n, ";%d", fULen[u]);
        char key[8]; std::snprintf(key, sizeof(key), "pat%d", u + 1);
        setState(key, buf);
        if ((int)std::lround(fParams[pDrumPattern]) != kFirstUserPattern + u) setParam(pDrumPattern, (float)(kFirstUserPattern + u));
    }
    char patStep(int pat, int lane, int st) const
    {
        if (!isUserPattern(pat)) { const DrumPattern& P = kPatterns[presetSlot(pat)]; return st < P.len ? P.lane[lane][st] : '-'; }
        const int u = pat - kFirstUserPattern;
        return st < fULen[u] ? fUL[u][lane][st] : '-';
    }
    int patLen(int pat) const { return !isUserPattern(pat) ? kPatterns[presetSlot(pat)].len : fULen[pat - kFirstUserPattern]; }
    static void noteName(int n, char* out, size_t sz)
    {
        static const char* const nm[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        std::snprintf(out, sz, "%s%d", nm[n % 12], n / 12 - 1);
    }
    bool hover(const Box& b) const { return b.hit(fMouseX, fMouseY); }

    void buildLayout()
    {
        // era tabs
        // era tabs (value = Era id): PC-98, WIN XP, ROCK, MY
        static const char* const tabs[4] = { "PC-98", "WIN XP", "ROCK \xE2\x9A\xA1", "MY \xE2\x99\xAA" };
        static const int tabEra[4] = { eraPC98, eraXP, eraRock, eraUser };
        for (int i = 0; i < 4; ++i) addCtl(ctEraTab, pInstrument, (float)tabEra[i], 24.f + i * 57.f, 82.f, 54.f, 22.f, tabs[i], gInst);
        // ROCK & battle list
        for (int i = 0; i < kNumRock; ++i)
            addCtl(ctSelect, pInstrument, (float)(kFirstRock + i), 26.f + (i % 2) * 117.f, 110.f + (i / 2) * 27.f, 112.f, 23.f, kInstrumentNames[kFirstRock + i], gRock);
        // PC-98 FM list
        for (int i = 0; i < kNumFM; ++i)
            addCtl(ctSelect, pInstrument, (float)i, 26.f + (i % 2) * 117.f, 110.f + (i / 2) * 27.f, 112.f, 23.f, kInstrumentNames[i], gFM);
        // Windows XP / 2000s list
        for (int i = 0; i < kNumXP; ++i)
            addCtl(ctSelect, pInstrument, (float)(kFirstXP + i), 26.f + (i % 2) * 117.f, 108.f + (i / 2) * 22.f, 112.f, 19.f, kInstrumentNames[kFirstXP + i], gXP);
        // user sample slots
        for (int k = 0; k < kNumUser; ++k)
        {
            const float y = 110.f + k * 82.f;
            addCtl(ctSelect,   pInstrument, (float)(kFirstUser + k), 26.f, y + 4, 224.f, 22.f, kInstrumentNames[kFirstUser + k], gUser);
            addCtl(ctLoad,     pInstrument, (float)k, 26.f,  y + 48, 62.f, 22.f, "LOAD", gUser);
            addCtl(ctClearSmp, pInstrument, (float)k, 92.f,  y + 48, 54.f, 22.f, "CLEAR", gUser);
            addCtl(ctRootDn,   pInstrument, (float)k, 152.f, y + 48, 24.f, 22.f, "\xE2\x97\x80", gUser);
            addCtl(ctRootUp,   pInstrument, (float)k, 226.f, y + 48, 24.f, 22.f, "\xE2\x96\xB6", gUser);
        }
        // knobs (right panel)
        static const int knobs[8] = { pVolume, pBrightness, pRelease, pVibDepth, pLofi, pChorus, pReverb, pVelSens };
        static const char* const klab[8] = { "VOLUME", "BRIGHT", "RELEASE", "VIBRATO", "LO-FI", "CHORUS", "REVERB", "VELOCITY" };
        for (int i = 0; i < 8; ++i)
            addCtl(ctKnob, knobs[i], 0, 716.f + (i % 2) * 114.f, 84.f + (i / 2) * 88.f, 104.f, 84.f, klab[i], gInst);
        // vocal mode: voice knobs, idol cards, lyrics controls
        static const int vknobs[8] = { pVGender, pVFormant, pVBreath, pVVib, pVVibRate, pVBright, pVGlide, pVolume };
        static const char* const vlab[8] = { "GENDER", "VOICE SIZE", "BREATH", "VIBRATO", "VIB SPEED", "BRIGHT", "GLIDE", "VOLUME" };
        for (int i = 0; i < 8; ++i)
            addCtl(ctKnob, vknobs[i], 0, 716.f + (i % 2) * 114.f, 84.f + (i / 2) * 88.f, 104.f, 84.f, vlab[i], gVocal);
        for (int i = 0; i < kNumIdols; ++i)
            addCtl(ctIdol, pIdol, (float)i, 26.f, 84.f + i * 58.f, 224.f, 54.f, kIdolVoices[i].name, gVocal);
        for (int i = 0; i < 6; ++i)
            addCtl(ctLyricPreset, 0, (float)i, 30.f + i * 92.f, 506.f, 88.f, 20.f, kLyricPresets[i][0], gVocal);
        addCtl(ctLyricEdit,   0, 0, 586.f, 506.f, 92.f, 20.f, "EDIT \xE2\x9C\x8E", gVocal);
        addCtl(ctLyricRewind, 0, 0, 682.f, 506.f, 80.f, 20.f, "\xE2\x9F\xB2 START", gVocal);
        addCtl(ctLyricEdit,   1, 0, 30.f, 474.f, 900.f, 28.f, nullptr, gVocal);   // click on the lyric line = edit
        // top bar: vocal mode switch
        addCtl(ctToggle, pVocalMode, 0, 250.f, 10.f, 92.f, 26.f, "VOCAL", gAlways);
        addCtl(ctScenesOpen, pScene, 0, 348.f, 10.f, 92.f, 26.f, "SCENES", gAlways);
        addCtl(ctStoryOpen, 0, 0, 446.f, 10.f, 92.f, 26.f, "STORY", gAlways);
        addCtl(ctLyricRU,    0, 0, 768.f, 506.f, 52.f, 20.f, "RU", gVocal);
        addCtl(ctLyricPaste, 0, 0, 824.f, 506.f, 78.f, 20.f, "PASTE", gVocal);
        // scene browser
        for (int i = 0; i < kNumScenes; ++i)
            addCtl(ctSceneCard, pScene, (float)(i + 1), 84.f + (i % 4) * 200.f, 112.f + (i / 4) * 56.f, 194.f, 52.f, kScenes[i].name, gScenes);
        addCtl(ctSceneOff,    pScene, 0, 610.f, 74.f, 160.f, 26.f, "SCENE OFF", gScenes);
        addCtl(ctScenesClose, 0, 0, 846.f, 72.f, 28.f, 28.f, "\xE2\x9C\x95", gScenes);
        // story menu
        for (int i = 0; i < kNumLessons; ++i)
            addCtl(ctLesson, 0, (float)i, 150.f, 118.f + i * 66.f, 660.f, 60.f, nullptr, gStoryMenu);
        addCtl(ctStoryMenuClose, 0, 0, 786.f, 72.f, 28.f, 28.f, "\xE2\x9C\x95", gStoryMenu);
        // top bar actions
        addCtl(ctAction, pInstrument, aPrev,   640.f, 10.f, 40.f, 26.f, "\xE2\x97\x80");
        addCtl(ctAction, pInstrument, aNext,   684.f, 10.f, 40.f, 26.f, "\xE2\x96\xB6");
        addCtl(ctAction, pInstrument, aRandom, 728.f, 10.f, 104.f, 26.f, "RANDOM");
        // drums
        static const char* const modes[3] = { "OFF", "SYNC", "ALWAYS" };
        for (int i = 0; i < 3; ++i) addCtl(ctSelect, pDrumMode, (float)i, 100.f + i * 56.f, 552.f, 52.f, 24.f, modes[i]);
        addCtl(ctKitStep, pDrumKit, -1.f, 278.f, 552.f, 26.f, 24.f, "\xE2\x97\x80");
        addCtl(ctKitStep, pDrumKit,  1.f, 420.f, 552.f, 26.f, 24.f, "\xE2\x96\xB6");
        addCtl(ctToggle, pDrumFill, 0, 456.f, 552.f, 60.f, 24.f, "FILLS");
        addCtl(ctEditOpen, pDrumPattern, 0, 522.f, 552.f, 62.f, 24.f, "EDIT \xE2\x9C\x8E");
        for (int g = 0; g < kNumGenres; ++g)
            addCtl(ctGenre, pDrumGenre, (float)g, 24.f + g * 62.f, 586.f, 59.f, 23.f, kGenres[g].name);
        for (int k = 0; k < 6; ++k)
            addCtl(ctGenrePat, pDrumPattern, (float)k, 24.f + k * 56.f, 614.f, 54.f, 23.f, nullptr);
        for (int u = 0; u < kNumUserPatterns; ++u)
            addCtl(ctSelect, pDrumPattern, (float)(kFirstUserPattern + u), 360.f + u * 56.f, 614.f, 54.f, 23.f, kUserPatternNames[u]);
        addCtl(ctKnob, pDrumVolume, 0, 868.f, 540.f, 72.f, 104.f, "DRUM VOL");
        // drum editor overlay
        addCtl(ctEditClose, 0, 0, 786.f, 68.f, 26.f, 26.f, "\xE2\x9C\x95", gEditor);
        for (int u = 0; u < 4; ++u) addCtl(ctEditUser, 0, (float)u, 160.f + u * 78.f, 100.f, 72.f, 24.f, kUserPatternNames[u], gEditor);
        addCtl(ctEditLen,   0, 0, 476.f, 100.f, 92.f, 24.f, "", gEditor);
        addCtl(ctEditClear, 0, 0, 574.f, 100.f, 62.f, 24.f, "CLEAR", gEditor);
        addCtl(ctEditCopy,  0, 0, 642.f, 100.f, 160.f, 24.f, "", gEditor);
        for (int l = 0; l < kDrumLanes; ++l)
            for (int st = 0; st < 16; ++st)
                addCtl(ctCell, 0, (float)(l * 16 + st), 238.f + st * 35.f, 140.f + l * 31.f, 33.f, 28.f, nullptr, gEditor);
    }

    void label(float x, float y, const char* s, float size, const Color& c, int align)
    {
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        fontSize(size);
        textAlign(align);
        fillColor(c);
        text(x, y, s, nullptr);
    }
    void outlinedLabel(float x, float y, const char* s, float size, const Color& fill, const Color& outline, int align, float ow = 1.5f)
    {
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        fontSize(size);
        textAlign(align);
        fillColor(outline);
        for (int k = 0; k < 8; ++k) text(x + std::cos(k * kPi / 4) * ow, y + std::sin(k * kPi / 4) * ow, s, nullptr);
        fillColor(fill);
        text(x, y, s, nullptr);
    }
    void glassPanel(float x, float y, float w, float h, uint32_t accent, float r = 14.f)
    {
        if (fPixelUI) r = 3.f;
        // drop shadow
        beginPath(); rect(x - 10, y - 6, w + 20, h + 22);
        fillPaint(boxGradient(x, y + 4, w, h, r, 14, Color(20, 10, 40, 0.28f), Color(0, 0, 0, 0)));
        fill();
        beginPath(); roundedRect(x, y, w, h, r);
        fillPaint(linearGradient(x, y, x, y + h, Color(255, 255, 255, 0.80f), Color(255, 244, 250, 0.68f)));
        fill();
        strokeWidth(2.f); strokeColor(hexc(accent, 0.75f)); stroke();
        beginPath(); roundedRect(x + 4, y + 4, w - 8, h - 8, r - 4);
        strokeWidth(1.f); strokeColor(Color(255, 255, 255, 0.9f)); stroke();
    }
    void panelTitle(float x, float y, const char* s, uint32_t accent)
    {
        beginPath(); roundedRect(x, y - 11, 10, 22, 3); fillColor(hexc(accent)); fill();
        label(x + 18, y + 6, s, 15.f, Color(70, 40, 90), ALIGN_LEFT | ALIGN_BASELINE);
    }

    // ================================================================== scene

    // ================================================================== Story Mode (interactive lessons)
    enum CondKind { ckNext, ckParamEq, ckParamGE, ckChanged, ckActivity, ckEraTab, ckEditorOpen, ckEditorClosed, ckScenesOpen, ckLyricsSakura };
    struct StoryStep { int heroine; const char* text; int tType, tParam; float tValue; int ck, cParam; float cValue; };
    struct Lesson { const char* title; const char* desc; int heroine; const StoryStep* steps; int count; };

    static const Lesson* lessons()
    {
        // heroine: 0..8 = kChars, 100+ = kIdols ; tType -1 = no target ; tValue -1 = any value
        static const StoryStep l1[] = {
            {0, "Привет! Я Хикари. Покажу, как тут всё устроено, — пара минут, не больше. Жми «Далее».", -1, 0, 0, ckNext, 0, 0},
            {0, "Слева вкладки эпох: PC-98 — 90-е, WIN XP — 2000-е, ROCK — битвы, MY — твои сэмплы. Открой PC-98.", ctEraTab, pInstrument, (float)eraPC98, ckEraTab, 0, (float)eraPC98},
            {0, "Выбери «FM E.Piano» — это мой инструмент!", ctSelect, pInstrument, 0, ckParamEq, pInstrument, 0},
            {0, "Сыграй пару нот: MIDI-клавиатурой или в piano roll твоей DAW. Я подожду~", -1, 0, 0, ckActivity, 0, 0},
            {0, "Ручка BRIGHT — звонкость. Потяни её мышкой вверх или вниз.", ctKnob, pBrightness, -1, ckChanged, pBrightness, 0},
            {0, "REVERB — эхо зала. Чуть больше — и звук как в школьном актовом зале.", ctKnob, pReverb, -1, ckChanged, pReverb, 0},
            {0, "LO-FI делает звук хрустящим, как из старого компьютера. Попробуй!", ctKnob, pLofi, -1, ckChanged, pLofi, 0},
            {0, "Подсказки: правый клик по ручке — сброс, колесо мыши — точная настройка, Shift — медленнее. Урок пройден!", -1, 0, 0, ckNext, 0, 0},
        };
        static const StoryStep l2[] = {
            {4, "Я Аканэ! Соберём трек в духе 90-х. В DAW сделай три дорожки с этим плагином: мелодия, аккорды и бас.", -1, 0, 0, ckNext, 0, 0},
            {4, "Это будет дорожка мелодии. Открой вкладку PC-98.", ctEraTab, pInstrument, (float)eraPC98, ckEraTab, 0, (float)eraPC98},
            {4, "Выбери «PC-98 Lead» — квадратная волна, как на старых компьютерах.", ctSelect, pInstrument, 6, ckParamEq, pInstrument, 6},
            {4, "Добавь LO-FI примерно на треть — так звучали звуковые чипы.", ctKnob, pLofi, -1, ckParamGE, pLofi, 0.25f},
            {4, "Немного CHORUS — для ширины звука.", ctKnob, pChorus, -1, ckChanged, pChorus, 0},
            {4, "Барабаны: включи SYNC — они будут играть вместе с DAW.", ctSelect, pDrumMode, 1, ckParamEq, pDrumMode, 1},
            {4, "Стиль барабанов VN/RPG — FM-барабаны PC-98, как в играх той эпохи.", ctGenre, pDrumGenre, 0, ckParamEq, pDrumKit, 0},
            {4, "Ритм «Town Theme» — типичный для RPG 90-х.", ctGenrePat, pDrumPattern, 1, ckParamEq, pDrumPattern, 2},
            {4, "На второй дорожке выбери «FM E.Piano» для аккордов, на третьей — «Synth Bass». Мелодию строй на нотах до-ре-ми-соль-ля — это пентатоника, звук сразу «японский».", -1, 0, 0, ckNext, 0, 0},
            {4, "Нажми Play в DAW! Все настройки сохранятся вместе с проектом. Вперёд, к приключениям!", -1, 0, 0, ckNext, 0, 0},
        };
        static const StoryStep l3[] = {
            {5, "Рин на связи. Сцены — это готовые наборы: инструменты, барабаны и эффекты для разных ситуаций.", -1, 0, 0, ckNext, 0, 0},
            {5, "Открой SCENES наверху.", ctScenesOpen, pScene, -1, ckScenesOpen, 0, 0},
            {5, "Выбери «Битва с боссом».", ctSceneCard, pScene, 16, ckParamEq, pScene, 16},
            {5, "Играй! Ниже C3 (левая рука) — бас-гитара, выше — гитара и струнные вместе. Барабаны уже на SYNC — нажми Play в DAW.", -1, 0, 0, ckActivity, 0, 0},
            {5, "Во вкладке ROCK есть ритмичные инструменты. Открой её.", ctEraTab, pInstrument, (float)eraRock, ckEraTab, 0, (float)eraRock},
            {5, "Выбери «Chug 16th» и зажми аккорд — гитара сама сыграет шестнадцатыми в темп песни.", ctSelect, pInstrument, (float)(kFirstRock + 5), ckParamEq, pInstrument, (float)(kFirstRock + 5)},
            {5, "Ещё попробуй «Battle Arp» и «Str Ostinato». Для кульминации — сцены «Кульминация» и «Финальный босс»!", -1, 0, 0, ckNext, 0, 0},
        };
        static const StoryStep l4[] = {
            {100, "Привет, я Сора! Включи VOCAL наверху.", ctToggle, pVocalMode, -1, ckParamEq, pVocalMode, 1},
            {100, "Выбери меня в списке айдолов слева.", ctIdol, pIdol, 0, ckParamEq, pIdol, 0},
            {100, "Нажми «Sakura» — это готовый текст.", ctLyricPreset, 0, 2, ckLyricsSakura, 0, 0},
            {100, "Сыграй мелодию: каждая нота — следующий слог. Перекрывай ноты — получится плавное легато.", -1, 0, 0, ckActivity, 0, 0},
            {100, "Свой текст: EDIT ✎ и набирай слоги через пробел. RU — русская раскладка, PASTE (или Ctrl+V) — вставить скопированный текст.", ctLyricEdit, 0, -1, ckNext, 0, 0},
            {100, "Ручка BREATH добавляет придыхание. Попробуй!", ctKnob, pVBreath, -1, ckChanged, pVBreath, 0},
            {100, "Готово! Текст начинается сначала каждый раз, когда ты жмёшь Play в DAW.", -1, 0, 0, ckNext, 0, 0},
        };
        static const StoryStep l5[] = {
            {5, "Сделаем свой ритм! Нажми EDIT ✎ у барабанов.", ctEditOpen, pDrumPattern, -1, ckEditorOpen, 0, 0},
            {5, "Кликай по клеткам: обычный удар → акцент (!) → тихий (·). Правый клик — стереть. Сделай пару ударов и жми «Далее».", -1, 0, 0, ckNext, 0, 0},
            {5, "COPY копирует выбранный готовый ритм — удобно начать с него и переделать.", ctEditCopy, 0, -1, ckNext, 0, 0},
            {5, "Закрой редактор. Твой паттерн уже выбран (User 1–4) и сохранится в проекте.", ctEditClose, 0, -1, ckEditorClosed, 0, 0},
        };
        static const Lesson L[] = {
            {"Первые шаги",        "эпохи, инструменты, главные ручки",     0, l1, (int)(sizeof(l1) / sizeof(l1[0]))},
            {"Трек в стиле 90-х",  "мелодия PC-98, lo-fi, барабаны, советы", 4, l2, (int)(sizeof(l2) / sizeof(l2[0]))},
            {"Сцены и битвы",      "готовые сцены, рок, ритмичные партии",   5, l3, (int)(sizeof(l3) / sizeof(l3[0]))},
            {"Поющие айдолы",      "вокал, тексты, голос",                   100, l4, (int)(sizeof(l4) / sizeof(l4[0]))},
            {"Свои барабаны",      "редактор паттернов",                     5, l5, (int)(sizeof(l5) / sizeof(l5[0]))},
        };
        return L;
    }

    void startLesson(int n)
    {
        fStory = n; fStoryStep = 0; storyEnterStep();
    }
    void storyEnterStep()
    {
        const StoryStep& S = lessons()[fStory].steps[fStoryStep];
        fStoryStartVal = S.cParam >= 0 && S.cParam < kParamCount ? fParams[S.cParam] : 0.f;
        fStoryT = fT; fStoryDoneT = -1;
        fTypeT = fT;
    }
    void storyAdvance()
    {
        if (fStory < 0) return;
        if (++fStoryStep >= lessons()[fStory].count) { fStory = -1; return; }
        storyEnterStep();
    }
    bool storyCondition(const StoryStep& S)
    {
        switch (S.ck)
        {
        case ckParamEq:  return std::fabs(fParams[S.cParam] - S.cValue) < 0.01f;
        case ckParamGE:  return fParams[S.cParam] >= S.cValue;
        case ckChanged:  return std::fabs(fParams[S.cParam] - fStoryStartVal) > 0.02f * (kParams[S.cParam].max - kParams[S.cParam].min);
        case ckActivity: return fParams[pOutActivity] > 0.08f || fParams[pOutMouth] > 0.1f;
        case ckEraTab:   return fEraTab == (int)S.cValue;
        case ckEditorOpen: return fEditor;
        case ckEditorClosed: return !fEditor;
        case ckScenesOpen: return fScenesOpen;
        case ckLyricsSakura: return fLyrics.find("sa ku ra") != std::string::npos;
        default: return false;
        }
    }
    const Control* findCtl(int type, int param, float value) const
    {
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != type) continue;
            if (type != ctEraTab && type != ctLyricEdit && c.param != param) continue;
            if (value >= 0 && std::fabs(c.value - value) > 0.01f) continue;
            if (!visible(c)) continue;
            return &c;
        }
        return nullptr;
    }

    void drawOverlays()
    {
        if (fScenesOpen) drawSceneBrowser();
        if (fStoryMenu) drawStoryMenu();
        if (fStory >= 0) drawStory();
    }

    void drawSceneBrowser()
    {
        beginPath(); rect(0, 0, kW, kH); fillColor(Color(16, 8, 36, 0.55f)); fill();
        fPixelUI = false;
        glassPanel(64, 60, 832, 470, 0xffb347, 16.f);
        fillColor(hexc(0xff8a3a)); drawFlower(86, 86, 9);
        label(102, 92, "SCENES", 17.f, Color(80, 40, 60), ALIGN_LEFT | ALIGN_BASELINE);
        label(190, 92, "готовые наборы для ситуаций: до 3 инструментов + барабаны + эффекты", 10.5f, Color(130, 90, 110), ALIGN_LEFT | ALIGN_BASELINE);
        const int cur = (int)std::lround(fParams[pScene]);
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.group != gScenes) continue;
            const bool hv = hover(c.b);
            if (c.type == ctSceneCard)
            {
                const ScenePreset& S = kScenes[(int)c.value - 1];
                const CharacterDef& H = kChars[kFlavor[S.part[0].inst].ch];
                const bool sel = (int)c.value == cur;
                beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 10);
                if (sel) fillPaint(linearGradient(c.b.x, 0, c.b.x + c.b.w, 0, hexc(H.accent), mixc(H.accent, 0x8a5cff, 0.5f)));
                else fillColor(hv ? Color(255, 236, 246) : Color(255, 255, 255, 0.88f));
                fill(); strokeWidth(1.f); strokeColor(hexc(H.accent, 0.7f)); stroke();
                beginPath(); roundedRect(c.b.x + 6, c.b.y + 6, 6, c.b.h - 12, 3); fillColor(hexc(H.accent)); fill();
                label(c.b.x + 18, c.b.y + 20, S.name, 12.f, sel ? Color(255, 255, 255) : Color(60, 40, 70), ALIGN_LEFT | ALIGN_BASELINE);
                label(c.b.x + 18, c.b.y + 36, S.desc, 9.f, sel ? Color(255, 255, 255, 0.9f) : Color(120, 90, 130), ALIGN_LEFT | ALIGN_BASELINE);
                label(c.b.x + c.b.w - 8, c.b.y + 46, S.category, 8.f, sel ? Color(255, 255, 255, 0.8f) : hexc(H.accent), ALIGN_RIGHT | ALIGN_BASELINE);
            }
            else if (c.type == ctSceneOff)
            {
                pill(c.b, cur == 0, hv, 0xff8a3a);
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, "SCENE OFF", 11.f, cur == 0 ? Color(255, 255, 255) : Color(80, 40, 60), ALIGN_CENTER | ALIGN_MIDDLE);
            }
            else if (c.type == ctScenesClose)
            {
                beginPath(); circle(c.b.x + 14, c.b.y + 14, 13); fillColor(hv ? Color(255, 120, 160) : Color(230, 180, 200)); fill();
                label(c.b.x + 14, c.b.y + 15, c.label, 14.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
            }
        }
        label(480, 520, "левая рука (ниже C3) обычно играет бас, правая — мелодию со слоями  •  барабаны включаются в режиме SYNC", 10.f, Color(120, 90, 130), ALIGN_CENTER | ALIGN_BASELINE);
    }

    const CharacterDef& storyHeroine(int h) const { return h >= 100 ? kIdols[h - 100] : kChars[h]; }

    void drawPortrait(const CharacterDef& H, float x, float y, float w, float h)
    {
        save();
        scissor(x, y, w, h);
        beginPath(); roundedRect(x, y, w, h, 10);
        fillPaint(linearGradient(0, y, 0, y + h, hexc(H.skyTop), hexc(H.skyBottom))); fill();
        const float oldU = fHeroU, oldY = fHeroY; const bool oldIdol = fDrawingIdol;
        fHeroU = h * 0.32f; fHeroY = y + h * 0.56f; fDrawingIdol = false;
        drawHeroine(H, x + w / 2, 1.f);
        fHeroU = oldU; fHeroY = oldY; fDrawingIdol = oldIdol;
        resetScissor();
        restore();
        beginPath(); roundedRect(x, y, w, h, 10); strokeWidth(2.f); strokeColor(hexc(H.accent)); stroke();
    }

    void drawStoryMenu()
    {
        beginPath(); rect(0, 0, kW, kH); fillColor(Color(16, 8, 36, 0.55f)); fill();
        fPixelUI = false;
        glassPanel(136, 60, 688, 470, 0xff5fa8, 16.f);
        fillColor(hexc(0xff5fa8)); drawFlower(158, 86, 9);
        label(174, 92, "STORY MODE", 17.f, Color(80, 40, 80), ALIGN_LEFT | ALIGN_BASELINE);
        label(310, 92, "мастер-класс: героини объясняют, как всё работает", 10.5f, Color(130, 90, 130), ALIGN_LEFT | ALIGN_BASELINE);
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.group != gStoryMenu) continue;
            const bool hv = hover(c.b);
            if (c.type == ctLesson)
            {
                const Lesson& L = lessons()[(int)c.value];
                const CharacterDef& H = storyHeroine(L.heroine);
                beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 12);
                fillColor(hv ? Color(255, 236, 246) : Color(255, 255, 255, 0.9f)); fill();
                strokeWidth(1.5f); strokeColor(hexc(H.accent, 0.8f)); stroke();
                drawPortrait(H, c.b.x + 6, c.b.y + 5, 70, c.b.h - 10);
                char t[96]; std::snprintf(t, sizeof(t), "Урок %d. %s", (int)c.value + 1, L.title);
                label(c.b.x + 92, c.b.y + 25, t, 15.f, Color(60, 40, 70), ALIGN_LEFT | ALIGN_BASELINE);
                char d[160]; std::snprintf(d, sizeof(d), "%s  \xE2\x80\xA2  ведёт %s  \xE2\x80\xA2  %d шагов", L.desc, H.name, L.count);
                label(c.b.x + 92, c.b.y + 45, d, 10.5f, Color(130, 100, 140), ALIGN_LEFT | ALIGN_BASELINE);
                label(c.b.x + c.b.w - 18, c.b.y + c.b.h / 2 + 5, "\xE2\x96\xB6", 16.f, hexc(H.accent), ALIGN_RIGHT | ALIGN_BASELINE);
            }
            else
            {
                beginPath(); circle(c.b.x + 14, c.b.y + 14, 13); fillColor(hv ? Color(255, 120, 160) : Color(230, 180, 200)); fill();
                label(c.b.x + 14, c.b.y + 15, c.label, 14.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
            }
        }
    }

    void drawStory()
    {
        const Lesson& L = lessons()[fStory];
        const StoryStep& S = L.steps[fStoryStep];
        const CharacterDef& H = storyHeroine(S.heroine);
        fPixelUI = false;
        // condition check -> short "great!" then next step
        const bool done = S.ck != ckNext && storyCondition(S);
        if (done && fStoryDoneT < 0) fStoryDoneT = fT;
        if (fStoryDoneT >= 0 && fT - fStoryDoneT > 0.9f) { storyAdvance(); if (fStory < 0) return; }
        const StoryStep& S2 = lessons()[fStory].steps[fStoryStep];
        // spotlight on the target control
        const Control* tc = S2.tType >= 0 ? findCtl(S2.tType, S2.tParam, S2.tValue) : nullptr;
        Box hb; bool hasT = false;
        if (tc) { hb = tc->b; hasT = true; }
        if (hasT)
        {
            const float pad = 6;
            beginPath(); rect(0, 0, kW, kH);
            roundedRect(hb.x - pad, hb.y - pad, hb.w + pad * 2, hb.h + pad * 2, 10); pathWinding(CW);
            fillColor(Color(10, 6, 26, 0.45f)); fill();
            const float pulse = 0.5f + 0.5f * std::sin(fT * 6.f);
            beginPath(); roundedRect(hb.x - pad, hb.y - pad, hb.w + pad * 2, hb.h + pad * 2, 10);
            strokeWidth(3.f + pulse * 2.f); strokeColor(hexc(H.accent, 0.6f + 0.4f * pulse)); stroke();
        }
        // dialogue box: above or below the target so it never covers it
        const float bw = 600, bh = 128;
        float by = 300;
        if (hasT && hb.y > 250) by = 70;
        const float bx = 180;
        beginPath(); roundedRect(bx, by, bw, bh, 14);
        fillPaint(linearGradient(0, by, 0, by + bh, Color(26, 18, 56, 0.96f), Color(52, 26, 84, 0.96f))); fill();
        strokeWidth(2.f); strokeColor(hexc(H.accent)); stroke();
        drawPortrait(H, bx + 10, by + 10, 96, bh - 20);
        char hdr[128]; std::snprintf(hdr, sizeof(hdr), "%s  \xE2\x80\xA2  %s  (%d/%d)", H.name, L.title, fStoryStep + 1, L.count);
        label(bx + 118, by + 26, hdr, 13.f, hexc(H.accent), ALIGN_LEFT | ALIGN_BASELINE);
        const int total = utf8len(S2.text), shown = std::min(total, (int)((fT - fTypeT) * 45.f));
        fontFace(NANOVG_DEJAVU_SANS_TTF); fontSize(14.5f); textAlign(ALIGN_LEFT | ALIGN_TOP);
        fillColor(Color(255, 255, 255)); textBox(bx + 118, by + 36, bw - 134, S2.text, utf8advance(S2.text, shown));
        // buttons
        fStoryNext.x = bx + bw - 128; fStoryNext.y = by + bh - 32; fStoryNext.w = 116; fStoryNext.h = 24;
        fStoryExit.x = bx + bw - 34; fStoryExit.y = by + 6; fStoryExit.w = 26; fStoryExit.h = 22;
        const bool waiting = S2.ck != ckNext && fStoryDoneT < 0;
        beginPath(); roundedRect(fStoryNext.x, fStoryNext.y, fStoryNext.w, fStoryNext.h, 12);
        fillColor(waiting ? Color(255, 255, 255, hover(fStoryNext) ? 0.25f : 0.12f) : hexc(H.accent, hover(fStoryNext) ? 1.f : 0.85f)); fill();
        const bool last = fStoryStep + 1 >= L.count;
        label(fStoryNext.x + fStoryNext.w / 2, fStoryNext.y + 13, fStoryDoneT >= 0 ? "Отлично! \xE2\x9C\x93" : (waiting ? "пропустить \xE2\x96\xB7" : (last ? "Готово \xE2\x9C\x93" : "Далее \xE2\x96\xB6")), 11.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
        if (waiting) label(bx + 118, by + bh - 16, "жду, пока ты сделаешь это \xE2\x86\x92", 10.f, Color(255, 255, 255, 0.55f + 0.3f * std::sin(fT * 4.f)), ALIGN_LEFT | ALIGN_BASELINE);
        label(fStoryExit.x + 13, fStoryExit.y + 12, "\xE2\x9C\x95", 13.f, Color(255, 255, 255, hover(fStoryExit) ? 1.f : 0.6f), ALIGN_CENTER | ALIGN_MIDDLE);
        // arrow from the box to the target
        if (hasT)
        {
            const float ax = hb.x + hb.w / 2, ay = by > hb.y ? hb.y + hb.h + 12 : hb.y - 12;
            const float bob = std::sin(fT * 5.f) * 4.f;
            const float dir = by > hb.y ? 1.f : -1.f;
            beginPath(); moveTo(ax, ay + bob * dir); lineTo(ax - 10, ay + dir * 16 + bob * dir); lineTo(ax + 10, ay + dir * 16 + bob * dir); closePath();
            fillColor(hexc(H.accent)); fill();
        }
    }

    // ================================================================== vocal stage
    void drawStage(const CharacterDef& I)
    {
        const float beat = fParams[pOutBeat], sing = fParams[pOutMouth];
        beginPath(); rect(-40, -40, kW + 80, kH + 80);
        fillPaint(linearGradient(0, 0, 0, 500, hexc(0x07061a), hexc(0x2a1048))); fill();
        // LED wall (dot matrix) reacting to the beat and the voice
        save(); layer(6.f);
        beginPath(); rect(150, 52, 660, 262); fillColor(Color(8, 6, 20)); fill();
        for (int lvl = 0; lvl < 4; ++lvl)
        {
            beginPath();
            for (int gy = 0; gy < 13; ++gy)
                for (int gx = 0; gx < 33; ++gx)
                {
                    const float u = gx / 33.f, v = gy / 13.f;
                    float w = 0.3f + 0.3f * std::sin(u * 9.f + fT * 1.7f) * std::cos(v * 6.f - fT * 1.1f);
                    const float bar = std::fabs(std::sin(gx * 0.74f + fT * 2.f)) * (0.25f + 0.55f * sing) + beat * 0.25f;
                    if (1.f - v < bar) w = std::fmax(w, 1.f - v > bar - 0.05f ? 0.95f : 0.7f);
                    const int L = (int)(w * 4.f);
                    if (std::min(3, L) == lvl) rect(153 + gx * 20.f, 55 + gy * 20.f, 15, 15);
                }
            static const float al[4] = { 0.06f, 0.2f, 0.55f, 0.95f };
            fillColor(lvl == 3 ? mixc(I.accent, 0xffffff, 0.6f) : hexc(I.accent, al[lvl])); fill();
        }
        // stage name on the wall
        outlinedLabel(480, 76, "LIVE  \xE2\x98\x85  RETRO NOVEL", 16.f, Color(255, 255, 255, 0.9f), hexc(I.accent, 0.6f), ALIGN_CENTER | ALIGN_BASELINE);
        restore();
        // truss with lamps
        save(); layer(10.f);
        beginPath(); rect(-40, 30, kW + 80, 14); fillColor(Color(40, 40, 56)); fill();
        for (int k = 0; k < 12; ++k)
        {
            const float x = 40 + k * 80.f;
            beginPath(); circle(x, 46, 9); fillColor(Color(70, 70, 90)); fill();
            beginPath(); circle(x, 48, 5); fillColor(k % 2 ? hexc(I.accent) : Color(255, 240, 200)); fill();
        }
        restore();
        // moving spotlights
        for (int k = 0; k < 5; ++k)
        {
            const float ox = 120 + k * 180.f, a = std::sin(fT * (0.5f + k * 0.13f) + k) * 0.45f;
            const float ex = ox + std::sin(a) * 520, ey = 46 + std::cos(a) * 520;
            beginPath(); moveTo(ox - 8, 46); lineTo(ox + 8, 46); lineTo(ex + 110, ey); lineTo(ex - 110, ey); closePath();
            fillPaint(linearGradient(ox, 46, ex, ey, k % 2 ? hexc(I.accent, 0.2f + beat * 0.15f) : Color(255, 245, 220, 0.12f + beat * 0.1f), Color(255, 255, 255, 0.f)));
            fill();
        }
        // glossy stage floor with reflection
        save(); layer(16.f);
        beginPath(); rect(-60, 330, kW + 120, 140);
        fillPaint(linearGradient(0, 330, 0, 470, Color(40, 30, 70), Color(10, 8, 24))); fill();
        beginPath(); rect(150, 334, 660, 60); fillPaint(linearGradient(0, 334, 0, 394, hexc(I.accent, 0.25f), hexc(I.accent, 0.f))); fill();
        for (int k = 0; k < 16; ++k) { beginPath(); circle(30 + k * 60.f, 332, 3); fillColor(hexc(k % 2 ? I.accent : 0xffffff, 0.8f)); fill(); }
        restore();
        // crowd with glow sticks
        save(); layer(30.f);
        for (int row = 0; row < 3; ++row)
            for (int k = 0; k < 18; ++k)
            {
                const float x = -20 + k * 58.f + (row % 2) * 29.f + hash01(k + row * 50) * 10;
                const float y = 470 + row * 55.f;
                const float sway = std::sin(fT * 2.2f + k * 0.7f + row) * 0.35f + beat * 0.25f * ((k % 2) ? 1 : -1);
                // glow stick
                save(); translate(x + 14, y - 8); rotate(sway);
                beginPath(); roundedRect(-3, -46, 6, 40, 3);
                fillColor(hexc((k + row) % 3 == 0 ? 0xffffff : I.accent, 0.95f)); fill();
                beginPath(); roundedRect(-8, -52, 16, 52, 8); fillColor(hexc(I.accent, 0.18f)); fill();
                restore();
                beginPath(); ellipse(x, y, 22, 26); fillColor(Color(16, 12, 30)); fill();
                beginPath(); ellipse(x, y + 40, 34, 26); fill();
            }
        restore();
        // floating sparkles
        for (int i = 0; i < 30; ++i)
        {
            const float x = std::fmod(hash01(i + 700) * kW + fT * 10.f, kW), y = std::fmod(hash01(i + 800) * 420 + fT * (8 + i % 5), 420.f);
            const float sz = 1.5f + hash01(i) * 2.5f, tw = 0.5f + 0.5f * std::sin(fT * 4 + i);
            beginPath(); circle(x, y, sz); fillColor(Color(255, 255, 255, 0.6f * tw)); fill();
        }
    }

    void drawVocalScreen(int inst)
    {
        const int idol = clampi((int)std::lround(fParams[pIdol]), 0, kNumIdols - 1);
        if (idol != fIdolShown) { fPrevIdol = fIdolShown < 0 ? idol : fIdolShown; fIdolShown = idol; fIdolSwitchT = fT; }
        const CharacterDef& I = kIdols[idol];
        fPixelUI = false;
        drawStage(I);
        fHeroU = 100.f; fHeroY = 206.f; fDrawingIdol = true;
        const float sw = std::fmin(1.f, (fT - fIdolSwitchT) / 0.45f), ease = 1.f - (1.f - sw) * (1.f - sw);
        if (sw < 1.f && fPrevIdol != idol) { globalAlpha(1.f - ease); drawHeroine(kIdols[fPrevIdol], 480.f - 60.f * ease, 1.f); }
        globalAlpha(ease); drawHeroine(I, 480.f + 60.f * (1.f - ease), 1.f); globalAlpha(1.f);
        fDrawingIdol = false;
        drawNotes(I);
        drawTopBar(I);
        drawIdolPanel(I);
        drawSoundPanel(I);
        drawLyricsBox(I);
        drawDrumPanel(I);
        if (fEditor) drawEditor(I);
        drawOverlays();
        (void)inst;
    }

    void drawIdolPanel(const CharacterDef& I)
    {
        glassPanel(16, 56, 244, 390, I.accent);
        panelTitle(28, 72, "IDOLS", I.accent);
        const int cur = clampi((int)std::lround(fParams[pIdol]), 0, kNumIdols - 1);
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != ctIdol) continue;
            const int k = (int)c.value;
            const CharacterDef& D = kIdols[k];
            const bool sel = k == cur;
            pill(c.b, sel, hover(c.b), D.accent, 14.f);
            // tiny portrait: hair colour circle with eyes
            beginPath(); circle(c.b.x + 27, c.b.y + 27, 20); fillColor(hexc(D.hair)); fill(); strokeWidth(1.5f); strokeColor(hexc(D.hairDark)); stroke();
            beginPath(); ellipse(c.b.x + 27, c.b.y + 32, 13, 12); fillColor(Color(255, 236, 224)); fill();
            beginPath(); ellipse(c.b.x + 22, c.b.y + 32, 2.5f, 3.5f); ellipse(c.b.x + 32, c.b.y + 32, 2.5f, 3.5f); fillColor(hexc(D.eye)); fill();
            char nm[64]; std::snprintf(nm, sizeof(nm), "%s %s", D.name, D.male ? "\xE2\x99\x82" : "\xE2\x99\x80");
            label(c.b.x + 56, c.b.y + 24, nm, 15.f, sel ? Color(255, 255, 255) : Color(60, 40, 80), ALIGN_LEFT | ALIGN_BASELINE);
            label(c.b.x + 56, c.b.y + 42, D.role, 10.5f, sel ? Color(255, 255, 255, 0.9f) : Color(120, 90, 140), ALIGN_LEFT | ALIGN_BASELINE);
        }
    }

    void drawLyricsBox(const CharacterDef& I)
    {
        const float x = 16, y = 456, w = 928, h = 76;
        beginPath(); roundedRect(x, y, w, h, 12);
        fillPaint(linearGradient(0, y, 0, y + h, Color(20, 14, 46, 0.92f), Color(44, 20, 70, 0.92f))); fill();
        strokeWidth(2.f); strokeColor(hexc(I.accent, 0.9f)); stroke();
        // name plate
        beginPath(); roundedRect(x + 18, y - 18, 240, 30, 10);
        fillPaint(linearGradient(x + 18, 0, x + 258, 0, hexc(I.accent), mixc(I.accent, 0xff5fa8, 0.5f))); fill();
        strokeWidth(2.f); strokeColor(Color(255, 255, 255)); stroke();
        char nm[96]; std::snprintf(nm, sizeof(nm), "%s \xE2\x99\xAA %s", I.name, I.role);
        outlinedLabel(x + 32, y + 3, nm, 15.f, Color(255, 255, 255), hexc(I.accent, 0.6f), ALIGN_LEFT | ALIGN_BASELINE, 1.2f);
        label(x + w - 22, y + 14, "1 note = 1 syllable  \xE2\x80\xA2  \"-\" = hold", 10.f, Color(255, 255, 255, 0.6f), ALIGN_RIGHT | ALIGN_BASELINE);

        fontFace(NANOVG_DEJAVU_SANS_TTF); fontSize(19.f); textAlign(ALIGN_LEFT | ALIGN_BASELINE);
        if (fEditing)
        {
            beginPath(); roundedRect(x + 12, y + 16, w - 24, 30, 6); fillColor(Color(255, 255, 255, 0.12f)); fill();
            strokeWidth(1.5f); strokeColor(hexc(I.accent)); stroke();
            // show the tail of the text so the caret stays visible
            const char* tx = fEditBuf.c_str();
            Rectangle<float> bnd;
            if (textBounds(0, 0, tx, nullptr, bnd) > w - 70)
            {
                // binary search the first byte offset whose tail fits
                size_t lo = 0, hi = fEditBuf.size();
                while (lo < hi)
                {
                    size_t mid = (lo + hi) / 2;
                    while (mid < fEditBuf.size() && (fEditBuf[mid] & 0xC0) == 0x80) ++mid;
                    if (mid >= hi) { hi = mid; break; }
                    if (textBounds(0, 0, fEditBuf.c_str() + mid, nullptr, bnd) > w - 70) lo = mid + 1; else hi = mid;
                }
                while (hi < fEditBuf.size() && (fEditBuf[hi] & 0xC0) == 0x80) ++hi;
                tx = fEditBuf.c_str() + hi;
            }
            fillColor(Color(255, 255, 255));
            const float ex = text(x + 22, y + 38, tx, nullptr);
            if (std::fmod(fT, 1.f) < 0.55f) { beginPath(); rect(ex + 2, y + 21, 2, 21); fillColor(hexc(I.accent)); fill(); }
        }
        else
        {
            // karaoke line: syllables around the current one
            const int cur = (int)std::lround(fParams[pOutSyll]);
            const int n = (int)fSyl.size();
            int start = 0;
            if (cur > 4) start = cur - 4;
            float px = x + 24;
            for (int k = start; k < n && px < x + w - 40; ++k)
            {
                const bool now = k == cur, past = cur >= 0 && k < cur;
                const char* t = fSyl[k].text;
                Rectangle<float> bnd; const float adv = textBounds(0, 0, t, nullptr, bnd);
                if (now)
                {
                    beginPath(); roundedRect(px - 5, y + 18, adv + 10, 28, 8); fillColor(hexc(I.accent, 0.85f)); fill();
                }
                fillColor(now ? Color(255, 255, 255) : (past ? Color(255, 255, 255, 0.4f) : Color(255, 255, 255, 0.9f)));
                text(px, y + 39, t, nullptr);
                px += adv + 14;
            }
            if (n == 0) { fillColor(Color(255, 255, 255, 0.6f)); text(x + 24, y + 39, "(click here and type lyrics)", nullptr); }
        }
        // preset chips + edit + restart
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.group != gVocal || c.b.h > 24.f || (c.type != ctLyricPreset && c.type != ctLyricEdit && c.type != ctLyricRewind && c.type != ctLyricRU && c.type != ctLyricPaste)) continue;
            const bool sel = (c.type == ctLyricEdit && fEditing) || (c.type == ctLyricRU && fRuLayout);
            beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 10);
            fillColor(sel ? hexc(I.accent) : (hover(c.b) ? Color(255, 255, 255, 0.3f) : Color(255, 255, 255, 0.14f))); fill();
            strokeWidth(1.f); strokeColor(hexc(I.accent, 0.8f)); stroke();
            const char* t = c.label;
            if (c.type == ctLyricEdit && fEditing) t = "OK \xE2\x86\xB5";
            if (c.type == ctLyricRU) t = fRuLayout ? "RU" : "EN";
            label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, t, 10.5f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
        }
    }

    // ================================================================== PC-98 pixel art
    NanoImage& pxImage(int c, int frame)   // frame -1 = background
    {
        NanoImage& img = frame < 0 ? fPxBgImg[c] : fPxGirlImg[c][frame];
        if (!img.isValid())
        {
            const PixelImage& P = frame < 0 ? kPxBg[c] : kPxGirl[c][frame];
            std::vector<uint8_t> rgba((size_t)P.w * P.h * 4);
            for (int i = 0; i < P.w * P.h; ++i)
            {
                const int v = (P.idx[i >> 1] >> ((i & 1) ? 0 : 4)) & 15;
                rgba[i * 4 + 0] = P.pal[v * 3 + 0]; rgba[i * 4 + 1] = P.pal[v * 3 + 1]; rgba[i * 4 + 2] = P.pal[v * 3 + 2];
                rgba[i * 4 + 3] = (P.transparent >= 0 && v == P.transparent) ? 0 : 255;
            }
            img = createImageFromRGBA((uint)P.w, (uint)P.h, rgba.data(), 1 << 5 /* NVG_IMAGE_NEAREST */);
        }
        return img;
    }
    void blit(NanoImage& img, float x, float y, float w, float h)
    {
        beginPath(); rect(x, y, w, h);
        fillPaint(imagePattern(x, y, w, h, 0.f, img, 1.f)); fill();
    }
    static float snap(float v) { return std::floor(v / kPixelScale + 0.5f) * kPixelScale; }

    void drawWorldBg(int c, bool pixel, float a)
    {
        if (!pixel) { drawScene(kChars[c], a); return; }
        blit(pxImage(c, -1), 0, 0, kW, kH);
        // pixel particles snapped to the 3px grid
        const CharacterDef& C = kChars[c];
        for (int i = 0; i < 40; ++i)
        {
            const float sp = 25.f + hash01(i + 900) * 35.f;
            const float x = snap(std::fmod(hash01(i + 1000) * (kW + 200) + fT * 22.f + std::sin(fT * 0.9f + i) * 30.f, kW + 200) - 100);
            const float y = snap(std::fmod(hash01(i + 1100) * (kH + 60) + fT * sp, kH + 60) - 30);
            beginPath();
            if (C.pt == ptSnow) { rect(x, y, 3, 3); fillColor(Color(255, 255, 255)); }
            else if (C.pt == ptSparkle) { rect(x, y - 3, 3, 9); rect(x - 3, y, 9, 3); fillColor(Color(255, 240, 170, std::sin(fT * 4 + i) > 0 ? 1.f : 0.5f)); }
            else { const bool wide = std::sin(fT * 2.f + i) > 0; rect(x, y, wide ? 6.f : 3.f, wide ? 3.f : 6.f); fillColor(hexc(C.particle)); }
            fill();
        }
    }
    void drawWorldGirl(int c, bool pixel, float cx)
    {
        if (!pixel) { drawHeroine(kChars[c], cx, 1.f); return; }
        const float act = fParams[pOutActivity], beat = fParams[pOutBeat];
        const float bt = std::fmod(fT + 0.37f * c, 4.3f);
        int frame = 0;
        if (bt > 4.15f || (act > 0.35f && std::fmod(fT, 2.5f) < 0.6f)) frame = 1;
        else if (act > 0.03f && std::sin(fT * 13.f) > 0.f) frame = 2;
        const PixelImage& P = kPxGirl[c][frame];
        const float x = snap(P.x * kPixelScale + (cx - 480.f) + fLX * 4.f);
        const float y = snap(P.y * kPixelScale + (std::sin(fT * 2.1f) > 0.6f ? -3.f : 0.f) + beat * beat * 6.f);
        blit(pxImage(c, frame), x, y, (float)P.w * kPixelScale, (float)P.h * kPixelScale);
    }
    void drawScanlines()
    {
        beginPath();
        for (int r = 0; r < (int)(kH / kPixelScale); ++r) rect(0, r * kPixelScale + 2, kW, 1);
        fillColor(Color(0, 0, 0, 0.07f)); fill();
    }

    // parallax helper: deeper layers move less when she (and the camera) turns
    void layer(float depth) { translate(-fLX * depth, -fLY * depth * 0.35f); }

    void cloud(const CharacterDef& C, float x, float y, float s, float al)
    {
        // shaded cumulus: shadow underside, body, sunlit top
        fillColor(mixc(C.skyBottom, 0x8890b0, 0.45f, al * 0.6f));
        beginPath(); ellipse(x + 4 * s, y + 10 * s, 78 * s, 20 * s); ellipse(x - 44 * s, y + 12 * s, 40 * s, 13 * s); fill();
        fillColor(Color(255, 255, 255, al));
        beginPath();
        ellipse(x, y, 72 * s, 22 * s); ellipse(x - 42 * s, y + 6 * s, 44 * s, 16 * s);
        ellipse(x + 44 * s, y + 4 * s, 50 * s, 18 * s); ellipse(x + 6 * s, y - 16 * s, 42 * s, 24 * s); ellipse(x - 22 * s, y - 10 * s, 28 * s, 18 * s);
        fill();
        fillColor(Color(255, 255, 255, al * 0.9f));
        beginPath(); ellipse(x + 2 * s, y - 22 * s, 26 * s, 10 * s); fill();
    }

    void drawScene(const CharacterDef& C, float /*a*/)
    {
        const bool night = C.ce == ceMoon || C.ce == ceRedMoon;
        // sky
        beginPath(); rect(-40, -40, kW + 80, kH + 80);
        fillPaint(linearGradient(0, 0, 0, 470, hexc(C.skyTop), hexc(C.skyBottom)));
        fill();
        // horizon haze
        beginPath(); rect(-40, 220, kW + 80, 200);
        fillPaint(linearGradient(0, 220, 0, 400, hexc(C.skyBottom, 0.f), mixc(C.skyBottom, 0xffffff, 0.35f, night ? 0.15f : 0.6f))); fill();

        save(); layer(4.f);
        // stars for night / sparkles for dusk
        if (night || C.pt == ptSparkle)
        {
            for (int i = 0; i < 90; ++i)
            {
                const float x = hash01(i * 3 + 1) * kW, y = hash01(i * 3 + 2) * 330.f;
                const float tw = fExport ? 1.f : 0.45f + 0.55f * std::sin(fT * (1.2f + hash01(i) * 2.f) + i);
                beginPath(); circle(x, y, 0.8f + hash01(i * 7) * 1.4f);
                fillColor(Color(255, 255, 240, (night ? 0.85f : 0.45f) * tw)); fill();
            }
        }
        // sun / moon with glow and light rays
        float sunX = -1, sunY = -1;
        if (C.ce == ceSun) { sunX = 800; sunY = 96; }
        else if (C.ce == ceSunset) { sunX = 250; sunY = 360; }
        if (sunX >= 0)
        {
            const float big = C.ce == ceSunset ? 1.6f : 1.f;
            // rays
            save(); translate(sunX, sunY); rotate(fExport ? 0.f : fT * 0.02f);
            for (int k = 0; k < 9; ++k)
            {
                const float a = k * 0.698f + hash01(k + 77) * 0.3f, w = 0.06f + hash01(k + 90) * 0.07f;
                beginPath(); moveTo(0, 0); lineTo(std::cos(a - w) * 900, std::sin(a - w) * 900); lineTo(std::cos(a + w) * 900, std::sin(a + w) * 900); closePath();
                fillPaint(radialGradient(0, 0, 40, 700, Color(255, 250, 220, C.ce == ceSunset ? 0.16f : 0.11f), Color(255, 250, 220, 0.f))); fill();
            }
            restore();
            beginPath(); circle(sunX, sunY, 170 * big);
            fillPaint(radialGradient(sunX, sunY, 20, 170 * big, C.ce == ceSunset ? Color(255, 225, 160, 0.9f) : Color(255, 255, 235, 0.95f), Color(255, 220, 170, 0.f))); fill();
            beginPath(); circle(sunX, sunY, C.ce == ceSunset ? 62.f : 34.f); fillColor(C.ce == ceSunset ? Color(255, 238, 195, 0.97f) : Color(255, 255, 245, 0.97f)); fill();
        }
        else if (C.ce == ceMoon)
        {
            beginPath(); circle(790, 110, 130);
            fillPaint(radialGradient(790, 110, 30, 130, Color(220, 230, 255, 0.5f), Color(200, 210, 255, 0.f))); fill();
            beginPath(); circle(790, 110, 38); fillPaint(radialGradient(780, 100, 5, 40, Color(255, 255, 245), Color(235, 232, 210))); fill();
            beginPath(); circle(774, 100, 34); fillColor(hexc(C.skyTop, 0.92f)); fill();
        }
        else if (C.ce == ceRedMoon)
        {
            beginPath(); circle(790, 120, 170);
            fillPaint(radialGradient(790, 120, 30, 170, Color(255, 60, 70, 0.45f), Color(160, 0, 30, 0.f))); fill();
            beginPath(); circle(790, 120, 46); fillPaint(radialGradient(780, 110, 5, 50, Color(255, 170, 160), Color(200, 30, 50))); fill();
            beginPath(); ellipse(800, 128, 9, 6); ellipse(774, 108, 6, 5); fillColor(Color(150, 20, 40, 0.45f)); fill();
        }
        restore();

        // clouds (shaded)
        if (!night)
        {
            save(); layer(8.f);
            const float al = C.ce == ceCloudy ? 0.85f : 0.75f;
            for (int i = 0; i < 6; ++i)
            {
                const float x = std::fmod(hash01(i + 40) * 1300.f + (fExport ? 0.f : fT) * (6.f + i * 2.f), 1300.f) - 170.f;
                const float y = 60.f + hash01(i + 60) * 200.f, sc2 = 0.7f + hash01(i + 80) * 0.8f;
                cloud(C, x, y, sc2, al);
            }
            restore();
        }

        // far hills
        save(); layer(10.f);
        beginPath(); moveTo(-40, 330);
        for (int k = 0; k <= 12; ++k) { const float x = -40 + k * 90.f; quadTo(x + 45, 285 + hash01(k + 300) * 40, x + 90, 320 + hash01(k + 320) * 15); }
        lineTo(kW + 60, 420); lineTo(-40, 420); closePath();
        fillColor(mixc(C.skyBottom, C.horizon, 0.55f, 0.75f)); fill();
        restore();

        // city: two-tone buildings with windows
        save(); layer(13.f);
        for (int i = 0; i < 24; ++i)
        {
            const float bw = 30 + hash01(i + 100) * 50, bh = 40 + hash01(i + 120) * 120;
            const float bx = i * 44.f - 40 + hash01(i + 140) * 10;
            const Color face = mixc(C.horizon, 0x2a2440, 0.32f + hash01(i) * 0.15f, 0.82f);
            const Color side = mixc(C.horizon, 0x2a2440, 0.5f + hash01(i) * 0.15f, 0.85f);
            beginPath(); rect(bx, 400 - bh, bw * 0.75f, bh + 80); fillColor(face); fill();
            beginPath(); rect(bx + bw * 0.75f, 400 - bh + 4, bw * 0.25f, bh + 80); fillColor(side); fill();
            beginPath(); rect(bx - 2, 400 - bh - 3, bw * 0.75f + 4, 4); fillColor(mixc(C.horizon, 0x2a2440, 0.55f, 0.85f)); fill();
            for (int w = 0; w < 10; ++w)
            {
                const float wx = bx + 5 + (w % 3) * (bw * 0.22f), wy = 400 - bh + 10 + (w / 3) * 15;
                if (wy > 395) break;
                const bool lit = night || C.ce == ceSunset ? hash01(i * 13 + w) > 0.45f : false;
                beginPath(); rect(wx, wy, 5, 7);
                fillColor(lit ? Color(255, 228, 150, 0.85f) : mixc(C.skyBottom, 0x40506a, 0.4f, 0.55f)); fill();
            }
        }
        restore();

        // utility poles and power lines - a staple of 2000s VN backgrounds
        save(); layer(18.f);
        {
            const Color wire = mixc(C.horizon, 0x101018, 0.75f, 0.85f);
            const float px1 = 118, px2 = 852;
            for (int pi = 0; pi < 2; ++pi)
            {
                const float px = pi ? px2 : px1;
                beginPath(); rect(px - 5, 40, 10, 300); fillColor(wire); fill();
                beginPath(); rect(px - 46, 70, 92, 6); rect(px - 34, 100, 68, 5); fill();
                for (int k = 0; k < 3; ++k) { beginPath(); rect(px - 40 + k * 38, 62, 5, 9); fill(); }
            }
            strokeColor(wire); strokeWidth(1.6f);
            static const float yy[4] = { 66, 66, 66, 98 };
            static const float xo[4] = { -40, -2, 36, -30 };
            for (int k = 0; k < 4; ++k)
            {
                const float y0 = yy[k];
                beginPath(); moveTo(px1 + xo[k] + 2, y0); bezierTo(px1 + 250, y0 + 70, px2 - 250, y0 + 70, px2 + xo[k] + 2, y0); stroke();
                beginPath(); moveTo(-60, y0 + 40); quadTo(40, y0 + 55, px1 + xo[k] + 2, y0); stroke();
                beginPath(); moveTo(px2 + xo[k] + 2, y0); quadTo(px2 + 70, y0 + 50, kW + 60, y0 + 38); stroke();
            }
            // a bird or two on the wire
            if (!night)
                for (int b = 0; b < 2; ++b)
                {
                    const float bx = 330 + b * 37, by = 66 + 70 * 0.75f * 0.75f + 2;
                    beginPath(); ellipse(bx, by - 5, 6, 4.5f); circle(bx + 5, by - 9, 3); fillColor(wire); fill();
                }
        }
        restore();

        // rooftop floor + fence
        save(); layer(22.f);
        beginPath(); rect(-60, 400, kW + 120, 300);
        fillPaint(linearGradient(0, 400, 0, 660, mixc(C.horizon, 0x6a6f88, 0.5f), mixc(C.horizon, 0x2d2a3c, 0.7f)));
        fill();
        strokeColor(mixc(C.horizon, 0x303040, 0.65f, 0.85f));
        strokeWidth(5.f);
        beginPath(); moveTo(-60, 318); lineTo(kW + 60, 318); stroke();
        beginPath(); moveTo(-60, 398); lineTo(kW + 60, 398); stroke();
        strokeWidth(2.5f);
        for (int i = -3; i < 52; ++i) { beginPath(); moveTo(i * 20.f, 318); lineTo(i * 20.f, 398); stroke(); }
        strokeWidth(1.f);
        strokeColor(mixc(C.horizon, 0x303040, 0.6f, 0.35f));
        for (int i = -2; i < 26; ++i) { beginPath(); moveTo(i * 40.f, 318); lineTo(i * 40.f + 40.f, 398); stroke(); }
        // fence highlight from the sky light
        strokeWidth(1.5f); strokeColor(mixc(C.skyBottom, 0xffffff, 0.4f, night ? 0.15f : 0.45f));
        beginPath(); moveTo(-60, 316); lineTo(kW + 60, 316); stroke();
        // street lamps glow at night
        if (night)
            for (int k = 0; k < 3; ++k)
            {
                const float lx2 = 200 + k * 290;
                beginPath(); circle(lx2, 300, 60);
                fillPaint(radialGradient(lx2, 300, 4, 60, Color(255, 220, 160, 0.55f), Color(255, 200, 140, 0.f))); fill();
                beginPath(); circle(lx2, 300, 5); fillColor(Color(255, 245, 220)); fill();
            }
        restore();

        // lens flare (daylight)
        if (C.ce == ceSun && !fExport)
        {
            const float cx2 = 480 - fLX * 30, cy2 = 330;
            for (int k = 1; k <= 4; ++k)
            {
                const float t = k / 5.f, fxp = sunX + (cx2 - sunX) * t * 1.4f, fyp = sunY + (cy2 - sunY) * t * 1.4f;
                beginPath(); circle(fxp, fyp, 10 + k * 7);
                fillColor(k % 2 ? Color(255, 230, 200, 0.08f) : Color(200, 230, 255, 0.07f)); fill();
            }
        }

        // trees (sakura / maple / snowy / green) in the corners
        save(); layer(30.f);
        drawBranch(C, -20.f, -10.f, 1.f);
        drawBranch(C, kW + 20.f, -10.f, -1.f);
        restore();

        if (fExport) return;
        // falling particles
        for (int i = 0; i < 46; ++i) drawParticle(C, i);

        // soft vignette
        beginPath(); rect(0, 0, kW, kH);
        fillPaint(radialGradient(kW / 2, kH / 2, 300, 720, Color(0, 0, 0, 0.f), Color(20, 10, 40, 0.35f)));
        fill();
    }

    void drawBranch(const CharacterDef& C, float x, float y, float dir)
    {
        save();
        translate(x, y);
        scale(dir, 1.f);
        const float sway = std::sin(fT * 0.8f) * 0.015f;
        rotate(sway);
        lineCap(ROUND);
        strokeColor(Color(74, 44, 40));
        strokeWidth(18.f); beginPath(); moveTo(0, 30); bezierTo(80, 50, 150, 80, 250, 70); stroke();
        strokeWidth(9.f);  beginPath(); moveTo(120, 62); bezierTo(150, 100, 170, 130, 215, 150); stroke();
        strokeWidth(7.f);  beginPath(); moveTo(190, 76); bezierTo(230, 40, 270, 30, 320, 34); stroke();
        strokeWidth(5.f);  beginPath(); moveTo(60, 42); bezierTo(80, 90, 70, 130, 90, 170); stroke();
        // blossoms / leaves
        for (int i = 0; i < 70; ++i)
        {
            const float t = hash01(i + 300);
            float bx, by;
            const int br = i % 4;
            if (br == 0)      { bx = 20 + t * 230;  by = 32 + t * 40; }
            else if (br == 1) { bx = 120 + t * 95;  by = 62 + t * 88; }
            else if (br == 2) { bx = 190 + t * 130; by = 76 - t * 42; }
            else              { bx = 60 + t * 30;   by = 42 + t * 128; }
            bx += (hash01(i + 400) - 0.5f) * 44; by += (hash01(i + 500) - 0.5f) * 36;
            const float r = 7 + hash01(i + 600) * 7;
            if (C.pt == ptSnow)
            {
                beginPath(); ellipse(bx, by, r * 1.3f, r * 0.8f); fillColor(Color(255, 255, 255, 0.95f)); fill();
            }
            else if (C.pt == ptMaple || C.pt == ptLeaf)
            {
                fillColor(mixc(C.particle, C.pt == ptMaple ? 0xffc040 : 0x4f9a40, hash01(i + 700) * 0.6f, 0.95f));
                drawLeaf(bx, by, r * 1.1f, hash01(i + 800) * 6.28f, C.pt == ptMaple);
            }
            else
            {
                fillColor(mixc(C.particle, 0xffffff, hash01(i + 700) * 0.5f, 0.95f));
                drawFlower(bx, by, r);
            }
        }
        restore();
    }

    void drawFlower(float x, float y, float r)
    {
        beginPath();
        for (int k = 0; k < 5; ++k)
        {
            const float a = k * 2 * kPi / 5;
            ellipse(x + std::cos(a) * r * 0.55f, y + std::sin(a) * r * 0.55f, r * 0.5f, r * 0.5f);
        }
        fill();
        beginPath(); circle(x, y, r * 0.22f); fillColor(Color(255, 120, 160, 0.9f)); fill();
    }

    void drawLeaf(float x, float y, float r, float rot, bool maple)
    {
        save(); translate(x, y); rotate(rot);
        beginPath();
        if (maple)
        {
            for (int k = 0; k < 5; ++k)
            {
                const float a = -kPi / 2 + (k - 2) * 0.62f;
                const float a2 = a + 0.31f;
                if (k == 0) moveTo(std::cos(a) * r, std::sin(a) * r); else lineTo(std::cos(a) * r, std::sin(a) * r);
                if (k < 4) lineTo(std::cos(a2) * r * 0.45f, std::sin(a2) * r * 0.45f);
            }
            lineTo(0, r * 0.3f);
            closePath();
        }
        else
        {
            moveTo(0, -r); quadTo(r * 0.7f, 0, 0, r); quadTo(-r * 0.7f, 0, 0, -r);
        }
        fill();
        restore();
    }

    void drawParticle(const CharacterDef& C, int i)
    {
        const float sp = 25.f + hash01(i + 900) * 35.f;
        const float drift = C.pt == ptSnow ? 6.f : 22.f;
        float x = std::fmod(hash01(i + 1000) * (kW + 200) + fT * drift + std::sin(fT * 0.9f + i) * 30.f, kW + 200) - 100;
        float y = std::fmod(hash01(i + 1100) * (kH + 60) + fT * sp, kH + 60) - 30;
        const float rot = fT * (1.f + hash01(i + 1200) * 2.f) + i;
        switch (C.pt)
        {
        case ptSnow:
            beginPath(); circle(x, y, 1.5f + hash01(i + 1300) * 3.f); fillColor(Color(255, 255, 255, 0.85f)); fill(); break;
        case ptMaple:
        case ptLeaf:
            fillColor(mixc(C.particle, C.pt == ptMaple ? 0xffc040 : 0xb6e08a, hash01(i) * 0.5f, 0.85f));
            drawLeaf(x, y, 6 + hash01(i + 1400) * 5, rot, C.pt == ptMaple); break;
        case ptSparkle:
        {
            const float s = 2 + hash01(i + 1500) * 3, tw = 0.5f + 0.5f * std::sin(fT * 4 + i);
            fillColor(Color(255, 245, 190, 0.8f * tw));
            beginPath(); moveTo(x, y - s * 2); lineTo(x + s * 0.5f, y); lineTo(x, y + s * 2); lineTo(x - s * 0.5f, y); closePath(); fill();
            beginPath(); moveTo(x - s * 2, y); lineTo(x, y + s * 0.5f); lineTo(x + s * 2, y); lineTo(x, y - s * 0.5f); closePath(); fill();
            break;
        }
        default:
            save(); translate(x, y); rotate(rot); scale(1.f, 0.55f + 0.45f * std::sin(rot * 1.7f));
            beginPath(); moveTo(0, -6); bezierTo(6, -6, 7, 3, 0, 7); bezierTo(-7, 3, -6, -6, 0, -6);
            fillColor(hexc(C.particle, 0.85f)); fill();
            restore();
        }
    }

    // ================================================================== heroine
    void drawHeroine(const CharacterDef& C, float cx, float /*appear*/)
    {
        const float act = fParams[pOutActivity];
        const float beat = fParams[pOutBeat];
        const float breathe = fExport ? 0.f : std::sin(fT * 2.1f) * 2.2f;
        const float bob = fExport ? 0.f : beat * beat * 5.f;
        const float u = fHeroU;             // pixels per head-radius
        const float cy = fHeroY + breathe * 0.4f + bob;
        const float lx = fLX, ly = fLY;    // where she looks (-1..1), drives the "Live2D" turn

        save();
        translate(cx, cy);
        scale(u, u);
        const float sway = fExport ? 0.f : std::sin(fT * 0.9f) * 0.012f + (act > 0.05f ? std::sin(fT * 3.f) * 0.02f * std::fmin(1.f, act * 2) : 0.f);
        fHairSway = fExport ? 0.f : std::sin(fT * 1.4f) * 0.035f - lx * 0.05f;

        // ---- back layer: hair behind body moves opposite to the face (depth)
        save(); translate(-lx * 0.05f, -ly * 0.02f);
        drawBackHair(C);
        restore();

        // ---- body
        save();
        translate(lx * 0.015f, breathe * 0.004f);
        drawBody(C);
        restore();

        // ---- head: turns towards the cursor
        save();
        translate(0, 0.7f); rotate(sway + lx * 0.035f); translate(0, -0.7f);
        translate(lx * 0.03f, ly * 0.02f);
        drawHairBehindHead(C);
        drawFace(C, act);
        drawFrontHair(C);
        drawAccessory(C);
        restore();

        // braids fall over the shoulders in front
        if (C.hs == hsBraids) { drawBraid(C, -1.f); drawBraid(C, 1.f); }

        restore();
    }

    void hairStroke(const CharacterDef& C) { strokeWidth(0.022f); strokeColor(hexc(C.hairDark, 0.9f)); stroke(); }
    Paint hairPaint(const CharacterDef& C, float y0, float y1) { return linearGradient(0, y0, 0, y1, hexc(C.hair), mixc(C.hair, C.hairDark, 0.45f)); }

    void drawBackHair(const CharacterDef& C)
    {
        lineJoin(ROUND);
        switch (C.hs)
        {
        case hsLong:
        case hsHime:
            beginPath();
            moveTo(-0.6f, -0.78f);
            bezierTo(-1.15f, -0.75f, -1.22f, 0.2f, -1.3f, 1.1f);
            bezierTo(-1.38f, 1.9f, -1.45f, 2.4f, -1.5f, 2.9f);
            lineTo(1.5f, 2.9f);
            bezierTo(1.45f, 2.4f, 1.38f, 1.9f, 1.3f, 1.1f);
            bezierTo(1.22f, 0.2f, 1.15f, -0.75f, 0.6f, -0.78f);
            closePath();
            fillPaint(hairPaint(C, -0.4f, 2.9f)); fill(); hairStroke(C);
            break;
        case hsWavy:
            beginPath();
            moveTo(-0.6f, -0.78f);
            bezierTo(-1.2f, -0.7f, -1.1f, 0.6f, -1.45f, 1.4f);
            bezierTo(-1.75f, 1.9f, -1.3f, 2.3f, -1.6f, 2.9f);
            lineTo(1.6f, 2.9f);
            bezierTo(1.3f, 2.3f, 1.75f, 1.9f, 1.45f, 1.4f);
            bezierTo(1.1f, 0.6f, 1.2f, -0.7f, 0.6f, -0.78f);
            closePath();
            fillPaint(hairPaint(C, -0.4f, 2.9f)); fill(); hairStroke(C);
            break;
        case hsTwintails:
            for (int s = -1; s <= 1; s += 2)
            {
                const float sw = std::sin(fT * 1.3f + s) * 0.05f;
                beginPath();
                moveTo(s * 0.78f, -0.72f);
                bezierTo(s * 1.45f, -0.75f, s * (1.5f + sw), 0.3f, s * (1.32f + sw), 1.45f);
                bezierTo(s * (1.2f + sw), 1.1f, s * (1.0f + sw), 0.9f, s * (1.12f + sw), 1.6f);
                bezierTo(s * 0.9f, 0.6f, s * 1.05f, -0.2f, s * 0.7f, -0.5f);
                closePath();
                fillPaint(hairPaint(C, -0.7f, 1.6f)); fill(); hairStroke(C);
            }
            beginPath(); moveTo(-0.55f, -0.78f); bezierTo(-1.15f, -0.7f, -1.05f, 0.6f, -0.75f, 0.9f); lineTo(0.75f, 0.9f);
            bezierTo(1.05f, 0.6f, 1.15f, -0.7f, 0.55f, -0.78f); closePath();
            fillPaint(hairPaint(C, -0.3f, 0.9f)); fill(); hairStroke(C);
            break;
        case hsPonytail:
        {
            const float sw = std::sin(fT * 1.6f) * 0.08f;
            beginPath();
            moveTo(0.55f, -0.95f);
            bezierTo(1.5f, -1.2f, 1.6f + sw, 0.0f, 1.25f + sw, 1.35f);
            bezierTo(1.2f + sw, 0.6f, 1.05f, 0.2f, 0.95f, -0.2f);
            closePath();
            fillPaint(hairPaint(C, -1.0f, 1.4f)); fill(); hairStroke(C);
            beginPath(); moveTo(-0.55f, -0.78f); bezierTo(-1.12f, -0.7f, -1.0f, 0.5f, -0.7f, 0.95f); lineTo(0.7f, 0.95f);
            bezierTo(1.0f, 0.5f, 1.12f, -0.7f, 0.55f, -0.78f); closePath();
            fillPaint(hairPaint(C, -0.3f, 0.95f)); fill(); hairStroke(C);
            break;
        }
        case hsSideTail:
        {
            const float sw = std::sin(fT * 1.5f) * 0.06f;
            beginPath();
            moveTo(0.62f, -0.7f);
            bezierTo(1.45f, -0.75f, 1.55f + sw, 0.5f, 1.4f + sw, 1.6f);
            bezierTo(1.35f + sw, 2.2f, 1.1f + sw, 2.5f, 1.25f + sw, 2.9f);
            bezierTo(0.95f + sw, 2.3f, 0.95f, 1.2f, 0.92f, 0.2f);
            closePath();
            fillPaint(hairPaint(C, -0.7f, 2.9f)); fill(); hairStroke(C);
            beginPath(); moveTo(-0.55f, -0.78f); bezierTo(-1.2f, -0.7f, -1.12f, 0.75f, -0.82f, 1.1f); lineTo(0.82f, 1.1f);
            bezierTo(1.12f, 0.75f, 1.2f, -0.7f, 0.55f, -0.78f); closePath();
            fillPaint(hairPaint(C, -0.4f, 1.1f)); fill(); hairStroke(C);
            break;
        }
        case hsBoyShort:
        case hsBoySpiky:
        {
            beginPath(); moveTo(-0.55f, -0.78f); bezierTo(-1.12f, -0.7f, -1.08f, 0.3f, -0.92f, 0.55f);
            lineTo(-0.8f, 0.45f); lineTo(-0.7f, 0.62f); lineTo(0.7f, 0.62f); lineTo(0.8f, 0.45f); lineTo(0.92f, 0.55f);
            bezierTo(1.08f, 0.3f, 1.12f, -0.7f, 0.55f, -0.78f); closePath();
            fillPaint(hairPaint(C, -0.6f, 0.6f)); fill(); hairStroke(C);
            break;
        }
        case hsBob:
        case hsBraids:
        case hsBun:
            beginPath(); moveTo(-0.55f, -0.78f); bezierTo(-1.2f, -0.7f, -1.12f, 0.75f, -0.82f, 1.02f); lineTo(0.82f, 1.02f);
            bezierTo(1.12f, 0.75f, 1.2f, -0.7f, 0.55f, -0.78f); closePath();
            fillPaint(hairPaint(C, -0.4f, 1.0f)); fill(); hairStroke(C);
            break;
        }
    }

    void drawBody(const CharacterDef& C)
    {
        save();
        if (C.male) scale(1.1f, 1.f);
        drawBodyInner(C);
        restore();
    }

    void drawBodyInner(const CharacterDef& C)
    {
        const Color skin(255, 233, 220), skinSh(240, 196, 182);
        lineJoin(ROUND);
        // hood behind shoulders
        if (C.of == ofHoodie)
        {
            beginPath(); ellipse(0, 1.28f, 0.9f, 0.32f); fillColor(mixc(C.outfit, 0x000000, 0.25f)); fill();
        }
        // neck
        beginPath(); rect(-0.2f, 0.6f, 0.4f, 0.7f); fillColor(skin); fill();
        beginPath(); moveTo(-0.2f, 0.82f); quadTo(0, 1.0f, 0.2f, 0.82f); lineTo(0.2f, 0.95f); quadTo(0, 1.1f, -0.2f, 0.95f); closePath();
        fillColor(skinSh); fill();
        // torso
        beginPath();
        moveTo(-0.24f, 1.12f);
        bezierTo(-0.7f, 1.22f, -1.15f, 1.3f, -1.3f, 1.6f);
        bezierTo(-1.45f, 1.95f, -1.52f, 2.4f, -1.55f, 3.0f);
        lineTo(1.55f, 3.0f);
        bezierTo(1.52f, 2.4f, 1.45f, 1.95f, 1.3f, 1.6f);
        bezierTo(1.15f, 1.3f, 0.7f, 1.22f, 0.24f, 1.12f);
        closePath();
        fillPaint(linearGradient(0, 1.1f, 0, 3.0f, hexc(C.outfit), mixc(C.outfit, 0x404060, 0.25f)));
        fill();
        strokeWidth(0.02f); strokeColor(mixc(C.outfit, 0x202030, 0.6f)); stroke();

        switch (C.of)
        {
        case ofSailor:
        {
            // collar flaps
            for (int s = -1; s <= 1; s += 2)
            {
                beginPath();
                moveTo(s * 0.22f, 1.1f); lineTo(s * 1.18f, 1.42f); lineTo(s * 0.92f, 1.95f); lineTo(0, 2.25f); lineTo(0, 1.72f);
                closePath();
                fillColor(hexc(C.collar)); fill();
                strokeWidth(0.018f); strokeColor(mixc(C.collar, 0x000000, 0.4f)); stroke();
                // white stripe
                beginPath(); moveTo(s * 1.08f, 1.47f); lineTo(s * 0.86f, 1.88f); lineTo(0.0f, 2.16f);
                strokeWidth(0.03f); strokeColor(Color(255, 255, 255, 0.9f)); stroke();
            }
            drawBow(C.ribbon, 0, 1.98f, 0.36f);
            break;
        }
        case ofBlazer:
        {
            // shirt V + tie
            beginPath(); moveTo(-0.24f, 1.12f); lineTo(0, 1.9f); lineTo(0.24f, 1.12f); closePath(); fillColor(hexc(C.collar)); fill();
            beginPath(); moveTo(-0.06f, 1.2f); lineTo(0.06f, 1.2f); lineTo(0.09f, 1.85f); lineTo(0, 1.95f); lineTo(-0.09f, 1.85f); closePath();
            fillColor(hexc(C.ribbon)); fill();
            for (int s = -1; s <= 1; s += 2)
            {
                beginPath(); moveTo(s * 0.26f, 1.12f); lineTo(s * 0.62f, 1.3f); lineTo(s * 0.34f, 1.6f); lineTo(s * 0.02f, 2.2f); closePath();
                fillColor(mixc(C.outfit, 0xffffff, 0.12f)); fill();
                strokeWidth(0.015f); strokeColor(hexc(C.ribbon, 0.8f)); stroke();
            }
            beginPath(); circle(0.6f, 1.95f, 0.1f); fillColor(hexc(C.ribbon, 0.9f)); fill();
            break;
        }
        case ofSweater:
        {
            beginPath(); roundedRect(-0.3f, 0.95f, 0.6f, 0.32f, 0.12f); fillColor(mixc(C.outfit, 0xffffff, 0.3f)); fill();
            strokeWidth(0.015f); strokeColor(mixc(C.outfit, 0x405070, 0.5f)); stroke();
            strokeWidth(0.012f); strokeColor(mixc(C.outfit, 0x405070, 0.3f, 0.7f));
            for (int k = -5; k <= 5; ++k) { beginPath(); moveTo(k * 0.22f, 1.5f); lineTo(k * 0.24f, 3.0f); stroke(); }
            beginPath(); circle(-0.45f, 2.0f, 0.06f); circle(0.45f, 2.0f, 0.06f); fillColor(hexc(C.ribbon)); fill();
            break;
        }
        case ofHoodie:
        {
            beginPath(); moveTo(-0.3f, 1.12f); quadTo(0, 1.5f, 0.3f, 1.12f); strokeWidth(0.05f); strokeColor(mixc(C.outfit, 0x000000, 0.3f)); stroke();
            strokeWidth(0.025f); strokeColor(hexc(C.ribbon));
            beginPath(); moveTo(-0.18f, 1.3f); lineTo(-0.2f, 1.9f); stroke();
            beginPath(); moveTo(0.18f, 1.3f); lineTo(0.2f, 1.9f); stroke();
            beginPath(); circle(-0.2f, 1.93f, 0.04f); circle(0.2f, 1.93f, 0.04f); fillColor(hexc(C.ribbon)); fill();
            // headphones resting on the neck/shoulders
            beginPath(); moveTo(-0.55f, 1.15f); quadTo(0, 1.55f, 0.55f, 1.15f); strokeWidth(0.07f); strokeColor(Color(40, 40, 52)); stroke();
            for (int s = -1; s <= 1; s += 2)
            {
                beginPath(); ellipse(s * 0.58f, 1.12f, 0.17f, 0.2f); fillColor(Color(52, 52, 66)); fill();
                beginPath(); ellipse(s * 0.58f, 1.12f, 0.09f, 0.11f); fillColor(hexc(C.ribbon)); fill();
            }
            break;
        }
        case ofVest:
        {
            beginPath(); moveTo(-0.24f, 1.12f); lineTo(0, 1.55f); lineTo(0.24f, 1.12f); closePath(); fillColor(Color(255, 255, 255)); fill();
            for (int s = -1; s <= 1; s += 2)
            {
                beginPath(); moveTo(s * 0.24f, 1.12f); lineTo(s * 0.12f, 1.12f); lineTo(s * 0.3f, 1.32f); closePath(); fillColor(Color(250, 250, 250)); fill();
                beginPath(); moveTo(s * 0.5f, 1.25f); lineTo(0, 2.1f); lineTo(0, 3.0f); lineTo(s * 1.2f, 3.0f); lineTo(s * 1.2f, 1.6f); closePath();
                fillColor(hexc(C.collar)); fill();
            }
            drawBow(C.ribbon, 0, 1.3f, 0.22f);
            break;
        }
        case ofKimono:
        {
            // crossed collar (left over right)
            beginPath(); moveTo(-0.24f, 1.1f); lineTo(0.38f, 2.4f); lineTo(0.55f, 2.4f); lineTo(-0.05f, 1.1f); closePath();
            fillColor(hexc(C.collar)); fill();
            beginPath(); moveTo(0.24f, 1.1f); lineTo(-0.1f, 1.75f); lineTo(0.02f, 1.85f); lineTo(0.36f, 1.12f); closePath();
            fillColor(Color(255, 250, 240)); fill();
            // pattern dots (little flowers)
            for (int k = 0; k < 14; ++k)
            {
                const float x = -1.3f + hash01(k + 2000) * 2.6f, y = 1.6f + hash01(k + 2100) * 1.3f;
                fillColor(hexc(C.collar, 0.8f)); drawFlower(x, y, 0.1f);
            }
            beginPath(); rect(-1.6f, 2.55f, 3.2f, 0.5f); fillColor(hexc(C.ribbon)); fill();
            beginPath(); rect(-1.6f, 2.62f, 3.2f, 0.05f); fillColor(hexc(C.collar)); fill();
            break;
        }
        case ofIdolDress:
        {
            // stage dress: frilled neckline, big bow, sparkles, puff sleeves
            for (int sd = -1; sd <= 1; sd += 2)
            {
                beginPath(); ellipse(sd * 1.2f, 1.62f, 0.3f, 0.28f); fillColor(hexc(C.collar)); fill();
                strokeWidth(0.015f); strokeColor(mixc(C.collar, 0x000000, 0.3f)); stroke();
                for (int k = 0; k < 4; ++k) { beginPath(); circle(sd * (1.02f + k * 0.11f), 1.88f + (k % 2) * 0.02f, 0.05f); fillColor(Color(255, 255, 255)); fill(); }
            }
            beginPath(); moveTo(-0.45f, 1.18f); quadTo(0, 1.55f, 0.45f, 1.18f); lineTo(0.5f, 1.32f); quadTo(0, 1.7f, -0.5f, 1.32f); closePath();
            fillColor(Color(255, 255, 255)); fill(); strokeWidth(0.012f); strokeColor(Color(200, 200, 220)); stroke();
            beginPath(); rect(-0.22f, 1.02f, 0.44f, 0.07f); fillColor(hexc(C.ribbon)); fill();   // choker
            drawBow(C.ribbon, 0, 1.72f, 0.42f);
            for (int k = 0; k < 18; ++k)
            {
                const float x = -1.3f + hash01(k + 3000) * 2.6f, y = 2.0f + hash01(k + 3100) * 0.9f;
                const float tw = 0.5f + 0.5f * std::sin(fT * 3.f + k);
                beginPath(); circle(x, y, 0.022f); fillColor(Color(255, 255, 255, 0.4f + 0.6f * tw)); fill();
            }
            beginPath(); rect(-1.6f, 2.45f, 3.2f, 0.06f); fillColor(hexc(C.ribbon, 0.9f)); fill();
            break;
        }
        case ofIdolJacket:
        {
            // open stage jacket over a shirt, tie, star badge
            beginPath(); moveTo(-0.3f, 1.12f); lineTo(0, 1.95f); lineTo(0.3f, 1.12f); closePath(); fillColor(Color(245, 245, 250)); fill();
            beginPath(); moveTo(-0.06f, 1.2f); lineTo(0.06f, 1.2f); lineTo(0.09f, 1.85f); lineTo(0, 1.97f); lineTo(-0.09f, 1.85f); closePath(); fillColor(hexc(C.ribbon)); fill();
            for (int sd = -1; sd <= 1; sd += 2)
            {
                beginPath(); moveTo(sd * 0.3f, 1.1f); lineTo(sd * 0.7f, 1.3f); lineTo(sd * 0.42f, 1.62f); lineTo(sd * 0.05f, 2.4f); lineTo(sd * 0.05f, 3.0f); lineTo(sd * 1.6f, 3.0f); lineTo(sd * 1.5f, 1.6f); closePath();
                fillColor(mixc(C.outfit, 0xffffff, 0.06f)); fill();
                beginPath(); moveTo(sd * 0.3f, 1.1f); lineTo(sd * 0.7f, 1.3f); lineTo(sd * 0.42f, 1.62f); lineTo(sd * 0.05f, 2.4f);
                strokeWidth(0.04f); strokeColor(hexc(C.collar)); stroke();
            }
            save(); translate(0.75f, 1.85f); rotate(0.2f);
            beginPath();
            for (int k = 0; k < 10; ++k) { const float r = (k & 1) ? 0.05f : 0.12f, a = -kPi / 2 + k * kPi / 5; if (k == 0) moveTo(std::cos(a) * r, std::sin(a) * r); else lineTo(std::cos(a) * r, std::sin(a) * r); }
            closePath(); fillColor(Color(255, 220, 90)); fill();
            restore();
            break;
        }
        case ofGothic:
        {
            // lace collar: two rows of white scallops along the neckline
            for (int row = 0; row < 2; ++row)
                for (int k = -6; k <= 6; ++k)
                {
                    const float t = k / 6.f, cx = t * (0.5f + row * 0.12f), cy = 1.18f + std::fabs(t) * 0.12f + row * 0.11f;
                    beginPath(); circle(cx, cy, 0.075f - row * 0.01f);
                    fillColor(row ? Color(235, 228, 240) : hexc(C.collar)); fill();
                    strokeWidth(0.01f); strokeColor(Color(150, 140, 160)); stroke();
                }
            // corset lacing
            strokeWidth(0.02f); strokeColor(hexc(C.ribbon, 0.9f));
            for (int k = 0; k < 5; ++k)
            {
                const float y0 = 1.85f + k * 0.18f;
                beginPath(); moveTo(-0.14f, y0); lineTo(0.14f, y0 + 0.12f); stroke();
                beginPath(); moveTo(0.14f, y0); lineTo(-0.14f, y0 + 0.12f); stroke();
            }
            // puff sleeves with lace trim
            for (int sd = -1; sd <= 1; sd += 2)
            {
                beginPath(); ellipse(sd * 1.18f, 1.72f, 0.26f, 0.3f); fillColor(mixc(C.outfit, 0x4a3a56, 0.35f)); fill();
                strokeWidth(0.015f); strokeColor(Color(10, 6, 14)); stroke();
                for (int k = 0; k < 4; ++k) { beginPath(); circle(sd * (1.03f + k * 0.1f), 1.98f + (k % 2) * 0.02f, 0.045f); fillColor(hexc(C.collar)); fill(); }
            }
            // rose brooch
            beginPath(); circle(0, 1.55f, 0.12f); fillPaint(radialGradient(-0.03f, 1.52f, 0.01f, 0.13f, Color(255, 90, 110), hexc(C.ribbon))); fill();
            strokeWidth(0.015f); strokeColor(Color(90, 0, 20)); beginPath(); arc(0, 1.55f, 0.06f, 0.5f, 5.0f, CW); stroke();
            break;
        }
        }
    }

    void drawBow(uint32_t col, float x, float y, float s)
    {
        fillColor(hexc(col));
        strokeColor(mixc(col, 0x000000, 0.35f)); strokeWidth(0.016f);
        for (int k = -1; k <= 1; k += 2)
        {
            beginPath(); moveTo(x, y); bezierTo(x + k * s * 0.8f, y - s * 0.55f, x + k * s * 1.1f, y + s * 0.15f, x + k * s * 0.95f, y + s * 0.3f);
            lineTo(x, y); closePath(); fill(); stroke();
            beginPath(); moveTo(x, y); lineTo(x + k * s * 0.3f, y + s * 0.95f); lineTo(x + k * s * 0.55f, y + s * 0.85f); closePath(); fill(); stroke();
        }
        beginPath(); ellipse(x, y + 0.02f, s * 0.16f, s * 0.14f); fill(); stroke();
    }

    void drawHairBehindHead(const CharacterDef& C)
    {
        if (C.hs == hsBoySpiky)
        {
            // spikes sticking out of the crown
            static const float sp[7][3] = { {-0.95f, -0.35f, -1.25f}, {-0.75f, -0.75f, -1.05f}, {-0.35f, -1.0f, -1.3f}, {0.05f, -1.05f, -1.42f},
                                            {0.45f, -0.98f, -1.28f}, {0.8f, -0.7f, -0.98f}, {0.98f, -0.3f, -1.2f} };
            for (const auto& q : sp)
            {
                const float a = q[0], b = q[1];
                beginPath(); moveTo(a * 0.75f, b * 0.8f + 0.05f); lineTo(a * 1.22f, b * 0.95f + (q[2] - b) * 0.55f); lineTo(a * 0.95f + 0.12f, b * 0.8f + 0.1f); closePath();
                fillPaint(hairPaint(C, -1.4f, -0.3f)); fill(); hairStroke(C);
            }
        }
        if (C.hs == hsBun)
        {
            beginPath(); circle(0.0f, -1.08f, 0.42f); fillPaint(hairPaint(C, -1.5f, -0.7f)); fill(); hairStroke(C);
            strokeWidth(0.018f); strokeColor(hexc(C.hairDark, 0.6f));
            beginPath(); arc(0, -1.08f, 0.28f, 3.4f, 5.6f, CW); stroke();
        }
    }

    void drawFace(const CharacterDef& C, float act)
    {
        const Color skin(255, 239, 230), skinMid(252, 222, 210), skinSh(238, 186, 178);
        const float fx = fLX * 0.07f, fy = fLY * 0.045f;   // facial features shift with the turn
        // face
        beginPath();
        moveTo(-0.84f, -0.2f);
        bezierTo(-0.86f + fx * 0.3f, 0.45f, -0.42f + fx * 0.5f, 0.86f, 0.0f + fx * 0.6f, 0.97f);
        bezierTo(0.42f + fx * 0.5f, 0.86f, 0.86f + fx * 0.3f, 0.45f, 0.84f, -0.2f);
        bezierTo(0.84f, -1.0f, -0.84f, -1.0f, -0.84f, -0.2f);
        closePath();
        fillPaint(radialGradient(fx * 2.f - 0.15f, 0.05f, 0.25f, 1.15f, skin, skinMid));
        fill();
        strokeWidth(0.016f); strokeColor(Color(196, 128, 116, 0.85f)); stroke();
        // ears (the far one hides when she turns)
        for (int s2 = -1; s2 <= 1; s2 += 2)
        {
            const float vis = 1.f - std::fmax(0.f, -s2 * fLX) * 0.6f;
            beginPath(); ellipse(s2 * 0.84f, 0.2f, 0.085f * vis, 0.15f); fillColor(Color(248, 210, 198)); fill();
            beginPath(); ellipse(s2 * 0.85f, 0.21f, 0.04f * vis, 0.08f); fillColor(Color(232, 178, 168, 0.8f)); fill();
        }
        // soft side shading of the cheek opposite to the light
        beginPath(); ellipse(0.62f + fx, 0.35f, 0.3f, 0.5f);
        fillPaint(radialGradient(0.72f + fx, 0.35f, 0.05f, 0.4f, Color(240, 190, 180, 0.35f), Color(240, 190, 180, 0.f))); fill();

        save(); translate(fx, fy);
        // male idols: visible brows, softer blush
        if (C.male)
        {
            lineCap(ROUND); strokeWidth(0.04f); strokeColor(mixc(C.hairDark, 0x000000, 0.2f));
            for (int s2 = -1; s2 <= 1; s2 += 2) { beginPath(); moveTo(s2 * 0.2f, -0.12f); quadTo(s2 * 0.38f, -0.19f, s2 * 0.56f, -0.14f); stroke(); }
        }
        // airbrushed blush + 2000s hatch lines
        for (int s2 = -1; s2 <= 1; s2 += 2)
        {
            const float bx = s2 * 0.5f, by = 0.47f, ba = (C.male ? 0.12f : 0.38f) + 0.2f * std::fmin(1.f, act * 2);
            beginPath(); ellipse(bx, by, 0.24f, 0.13f);
            fillPaint(radialGradient(bx, by, 0.02f, 0.22f, Color(255, 120, 140, ba), Color(255, 120, 140, 0.f))); fill();
            if (!C.male)
            {
                strokeWidth(0.011f); strokeColor(Color(235, 100, 120, 0.55f));
                for (int k = 0; k < 3; ++k) { beginPath(); moveTo(bx - 0.06f + k * 0.055f, by - 0.035f); lineTo(bx - 0.08f + k * 0.055f, by + 0.03f); stroke(); }
            }
        }

        // eyes
        const float bt = std::fmod(fT + 0.37f * (float)(&C - kChars), 4.3f);
        const bool blink = fForceBlink >= 0 ? fForceBlink == 1 : bt > 4.15f;
        const bool happy = fForceBlink < 0 && act > 0.35f && std::fmod(fT, 2.5f) < 0.6f;
        for (int s2 = -1; s2 <= 1; s2 += 2) drawEye(C, s2 * 0.37f, 0.15f, (float)s2, blink, happy);

        // brows (thin, slightly raised in 2000s style)
        lineCap(ROUND);
        strokeWidth(0.026f); strokeColor(mixc(C.hairDark, C.hair, 0.3f, 0.9f));
        for (int s2 = -1; s2 <= 1; s2 += 2) { beginPath(); moveTo(s2 * 0.24f, -0.27f); quadTo(s2 * 0.38f, -0.32f, s2 * 0.52f, -0.28f); stroke(); }

        // nose: tiny shadow + highlight
        strokeWidth(0.016f); strokeColor(Color(214, 146, 136));
        beginPath(); moveTo(0.025f, 0.4f); quadTo(0.035f, 0.44f, 0.005f, 0.455f); stroke();
        beginPath(); circle(-0.025f, 0.41f, 0.012f); fillColor(Color(255, 255, 255, 0.7f)); fill();

        // mouth
        float open = fForceMouth >= 0 ? (float)fForceMouth : (act > 0.03f ? (0.5f + 0.5f * std::sin(fT * 13.f)) * std::fmin(1.f, act * 2.5f) : 0.f);
        if (fDrawingIdol)
        {
            // lip sync with the sung vowel
            open = fParams[pOutMouth];
            const int vw = (int)std::lround(fParams[pOutVowel]);
            static const float mw[6] = { 0.085f, 0.1f, 0.045f, 0.09f, 0.06f, 0.07f }, mh[6] = { 0.13f, 0.045f, 0.06f, 0.075f, 0.1f, 0.0f };
            if (open > 0.06f && vw < 5)
            {
                const float w = mw[vw], h = 0.012f + mh[vw] * open;
                beginPath(); ellipse(0, 0.63f, w, h); fillColor(Color(150, 44, 66)); fill();
                beginPath(); ellipse(0, 0.63f + h * 0.55f, w * 0.6f, h * 0.35f); fillColor(Color(255, 140, 160)); fill();
                if (h > 0.03f) { beginPath(); rect(-w * 0.6f, 0.63f - h + 0.004f, w * 1.2f, 0.016f); fillColor(Color(255, 255, 255, 0.9f)); fill(); }
                restore();
                return;
            }
            open = 0;
        }
        if (open > 0.08f)
        {
            beginPath(); moveTo(-0.085f, 0.6f); quadTo(0, 0.615f, 0.085f, 0.6f); quadTo(0.065f, 0.67f + open * 0.1f, 0, 0.68f + open * 0.1f);
            quadTo(-0.065f, 0.67f + open * 0.1f, -0.085f, 0.6f); closePath();
            fillColor(Color(160, 48, 70)); fill();
            beginPath(); ellipse(0, 0.66f + open * 0.08f, 0.045f, 0.02f + open * 0.02f); fillColor(Color(255, 140, 160)); fill();
            beginPath(); rect(-0.05f, 0.603f, 0.1f, 0.018f); fillColor(Color(255, 255, 255, 0.9f)); fill();
        }
        else
        {
            strokeWidth(0.02f); strokeColor(Color(178, 74, 86));
            beginPath(); moveTo(-0.07f, 0.61f); quadTo(0, 0.665f, 0.07f, 0.61f); stroke();
        }
        restore();
        (void)skinSh;
    }

    void drawEye(const CharacterDef& C, float x, float y, float dir, bool blink, bool happy)
    {
        save();
        translate(x, y); scale(dir, C.male ? 0.8f : 1.f);
        lineCap(ROUND); lineJoin(ROUND);
        const Color lash = mixc(C.hairDark, 0x1a0c14, 0.6f);
        if (blink || happy)
        {
            beginPath();
            if (happy) { moveTo(-0.2f, 0.09f); quadTo(0.0f, -0.13f, 0.23f, 0.06f); }
            else       { moveTo(-0.21f, 0.04f); quadTo(0.0f, 0.14f, 0.24f, 0.0f); lineTo(0.3f, -0.03f); }
            strokeWidth(0.045f); strokeColor(lash); stroke();
            if (!happy) { strokeWidth(0.016f); beginPath(); moveTo(-0.1f, 0.1f); lineTo(-0.12f, 0.15f); moveTo(0.05f, 0.105f); lineTo(0.05f, 0.16f); moveTo(0.17f, 0.07f); lineTo(0.2f, 0.12f); stroke(); }
            restore();
            return;
        }
        // eye white
        beginPath(); moveTo(-0.21f, 0.0f); bezierTo(-0.13f, -0.22f, 0.13f, -0.24f, 0.24f, -0.06f);
        bezierTo(0.25f, 0.12f, 0.14f, 0.26f, 0.0f, 0.265f); bezierTo(-0.15f, 0.26f, -0.23f, 0.13f, -0.21f, 0.0f); closePath();
        fillColor(Color(255, 255, 255)); fill();
        // shadow of the upper lid on the white
        beginPath(); moveTo(-0.21f, 0.0f); bezierTo(-0.13f, -0.22f, 0.13f, -0.24f, 0.24f, -0.06f); lineTo(0.24f, 0.0f);
        bezierTo(0.12f, -0.14f, -0.12f, -0.14f, -0.21f, 0.04f); closePath();
        fillColor(Color(170, 180, 215, 0.55f)); fill();

        // iris (follows the cursor)
        const float ix = 0.012f + fLX * 0.04f * dir, iy = 0.075f + fLY * 0.03f;
        beginPath(); ellipse(ix, iy, 0.15f, 0.2f);
        fillPaint(linearGradient(0, iy - 0.2f, 0, iy + 0.2f, mixc(C.eyeDark, 0x000000, 0.2f), mixc(C.eye, 0xffffff, 0.15f))); fill();
        strokeWidth(0.014f); strokeColor(mixc(C.eyeDark, 0x000000, 0.35f)); stroke();
        // radial streaks
        strokeWidth(0.007f); strokeColor(mixc(C.eye, 0xffffff, 0.4f, 0.35f));
        for (int k = 0; k < 10; ++k)
        {
            const float a = k * 0.628f + 0.3f;
            beginPath(); moveTo(ix + std::cos(a) * 0.06f, iy + std::sin(a) * 0.08f); lineTo(ix + std::cos(a) * 0.13f, iy + std::sin(a) * 0.17f); stroke();
        }
        // pupil + dark upper crescent
        beginPath(); ellipse(ix, iy + 0.01f, 0.065f, 0.095f); fillColor(mixc(C.eyeDark, 0x000000, 0.55f)); fill();
        beginPath(); ellipse(ix, iy - 0.1f, 0.14f, 0.09f); fillColor(mixc(C.eyeDark, 0x000000, 0.3f, 0.55f)); fill();
        // lower glow
        beginPath(); ellipse(ix, iy + 0.12f, 0.1f, 0.06f);
        fillPaint(radialGradient(ix, iy + 0.13f, 0.01f, 0.1f, mixc(C.eye, 0xffffff, 0.65f, 0.85f), mixc(C.eye, 0xffffff, 0.4f, 0.f))); fill();
        // highlights
        const float tw = 0.88f + 0.12f * std::sin(fT * 3.f);
        beginPath(); ellipse(ix - 0.06f, iy - 0.07f, 0.055f, 0.065f); fillColor(Color(255, 255, 255, tw)); fill();
        beginPath(); circle(ix + 0.07f, iy + 0.08f, 0.024f); fillColor(Color(255, 255, 255, 0.9f)); fill();
        beginPath(); circle(ix + 0.02f, iy - 0.1f, 0.012f); fillColor(Color(255, 255, 255, 0.8f)); fill();

        // upper lash line: tapered filled shape with flicks at the outer corner
        beginPath();
        moveTo(-0.23f, 0.02f);
        bezierTo(-0.14f, -0.22f, 0.13f, -0.25f, 0.26f, -0.07f);
        if (C.male) lineTo(0.27f, -0.02f);
        else { lineTo(0.33f, -0.12f); lineTo(0.28f, -0.03f); lineTo(0.33f, -0.02f); lineTo(0.25f, 0.0f); }
        bezierTo(0.12f, -0.16f, -0.12f, -0.16f, -0.2f, 0.04f);
        closePath();
        fillColor(lash); fill();
        // lid crease
        strokeWidth(0.009f); strokeColor(mixc(C.hairDark, 0xd09080, 0.7f, 0.45f));
        beginPath(); moveTo(-0.08f, -0.22f); quadTo(0.07f, -0.28f, 0.2f, -0.19f); stroke();
        // lower lashes
        strokeWidth(0.014f); strokeColor(mixc(C.hairDark, 0x804050, 0.5f, 0.85f));
        beginPath(); moveTo(0.06f, 0.26f); quadTo(0.17f, 0.24f, 0.23f, 0.15f); stroke();
        beginPath(); moveTo(-0.17f, 0.2f); lineTo(-0.12f, 0.24f); stroke();
        restore();
    }

    void bangsPath(const CharacterDef& C, float dy)
    {
        beginPath();
        moveTo(-0.92f, 0.05f + dy);
        bezierTo(-1.08f, -1.25f + dy, 1.08f, -1.25f + dy, 0.92f, 0.05f + dy);
        if (C.hs == hsBoyShort || C.hs == hsBoySpiky)
        {
            static const float bp[][4] = {
                { 0.86f, -0.25f, 0.78f, 0.05f }, { 0.68f, -0.2f, 0.55f, -0.15f }, { 0.5f, -0.05f, 0.38f, 0.14f },
                { 0.3f, -0.15f, 0.16f, -0.2f }, { 0.08f, 0.0f, -0.05f, 0.16f }, { -0.12f, -0.12f, -0.24f, -0.18f },
                { -0.36f, 0.0f, -0.46f, 0.12f }, { -0.55f, -0.15f, -0.66f, -0.12f }, { -0.78f, 0.0f, -0.84f, 0.08f }, { -0.88f, -0.05f, -0.92f, 0.05f },
            };
            for (const auto& q : bp) quadTo(q[0], q[1] + dy, q[2], q[3] + dy);
        }
        else if (C.hs == hsHime)
        {
            lineTo(0.86f, -0.02f + dy); lineTo(0.7f, 0.04f + dy);
            for (int k = 0; k < 8; ++k) { const float x0 = 0.7f - k * 0.175f; lineTo(x0 - 0.08f, -0.06f + dy); lineTo(x0 - 0.175f, 0.03f + dy); }
        }
        else
        {
            static const float pts[][4] = {
                { 0.88f, -0.2f, 0.74f, 0.16f }, { 0.66f, -0.1f, 0.58f, -0.08f },
                { 0.55f, -0.05f, 0.44f, 0.1f }, { 0.38f, -0.12f, 0.3f, -0.12f },
                { 0.26f, -0.02f, 0.14f, 0.12f }, { 0.06f, -0.15f, 0.0f, -0.12f },
                { -0.06f, -0.02f, -0.18f, 0.14f }, { -0.22f, -0.12f, -0.32f, -0.1f },
                { -0.4f, 0.0f, -0.5f, 0.1f }, { -0.55f, -0.08f, -0.62f, -0.08f },
                { -0.7f, 0.02f, -0.8f, 0.18f }, { -0.82f, -0.05f, -0.92f, 0.05f },
            };
            for (const auto& p : pts) quadTo(p[0], p[1] + dy, p[2], p[3] + dy);
        }
        closePath();
    }

    void drawFrontHair(const CharacterDef& C)
    {
        lineJoin(ROUND);
        // cel shadow cast by the bangs onto the forehead
        save(); translate(0.02f, 0.0f);
        bangsPath(C, 0.075f);
        fillColor(Color(232, 168, 162, 0.55f)); fill();
        restore();
        // side locks
        if (C.hs != hsBraids)
            for (int s = -1; s <= 1; s += 2)
            {
                const float len = (C.hs == hsHime) ? 0.85f : (C.hs == hsBob ? 0.95f : ((C.hs == hsBoyShort || C.hs == hsBoySpiky) ? 0.5f : 1.25f));
                const float sw = fHairSway * len;
                beginPath();
                moveTo(s * 0.72f, -0.45f);
                bezierTo(s * 1.0f, 0.1f, s * 0.98f + sw * 0.5f, len * 0.6f, s * 0.9f + sw, len);
                if (C.hs == hsHime) lineTo(s * 0.68f + sw, len);
                else bezierTo(s * 0.86f + sw, len * 0.7f, s * 0.78f + sw * 0.5f, len * 0.4f, s * 0.7f + sw * 0.6f, len * 0.75f);
                bezierTo(s * 0.72f, 0.3f, s * 0.68f, 0.0f, s * 0.6f, -0.3f);
                closePath();
                fillPaint(hairPaint(C, -0.45f, len)); fill(); hairStroke(C);
            }

        // cap + bangs
        bangsPath(C, 0.f);
        fillPaint(linearGradient(0, -1.0f, 0, 0.1f, mixc(C.hair, C.hairLight, 0.12f), mixc(C.hair, C.hairDark, 0.3f)));
        fill(); strokeWidth(0.017f); strokeColor(hexc(C.hairDark, 0.95f)); stroke();
        // darker strand wedges between the bangs (2000s cel shading)
        static const float tips[6] = { 0.74f, 0.44f, 0.14f, -0.18f, -0.5f, -0.8f };
        for (int k = 0; k < 6; ++k)
        {
            const float tx = tips[k];
            beginPath(); moveTo(tx * 0.35f, -0.78f); quadTo(tx * 0.8f + 0.06f, -0.4f, tx + 0.08f, -0.02f); quadTo(tx * 0.8f, -0.35f, tx * 0.35f - 0.05f, -0.78f); closePath();
            fillColor(hexc(C.hairDark, 0.28f)); fill();
        }
        // strand lines
        strokeWidth(0.014f); strokeColor(hexc(C.hairDark, 0.45f));
        for (int k = -3; k <= 3; ++k) { beginPath(); moveTo(k * 0.12f, -0.85f); quadTo(k * 0.2f, -0.45f, k * 0.24f, -0.12f); stroke(); }

        // angel ring: smooth glossy crescent, cut by a few strand lines
        beginPath();
        arc(0, -0.1f, 0.74f, 3.75f, 5.68f, CW);
        arc(0, -0.13f, 0.66f, 5.62f, 3.81f, CCW);
        closePath();
        fillPaint(linearGradient(0, -0.9f, 0, -0.6f, hexc(C.hairLight, 0.6f), hexc(C.hairLight, 0.15f)));
        fill();
        strokeWidth(0.016f); strokeColor(mixc(C.hair, C.hairDark, 0.4f, 0.7f));
        for (int k = -3; k <= 3; ++k) { beginPath(); moveTo(k * 0.16f, -0.86f); lineTo(k * 0.19f, -0.7f); stroke(); }
        beginPath(); ellipse(-0.28f, -0.76f, 0.09f, 0.022f); fillColor(Color(255, 255, 255, 0.6f)); fill();
    }

    void drawBraid(const CharacterDef& C, float s)
    {
        save();
        translate(0, 0.0f);
        for (int k = 0; k < 9; ++k)
        {
            const float y = 0.35f + k * 0.2f, x = s * (0.8f + k * 0.035f);
            beginPath(); ellipse(x + s * ((k & 1) ? 0.03f : -0.03f), y, 0.13f, 0.14f);
            fillPaint(linearGradient(0, y - 0.14f, 0, y + 0.14f, hexc(C.hairLight), hexc(C.hair))); fill(); hairStroke(C);
        }
        const float ex = s * 1.12f, ey = 2.2f;
        beginPath(); ellipse(ex, ey, 0.1f, 0.06f); fillColor(hexc(C.ribbon)); fill();
        beginPath(); moveTo(ex - 0.08f, ey + 0.04f); quadTo(ex, ey + 0.35f, ex + 0.1f, ey + 0.04f); fillColor(hexc(C.hair)); fill(); hairStroke(C);
        restore();
    }

    void drawAccessory(const CharacterDef& C)
    {
        switch (C.acc)
        {
        case acRibbon:
            drawBow(C.accent, -0.55f, -0.82f, 0.36f);
            break;
        case acBell:
            for (int s = -1; s <= 1; s += 2)
            {
                const float x = s * 0.8f, y = -0.72f;
                drawBow(C.ribbon == 0xffb43c ? 0xff8fb0 : C.ribbon, x, y - 0.05f, 0.18f);
                beginPath(); circle(x, y + 0.13f, 0.1f);
                fillPaint(radialGradient(x - 0.03f, y + 0.1f, 0.01f, 0.12f, Color(255, 245, 180), Color(220, 160, 30))); fill();
                strokeWidth(0.012f); strokeColor(Color(150, 100, 20)); stroke();
                beginPath(); moveTo(x - 0.06f, y + 0.17f); lineTo(x + 0.06f, y + 0.17f); stroke();
            }
            break;
        case acSnow:
        {
            save(); translate(-0.58f, -0.55f); rotate(fT * 0.2f);
            strokeWidth(0.03f); strokeColor(Color(130, 190, 255)); lineCap(ROUND);
            for (int k = 0; k < 6; ++k)
            {
                const float a = k * kPi / 3;
                beginPath(); moveTo(0, 0); lineTo(std::cos(a) * 0.16f, std::sin(a) * 0.16f); stroke();
                beginPath(); moveTo(std::cos(a) * 0.1f, std::sin(a) * 0.1f); lineTo(std::cos(a + 0.5f) * 0.14f, std::sin(a + 0.5f) * 0.14f); stroke();
            }
            restore();
            break;
        }
        case acScrunchie:
            beginPath(); ellipse(0.66f, -0.86f, 0.13f, 0.11f); fillColor(hexc(C.ribbon)); fill();
            strokeWidth(0.015f); strokeColor(mixc(C.ribbon, 0x000000, 0.35f)); stroke();
            // star hair clip
            save(); translate(-0.6f, -0.45f); rotate(0.3f);
            beginPath();
            for (int k = 0; k < 10; ++k) { const float r = (k & 1) ? 0.05f : 0.12f, a = -kPi / 2 + k * kPi / 5; if (k == 0) moveTo(std::cos(a) * r, std::sin(a) * r); else lineTo(std::cos(a) * r, std::sin(a) * r); }
            closePath(); fillColor(hexc(C.ribbon)); fill(); strokeColor(mixc(C.ribbon, 0x000000, 0.35f)); stroke();
            restore();
            break;
        case acHeadphones:
            break; // drawn around the neck with the hoodie
        case acGlasses:
            strokeWidth(0.028f); strokeColor(Color(150, 40, 50, 0.95f));
            for (int s = -1; s <= 1; s += 2)
            {
                beginPath(); roundedRect(s * 0.38f - 0.22f, 0.0f, 0.44f, 0.36f, 0.12f);
                fillColor(Color(220, 240, 255, 0.12f)); fill(); stroke();
            }
            beginPath(); moveTo(-0.16f, 0.12f); quadTo(0, 0.07f, 0.16f, 0.12f); stroke();
            break;
        case acKanzashi:
        {
            const float x = 0.42f, y = -1.05f;
            strokeWidth(0.012f); strokeColor(Color(210, 170, 60));
            for (int k = 0; k < 3; ++k)
            {
                const float dx = k * 0.05f, sw = std::sin(fT * 2.f + k) * 0.02f;
                beginPath(); moveTo(x + dx, y + 0.1f); lineTo(x + dx + sw, y + 0.5f + k * 0.08f); stroke();
                beginPath(); circle(x + dx + sw, y + 0.52f + k * 0.08f, 0.03f); fillColor(Color(255, 90, 120)); fill();
            }
            fillColor(Color(255, 120, 150)); drawFlower(x, y, 0.2f);
            fillColor(Color(255, 255, 255, 0.9f)); drawFlower(x + 0.2f, y + 0.08f, 0.12f);
            break;
        }
        case acHeadset:
        {
            lineCap(ROUND);
            beginPath(); arc(0, -0.08f, 1.0f, kPi + 0.25f, 2 * kPi - 0.25f, CW); strokeWidth(0.06f); strokeColor(Color(50, 52, 64)); stroke();
            beginPath(); arc(0, -0.08f, 1.0f, kPi + 0.35f, 2 * kPi - 0.35f, CW); strokeWidth(0.015f); strokeColor(hexc(C.accent)); stroke();
            for (int sd = -1; sd <= 1; sd += 2)
            {
                beginPath(); ellipse(sd * 0.93f, 0.14f, 0.13f, 0.17f); fillColor(Color(52, 54, 68)); fill();
                beginPath(); ellipse(sd * 0.93f, 0.14f, 0.07f, 0.1f); fillColor(hexc(C.accent, 0.6f + 0.4f * std::sin(fT * 3.f))); fill();
            }
            beginPath(); moveTo(-0.9f, 0.25f); quadTo(-0.75f, 0.62f, -0.3f, 0.66f); strokeWidth(0.025f); strokeColor(Color(50, 52, 64)); stroke();
            beginPath(); circle(-0.27f, 0.66f, 0.045f); fillColor(Color(40, 40, 50)); fill();
            break;
        }
        case acHeaddress:
        {
            // black lace headdress with a red rose
            beginPath(); arc(0, -0.05f, 0.9f, 3.55f, 5.87f, CW); strokeWidth(0.13f); strokeColor(Color(24, 18, 28)); lineCap(ROUND); stroke();
            for (int k = 0; k <= 12; ++k)
            {
                const float a = 3.6f + k * (2.22f / 12.f);
                beginPath(); circle(std::cos(a) * 0.98f, -0.05f + std::sin(a) * 0.98f, 0.06f); fillColor(Color(245, 240, 248)); fill();
            }
            fillColor(Color(176, 16, 42)); drawFlower(-0.62f, -0.78f, 0.17f);
            beginPath(); circle(-0.62f, -0.78f, 0.06f); fillColor(Color(110, 0, 20)); fill();
            break;
        }
        case acNone:
            break;
        }
    }

    void drawNotes(const CharacterDef& C)
    {
        const float act = fParams[pOutActivity];
        if (act < 0.03f) return;
        for (int i = 0; i < 6; ++i)
        {
            const float ph = std::fmod(fT * 0.5f + i / 6.f, 1.f);
            const float x = 560.f + i * 18.f + std::sin(fT * 2 + i) * 14.f;
            const float y = 230.f - ph * 170.f;
            const float al = std::fmin(1.f, act * 3.f) * (1.f - ph) * std::fmin(1.f, ph * 5.f);
            outlinedLabel(x, y, (i & 1) ? "\xE2\x99\xAA" : "\xE2\x99\xAB", 20.f + (i % 3) * 4.f, hexc(C.accent, al), Color(255, 255, 255, al), ALIGN_CENTER | ALIGN_MIDDLE);
        }
    }

    // ================================================================== UI panels
    void drawTopBar(const CharacterDef& C)
    {
        beginPath(); rect(0, 0, kW, 46);
        fillPaint(linearGradient(0, 0, 0, 46, Color(255, 255, 255, 0.85f), Color(255, 235, 245, 0.75f))); fill();
        beginPath(); rect(0, 45, kW, 2); fillColor(hexc(C.accent, 0.8f)); fill();
        fillColor(hexc(C.accent)); drawFlower(26, 23, 13);
        outlinedLabel(48, 31, "RETRO NOVEL FM", 22.f, hexc(C.accent), Color(255, 255, 255), ALIGN_LEFT | ALIGN_BASELINE, 2.f);
        {
            const int ci = clampi((int)std::lround(fParams[pInstrument]), 0, kNumInstruments - 1);
            static const char* const eras[3] = { "\xE2\x99\xAA PC-98 / 90s FM", "\xE2\x99\xAA Windows XP / 2000s VN", "\xE2\x99\xAA My samples" };
            (void)ci; (void)eras;
        }

        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type == ctScenesOpen || c.type == ctStoryOpen)
            {
                const bool on = c.type == ctScenesOpen ? fParams[pScene] > 0.5f : fStory >= 0, hv = hover(c.b);
                beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 13);
                if (on) fillPaint(linearGradient(c.b.x, 0, c.b.x + c.b.w, 0, hexc(0xffb347), hexc(0xff5fa8)));
                else fillColor(hv ? Color(255, 230, 242) : Color(255, 255, 255, 0.9f));
                fill(); strokeWidth(1.5f); strokeColor(on ? Color(255, 255, 255) : hexc(C.accent, 0.8f)); stroke();
                const char* t = c.type == ctScenesOpen ? "\xE2\x9C\xA6 SCENES" : "\xE2\x98\x85 STORY";
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, t, 12.f, on ? Color(255, 255, 255) : hexc(C.accent), ALIGN_CENTER | ALIGN_MIDDLE);
                continue;
            }
            if (c.type == ctToggle && c.param == pVocalMode)
            {
                const bool on = fParams[pVocalMode] > 0.5f, hv = hover(c.b);
                beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 13);
                if (on) fillPaint(linearGradient(c.b.x, 0, c.b.x + c.b.w, 0, hexc(0xff5fa8), hexc(0x46a8ff)));
                else fillColor(hv ? Color(255, 230, 242) : Color(255, 255, 255, 0.9f));
                fill(); strokeWidth(1.5f); strokeColor(on ? Color(255, 255, 255) : hexc(C.accent, 0.8f)); stroke();
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, on ? "\xE2\x99\xAB VOCAL" : "\xE2\x99\xAA VOCAL", 12.f, on ? Color(255, 255, 255) : hexc(C.accent), ALIGN_CENTER | ALIGN_MIDDLE);
                continue;
            }
            if (c.type != ctAction) continue;
            const bool h = hover(c.b);
            beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 13);
            fillPaint(linearGradient(0, c.b.y, 0, c.b.y + c.b.h, hexc(C.accent, h ? 1.f : 0.85f), mixc(C.accent, 0x803060, 0.3f)));
            fill();
            strokeWidth(1.5f); strokeColor(Color(255, 255, 255, 0.9f)); stroke();
            label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, c.label, 13.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
        }
        label(940, 30, "v7.0", 11.f, Color(150, 120, 160), ALIGN_RIGHT | ALIGN_BASELINE);
    }

    void pill(const Box& b, bool sel, bool hv, uint32_t accent, float r = 11.f)
    {
        beginPath(); roundedRect(b.x, b.y, b.w, b.h, r);
        if (sel) fillPaint(linearGradient(0, b.y, 0, b.y + b.h, hexc(accent), mixc(accent, 0x60305a, 0.35f)));
        else fillColor(hv ? Color(255, 225, 238, 0.95f) : Color(255, 255, 255, 0.78f));
        fill();
        strokeWidth(1.f); strokeColor(sel ? Color(255, 255, 255) : hexc(accent, 0.45f)); stroke();
    }

    void drawInstrumentPanel()
    {
        glassPanel(16, 56, 244, 390, 0xff7fae);
        panelTitle(28, 72, "INSTRUMENT", 0xff7fae);
        const int cur = clampi((int)std::lround(fParams[pInstrument]), 0, kNumInstruments - 1);
        char buf[64];
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type == ctEraTab)
            {
                const bool sel = fEraTab == (int)c.value;
                pill(c.b, sel, hover(c.b), 0xff7fae, 8.f);
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, c.label, 9.5f, sel ? Color(255, 255, 255) : Color(80, 50, 90), ALIGN_CENTER | ALIGN_MIDDLE);
                if (eraOf(cur) == (int)c.value && !sel) { beginPath(); circle(c.b.x + c.b.w - 6, c.b.y + 5, 3); fillColor(hexc(0xff4f8b)); fill(); }
                continue;
            }
            if (!visible(c) || (c.group != gFM && c.group != gXP && c.group != gUser && c.group != gRock)) continue;
            if (c.type == ctSelect)
            {
                const int idx = (int)c.value;
                const CharacterDef& hc = kChars[kFlavor[idx].ch];
                const bool sel = idx == cur;
                pill(c.b, sel, hover(c.b), hc.accent, c.b.h * 0.5f);
                beginPath(); circle(c.b.x + 11, c.b.y + c.b.h / 2, c.b.h < 21 ? 4.f : 5.f);
                fillColor(hexc(hc.hair)); fill(); strokeWidth(1.f); strokeColor(hexc(hc.hairDark)); stroke();
                label(c.b.x + 21, c.b.y + c.b.h / 2 + 1, c.label, c.b.h < 21 ? 10.5f : 11.5f, sel ? Color(255, 255, 255) : Color(80, 50, 90), ALIGN_LEFT | ALIGN_MIDDLE);
                if (c.group == gUser)
                {
                    const int k = idx - kFirstUser;
                    // slot card background, file name and root note
                    const float y = c.b.y - 4;
                    beginPath(); roundedRect(22, y, 232, 76, 10); strokeWidth(1.f); strokeColor(hexc(hc.accent, 0.35f)); stroke();
                    const char* fn = fSmpPath[k].empty() ? "(empty - press LOAD)" : baseName(fSmpPath[k].c_str());
                    std::snprintf(buf, sizeof(buf), "%s", fn);
                    char* end = const_cast<char*>(utf8advance(buf, 30)); *end = 0;
                    label(30, y + 42, buf, 10.f, fSmpPath[k].empty() ? Color(150, 120, 160) : Color(70, 40, 90), ALIGN_LEFT | ALIGN_BASELINE);
                    char nn[8]; noteName(fRoot[k], nn, sizeof(nn));
                    std::snprintf(buf, sizeof(buf), "root %s", nn);
                    label(201, y + 63, buf, 10.f, Color(80, 50, 90), ALIGN_CENTER | ALIGN_MIDDLE);
                }
            }
            else
            {
                const CharacterDef& hc = kChars[5];
                pill(c.b, false, hover(c.b), hc.accent, 8.f);
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, c.label, 10.f, Color(80, 50, 90), ALIGN_CENTER | ALIGN_MIDDLE);
            }
        }
    }

    static const char* baseName(const char* p)
    {
        const char* b = p;
        for (const char* c = p; *c; ++c) if (*c == '/' || *c == '\\') b = c + 1;
        return b;
    }

    void drawKnob(const Control& c, uint32_t accent, bool small = false)
    {
        const float cx = c.b.x + c.b.w / 2, cy = c.b.y + (small ? 40.f : 32.f), r = small ? 20.f : 23.f;
        const float v = norm(c.param);
        const float a0 = 0.75f * kPi, a1 = 2.25f * kPi, av = a0 + (a1 - a0) * v;
        const bool h = hover(c.b) || fDrag >= 0 && &fCtl[fDrag] == &c;
        beginPath(); circle(cx, cy + 2, r + 4); fillColor(Color(120, 60, 120, 0.15f)); fill();
        beginPath(); circle(cx, cy, r);
        fillPaint(radialGradient(cx - 6, cy - 8, 2, r * 1.4f, Color(255, 255, 255), h ? Color(255, 222, 238) : Color(240, 228, 245))); fill();
        strokeWidth(1.5f); strokeColor(hexc(accent, 0.5f)); stroke();
        lineCap(ROUND);
        beginPath(); arc(cx, cy, r + 6, a0, a1, CW); strokeWidth(4.f); strokeColor(Color(200, 190, 215, 0.7f)); stroke();
        if (v > 0.001f)
        {
            beginPath(); arc(cx, cy, r + 6, a0, av, CW); strokeWidth(4.f);
            strokePaint(linearGradient(cx - r, cy, cx + r, cy, hexc(accent), mixc(accent, 0x8a5cff, 0.6f))); stroke();
        }
        beginPath(); moveTo(cx + std::cos(av) * r * 0.35f, cy + std::sin(av) * r * 0.35f); lineTo(cx + std::cos(av) * r * 0.85f, cy + std::sin(av) * r * 0.85f);
        strokeWidth(3.f); strokeColor(hexc(accent)); stroke();
        label(cx, cy + r + 18, c.label, 10.5f, Color(90, 60, 100), ALIGN_CENTER | ALIGN_BASELINE);
        char buf[32];
        const ParamInfo& p = kParams[c.param];
        if (p.unit[0] == 'd') std::snprintf(buf, sizeof(buf), "%.1f dB", fParams[c.param]);
        else if (c.param == pRelease) std::snprintf(buf, sizeof(buf), "x%.2f", fParams[c.param]);
        else if (c.param == pVGender) std::snprintf(buf, sizeof(buf), "%s", fParams[c.param] < 0.35f ? "male" : (fParams[c.param] > 0.65f ? "female" : "neutral"));
        else if (c.param == pVFormant) std::snprintf(buf, sizeof(buf), "%+.2f", fParams[c.param]);
        else if (c.param == pVVibRate) std::snprintf(buf, sizeof(buf), "%.1f Hz", fParams[c.param]);
        else if (p.max <= 2.f) std::snprintf(buf, sizeof(buf), "%d%%", (int)std::lround(fParams[c.param] / p.max * 100));
        else std::snprintf(buf, sizeof(buf), "%.0f %s", fParams[c.param], p.unit);
        label(cx, cy + r + 30, buf, 10.f, Color(150, 110, 160), ALIGN_CENTER | ALIGN_BASELINE);
    }

    void drawSoundPanel(const CharacterDef& C)
    {
        glassPanel(700, 56, 244, 390, C.accent);
        panelTitle(712, 72, fParams[pVocalMode] > 0.5f ? "VOICE" : "SOUND", C.accent);
        for (int i = 0; i < fCtlCount; ++i)
            if (fCtl[i].type == ctKnob && fCtl[i].param != pDrumVolume && visible(fCtl[i])) drawKnob(fCtl[i], C.accent);
    }

    void drawTextbox(const CharacterDef& C, int inst)
    {
        const float x = 16, y = 456, w = 928, h = 76;
        const float rr = fPixelUI ? 0.f : 12.f;
        beginPath(); roundedRect(x, y, w, h, rr);
        if (fPixelUI) fillColor(Color(16, 16, 64, 0.93f));
        else fillPaint(linearGradient(0, y, 0, y + h, Color(28, 22, 58, 0.86f), Color(50, 30, 80, 0.86f)));
        fill();
        strokeWidth(fPixelUI ? 3.f : 2.f); strokeColor(fPixelUI ? Color(255, 255, 255) : hexc(C.accent, 0.9f)); stroke();
        beginPath(); roundedRect(x + 5, y + 5, w - 10, h - 10, fPixelUI ? 0.f : 9.f); strokeWidth(fPixelUI ? 3.f : 1.f);
        strokeColor(fPixelUI ? hexc(C.accent) : Color(255, 255, 255, 0.35f)); stroke();

        // name plate
        char nm[96];
        std::snprintf(nm, sizeof(nm), "%s", C.name);
        beginPath(); roundedRect(x + 18, y - 18, 190, 30, fPixelUI ? 0.f : 10.f);
        if (fPixelUI) fillColor(hexc(C.accent));
        else fillPaint(linearGradient(x + 18, 0, x + 208, 0, hexc(C.accent), mixc(C.accent, 0x8a5cff, 0.5f)));
        fill();
        strokeWidth(2.f); strokeColor(Color(255, 255, 255)); stroke();
        outlinedLabel(x + 32, y + 3, nm, 17.f, Color(255, 255, 255), hexc(C.accent, 0.6f), ALIGN_LEFT | ALIGN_BASELINE, 1.2f);
        char role[96];
        {
            const int sc = (int)std::lround(fParams[pScene]);
            if (sc > 0 && sc <= kNumScenes) std::snprintf(role, sizeof(role), "\xE2\x9C\xA6 %s: %s", kScenes[sc - 1].name, kScenes[sc - 1].desc);
            else std::snprintf(role, sizeof(role), "%s \xE2\x80\xA2 %s", C.role, kInstrumentNames[inst]);
        }
        label(x + w - 22, y + 24, role, 11.5f, hexc(C.accent, 0.95f), ALIGN_RIGHT | ALIGN_BASELINE);

        // typewriter text (UTF-8 aware)
        const char* line = kFlavor[inst].line;
        const int total = utf8len(line);
        const int shown = (int)((fT - fTypeT) * 38.f);
        const int n = shown < total ? shown : total;
        const char* end = utf8advance(line, n);
        fontFace(NANOVG_DEJAVU_SANS_TTF); fontSize(17.f); textAlign(ALIGN_LEFT | ALIGN_TOP);
        fillColor(Color(0, 0, 0, 0.5f)); textBox(x + 25, y + 31, w - 80, line, end);
        fillColor(Color(255, 255, 255)); textBox(x + 24, y + 30, w - 80, line, end);
        if (n >= total)
        {
            const float b = std::fmod(fT, 1.f) < 0.6f ? 1.f : 0.f;
            label(x + w - 28, y + h - 14 + std::sin(fT * 6) * 2, "\xE2\x96\xBC", 14.f, hexc(C.accent, b), ALIGN_CENTER | ALIGN_BASELINE);
        }
    }

    void drawDrumPanel(const CharacterDef& C)
    {
        const float x = 16, y = 540, w = 928, h = 112;
        glassPanel(x, y, w, h, C.accent);
        const float beat = fParams[pOutBeat];
        fillColor(hexc(C.accent)); drawFlower(32, 564, 8 + beat * 4);
        label(46, 570, "DRUMS", 14.f, Color(70, 40, 90), ALIGN_LEFT | ALIGN_BASELINE);

        const int mode = (int)std::lround(fParams[pDrumMode]);
        const int pat = clampi((int)std::lround(fParams[pDrumPattern]), 0, kNumPatterns - 1);
        const int genre = clampi((int)std::lround(fParams[pDrumGenre]), 0, kNumGenres - 1);
        const int kit = clampi((int)std::lround(fParams[pDrumKit]), 0, kNumKits - 1);
        // kit name between the arrows
        {
            Box kb; kb.x = 306; kb.y = 552; kb.w = 112; kb.h = 24;
            beginPath(); roundedRect(kb.x, kb.y, kb.w, kb.h, 11); fillColor(Color(255, 255, 255, 0.9f)); fill();
            strokeWidth(1.f); strokeColor(hexc(C.accent, 0.6f)); stroke();
            label(kb.x + kb.w / 2, kb.y + 9, "KIT", 7.5f, Color(150, 120, 160), ALIGN_CENTER | ALIGN_MIDDLE);
            label(kb.x + kb.w / 2, kb.y + 17, kDrumKitNames[kit], 10.f, Color(70, 40, 90), ALIGN_CENTER | ALIGN_MIDDLE);
        }
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.group == gEditor || !visible(c)) continue;
            const bool drumCtl = c.param == pDrumMode || c.param == pDrumKit || c.param == pDrumPattern || c.param == pDrumFill || c.param == pDrumGenre;
            if (!drumCtl || c.type == ctKnob) continue;
            bool sel = false;
            const char* text = c.label;
            char buf[48];
            float fs = 10.f;
            uint32_t acc = C.accent;
            switch (c.type)
            {
            case ctToggle: sel = fParams[c.param] > 0.5f; std::snprintf(buf, sizeof(buf), "%s %s", sel ? "\xE2\x9C\x93" : "\xE2\x97\x8B", c.label); text = buf; break;
            case ctEditOpen: sel = fEditor; break;
            case ctKitStep: break;
            case ctGenre: sel = (int)c.value == genre; fs = 8.5f; acc = 0xff8a3a; break;
            case ctGenrePat:
            {
                const int pt = kGenres[genre].pats[(int)c.value];
                sel = pt == pat; text = kPatterns[presetSlot(pt)].name; fs = 8.5f;
                break;
            }
            default:
                sel = (int)std::lround(fParams[c.param]) == (int)c.value;
                if (c.param == pDrumPattern) { fs = 8.5f; acc = 0x8a5cff; }
                break;
            }
            pill(c.b, sel, hover(c.b), acc);
            label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, text, fs,
                  sel ? Color(255, 255, 255) : (acc == 0x8a5cff ? Color(90, 50, 150) : (acc == 0xff8a3a ? Color(150, 70, 30) : Color(80, 50, 90))), ALIGN_CENTER | ALIGN_MIDDLE);
        }

        // step view: kick / snare+clap+rim / hats
        const int step = (int)std::lround(fParams[pOutDrumStep]);
        const int len = patLen(pat);
        const float gx = 606, gy = 588, cw = 16.f, chh = 14.f;
        static const char* const rows[3] = { "K", "S", "H" };
        for (int r = 0; r < 3; ++r)
        {
            label(gx - 6, gy + r * (chh + 3) + chh / 2 + 1, rows[r], 9.f, Color(110, 80, 120), ALIGN_RIGHT | ALIGN_MIDDLE);
            for (int s2 = 0; s2 < 16; ++s2)
            {
                const float cx = gx + s2 * cw, cy = gy + r * (chh + 3);
                float v = 0;
                if (s2 < len)
                {
                    if (r == 0) v = stepVel(patStep(pat, dKick, s2));
                    else if (r == 1) v = std::fmax(stepVel(patStep(pat, dSnare, s2)), std::fmax(stepVel(patStep(pat, dClap, s2)), stepVel(patStep(pat, dRim, s2))));
                    else v = std::fmax(stepVel(patStep(pat, dCHat, s2)), stepVel(patStep(pat, dOHat, s2)));
                }
                beginPath(); roundedRect(cx + 1, cy, cw - 2, chh, 3);
                if (s2 >= len) fillColor(Color(200, 190, 210, 0.25f));
                else if (v > 0) fillColor(hexc(C.accent, (mode > 0 ? 0.35f : 0.2f) + 0.6f * v * (mode > 0 ? 1.f : 0.5f)));
                else fillColor(Color(255, 255, 255, 0.55f));
                fill();
                if (s2 == step && mode > 0) { strokeWidth(2.f); strokeColor(Color(90, 40, 110)); stroke(); }
            }
        }
        const char* hint = mode == 0 ? "Drums off: choose SYNC or ALWAYS" : (mode == 1 ? "Plays when the DAW is playing" : "Plays all the time");
        label(592, 569, hint, 9.f, Color(120, 90, 130), ALIGN_LEFT | ALIGN_BASELINE);

        for (int i = 0; i < fCtlCount; ++i) if (fCtl[i].param == pDrumVolume && fCtl[i].type == ctKnob) drawKnob(fCtl[i], C.accent, true);
    }

    void drawEditor(const CharacterDef& C)
    {
        // dim everything behind
        beginPath(); rect(0, 0, kW, kH); fillColor(Color(20, 10, 40, 0.45f)); fill();
        glassPanel(140, 60, 680, 392, 0x8a5cff, 16.f);
        fillColor(hexc(0x8a5cff)); drawFlower(160, 80, 9);
        label(176, 86, "DRUM EDITOR", 16.f, Color(70, 40, 110), ALIGN_LEFT | ALIGN_BASELINE);
        label(300, 86, "свои барабанные партии \xE2\x80\xA2 сохраняются в проекте", 10.5f, Color(120, 90, 150), ALIGN_LEFT | ALIGN_BASELINE);

        const int u = fEditUser;
        const int pat = clampi((int)std::lround(fParams[pDrumPattern]), 0, kNumPatterns - 1);
        const int step = (int)std::lround(fParams[pOutDrumStep]);
        const int mode = (int)std::lround(fParams[pDrumMode]);
        char buf[64];
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.group != gEditor) continue;
            const bool hv = hover(c.b);
            switch (c.type)
            {
            case ctEditClose:
                beginPath(); circle(c.b.x + 13, c.b.y + 13, 12); fillColor(hv ? Color(255, 120, 160) : Color(200, 170, 230)); fill();
                label(c.b.x + 13, c.b.y + 14, c.label, 13.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
                break;
            case ctEditUser:
            case ctEditLen:
            case ctEditClear:
            case ctEditCopy:
            {
                const bool sel = c.type == ctEditUser && (int)c.value == u;
                pill(c.b, sel, hv, 0x8a5cff);
                const char* t = c.label;
                if (c.type == ctEditLen) { std::snprintf(buf, sizeof(buf), "%d STEPS", fULen[u]); t = buf; }
                if (c.type == ctEditCopy) { std::snprintf(buf, sizeof(buf), "COPY \xE2\x86\x90 %s", kPatterns[presetSlot(isUserPattern(fLastPreset) ? 0 : fLastPreset)].name); t = buf; }
                label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, t, 10.5f, sel ? Color(255, 255, 255) : Color(70, 40, 110), ALIGN_CENTER | ALIGN_MIDDLE);
                break;
            }
            case ctCell:
            {
                const int lane = (int)c.value / 16, st = (int)c.value % 16;
                if (st == 0) label(c.b.x - 10, c.b.y + c.b.h / 2 + 1, kLaneNames[lane], 10.5f, Color(80, 50, 110), ALIGN_RIGHT | ALIGN_MIDDLE);
                const bool off = st >= fULen[u];
                const char chr = fUL[u][lane][st];
                const float v = off ? 0.f : stepVel(chr);
                beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 5);
                if (off) fillColor(Color(200, 190, 210, 0.3f));
                else if (v > 0) fillPaint(linearGradient(0, c.b.y, 0, c.b.y + c.b.h, hexc(C.accent, 0.35f + 0.65f * v), mixc(C.accent, 0x8a5cff, 0.5f, 0.35f + 0.65f * v)));
                else fillColor((st / 4) % 2 ? Color(255, 255, 255, 0.85f) : Color(240, 232, 250, 0.9f));
                fill();
                strokeWidth(hv && !off ? 2.f : 1.f); strokeColor(hv && !off ? hexc(0x8a5cff) : Color(180, 160, 210, 0.7f)); stroke();
                if (chr == 'X' && !off) label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, "!", 12.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
                if (chr == 'g' && !off) label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, "\xC2\xB7", 14.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
                if (pat == kFirstUserPattern + u && mode > 0 && st == step)
                { beginPath(); roundedRect(c.b.x - 1, c.b.y - 1, c.b.w + 2, c.b.h + 2, 6); strokeWidth(2.f); strokeColor(Color(90, 40, 110)); stroke(); }
                break;
            }
            default: break;
            }
        }
        label(480, 438, "клик: обычный \xE2\x86\x92 акцент (!) \xE2\x86\x92 тихий (\xC2\xB7) \xE2\x86\x92 пусто   \xE2\x80\xA2   правый клик \xE2\x80\x94 стереть", 10.5f, Color(110, 80, 140), ALIGN_CENTER | ALIGN_BASELINE);
    }

    static int utf8len(const char* s) { int n = 0; for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++n; return n; }
    static const char* utf8advance(const char* s, int n)
    {
        while (*s && n > 0) { ++s; while ((*s & 0xC0) == 0x80) ++s; --n; }
        return s;
    }

    // ================================================================== state
    float fParams[kParamCount];
    std::chrono::steady_clock::time_point fStart;
    float fT = 0, fSwitchT = -10, fTypeT = 0;
    int fPrevChar = 0;
    bool fInited = false;

    static constexpr int kMaxCtl = 400;
    Control fCtl[kMaxCtl];
    int fEraTab = 0, fLastPreset = 0, fEditUser = 0;
    float fLX = 0, fLY = 0, fHairSway = 0, fMouseT = -100, fLastFrameT = 0;
    int fForceBlink = -1, fForceMouth = -1;
    bool fExport = false, fExportGirl = false, fExportWhite = false;
    bool fPixelUI = true, fPrevPixel = true;
    float fHeroU = 92.f, fHeroY = 214.f;
    bool fDrawingIdol = false, fEditing = false, fForceVocal = false;
    bool fRuLayout = false, fScenesOpen = false, fStoryMenu = false, fAutoRu = false;
    float fLastPasteT = -10;
    int fStory = -1, fStoryStep = 0, fStoryForce = -1; float fStoryStartVal = 0, fStoryT = 0, fStoryDoneT = -1;
    Box fStoryNext, fStoryExit;
    int fIdolShown = -1, fPrevIdol = 0; float fIdolSwitchT = -10;
    std::string fLyrics, fEditBuf;
    std::vector<Syllable> fSyl;
    NanoImage fPxBgImg[kPxCount];
    NanoImage fPxGirlImg[kPxCount][3];
    bool fEditor = false;
    std::string fSmpPath[4];
    int fRoot[4] = {60, 60, 60, 60};
    char fUL[4][kDrumLanes][17];
    int fULen[4] = {16, 16, 16, 16};
    int fCtlCount = 0;
    int fDrag = -1;
    float fDragY = 0, fDragV = 0, fMouseX = -1, fMouseY = -1;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroNovelFMUI)
};

UI* createUI()
{
    return new RetroNovelFMUI();
}

END_NAMESPACE_DISTRHO
