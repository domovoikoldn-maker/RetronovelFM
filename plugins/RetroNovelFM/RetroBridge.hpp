// RetroBridge.hpp - what the UI may ask the DSP directly (same binary, DISTRHO_PLUGIN_WANT_DIRECT_ACCESS)
#pragma once
#include <cstdint>
#include <vector>

namespace retrofm {

// one recorded note event; t in 16th notes
struct RecEvent { double t; uint8_t on, note, vel, vocal; int16_t syl; };

// implemented in RetroNovelFM.cpp; plugin = UI::getPluginInstancePointer()
bool retroTakeSnapshot(void* plugin, std::vector<RecEvent>& out, double& bpm);
void retroTakeClear(void* plugin);

} // namespace retrofm
