#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "Session.hpp"
#include "Menu.hpp"

using namespace geode::prelude;

static void feedInputs(GJBaseGameLayer* l, atfw::Feed& f) {
    if (!f.inputs) return;
    auto prog = (uint32_t)l->m_gameState.m_currentProgress;            // VERIFY
    while (f.next < f.inputs->size() && (*f.inputs)[f.next].frame <= prog) {
        auto& e = (*f.inputs)[f.next++];
        l->handleButton(e.flags & 1, e.button, !(e.flags & 2));        // VERIFY signature
    }
}

class $modify(ATFWBase, GJBaseGameLayer) {
    void processCommands(float dt) {                                   // VERIFY: hook point
        if (atfw::simFeed().active) feedInputs(this, atfw::simFeed());
        else if (atfw::playFeed().active) feedInputs(this, atfw::playFeed());
        GJBaseGameLayer::processCommands(dt);
    }
};

class $modify(ATFWMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("ATFW"), this, menu_selector(ATFWMenu::onATFW));
        if (auto menu = this->getChildByID("bottom-menu")) { menu->addChild(btn); menu->updateLayout(); }
        return true;
    }
    void onATFW(CCObject*) { atfw::ATFWPopup::create()->show(); }
};

class $modify(ATFWPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("ATFW", 40, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.6f),
            this, menu_selector(ATFWPause::onATFW));
        if (auto menu = this->getChildByID("right-button-menu")) { menu->addChild(btn); menu->updateLayout(); }
    }
    void onATFW(CCObject*) { atfw::ATFWPopup::create()->show(); }
};

class $modify(ATFWPlay, PlayLayer) {
    struct Fields {
        CCLabelBMFont* hud = nullptr;
        int64_t lastDing = -1;
    };

    void startFeed() {
        auto& S = atfw::Session::get();
        atfw::playFeed() = {S.replaying && S.haveFile, false, &S.file.inputs, 0};
        m_fields->lastDing = -1;
        if (m_fields->hud) m_fields->hud->setString("");
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto l = CCLabelBMFont::create("", "bigFont.fnt");
        l->setAnchorPoint({0.f, 1.f});
        l->setScale(0.4f);
        auto win = CCDirector::get()->getWinSize();
        l->setPosition({6.f, win.height - 6.f});
        l->setAlignment(kCCTextAlignmentLeft);
        l->setZOrder(1000);
        this->addChild(l);
        m_fields->hud = l;
        startFeed();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        startFeed();
    }

    void destroyPlayer(PlayerObject* p, GameObject* o) {
        if (atfw::simFeed().active) { atfw::simFeed().died = true; return; }   // don't really die during analysis
        PlayLayer::destroyPlayer(p, o);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& S = atfw::Session::get();
        if (!S.replaying || !S.haveFile || atfw::simFeed().active) return;
        int64_t now = m_gameState.m_currentProgress;                    // VERIFY
        bool changed = false;
        for (auto& e : S.file.sounds) {
            if ((int64_t)e.frame > m_fields->lastDing && (int64_t)e.frame <= now) {
                changed = true;
                if (Mod::get()->getSettingValue<bool>("audio"))
                    FMODAudioEngine::sharedEngine()->playEffect("ding.ogg"_spr);
            }
        }
        m_fields->lastDing = now;
        if (changed && Mod::get()->getSettingValue<bool>("counter")) {
            std::string txt = fmt::format("{} FPS\n", (int)S.file.fps);
            for (size_t k = 0; k < S.file.results.size(); k++)
                if ((int64_t)S.file.sounds[k].frame <= now)
                    txt += fmt::format("{} / {:.1f}\n", S.file.results[k].frames, S.file.results[k].precise);
            m_fields->hud->setString(txt.c_str());
        }
    }
};
