// Retro Novel FM - visual-novel style UI (NanoVG)
#include "DistrhoUI.hpp"
#include "RetroData.hpp"
#include "RetroCharacters.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

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

enum CtlType { ctSelect, ctKnob, ctToggle, ctAction };
enum ActionId { aPrev, aNext, aRandom };

struct Control {
    CtlType type; int param; float value; Box b; const char* label;
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

        buildLayout();
       #ifdef RETRO_SCREENSHOT
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
        if (index == pOutActivity && std::getenv("RETRO_ACT")) return;
        if ((index == pDrumMode || index == pDrumPattern || index == pOutDrumStep) && std::getenv("RETRO_DRUMS")) return;
       #endif
        if (index == pInstrument)
        {
            const int ni = clampi((int)std::lround(value), 0, 23);
            const int oi = clampi((int)std::lround(fParams[pInstrument]), 0, 23);
            if (ni != oi || !fInited) { fPrevChar = kFlavor[oi].ch; fSwitchT = now(); fTypeT = now(); }
            fInited = true;
        }
        fParams[index] = value;
        repaint();
    }

    void uiIdle() override { repaint(); }

    // ------------------------------------------------------------------ drawing
    void onNanoDisplay() override
    {
        const float sc = getHeight() / kH;
        fT = now();
        save();
        scale(sc, sc);

        const int inst = clampi((int)std::lround(fParams[pInstrument]), 0, 23);
        const int ch = kFlavor[inst].ch;
        const CharacterDef& C = kChars[ch];
        const float sw = std::fmin(1.f, (fT - fSwitchT) / 0.45f);

        // background: crossfade scenes when the heroine changes
        drawScene(kChars[fPrevChar], 1.f);
        if (sw < 1.f) { globalAlpha(sw); drawScene(C, sw); globalAlpha(1.f); }
        else drawScene(C, 1.f);

        // heroine
        const float ease = 1.f - (1.f - sw) * (1.f - sw);
        if (sw < 1.f && fPrevChar != ch) {
            globalAlpha(1.f - ease);
            drawHeroine(kChars[fPrevChar], 480.f - 60.f * ease, 1.f);
        }
        globalAlpha(ease);
        drawHeroine(C, 480.f + 60.f * (1.f - ease), ease);
        globalAlpha(1.f);

        drawNotes(C);
        drawTopBar(C);
        drawInstrumentPanel();
        drawSoundPanel(C);
        drawTextbox(C, inst);
        drawDrumPanel(C);

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
        for (int i = 0; i < fCtlCount; ++i)
        {
            Control& c = fCtl[i];
            if (!c.b.hit(px, py)) continue;
            switch (c.type)
            {
            case ctSelect: setParam(c.param, c.value); break;
            case ctToggle: setParam(c.param, fParams[c.param] > 0.5f ? 0.f : 1.f); break;
            case ctAction:
            {
                const int cur = clampi((int)std::lround(fParams[pInstrument]), 0, 23);
                int n = cur;
                if (c.value == aPrev) n = (cur + 23) % 24;
                else if (c.value == aNext) n = (cur + 1) % 24;
                else { n = (int)(hash01((int)(fT * 1000)) * 24) % 24; if (n == cur) n = (n + 7) % 24; }
                setParam(pInstrument, (float)n);
                break;
            }
            case ctKnob:
                if (ev.button == 3) { setParam(c.param, kParams[c.param].def); break; }
                fDrag = i; fDragY = py; fDragV = norm(c.param);
                editParameter(c.param, true);
                break;
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
        fMouseX = px; fMouseY = py;
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

    bool onScroll(const ScrollEvent& ev) override
    {
        const float sc = getHeight() / kH;
        const float px = ev.pos.getX() / sc, py = ev.pos.getY() / sc;
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != ctKnob || !c.b.hit(px, py)) continue;
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
    void addCtl(CtlType t, int param, float value, float x, float y, float w, float h, const char* label = nullptr)
    {
        if (fCtlCount >= 96) return;
        Control& c = fCtl[fCtlCount++];
        c.type = t; c.param = param; c.value = value; c.b.x = x; c.b.y = y; c.b.w = w; c.b.h = h; c.label = label;
    }
    bool hover(const Box& b) const { return b.hit(fMouseX, fMouseY); }

    void buildLayout()
    {
        // instrument grid (left panel)
        for (int i = 0; i < 24; ++i)
        {
            const int col = i % 2, row = i / 2;
            addCtl(ctSelect, pInstrument, (float)i, 26.f + col * 117.f, 84.f + row * 29.f, 112.f, 25.f, kInstrumentNames[i]);
        }
        // knobs (right panel)
        static const int knobs[8] = { pVolume, pBrightness, pRelease, pVibDepth, pLofi, pChorus, pReverb, pVelSens };
        static const char* const klab[8] = { "VOLUME", "BRIGHT", "RELEASE", "VIBRATO", "LO-FI", "CHORUS", "REVERB", "VELOCITY" };
        for (int i = 0; i < 8; ++i)
        {
            const int col = i % 2, row = i / 2;
            addCtl(ctKnob, knobs[i], 0, 716.f + col * 114.f, 84.f + row * 88.f, 104.f, 84.f, klab[i]);
        }
        // top bar actions
        addCtl(ctAction, pInstrument, aPrev,   640.f, 10.f, 40.f, 26.f, "\xE2\x97\x80");
        addCtl(ctAction, pInstrument, aNext,   684.f, 10.f, 40.f, 26.f, "\xE2\x96\xB6");
        addCtl(ctAction, pInstrument, aRandom, 728.f, 10.f, 104.f, 26.f, "RANDOM");
        // drums
        for (int i = 0; i < 3; ++i) addCtl(ctSelect, pDrumMode, (float)i, 120.f + i * 70.f, 554.f, 66.f, 24.f, i == 0 ? "OFF" : (i == 1 ? "SYNC" : "ALWAYS"));
        for (int i = 0; i < 3; ++i) addCtl(ctSelect, pDrumKit, (float)i, 350.f + i * 74.f, 554.f, 70.f, 24.f, i == 0 ? "PC-98" : (i == 1 ? "808" : "8-BIT"));
        addCtl(ctToggle, pDrumFill, 0, 584.f, 554.f, 74.f, 24.f, "FILLS");
        addCtl(ctKnob, pDrumVolume, 0, 868.f, 540.f, 72.f, 104.f, "DRUM VOL");
        for (int i = 0; i < 12; ++i)
        {
            const int col = i % 6, row = i / 6;
            addCtl(ctSelect, pDrumPattern, (float)i, 30.f + col * 88.f, 588.f + row * 28.f, 84.f, 24.f, kPatterns[i].name);
        }
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
    void drawScene(const CharacterDef& C, float /*a*/)
    {
        // sky
        beginPath(); rect(0, 0, kW, kH);
        fillPaint(linearGradient(0, 0, 0, 470, hexc(C.skyTop), hexc(C.skyBottom)));
        fill();

        // stars for night / sparkles for dusk
        if (C.ce == ceMoon || C.pt == ptSparkle)
        {
            for (int i = 0; i < 70; ++i)
            {
                const float x = hash01(i * 3 + 1) * kW, y = hash01(i * 3 + 2) * 330.f;
                const float tw = 0.45f + 0.55f * std::sin(fT * (1.2f + hash01(i) * 2.f) + i);
                beginPath(); circle(x, y, 0.8f + hash01(i * 7) * 1.4f);
                fillColor(Color(255, 255, 240, (C.ce == ceMoon ? 0.85f : 0.45f) * tw)); fill();
            }
        }
        // sun / moon
        if (C.ce == ceSun)
        {
            beginPath(); circle(800, 96, 140);
            fillPaint(radialGradient(800, 96, 20, 140, Color(255, 255, 230, 0.9f), Color(255, 255, 230, 0.f))); fill();
            beginPath(); circle(800, 96, 34); fillColor(Color(255, 255, 240, 0.95f)); fill();
        }
        else if (C.ce == ceSunset)
        {
            beginPath(); circle(250, 360, 230);
            fillPaint(radialGradient(250, 360, 40, 230, Color(255, 230, 160, 0.85f), Color(255, 160, 120, 0.f))); fill();
            beginPath(); circle(250, 360, 62); fillColor(Color(255, 236, 190, 0.95f)); fill();
        }
        else if (C.ce == ceMoon)
        {
            beginPath(); circle(790, 110, 110);
            fillPaint(radialGradient(790, 110, 30, 110, Color(220, 230, 255, 0.5f), Color(200, 210, 255, 0.f))); fill();
            beginPath(); circle(790, 110, 38); fillColor(Color(250, 248, 230)); fill();
            beginPath(); circle(774, 100, 34); fillColor(hexc(C.skyTop, 0.92f)); fill();
        }
        // clouds
        if (C.ce != ceMoon)
        {
            for (int i = 0; i < 5; ++i)
            {
                float x = std::fmod(hash01(i + 40) * 1300.f + fT * (6.f + i * 2.f), 1300.f) - 170.f;
                const float y = 60.f + hash01(i + 60) * 200.f, s = 0.7f + hash01(i + 80) * 0.7f;
                const float al = C.ce == ceCloudy ? 0.75f : 0.55f;
                fillColor(Color(255, 255, 255, al));
                beginPath();
                ellipse(x, y, 70 * s, 22 * s); ellipse(x - 40 * s, y + 6 * s, 44 * s, 16 * s);
                ellipse(x + 42 * s, y + 4 * s, 50 * s, 18 * s); ellipse(x + 8 * s, y - 14 * s, 40 * s, 22 * s);
                fill();
            }
        }
        // distant city
        for (int i = 0; i < 22; ++i)
        {
            const float bw = 30 + hash01(i + 100) * 50, bh = 40 + hash01(i + 120) * 110;
            const float bx = i * 46.f - 20 + hash01(i + 140) * 10;
            beginPath(); rect(bx, 400 - bh, bw, bh + 80);
            fillColor(mixc(C.horizon, 0x2a2440, 0.35f + hash01(i) * 0.15f, 0.55f)); fill();
            if (C.ce == ceMoon || C.ce == ceSunset)
                for (int w = 0; w < 6; ++w)
                {
                    if (hash01(i * 13 + w) < 0.45f) continue;
                    beginPath(); rect(bx + 6 + (w % 2) * (bw * 0.45f), 400 - bh + 10 + (w / 2) * 16, 6, 7);
                    fillColor(Color(255, 230, 150, 0.75f)); fill();
                }
        }
        // rooftop floor + fence
        beginPath(); rect(0, 400, kW, 260);
        fillPaint(linearGradient(0, 400, 0, 660, mixc(C.horizon, 0x6a6f88, 0.5f), mixc(C.horizon, 0x2d2a3c, 0.7f)));
        fill();
        strokeColor(mixc(C.horizon, 0x303040, 0.65f, 0.8f));
        strokeWidth(5.f);
        beginPath(); moveTo(0, 318); lineTo(kW, 318); stroke();
        beginPath(); moveTo(0, 398); lineTo(kW, 398); stroke();
        strokeWidth(2.5f);
        for (int i = 0; i < 49; ++i) { beginPath(); moveTo(i * 20.f, 318); lineTo(i * 20.f, 398); stroke(); }
        strokeWidth(1.f);
        strokeColor(mixc(C.horizon, 0x303040, 0.6f, 0.35f));
        for (int i = 0; i < 24; ++i) { beginPath(); moveTo(i * 40.f, 318); lineTo(i * 40.f + 40.f, 398); stroke(); }

        // trees (sakura / maple / snowy / green) in the corners
        drawBranch(C, -20.f, -10.f, 1.f);
        drawBranch(C, kW + 20.f, -10.f, -1.f);

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
        const float breathe = std::sin(fT * 2.1f) * 2.2f;
        const float bob = beat * beat * 5.f;
        const float u = 92.f;               // pixels per head-radius
        const float cy = 214.f + breathe * 0.4f + bob;

        save();
        translate(cx, cy);
        scale(u, u);
        const float sway = std::sin(fT * 0.9f) * 0.012f + (act > 0.05f ? std::sin(fT * 3.f) * 0.02f * std::fmin(1.f, act * 2) : 0.f);

        // ---- back layer: hair behind body
        drawBackHair(C);

        // ---- body
        save();
        translate(0, breathe * 0.004f);
        drawBody(C);
        restore();

        // ---- head (sways slightly)
        save();
        translate(0, 0.7f); rotate(sway); translate(0, -0.7f);
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
            moveTo(-0.6f, -0.95f);
            bezierTo(-1.15f, -0.75f, -1.22f, 0.2f, -1.3f, 1.1f);
            bezierTo(-1.38f, 1.9f, -1.45f, 2.4f, -1.5f, 2.9f);
            lineTo(1.5f, 2.9f);
            bezierTo(1.45f, 2.4f, 1.38f, 1.9f, 1.3f, 1.1f);
            bezierTo(1.22f, 0.2f, 1.15f, -0.75f, 0.6f, -0.95f);
            closePath();
            fillPaint(hairPaint(C, -0.4f, 2.9f)); fill(); hairStroke(C);
            break;
        case hsWavy:
            beginPath();
            moveTo(-0.6f, -0.95f);
            bezierTo(-1.2f, -0.7f, -1.1f, 0.6f, -1.45f, 1.4f);
            bezierTo(-1.75f, 1.9f, -1.3f, 2.3f, -1.6f, 2.9f);
            lineTo(1.6f, 2.9f);
            bezierTo(1.3f, 2.3f, 1.75f, 1.9f, 1.45f, 1.4f);
            bezierTo(1.1f, 0.6f, 1.2f, -0.7f, 0.6f, -0.95f);
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
            beginPath(); moveTo(-0.55f, -0.95f); bezierTo(-1.15f, -0.7f, -1.05f, 0.6f, -0.75f, 0.9f); lineTo(0.75f, 0.9f);
            bezierTo(1.05f, 0.6f, 1.15f, -0.7f, 0.55f, -0.95f); closePath();
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
            beginPath(); moveTo(-0.55f, -0.95f); bezierTo(-1.12f, -0.7f, -1.0f, 0.5f, -0.7f, 0.95f); lineTo(0.7f, 0.95f);
            bezierTo(1.0f, 0.5f, 1.12f, -0.7f, 0.55f, -0.95f); closePath();
            fillPaint(hairPaint(C, -0.3f, 0.95f)); fill(); hairStroke(C);
            break;
        }
        case hsBob:
        case hsBraids:
        case hsBun:
            beginPath(); moveTo(-0.55f, -0.95f); bezierTo(-1.2f, -0.7f, -1.12f, 0.75f, -0.82f, 1.02f); lineTo(0.82f, 1.02f);
            bezierTo(1.12f, 0.75f, 1.2f, -0.7f, 0.55f, -0.95f); closePath();
            fillPaint(hairPaint(C, -0.4f, 1.0f)); fill(); hairStroke(C);
            break;
        }
    }

    void drawBody(const CharacterDef& C)
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
        if (C.hs == hsBun)
        {
            beginPath(); circle(0.0f, -1.08f, 0.42f); fillPaint(hairPaint(C, -1.5f, -0.7f)); fill(); hairStroke(C);
            strokeWidth(0.018f); strokeColor(hexc(C.hairDark, 0.6f));
            beginPath(); arc(0, -1.08f, 0.28f, 3.4f, 5.6f, CW); stroke();
        }
    }

    void drawFace(const CharacterDef& C, float act)
    {
        const Color skin(255, 236, 224);
        // face
        beginPath();
        moveTo(-0.84f, -0.2f);
        bezierTo(-0.86f, 0.42f, -0.48f, 0.84f, 0.0f, 0.98f);
        bezierTo(0.48f, 0.84f, 0.86f, 0.42f, 0.84f, -0.2f);
        bezierTo(0.84f, -1.0f, -0.84f, -1.0f, -0.84f, -0.2f);
        closePath();
        fillPaint(radialGradient(0, 0.1f, 0.3f, 1.1f, skin, Color(250, 215, 200)));
        fill();
        strokeWidth(0.018f); strokeColor(Color(190, 130, 115, 0.8f)); stroke();
        // ears
        for (int s = -1; s <= 1; s += 2)
        {
            beginPath(); ellipse(s * 0.84f, 0.18f, 0.09f, 0.16f); fillColor(Color(248, 214, 200)); fill();
        }
        // hair shadow on forehead
        beginPath(); ellipse(0, -0.18f, 0.8f, 0.2f); fillColor(Color(220, 160, 160, 0.35f)); fill();

        // blush
        for (int s = -1; s <= 1; s += 2)
        {
            beginPath(); ellipse(s * 0.5f, 0.46f, 0.16f, 0.065f);
            fillColor(Color(255, 120, 140, 0.32f + 0.15f * std::fmin(1.f, act * 2))); fill();
            strokeWidth(0.012f); strokeColor(Color(240, 110, 130, 0.5f));
            for (int k = 0; k < 3; ++k) { beginPath(); moveTo(s * 0.43f + k * 0.06f, 0.43f); lineTo(s * 0.41f + k * 0.06f, 0.49f); stroke(); }
        }

        // eyes
        const float bt = std::fmod(fT + 0.37f * (float)(&C - kChars), 4.3f);
        const bool blink = bt > 4.15f;
        const bool happy = act > 0.35f && std::fmod(fT, 2.5f) < 0.6f;
        for (int s = -1; s <= 1; s += 2) drawEye(C, s * 0.38f, 0.17f, (float)s, blink, happy);

        // brows
        lineCap(ROUND);
        strokeWidth(0.03f); strokeColor(hexc(C.hairDark, 0.85f));
        for (int s = -1; s <= 1; s += 2) { beginPath(); moveTo(s * 0.22f, -0.13f); quadTo(s * 0.38f, -0.2f, s * 0.54f, -0.15f); stroke(); }

        // nose
        strokeWidth(0.018f); strokeColor(Color(215, 150, 140));
        beginPath(); moveTo(0.02f, 0.4f); lineTo(-0.01f, 0.45f); stroke();

        // mouth
        const float open = act > 0.03f ? (0.5f + 0.5f * std::sin(fT * 13.f)) * std::fmin(1.f, act * 2.5f) : 0.f;
        if (open > 0.08f)
        {
            beginPath(); moveTo(-0.09f, 0.6f); quadTo(0, 0.6f + 0.02f, 0.09f, 0.6f); quadTo(0.06f, 0.66f + open * 0.1f, 0, 0.67f + open * 0.1f);
            quadTo(-0.06f, 0.66f + open * 0.1f, -0.09f, 0.6f); closePath();
            fillColor(Color(150, 40, 60)); fill();
            beginPath(); ellipse(0, 0.65f + open * 0.08f, 0.045f, 0.02f + open * 0.02f); fillColor(Color(255, 130, 150)); fill();
        }
        else
        {
            strokeWidth(0.022f); strokeColor(Color(170, 70, 80));
            beginPath(); moveTo(-0.08f, 0.61f); quadTo(0, 0.67f, 0.08f, 0.61f); stroke();
        }
    }

    void drawEye(const CharacterDef& C, float x, float y, float dir, bool blink, bool happy)
    {
        save();
        translate(x, y); scale(dir, 1.f);
        lineCap(ROUND); lineJoin(ROUND);
        const Color lash(40, 24, 30);
        if (blink || happy)
        {
            strokeWidth(0.045f); strokeColor(lash);
            beginPath();
            if (happy) { moveTo(-0.18f, 0.08f); quadTo(0.0f, -0.12f, 0.2f, 0.06f); }
            else       { moveTo(-0.19f, 0.03f); quadTo(0.0f, 0.12f, 0.21f, 0.0f); }
            stroke();
            restore();
            return;
        }
        // sclera
        beginPath(); moveTo(-0.19f, -0.02f); bezierTo(-0.12f, -0.2f, 0.12f, -0.2f, 0.21f, -0.06f);
        bezierTo(0.22f, 0.12f, 0.12f, 0.24f, 0.0f, 0.24f); bezierTo(-0.14f, 0.24f, -0.21f, 0.12f, -0.19f, -0.02f); closePath();
        fillColor(Color(255, 255, 255)); fill();
        // iris
        beginPath(); ellipse(0.01f, 0.06f, 0.14f, 0.18f);
        fillPaint(linearGradient(0, -0.12f, 0, 0.24f, hexc(C.eyeDark), hexc(C.eye))); fill();
        beginPath(); ellipse(0.01f, 0.07f, 0.075f, 0.1f); fillColor(mixc(C.eyeDark, 0x000000, 0.45f)); fill();
        beginPath(); ellipse(0.01f, 0.17f, 0.09f, 0.045f); fillColor(mixc(C.eye, 0xffffff, 0.5f, 0.55f)); fill();
        // highlights
        const float tw = 0.85f + 0.15f * std::sin(fT * 3.f);
        beginPath(); ellipse(-0.05f, -0.01f, 0.05f, 0.055f); fillColor(Color(255, 255, 255, tw)); fill();
        beginPath(); circle(0.07f, 0.14f, 0.022f); fillColor(Color(255, 255, 255, 0.85f)); fill();
        // upper lash line with outer wing
        beginPath(); moveTo(-0.21f, 0.0f); bezierTo(-0.12f, -0.21f, 0.12f, -0.22f, 0.24f, -0.06f); lineTo(0.3f, -0.1f); lineTo(0.25f, -0.01f);
        strokeWidth(0.05f); strokeColor(lash); stroke();
        strokeWidth(0.02f);
        beginPath(); moveTo(0.22f, -0.07f); lineTo(0.31f, -0.04f); stroke();
        // lower lash
        strokeWidth(0.016f); strokeColor(Color(90, 50, 60, 0.8f));
        beginPath(); moveTo(-0.12f, 0.23f); quadTo(0.02f, 0.27f, 0.15f, 0.2f); stroke();
        restore();
    }

    void drawFrontHair(const CharacterDef& C)
    {
        lineJoin(ROUND);
        // side locks
        if (C.hs != hsBraids)
            for (int s = -1; s <= 1; s += 2)
            {
                const float len = (C.hs == hsHime) ? 0.85f : (C.hs == hsBob ? 0.95f : 1.25f);
                beginPath();
                moveTo(s * 0.72f, -0.45f);
                bezierTo(s * 1.0f, 0.1f, s * 0.98f, len * 0.6f, s * 0.9f, len);
                if (C.hs == hsHime) lineTo(s * 0.68f, len);
                else bezierTo(s * 0.86f, len * 0.7f, s * 0.78f, len * 0.4f, s * 0.7f, len * 0.75f);
                bezierTo(s * 0.72f, 0.3f, s * 0.68f, 0.0f, s * 0.6f, -0.3f);
                closePath();
                fillPaint(hairPaint(C, -0.45f, len)); fill(); hairStroke(C);
            }

        // cap + bangs
        beginPath();
        moveTo(-0.92f, 0.05f);
        bezierTo(-1.08f, -1.25f, 1.08f, -1.25f, 0.92f, 0.05f);
        if (C.hs == hsHime)
        {
            // straight-cut bangs
            lineTo(0.86f, -0.02f); lineTo(0.7f, 0.04f);
            for (int k = 0; k < 8; ++k)
            {
                const float x0 = 0.7f - k * 0.175f;
                lineTo(x0 - 0.08f, -0.06f); lineTo(x0 - 0.175f, 0.03f);
            }
        }
        else
        {
            static const float pts[][4] = {
                // control x,y then tip x,y   (right -> left)
                { 0.88f, -0.2f, 0.74f, 0.16f }, { 0.66f, -0.1f, 0.58f, -0.08f },
                { 0.55f, -0.05f, 0.44f, 0.1f }, { 0.38f, -0.12f, 0.3f, -0.12f },
                { 0.26f, -0.02f, 0.14f, 0.12f }, { 0.06f, -0.15f, 0.0f, -0.12f },
                { -0.06f, -0.02f, -0.18f, 0.14f }, { -0.22f, -0.12f, -0.32f, -0.1f },
                { -0.4f, 0.0f, -0.5f, 0.1f }, { -0.55f, -0.08f, -0.62f, -0.08f },
                { -0.7f, 0.02f, -0.8f, 0.18f }, { -0.82f, -0.05f, -0.92f, 0.05f },
            };
            for (const auto& p : pts) quadTo(p[0], p[1], p[2], p[3]);
        }
        closePath();
        fillPaint(linearGradient(0, -1.0f, 0, 0.1f, hexc(C.hair), mixc(C.hair, C.hairDark, 0.35f)));
        fill(); hairStroke(C);

        // strand lines
        strokeWidth(0.014f); strokeColor(hexc(C.hairDark, 0.45f));
        for (int k = -3; k <= 3; ++k) { beginPath(); moveTo(k * 0.12f, -0.85f); quadTo(k * 0.2f, -0.45f, k * 0.24f, -0.12f); stroke(); }

        // angel ring highlight: soft glossy band broken by strands
        for (int k = 0; k < 5; ++k)
        {
            const float a0 = 3.62f + k * 0.4f, a1 = a0 + 0.3f;
            beginPath();
            arc(0, -0.08f, 0.76f, a0, a1, CW);
            arc(0, -0.08f, 0.66f, a1 - 0.02f, a0 + 0.02f, CCW);
            closePath();
            fillColor(hexc(C.hairLight, 0.55f)); fill();
        }
        beginPath(); ellipse(-0.3f, -0.7f, 0.1f, 0.03f); fillColor(Color(255, 255, 255, 0.55f)); fill();
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
        label(250, 30, "FM synth  \xE2\x99\xA5  drum machine  \xE2\x99\xA5  PC-98 style", 12.f, Color(120, 90, 130), ALIGN_LEFT | ALIGN_BASELINE);

        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != ctAction) continue;
            const bool h = hover(c.b);
            beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 13);
            fillPaint(linearGradient(0, c.b.y, 0, c.b.y + c.b.h, hexc(C.accent, h ? 1.f : 0.85f), mixc(C.accent, 0x803060, 0.3f)));
            fill();
            strokeWidth(1.5f); strokeColor(Color(255, 255, 255, 0.9f)); stroke();
            label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, c.label, 13.f, Color(255, 255, 255), ALIGN_CENTER | ALIGN_MIDDLE);
        }
        label(940, 30, "v2.0", 11.f, Color(150, 120, 160), ALIGN_RIGHT | ALIGN_BASELINE);
    }

    void drawInstrumentPanel()
    {
        glassPanel(16, 56, 244, 390, 0xff7fae);
        panelTitle(28, 72, "INSTRUMENT", 0xff7fae);
        const int cur = clampi((int)std::lround(fParams[pInstrument]), 0, 23);
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.type != ctSelect || c.param != pInstrument) continue;
            const int idx = (int)c.value;
            const CharacterDef& hc = kChars[kFlavor[idx].ch];
            const bool sel = idx == cur, h = hover(c.b);
            beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 12);
            if (sel) fillPaint(linearGradient(0, c.b.y, 0, c.b.y + c.b.h, hexc(hc.accent), mixc(hc.accent, 0x60305a, 0.35f)));
            else fillColor(h ? Color(255, 225, 238, 0.95f) : Color(255, 255, 255, 0.75f));
            fill();
            strokeWidth(1.f); strokeColor(sel ? Color(255, 255, 255) : hexc(hc.accent, 0.45f)); stroke();
            // colour dot = which heroine plays it
            beginPath(); circle(c.b.x + 11, c.b.y + c.b.h / 2, 5);
            fillColor(hexc(hc.hair)); fill(); strokeWidth(1.f); strokeColor(hexc(hc.hairDark)); stroke();
            label(c.b.x + 21, c.b.y + c.b.h / 2 + 1, c.label, 11.5f, sel ? Color(255, 255, 255) : Color(80, 50, 90), ALIGN_LEFT | ALIGN_MIDDLE);
        }
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
        else if (p.max <= 2.f) std::snprintf(buf, sizeof(buf), "%d%%", (int)std::lround(fParams[c.param] / p.max * 100));
        else std::snprintf(buf, sizeof(buf), "%.0f %s", fParams[c.param], p.unit);
        label(cx, cy + r + 30, buf, 10.f, Color(150, 110, 160), ALIGN_CENTER | ALIGN_BASELINE);
    }

    void drawSoundPanel(const CharacterDef& C)
    {
        glassPanel(700, 56, 244, 390, C.accent);
        panelTitle(712, 72, "SOUND", C.accent);
        for (int i = 0; i < fCtlCount; ++i)
            if (fCtl[i].type == ctKnob && fCtl[i].param != pDrumVolume) drawKnob(fCtl[i], C.accent);
    }

    void drawTextbox(const CharacterDef& C, int inst)
    {
        const float x = 16, y = 456, w = 928, h = 76;
        beginPath(); roundedRect(x, y, w, h, 12);
        fillPaint(linearGradient(0, y, 0, y + h, Color(28, 22, 58, 0.86f), Color(50, 30, 80, 0.86f))); fill();
        strokeWidth(2.f); strokeColor(hexc(C.accent, 0.9f)); stroke();
        beginPath(); roundedRect(x + 5, y + 5, w - 10, h - 10, 9); strokeWidth(1.f); strokeColor(Color(255, 255, 255, 0.35f)); stroke();

        // name plate
        char nm[96];
        std::snprintf(nm, sizeof(nm), "%s", C.name);
        beginPath(); roundedRect(x + 18, y - 18, 190, 30, 10);
        fillPaint(linearGradient(x + 18, 0, x + 208, 0, hexc(C.accent), mixc(C.accent, 0x8a5cff, 0.5f))); fill();
        strokeWidth(2.f); strokeColor(Color(255, 255, 255)); stroke();
        outlinedLabel(x + 32, y + 3, nm, 17.f, Color(255, 255, 255), hexc(C.accent, 0.6f), ALIGN_LEFT | ALIGN_BASELINE, 1.2f);
        char role[96];
        std::snprintf(role, sizeof(role), "%s \xE2\x80\xA2 %s", C.role, kInstrumentNames[inst]);
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
        fillColor(hexc(C.accent)); drawFlower(36, 566, 9 + beat * 4);
        label(52, 572, "DRUMS", 15.f, Color(70, 40, 90), ALIGN_LEFT | ALIGN_BASELINE);

        const int mode = (int)std::lround(fParams[pDrumMode]);
        for (int i = 0; i < fCtlCount; ++i)
        {
            const Control& c = fCtl[i];
            if (c.param != pDrumMode && c.param != pDrumKit && c.param != pDrumPattern && c.param != pDrumFill) continue;
            bool sel;
            if (c.type == ctToggle) sel = fParams[c.param] > 0.5f;
            else sel = (int)std::lround(fParams[c.param]) == (int)c.value;
            const bool hv = hover(c.b);
            beginPath(); roundedRect(c.b.x, c.b.y, c.b.w, c.b.h, 11);
            if (sel) fillPaint(linearGradient(0, c.b.y, 0, c.b.y + c.b.h, hexc(C.accent), mixc(C.accent, 0x60305a, 0.35f)));
            else fillColor(hv ? Color(255, 225, 238, 0.95f) : Color(255, 255, 255, 0.8f));
            fill();
            strokeWidth(1.f); strokeColor(sel ? Color(255, 255, 255) : hexc(C.accent, 0.45f)); stroke();
            char buf[48];
            if (c.type == ctToggle) std::snprintf(buf, sizeof(buf), "%s %s", sel ? "\xE2\x9C\x93" : "\xE2\x97\x8B", c.label);
            else std::snprintf(buf, sizeof(buf), "%s", c.label);
            label(c.b.x + c.b.w / 2, c.b.y + c.b.h / 2 + 1, buf, 11.f, sel ? Color(255, 255, 255) : Color(80, 50, 90), ALIGN_CENTER | ALIGN_MIDDLE);
        }

        // step view: kick / snare+clap / hats
        const int pat = clampi((int)std::lround(fParams[pDrumPattern]), 0, 11);
        const DrumPattern& P = kPatterns[pat];
        const int step = (int)std::lround(fParams[pOutDrumStep]);
        const float gx = 566, gy = 590, cw = 290.f / 16.f, chh = 14.f;
        static const char* const rows[3] = { "K", "S", "H" };
        for (int r = 0; r < 3; ++r)
        {
            label(gx - 8, gy + r * (chh + 3) + chh / 2 + 1, rows[r], 10.f, Color(110, 80, 120), ALIGN_RIGHT | ALIGN_MIDDLE);
            for (int s = 0; s < 16; ++s)
            {
                const float cx = gx + s * cw, cy = gy + r * (chh + 3);
                float v = 0;
                if (s < P.len)
                {
                    if (r == 0) v = stepVel(P.lane[dKick][s]);
                    else if (r == 1) v = std::fmax(stepVel(P.lane[dSnare][s]), std::fmax(stepVel(P.lane[dClap][s]), stepVel(P.lane[dRim][s])));
                    else v = std::fmax(stepVel(P.lane[dCHat][s]), stepVel(P.lane[dOHat][s]));
                }
                beginPath(); roundedRect(cx + 1, cy, cw - 2, chh, 3);
                const bool now = (s == step) && mode > 0;
                if (s >= P.len) fillColor(Color(200, 190, 210, 0.25f));
                else if (v > 0) fillColor(hexc(C.accent, (mode > 0 ? 0.35f : 0.2f) + 0.6f * v * (mode > 0 ? 1.f : 0.5f)));
                else fillColor(Color(255, 255, 255, 0.55f));
                fill();
                if (now) { strokeWidth(2.f); strokeColor(Color(90, 40, 110)); stroke(); }
                if ((s & 3) == 0 && s < P.len) { beginPath(); rect(cx + 1, cy + chh + 1, cw - 2, 1); fillColor(Color(150, 120, 170, 0.5f)); fill(); }
            }
        }
        const char* hint = mode == 0 ? "Drums off: choose SYNC or ALWAYS" : (mode == 1 ? "Plays when the DAW is playing" : "Plays all the time");
        label(668, 571, hint, 10.f, Color(120, 90, 130), ALIGN_LEFT | ALIGN_BASELINE);

        for (int i = 0; i < fCtlCount; ++i) if (fCtl[i].param == pDrumVolume) drawKnob(fCtl[i], C.accent, true);
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

    Control fCtl[96];
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
