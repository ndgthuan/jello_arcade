#include "UserData.h"
#include "Achievements.h"
#include "../Config/json.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace
{
    namespace fs = std::filesystem;

    // Old location (working directory); read once as a migration source when the new file does not exist yet.
    const char *kLegacyFile = "userdata.json";

    // %APPDATA%\Jello\userdata.json on Windows; $HOME/.local/share/Jello/userdata.json elsewhere when HOME is set;
    // otherwise the working directory.
    fs::path dataFile()
    {
        if (const char *appData = std::getenv("APPDATA"); appData && *appData)
        {
            return fs::path(appData) / "Jello" / "userdata.json";
        }
#ifndef _WIN32
        if (const char *home = std::getenv("HOME"); home && *home)
        {
            return fs::path(home) / ".local" / "share" / "Jello" / "userdata.json";
        }
#endif
        return fs::path(kLegacyFile);
    }

    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    std::string trim(const std::string &s)
    {
        std::size_t a = 0;
        std::size_t b = s.size();
        while (a < b && std::isspace(static_cast<unsigned char>(s[a])))
        {
            ++a;
        }
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
        {
            --b;
        }
        return s.substr(a, b - a);
    }

    int clampInt(int v, int lo, int hi)
    {
        return std::max(lo, std::min(hi, v));
    }
} // namespace

UserData::UserData()
{
    resetDefaults();
}

UserData &UserData::instance()
{
    static UserData data;
    return data;
}

void UserData::resetDefaults()
{
    m_settings = Settings{};
    m_profiles.clear();
    m_profiles.push_back(Profile{});
    m_active = 0;
}

bool UserData::load()
{
    const fs::path file = dataFile();
    std::error_code ec;
    fs::path source = file;
    if (!fs::exists(file, ec) && fs::exists(kLegacyFile, ec))
    {
        // Migration: the next save() writes the new location; the old file is left in place
        source = kLegacyFile;
    }

    std::ifstream in(source);
    if (!in)
    {
        return false;
    }

    try
    {
        const nlohmann::json j = nlohmann::json::parse(in);

        UserData loaded;
        if (j.contains("settings") && j["settings"].is_object())
        {
            const auto &s = j["settings"];
            loaded.m_settings.musicVolume = clampInt(s.value("musicVolume", 70), 0, 100);
            loaded.m_settings.sfxVolume = clampInt(s.value("sfxVolume", 80), 0, 100);
            loaded.m_settings.muted = s.value("muted", false);
            loaded.m_settings.accentIndex = clampInt(s.value("accentIndex", 0), 0, 3);
            loaded.m_settings.difficulty = clampInt(s.value("difficulty", 1), 0, 2);
            loaded.m_settings.screenShake = s.value("screenShake", true);
        }

        if (j.contains("profiles") && j["profiles"].is_array())
        {
            loaded.m_profiles.clear();
            for (const auto &p : j["profiles"])
            {
                if (loaded.m_profiles.size() >= kMaxProfiles || !p.is_object())
                {
                    continue;
                }
                Profile profile;
                profile.name = trim(p.value("name", std::string("Player"))).substr(0, kMaxNameLength);
                if (profile.name.empty())
                {
                    profile.name = "Player";
                }
                if (p.contains("best") && p["best"].is_array())
                {
                    const auto &best = p["best"];
                    if (!best.empty() && best[0].is_number())
                    {
                        // Old format: one score per difficulty, all for the first game
                        for (std::size_t i = 0; i < 3 && i < best.size(); ++i)
                        {
                            profile.best[0][i] = std::max(0, best[i].get<int>());
                        }
                    }
                    else
                    {
                        for (std::size_t g = 0; g < kGameCount && g < best.size(); ++g)
                        {
                            if (!best[g].is_array())
                            {
                                continue;
                            }
                            for (std::size_t i = 0; i < 3 && i < best[g].size(); ++i)
                            {
                                profile.best[g][i] = std::max(0, best[g][i].get<int>());
                            }
                        }
                    }
                }
                if (p.contains("plays") && p["plays"].is_array())
                {
                    for (std::size_t g = 0; g < kGameCount && g < p["plays"].size(); ++g)
                    {
                        profile.plays[g] = std::max(0, p["plays"][g].get<int>());
                    }
                }
                profile.achievements = p.value("achievements", 0u);
                profile.gamesPlayed = std::max(0, p.value("gamesPlayed", 0));
                profile.totalScore = std::max<long long>(0, p.value("totalScore", 0LL));
                loaded.m_profiles.push_back(profile);
            }
            if (loaded.m_profiles.empty())
            {
                loaded.m_profiles.push_back(Profile{});
            }
        }
        loaded.m_active = clampInt(j.value("active", 0), 0, static_cast<int>(loaded.m_profiles.size()) - 1);

        m_settings = loaded.m_settings;
        m_profiles = loaded.m_profiles;
        m_active = loaded.m_active;
        return true;
    }
    catch (const std::exception &)
    {
        // Damaged file: keep whatever we have (defaults on first run)
        return false;
    }
}

bool UserData::save() const
{
    nlohmann::json j;
    j["settings"] = {
        {"musicVolume", m_settings.musicVolume},
        {"sfxVolume", m_settings.sfxVolume},
        {"muted", m_settings.muted},
        {"accentIndex", m_settings.accentIndex},
        {"difficulty", m_settings.difficulty},
        {"screenShake", m_settings.screenShake}};
    j["active"] = m_active;
    j["profiles"] = nlohmann::json::array();
    for (const Profile &p : m_profiles)
    {
        j["profiles"].push_back({{"name", p.name},
                                 {"best", p.best},
                                 {"plays", p.plays},
                                 {"achievements", p.achievements},
                                 {"gamesPlayed", p.gamesPlayed},
                                 {"totalScore", p.totalScore}});
    }

    const fs::path file = dataFile();
    if (file.has_parent_path())
    {
        std::error_code ec;
        fs::create_directories(file.parent_path(), ec); // errors are ignored; the open below reports failure
    }

    std::ofstream out(file, std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << j.dump(2) << '\n';
    return static_cast<bool>(out);
}

bool UserData::addProfile(const std::string &rawName, std::string *error)
{
    auto fail = [error](const char *message)
    {
        if (error)
        {
            *error = message;
        }
        return false;
    };

    const std::string name = trim(rawName);
    if (name.empty())
    {
        return fail("Type a name first");
    }
    if (name.size() > kMaxNameLength)
    {
        return fail("Name is too long");
    }
    for (unsigned char c : name)
    {
        if (c < 32 || c > 126)
        {
            return fail("Use letters, numbers and symbols only");
        }
    }
    if (m_profiles.size() >= kMaxProfiles)
    {
        return fail("Profile list is full");
    }
    for (const Profile &p : m_profiles)
    {
        if (lower(p.name) == lower(name))
        {
            return fail("That name is taken");
        }
    }

    Profile profile;
    profile.name = name;
    m_profiles.push_back(profile);
    m_active = static_cast<int>(m_profiles.size()) - 1;
    save();
    return true;
}

bool UserData::deleteProfile(int index)
{
    if (m_profiles.size() <= 1 || index < 0 || index >= static_cast<int>(m_profiles.size()))
    {
        return false;
    }
    m_profiles.erase(m_profiles.begin() + index);
    if (m_active > index)
    {
        --m_active;
    }
    m_active = clampInt(m_active, 0, static_cast<int>(m_profiles.size()) - 1);
    save();
    return true;
}

void UserData::setActive(int index)
{
    if (index >= 0 && index < static_cast<int>(m_profiles.size()) && index != m_active)
    {
        m_active = index;
        save();
    }
}

int UserData::best(int game, int difficulty) const
{
    return active().best[static_cast<std::size_t>(clampInt(game, 0, kGameCount - 1))]
                        [static_cast<std::size_t>(clampInt(difficulty, 0, 2))];
}

void UserData::recordGame(int game, int difficulty, int score)
{
    Profile &p = m_profiles[static_cast<std::size_t>(m_active)];
    const auto g = static_cast<std::size_t>(clampInt(game, 0, kGameCount - 1));
    const auto d = static_cast<std::size_t>(clampInt(difficulty, 0, 2));
    p.best[g][d] = std::max(p.best[g][d], score);
    ++p.plays[g];
    ++p.gamesPlayed;
    p.totalScore += std::max(0, score);
    save();
}

bool UserData::unlockAchievement(int id)
{
    if (id < 0 || id >= Achievements::Count)
    {
        return false;
    }
    Profile &p = m_profiles[static_cast<std::size_t>(m_active)];
    const std::uint32_t bit = 1u << id;
    if (p.achievements & bit)
    {
        return false;
    }
    p.achievements |= bit;
    save();
    return true;
}

bool UserData::hasAchievement(int profile, int id) const
{
    if (profile < 0 || profile >= static_cast<int>(m_profiles.size()) || id < 0 || id >= Achievements::Count)
    {
        return false;
    }
    return (m_profiles[static_cast<std::size_t>(profile)].achievements & (1u << id)) != 0;
}

int UserData::unlockedCount(int profile) const
{
    int count = 0;
    for (int id = 0; id < Achievements::Count; ++id)
    {
        if (hasAchievement(profile, id))
        {
            ++count;
        }
    }
    return count;
}

int UserData::globalBest(int game, int difficulty, std::string *owner) const
{
    const auto g = static_cast<std::size_t>(clampInt(game, 0, kGameCount - 1));
    const auto d = static_cast<std::size_t>(clampInt(difficulty, 0, 2));
    int best = 0;
    if (owner)
    {
        owner->clear();
    }
    for (const Profile &p : m_profiles)
    {
        if (p.best[g][d] > best)
        {
            best = p.best[g][d];
            if (owner)
            {
                *owner = p.name;
            }
        }
    }
    return best;
}
