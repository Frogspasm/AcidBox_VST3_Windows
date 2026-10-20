#include "PluginEditor.h"

namespace {
const char* const kNoteNames[13] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C"};
const char* const kDrumNames[5] = {"Kick", "Snare", "Clap", "Closed hat", "Open hat"};
const int kDrumBits[5] = {acid::Kick, acid::Snare, acid::Clap, acid::ClosedHat, acid::OpenHat};
bool isBlack(int k) { const int m = k % 12; return m == 1 || m == 3 || m == 6 || m == 8 || m == 10; }
juce::String letter(int i) { return juce::String::charToString((juce::juce_wchar) ('A' + i)); }
} // namespace

// ---------------------------------------------------------------- look
AcidLook::AcidLook() {
    setColour(juce::ResizableWindow::backgroundColourId, ui::butter);
    setColour(juce::Label::textColourId, ui::ink);
    setColour(juce::Slider::textBoxTextColourId, ui::ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, ui::led.withAlpha(0.4f));
    setColour(juce::Slider::thumbColourId, ui::led);
    setColour(juce::Slider::trackColourId, ui::ink);
    setColour(juce::Slider::backgroundColourId, ui::ink.withAlpha(0.2f));
    setColour(juce::ComboBox::backgroundColourId, ui::ink);
    setColour(juce::ComboBox::textColourId, ui::butter);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::arrowColourId, ui::butter);
    setColour(juce::PopupMenu::backgroundColourId, ui::ink);
    setColour(juce::PopupMenu::textColourId, ui::butter);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::led);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TextButton::buttonColourId, ui::ink);
    setColour(juce::TextButton::buttonOnColourId, ui::led);
    setColour(juce::TextButton::textColourOffId, ui::butter);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::ToggleButton::textColourId, ui::ink);
    setColour(juce::ToggleButton::tickColourId, ui::ink);
    setColour(juce::ToggleButton::tickDisabledColourId, ui::ink.withAlpha(0.4f));
    setColour(juce::TabbedButtonBar::tabTextColourId, ui::ink.withAlpha(0.7f));
    setColour(juce::TabbedButtonBar::frontTextColourId, ui::ink);
    setColour(juce::TabbedButtonBar::tabOutlineColourId, ui::ink.withAlpha(0.3f));
    setColour(juce::TabbedButtonBar::frontOutlineColourId, ui::ink);
    setColour(juce::TabbedComponent::backgroundColourId, ui::butter);
    setColour(juce::TabbedComponent::outlineColourId, ui::ink.withAlpha(0.3f));
}

void AcidLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) {
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(3.0f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startA + pos * (endA - startA);
    const juce::PathStrokeType stroke(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc(c.x, c.y, r, r, 0.0f, startA, endA, true);
    g.setColour(ui::ink.withAlpha(0.2f));
    g.strokePath(track, stroke);
    juce::Path value;
    value.addCentredArc(c.x, c.y, r, r, 0.0f, startA, ang, true);
    g.setColour(ui::led);
    g.strokePath(value, stroke);

    const float br = juce::jmax(2.0f, r - 6.0f);
    g.setColour(ui::ink);
    g.fillEllipse(c.x - br, c.y - br, br * 2, br * 2);
    g.setColour(ui::butter);
    g.drawLine(c.x + std::sin(ang) * br * 0.3f, c.y - std::cos(ang) * br * 0.3f,
               c.x + std::sin(ang) * (br - 3), c.y - std::cos(ang) * (br - 3), 2.5f);
}

// ---------------------------------------------------------------- knobs and sections
Knob::Knob(AcidBoxProcessor& p, const juce::String& id, const juce::String& label)
    : proc(p), paramId(id), index(p.paramIndex(id)) {
    auto& s = p.apvts;
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    name.setText(label, juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(ui::font(12.0f, true));
    name.setMinimumHorizontalScale(0.7f);
    addAndMakeVisible(name);
    addAndMakeVisible(slider);
    attach = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    if (auto* prm = s.getParameter(id)) slider.setDoubleClickReturnValue(true, prm->convertFrom0to1(prm->getDefaultValue()));
    slider.setPopupMenuEnabled(false);
    slider.addMouseListener(this, false);
    name.addMouseListener(this, false);
}

// a small "CC 74" badge shows which controller moves this knob; a red frame while it waits for MIDI Learn
void Knob::paintOverChildren(juce::Graphics& g) {
    if (index >= 0 && proc.armedIndex() == index) {
        g.setColour(ui::led);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 2.0f);
    }
    int cc = proc.learnedCC(index);
    const bool mine = cc >= 0;
    if (!mine) cc = proc.builtinCC(index);
    if (cc < 0) return;
    g.setColour(mine ? ui::led : ui::ink.withAlpha(0.45f));
    g.setFont(ui::font(9.0f, true));
    g.drawText("CC " + juce::String(cc), getWidth() - 34, 15, 32, 11, juce::Justification::centredRight);
}

void Knob::mouseDown(const juce::MouseEvent& e) {
    if (!e.mods.isPopupMenu() || index < 0) return;
    const int lc = proc.learnedCC(index), bc = proc.builtinCC(index);
    const bool armed = proc.armedIndex() == index;
    juce::PopupMenu m;
    m.addSectionHeader(proc.param(index)->getName(40));
    m.addItem(1, armed ? "Cancel MIDI Learn" : "MIDI Learn (then move a control)");
    if (lc >= 0) m.addItem(2, "Forget CC " + juce::String(lc));
    else if (bc >= 0) m.addItem(3, "Built-in: CC " + juce::String(bc), false);
    juce::Component::SafePointer<Knob> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safe, lc](int r) {
        auto* k = safe.getComponent();
        if (k == nullptr) return;
        auto& pr = k->proc;
        if (r == 1) { if (pr.armedIndex() == k->index) pr.learnCancel(); else pr.learnArm(k->paramId); }
        else if (r == 2) pr.forgetCC(lc);
        if (auto* top = k->getTopLevelComponent()) top->repaint();
    });
}

Toggle::Toggle(APVTS& s, const juce::String& id, const juce::String& label) {
    name.setText(label, juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(ui::font(12.0f, true));
    addAndMakeVisible(name);
    addAndMakeVisible(button);
    attach = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
}

void Toggle::resized() {
    auto r = getLocalBounds();
    name.setBounds(r.removeFromTop(16));
    button.setBounds(r.withSizeKeepingCentre(juce::jmin(r.getWidth(), 56), 26));
}

void Knob::resized() {
    auto r = getLocalBounds();
    name.setBounds(r.removeFromTop(16));
    slider.setBounds(r);
}

Choice::Choice(APVTS& s, const juce::String& id, const juce::String& label) {
    name.setText(label, juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(ui::font(12.0f, true));
    addAndMakeVisible(name);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(id))) box.addItemList(p->choices, 1);
    addAndMakeVisible(box);
    attach = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
}

void Choice::resized() {
    auto r = getLocalBounds();
    name.setBounds(r.removeFromTop(16));
    box.setBounds(r.withSizeKeepingCentre(r.getWidth() - 8, 26));
}

void Section::paint(juce::Graphics& g) {
    g.setColour(ui::ink.withAlpha(0.6f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.5f);
    g.setColour(ui::ink);
    g.setFont(ui::font(11.0f, true));
    g.drawText(title.toUpperCase(), getLocalBounds().reduced(10, 5).removeFromTop(14), juce::Justification::centredLeft);
}

void Section::resized() {
    auto r = getLocalBounds().reduced(6);
    r.removeFromTop(18);
    for (int i = 0; i < items.size(); ++i) items[i]->setBounds(r.removeFromLeft(widths[(size_t) i]));
}

void Page::resized() {
    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignContent = juce::FlexBox::AlignContent::flexStart;
    for (auto* s : sections)
        fb.items.add(juce::FlexItem(*s).withWidth((float) s->preferredWidth()).withHeight((float) Section::preferredHeight).withMargin(4.0f));
    fb.performLayout(getLocalBounds().reduced(4));
}

// ---------------------------------------------------------------- sequencer
void SeqView::resized() {
    const auto r = getLocalBounds().reduced(12, 10);
    const int labelW = 92;
    const int gx = r.getX() + labelW, gw = r.getWidth() - labelW;
    auto col = [&](int i, int y, int h) {
        const int x0 = gx + gw * i / 16, x1 = gx + gw * (i + 1) / 16;
        return juce::Rectangle<int>(x0 + 2, y, x1 - x0 - 4, h);
    };
    int y = r.getY();

    auto row = juce::Rectangle<int>(r.getX(), y, r.getWidth(), 26);
    bassBankLabel = row.removeFromLeft(46);
    for (auto& b : bassBank) { b = row.removeFromLeft(30); row.removeFromLeft(4); }
    row.removeFromLeft(18);
    drumBankLabel = row.removeFromLeft(56);
    for (auto& b : drumBank) { b = row.removeFromLeft(30); row.removeFromLeft(4); }
    clearDrums = row.removeFromRight(100); row.removeFromRight(6);
    clearBass = row.removeFromRight(100); row.removeFromRight(18);
    songBtn = row.removeFromRight(100); row.removeFromRight(4);
    loopBtn = row.removeFromRight(112);
    y += 38;

    stepsLabel = {r.getX(), y, labelW, 48};
    for (int i = 0; i < 16; ++i) steps[i] = col(i, y, 48);
    y += 56;

    editLabel = {r.getX(), y, labelW, 44};
    auto ed = juce::Rectangle<int>(gx + 2, y, gw - 4, 44);
    for (auto& k : keys) { k = ed.removeFromLeft(34); ed.removeFromLeft(3); }
    ed.removeFromLeft(14);
    octDown = ed.removeFromLeft(32); octLabel = ed.removeFromLeft(58); octUp = ed.removeFromLeft(32);
    ed.removeFromLeft(14);
    accentBtn = ed.removeFromLeft(76); ed.removeFromLeft(6);
    slideBtn = ed.removeFromLeft(76); ed.removeFromLeft(6);
    restBtn = ed.removeFromLeft(76);
    y += 54;

    for (int d = 0; d < 5; ++d) {
        drumLabels[d] = {r.getX(), y + d * 22, labelW - 8, 20};
        for (int i = 0; i < 16; ++i) drumCells[d][i] = col(i, y + d * 22, 20);
    }
    y += 5 * 22 + 10;

    songLabel = {r.getX(), y, labelW - 40, 30};
    removeBar = {r.getX() + labelW - 38, y + 3, 30, 24};
    for (int i = 0; i < 16; ++i) chainCells[i] = col(i, y, 30);
    y += 40;
    hintArea = {r.getX(), y, r.getWidth(), 18};
}

void SeqView::paint(juce::Graphics& g) {
    const auto q = proc.getSeq();
    const bool running = proc.running.load();
    const int playStep = running ? proc.uiStep.load() : -1;
    const int playBar = running ? proc.uiBarPos.load() : -1;
    const int playBank = proc.uiBank.load(), playDBank = proc.uiDBank.load();
    const auto& cur = q.banks[editBank][sel];

    g.setColour(ui::navy);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);

    auto label = [&](juce::Rectangle<int> r, const juce::String& t) {
        g.setColour(ui::butter.withAlpha(0.75f));
        g.setFont(ui::font(11.0f, true));
        g.drawText(t, r, juce::Justification::centredLeft);
    };
    auto button = [&](juce::Rectangle<int> r, const juce::String& t, bool on, bool lit = false) {
        const auto f = r.toFloat();
        g.setColour(on ? ui::butter : ui::soft);
        g.fillRoundedRectangle(f, 5.0f);
        g.setColour(on ? ui::ink : ui::butter);
        g.setFont(ui::font(13.0f, true));
        g.drawText(t, r, juce::Justification::centred);
        if (lit) { g.setColour(ui::led); g.fillEllipse(f.getRight() - 8.0f, f.getY() + 3.0f, 5.0f, 5.0f); }
    };

    label(bassBankLabel, "BASS");
    for (int i = 0; i < 4; ++i) button(bassBank[i], letter(i), editBank == i, running && playBank == i);
    label(drumBankLabel, "DRUMS");
    for (int i = 0; i < 4; ++i) button(drumBank[i], letter(i), editDBank == i, running && playDBank == i);
    button(loopBtn, "Loop pattern", !q.songOn);
    button(songBtn, "Play song", q.songOn);
    button(clearBass, "Clear bass", false);
    button(clearDrums, "Clear drums", false);

    // bass steps
    label(stepsLabel, "BASS LINE");
    for (int i = 0; i < 16; ++i) {
        const auto& s = q.banks[editBank][i];
        const auto f = steps[i].toFloat();
        g.setColour((i / 4) % 2 ? ui::soft : ui::soft.brighter(0.15f));
        g.fillRoundedRectangle(f, 6.0f);
        if (i == playStep) { g.setColour(ui::led.withAlpha(0.35f)); g.fillRoundedRectangle(f, 6.0f); }
        if (i == sel) { g.setColour(ui::butter); g.drawRoundedRectangle(f.reduced(1.0f), 6.0f, 2.0f); }
        g.setColour(s.r ? ui::butter.withAlpha(0.3f) : ui::butter);
        g.setFont(ui::font(15.0f, true));
        g.drawText(s.r ? juce::String("-") : juce::String(kNoteNames[s.n]), steps[i].withTrimmedBottom(16), juce::Justification::centred);
        if (!s.r) {
            auto badges = steps[i].withTrimmedTop(30).reduced(4, 0);
            g.setFont(ui::font(10.0f, true));
            if (s.o != 0) { g.setColour(ui::butter.withAlpha(0.7f)); g.drawText(s.o > 0 ? "hi" : "lo", badges, juce::Justification::centredLeft); }
            if (s.a) { g.setColour(ui::led); g.drawText("A", badges, juce::Justification::centred); }
            if (s.s) { g.setColour(ui::butter); g.drawText("S", badges, juce::Justification::centredRight); }
        }
    }

    // step editor
    label(editLabel, "STEP " + juce::String(sel + 1));
    for (int k = 0; k < 13; ++k) {
        const bool on = !cur.r && cur.n == k;
        const auto f = keys[k].toFloat();
        g.setColour(on ? ui::led : isBlack(k) ? ui::ink : ui::butter.withAlpha(0.9f));
        g.fillRoundedRectangle(f, 4.0f);
        g.setColour(on ? juce::Colours::white : isBlack(k) ? ui::butter : ui::ink);
        g.setFont(ui::font(11.0f, true));
        g.drawText(kNoteNames[k], keys[k].withTrimmedTop(20), juce::Justification::centred);
    }
    button(octDown, "-", false);
    g.setColour(ui::butter);
    g.setFont(ui::font(12.0f, true));
    g.drawText(cur.o > 0 ? "Oct high" : cur.o < 0 ? "Oct low" : "Oct mid", octLabel, juce::Justification::centred);
    button(octUp, "+", false);
    button(accentBtn, "Accent", !cur.r && cur.a);
    button(slideBtn, "Slide", !cur.r && cur.s);
    button(restBtn, "Rest", cur.r);

    // drum grid
    for (int d = 0; d < 5; ++d) {
        label(drumLabels[d], kDrumNames[d]);
        for (int i = 0; i < 16; ++i) {
            const bool on = (q.drums[editDBank][i] & kDrumBits[d]) != 0;
            const auto f = drumCells[d][i].toFloat();
            g.setColour(on ? ui::led : ((i / 4) % 2 ? ui::soft : ui::soft.brighter(0.15f)));
            g.fillRoundedRectangle(f, 4.0f);
            if (i == playStep) { g.setColour(ui::butter.withAlpha(0.6f)); g.drawRoundedRectangle(f.reduced(0.5f), 4.0f, 1.5f); }
        }
    }

    // song chain
    label(songLabel, "SONG");
    if (q.chainLen > 1) button(removeBar, "-", false);
    for (int i = 0; i < 16; ++i) {
        const auto f = chainCells[i].toFloat();
        if (i < q.chainLen) {
            const bool now = q.songOn && playBar == i;
            g.setColour(now ? ui::led : ui::soft);
            g.fillRoundedRectangle(f, 5.0f);
            g.setColour(ui::butter);
            g.setFont(ui::font(13.0f, true));
            g.drawText(letter(q.chain[i].b), chainCells[i].withTrimmedBottom(12), juce::Justification::centred);
            g.setColour(ui::butter.withAlpha(0.6f));
            g.setFont(ui::font(9.0f, true));
            g.drawText("drums " + letter(q.chain[i].d), chainCells[i].withTrimmedTop(17), juce::Justification::centred);
        } else if (i == q.chainLen) {
            g.setColour(ui::butter.withAlpha(0.4f));
            g.drawRoundedRectangle(f.reduced(1.0f), 5.0f, 1.0f);
            g.setFont(ui::font(16.0f, true));
            g.drawText("+", chainCells[i], juce::Justification::centred);
        }
    }

    g.setColour(ui::butter.withAlpha(0.55f));
    g.setFont(ui::font(11.0f));
    g.drawText("Click a step, then a key to set its note. Click a song bar to change its bass pattern, right-click for its drums. "
               "MIDI notes from your DAW play the bass live.",
               hintArea, juce::Justification::centredLeft);
}

void SeqView::mouseDown(const juce::MouseEvent& e) {
    const auto pt = e.getPosition();
    const bool alt = e.mods.isPopupMenu() || e.mods.isShiftDown();
    const int b = editBank, db = editDBank, s = sel;
    auto edit = [&](auto fn) { proc.editSeq(fn); repaint(); };
    auto auditionSel = [&] {
        const auto x = proc.getSeq().banks[b][s];
        if (!x.r) proc.audition(x.n, x.o, x.a);
    };

    for (int i = 0; i < 4; ++i) {
        if (bassBank[i].contains(pt)) { editBank = i; edit([i](acid::SeqState& q) { q.loopBank = i; }); return; }
        if (drumBank[i].contains(pt)) { editDBank = i; edit([i](acid::SeqState& q) { q.loopDrum = i; }); return; }
    }
    if (loopBtn.contains(pt)) { edit([](acid::SeqState& q) { q.songOn = false; }); return; }
    if (songBtn.contains(pt)) { edit([](acid::SeqState& q) { q.songOn = true; }); return; }
    if (clearBass.contains(pt)) { edit([b](acid::SeqState& q) { for (auto& x : q.banks[b]) x = acid::Step(); }); return; }
    if (clearDrums.contains(pt)) { edit([db](acid::SeqState& q) { for (auto& x : q.drums[db]) x = 0; }); return; }

    for (int i = 0; i < 16; ++i)
        if (steps[i].contains(pt)) {
            sel = i;
            const auto x = proc.getSeq().banks[b][i];
            if (!x.r) proc.audition(x.n, x.o, x.a);
            repaint();
            return;
        }
    for (int k = 0; k < 13; ++k)
        if (keys[k].contains(pt)) { edit([b, s, k](acid::SeqState& q) { q.banks[b][s].n = k; q.banks[b][s].r = false; }); auditionSel(); return; }
    if (octDown.contains(pt)) { edit([b, s](acid::SeqState& q) { auto& x = q.banks[b][s]; x.o = juce::jmax(-1, x.o - 1); }); auditionSel(); return; }
    if (octUp.contains(pt)) { edit([b, s](acid::SeqState& q) { auto& x = q.banks[b][s]; x.o = juce::jmin(1, x.o + 1); }); auditionSel(); return; }
    if (accentBtn.contains(pt)) { edit([b, s](acid::SeqState& q) { auto& x = q.banks[b][s]; x.a = !x.a; x.r = false; }); return; }
    if (slideBtn.contains(pt)) { edit([b, s](acid::SeqState& q) { auto& x = q.banks[b][s]; x.s = !x.s; x.r = false; }); return; }
    if (restBtn.contains(pt)) { edit([b, s](acid::SeqState& q) { auto& x = q.banks[b][s]; x.r = !x.r; }); return; }

    for (int d = 0; d < 5; ++d) {
        if (drumLabels[d].contains(pt)) { proc.previewDrum(kDrumBits[d]); return; }
        for (int i = 0; i < 16; ++i)
            if (drumCells[d][i].contains(pt)) {
                const int bit = kDrumBits[d];
                edit([db, i, bit](acid::SeqState& q) { q.drums[db][i] ^= bit; });
                return;
            }
    }

    if (removeBar.contains(pt)) { edit([](acid::SeqState& q) { if (q.chainLen > 1) --q.chainLen; }); return; }
    for (int i = 0; i < 16; ++i)
        if (chainCells[i].contains(pt)) {
            edit([i, alt](acid::SeqState& q) {
                if (i < q.chainLen) {
                    auto& c = q.chain[i];
                    if (alt) c.d = (c.d + 1) % 4; else c.b = (c.b + 1) % 4;
                } else if (i == q.chainLen && q.chainLen < 16) {
                    q.chain[q.chainLen] = q.chain[q.chainLen - 1];
                    ++q.chainLen;
                }
            });
            return;
        }
}

void SeqView::sync() {
    const auto q = proc.getSeq();
    const bool run = proc.running.load();
    int b = q.loopBank, d = q.loopDrum;
    if (q.songOn && run) { b = proc.uiBank.load(); d = proc.uiDBank.load(); }   // edit what you hear
    const int st = run ? proc.uiStep.load() : -1, bar = run ? proc.uiBarPos.load() : -1, v = proc.seqVersion.load();
    if (b != editBank || d != editDBank || st != shownStep || bar != shownBar || v != shownVersion || run != shownRun) {
        editBank = b; editDBank = d; shownStep = st; shownBar = bar; shownVersion = v; shownRun = run;
        repaint();
    }
}

// ---------------------------------------------------------------- editor
AcidBoxEditor::AcidBoxEditor(AcidBoxProcessor& p)
    : AudioProcessorEditor(&p), proc(p), bassPage(p), fxPage(p), drumPage(p), seqView(p) {
    setLookAndFeel(&look);

    bassPage.section("Bass").knob("tune", "Tune").knob("cutoff", "Cutoff").knob("reso", "Resonance").knob("envmod", "Env mod")
        .knob("decay", "Decay").knob("accent", "Accent").knob("drive", "Drive");
    bassPage.section("Oscillators").choice("wave", "Osc 1").choice("wave2", "Osc 2").choice("interval", "Interval")
        .knob("morph", "Morph").knob("detune", "Detune").knob("blevel", "Bass vol");

    fxPage.section("Distortion").choice("dtype", "Type").knob("dist", "Amount").knob("dtone", "Tone").knob("dmix", "Mix");
    fxPage.section("Ring mod").choice("rmode", "Mode", 104).knob("ring", "Amount").knob("rpitch", "Pitch");
    fxPage.section("Space").knob("delay", "Echo").knob("reverb", "Reverb").knob("rsize", "Room size");
    fxPage.section("Kick + snare compressor").toggle("cmp", "Comp").knob("cthr", "Threshold").knob("cratio", "Ratio")
        .knob("catk", "Attack").knob("crel", "Release").knob("cgain", "Makeup").knob("cmix", "Mix");

    drumPage.section("Mix").knob("dlevel", "Drum vol").knob("duck", "Duck").knob("ducktime", "Duck time").knob("lvk", "Kick")
        .knob("lvs", "Snare").knob("lvc", "Clap").knob("lvch", "Closed hat").knob("lvoh", "Open hat");
    drumPage.section("Kick").knob("ktune", "Tune").knob("kpunch", "Punch").knob("ksweep", "Sweep").knob("kdecay", "Decay")
        .knob("kclick", "Click").knob("kdrive", "Drive");
    drumPage.section("Snare").knob("stune", "Tune").knob("ssnap", "Snappy").knob("stone", "Tone").knob("sdecay", "Decay");
    drumPage.section("Clap").knob("ctone", "Tone").knob("cspread", "Spread").knob("cdecay", "Decay");
    drumPage.section("Closed hat").knob("chtune", "Tune").knob("chtone", "Tone").knob("chdecay", "Decay").knob("chnoise", "Noise");
    drumPage.section("Open hat").knob("ohtune", "Tune").knob("ohtone", "Tone").knob("ohdecay", "Decay").knob("ohnoise", "Noise");

    const auto tabBg = ui::butter.darker(0.06f);
    tabs.addTab("Bass", tabBg, &bassPage, false);
    tabs.addTab("Effects", tabBg, &fxPage, false);
    tabs.addTab("Drums", tabBg, &drumPage, false);
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);
    addAndMakeVisible(tabs);
    addAndMakeVisible(seqView);

    playBtn.onClick = [this] { if (!proc.hostDriven.load()) proc.internalPlay = !proc.internalPlay.load(); };
    addAndMakeVisible(playBtn);
    midiBtn.onClick = [this] { showMidiMenu(); };
    addAndMakeVisible(midiBtn);

    addAndMakeVisible(syncBtn);
    syncAttach = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "hostsync", syncBtn);

    tempo.setSliderStyle(juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 20);
    addAndMakeVisible(tempo);
    tempoAttach = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "tempo", tempo);
    tempoLabel.setText("Tempo", juce::dontSendNotification);
    tempoLabel.setFont(ui::font(13.0f, true));
    addAndMakeVisible(tempoLabel);
    status.setFont(ui::font(12.0f));
    status.setJustificationType(juce::Justification::centredRight);
    status.setMinimumHorizontalScale(0.6f);
    addAndMakeVisible(status);

    setSize(1040, 720);
    startTimerHz(30);
}

AcidBoxEditor::~AcidBoxEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void AcidBoxEditor::showMidiMenu() {
    auto pname = [this](int i) { auto* prm = proc.param(i); return prm != nullptr ? prm->getName(40) : juce::String(); };
    juce::PopupMenu m;
    const auto mine = proc.learnedList();
    if (!mine.empty()) {
        m.addSectionHeader("Your mappings (click one to forget it)");
        for (const auto& [cc, idx] : mine) m.addItem(1000 + cc, "CC " + juce::String(cc) + "   " + pname(idx));
    }
    m.addSectionHeader("Built in");
    for (const auto& [cc, idx] : proc.builtinList()) m.addItem(2000 + cc, "CC " + juce::String(cc) + "   " + pname(idx), false);
    m.addItem(3, "Notes   Play the bass; velocity 100+ adds accent", false);
    m.addItem(4, "CC 123   All notes off", false);
    m.addSeparator();
    m.addItem(5, "Right-click any knob to MIDI Learn it", false);
    if (proc.armedIndex() >= 0) m.addItem(2, "Cancel MIDI Learn");
    if (!mine.empty()) m.addItem(1, "Forget my mappings");
    juce::Component::SafePointer<AcidBoxEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&midiBtn), [safe](int r) {
        auto* ed = safe.getComponent();
        if (ed == nullptr) return;
        if (r == 1) ed->proc.forgetAll();
        else if (r == 2) ed->proc.learnCancel();
        else if (r >= 1000 && r < 1128) ed->proc.forgetCC(r - 1000);
        ed->repaint();
    });
}

void AcidBoxEditor::paint(juce::Graphics& g) {
    g.fillAll(ui::butter);
    g.setColour(ui::ink);
    g.setFont(ui::font(28.0f, true));
    g.drawText("ACIDBOX", 16, 8, 160, 40, juce::Justification::centredLeft);
}

void AcidBoxEditor::resized() {
    auto r = getLocalBounds();
    auto head = r.removeFromTop(56).reduced(12, 12);
    head.removeFromLeft(168);
    playBtn.setBounds(head.removeFromLeft(88));
    head.removeFromLeft(14);
    syncBtn.setBounds(head.removeFromLeft(128));
    head.removeFromLeft(8);
    tempoLabel.setBounds(head.removeFromLeft(52));
    tempo.setBounds(head.removeFromLeft(200));
    head.removeFromLeft(12);
    midiBtn.setBounds(head.removeFromRight(64));
    head.removeFromRight(10);
    status.setBounds(head);
    tabs.setBounds(r.removeFromTop(268).reduced(6, 0));
    seqView.setBounds(r.reduced(10, 8));
}

void AcidBoxEditor::timerCallback() {
    const bool run = proc.running.load(), host = proc.hostDriven.load();
    playBtn.setButtonText(run ? "Stop" : "Play");
    playBtn.setToggleState(run, juce::dontSendNotification);
    playBtn.setEnabled(!host);
    const double bpm = proc.hostBpm.load();
    const bool sync = proc.apvts.getRawParameterValue("hostsync")->load() > 0.5f;
    tempo.setEnabled(!(sync && bpm > 0));
    const int map = proc.mapVersion.load(), arm = proc.armedIndex(), learnN = proc.learnCount.load();
    if (map != shownMap || arm != shownArm) { shownMap = map; shownArm = arm; repaint(); }
    if (learnN != shownLearn) { shownLearn = learnN; learnMsgUntil = juce::Time::getMillisecondCounter() + 3500; }
    auto pname = [this](int i) { auto* prm = proc.param(i); return prm != nullptr ? prm->getName(40) : juce::String(); };

    juce::String s;
    if (arm >= 0) s = "Move a knob or fader on your MIDI controller to map " + pname(arm);
    else if (juce::Time::getMillisecondCounter() < learnMsgUntil)
        s = "CC " + juce::String(proc.lastLearnCC.load()) + " now controls " + pname(proc.lastLearnIndex.load());
    else if (host) s = "Playing in time with your DAW";
    else if (sync && bpm > 0) s = "Tempo follows your DAW: " + juce::String(bpm, 1) + " BPM";
    else s = "Play MIDI notes to take over the bass line";
    if (status.getText() != s) status.setText(s, juce::dontSendNotification);
    seqView.sync();
}
