#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

namespace atfw {

struct Input { uint32_t frame; uint8_t button; uint8_t flags; };
struct Result {
    uint32_t id, clickIndex, firstValid, lastValid;
    uint16_t frames; uint8_t type; float precise;
};
struct SoundEvent { uint32_t frame, resultId; uint8_t soundId; };

struct File {
    float fps = 60.f;   // counter FPS selected by the user
    float tps = 240.f;  // tick rate stored in the replay
    uint32_t levelId = 0, totalFrames = 0;
    std::vector<Input> inputs;
    std::vector<Result> results;
    std::vector<SoundEvent> sounds;
};

inline uint32_t crc32(const std::string& d) {
    uint32_t c = ~0u;
    for (unsigned char b : d) {
        c ^= b;
        for (int i = 0; i < 8; i++) c = (c >> 1) ^ (0xEDB88320u & -(c & 1));
    }
    return ~c;
}

template <class T> void put(std::string& s, T v) { s.append(reinterpret_cast<char*>(&v), sizeof(T)); }
template <class T> bool get(const std::string& s, size_t& p, T& v) {
    if (p + sizeof(T) > s.size()) return false;
    std::memcpy(&v, s.data() + p, sizeof(T)); p += sizeof(T); return true;
}

inline bool write(const File& f, const std::string& path) {
    std::string s("TTRG", 4);
    put<uint16_t>(s, 1); put<uint16_t>(s, 0);
    put(s, f.fps); put(s, f.tps); put(s, f.levelId); put(s, f.totalFrames);
    put<uint32_t>(s, f.inputs.size());
    put<uint32_t>(s, f.results.size());
    put<uint32_t>(s, f.sounds.size());
    for (auto& i : f.inputs) { put(s, i.frame); put(s, i.button); put(s, i.flags); }
    for (auto& r : f.results) {
        put(s, r.id); put(s, r.clickIndex); put(s, r.firstValid); put(s, r.lastValid);
        put(s, r.frames); put(s, r.type); put<uint8_t>(s, 0); put(s, r.precise);
    }
    for (auto& e : f.sounds) { put(s, e.frame); put(s, e.resultId); put(s, e.soundId); }
    put(s, crc32(s));
    std::ofstream o(path, std::ios::binary);
    o.write(s.data(), s.size());
    return o.good();
}

inline bool read(File& f, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(in)), {});
    if (s.size() < 40 || s.compare(0, 4, "TTRG") != 0) return false;
    uint32_t stored; std::memcpy(&stored, s.data() + s.size() - 4, 4);
    if (crc32(s.substr(0, s.size() - 4)) != stored) return false;
    size_t p = 4; uint16_t ver, flags; uint32_t ni, nr, ns;
    get(s, p, ver); get(s, p, flags);
    if (ver > 1) return false;
    get(s, p, f.fps); get(s, p, f.tps); get(s, p, f.levelId); get(s, p, f.totalFrames);
    get(s, p, ni); get(s, p, nr); get(s, p, ns);
    f.inputs.resize(ni); f.results.resize(nr); f.sounds.resize(ns);
    for (auto& i : f.inputs) { get(s, p, i.frame); get(s, p, i.button); get(s, p, i.flags); }
    for (auto& r : f.results) {
        uint8_t pad;
        get(s, p, r.id); get(s, p, r.clickIndex); get(s, p, r.firstValid); get(s, p, r.lastValid);
        get(s, p, r.frames); get(s, p, r.type); get(s, p, pad); get(s, p, r.precise);
    }
    for (auto& e : f.sounds) { get(s, p, e.frame); get(s, p, e.resultId); get(s, p, e.soundId); }
    return true;
}

} // namespace atfw
