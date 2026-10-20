#pragma once
#include "TTRGF.hpp"
#include "TTRL.hpp"

namespace atfw {

// Shared state between the menu, the hooks and the HUD.
struct Session {
    TtrlReplay ttrl; bool haveTtrl = false;
    File file;       bool haveFile = false;   // analyzed or loaded .TTRGF
    bool replaying = false;
    float fps = 60.f;                         // counter FPS (user choice)
    std::string name = "replay";
    static Session& get() { static Session s; return s; }
};

// Feeds recorded inputs into the game from inside processCommands.
struct Feed {
    bool active = false, died = false;
    const std::vector<Input>* inputs = nullptr;
    size_t next = 0;
};
inline Feed& simFeed()  { static Feed f; return f; }   // analysis re-simulation
inline Feed& playFeed() { static Feed f; return f; }   // visible .TTRGF playback

} // namespace atfw
