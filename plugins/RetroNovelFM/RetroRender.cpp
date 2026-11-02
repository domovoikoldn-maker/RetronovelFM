// RetroRender.cpp - offline render of the Random Song to a WAV file (TOOLS -> EXPORT WAV).
// Runs a private copy of the engine in the UI thread, as if the DAW were playing at the song's tempo.
#include "RetroFMEngine.hpp"
#include <cstdio>
#include <memory>
#include <string>

namespace retrofm {

FILE* retroFopenUtf8(const char* path, const char* mode);

bool retroRenderSongWav(const char* path, const float* params, const char* song, const char* lyrics,
                        const std::string* pats, const std::string* smp, const int* roots, int bpm, int loops, std::string& info)
{
    if (!song || std::strncmp(song, "song1;", 6)) { info = "no song"; return false; }
    const int loopSteps = std::atoi(song + 6);
    if (loopSteps <= 0) { info = "no song"; return false; }
    const int sr = 44100;
    std::unique_ptr<Engine> e(new Engine());
    e->setSampleRate(sr);
    for (int i = 0; i < kParamCount; ++i) if (!kParams[i].output) e->setParam(i, params[i]);
    for (int u = 0; u < 4; ++u)
    {
        if (!pats[u].empty()) e->setUserPattern(u, pats[u].c_str());
        if (!smp[u].empty()) e->loadUserSample(u, smp[u].c_str());
        e->setUserRoot(u, roots[u]);
    }
    if (lyrics) e->setLyrics(lyrics);
    e->setSong(song);
    e->setParam(pSongPlay, 1.f);
    if (bpm < 40 || bpm > 300) bpm = 120;
    const double spb = bpm / 60.0 * 4.0 / sr;                    // 16ths per sample
    const size_t frames = (size_t)(loopSteps * loops / spb) + (size_t)(sr * 2.5);   // + reverb tail
    const size_t stopAt = (size_t)(loopSteps * loops / spb);
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) { info = "cannot write file"; return false; }
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t)(frames * 4);
    std::fwrite("RIFF", 1, 4, f); w32(36 + bytes); std::fwrite("WAVEfmt ", 1, 8, f);
    w32(16); w16(1); w16(2); w32(sr); w32(sr * 4); w16(4); w16(16);
    std::fwrite("data", 1, 4, f); w32(bytes);
    const int B = 256;
    float L[B], R[B]; int16_t buf[B * 2];
    double pos = 0;
    for (size_t i = 0; i < frames; i += B)
    {
        const uint32_t n = (uint32_t)std::min<size_t>(B, frames - i);
        if (i >= stopAt && i < stopAt + B) e->setParam(pSongPlay, 0.f);    // let the last notes ring out
        e->transport(i < stopAt, true, pos, bpm);
        e->process(L, R, n);
        for (uint32_t k = 0; k < n; ++k)
        {
            buf[k * 2]     = (int16_t)std::lround(std::max(-1.f, std::min(1.f, L[k])) * 32767.f);
            buf[k * 2 + 1] = (int16_t)std::lround(std::max(-1.f, std::min(1.f, R[k])) * 32767.f);
        }
        std::fwrite(buf, 2, n * 2, f);
        pos += n * spb;
    }
    std::fclose(f);
    char b[96]; std::snprintf(b, sizeof(b), "%.0f s, %d BPM", frames / (double)sr, bpm);
    info = b;
    return true;
}

} // namespace retrofm
