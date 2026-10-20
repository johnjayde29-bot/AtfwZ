#pragma once
#include "TTRGF.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

namespace atfw {

// The only piece that touches the real game. Implement it in Geode:
//  - keep checkpoints taken during one normal pass of the replay
//  - run(): restore the latest checkpoint at or before fromTick, play `inputs`
//    through the same path replay playback uses, step to toTick,
//    return true if the player is still alive.
// UNVERIFIED against the real GD/Geode headers.
struct ISim {
    virtual ~ISim() = default;
    virtual bool run(uint32_t fromTick, uint32_t toTick, const std::vector<Input>& inputs) = 0;
};

struct Options {
    float fps = 60.f;               // counter FPS
    uint32_t maxWindowFrames = 10;  // ignore clicks with a bigger window (they don't constrain the run)
    uint32_t lookaheadTicks = 120;  // how far past the click to check survival
    bool shiftRelease = true;       // move the matching release with the press
};

// Shift one press (and optionally its matching release) by `off` ticks.
// Returns false if the result is out of range or reorders events.
inline bool shifted(const File& f, size_t press, size_t release, int off, bool withRelease,
                    std::vector<Input>& out) {
    out = f.inputs;
    auto mv = [&](size_t i) {
        int64_t t = (int64_t)out[i].frame + off;
        if (t < 0 || t >= (int64_t)f.totalFrames) return false;
        out[i].frame = (uint32_t)t; return true;
    };
    if (!mv(press)) return false;
    if (withRelease && release != SIZE_MAX && !mv(release)) return false;
    return std::is_sorted(out.begin(), out.end(),
                          [](const Input& a, const Input& b) { return a.frame < b.frame; });
}

inline size_t matchingRelease(const File& f, size_t press) {
    auto& p = f.inputs[press];
    for (size_t j = press + 1; j < f.inputs.size(); j++) {
        auto& q = f.inputs[j];
        if (q.button == p.button && (q.flags & 2) == (p.flags & 2) && !(q.flags & 1)) return j;
    }
    return SIZE_MAX;
}

// Fills f.results and f.sounds. For each press: scan outward from the original
// timing until the first death in each direction; the run of survivors is the window.
inline void analyze(File& f, ISim& sim, const Options& o,
                    const std::function<void(size_t, size_t)>& progress = {}) {
    f.fps = o.fps;
    f.results.clear(); f.sounds.clear();
    double tpf = f.tps / o.fps;                           // ticks per counter frame
    int cap = (int)std::ceil(o.maxWindowFrames * tpf);    // search radius in ticks
    for (size_t i = 0; i < f.inputs.size(); i++) {
        if (progress) progress(i, f.inputs.size());
        if (!(f.inputs[i].flags & 1)) continue;
        uint32_t tick = f.inputs[i].frame;
        size_t rel = matchingRelease(f, i);
        uint32_t from = tick > (uint32_t)cap + 1 ? tick - cap - 1 : 0;
        uint32_t to = std::min<uint32_t>(f.totalFrames, tick + cap + o.lookaheadTicks);
        std::vector<Input> in;
        auto ok = [&](int off) {
            return shifted(f, i, rel, off, o.shiftRelease, in) && sim.run(from, to, in);
        };
        if (!ok(0)) continue;                             // recording itself dies here: skip
        int lo = 0, hi = 0;
        while (lo < cap && ok(-(lo + 1))) lo++;
        while (hi < cap && ok(hi + 1)) hi++;
        if (lo == cap && hi == cap) continue;             // unconstrained click
        int W = lo + hi + 1;                              // window in ticks
        Result r{};
        r.id = (uint32_t)f.results.size();
        r.clickIndex = (uint32_t)i;
        r.firstValid = tick - lo; r.lastValid = tick + hi;
        r.precise = (float)(W / tpf);
        r.frames = (uint16_t)std::floor(r.precise + 1e-4);
        r.type = r.precise <= 1.0001f ? 1 : 0;            // 1 = frame-perfect (<= 1 frame)
        f.results.push_back(r);
        f.sounds.push_back({tick, r.id, r.type});         // soundId 0 = ding, 1 = frame-perfect ding
    }
}

} // namespace atfw
