#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

namespace ui {
inline const juce::Colour butter{0xFFF0CF3E}, ink{0xFF1D2547}, led{0xFFD93A25}, navy{0xFF121935}, soft{0xFF2B3566};
inline juce::Font font(float h, bool bold = false) {
    auto o = juce::FontOptions(h);
    if (bold) o = o.withStyle("Bold");
    return juce::Font(o);
}
} // namespace ui

class AcidLook : public juce::LookAndFeel_V4 {
public:
    AcidLook();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) override;
};

using APVTS = juce::AudioProcessorValueTreeState;

class Knob : public juce::Component {
public:
    Knob(AcidBoxProcessor& p, const juce::String& id, const juce::String& label);
    void resized() override;
    void paintOverChildren(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    AcidBoxProcessor& proc;
    juce::String paramId;
    int index = -1;
    juce::Label name;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> attach;
};

class Toggle : public juce::Component {
public:
    Toggle(APVTS& s, const juce::String& id, const juce::String& label);
    void resized() override;
private:
    juce::Label name;
    juce::ToggleButton button{"On"};
    std::unique_ptr<APVTS::ButtonAttachment> attach;
};

class Choice : public juce::Component {
public:
    Choice(APVTS& s, const juce::String& id, const juce::String& label);
    void resized() override;
private:
    juce::Label name;
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> attach;
};

class Section : public juce::Component {
public:
    Section(AcidBoxProcessor& p, const juce::String& t) : proc(p), title(t) {}
    Section& knob(const juce::String& id, const juce::String& label) { add(new Knob(proc, id, label), 62); return *this; }
    Section& choice(const juce::String& id, const juce::String& label, int w = 96) { add(new Choice(proc.apvts, id, label), w); return *this; }
    Section& toggle(const juce::String& id, const juce::String& label, int w = 70) { add(new Toggle(proc.apvts, id, label), w); return *this; }
    int preferredWidth() const { int w = 12; for (int x : widths) w += x; return w; }
    static constexpr int preferredHeight = 108;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void add(juce::Component* c, int w) { items.add(c); widths.push_back(w); addAndMakeVisible(c); }
    AcidBoxProcessor& proc;
    juce::String title;
    juce::OwnedArray<juce::Component> items;
    std::vector<int> widths;
};

class Page : public juce::Component {
public:
    explicit Page(AcidBoxProcessor& p) : proc(p) {}
    Section& section(const juce::String& title) {
        auto* x = sections.add(new Section(proc, title));
        addAndMakeVisible(x);
        return *x;
    }
    void resized() override;
private:
    AcidBoxProcessor& proc;
    juce::OwnedArray<Section> sections;
};

// step sequencer, drum grid and song chain, all drawn by hand
class SeqView : public juce::Component {
public:
    explicit SeqView(AcidBoxProcessor& p) : proc(p) {}
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void sync();
private:
    AcidBoxProcessor& proc;
    int editBank = 0, editDBank = 0, sel = 0;
    int shownStep = -2, shownBar = -2, shownVersion = -1;
    bool shownRun = false;
    juce::Rectangle<int> bassBankLabel, drumBankLabel, bassBank[4], drumBank[4], loopBtn, songBtn, clearBass, clearDrums;
    juce::Rectangle<int> stepsLabel, steps[16], editLabel, keys[13], octDown, octLabel, octUp, accentBtn, slideBtn, restBtn;
    juce::Rectangle<int> drumLabels[5], drumCells[5][16], songLabel, removeBar, chainCells[16], hintArea;
};

class AcidBoxEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit AcidBoxEditor(AcidBoxProcessor&);
    ~AcidBoxEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;

    AcidBoxProcessor& proc;
    AcidLook look;
    void showMidiMenu();
    juce::TextButton playBtn{"Play"};
    juce::TextButton midiBtn{"MIDI"};
    int shownMap = -1, shownArm = -2, shownLearn = 0;
    juce::uint32 learnMsgUntil = 0;
    juce::ToggleButton syncBtn{"Sync to host"};
    juce::Slider tempo;
    juce::Label tempoLabel, status;
    std::unique_ptr<APVTS::ButtonAttachment> syncAttach;
    std::unique_ptr<APVTS::SliderAttachment> tempoAttach;
    Page bassPage, fxPage, drumPage;
    juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};
    SeqView seqView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcidBoxEditor)
};
