#pragma once
#include <Geode/Geode.hpp>

namespace atfw {
class ATFWPopup : public geode::Popup {            // VERIFY: Geode 5 Popup base
public:
    static ATFWPopup* create();
protected:
    bool init();
    void onImport(cocos2d::CCObject*);
    void onFps(cocos2d::CCObject*);
    void onAnalyze(cocos2d::CCObject*);
    void onReplay(cocos2d::CCObject*);
    void onToggle(cocos2d::CCObject*);
    void status(std::string const& s);
    geode::TaskHolder<geode::Result<std::filesystem::path>> m_pick;   // VERIFY
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCLabelBMFont* m_fpsLabel = nullptr;
};
}
