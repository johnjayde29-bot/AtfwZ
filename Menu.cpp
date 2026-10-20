#include "Menu.hpp"
#include "Session.hpp"
#include "GameSim.hpp"
using namespace geode::prelude;

namespace atfw {

ATFWPopup* ATFWPopup::create() {
    auto r = new ATFWPopup();
    if (r->init()) { r->autorelease(); return r; }
    delete r; return nullptr;
}

bool ATFWPopup::init() {
    if (!Popup::init(300.f, 250.f)) return false;      // VERIFY
    this->setTitle("ATFW - Auto Frame Window");
    auto add = [&](char const* text, SEL_MenuHandler cb, float y) {
        auto b = CCMenuItemSpriteExtra::create(ButtonSprite::create(text), this, cb);
        m_buttonMenu->addChildAtPosition(b, Anchor::Center, {0.f, y});
    };
    add("Import TTRL", menu_selector(ATFWPopup::onImport), 85.f);
    add("Analyze",     menu_selector(ATFWPopup::onAnalyze), 50.f);
    add("Replay TTRGF", menu_selector(ATFWPopup::onReplay), 15.f);

    auto fps = CCMenuItemSpriteExtra::create(ButtonSprite::create("FPS"), this, menu_selector(ATFWPopup::onFps));
    m_buttonMenu->addChildAtPosition(fps, Anchor::Center, {-60.f, -20.f});
    m_fpsLabel = CCLabelBMFont::create(fmt::format("{}", (int)Session::get().fps).c_str(), "bigFont.fnt");
    m_fpsLabel->setScale(0.5f);
    m_mainLayer->addChildAtPosition(m_fpsLabel, Anchor::Center, {10.f, -20.f});

    for (int i = 0; i < 2; i++) {
        char const* key = i == 0 ? "counter" : "audio";
        auto t = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ATFWPopup::onToggle), 0.6f);
        t->setTag(i);
        t->toggle(Mod::get()->getSettingValue<bool>(key));
        m_buttonMenu->addChildAtPosition(t, Anchor::Center, {-80.f, -55.f - 30.f * i});
        auto l = CCLabelBMFont::create(i == 0 ? "Counter Display" : "Audio Feedback", "bigFont.fnt");
        l->setScale(0.4f);
        l->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChildAtPosition(l, Anchor::Center, {-55.f, -55.f - 30.f * i});
    }
    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.6f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Bottom, {0.f, 15.f});
    return true;
}

void ATFWPopup::status(std::string const& s) { m_status->setString(s.c_str()); }

void ATFWPopup::onToggle(CCObject* o) {
    auto t = static_cast<CCMenuItemToggler*>(o);
    // toggler reports the state before the click
    Mod::get()->setSettingValue<bool>(t->getTag() == 0 ? "counter" : "audio", !t->isToggled());
}

void ATFWPopup::onFps(CCObject*) {
    auto& S = Session::get();
    S.fps = S.fps == 60.f ? 120.f : S.fps == 120.f ? 240.f : 60.f;
    m_fpsLabel->setString(fmt::format("{}", (int)S.fps).c_str());
}

void ATFWPopup::onImport(CCObject*) {
    file::FilePickOptions opts;
    opts.filters.push_back({"ToastyReplay", {"*.ttrl"}});
    m_pick.spawn(file::pick(file::PickMode::OpenFile, opts),            // VERIFY
        [this](Result<std::filesystem::path> r) {
            if (!r || !r.unwrap().has_filename()) return;
            auto& S = Session::get();
            std::string err;
            auto p = r.unwrap();
            if (!readTTRL(p.string(), S.ttrl, err)) { status("Import failed: " + err); return; }
            S.haveTtrl = true;
            S.name = p.stem().string();
            status(fmt::format("Loaded: level {}, {} ticks, {} TPS", S.ttrl.levelId, S.ttrl.tickCount, (int)S.ttrl.tps()));
        });
}

void ATFWPopup::onAnalyze(CCObject*) {
    auto& S = Session::get();
    if (!S.haveTtrl) return status("Import a .ttrl first");
    auto pl = PlayLayer::get();
    if (!pl || (uint64_t)pl->m_level->m_levelID.value() != S.ttrl.levelId)    // VERIFY
        return status(fmt::format("Open level {} first (pause menu)", S.ttrl.levelId));
    S.file = toFile(S.ttrl);
    GameSim sim(pl, S.file.tps);
    status("Analyzing... game may freeze for a while");
    if (!sim.capture(S.file)) return status("Checkpoint pass failed (see log)");
    Options o; o.fps = S.fps;
    analyze(S.file, sim, o);
    S.haveFile = true;
    auto dir = Mod::get()->getSaveDir() / "replays";
    (void)file::createDirectoryAll(dir);
    auto out = dir / (S.name + ".ttrgf");
    bool ok = write(S.file, out.string());
    status(fmt::format("{} windows. {}", S.file.results.size(), ok ? "Saved .ttrgf" : "Save failed"));
}

void ATFWPopup::onReplay(CCObject*) {
    file::FilePickOptions opts;
    opts.filters.push_back({"ATFW analysis", {"*.ttrgf"}});
    m_pick.spawn(file::pick(file::PickMode::OpenFile, opts),
        [this](Result<std::filesystem::path> r) {
            if (!r || !r.unwrap().has_filename()) return;
            auto& S = Session::get();
            if (!read(S.file, r.unwrap().string())) { status("Invalid or corrupt .ttrgf"); return; }
            S.haveFile = true; S.replaying = true;
            status(fmt::format("Loaded. Open level {} and start it", S.file.levelId));
            if (auto pl = PlayLayer::get()) pl->resetLevel();
        });
}

} // namespace atfw
