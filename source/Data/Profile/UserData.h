#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// Everything the player can change and everything the game remembers: settings and local player profiles.
// Stored as JSON in %APPDATA%/Jello/userdata.json (the working directory is used when APPDATA is not set).
// All mutating calls save immediately.
struct Settings
{
    int musicVolume = 70;  // 0..100
    int sfxVolume = 80;    // 0..100
    bool muted = false;
    int accentIndex = 0;   // 0..3, see Flappy::accentChoice
    int difficulty = 1;    // 0 easy, 1 normal, 2 hard
    bool screenShake = true;
};

// The games in the menu: 0 Jello Bounce, 1 Jello Smash, 2 Jello Blaster, 3 Jello Slither, 4 Jello Stack
inline constexpr int kGameCount = 5;

struct Profile
{
    std::string name = "Player";
    std::array<std::array<int, 3>, kGameCount> best{}; // best score per game per difficulty
    int gamesPlayed = 0;
    long long totalScore = 0;
    std::array<int, kGameCount> plays{}; // runs per game
    std::uint32_t achievements = 0;     // bit mask of Achievements::Id
};

class UserData
{
public:
    static constexpr std::size_t kMaxProfiles = 6;
    static constexpr std::size_t kMaxNameLength = 12;

    static UserData &instance();

    // Reads userdata.json if it exists (a missing or damaged file leaves the defaults). Returns true when loaded.
    bool load();
    bool save() const;

    Settings &settings() { return m_settings; }
    const Settings &settings() const { return m_settings; }

    const std::vector<Profile> &profiles() const { return m_profiles; }
    int activeIndex() const { return m_active; }
    const Profile &active() const { return m_profiles[static_cast<std::size_t>(m_active)]; }

    // Trims the name; accepts 1..kMaxNameLength printable ASCII characters, unique ignoring case, while there is
    // room. On success the new profile becomes the active one. On failure `error` (if given) says why.
    bool addProfile(const std::string &name, std::string *error = nullptr);
    // Removes a profile. The last remaining profile cannot be removed (returns false).
    bool deleteProfile(int index);
    void setActive(int index);

    // Best score of the active profile for a game (0..3) and difficulty (0..2).
    int best(int game, int difficulty) const;
    // Call when a run ends: updates best score, games played and total score of the active profile, then saves.
    void recordGame(int game, int difficulty, int score);

    // Trophies of the ACTIVE profile. unlockAchievement returns true only the first time (and saves).
    bool unlockAchievement(int id);
    bool hasAchievement(int profile, int id) const;
    int unlockedCount(int profile) const;
    // Best score of a game over every profile (and the name of the player who owns it, "" when nobody scored)
    int globalBest(int game, int difficulty, std::string *owner = nullptr) const;

    // Persist after changing settings() directly.
    void commitSettings() { save(); }

private:
    UserData();
    void resetDefaults();

    Settings m_settings;
    std::vector<Profile> m_profiles;
    int m_active = 0;
};
