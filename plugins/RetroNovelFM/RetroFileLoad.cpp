// RetroFileLoad.cpp - loads user samples (WAV / OGG) as mono float
#include "RetroSampler.hpp"
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <windows.h>
#endif

#define STB_VORBIS_NO_STDIO 1
#define STB_VORBIS_NO_PUSHDATA_API 1
#include "thirdparty/stb_vorbis.c"
#undef L
#undef C
#undef R

namespace retrofm {

static bool readWholeFile(const char* path, std::vector<uint8_t>& out)
{
#ifdef _WIN32
    const int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (n <= 0) return false;
    std::vector<wchar_t> w((size_t)n);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, w.data(), n);
    FILE* f = _wfopen(w.data(), L"rb");
#else
    FILE* f = std::fopen(path, "rb");
#endif
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 512L * 1024 * 1024) { std::fclose(f); return false; }
    out.resize((size_t)size);
    const size_t got = std::fread(out.data(), 1, (size_t)size, f);
    std::fclose(f);
    return got == (size_t)size;
}

static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static bool parseWav(const std::vector<uint8_t>& b, std::vector<float>& out, double& rate)
{
    if (b.size() < 44 || std::memcmp(b.data(), "RIFF", 4) != 0 || std::memcmp(b.data() + 8, "WAVE", 4) != 0) return false;
    size_t p = 12;
    int fmt = 0, ch = 0, bits = 0; uint32_t sr = 0;
    const uint8_t* data = nullptr; size_t dlen = 0;
    while (p + 8 <= b.size()) {
        const uint32_t sz = rd32(&b[p + 4]);
        const uint8_t* c = &b[p + 8];
        const size_t avail = std::min<size_t>(sz, b.size() - (p + 8));
        if (!std::memcmp(&b[p], "fmt ", 4) && avail >= 16) {
            fmt = rd16(c); ch = rd16(c + 2); sr = rd32(c + 4); bits = rd16(c + 14);
            if (fmt == 0xFFFE && avail >= 26) fmt = rd16(c + 24);
        } else if (!std::memcmp(&b[p], "data", 4)) {
            data = c; dlen = avail;
        }
        p += 8 + sz + (sz & 1);
    }
    if (!data || ch < 1 || sr == 0) return false;
    const int bps = bits / 8;
    if (bps < 1) return false;
    const size_t frames = dlen / (size_t)(bps * ch);
    out.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
        double acc = 0;
        for (int c = 0; c < ch; ++c) {
            const uint8_t* s = data + (i * ch + c) * bps;
            double v = 0;
            if (fmt == 3 && bits == 32) { float f; std::memcpy(&f, s, 4); v = f; }
            else if (fmt == 3 && bits == 64) { double d; std::memcpy(&d, s, 8); v = d; }
            else if (bits == 8) v = (s[0] - 128) / 128.0;
            else if (bits == 16) v = (int16_t)rd16(s) / 32768.0;
            else if (bits == 24) { int32_t x = s[0] | (s[1] << 8) | (s[2] << 16); if (x & 0x800000) x |= ~0xFFFFFF; v = x / 8388608.0; }
            else if (bits == 32) v = (int32_t)rd32(s) / 2147483648.0;
            else return false;
            acc += v;
        }
        out[i] = (float)(acc / ch);
    }
    rate = sr;
    return frames > 0;
}

bool loadAudioFileMono(const char* path, std::vector<float>& out, double& rate)
{
    std::vector<uint8_t> b;
    if (!path || !*path || !readWholeFile(path, b)) return false;
    if (parseWav(b, out, rate)) return true;
    if (b.size() > 4 && std::memcmp(b.data(), "OggS", 4) == 0) {
        int ch = 0, sr = 0; short* pcm = nullptr;
        const int frames = stb_vorbis_decode_memory(b.data(), (int)b.size(), &ch, &sr, &pcm);
        if (frames <= 0 || !pcm || ch < 1) { std::free(pcm); return false; }
        out.resize((size_t)frames);
        for (int i = 0; i < frames; ++i) {
            int acc = 0;
            for (int c = 0; c < ch; ++c) acc += pcm[i * ch + c];
            out[(size_t)i] = acc / (32768.f * ch);
        }
        std::free(pcm);
        rate = sr;
        return true;
    }
    return false;
}

} // namespace retrofm
