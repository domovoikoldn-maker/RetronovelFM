// RetroWinKeys.cpp - keyboard helpers that ask Windows directly for the typed character.
// Many DAWs forward only the English (US) key to plugins, or nothing at all for non-English
// layouts. Here we look at which physical key is down and translate it with the keyboard layout
// the user currently has active in the host window (Russian, Ukrainian, ...).
#include <cstdint>

#ifdef _WIN32
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
#endif

namespace retrofm {

#ifdef _WIN32

static HKL activeLayout()
{
    HWND fg = GetForegroundWindow();
    return GetKeyboardLayout(fg ? GetWindowThreadProcessId(fg, nullptr) : 0);
}

static int vkForAscii(uint32_t c)
{
    if (c >= 'a' && c <= 'z') return (int)(c - 32);
    if (c >= 'A' && c <= 'Z') return (int)c;
    if (c >= '0' && c <= '9') return (int)c;
    switch (c) {
    case '[': case '{': return VK_OEM_4;
    case ']': case '}': return VK_OEM_6;
    case ';': case ':': return VK_OEM_1;
    case '\'': case '"': return VK_OEM_7;
    case ',': case '<': return VK_OEM_COMMA;
    case '.': case '>': return VK_OEM_PERIOD;
    case '`': case '~': return VK_OEM_3;
    case '/': case '?': return VK_OEM_2;
    case '\\': case '|': return VK_OEM_5;
    case '-': case '_': return VK_OEM_MINUS;
    case '=': case '+': return VK_OEM_PLUS;
    default: return 0;
    }
}

static uint32_t charForVk(int vk, HKL hkl)
{
    BYTE ks[256] = {0};
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { ks[VK_SHIFT] = 0x80; ks[VK_LSHIFT] = 0x80; }
    if (GetKeyState(VK_CAPITAL) & 1) ks[VK_CAPITAL] = 0x01;
    const UINT sc = MapVirtualKeyExW((UINT)vk, MAPVK_VK_TO_VSC, hkl);
    WCHAR buf[4] = {0};
    const int n = ToUnicodeEx((UINT)vk, sc, ks, buf, 4, 0x4 /* do not change keyboard state */, hkl);
    if (n == 1 && buf[0] >= 32) return (uint32_t)buf[0];
    return 0;
}

// Returns the character the user really typed, or the host's character when we cannot tell.
uint32_t winTranslateTyped(uint32_t hostChar)
{
    // host already sent a proper non-English character: trust it
    if (hostChar >= 0x100) return hostChar;
    const HKL hkl = activeLayout();
    const WORD lang = PRIMARYLANGID(LOWORD((ULONG_PTR)hkl));
    // some VST2 hosts send the ANSI (code page 1251) byte for Cyrillic
    if (hostChar >= 0xC0 && hostChar <= 0xFF && (lang == LANG_RUSSIAN || lang == LANG_UKRAINIAN || lang == LANG_BELARUSIAN))
        return 0x410 + (hostChar - 0xC0);
    if (hostChar == 0xA8) return 0x401;
    if (hostChar == 0xB8) return 0x451;
    if (lang == LANG_ENGLISH && hostChar >= 32) return hostChar;
    // English key from the host on a non-English layout: translate the same physical key
    int vk = vkForAscii(hostChar);
    if (vk && (GetAsyncKeyState(vk) & 0x8000)) {
        const uint32_t c = charForVk(vk, hkl);
        if (c) return c;
    }
    // nothing usable from the host: find the letter key that is down right now
    if (hostChar == 0 || vk == 0) {
        static const int extra[] = { VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7, VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_MINUS, VK_OEM_PLUS };
        for (int k = 'A'; k <= 'Z'; ++k) if (GetAsyncKeyState(k) & 0x8000) { const uint32_t c = charForVk(k, hkl); if (c) return c; }
        for (int k : extra) if (GetAsyncKeyState(k) & 0x8000) { const uint32_t c = charForVk(k, hkl); if (c) return c; }
    }
    return hostChar;
}

bool winCtrlDown() { return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0; }
bool winKeyVDown() { return (GetAsyncKeyState('V') & 0x8000) != 0; }
bool winLayoutIsCyrillic()
{
    const WORD lang = PRIMARYLANGID(LOWORD((ULONG_PTR)activeLayout()));
    return lang == LANG_RUSSIAN || lang == LANG_UKRAINIAN || lang == LANG_BELARUSIAN || lang == LANG_BULGARIAN || lang == LANG_SERBIAN;
}
#else
uint32_t winTranslateTyped(uint32_t hostChar) { return hostChar; }
bool winCtrlDown() { return false; }
bool winKeyVDown() { return false; }
bool winLayoutIsCyrillic() { return false; }
#endif

} // namespace retrofm

// ---- UTF-8 file names (Windows needs the wide API for non-ASCII paths)
#include <cstdio>
#include <string>
#ifdef _WIN32
namespace retrofm {
FILE* retroFopenUtf8(const char* path, const char* mode)
{
    if (!path) return nullptr;
    wchar_t wp[1024], wm[16];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wp, 1024)) return nullptr;
    if (!MultiByteToWideChar(CP_UTF8, 0, mode, -1, wm, 16)) return nullptr;
    return _wfopen(wp, wm);
}
}
#else
namespace retrofm {
FILE* retroFopenUtf8(const char* path, const char* mode) { return path ? std::fopen(path, mode) : nullptr; }
}
#endif
