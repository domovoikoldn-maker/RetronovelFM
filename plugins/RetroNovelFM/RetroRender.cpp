// RetroRender.cpp - offline render of the Random Song to a WAV file (TOOLS -> EXPORT WAV).
// Runs a private copy of the engine in the UI thread, as if the DAW were playing at the song's tempo.
#include "RetroFMEngine.hpp"
#include <cstdio>
#include <memory>
#include <string>

namespace retrofm {

FILE* retroFopenUtf8(const char* path, const char* mode);

bool retroWriteWav(const char* path, const float* L, const float* R, size_t n, int sr);
bool retroWriteMp3(const char* path, const float* L, const float* R, size_t n, int sr, int kbps);
bool retroWriteOgg(const char* path, const float* L, const float* R, size_t n, int sr, float quality);

// format: 0 WAV, 1 MP3 (192 kbps), 2 OGG Vorbis (q 0.6)
bool retroRenderSong(const char* path, int format, const float* params, const char* song, const char* lyrics,
                     const std::string* pats, const std::string* smp, const int* roots, int bpm, int loops, std::string& info)
{
    if (!song || std::strncmp(song, "song1;", 6)) { info = "no song"; return false; }
    const int loopSteps = std::atoi(song + 6);
    if (loopSteps <= 0) { info = "no song"; return false; }
    const int sr = 44100;
    std::unique_ptr<Engine> e(new Engine());
    e->setSampleRate(sr);
    for (int i = 0; i < kParamCount; ++i) if (!kParams[i].output) e->setParam(i, params[i]);
    e->setParam(pQuality, 2.f);   // offline: always full quality
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
    const double spb = bpm / 60.0 * 4.0 / sr;                                   // 16ths per sample
    const size_t stopAt = (size_t)(loopSteps * loops / spb);
    const size_t frames = stopAt + (size_t)(sr * 2.5);                         // + reverb tail
    std::vector<float> L(frames), R(frames);
    const int B = 256;
    double pos = 0;
    for (size_t i = 0; i < frames; i += B)
    {
        const uint32_t n = (uint32_t)std::min<size_t>(B, frames - i);
        if (i >= stopAt && i < stopAt + B) e->setParam(pSongPlay, 0.f);        // let the last notes ring out
        e->beginBlock();
        e->transport(i < stopAt, true, pos, bpm);
        e->process(&L[i], &R[i], n);
        pos += n * spb;
    }
    // gentle fade over the last second
    for (size_t i = frames - (size_t)sr; i < frames; ++i) { const float g = (float)(frames - i) / sr; L[i] *= g; R[i] *= g; }
    bool ok = false;
    if (format == 1) ok = retroWriteMp3(path, L.data(), R.data(), frames, sr, 256);
    else if (format == 2) ok = retroWriteOgg(path, L.data(), R.data(), frames, sr, 0.6f);
    else ok = retroWriteWav(path, L.data(), R.data(), frames, sr);
    char b[96]; std::snprintf(b, sizeof(b), "%.0f s, %d BPM", frames / (double)sr, bpm);
    info = b;
    if (!ok) info = "cannot write file";
    return ok;
}

} // namespace retrofm
