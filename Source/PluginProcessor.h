#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <vector>
#include "AcidCore.h"

class AcidBoxProcessor : public juce::AudioProcessor, private juce::Timer {
public:
    AcidBoxProcessor();
    ~AcidBoxProcessor() override { stopTimer(); }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // sequencer data: the editor changes it, the audio thread picks up a copy at the next block
    acid::SeqState getSeq() const {
        const juce::SpinLock::ScopedLockType sl(seqLock);
        return seq;
    }
    template <typename Fn> void editSeq(Fn&& fn) {
        {
            const juce::SpinLock::ScopedLockType sl(seqLock);
            fn(seq);
        }
        seqDirty = true;
        ++seqVersion;
    }

    void audition(int n, int o, bool a) { pushCmd({0, n, o, a ? 1 : 0}); }
    void previewDrum(int bits) { pushCmd({1, bits, 0, 0}); }

    // ---- MIDI controllers: built-in map plus MIDI Learn (your mappings win) ----
    juce::RangedAudioParameter* param(int index) const { return index >= 0 && index < (int) allParams.size() ? allParams[(size_t) index] : nullptr; }
    int paramIndex(const juce::String& id) const;
    void learnArm(const juce::String& id) { learnTarget = paramIndex(id); }
    void learnCancel() { learnTarget = -1; }
    int armedIndex() const { return learnTarget.load(); }
    int learnedCC(int index) const;     // your mapping for this parameter, or -1
    int builtinCC(int index) const;     // built-in mapping still in effect, or -1
    void forgetCC(int cc) { if (cc >= 0 && cc < 128) learned[(size_t) cc] = -1; ++mapVersion; }
    void forgetAll() { for (auto& l : learned) l = -1; ++mapVersion; }
    std::vector<std::pair<int, int>> learnedList() const;    // (cc, parameter index)
    std::vector<std::pair<int, int>> builtinList() const;
    std::atomic<int> lastLearnCC{-1}, lastLearnIndex{-1}, mapVersion{0}, learnCount{0};

    // shared with the editor
    std::atomic<bool> internalPlay{false}, running{false}, hostDriven{false};
    std::atomic<int> uiStep{-1}, uiBarPos{-1}, uiBank{0}, uiDBank{0}, seqVersion{0};
    std::atomic<double> hostBpm{0.0};
    std::atomic<int> skinIndex{-1};   // -1: not chosen in this project yet, use the last skin you picked

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Cmd { int type, a, b, c; };
    void pushCmd(Cmd c);
    void handleMidi(const juce::MidiMessage& m);
    void timerCallback() override;

    std::vector<juce::RangedAudioParameter*> allParams;
    std::array<int, 128> builtin{};
    std::array<std::atomic<int>, 128> learned;
    std::array<std::atomic<float>, 256> ccValue;    // pending controller moves, applied on the message thread
    std::atomic<int> learnTarget{-1};

    std::unique_ptr<acid::Core> core;
    acid::SeqState seq;
    mutable juce::SpinLock seqLock;
    std::atomic<bool> seqDirty{true};

    struct Binding { double acid::Params::* m; std::atomic<float>* src; };
    std::vector<Binding> bindings;
    std::atomic<float>* pTempo = nullptr;
    std::atomic<float>* pWave = nullptr;
    std::atomic<float>* pWave2 = nullptr;
    std::atomic<float>* pInterval = nullptr;
    std::atomic<float>* pDtype = nullptr;
    std::atomic<float>* pRmode = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pCmp = nullptr;

    juce::AbstractFifo fifo{128};
    std::array<Cmd, 128> cmds{};

    bool wasExt = false;
    int64_t lastK = std::numeric_limits<int64_t>::min();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcidBoxProcessor)
};
