// Retro Novel FM - VST2 / VST3 / CLAP / AU instrument built with DPF
#include "DistrhoPlugin.hpp"
#include "RetroFMEngine.hpp"
#include <memory>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

START_NAMESPACE_DISTRHO

class RetroNovelFM : public Plugin
{
public:
    RetroNovelFM()
        : Plugin(retrofm::kParamCount, 0, 17),
          engine(new retrofm::Engine())
    {
        engine->setSampleRate(getSampleRate());
    }

protected:
    const char* getLabel() const override { return "RetroNovelFM"; }
    const char* getDescription() const override { return "FM synth + drum machine in the style of PC-98 / 90s visual novels and RPG Maker music"; }
    const char* getMaker() const override { return "RetroNovel"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(8, 0, 0); }

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
        case retrofm::pInstrument: setEnum(p, retrofm::kInstrumentNames, retrofm::kNumInstruments); break;
        case retrofm::pDrumMode:   setEnum(p, retrofm::kDrumModeNames, 3); break;
        case retrofm::pDrumKit:    setEnum(p, retrofm::kDrumKitNames, retrofm::kNumKits); break;
        case retrofm::pDrumGenre:
        {
            const char* names[retrofm::kNumGenres];
            for (int i = 0; i < retrofm::kNumGenres; ++i) names[i] = retrofm::kGenres[i].name;
            setEnum(p, names, retrofm::kNumGenres);
            break;
        }
        case retrofm::pDrumPattern:
        {
            const char* names[retrofm::kNumPatterns];
            for (int i = 0; i < retrofm::kNumPatterns; ++i)
                names[i] = retrofm::isUserPattern(i) ? retrofm::kUserPatternNames[i - retrofm::kFirstUserPattern] : retrofm::kPatterns[retrofm::presetSlot(i)].name;
            setEnum(p, names, retrofm::kNumPatterns);
            break;
        }
        case retrofm::pMeter:
        {
            const char* names[retrofm::kNumMeters];
            for (int i = 0; i < retrofm::kNumMeters; ++i) names[i] = retrofm::kMeters[i].name;
            setEnum(p, names, retrofm::kNumMeters);
            break;
        }
        case retrofm::pKey:   setEnum(p, retrofm::kKeyNames, 12); break;
        case retrofm::pChord: setEnum(p, retrofm::kChordModeNames, retrofm::kNumChordModes); break;
        case retrofm::pScale:
        {
            const char* names[retrofm::kNumScales];
            for (int i = 0; i < retrofm::kNumScales; ++i) names[i] = retrofm::kScales[i].name;
            setEnum(p, names, retrofm::kNumScales);
            break;
        }
        case retrofm::pScaleLock:
        case retrofm::pSongPlay:
        case retrofm::pDrumFill:
        case retrofm::pVocalMode:
            p.hints |= kParameterIsBoolean;
            break;
        case retrofm::pScene:
        {
            const char* names[retrofm::kNumScenes + 1];
            names[0] = "Off";
            for (int i = 0; i < retrofm::kNumScenes; ++i) names[i + 1] = retrofm::kScenes[i].name;
            setEnum(p, names, retrofm::kNumScenes + 1);
            break;
        }
        case retrofm::pIdol:
        {
            const char* names[retrofm::kNumIdols];
            for (int i = 0; i < retrofm::kNumIdols; ++i) names[i] = retrofm::kIdolVoices[i].name;
            setEnum(p, names, retrofm::kNumIdols);
            break;
        }
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case retrofm::pOutActivity: return engine->outActivity();
        case retrofm::pOutDrumStep: return engine->outDrumStep();
        case retrofm::pOutBeat:     return engine->outBeat();
        case retrofm::pOutMouth:    return engine->outMouth();
        case retrofm::pOutVowel:    return engine->outVowel();
        case retrofm::pOutSyll:     return engine->outSyll();
        default:                    return engine->getParam((int)index);
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!retrofm::kParams[index].output)
            engine->setParam((int)index, value);
    }

    // ---------------------------------------------------------------- states
    // smp1..4 = user sample paths, root1..4 = their root notes, pat1..4 = user drum patterns
    void initState(uint32_t index, State& st) override
    {
        if (index == 12)
        {
            st.key = "lyrics"; st.defaultValue = retrofm::kLyricPresets[0][1]; st.hints = 0; st.label = "Lyrics";
            return;
        }
        if (index >= 13)
        {
            static const char* const keys[4] = { "song", "take", "slots", "lessons" };
            static const char* const labels[4] = { "Random Song", "Recorded Take", "Preset Slots", "Lesson Progress" };
            st.key = keys[index - 13]; st.defaultValue = ""; st.hints = 0; st.label = labels[index - 13];
            return;
        }
        const int k = (int)(index % 4) + 1;
        char key[16];
        if (index < 4)
        {
            std::snprintf(key, sizeof(key), "smp%d", k);
            st.key = key; st.defaultValue = ""; st.hints = kStateIsFilenamePath;
            char label[32]; std::snprintf(label, sizeof(label), "My Sample %d", k); st.label = label;
        }
        else if (index < 8)
        {
            std::snprintf(key, sizeof(key), "root%d", k);
            st.key = key; st.defaultValue = "60"; st.hints = 0;
            char label[32]; std::snprintf(label, sizeof(label), "My Sample %d Root", k); st.label = label;
        }
        else
        {
            std::snprintf(key, sizeof(key), "pat%d", k);
            st.key = key; st.defaultValue = engine->getUserPattern(k - 1).c_str(); st.hints = 0;
            char label[32]; std::snprintf(label, sizeof(label), "User Pattern %d", k); st.label = label;
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (key && value && !std::strcmp(key, "lyrics")) { engine->setLyrics(value); return; }
        if (key && !std::strcmp(key, "rewind")) { engine->rewindLyrics(); return; }
        if (key && value && !std::strcmp(key, "song")) { engine->setSong(value); return; }
        if (key && value && !std::strcmp(key, "take")) { if (!*value) engine->recorder().clear(); else engine->recorder().deserialize(value); return; }
        if (key && value && !std::strcmp(key, "slots")) { slots = value; return; }
        if (key && value && !std::strcmp(key, "lessons")) { lessons = value; return; }
        if (!key || !value || std::strlen(key) < 4) return;
        const int k = key[std::strlen(key) - 1] - '1';
        if (k < 0 || k > 3) return;
        if (!std::strncmp(key, "smp", 3)) engine->loadUserSample(k, value);
        else if (!std::strncmp(key, "root", 4)) engine->setUserRoot(k, std::atoi(value));
        else if (!std::strncmp(key, "pat", 3)) engine->setUserPattern(k, value);
    }

    String getState(const char* key) const override
    {
        if (key && !std::strcmp(key, "lyrics")) return String(engine->getLyrics().c_str());
        if (key && !std::strcmp(key, "song")) return String(engine->getSong().c_str());
        if (key && !std::strcmp(key, "take")) return String(engine->recorder().serialize(engine->tempo()).c_str());
        if (key && !std::strcmp(key, "slots")) return String(slots.c_str());
        if (key && !std::strcmp(key, "lessons")) return String(lessons.c_str());
        if (!key || std::strlen(key) < 4) return String();
        const int k = key[std::strlen(key) - 1] - '1';
        if (k < 0 || k > 3) return String();
        if (!std::strncmp(key, "smp", 3)) return String(engine->getUserPath(k).c_str());
        if (!std::strncmp(key, "root", 4)) return String(engine->getUserRoot(k));
        if (!std::strncmp(key, "pat", 3)) return String(engine->getUserPattern(k).c_str());
        return String();
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

public:
    std::unique_ptr<retrofm::Engine> engine;
private:
    std::string slots, lessons;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RetroNovelFM)
};

Plugin* createPlugin()
{
    return new RetroNovelFM();
}

END_NAMESPACE_DISTRHO

// ---- direct access for the UI (MIDI export of the recorded take)
namespace retrofm {
bool retroTakeSnapshot(void* plugin, std::vector<RecEvent>& out, double& bpm)
{
    if (!plugin) return false;
    DISTRHO::RetroNovelFM* p = static_cast<DISTRHO::RetroNovelFM*>(static_cast<DISTRHO::Plugin*>(plugin));
    p->engine->recorder().snapshot(out);
    bpm = p->engine->tempo();
    return true;
}
void retroTakeClear(void* plugin)
{
    if (!plugin) return;
    DISTRHO::RetroNovelFM* p = static_cast<DISTRHO::RetroNovelFM*>(static_cast<DISTRHO::Plugin*>(plugin));
    p->engine->recorder().clear();
}
}
