#pragma once
#include <Geode/Geode.hpp>
#include "Analyzer.hpp"
#include "Session.hpp"
#include <map>

namespace atfw {

// Real-game implementation of ISim. NOT compiled or tested against the real
// bindings: every line marked VERIFY relies on a name/behavior I could not check.
class GameSim : public ISim {
public:
    GameSim(PlayLayer* pl, float tps) : m_pl(pl), m_tps(tps) {}

    // One silent pass over the recording, saving a checkpoint every kEvery ticks.
    bool capture(const File& f);

    bool run(uint32_t fromTick, uint32_t toTick, const std::vector<Input>& inputs) override;

private:
    static constexpr uint32_t kEvery = 60;
    uint32_t progress() const;
    bool stepOne();                                   // advance exactly one tick

    PlayLayer* m_pl;
    float m_tps;
    std::map<uint32_t, geode::Ref<CheckpointObject>> m_cps;
};

} // namespace atfw
