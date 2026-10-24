#include "PluginEditor.h"

namespace {
const char* const kNoteNames[13] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C"};
const char* const kDrumNames[5] = {"Kick", "Snare", "Clap", "Closed hat", "Open hat"};
const int kDrumBits[5] = {acid::Kick, acid::Snare, acid::Clap, acid::ClosedHat, acid::OpenHat};
bool isBlack(int k) { const int m = k % 12; return m == 1 || m == 3 || m == 6 || m == 8 || m == 10; }
juce::String letter(int i) { return juce::String::charToString((juce::juce_wchar) ('A' + i)); }

// remembers your last skin, so new AcidBox instances open with it
std::unique_ptr<juce::PropertiesFile> settingsFile() {
    juce::PropertiesFile::Options o;
    o.applicationName = "AcidBox";
    o.filenameSuffix = ".settings";
    o.folderName = "AcidBox";
    o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<juce::PropertiesFile>(o);
}
} // namespace

// ---------------------------------------------------------------- look
AcidLook::AcidLook() { setPalette(ui::resolve(1, false)); }

void AcidLook::setPalette(const ui::Palette& p) {
    pal = p;
    setColour(juce::ResizableWindow::backgroundColourId, p.bg);
    setColour(juce::Label::textColourId, p.ink);
    setColour(juce::Slider::textBoxTextColourId, p.ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, p.led.withAlpha(0.4f));
    setColour(juce::Slider::thumbColourId, p.led);
    setColour(juce::Slider::trackColourId, p.ink);
    setColour(juce::Slider::backgroundColourId, p.ink.withAlpha(0.2f));
    setColour(juce::ComboBox::backgroundColourId, p.bar);
    setColour(juce::ComboBox::textColourId, p.bg);
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::arrowColourId, p.bg);
    setColour(juce::PopupMenu::backgroundColourId, p.panel);
    setColour(juce::PopupMenu::textColourId, p.ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, p.led);
    setColour(juce::PopupMenu::highlightedTextColourId, p.onLed);
    setColour(juce::TextButton::buttonColourId, p.bar);
    setColour(juce::TextButton::buttonOnColourId, p.led);
    setColour(juce::TextButton::textColourOffId, p.bg);
    setColour(juce::TextButton::textColourOnId, p.onLed);
    setColour(juce::ToggleButton::textColourId, p.ink);
    setColour(juce::ToggleButton::tickColourId, p.ink);
    setColour(juce::ToggleButton::tickDisabledColourId, p.ink.withAlpha(0.4f));
    setColour(juce::TabbedButtonBar::tabTextColourId, p.ink.withAlpha(0.7f));
    setColour(juce::TabbedButtonBar::frontTextColourId, p.ink);
    setColour(juce::TabbedButtonBar::tabOutlineColourId, p.ink.withAlpha(0.3f));
    setColour(juce::TabbedButtonBar::frontOutlineColourId, p.ink);
    setColour(juce::TabbedComponent::backgroundColourId, p.bg);
    setColour(juce::TabbedComponent::outlineColourId, p.ink.withAlpha(0.3f));
    setColour(juce::PopupMenu::headerTextColourId, p.soft);
    setColour(juce::Label::textWhenEditingColourId, p.ink);
    setColour(juce::TextEditor::textColourId, p.ink);
    setColour(juce::TextEditor::backgroundColourId, p.cell);
    setColour(juce::TextEditor::highlightColourId, p.led.withAlpha(0.4f));
    setColour(juce::CaretComponent::caretColourId, p.ink);
}

void AcidLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider&) {
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(3.0f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startA + pos * (endA - startA);
    const juce::PathStrokeType stroke(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc(c.x, c.y, r, r, 0.0f, startA, endA, true);
    g.setColour(pal.track);
    g.strokePath(track, stroke);
    juce::Path value;
    value.addCentredArc(c.x, c.y, r, r, 0.0f, startA, ang, true);
    g.setColour(pal.led);
    g.strokePath(value, stroke);

    const float br = juce::jmax(2.0f, r - 6.0f);
    g.setColour(pal.knobBody);
    g.fillEllipse(c.x - br, c.y - br, br * 2, br * 2);
    g.setColour(pal.knobMark);
    g.drawLine(c.x + std::sin(ang) * br * 0.3f, c.y - std::cos(ang) * br * 0.3f,
               c.x + std::sin(ang) * (br - 3), c.y - std::cos(ang) * (br - 3), 2.5f);
}

// the window's resize grip: three diagonal ridges in the skin's ink, brighter under the mouse
// (JUCE's default grip is drawn in near-black, which disappears on the dark skins)
void AcidLook::drawCornerResizer(juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) {
    const float size = (float) juce::jmin(w, h);
    g.setColour((isMouseOver || isMouseDragging) ? pal.led : pal.ink.withAlpha(0.7f));
    for (float k = 0.25f; k < 1.0f; k += 0.25f)
        g.drawLine((float) w - size * k, (float) h - 2.0f, (float) w - 2.0f, (float) h - size * k, 2.0f);
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
    const auto& P = palOf(*this);
    if (index >= 0 && proc.armedIndex() == index) {
        g.setColour(P.led);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 2.0f);
    }
    int cc = proc.learnedCC(index);
    const bool mine = cc >= 0;
    if (!mine) cc = proc.builtinCC(index);
    if (cc < 0) return;
    g.setColour(mine ? P.led : P.soft);
    g.setFont(ui::font(9.0f, true));
    g.drawText("CC " + juce::String(cc), getWidth() - 34, 15, 32, 11, juce::Justification::centredRight);
}

void Knob::mouseDown(const juce::MouseEvent& e) {
    if (!e.mods.isPopupMenu() || index < 0) return;
    const int lc = proc.learnedCC(index), bc = proc.builtinCC(index);
    const bool armed = proc.armedIndex() == index;
    juce::PopupMenu m;
    m.setLookAndFeel(&getLookAndFeel());   // menus follow the skin too
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
    const auto& P = palOf(*this);
    g.setColour(P.ink.withAlpha(0.45f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 8.0f, 1.5f);
    g.setColour(P.ink);
    g.setFont(ui::font(11.0f, true));
    g.drawText(title.toUpperCase(), getLocalBounds().reduced(10, 5).removeFromTop(14), juce::Justification::centredLeft);
}

void Section::resized() {
    auto r = getLocalBounds().reduced(6);
    r.removeFromTop(18);
    for (int i = 0; i < items.size(); ++i) items[i]->setBounds(r.removeFromLeft(widths[(size_t) i]));
}

int Page::requiredHeight(int width) const {
    const int avail = width - 8;   // the layout insets the page by 4 px on each side
    int rows = 0, x = 0;
    for (auto* s : sections) {
        const int w = s->preferredWidth() + 8;   // plus a 4 px margin each side
        if (rows == 0 || x + w > avail) { ++rows; x = 0; }
        x += w;
    }
    return rows * (Section::preferredHeight + 8) + 8;
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
int SeqView::blockHeight(SeqBlockId id) {
    switch (id) {
        case SeqBlockId::Patterns: return 26;
        case SeqBlockId::BassLine: return 48 + 8 + 44;    // steps, gap, step editor
        case SeqBlockId::Drums: return 5 * 22 - 2;         // five rows of 20 with 2 px between
        case SeqBlockId::Song: return 30;
        case SeqBlockId::Tips: return 18;
    }
    return 0;
}

int SeqView::requiredHeight() {
    int h = 20, n = 0;   // 10 px padding top and bottom
    for (const auto& b : kSeqLayout) {
        if (!b.visible) continue;
        h += blockHeight(b.id) + (b.outline ? 12 : 0) + (n++ > 0 ? 10 : 0);
    }
    return h;
}

void SeqView::resized() {
    const auto r = getLocalBounds().reduced(12, 10);
    const int labelW = 92;
    const int gx = r.getX() + labelW, gw = r.getWidth() - labelW;
    auto col = [&](int i, int y, int h) {
        const int x0 = gx + gw * i / 16, x1 = gx + gw * (i + 1) / 16;
        return juce::Rectangle<int>(x0 + 2, y, x1 - x0 - 4, h);
    };

    auto placePatterns = [&](int y) {
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
    };
    auto placeBass = [&](int y) {
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
    };
    auto placeDrums = [&](int y) {
        for (int d = 0; d < 5; ++d) {
            drumLabels[d] = {r.getX(), y + d * 22, labelW - 8, 20};
            for (int i = 0; i < 16; ++i) drumCells[d][i] = col(i, y + d * 22, 20);
        }
    };
    auto placeSong = [&](int y) {
        songLabel = {r.getX(), y, labelW - 40, 30};
        removeBar = {r.getX() + labelW - 38, y + 3, 30, 24};
        for (int i = 0; i < 16; ++i) chainCells[i] = col(i, y, 30);
    };

    hintArea = {};
    for (auto& o : outlines) o = {};
    int y = r.getY(), n = 0;
    for (const auto& b : kSeqLayout) {
        if (!b.visible) continue;
        if (b.outline) y += 6;
        switch (b.id) {
            case SeqBlockId::Patterns: placePatterns(y); break;
            case SeqBlockId::BassLine: placeBass(y); break;
            case SeqBlockId::Drums: placeDrums(y); break;
            case SeqBlockId::Song: placeSong(y); break;
            case SeqBlockId::Tips: hintArea = {r.getX(), y, r.getWidth(), 18}; break;
        }
        const int h = blockHeight(b.id);
        if (b.outline && n < 5) outlines[n++] = juce::Rectangle<int>(r.getX(), y, r.getWidth(), h).expanded(6);
        y += h + (b.outline ? 6 : 0) + 10;
    }
}

void SeqView::paint(juce::Graphics& g) {
    const auto& P = palOf(*this);
    const auto stripe = P.cell.interpolatedWith(P.ink, 0.08f);   // every other group of four steps
    auto readable = [&](juce::Colour bg) {   // whichever of ink and background stands out more on bg
        const float b = bg.getPerceivedBrightness();
        return std::abs(P.ink.getPerceivedBrightness() - b) >= std::abs(P.bg.getPerceivedBrightness() - b) ? P.ink : P.bg;
    };
    const auto q = proc.getSeq();
    const bool running = proc.running.load();
    const int playStep = running ? proc.uiStep.load() : -1;
    const int playBar = running ? proc.uiBarPos.load() : -1;
    const int playBank = proc.uiBank.load(), playDBank = proc.uiDBank.load();
    const auto& cur = q.banks[editBank][sel];

    g.setColour(P.panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
    g.setColour(P.ink.withAlpha(0.45f));   // same outline as the knob sections
    for (const auto& o : outlines)
        if (!o.isEmpty()) g.drawRoundedRectangle(o.toFloat(), 8.0f, 1.5f);

    auto label = [&](juce::Rectangle<int> r, const juce::String& t) {
        g.setColour(P.soft);
        g.setFont(ui::font(11.0f, true));
        g.drawText(t, r, juce::Justification::centredLeft);
    };
    auto button = [&](juce::Rectangle<int> r, const juce::String& t, bool on, bool lit = false) {
        const auto f = r.toFloat();
        g.setColour(on ? P.bar : P.cell);
        g.fillRoundedRectangle(f, 5.0f);
        g.setColour(on ? P.bg : P.ink);
        g.setFont(ui::font(13.0f, true));
        g.drawText(t, r, juce::Justification::centred);
        if (lit) { g.setColour(P.led); g.fillEllipse(f.getRight() - 8.0f, f.getY() + 3.0f, 5.0f, 5.0f); }
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
        g.setColour((i / 4) % 2 ? P.cell : stripe);
        g.fillRoundedRectangle(f, 6.0f);
        if (i == playStep) { g.setColour(P.led.withAlpha(0.35f)); g.fillRoundedRectangle(f, 6.0f); }
        if (i == sel) { g.setColour(P.ink); g.drawRoundedRectangle(f.reduced(1.0f), 6.0f, 2.0f); }
        g.setColour(s.r ? P.soft.withAlpha(0.6f) : P.ink);
        g.setFont(ui::font(15.0f, true));
        g.drawText(s.r ? juce::String("-") : juce::String(kNoteNames[s.n]), steps[i].withTrimmedBottom(16), juce::Justification::centred);
        if (!s.r) {
            auto badges = steps[i].withTrimmedTop(30).reduced(4, 0);
            g.setFont(ui::font(10.0f, true));
            if (s.o != 0) { g.setColour(P.soft); g.drawText(s.o > 0 ? "hi" : "lo", badges, juce::Justification::centredLeft); }
            if (s.a) { g.setColour(P.led); g.drawText("A", badges, juce::Justification::centred); }
            if (s.s) { g.setColour(P.ink); g.drawText("S", badges, juce::Justification::centredRight); }
        }
    }

    // step editor
    label(editLabel, "STEP " + juce::String(sel + 1));
    for (int k = 0; k < 13; ++k) {
        const bool on = !cur.r && cur.n == k;
        const auto f = keys[k].toFloat();
        g.setColour(on ? P.keySel : isBlack(k) ? P.keyBlack : P.keyWhite);
        g.fillRoundedRectangle(f, 4.0f);
        g.setColour(on ? P.onLed : readable(isBlack(k) ? P.keyBlack : P.keyWhite));
        g.setFont(ui::font(11.0f, true));
        g.drawText(kNoteNames[k], keys[k].withTrimmedTop(20), juce::Justification::centred);
    }
    button(octDown, "-", false);
    g.setColour(P.ink);
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
            g.setColour(on ? P.led : ((i / 4) % 2 ? P.cell : stripe));
            g.fillRoundedRectangle(f, 4.0f);
            if (i == playStep) { g.setColour(P.ink.withAlpha(0.6f)); g.drawRoundedRectangle(f.reduced(0.5f), 4.0f, 1.5f); }
        }
    }

    // song chain
    label(songLabel, "SONG");
    if (q.chainLen > 1) button(removeBar, "-", false);
    for (int i = 0; i < 16; ++i) {
        const auto f = chainCells[i].toFloat();
        if (i < q.chainLen) {
            const bool now = q.songOn && playBar == i;
            g.setColour(now ? P.led : P.cell);
            g.fillRoundedRectangle(f, 5.0f);
            g.setColour(now ? P.onLed : P.ink);
            g.setFont(ui::font(13.0f, true));
            g.drawText(letter(q.chain[i].b), chainCells[i].withTrimmedBottom(12), juce::Justification::centred);
            g.setColour(now ? P.onLed : P.soft);
            g.setFont(ui::font(9.0f, true));
            g.drawText("drums " + letter(q.chain[i].d), chainCells[i].withTrimmedTop(17), juce::Justification::centred);
        } else if (i == q.chainLen) {
            g.setColour(P.soft);
            g.drawRoundedRectangle(f.reduced(1.0f), 5.0f, 1.0f);
            g.setFont(ui::font(16.0f, true));
            g.drawText("+", chainCells[i], juce::Justification::centred);
        }
    }

    g.setColour(P.soft);
    g.setFont(ui::font(11.0f));
    g.drawText("Click a step, then a key to set its note. Click a song bar to change its bass pattern, right-click for its drums. "
               "MIDI notes play the bass live; channel 10 plays the drums.",
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
    : AudioProcessorEditor(&p), proc(p), bassPage(p), drumPage(p), seqView(p) {
    setLookAndFeel(&look);
    addAndMakeVisible(body);
    body.onPaint = [this](juce::Graphics& g) {
        const auto& P = look.pal;
        g.fillAll(P.bg);
        g.setColour(P.ink);
        g.setFont(ui::font(28.0f, true));
        g.drawText("ACIDBOX", 16, 8, 160, 40, juce::Justification::centredLeft);
    };
    body.onResize = [this] { layoutBody(); };

    bassPage.section("Bass").knob("blevel", "Vol").knob("tune", "Tune").knob("cutoff", "Cutoff").knob("reso", "Resonance")
        .knob("envmod", "Env mod").knob("decay", "Decay").knob("accent", "Accent").knob("drive", "Drive");
    bassPage.section("Oscillators").choice("wave", "Osc 1").choice("wave2", "Osc 2").choice("interval", "Interval")
        .knob("morph", "Morph").knob("detune", "Detune");

    // second row of the Bass tab
    bassPage.section("Distortion").choice("dtype", "Type").knob("dist", "Amount").knob("dtone", "Tone").knob("dmix", "Mix");
    bassPage.section("Ring mod").choice("rmode", "Mode", 104).knob("ring", "Amount").knob("rpitch", "Pitch");
    bassPage.section("Space").toggle("spaceon", "Space").knob("delay", "Echo").knob("rsize", "Room size").knob("reverb", "Reverb");
    // third row of the Bass tab
    bassPage.section("Filter").choice("ftype", "Type").knob("fslope", "Slope").knob("fslopeenv", "Slope env");


    drumPage.section("Mix").knob("dlevel", "Drum vol").knob("duck", "Duck").knob("ducktime", "Duck time").knob("lvk", "Kick")
        .knob("lvs", "Snare").knob("lvc", "Clap").knob("lvch", "Closed hat").knob("lvoh", "Open hat");
    drumPage.section("Kick").knob("ktune", "Tune").knob("kpunch", "Punch").knob("ksweep", "Sweep").knob("kdecay", "Decay")
        .knob("kclick", "Click").knob("kdrive", "Drive");
    drumPage.section("Snare").knob("stune", "Tune").knob("ssnap", "Snappy").knob("stone", "Tone").knob("sdecay", "Decay");
    drumPage.section("Clap").knob("ctone", "Tone").knob("cspread", "Spread").knob("cdecay", "Decay");
    drumPage.section("Closed hat").knob("chtune", "Tune").knob("chtone", "Tone").knob("chdecay", "Decay").knob("chnoise", "Noise");
    drumPage.section("Open hat").knob("ohtune", "Tune").knob("ohtone", "Tone").knob("ohdecay", "Decay").knob("ohnoise", "Noise");
    // third row of the Drums tab
    drumPage.section("Kick + snare compressor").toggle("cmp", "Comp").knob("cthr", "Threshold").knob("cratio", "Ratio")
        .knob("catk", "Attack").knob("crel", "Release").knob("cgain", "Makeup").knob("cmix", "Mix");

    const auto tabBg = look.pal.panel;
    tabs.addTab("Bass", tabBg, &bassPage, false);
    tabs.addTab("Drums", tabBg, &drumPage, false);
    tabs.addTab("Skins", tabBg, &skinPage, false);
    skinPage.onPick = [this](int i) { applySkin(i); };
    tabs.setTabBarDepth(28);
    tabs.setOutline(0);
    body.addAndMakeVisible(tabs);
    body.addAndMakeVisible(seqView);

    playBtn.onClick = [this] { if (!proc.hostDriven.load()) proc.internalPlay = !proc.internalPlay.load(); };
    body.addAndMakeVisible(playBtn);
    midiBtn.onClick = [this] { showMidiMenu(); };
    body.addAndMakeVisible(midiBtn);

    body.addAndMakeVisible(syncBtn);
    syncAttach = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "hostsync", syncBtn);

    tempo.setSliderStyle(juce::Slider::LinearHorizontal);
    tempo.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 20);
    body.addAndMakeVisible(tempo);
    tempoAttach = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "tempo", tempo);
    tempoLabel.setText("Tempo", juce::dontSendNotification);
    tempoLabel.setFont(ui::font(13.0f, true));
    body.addAndMakeVisible(tempoLabel);
    status.setFont(ui::font(12.0f));
    status.setJustificationType(juce::Justification::centredRight);
    status.setMinimumHorizontalScale(0.6f);
    body.addAndMakeVisible(status);

    int skin = proc.skinIndex.load();
    if (skin < 0) skin = settingsFile()->getIntValue("skin", 0);
    applySkin(skin);

    // the window grows to fit the tallest knob tab: two rows of sections is the standard 720 px window
    const int pagesH = juce::jmax(240, bassPage.requiredHeight(baseW - 12), drumPage.requiredHeight(baseW - 12));
    tabsH = 28 + pagesH;
    baseH = 56 + tabsH + juce::jmax(380, SeqView::requiredHeight()) + 16;   // the sequencer area grows if its blocks need more room
    body.setSize(baseW, baseH);

    // drag the corner to resize; everything scales together, keeping the shape
    float scale = proc.uiScale.load();
    if (scale <= 0.0f) scale = (float) settingsFile()->getDoubleValue("scale", 1.0);
    scale = juce::jlimit(0.6f, 2.0f, scale);
    setResizable(true, true);
    setResizeLimits(juce::roundToInt(baseW * 0.6), juce::roundToInt(baseH * 0.6), baseW * 2, baseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio((double) baseW / (double) baseH);
    setSize(juce::roundToInt(baseW * scale), juce::roundToInt(baseH * scale));
    startTimerHz(30);
}

void AcidBoxEditor::applySkin(int index) {
    index = juce::jlimit(0, (int) ui::skins().size() - 1, index);
    const bool changed = proc.skinIndex.load() != index;
    proc.skinIndex = index;
    skinPage.selected = index;
    appliedDark = juce::Desktop::getInstance().isDarkModeActive();
    look.setPalette(ui::resolve(index, appliedDark));
    for (int t = 0; t < tabs.getNumTabs(); ++t) tabs.setTabBackgroundColour(t, look.pal.panel);
    sendLookAndFeelChange();
    repaint();
    if (changed) {
        auto f = settingsFile();
        f->setValue("skin", index);
        f->saveIfNeeded();
    }
}

// ---------------------------------------------------------------- skins tab
void SkinPage::resized() {
    swatches.clear();
    const auto r = getLocalBounds().reduced(14, 10);
    const int w = 100, h = 92, gap = 10;
    int x = r.getX(), y = r.getY() + 30;
    for (size_t i = 0; i < ui::skins().size(); ++i) {
        if (x + w > r.getRight()) { x = r.getX(); y += h + gap; }
        swatches.push_back({x, y, w, h});
        x += w + gap;
    }
}

void SkinPage::paint(juce::Graphics& g) {
    const auto& P = palOf(*this);
    g.setColour(P.ink);
    g.setFont(ui::font(13.0f, true));
    g.drawText("Pick a skin. Auto follows your computer's dark mode, and new AcidBox instances open with your last choice.",
               getLocalBounds().reduced(14, 10).removeFromTop(22), juce::Justification::centredLeft);

    auto preview = [&](const ui::Palette& q, juce::Rectangle<float> r) {
        g.setColour(q.bg);
        g.fillRoundedRectangle(r, 10.0f);
        const auto strip = r.withTrimmedTop(r.getHeight() * 0.55f).reduced(6.0f, 6.0f);
        g.setColour(q.panel);
        g.fillRoundedRectangle(strip, 5.0f);
        const float cw = (strip.getWidth() - 10.0f) / 4.0f;
        for (int k = 0; k < 4; ++k) {
            g.setColour(k == 1 ? q.led : q.cell);
            g.fillRoundedRectangle(strip.getX() + 3.0f + k * (cw + 1.3f), strip.getY() + 4.0f, cw - 1.0f, strip.getHeight() - 8.0f, 3.0f);
        }
        const float kr = 10.0f, kx = r.getX() + 18.0f, ky = r.getY() + 16.0f;
        g.setColour(q.knobBody);
        g.fillEllipse(kx - kr, ky - kr, kr * 2, kr * 2);
        g.setColour(q.knobMark);
        g.drawLine(kx, ky, kx + 5.0f, ky - 6.0f, 2.0f);
        g.setColour(q.ink);
        g.fillRoundedRectangle(r.getX() + 36.0f, r.getY() + 11.0f, 40.0f, 4.0f, 2.0f);
        g.setColour(q.soft);
        g.fillRoundedRectangle(r.getX() + 36.0f, r.getY() + 19.0f, 28.0f, 3.0f, 1.5f);
    };

    for (size_t i = 0; i < swatches.size(); ++i) {
        const auto box = swatches[i];
        const auto pr = box.withHeight(66).toFloat();
        if (i == 0) {
            {
                juce::Graphics::ScopedSaveState ss(g);
                g.reduceClipRegion(box.withWidth(box.getWidth() / 2));
                preview(ui::resolve(0, false), pr);
            }
            juce::Graphics::ScopedSaveState ss(g);
            g.reduceClipRegion(box.withTrimmedLeft(box.getWidth() / 2));
            preview(ui::resolve(0, true), pr);
        } else {
            preview(ui::skins()[i].pal, pr);
        }
        g.setColour(P.ink.withAlpha(0.25f));
        g.drawRoundedRectangle(pr.reduced(0.5f), 10.0f, 1.0f);
        if ((int) i == selected) {
            g.setColour(P.led);
            g.drawRoundedRectangle(pr.expanded(2.0f), 12.0f, 3.0f);
        }
        g.setColour(P.ink);
        g.setFont(ui::font(12.0f, (int) i == selected));
        g.drawText(ui::skins()[i].name, box.withTrimmedTop(70), juce::Justification::centred);
    }
}

void SkinPage::mouseDown(const juce::MouseEvent& e) {
    for (size_t i = 0; i < swatches.size(); ++i)
        if (swatches[i].contains(e.getPosition())) {
            selected = (int) i;
            if (onPick) onPick((int) i);
            repaint();
            return;
        }
}

AcidBoxEditor::~AcidBoxEditor() {
    stopTimer();
    if (const float s = proc.uiScale.load(); s > 0.0f) {   // new instances open at the size you left this one
        auto f = settingsFile();
        f->setValue("scale", (double) s);
        f->saveIfNeeded();
    }
    setLookAndFeel(nullptr);
}

void AcidBoxEditor::showMidiMenu() {
    auto pname = [this](int i) { auto* prm = proc.param(i); return prm != nullptr ? prm->getName(40) : juce::String(); };
    juce::PopupMenu m;
    m.setLookAndFeel(&getLookAndFeel());
    const auto mine = proc.learnedList();
    if (!mine.empty()) {
        m.addSectionHeader("Your mappings (click one to forget it)");
        for (const auto& [cc, idx] : mine) m.addItem(1000 + cc, "CC " + juce::String(cc) + "   " + pname(idx));
    }
    m.addSectionHeader("Built in");
    for (const auto& [cc, idx] : proc.builtinList()) m.addItem(2000 + cc, "CC " + juce::String(cc) + "   " + pname(idx), false);
    m.addItem(3, "Notes   Play the bass; velocity 100+ adds accent", false);
    m.addItem(6, "Channel 10   Drums: C1 kick, D1 snare, D#1 clap, F#1 closed hat, A#1 open hat", false);
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

void AcidBoxEditor::paint(juce::Graphics& g) { g.fillAll(look.pal.bg); }

void AcidBoxEditor::resized() {
    const float s = (float) getWidth() / (float) baseW;
    body.setBounds(0, 0, baseW, baseH);
    body.setTransform(juce::AffineTransform::scale(s));
    proc.uiScale = s;
}

void AcidBoxEditor::layoutBody() {
    auto r = body.getLocalBounds();
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
    tabs.setBounds(r.removeFromTop(tabsH).reduced(6, 0));
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

    const int skin = proc.skinIndex.load();
    if (skin >= 0 && skin != skinPage.selected) applySkin(skin);
    else if (skin <= 0 && juce::Time::getMillisecondCounter() >= nextDarkCheck) {
        nextDarkCheck = juce::Time::getMillisecondCounter() + 1000;
        if (juce::Desktop::getInstance().isDarkModeActive() != appliedDark) applySkin(0);
    }
}
