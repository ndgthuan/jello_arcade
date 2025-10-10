#include "Audio.h"
#include "../Logging/Logger.h"
#include <iostream>

AudioManager &AudioManager::getInstance()
{
    static AudioManager instance;
    return instance;
}

void AudioManager::loadSound(const std::string &name, const std::string &filePath)
{
    try
    {
        auto buffer = std::make_unique<sf::SoundBuffer>();
        if (buffer->loadFromFile(filePath))
        {
            m_soundBuffers[name] = std::move(buffer);

            // Create sound object for this buffer
            auto sound = std::make_unique<sf::Sound>(*m_soundBuffers[name]);
            sound->setVolume(m_soundVolume);
            m_sounds[name] = std::move(sound);

            Logger::getInstance().log(Logger::Level::INFO, "Audio loaded: " + name + " from " + filePath);
        }
        else
        {
            Logger::getInstance().log(Logger::Level::ERROR, "Failed to load audio: " + filePath);
        }
    }
    catch (const std::exception &e)
    {
        Logger::getInstance().log(Logger::Level::ERROR, "Exception loading audio " + name + ": " + e.what());
    }
}

void AudioManager::loadMusic(const std::string &name, const std::string &filePath)
{
    try
    {
        auto music = std::make_unique<sf::Music>();
        if (music->openFromFile(filePath))
        {
            music->setVolume(m_musicVolume);
            m_music[name] = std::move(music);

            Logger::getInstance().log(Logger::Level::INFO, "Music loaded: " + name + " from " + filePath);
        }
        else
        {
            Logger::getInstance().log(Logger::Level::ERROR, "Failed to load music: " + filePath);
        }
    }
    catch (const std::exception &e)
    {
        Logger::getInstance().log(Logger::Level::ERROR, "Exception loading music " + name + ": " + e.what());
    }
}

void AudioManager::playSound(const std::string &name)
{
    if (m_sounds.find(name) != m_sounds.end())
    {
        auto &soundRef = *m_sounds[name];

        // Force restart if already playing
        if (soundRef.getStatus() == sf::Sound::Status::Playing)
        {
            soundRef.stop();
        }
        soundRef.setPlayingOffset(sf::Time::Zero);

        // Set volume before playing
        soundRef.setVolume(m_soundVolume);
        soundRef.play();
    }
    else
    {
        Logger::getInstance().log(Logger::Level::WARNING, "Sound not found: " + name);
    }
}

void AudioManager::playMusic(const std::string &name, bool loop)
{
    if (m_music.find(name) != m_music.end())
    {
        m_music[name]->setLooping(loop);
        m_music[name]->play();
    }
    else
    {
        Logger::getInstance().log(Logger::Level::WARNING, "Music not found: " + name);
    }
}

void AudioManager::stopMusic(const std::string &name)
{
    if (m_music.find(name) != m_music.end())
    {
        m_music[name]->stop();
    }
}

void AudioManager::setSoundVolume(float volume)
{
    m_soundVolume = std::max(0.0f, std::min(100.0f, volume));

    // Update all existing sounds
    for (auto &pair : m_sounds)
    {
        pair.second->setVolume(m_soundVolume);
    }
}

void AudioManager::setMusicVolume(float volume)
{
    m_musicVolume = std::max(0.0f, std::min(100.0f, volume));

    // Update all existing music
    for (auto &pair : m_music)
    {
        pair.second->setVolume(m_musicVolume);
    }
}

bool AudioManager::hasSound(const std::string &name) const
{
    return m_sounds.find(name) != m_sounds.end();
}

bool AudioManager::hasMusic(const std::string &name) const
{
    return m_music.find(name) != m_music.end();
}
