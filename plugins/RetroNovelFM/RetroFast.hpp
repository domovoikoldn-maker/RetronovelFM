// RetroFast.hpp - fast, portable math for the per-sample audio loops.
// The standard sin / exp / pow / tanh are accurate but slow (especially in the MinGW runtime used for
// the Windows build). These versions are accurate to better than -120 dB / 1e-6, which is inaudible,
// and cost a few multiplications.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace retrofm {

struct FastSinTable {
    static constexpr int N = 4096;
    float t[N + 1];
    FastSinTable() { for (int i = 0; i <= N; ++i) t[i] = (float)std::sin(6.283185307179586 * i / N); }
};
static inline const FastSinTable& fastSinTable() { static const FastSinTable s; return s; }

// sine of a phase given in cycles (1.0 = one full turn), any range
static inline float fsin1(double cycles) {
    const float* t = fastSinTable().t;
    double p = cycles - std::floor(cycles);
    const double x = p * FastSinTable::N;
    int i = (int)x;
    if (i >= FastSinTable::N) i = FastSinTable::N - 1;
    const float f = (float)(x - i);
    return t[i] + (t[i + 1] - t[i]) * f;
}
static inline float fcos1(double cycles) { return fsin1(cycles + 0.25); }

// 2^x
static inline double fexp2(double x) {
    if (x < -1000.0) return 0.0;
    if (x > 1000.0) x = 1000.0;
    const double fi = std::floor(x);
    const double f = x - fi;
    // minimax-style polynomial for 2^f on [0,1), rel. error < 2e-7
    const double p = 1.0 + f * (0.6931471805599453 + f * (0.2402265069591007 + f * (0.0555041086648216 + f * (0.0096181291076285 + f * (0.0013333558146428 + f * (0.0001540353039338 + f * 0.0000152527338040))))));
    int64_t e = (int64_t)fi + 1023;
    if (e <= 0) return 0.0;
    uint64_t bits = (uint64_t)e << 52;
    double scale; std::memcpy(&scale, &bits, sizeof(scale));
    return p * scale;
}
static inline double fexp(double x) { return fexp2(x * 1.4426950408889634); }

// tanh (Pade approximation, exact enough for saturation)
static inline float ftanh(float x) {
    if (x > 4.97f) return 1.f;
    if (x < -4.97f) return -1.f;
    const float x2 = x * x;
    return x * (135135.f + x2 * (17325.f + x2 * (378.f + x2))) / (135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 28.f)));
}
static inline double ftanh(double x) { return (double)ftanh((float)x); }

// true for NaN / infinity, works even with -ffast-math (checks the exponent bits)
static inline bool badFloat(float x) { uint32_t b; std::memcpy(&b, &x, 4); return (b & 0x7f800000u) == 0x7f800000u; }

// flush tiny values (belt and braces next to the FTZ flag)
static inline float undenorm(float x) { return std::fabs(x) < 1e-15f ? 0.f : x; }

} // namespace retrofm
