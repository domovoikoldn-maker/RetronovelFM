// RetroEncode.cpp - save rendered audio as WAV, MP3 (shine, LGPL) or OGG Vorbis (libvorbis, BSD)
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
extern "C" {
#include "thirdparty/shine/layer3.h"
#include "vorbis/vorbisenc.h"
}

namespace retrofm {

FILE* retroFopenUtf8(const char* path, const char* mode);

static int16_t toI16(float x) { x = x < -1.f ? -1.f : (x > 1.f ? 1.f : x); return (int16_t)std::lround(x * 32767.f); }

bool retroWriteWav(const char* path, const float* L, const float* R, size_t n, int sr)
{
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) return false;
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t)(n * 4);
    std::fwrite("RIFF", 1, 4, f); w32(36 + bytes); std::fwrite("WAVEfmt ", 1, 8, f);
    w32(16); w16(1); w16(2); w32((uint32_t)sr); w32((uint32_t)sr * 4); w16(4); w16(16);
    std::fwrite("data", 1, 4, f); w32(bytes);
    std::vector<int16_t> buf(4096 * 2);
    for (size_t i = 0; i < n; i += 4096)
    {
        const size_t m = std::min<size_t>(4096, n - i);
        for (size_t k = 0; k < m; ++k) { buf[k * 2] = toI16(L[i + k]); buf[k * 2 + 1] = toI16(R[i + k]); }
        std::fwrite(buf.data(), 2, m * 2, f);
    }
    std::fclose(f);
    return true;
}

bool retroWriteMp3(const char* path, const float* L, const float* R, size_t n, int sr, int kbps)
{
    if (shine_check_config(sr, kbps) < 0) return false;
    shine_config_t cfg;
    shine_set_config_mpeg_defaults(&cfg.mpeg);
    cfg.wave.channels = PCM_STEREO;
    cfg.wave.samplerate = sr;
    cfg.mpeg.mode = JOINT_STEREO;
    cfg.mpeg.bitr = kbps;
    shine_t s = shine_initialise(&cfg);
    if (!s) return false;
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) { shine_close(s); return false; }
    const int spp = shine_samples_per_pass(s);
    std::vector<int16_t> buf((size_t)spp * 2);
    for (size_t i = 0; i < n; i += (size_t)spp)
    {
        for (int k = 0; k < spp; ++k)
        {
            const size_t j = i + (size_t)k;
            buf[(size_t)k * 2]     = j < n ? toI16(L[j]) : 0;
            buf[(size_t)k * 2 + 1] = j < n ? toI16(R[j]) : 0;
        }
        int written = 0;
        unsigned char* out = shine_encode_buffer_interleaved(s, buf.data(), &written);
        if (written > 0) std::fwrite(out, 1, (size_t)written, f);
    }
    int written = 0;
    unsigned char* out = shine_flush(s, &written);
    if (written > 0) std::fwrite(out, 1, (size_t)written, f);
    shine_close(s);
    std::fclose(f);
    return true;
}

bool retroWriteOgg(const char* path, const float* L, const float* R, size_t n, int sr, float quality)
{
    FILE* f = retroFopenUtf8(path, "wb");
    if (!f) return false;
    vorbis_info vi; vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, 2, sr, quality) != 0) { vorbis_info_clear(&vi); std::fclose(f); return false; }
    vorbis_comment vc; vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ENCODER", "Retro Novel FM");
    vorbis_comment_add_tag(&vc, "TITLE", "Random Song");
    vorbis_dsp_state vd; vorbis_analysis_init(&vd, &vi);
    vorbis_block vb; vorbis_block_init(&vd, &vb);
    ogg_stream_state os; ogg_stream_init(&os, (int)std::time(nullptr) & 0x7fffffff);
    ogg_packet hdr, hcomm, hcode;
    vorbis_analysis_headerout(&vd, &vc, &hdr, &hcomm, &hcode);
    ogg_stream_packetin(&os, &hdr); ogg_stream_packetin(&os, &hcomm); ogg_stream_packetin(&os, &hcode);
    ogg_page og; ogg_packet op;
    while (ogg_stream_flush(&os, &og)) { std::fwrite(og.header, 1, (size_t)og.header_len, f); std::fwrite(og.body, 1, (size_t)og.body_len, f); }
    size_t pos = 0;
    bool eos = false;
    while (!eos)
    {
        const size_t m = std::min<size_t>(4096, n - pos);
        if (m == 0) vorbis_analysis_wrote(&vd, 0);
        else
        {
            float** b = vorbis_analysis_buffer(&vd, (int)m);
            std::memcpy(b[0], L + pos, m * sizeof(float));
            std::memcpy(b[1], R + pos, m * sizeof(float));
            vorbis_analysis_wrote(&vd, (int)m);
            pos += m;
        }
        while (vorbis_analysis_blockout(&vd, &vb) == 1)
        {
            vorbis_analysis(&vb, nullptr);
            vorbis_bitrate_addblock(&vb);
            while (vorbis_bitrate_flushpacket(&vd, &op))
            {
                ogg_stream_packetin(&os, &op);
                while (!eos && ogg_stream_pageout(&os, &og))
                {
                    std::fwrite(og.header, 1, (size_t)og.header_len, f); std::fwrite(og.body, 1, (size_t)og.body_len, f);
                    if (ogg_page_eos(&og)) eos = true;
                }
            }
        }
        if (m == 0) break;
    }
    // drain whatever is left
    while (ogg_stream_flush(&os, &og)) { std::fwrite(og.header, 1, (size_t)og.header_len, f); std::fwrite(og.body, 1, (size_t)og.body_len, f); }
    ogg_stream_clear(&os); vorbis_block_clear(&vb); vorbis_dsp_clear(&vd); vorbis_comment_clear(&vc); vorbis_info_clear(&vi);
    std::fclose(f);
    return true;
}

} // namespace retrofm
