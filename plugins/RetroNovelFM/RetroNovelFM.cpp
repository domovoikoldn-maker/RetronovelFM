// Retro Novel FM - VST2 / VST3 / CLAP / AU instrument built with DPF
#include "DistrhoPlugin.hpp"
#include "RetroFMEngine.hpp"
#include <memory>

START_NAMESPACE_DISTRHO

class RetroNovelFM : public Plugin
{
public:
    RetroNovelFM()
        : Plugin(retrofm::kParamCount, 0, 0),
          engine(new retrofm::Engine())
    {
        engine->setSampleRate(getSampleRate());
    }

protected:
    const char* getLabel() const override { return "RetroNovelFM"; }
    const char* getDescription() const override { return "4-operator FM synth in the style of PC-98 / 90s visual novels and RPG Maker music"; }
    const char* getMaker() const override { return "RetroNovel"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }

    void initParameter(uint32_t index, Parameter& p) override
    {
        const retrofm::ParamInfo& info = retrofm::kParams[index];
        p.hints = kParameterIsAutomatable;
        if (info.integer) p.hints |= kParameterIsInteger;
        p.name = info.name;
        p.symbol = info.symbol;
        p.unit = info.unit;
        p.ranges.def = info.def;
        p.ranges.min = info.min;
        p.ranges.max = info.max;

        if (index == retrofm::pInstrument)
        {
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[24];
            for (int i = 0; i < 24; ++i)
            {
                values[i].label = retrofm::kInstrumentNames[i];
                values[i].value = (float)i;
            }
            p.enumValues.count = 24;
            p.enumValues.restrictedMode = true;
            p.enumValues.values = values;
        }
    }

    float getParameterValue(uint32_t index) const override { return engine->getParam((int)index); }
    void setParameterValue(uint32_t index, float value) override { engine->setParam((int)index, value); }

    void sampleRateChanged(double newSampleRate) override { engine->setSampleRate(newSampleRate); }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* const outL = outputs[0];
        float* const outR = outputs[1];
        uint32_t pos = 0;
        for (uint32_t i = 0; i < midiEventCount; ++i)
        {
            const MidiEvent& ev = midiEvents[i];
            const uint32_t at = ev.frame < frames ? ev.frame : frames;
            if (at > pos) { engine->process(outL + pos, outR + pos, at - pos); pos = at; }
            const uint8_t* data = ev.size > MidiEvent::kDataSize ? ev.dataExt : ev.data;
            engine->midi(data, ev.size);
        }
        if (pos < frames) engine->process(outL + pos, outR + pos, frames - pos);
    }

private:
    std::unique_ptr<retrofm::Engine> engine;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroNovelFM)
};

Plugin* createPlugin()
{
    return new RetroNovelFM();
}

END_NAMESPACE_DISTRHO
