#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <functional>

namespace {
using Fmt = std::function<juce::String(float, int)>;

juce::String signedInt(int v, const char* unit) {
    return juce::String(v > 0 ? "+" : v < 0 ? "-" : "") + juce::String(std::abs(v)) + unit;
}
Fmt pct() { return [](float v, int) { return juce::String(juce::roundToInt(v * 100)) + "%"; }; }
Fmt offPct() { return [](float v, int) { return v < 0.003f ? juce::String("Off") : juce::String(juce::roundToInt(v * 100)) + "%"; }; }
Fmt semis(double span) { return [span](float v, int) { return signedInt((int) std::floor((v - 0.5) * span + 0.5), " st"); }; }
Fmt hz(double base, double span) {
    return [base, span](float v, int) {
        const double f = base * std::pow(2.0, (v - 0.5) * span);
        return f >= 1000 ? juce::String(f / 1000, 1) + " kHz" : juce::String(juce::roundToInt(f)) + " Hz";
    };
}
Fmt msScaled(double base) {
    return [base](float v, int) { return juce::String(juce::roundToInt(base * std::pow(4.0, (v - 0.5) * 2) * 1000)) + " ms"; };
}
Fmt msLinear(double a, double b) { return [a, b](float v, int) { return juce::String(juce::roundToInt((a + v * b) * 1000)) + " ms"; }; }

struct FDef { const char* id; const char* name; float def; Fmt fmt; };

constexpr int kIntervals[4] = {-12, 0, 7, 12};
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout AcidBoxProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const std::vector<FDef> defs = {
        {"tune", "Tune", 0.5f, semis(24)},
        {"cutoff", "Cutoff", 0.35f, pct()},
        {"reso", "Resonance", 0.7f, pct()},
        {"envmod", "Env Mod", 0.6f, pct()},
        {"decay", "Decay", 0.35f, [](float v, int) { return juce::String(juce::roundToInt((0.06 + v * v * 2.0) * 1000)) + " ms"; }},
        {"accent", "Accent", 0.6f, pct()},
        {"drive", "Drive", 0.3f, pct()},
        {"delay", "Echo", 0.25f, pct()},
        {"reverb", "Reverb", 0.3f, pct()},
        {"rsize", "Room Size", 0.5f, pct()},
        {"morph", "Osc Morph", 0.0f, [](float v, int) {
             if (v < 0.005f) return juce::String("Osc 1");
             if (v > 0.995f) return juce::String("Osc 2");
             return juce::String(juce::roundToInt((1 - v) * 100)) + " / " + juce::String(juce::roundToInt(v * 100));
         }},
        {"detune", "Osc 2 Detune", 0.5f, [](float v, int) { return signedInt(juce::roundToInt((v - 0.5f) * 100), " ct"); }},
        {"blevel", "Bass Volume", 0.8f, pct()},
        {"dist", "Dist Amount", 0.0f, offPct()},
        {"dtone", "Dist Tone", 0.6f, [](float v, int) {
             const double f = 700 * std::pow(2.0, v * 4.8);
             return f >= 1000 ? juce::String(f / 1000, 1) + " kHz" : juce::String(juce::roundToInt(f)) + " Hz";
         }},
        {"dmix", "Dist Mix", 1.0f, pct()},
        {"ring", "Ring Amount", 0.0f, offPct()},
        {"rpitch", "Ring Pitch", 0.5f, pct()},
        {"dlevel", "Drum Volume", 0.7f, pct()},
        {"kdecay", "Kick Decay", 0.5f, msLinear(0.12, 0.6)},
        {"duck", "Duck", 0.45f, pct()},
        {"ducktime", "Duck Time", 0.4f, msLinear(0.04, 0.36)},
        {"lvk", "Kick Level", 0.8f, pct()},
        {"lvs", "Snare Level", 0.8f, pct()},
        {"lvc", "Clap Level", 0.8f, pct()},
        {"lvch", "Closed Hat Level", 0.8f, pct()},
        {"lvoh", "Open Hat Level", 0.8f, pct()},
        {"ktune", "Kick Tune", 0.5f, hz(45, 2)},
        {"kpunch", "Kick Punch", 0.4f, pct()},
        {"ksweep", "Kick Sweep", 0.5f, msScaled(0.03)},
        {"kclick", "Kick Click", 0.5f, pct()},
        {"kdrive", "Kick Drive", 0.5f, pct()},
        {"stune", "Snare Tune", 0.5f, hz(185, 2)},
        {"ssnap", "Snare Snappy", 0.5f, pct()},
        {"stone", "Snare Tone", 0.5f, hz(1800, 3.4)},
        {"sdecay", "Snare Decay", 0.5f, msScaled(0.15)},
        {"ctone", "Clap Tone", 0.5f, hz(1150, 3)},
        {"cspread", "Clap Spread", 0.5f, msLinear(0.004, 0.012)},
        {"cdecay", "Clap Decay", 0.5f, msScaled(0.11)},
        {"chtune", "Closed Hat Tune", 0.5f, semis(24)},
        {"chtone", "Closed Hat Tone", 0.5f, hz(7500, 2.4)},
        {"chdecay", "Closed Hat Decay", 0.5f, msScaled(0.04)},
        {"chnoise", "Closed Hat Noise", 0.5f, pct()},
        {"ohtune", "Open Hat Tune", 0.5f, semis(24)},
        {"ohtone", "Open Hat Tone", 0.5f, hz(7500, 2.4)},
        {"ohdecay", "Open Hat Decay", 0.5f, msScaled(0.3)},
        {"ohnoise", "Open Hat Noise", 0.5f, pct()},
        {"cthr", "Comp Threshold", 2.0f / 3.0f, [](float v, int) { return juce::String(juce::roundToInt(-36 + v * 36)) + " dB"; }},
        {"cratio", "Comp Ratio", 0.4f, [](float v, int) { return juce::String(1 + 19 * v * v, 1) + ":1"; }},
        {"catk", "Comp Attack", 0.741f, [](float v, int) {
             const double ms = 0.1 * std::pow(500.0, v);
             return (ms < 10 ? juce::String(ms, 1) : juce::String(juce::roundToInt(ms))) + " ms";
         }},
        {"crel", "Comp Release", 0.431f, [](float v, int) { return juce::String(juce::roundToInt(20 * std::pow(25.0, v))) + " ms"; }},
        {"cgain", "Comp Makeup", 1.0f / 3.0f, [](float v, int) { return "+" + juce::String(v * 18, 1) + " dB"; }},
        {"cmix", "Comp Mix", 1.0f, pct()},
        {"fslope", "Filter Slope", 1.0f, [](float v, int) { return juce::String(18 + 6 * v, 1) + " dB"; }},
        {"fslopeenv", "Filter Slope Env", 0.0f, offPct()},
    };
    for (const auto& d : defs)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{d.id, 1}, d.name, juce::NormalisableRange<float>(0.0f, 1.0f), d.def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(d.fmt)));

    const juce::StringArray waves{"Saw", "Square", "Pulse", "Triangle", "Sine"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"wave", 1}, "Osc 1 Wave", waves, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"wave2", 1}, "Osc 2 Wave", waves, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"interval", 1}, "Osc 2 Interval",
                                                            juce::StringArray{"-1 oct", "Unison", "+5th", "+1 oct"}, 1));
    // "None" switches the effect off completely (index 0, the engine's -1)
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"dtype", 1}, "Dist Type",
                                                            juce::StringArray{"None", "Overdrive", "Fuzz", "Fold", "Crush"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"rmode", 1}, "Ring Mode",
                                                            juce::StringArray{"None", "Follow note", "Fixed", "Osc 1 x 2"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tempo", 1}, "Tempo", juce::NormalisableRange<float>(40.0f, 220.0f, 1.0f), 125.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float v, int) { return juce::String(juce::roundToInt(v)) + " BPM"; })));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"hostsync", 1}, "Sync to Host", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"cmp", 1}, "Comp On", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"spaceon", 1}, "Space On", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"ftype", 1}, "Filter Type",
                                                            juce::StringArray{"Ladder", "Diode (303)"}, 0));
    return layout;
}

AcidBoxProcessor::AcidBoxProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout()),
      seq(acid::defaultSeq()) {
    for (const auto& f : acid::kFloatFields) {
        auto* src = apvts.getRawParameterValue(f.id);
        jassert(src != nullptr);
        if (src != nullptr) bindings.push_back({f.m, src});
    }
    pTempo = apvts.getRawParameterValue("tempo");
    pWave = apvts.getRawParameterValue("wave");
    pWave2 = apvts.getRawParameterValue("wave2");
    pInterval = apvts.getRawParameterValue("interval");
    pDtype = apvts.getRawParameterValue("dtype");
    pRmode = apvts.getRawParameterValue("rmode");
    pSync = apvts.getRawParameterValue("hostsync");
    pCmp = apvts.getRawParameterValue("cmp");
    pSpace = apvts.getRawParameterValue("spaceon");
    pFtype = apvts.getRawParameterValue("ftype");

    for (auto* p : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(p)) allParams.push_back(r);
    jassert(allParams.size() <= ccValue.size());
    for (auto& v : ccValue) v = -1.0f;
    for (auto& l : learned) l = -1;
    builtin.fill(-1);
    const std::pair<int, const char*> defaults[] = {{74, "cutoff"}, {71, "reso"}, {75, "envmod"}, {72, "decay"}, {76, "accent"},
                                                    {77, "drive"}, {78, "morph"}, {93, "delay"}, {91, "reverb"}, {79, "dist"},
                                                    {80, "ring"}, {7, "blevel"}};
    for (const auto& d : defaults) builtin[(size_t) d.first] = paramIndex(d.second);
    startTimerHz(60);
}

int AcidBoxProcessor::paramIndex(const juce::String& id) const {
    for (size_t i = 0; i < allParams.size(); ++i)
        if (allParams[i]->getParameterID() == id) return (int) i;
    return -1;
}

int AcidBoxProcessor::learnedCC(int index) const {
    for (int cc = 0; cc < 128; ++cc) if (index >= 0 && learned[(size_t) cc].load() == index) return cc;
    return -1;
}

int AcidBoxProcessor::builtinCC(int index) const {
    for (int cc = 0; cc < 128; ++cc) if (index >= 0 && builtin[(size_t) cc] == index && learned[(size_t) cc].load() < 0) return cc;
    return -1;
}

std::vector<std::pair<int, int>> AcidBoxProcessor::learnedList() const {
    std::vector<std::pair<int, int>> out;
    for (int cc = 0; cc < 128; ++cc) if (learned[(size_t) cc].load() >= 0) out.push_back({cc, learned[(size_t) cc].load()});
    return out;
}

std::vector<std::pair<int, int>> AcidBoxProcessor::builtinList() const {
    std::vector<std::pair<int, int>> out;
    for (int cc = 0; cc < 128; ++cc) if (builtin[(size_t) cc] >= 0 && learned[(size_t) cc].load() < 0) out.push_back({cc, builtin[(size_t) cc]});
    return out;
}

// controller moves arrive on the audio thread; they are handed to the host here so automation and the UI follow
void AcidBoxProcessor::timerCallback() {
    for (size_t i = 0; i < allParams.size(); ++i) {
        const float v = ccValue[i].exchange(-1.0f);
        if (v >= 0.0f) allParams[i]->setValueNotifyingHost(v);
    }
}

bool AcidBoxProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AcidBoxProcessor::prepareToPlay(double sampleRate, int) {
    core = std::make_unique<acid::Core>(sampleRate);
    core->setSeq(getSeq());
    seqDirty = false;
    wasExt = false;
    lastK = std::numeric_limits<int64_t>::min();
}

void AcidBoxProcessor::pushCmd(Cmd c) {
    const auto scope = fifo.write(1);
    if (scope.blockSize1 > 0) cmds[(size_t) scope.startIndex1] = c;
    else if (scope.blockSize2 > 0) cmds[(size_t) scope.startIndex2] = c;
}

// General MIDI drum notes on channel 10
static int drumForNote(int note) {
    switch (note) {
        case 35: case 36: return acid::Kick;          // B0, C1
        case 38: case 40: return acid::Snare;         // D1, E1
        case 39: return acid::Clap;                   // D#1
        case 42: case 44: return acid::ClosedHat;     // F#1, G#1
        case 46: return acid::OpenHat;                // A#1
        default: return 0;
    }
}

void AcidBoxProcessor::handleMidi(const juce::MidiMessage& m) {
    if (m.getChannel() == 10 && (m.isNoteOn() || m.isNoteOff())) {   // drums; note-offs are ignored, hits ring out
        if (m.isNoteOn())
            if (const int bit = drumForNote(m.getNoteNumber())) core->trigDrums(bit, (double) m.getVelocity() / 127.0);
        return;
    }
    if (m.isNoteOn()) core->liveOn(m.getNoteNumber(), (int) m.getVelocity());
    else if (m.isNoteOff()) core->liveOff(m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) core->allOff();
    else if (m.isController()) {
        const int cc = m.getControllerNumber();
        const float v = (float) m.getControllerValue() / 127.0f;
        const int arm = learnTarget.load();
        if (arm >= 0 && cc < 120) {
            for (auto& l : learned) if (l.load() == arm) l = -1;     // one control per knob
            learned[(size_t) cc] = arm;
            learnTarget = -1;
            lastLearnIndex = arm;
            lastLearnCC = cc;
            ++mapVersion;
            ++learnCount;
            ccValue[(size_t) arm] = v;
            return;
        }
        int t = learned[(size_t) cc].load();
        if (t < 0) t = builtin[(size_t) cc];
        if (t >= 0 && t < (int) allParams.size()) ccValue[(size_t) t] = v;
    }
}

void AcidBoxProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (core == nullptr || buffer.getNumChannels() < 2 || n <= 0) return;
    auto& c = *core;

    // sequencer edits from the editor
    if (seqDirty.exchange(false)) {
        const juce::SpinLock::ScopedTryLockType tl(seqLock);
        if (tl.isLocked()) c.setSeq(seq);
        else seqDirty = true;
    }

    // parameters
    for (const auto& b : bindings) c.p.*(b.m) = (double) b.src->load();
    c.p.wave = juce::jlimit(0, 4, (int) pWave->load());
    c.p.wave2 = juce::jlimit(0, 4, (int) pWave2->load());
    c.p.interval = kIntervals[juce::jlimit(0, 3, (int) pInterval->load())];
    c.p.dtype = juce::jlimit(0, 4, (int) pDtype->load()) - 1;
    c.p.rmode = juce::jlimit(0, 3, (int) pRmode->load()) - 1;
    c.p.space = pSpace->load() > 0.5f ? 1 : 0;
    c.p.ftype = juce::jlimit(0, 1, (int) pFtype->load());
    c.p.cmp = pCmp->load() > 0.5f ? 1 : 0;

    // clicks from the editor (note audition, drum preview)
    fifo.read(fifo.getNumReady()).forEach([&](int i) {
        const Cmd& m = cmds[(size_t) i];
        if (m.type == 0) c.audition(m.a, m.b, m.c != 0);
        else if (m.type == 1) c.trigDrums(m.a);
    });

    // host transport
    bool hostPlaying = false, havePpq = false;
    double bpm = 0, ppq = 0;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
        }
    }
    hostBpm = bpm;
    const bool sync = pSync->load() > 0.5f;
    const bool ext = sync && hostPlaying && havePpq && bpm > 0;
    c.p.tempo = (sync && bpm > 0) ? bpm : (double) pTempo->load();

    if (ext) {
        if (!wasExt) {
            c.setPlaying(false);
            c.setPlaying(true);
            lastK = std::numeric_limits<int64_t>::min();
            internalPlay = false;
        }
        c.externalClock = true;
    } else {
        if (wasExt) c.setPlaying(false);
        c.externalClock = false;
        const bool want = internalPlay.load();
        if (want != c.playing) c.setPlaying(want);
    }
    wasExt = ext;

    // where the sixteenth notes fall inside this block
    struct Bound { int s; int64_t k; };
    Bound bounds[64];
    int nb = 0;
    const double sps = getSampleRate() * 60.0 / c.p.tempo / 4.0;
    if (ext) {
        const double q16 = ppq * 4.0;
        for (int64_t k = (int64_t) std::ceil(q16 - 0.5 / sps - 1e-9); nb < 64; ++k) {
            const int s = juce::jmax(0, (int) std::floor((k - q16) * sps + 0.5));
            if (s >= n) break;
            if (k != lastK) bounds[nb++] = {s, k};
        }
    }

    // render, splitting the block at every step and every MIDI event
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);
    int cursor = 0, bi = 0;
    auto renderTo = [&](int end) {
        if (end > cursor) { c.process(L + cursor, R + cursor, end - cursor); cursor = end; }
    };
    auto it = midi.cbegin();
    const auto itEnd = midi.cend();
    while (bi < nb || it != itEnd) {
        const int nextB = bi < nb ? bounds[bi].s : n;
        const int nextM = it != itEnd ? juce::jlimit(0, n, (*it).samplePosition) : n;
        if (bi < nb && nextB <= nextM) {
            renderTo(nextB);
            c.triggerExternal(bounds[bi].k, sps);
            lastK = bounds[bi].k;
            ++bi;
        } else {
            renderTo(nextM);
            handleMidi((*it).getMessage());
            ++it;
        }
    }
    renderTo(n);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear(ch, 0, n);

    running = c.playing;
    hostDriven = ext;
    uiStep = c.lastStep;
    uiBarPos = c.lastBarPos;
    uiBank = c.ab;
    uiDBank = c.adb;
}

// ---------- state: knobs through the parameter tree, patterns and song as a child node ----------
namespace {
juce::ValueTree seqToTree(const acid::SeqState& q) {
    juce::ValueTree t("SEQ");
    for (int b = 0; b < 4; ++b) {
        juce::String steps, drums;
        for (int i = 0; i < 16; ++i) {
            const auto& x = q.banks[b][i];
            steps << x.n << ":" << x.o << ":" << (int) x.a << ":" << (int) x.s << ":" << (int) x.r << " ";
            drums << q.drums[b][i] << " ";
        }
        t.setProperty("b" + juce::String(b), steps.trim(), nullptr);
        t.setProperty("d" + juce::String(b), drums.trim(), nullptr);
    }
    juce::String chain;
    for (int i = 0; i < q.chainLen; ++i) chain << q.chain[i].b << ":" << q.chain[i].d << " ";
    t.setProperty("chain", chain.trim(), nullptr);
    t.setProperty("songOn", q.songOn, nullptr);
    t.setProperty("loopBank", q.loopBank, nullptr);
    t.setProperty("loopDrum", q.loopDrum, nullptr);
    return t;
}

acid::SeqState treeToSeq(const juce::ValueTree& t) {
    acid::SeqState q = acid::defaultSeq();
    for (int b = 0; b < 4; ++b) {
        const auto steps = juce::StringArray::fromTokens(t.getProperty("b" + juce::String(b)).toString(), " ", "");
        for (int i = 0; i < 16 && i < steps.size(); ++i) {
            const auto f = juce::StringArray::fromTokens(steps[i], ":", "");
            if (f.size() != 5) continue;
            auto& x = q.banks[b][i];
            x.n = juce::jlimit(0, 12, f[0].getIntValue());
            x.o = juce::jlimit(-1, 1, f[1].getIntValue());
            x.a = f[2].getIntValue() != 0;
            x.s = f[3].getIntValue() != 0;
            x.r = f[4].getIntValue() != 0;
        }
        const auto drums = juce::StringArray::fromTokens(t.getProperty("d" + juce::String(b)).toString(), " ", "");
        for (int i = 0; i < 16 && i < drums.size(); ++i) q.drums[b][i] = drums[i].getIntValue() & 31;
    }
    const auto chain = juce::StringArray::fromTokens(t.getProperty("chain").toString(), " ", "");
    if (chain.size() > 0) {
        q.chainLen = juce::jmin(16, chain.size());
        for (int i = 0; i < q.chainLen; ++i) {
            const auto f = juce::StringArray::fromTokens(chain[i], ":", "");
            q.chain[i].b = f[0].getIntValue() & 3;
            q.chain[i].d = (f.size() > 1 ? f[1].getIntValue() : q.chain[i].b) & 3;
        }
    }
    q.songOn = (bool) t.getProperty("songOn", false);
    q.loopBank = (int) t.getProperty("loopBank", 0) & 3;
    q.loopDrum = (int) t.getProperty("loopDrum", 0) & 3;
    return q;
}
} // namespace

void AcidBoxProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    auto old = state.getChildWithName("SEQ");
    if (old.isValid()) state.removeChild(old, nullptr);
    auto seqTree = seqToTree(getSeq());
    juce::String map;
    for (const auto& [cc, idx] : learnedList()) map << cc << ":" << allParams[(size_t) idx]->getParameterID() << " ";
    seqTree.setProperty("learned", map.trim(), nullptr);
    if (skinIndex.load() >= 0) seqTree.setProperty("skin", skinIndex.load(), nullptr);
    if (uiScale.load() > 0.0f) seqTree.setProperty("scale", (double) uiScale.load(), nullptr);
    state.appendChild(seqTree, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void AcidBoxProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr) return;
    auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.hasType(apvts.state.getType())) return;
    auto s = tree.getChildWithName("SEQ");
    if (s.isValid()) {
        const auto q = treeToSeq(s);
        editSeq([&](acid::SeqState& x) { x = q; });
        for (auto& l : learned) l = -1;
        for (const auto& tok : juce::StringArray::fromTokens(s.getProperty("learned").toString(), " ", "")) {
            const int cc = tok.upToFirstOccurrenceOf(":", false, false).getIntValue();
            const int idx = paramIndex(tok.fromFirstOccurrenceOf(":", false, false));
            if (cc >= 0 && cc < 120 && idx >= 0) learned[(size_t) cc] = idx;
        }
        ++mapVersion;
        if (s.hasProperty("skin")) skinIndex = (int) s.getProperty("skin");
        if (s.hasProperty("scale")) uiScale = (float) (double) s.getProperty("scale");
        tree.removeChild(s, nullptr);
    }
    apvts.replaceState(tree);
}

juce::AudioProcessorEditor* AcidBoxProcessor::createEditor() { return new AcidBoxEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AcidBoxProcessor(); }
