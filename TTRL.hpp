#pragma once
// ToastyReplay-Lite .ttrl reader. Layout taken from the TtrlCodec.cpp source (v1).
// Note: the pasted source was cut off inside the input decoder, so the input
// layout below is mirrored from the encoder. Tested against 4 real files.
#include "TTRGF.hpp"
#include <optional>
#include <string>

namespace atfw {

struct FrameFix {
    uint64_t afterTick = 0;
    bool player2 = false;
    float x = 0, y = 0, rotation = 0;
    double yVel = 0;
    uint16_t state = 0;
    float vehicleSize = 1, playerSpeed = 1, gravityMod = 1;
};

struct TtrlReplay {
    uint64_t tpsNum = 240, tpsDen = 1;
    uint32_t gameVersion = 0;
    uint64_t levelId = 0, levelRevision = 0, levelFingerprint = 0, tickCount = 0;
    uint8_t flags = 0;
    std::optional<uint64_t> seed;
    std::optional<float> startPos;
    std::optional<bool> controlFlip;
    std::vector<Input> inputs;      // frame = tick the input applies before
    std::vector<FrameFix> fixes;
    double tps() const { return double(tpsNum) / double(tpsDen); }
};

namespace ttrl {
struct Rd {
    const uint8_t* p; size_t n, i = 0; bool bad = false;
    uint8_t u8() { if (i >= n) { bad = true; return 0; } return p[i++]; }
    uint64_t var() {
        uint64_t v = 0;
        for (int s = 0; s <= 63; s += 7) {
            uint8_t b = u8(); if (bad) return 0;
            v |= uint64_t(b & 0x7f) << s;
            if (!(b & 0x80)) return v;
        }
        bad = true; return 0;
    }
    template <class T> T raw() {
        T v{}; if (n - i < sizeof(T)) { bad = true; return v; }
        std::memcpy(&v, p + i, sizeof(T)); i += sizeof(T); return v;
    }
};
}

inline bool readTTRL(const std::string& path, TtrlReplay& out, std::string& err) {
    std::ifstream in(path, std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(in)), {});
    if (s.size() < 25) { err = "file too small"; return false; }
    uint32_t stored; std::memcpy(&stored, s.data() + s.size() - 4, 4);
    if (crc32(s.substr(0, s.size() - 4)) != stored) { err = "checksum mismatch"; return false; }
    ttrl::Rd r{reinterpret_cast<const uint8_t*>(s.data()), s.size() - 4};
    if (std::memcmp(s.data(), "TTRL", 4) != 0) { err = "not a TTRL replay"; return false; }
    r.i = 4;
    if (r.u8() != 1) { err = "unsupported version"; return false; }
    uint8_t flags = r.u8(); out.flags = flags;
    if (flags & ~63) { err = "unknown flags"; return false; }
    out.tpsNum = r.var(); out.tpsDen = r.var();
    out.gameVersion = (uint32_t)r.var();
    out.levelId = r.var(); out.levelRevision = r.var();
    out.levelFingerprint = r.raw<uint64_t>();
    out.tickCount = r.var();
    if (flags & 8)  out.seed = r.var();
    if (flags & 16) out.startPos = r.raw<float>();
    if (flags & 32) out.controlFlip = r.u8() != 0;
    uint64_t inSize = r.var();
    if (r.bad || inSize > r.n - r.i) { err = "truncated"; return false; }
    ttrl::Rd ir{r.p + r.i, (size_t)inSize}; r.i += inSize;
    bool channels = flags & 2;
    uint64_t tick = 0;
    while (ir.i < ir.n) {
        uint64_t ev = ir.var(); if (ir.bad) { err = "bad input"; return false; }
        uint64_t state = channels ? (ev & 0xf) : (ev & 1);
        tick += ev >> (channels ? 4 : 1);
        Input in2{};
        in2.frame = (uint32_t)tick;
        if (channels) {
            in2.button = uint8_t(((state >> 2) & 3) + 1);   // 1 = jump (assumed enum order)
            in2.flags = uint8_t((state & 1) | ((state & 2) ? 2 : 0));
        } else { in2.button = 1; in2.flags = uint8_t(state & 1); }
        out.inputs.push_back(in2);
    }
    if (flags & 1) {
        uint8_t schema = r.u8();
        uint64_t size = r.var();
        if (r.bad || size > r.n - r.i) { err = "truncated frame fixes"; return false; }
        ttrl::Rd fr{r.p + r.i, (size_t)size}; r.i += size;
        uint64_t count = fr.var(), prev = 0;
        for (uint64_t k = 0; k < count && !fr.bad; k++) {
            FrameFix f; uint64_t v = fr.var(); uint64_t delta = v;
            if (schema != 1) { f.player2 = v & 1; delta = v >> 1; }
            prev += delta; f.afterTick = prev;
            f.x = fr.raw<float>(); f.y = fr.raw<float>(); f.rotation = fr.raw<float>();
            f.yVel = fr.raw<double>();
            if (schema == 3) {
                f.state = (uint16_t)fr.var();
                f.vehicleSize = fr.raw<float>(); f.playerSpeed = fr.raw<float>();
                f.gravityMod = fr.raw<float>();
            }
            out.fixes.push_back(f);
        }
        if (fr.bad) { err = "bad frame fix"; return false; }
    }
    return true;
}

// Convert into the ATFW working format. tps = tick rate stored in the replay;
// fps (counter FPS) is chosen by the user afterwards.
inline File toFile(const TtrlReplay& t) {
    File f; f.tps = (float)t.tps(); f.levelId = (uint32_t)t.levelId;
    f.totalFrames = (uint32_t)t.tickCount; f.inputs = t.inputs; return f;
}

} // namespace atfw
