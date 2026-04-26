#include <Geode/Geode.hpp>

using namespace geode::prelude;

$execute{
    for (auto path : {
        string::pathToString(Mod::get()->getConfigDir()),
        string::pathToString(Mod::get()->getSaveDir()),
        string::pathToString(Mod::get()->getTempDir())
    }) CCFileUtils::get()->addPriorityPath(path.c_str());
};


#include <Geode/modify/AchievementManager.hpp>
class $modify(MLE_AchievementManager, AchievementManager) {
    void resortAchievements() {
        //i can't regenerate changed sort at runtime cuz im stupid
        auto createdOrLoadedFileDeleted = false;
        createdOrLoadedFileDeleted = !fileExistsInSearchPaths("achievements-sort.txt")
            and CCFileUtils::get()->m_fullPathCache.contains("achievements-sort.txt");
        if (createdOrLoadedFileDeleted) return game::restart(true);
        // create(and load) or load sort
        if (!fileExistsInSearchPaths("achievements-sort.txt")) {
            auto list = std::stringstream();
            for (auto dict : CCArrayExt<CCDictionary*>(m_allAchievementsSorted)) {
                /*
                log::debug("{}", dict);
                for (auto [key, value] : CCDictionaryExt<std::string, CCString*>(dict)) {
                    log::debug("{} -> {}", key, value->getCString());
                }
                CCMessageBox("dsa", "asd");
                */
                list << CCDictionaryExt<std::string, CCString*>(
                    dict
                )["identifier"]->getCString() << "\n";
            }
            file::writeStringSafe(
                getMod()->getConfigDir() / "achievements-sort.txt",
                list.str()
            ).isOk();
            resortAchievements();
        }
        else {
            auto list = file::readString(CCFileUtils::get()->fullPathForFilename(
                "achievements-sort.txt", 0
            ).c_str()).unwrapOr("");
            m_allAchievementsSorted->removeAllObjects();
            for (auto identifier : string::split(list, "\n")) {
                auto dict = getAchievementsWithID(identifier.c_str());
                if (dict) m_allAchievementsSorted->addObject(dict);
            }
        }
    }
    $override void addAchievement(
        gd::string identifier, gd::string title,
        gd::string achievedDescription, gd::string unachievedDescription,
        gd::string icon, int limits
    ) {
        if (this->getUserObject("is-data-file-generating"_spr)) {
            auto val = file::readJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str()).unwrapOr(matjson::Value());

            auto entry = matjson::Value();
            entry["title"] = title.c_str();
            entry["achievedDescription"] = achievedDescription.c_str();
            entry["unachievedDescription"] = unachievedDescription.c_str();
            entry["icon"] = icon.c_str();
            entry["limits"] = limits;

            val[identifier.c_str()] = entry;

            file::writeToJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str(), val).err();
        }
        else {
            AchievementManager::addAchievement(
                identifier, title,
                achievedDescription, unachievedDescription,
                icon, limits
            );
        };
    }
    $override void addManualAchievements() {
        if (!fileExistsInSearchPaths("achievements.json")) { // generate default file
            file::writeStringSafe(getMod()->getConfigDir() / "achievements.json", "{}").err();
            auto object = new CCObject();
            object->autorelease();
            setUserObject("is-data-file-generating"_spr, object);
            AchievementManager::addManualAchievements();
            setUserObject("is-data-file-generating"_spr, nullptr);
            addManualAchievements();
        }
        else {
            auto val = file::readJson(CCFileUtils::get()->fullPathForFilename(
                "achievements.json", 0
            ).c_str()).unwrapOr(matjson::Value());
            for (auto& [identifier, entry] : val) {
                AchievementManager::addAchievement(
                    identifier, entry["title"].asString().unwrapOr("err").c_str(),
                    entry["achievedDescription"].asString().unwrapOr("err").c_str(),
                    entry["unachievedDescription"].asString().unwrapOr("err").c_str(),
                    entry["icon"].asString().unwrapOr("err").c_str(),
                    entry["limits"].asInt().unwrapOr(0)
                );
            };
        }

        queueInMainThread(
            [aw = Ref(this)] {
                if (aw) aw->resortAchievements();
            }
        );

    }
    inline static auto s_Events = matjson::Value();
};