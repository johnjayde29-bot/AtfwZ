#include "GameSim.hpp"
using namespace geode::prelude;

namespace atfw {

uint32_t GameSim::progress() const {
    return (uint32_t)m_pl->m_gameState.m_currentProgress;   // VERIFY: tick counter field
}

bool GameSim::stepOne() {
    auto before = progress();
    // VERIFY: one update(1/tps) should advance exactly one physics tick. If the game
    // accumulates delta, rounding may give 0 or 2 ticks, so retry and check.
    for (int guard = 0; guard < 4 && progress() == before; guard++)
        m_pl->update(1.f / m_tps);
    if (progress() != before + 1) {
        log::error("ATFW: expected 1 tick, got {}", (int)progress() - (int)before);
        return false;
    }
    return true;
}

bool GameSim::capture(const File& f) {
    m_cps.clear();
    auto& feed = simFeed();
    feed = {true, false, &f.inputs, 0};
    m_pl->resetLevel();                                     // VERIFY: returns to tick 0
    while (progress() < f.totalFrames && !feed.died) {
        if (progress() % kEvery == 0)
            m_cps[progress()] = m_pl->createCheckpoint();   // VERIFY: returns a CheckpointObject*
        if (!stepOne()) { feed.active = false; return false; }
    }
    feed.active = false;
    return !feed.died && m_cps.count(0);
}

bool GameSim::run(uint32_t from, uint32_t to, const std::vector<Input>& inputs) {
    auto it = m_cps.upper_bound(from);
    if (it == m_cps.begin()) return false;
    --it;                                                   // latest checkpoint <= from
    m_pl->loadFromCheckpoint(it->second);                   // VERIFY
    if (progress() != it->first) {
        log::error("ATFW: checkpoint restored tick {} not {}", progress(), it->first);
        return false;
    }
    auto& feed = simFeed();
    feed = {true, false, &inputs, 0};
    while (feed.next < inputs.size() && inputs[feed.next].frame < it->first) feed.next++;
    bool ok = true;
    while (progress() < to && !feed.died) {
        if (!stepOne()) { ok = false; break; }
    }
    feed.active = false;
    return ok && !feed.died;
}

} // namespace atfw
