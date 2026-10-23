// AcidCore: line-by-line C++ port of the AcidBox web engine (bass voice, drums, echo, reverb, ducking).
// No allocation happens after construction, so it is safe to run on a realtime audio thread.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace acid {

struct Step { int n = 0, o = 0; bool a = false, s = false, r = true; };
struct Bar  { int b = 0, d = 0; };

enum DrumBits { Kick = 1, Snare = 2, Clap = 4, ClosedHat = 8, OpenHat = 16 };

struct Params {
    double tune = 0.5, cutoff = 0.35, reso = 0.7, envmod = 0.6, decay = 0.35, accent = 0.6, drive = 0.3, delay = 0.25, tempo = 125;
    int wave = 0, wave2 = 0;
    double interval = 0, morph = 0, detune = 0.5, dlevel = 0.7, kdecay = 0.5, blevel = 0.8, duck = 0.45, ducktime = 0.4,
           reverb = 0.3, rsize = 0.5;
    int dtype = 0;
    double dist = 0, dtone = 0.6, dmix = 1, lvk = 0.8, lvs = 0.8, lvc = 0.8, lvch = 0.8, lvoh = 0.8;
    int rmode = 0;
    double ring = 0, rpitch = 0.5;
    double ktune = 0.5, kpunch = 0.4, ksweep = 0.5, kclick = 0.5, kdrive = 0.5, stune = 0.5, ssnap = 0.5, stone = 0.5, sdecay = 0.5,
           ctone = 0.5, cspread = 0.5, cdecay = 0.5, chtune = 0.5, chtone = 0.5, chdecay = 0.5, chnoise = 0.5,
           ohtune = 0.5, ohtone = 0.5, ohdecay = 0.5, ohnoise = 0.5;
    int cmp = 0;   // kick + snare compressor on/off
    double cthr = 2.0 / 3, cratio = 0.4, catk = 0.741, crel = 0.431, cgain = 1.0 / 3, cmix = 1;
};

// every continuous 0..1 parameter, by the same id the web version uses
struct FloatField { const char* id; double Params::* m; };
inline const FloatField kFloatFields[] = {
    {"tune", &Params::tune}, {"cutoff", &Params::cutoff}, {"reso", &Params::reso}, {"envmod", &Params::envmod},
    {"decay", &Params::decay}, {"accent", &Params::accent}, {"drive", &Params::drive}, {"delay", &Params::delay},
    {"reverb", &Params::reverb}, {"rsize", &Params::rsize}, {"morph", &Params::morph}, {"detune", &Params::detune},
    {"blevel", &Params::blevel}, {"dist", &Params::dist}, {"dtone", &Params::dtone}, {"dmix", &Params::dmix},
    {"ring", &Params::ring}, {"rpitch", &Params::rpitch}, {"dlevel", &Params::dlevel}, {"kdecay", &Params::kdecay},
    {"duck", &Params::duck}, {"ducktime", &Params::ducktime}, {"lvk", &Params::lvk}, {"lvs", &Params::lvs},
    {"lvc", &Params::lvc}, {"lvch", &Params::lvch}, {"lvoh", &Params::lvoh}, {"ktune", &Params::ktune},
    {"kpunch", &Params::kpunch}, {"ksweep", &Params::ksweep}, {"kclick", &Params::kclick}, {"kdrive", &Params::kdrive},
    {"stune", &Params::stune}, {"ssnap", &Params::ssnap}, {"stone", &Params::stone}, {"sdecay", &Params::sdecay},
    {"ctone", &Params::ctone}, {"cspread", &Params::cspread}, {"cdecay", &Params::cdecay}, {"chtune", &Params::chtune},
    {"chtone", &Params::chtone}, {"chdecay", &Params::chdecay}, {"chnoise", &Params::chnoise}, {"ohtune", &Params::ohtune},
    {"ohtone", &Params::ohtone}, {"ohdecay", &Params::ohdecay}, {"ohnoise", &Params::ohnoise},
    {"cthr", &Params::cthr}, {"cratio", &Params::cratio}, {"catk", &Params::catk}, {"crel", &Params::crel},
    {"cgain", &Params::cgain}, {"cmix", &Params::cmix},
};

// the sequencer data the editor changes and the engine plays
struct SeqState {
    Step banks[4][16];
    int drums[4][16] = {};
    Bar chain[16];
    int chainLen = 4;
    bool songOn = false;
    int loopBank = 0, loopDrum = 0;
};

inline Step st(int n, int o = 0, bool a = false, bool s = false) { Step x; x.n = n; x.o = o; x.a = a; x.s = s; x.r = false; return x; }
inline Step rest() { return Step(); }

inline SeqState defaultSeq() {
    SeqState q;
    const Step A[16] = { st(0,0,true), st(0), st(0,1), st(0,0,false,true), st(3), rest(), st(0,0,true), st(10),
                         st(0), st(7,0,false,true), st(0,1,true), rest(), st(0), st(3,0,true), st(5,0,false,true), st(7) };
    const Step B[16] = { st(0,0,true), st(0,1,false,true), st(10), st(0,0,true), rest(), st(3), st(5,0,false,true), st(7),
                         st(0,-1,true), st(0), rest(), st(0,1,true), st(10,0,false,true), st(7), st(3), st(5,0,true) };
    for (int i = 0; i < 16; ++i) {
        q.banks[0][i] = A[i]; q.banks[1][i] = B[i];
        q.drums[0][i] = (i % 4 == 0 ? Kick : 0) | (i == 4 || i == 12 ? Clap : 0) | (i % 4 == 2 ? OpenHat : ClosedHat);
        q.drums[1][i] = (i % 4 == 0 || i == 14 ? Kick : 0) | (i == 4 || i == 12 ? Snare : 0) | (i % 4 == 2 ? ClosedHat : 0) | (i == 15 ? OpenHat : 0);
    }
    q.chain[0] = {0, 0}; q.chain[1] = {0, 0}; q.chain[2] = {1, 1}; q.chain[3] = {1, 1};
    q.chainLen = 4;
    return q;
}

class Core {
public:
    static constexpr double PI = 3.14159265358979323846;
    static double jsRound(double x) { return std::floor(x + 0.5); }   // JavaScript Math.round

    struct Svf { double a1 = 0, a2 = 0, a3 = 0, k = 0, ic1 = 0, ic2 = 0, bp = 0, lp = 0, fc = -1; };
    struct Line { std::vector<float> b; int i = 0; double s = 0; };

    Params p;
    Step banks[4][16];
    int dbanks[4][16] = {};
    Bar chain[16];
    int chainLen = 1;
    bool songOn = false;
    int ab = 0, adb = 0, loopBank = 0, loopDrum = 0, pos = -1;
    bool playing = false;
    bool externalClock = false;          // true while the host's transport drives the steps
    int lastStep = -1, lastBarPos = -1;  // read by the editor for the play cursor

    explicit Core(double sampleRate) : sr(sampleRate) {
        const double sc = sr / 44100.0;
        auto mk = [&](int len) { Line l; l.b.assign((size_t) std::max(1.0, jsRound(len * sc)), 0.0f); return l; };
        const int cl[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617}, al[4] = {556, 441, 341, 225};
        for (int j = 0; j < 8; ++j) { cL[j] = mk(cl[j]); cR[j] = mk(cl[j] + 23); }
        for (int j = 0; j < 4; ++j) { aL[j] = mk(al[j]); aR[j] = mk(al[j] + 23); }
        dlen = (int) std::ceil(sr * 2.5);
        dbuf.assign((size_t) dlen, 0.0f);
        cutS = p.cutoff; resS = p.reso;
        fS = mkSvf(sr, 1800, 0.7); fC = mkSvf(sr, 1150, 2.2); fH = mkSvf(sr, 7500, 0.9); fH2 = mkSvf(sr, 7500, 0.9);
        chain[0] = {0, 0};
    }

    void setRandomSeed(uint32_t s) { rng = s; }

    void setSeq(const SeqState& q) {
        for (int b = 0; b < 4; ++b)
            for (int i = 0; i < 16; ++i) { banks[b][i] = q.banks[b][i]; dbanks[b][i] = q.drums[b][i]; }
        chainLen = std::clamp(q.chainLen, 1, 16);
        for (int i = 0; i < chainLen; ++i) chain[i] = q.chain[i];
        if (q.songOn && !songOn) pos = -1;
        songOn = q.songOn;
        loopBank = q.loopBank & 3; loopDrum = q.loopDrum & 3;
        if (!songOn) { ab = loopBank; adb = loopDrum; }
    }

    // Live MIDI notes: monophonic, last-note priority; overlapping notes slide like a 303
    double liveHz(int note) const { return 440 * std::pow(2.0, (note + jsRound((p.tune - 0.5) * 24) - 69) / 12.0); }
    void liveOn(int note, int vel) {
        const bool legato = liveN > 0;
        removeLive(note);
        if (liveN < 128) live[liveN++] = note;
        target = liveHz(note);
        accLevel = vel >= 100 ? 1 : 0;
        if (!legato) { freq = target; fenv = 1; accEnv = 1; }
        else if (vel >= 100) accEnv = 1;
        prevSlide = false;
    }
    void liveOff(int note) {
        removeLive(note);
        if (liveN) target = liveHz(live[liveN - 1]);
    }
    void allOff() { liveN = 0; }

    double hz(int n, int o) const {
        const double midi = 36 + n + 12 * o + jsRound((p.tune - 0.5) * 24);
        return 440 * std::pow(2.0, (midi - 69) / 12.0);
    }
    void setPlaying(bool on) {
        if (on && !playing) { step = 0; toNext = 0; prevSlide = false; pos = -1; }
        if (!on) { gateLeft = 0; prevSlide = false; lastStep = -1; lastBarPos = -1; }
        playing = on;
    }
    void audition(int n, int o, bool a) {
        if (playing) return;
        freq = target = hz(n, o);
        fenv = 1; accEnv = 1; accLevel = a ? 1 : 0;
        gateLeft = sr * 0.22; prevSlide = false;
    }

    // internal clock: called every sixteenth
    void trigger(double sps) {
        if (step == 0) {
            if (songOn) { pos = (pos + 1) % chainLen; ab = chain[pos].b; adb = chain[pos].d; }
            else { ab = loopBank; adb = loopDrum; }
            lastBarPos = songOn ? pos : -1;
        }
        doStep(sps);
    }
    // host clock: abs16 is the sixteenth count since the start of the song, so loops and jumps land correctly
    void triggerExternal(int64_t abs16, double sps) {
        const int64_t bar = abs16 >= 0 ? abs16 / 16 : (abs16 - 15) / 16;
        step = (int) (abs16 - bar * 16);
        if (songOn) { pos = (int) (((bar % chainLen) + chainLen) % chainLen); ab = chain[pos].b; adb = chain[pos].d; }
        else { ab = loopBank; adb = loopDrum; }
        lastBarPos = songOn ? pos : -1;
        doStep(sps);
    }

    void process(float* L, float* R, int n) {
        const double sps = sr * 60 / p.tempo / 4;
        const double decayT = 0.06 + p.decay * p.decay * 2.0;
        const double envC = std::exp(-1 / (decayT * sr));
        const double accC = std::exp(-1 / (0.18 * sr));
        const double attC = std::exp(-1 / (0.003 * sr));
        const double relC = std::exp(-1 / (0.008 * sr));
        const double glideC = std::exp(-1 / (0.045 * sr));
        const int dsamp = (int) std::min((double) dlen - 1, jsRound(sps * 3));
        const double nyq = 0.45 * sr;
        const double drive = 1 + p.drive * 7, dGain = 0.75 - p.drive * 0.3;
        const double mix = p.delay * 0.7;
        const double dAmt = p.dist; const bool dOn = dAmt > 0.002; const int dType = p.dtype; const double dMix = p.dmix;
        const double dPre = dType == 0 ? 1 + dAmt * dAmt * 24 : dType == 1 ? 2 + dAmt * dAmt * 60 : 1 + dAmt * 6;
        const double trims[4] = {0.52, 0.5, 0.7, 1};
        const double dOut = 1 + ((dType >= 0 && dType < 4 ? trims[dType] : 0.52) - 1) * std::min(1.0, dAmt * 5);
        const double dLpC = 1 - std::exp(-2 * PI * std::min(nyq, 700 * std::pow(2.0, p.dtone * 4.8)) / sr);
        const double crushLv = std::pow(2.0, 12 - dAmt * 9.5);
        const double crushHold = std::max(1.0, jsRound((1 + dAmt * dAmt * 20) * sr / 44100));
        const double ratio2 = std::pow(2.0, (p.interval * 100 + (p.detune - 0.5) * 100) / 1200);
        const double m1 = std::cos(p.morph * PI / 2), m2 = std::sin(p.morph * PI / 2);
        const double rAmt = p.ring; const int rMode = p.rmode;
        const double rDry = std::cos(rAmt * PI / 2), rWet = std::sin(rAmt * PI / 2);
        const double rRatio = std::pow(2.0, (p.rpitch - 0.5) * 4);
        const double rFixed = 20 * std::pow(2.0, p.rpitch * 7);
        const double kAmpC = std::exp(-1 / ((0.12 + p.kdecay * 0.6) * sr));
        auto oct = [](double v) { return std::pow(2.0, (v - 0.5) * 2); };
        auto tsc = [](double v) { return std::pow(4.0, (v - 0.5) * 2); };
        const double kBase = 45 * oct(p.ktune), kSweepHz = 375 * p.kpunch;
        const double kPitC = std::exp(-1 / (0.03 * tsc(p.ksweep) * sr));
        const double clickC = std::exp(-1 / (0.0015 * sr));
        const double kClickAmt = p.kclick * 0.5, kDrv = 0.6 + p.kdrive * 2;
        const double sHz = 185 * oct(p.stune);
        const double sToneAmt = 0.5 * std::min(1.0, 2 * (1 - p.ssnap)), sNoiseAmt = 0.55 * std::min(1.0, 2 * p.ssnap);
        const double sDec = tsc(p.sdecay);
        const double sToneC = std::exp(-1 / (0.07 * sDec * sr)), sNoiseC = std::exp(-1 / (0.15 * sDec * sr));
        const double cSp = 0.004 + p.cspread * 0.012, cTail = 0.11 * tsc(p.cdecay), cHit = cSp * 3, cEnd = cHit + cTail * 7;
        const double chC = std::exp(-1 / (0.04 * tsc(p.chdecay) * sr)), ohC = std::exp(-1 / (0.3 * tsc(p.ohdecay) * sr));
        auto hMetal = [](double v) { return 0.1 * (v < 0.5 ? 1 + (0.5 - v) * 6 : 2 * (1 - v)); };
        auto hNoise = [](double v) { return 0.4 * std::min(1.0, 2 * v); };
        const double chM = hMetal(p.chnoise), chN = hNoise(p.chnoise), ohM = hMetal(p.ohnoise), ohN = hNoise(p.ohnoise);
        const double chScale = oct(p.chtune), ohScale = oct(p.ohtune);
        for (int j = 0; j < 6; ++j) { chInc[j] = hF[j] * chScale / sr; ohInc[j] = hF[j] * ohScale / sr; }
        tuneSvf(fS, sr, 1800 * std::pow(2.0, (p.stone - 0.5) * 3.4), 0.7);
        tuneSvf(fC, sr, 1150 * std::pow(2.0, (p.ctone - 0.5) * 3), 2.2);
        tuneSvf(fH, sr, 7500 * std::pow(2.0, (p.chtone - 0.5) * 2.4), 0.9);
        tuneSvf(fH2, sr, 7500 * std::pow(2.0, (p.ohtone - 0.5) * 2.4), 0.9);
        const double bGain = 0.8 * std::pow(p.blevel / 0.8, 2);
        const double drumGain = 0.525 * std::pow(p.dlevel / 0.7, 2);
        auto lv = [](double v) { return std::pow(v / 0.8, 2); };
        const double gK = lv(p.lvk), gS = lv(p.lvs), gC = lv(p.lvc), gCH = lv(p.lvch), gOH = lv(p.lvoh);
        const double duckAmt = p.duck * 0.9 * std::min(1.0, p.dlevel * 5);
        const double duckAtkC = std::exp(-1 / (0.002 * sr));
        const double duckRelC = std::exp(-1 / ((0.04 + p.ducktime * 0.36) * sr));
        const double revMix = p.reverb * 1.8;
        const double revFb = 0.72 + p.rsize * 0.26;
        // kick + snare compressor (soft 6 dB knee). When it's off, the drums take the original path untouched.
        const bool cOn = p.cmp != 0;
        const double cThr = -36 + p.cthr * 36, cSlope = 1 - 1 / (1 + 19 * p.cratio * p.cratio);
        const double cAtkC = 1 - std::exp(-1 / (0.0001 * std::pow(500.0, p.catk) * sr));
        const double cRelC = 1 - std::exp(-1 / (0.02 * std::pow(25.0, p.crel) * sr));
        const double cMake = p.cgain * 18, cMix = p.cmix;

        for (int i = 0; i < n; ++i) {
            if (playing && !externalClock) {
                toNext -= 1;
                if (toNext <= 0) { trigger(sps); toNext += sps; }
            }
            const bool gate = gateLeft > 0 || liveN > 0;
            if (gate) gateLeft -= 1;

            cutS += (p.cutoff - cutS) * 0.002;
            resS += (p.reso - resS) * 0.002;

            freq = target + (freq - target) * glideC;
            const double dt = freq / sr;
            const double dt2 = dt * ratio2;
            const bool awake = playing || gate || aenv > 1e-9;
            double osc = 0;
            if (awake) {
                if (m1 > 0.001) osc += wave(p.wave, phase, dt) * m1;
                if (m2 > 0.001) osc += wave(p.wave2, phase2, dt2) * m2;
                if (rAmt > 0.002) {
                    if (rMode == 2) {
                        const double prod = wave(p.wave, phase, dt) * wave(p.wave2, phase2, dt2);
                        osc = osc * rDry + prod * rWet * 1.7;
                    } else {
                        const double car = std::sin(2 * PI * rPh);
                        rPh += (rMode == 0 ? freq * rRatio : rFixed) / sr;
                        if (rPh >= 1) rPh -= std::floor(rPh);
                        osc = osc * rDry + osc * car * rWet * 1.4;
                    }
                }
                phase += dt; if (phase >= 1) phase -= 1;
                phase2 += dt2; if (phase2 >= 1) phase2 -= 1;
            }

            fenv *= envC;
            accEnv *= accC;
            aenv = gate ? 1 + (aenv - 1) * attC : aenv * relC;

            const double acc = p.accent * accLevel;
            double sig = 0;
            if (awake) {
                double fc = 40 * std::pow(2.0, cutS * 7.5 + p.envmod * 4.5 * fenv + acc * 2.5 * accEnv);
                if (fc > nyq) fc = nyq;
                const double g = std::tan(PI * fc / sr);
                const double G = g / (1 + g);
                const double k = resS * 3.9;
                double x = std::tanh(osc - k * last);
                for (int j = 0; j < 4; ++j) {
                    const double v = (x - stg[j]) * G;
                    const double y = v + stg[j];
                    stg[j] = y + v;
                    x = y;
                }
                last = x;

                sig = x * (1 + resS * 1.3) * aenv * (1 + acc * 0.7);
                sig = std::tanh(sig * drive) * dGain;
                if (dOn) {
                    double w;
                    if (dType == 3) {
                        if (--dsHc <= 0) { dsHc = crushHold; dsHv = jsRound(sig * crushLv) / crushLv; }
                        w = dsHv;
                    } else {
                        const double u = sig * dPre;
                        if (dType == 0) { const double b = 0.25 * dAmt; w = std::tanh(u + b) - std::tanh(b); }
                        else if (dType == 1) { const double c = u > 1 ? 1 : u < -1 ? -1 : u; w = 1.5 * c - 0.5 * c * c * c; }
                        else w = std::sin(u * 1.3);
                    }
                    dsLp += (w - dsLp) * dLpC;
                    const double y = dsLp - dsX1 + 0.995 * dsY1;
                    dsX1 = dsLp; dsY1 = y;
                    sig = sig * (1 - dMix) + y * dOut * dMix;
                }
            }

            // tempo-synced echo (dotted eighth)
            double wet = 0;
            if (dlSleep && (sig > 1e-9 || sig < -1e-9)) dlSleep = false;
            if (!dlSleep) {
                int rd = dpos - dsamp; if (rd < 0) rd += dlen;
                wet = dbuf[(size_t) rd];
                dbuf[(size_t) dpos] = (float) (sig + wet * 0.42);
                dpos = (dpos + 1) % dlen;
                if (sig < 1e-9 && sig > -1e-9 && wet < 1e-9 && wet > -1e-9) {
                    if (++dlQuiet > dlen) { dlSleep = true; dlQuiet = 0; std::fill(dbuf.begin(), dbuf.end(), 0.0f); }
                } else dlQuiet = 0;
            }

            // drums
            double drum = 0, kickPart = 0, snarePart = 0;
            const double nz = random() * 2 - 1;
            if (kAmp > 1e-4) {
                kPh += (kBase + kSweepHz * kPit) / sr; if (kPh >= 1) kPh -= 1;
                kickPart = std::tanh((std::sin(2 * PI * kPh) * kAmp + nz * kClick * kClickAmt) * kDrv) * 0.95 * gK * kVel;
                drum += kickPart;
                kAmp *= kAmpC; kPit *= kPitC; kClick *= clickC;
            }
            if (sTone > 1e-4 || sNoise > 1e-4) {
                sPh += sHz / sr; if (sPh >= 1) sPh -= 1;
                const double hp = svf(fS, nz);
                snarePart = (std::sin(2 * PI * sPh) * sTone * sToneAmt + hp * sNoise * sNoiseAmt) * gS * sVel;
                drum += snarePart;
                sTone *= sToneC; sNoise *= sNoiseC;
            }
            if (cOn) {
                const double x = kickPart + snarePart, ax = x < 0 ? -x : x;
                double cTarget = 0;
                if (ax > 1e-6) {
                    const double over = std::log(ax) * 8.685889638 - cThr;   // dB above threshold
                    if (over >= 3) cTarget = over * cSlope;
                    else if (over > -3) { const double k = over + 3; cTarget = cSlope * k * k / 12; }
                }
                cGr += (cTarget - cGr) * (cTarget > cGr ? cAtkC : cRelC);
                const double m = 1 - cMix + std::exp((cMake - cGr) * 0.1151292546) * cMix;   // parallel blend
                kickPart *= m; snarePart *= m;
                drum = kickPart + snarePart;
            }
            if (cT >= 0) {
                const double t = cT / sr;
                const double e = t < cHit ? std::exp(-std::fmod(t, cSp) / 0.003) : std::exp(-(t - cHit) / cTail);
                svf(fC, nz);
                drum += fC.bp * e * 1.6 * gC * cVel;
                cT++; if (t > cEnd) cT = -1;
            }
            if (ch > 1e-4) {
                double metal = 0;
                for (int j = 0; j < 6; ++j) { hPh[j] += chInc[j]; if (hPh[j] >= 1) hPh[j] -= 1; metal += hPh[j] < 0.5 ? 1 : -1; }
                drum += svf(fH, metal * chM + nz * chN) * ch * gCH * 0.5 * chVel;
                ch *= chC;
            }
            if (oh > 1e-4) {
                double metal = 0;
                for (int j = 0; j < 6; ++j) { hPh2[j] += ohInc[j]; if (hPh2[j] >= 1) hPh2[j] -= 1; metal += hPh2[j] < 0.5 ? 1 : -1; }
                drum += svf(fH2, metal * ohM + nz * ohN) * oh * gOH * 0.5 * ohVel;
                oh *= ohC;
            }

            if (duckAtk) { duckEnv = 1 + (duckEnv - 1) * duckAtkC; if (duckEnv > 0.98) duckAtk = false; }
            else duckEnv *= duckRelC;
            const double duckGain = 1 - duckAmt * duckEnv;

            const double bassOut = (sig + wet * mix) * bGain * duckGain;
            const double drumOut = drum * drumGain;
            const double dry = bassOut + drumOut;

            double revL = 0, revR = 0;
            if (revMix > 0.001) {
                const double rin = (bassOut + (drum - kickPart) * drumGain * 0.7) * 0.02;
                const bool loud = rin > 1e-9 || rin < -1e-9;
                if (rvSleep && loud) rvSleep = false;
                if (!rvSleep) {
                    for (int j = 0; j < 8; ++j) {
                        revL += comb(cL[j], rin, revFb);
                        revR += comb(cR[j], rin, revFb);
                    }
                    for (int j = 0; j < 4; ++j) { revL = allpass(aL[j], revL); revR = allpass(aR[j], revR); }
                    if (!loud && revL < 1e-9 && revL > -1e-9 && revR < 1e-9 && revR > -1e-9) {
                        if (++rvQuiet > sr) { rvSleep = true; rvQuiet = 0; clearReverb(); }
                    } else rvQuiet = 0;
                }
            }
            if (dry == 0 && revL == 0 && revR == 0) { L[i] = 0; R[i] = 0; continue; }
            L[i] = (float) std::tanh(dry + revL * revMix);
            R[i] = (float) std::tanh(dry + revR * revMix);
        }
    }

    // v is the hit's loudness: 1 for grid hits, MIDI velocity / 127 for hits played from the DAW
    void trigDrums(int m, double v = 1) {
        if (m & Kick) { kAmp = 1; kPit = 1; kClick = 1; kPh = 0; duckAtk = true; kVel = v; }
        if (m & Snare) { sTone = 1; sNoise = 1; sVel = v; }
        if (m & Clap) { cT = 0; cVel = v; }
        if (m & ClosedHat) { ch = 1; oh = 0; chVel = v; }
        if (m & OpenHat) { oh = 1; ohVel = v; }
    }

private:
    double sr;
    uint32_t rng = 0x2545F491u;
    int live[128] = {}; int liveN = 0;
    Line cL[8], cR[8], aL[4], aR[4];
    double dsLp = 0, dsX1 = 0, dsY1 = 0, dsHc = 0, dsHv = 0;
    bool rvSleep = false, dlSleep = false; int rvQuiet = 0, dlQuiet = 0;
    double chInc[6] = {}, ohInc[6] = {};
    double phase = 0, phase2 = 0, rPh = 0, freq = 65.4, target = 65.4;
    double stg[4] = {}, last = 0;
    double fenv = 0, aenv = 0, accEnv = 0, accLevel = 0;
    double toNext = 0, gateLeft = 0; int step = 0; bool prevSlide = false;
    double cutS = 0, resS = 0;
    int dlen = 0; std::vector<float> dbuf; int dpos = 0;
    double duckEnv = 0; bool duckAtk = false;
    double cGr = 0;   // compressor gain reduction (dB)
    double kAmp = 0, kPit = 0, kClick = 0, kPh = 0, sTone = 0, sNoise = 0, sPh = 0, cT = -1, ch = 0, oh = 0;
    double hPh[6] = {}, hPh2[6] = {};
    double kVel = 1, sVel = 1, cVel = 1, chVel = 1, ohVel = 1;
    const double hF[6] = {420, 640, 674, 850, 941, 1352};
    Svf fS, fC, fH, fH2;

    double random() { rng = rng * 1664525u + 1013904223u; return rng / 4294967296.0; }

    void removeLive(int note) {
        int w = 0;
        for (int r = 0; r < liveN; ++r) if (live[r] != note) live[w++] = live[r];
        liveN = w;
    }

    void doStep(double sps) {
        trigDrums(dbanks[adb][step]);
        const Step& s = banks[ab][step];
        const bool tied = prevSlide && gateLeft > 0;
        if (liveN > 0) {
            // a held MIDI note takes over the bass line; drums keep running
        } else if (!s.r) {
            target = hz(s.n, s.o);
            accLevel = s.a ? 1 : 0;
            if (!tied) { freq = target; fenv = 1; accEnv = 1; }
            else if (s.a) accEnv = 1;
            gateLeft = s.s ? sps * 1.05 : sps * 0.5;
            prevSlide = s.s;
        } else {
            gateLeft = 0; prevSlide = false;
        }
        lastStep = step;
        step = (step + 1) % 16;
    }

    static double comb(Line& c, double in, double fb) {
        const double y = c.b[(size_t) c.i];
        const double f = y * 0.7 + c.s * 0.3; c.s = f;
        c.b[(size_t) c.i] = (float) (in + f * fb);
        if (++c.i >= (int) c.b.size()) c.i = 0;
        return y;
    }
    static double allpass(Line& a, double in) {
        const double bo = a.b[(size_t) a.i];
        a.b[(size_t) a.i] = (float) (in + bo * 0.5);
        if (++a.i >= (int) a.b.size()) a.i = 0;
        return bo - in;
    }
    void clearReverb() {
        for (Line* arr : {cL, cR}) for (int j = 0; j < 8; ++j) { std::fill(arr[j].b.begin(), arr[j].b.end(), 0.0f); arr[j].s = 0; arr[j].i = 0; }
        for (Line* arr : {aL, aR}) for (int j = 0; j < 4; ++j) { std::fill(arr[j].b.begin(), arr[j].b.end(), 0.0f); arr[j].s = 0; arr[j].i = 0; }
    }

    static double svf(Svf& f, double x) {
        const double v3 = x - f.ic2;
        const double v1 = f.a1 * f.ic1 + f.a2 * v3;
        const double v2 = f.ic2 + f.a2 * f.ic1 + f.a3 * v3;
        f.ic1 = 2 * v1 - f.ic1; f.ic2 = 2 * v2 - f.ic2;
        f.bp = v1; f.lp = v2;
        return x - f.k * v1 - v2;
    }
    static void tuneSvf(Svf& f, double sr, double fc, double q) {
        if (f.fc == fc) return;
        const Svf n = mkSvf(sr, fc, q);
        f.a1 = n.a1; f.a2 = n.a2; f.a3 = n.a3; f.k = n.k; f.fc = fc;
    }
    static Svf mkSvf(double sr, double fc, double q) {
        const double g = std::tan(PI * std::min(fc, sr * 0.45) / sr), k = 1 / q;
        Svf f; f.a1 = 1 / (1 + g * (g + k)); f.a2 = g * f.a1; f.a3 = g * f.a2; f.k = k;
        return f;
    }
    // 0 saw, 1 square, 2 pulse (25%), 3 triangle, 4 sine
    static double wave(int w, double ph, double dt) {
        switch (w) {
            case 1: { double p2 = ph + 0.5; if (p2 >= 1) p2 -= 1; return (ph < 0.5 ? 1 : -1) + blep(ph, dt) - blep(p2, dt); }
            case 2: { double p2 = ph + 0.75; if (p2 >= 1) p2 -= 1; return (ph < 0.25 ? 1 : -1) + blep(ph, dt) - blep(p2, dt) + 0.5; }
            case 3: return (4 * std::abs(ph - 0.5) - 1) * 1.2;
            case 4: return std::sin(2 * PI * ph) * 1.2;
            default: return 2 * ph - 1 - blep(ph, dt);
        }
    }
    static double blep(double t, double dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1; }
        if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
        return 0;
    }
};

} // namespace acid
