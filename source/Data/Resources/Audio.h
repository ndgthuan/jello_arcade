#pragma once

#include <SFML/Audio.hpp>
#include <memory>
#include <unordered_map>
#include <string>

/**
 * Audio Manager for handling sound effects and music
 * Follows Singleton pattern for centralized audio management
 * Manages loading, caching, and playback of audio resources
 */
class AudioManager
{
public:
    // Get singleton instance
    static AudioManager &getInstance();

    // Load audio resources
    void loadSound(const std::string &name, const std::string &filePath);
    void loadMusic(const std::string &name, const std::string &filePath);

    // Play audio
    void playSound(const std::string &name);
    void playMusic(const std::string &name, bool loop = true);
    void stopMusic(const std::string &name);

    // Audio control
    void setSoundVolume(float volume);
    void setMusicVolume(float volume);
    float getSoundVolume() const { return m_soundVolume; }
    float getMusicVolume() const { return m_musicVolume; }

    // Check if audio exists
    bool hasSound(const std::string &name) const;
    bool hasMusic(const std::string &name) const;

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    // Audio storage
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> m_soundBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> m_sounds;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> m_music;

    // Volume settings
    float m_soundVolume = 60.0f; // Tăng volume lên 200% để test
    float m_musicVolume = 30.0f;
};
