#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

namespace ui {
// the same colour tokens as the web version's skins
struct Palette {
    juce::Colour bg, panel, ink, soft, track, knobBody, knobMark, led, onLed, cell, bar, keyWhite, keyBlack, keySel;
};
struct Skin { const char* name; Palette pal; };
inline juce::Colour c(juce::uint32 argb) { return juce::Colour(argb); }
inline Palette mk(juce::uint32 bg, juce::uint32 panel, juce::uint32 ink, juce::uint32 soft, juce::uint32 knobBody, juce::uint32 knobMark,
                  juce::uint32 led, juce::uint32 onLed, juce::uint32 cell, juce::uint32 bar, juce::uint32 kw, juce::uint32 kb, juce::uint32 ks,
                  float trackAlpha) {
    return {c(bg), c(panel), c(ink), c(soft), c(ink).withAlpha(trackAlpha), c(knobBody), c(knobMark), c(led), c(onLed), c(cell), c(bar), c(kw), c(kb), c(ks)};
}
// index 0 is Auto: Butter in light mode, Midnight in dark mode. New skins go at the end so saved projects keep theirs.
inline const std::vector<Skin>& skins() {
    static const std::vector<Skin> s = {
        {"Auto", {}},
        {"Butter", mk(0xFFF0CF3E, 0xFFE6BF2B, 0xFF1D2547, 0xFF545571, 0xFF1D2547, 0xFFF0CF3E, 0xFFD93A25, 0xFFFFFFFF, 0xFFEDC93A, 0xFF1D2547, 0xFFFBF2CC, 0xFF1D2547, 0xFFD93A25, 0.2f)},
        {"Butter Night", mk(0xFF0B0B0A, 0xFF171714, 0xFFF0CF3E, 0xFFA99B5E, 0xFFF0CF3E, 0xFF0B0B0A, 0xFFFF5B40, 0xFF0B0B0A, 0xFF22221E, 0xFFF0CF3E, 0xFF2A2924, 0xFF050504, 0xFFFF5B40, 0.2f)},
        {"Midnight", mk(0xFF121935, 0xFF1A2347, 0xFFF0CF3E, 0xFFA9A487, 0xFFF0CF3E, 0xFF121935, 0xFFFF5B40, 0xFF121935, 0xFF212C58, 0xFFF0CF3E, 0xFF2A3666, 0xFF0C1128, 0xFFFF5B40, 0.22f)},
        {"Concrete", mk(0xFFC9C5BC, 0xFFBDB8AE, 0xFF1E1E1E, 0xFF46433E, 0xFF1E1E1E, 0xFFF26B1D, 0xFFC24A0A, 0xFFFFFFFF, 0xFFD3CFC6, 0xFF1E1E1E, 0xFFF1EEE8, 0xFF1E1E1E, 0xFFC24A0A, 0.18f)},
        {"Concrete Night", mk(0xFF1C1B19, 0xFF262522, 0xFFD9D5CC, 0xFFA39E94, 0xFFD9D5CC, 0xFF1C1B19, 0xFFFF7A2E, 0xFF1C1B19, 0xFF302E2B, 0xFFD9D5CC, 0xFF3A3834, 0xFF121110, 0xFFFF7A2E, 0.2f)},
        {"Bubblegum", mk(0xFFF6B3CF, 0xFFEFA0C2, 0xFF3B1342, 0xFF5A2E5D, 0xFF3B1342, 0xFFF6B3CF, 0xFFC8301A, 0xFFFFFFFF, 0xFFF8C2D9, 0xFF3B1342, 0xFFFFF0F6, 0xFF3B1342, 0xFFC8301A, 0.2f)},
        {"Bubblegum Night", mk(0xFF2A0E30, 0xFF36133D, 0xFFF6B3CF, 0xFFC08AA6, 0xFFF6B3CF, 0xFF2A0E30, 0xFFFF6A55, 0xFF2A0E30, 0xFF44194C, 0xFFF6B3CF, 0xFF4E1F57, 0xFF1A081E, 0xFFFF6A55, 0.22f)},
        {"Lagoon", mk(0xFF0D4A57, 0xFF0F5868, 0xFFF4E5C2, 0xFFA9C6C0, 0xFFF4E5C2, 0xFF0D4A57, 0xFFFF7A59, 0xFF0D4A57, 0xFF14687A, 0xFFF4E5C2, 0xFF1A7486, 0xFF08333C, 0xFFFF7A59, 0.22f)},
    };
    return s;
}
inline const Palette& resolve(int index, bool osDark) {
    const auto& s = skins();
    if (index <= 0 || index >= (int) s.size()) return s[osDark ? 3 : 1].pal;
    return s[(size_t) index].pal;
}
inline juce::Font font(float h, bool bold = false) {
    auto o = juce::FontOptions(h);
    if (bold) o = o.withStyle("Bold");
    return juce::Font(o);
}
} // namespace ui

class AcidLook : public juce::LookAndFeel_V4 {
public:
    AcidLook();
    void setPalette(const ui::Palette& p);
    ui::Palette pal;
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) override;
};

using APVTS = juce::AudioProcessorValueTreeState;

// the palette of the skin a component is drawn with
inline const ui::Palette& palOf(const juce::Component& c) {
    if (auto* l = dynamic_cast<const AcidLook*>(&c.getLookAndFeel())) return l->pal;
    return ui::resolve(1, false);
}

// the Skins tab: one swatch per skin
class SkinPage : public juce::Component {
public:
    std::function<void(int)> onPick;
    int selected = 0;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    std::vector<juce::Rectangle<int>> swatches;
};

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
    void applySkin(int index);
    bool appliedDark = false;
    juce::uint32 nextDarkCheck = 0;
    SkinPage skinPage;
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
