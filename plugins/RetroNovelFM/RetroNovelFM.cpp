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
    const char* getDescription() const override { return "FM synth + drum machine in the style of PC-98 / 90s visual novels and RPG Maker music"; }
    const char* getMaker() const override { return "RetroNovel"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(2, 0, 0); }

    static void setEnum(Parameter& p, const char* const* names, int count)
    {
        ParameterEnumerationValue* const values = new ParameterEnumerationValue[count];
        for (int i = 0; i < count; ++i)
        {
            values[i].label = names[i];
            values[i].value = (float)i;
        }
        p.enumValues.count = (uint8_t)count;
        p.enumValues.restrictedMode = true;
        p.enumValues.values = values;
    }

    void initParameter(uint32_t index, Parameter& p) override
    {
        const retrofm::ParamInfo& info = retrofm::kParams[index];
        p.hints = info.output ? kParameterIsOutput : kParameterIsAutomatable;
        if (info.integer) p.hints |= kParameterIsInteger;
        p.name = info.name;
        p.symbol = info.symbol;
        p.unit = info.unit;
        p.ranges.def = info.def;
        p.ranges.min = info.min;
        p.ranges.max = info.max;

        switch (index)
        {
        case retrofm::pInstrument: setEnum(p, retrofm::kInstrumentNames, 24); break;
        case retrofm::pDrumMode:   setEnum(p, retrofm::kDrumModeNames, 3); break;
        case retrofm::pDrumKit:    setEnum(p, retrofm::kDrumKitNames, 3); break;
        case retrofm::pDrumPattern:
        {
            const char* names[12];
            for (int i = 0; i < 12; ++i) names[i] = retrofm::kPatterns[i].name;
            setEnum(p, names, 12);
            break;
        }
        case retrofm::pDrumFill:
            p.hints |= kParameterIsBoolean;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case retrofm::pOutActivity: return engine->outActivity();
        case retrofm::pOutDrumStep: return engine->outDrumStep();
        case retrofm::pOutBeat:     return engine->outBeat();
        default:                    return engine->getParam((int)index);
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!retrofm::kParams[index].output)
            engine->setParam((int)index, value);
    }

    void sampleRateChanged(double newSampleRate) override { engine->setSampleRate(newSampleRate); }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        // host transport -> drum sequencer
        const TimePosition& tp = getTimePosition();
        double bpm = 120.0, posSteps = 0.0;
        bool posValid = false;
        if (tp.bbt.valid)
        {
            bpm = tp.bbt.beatsPerMinute;
            const double beats = (tp.bbt.bar - 1) * (double)tp.bbt.beatsPerBar + (tp.bbt.beat - 1)
                               + (tp.bbt.ticksPerBeat > 0 ? tp.bbt.tick / tp.bbt.ticksPerBeat : 0.0);
            posSteps = beats * 4.0;
            posValid = true;
        }
        else if (getSampleRate() > 0)
        {
            posSteps = tp.frame / getSampleRate() * bpm / 60.0 * 4.0;
            posValid = true;
        }
        engine->transport(tp.playing, posValid, posSteps, bpm);

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
