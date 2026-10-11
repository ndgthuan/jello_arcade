#pragma once
#include <SFML/Audio.hpp>
#include <array>
#include <optional>
#include <vector>

namespace Flappy
{
    // Sound effects and background music for Jello Bounce. There are no audio files: every sound is synthesized
    // into an sf::SoundBuffer when the object is created (a few hundred milliseconds of work at startup).
    class GameAudio
    {
    public:
        GameAudio();
        // The one shared audio engine (every game and the menu use it, so music can switch smoothly)
        static GameAudio &instance();
        GameAudio(const GameAudio &) = delete;
        GameAudio &operator=(const GameAudio &) = delete;

        void playFlap();
        void playScore(int streak); // pitch climbs a little with the streak
        void playCrash();
        void playNewBest();
        void playClick();

        // Effects for the other games
        void playShoot();                 // laser pew (3 voices round-robin, pitch varies a little)
        void playPop(float pitch = 1.0f); // brick / bubble pop
        void playHit();                   // thud: ball on paddle, player hurt
        void playPower();                 // rising arpeggio: pickup / power-up
        void playEat(float pitch = 1.0f); // fruit / coin blip
        void playBlast();                 // small explosion (enemy destroyed)

        // Starts the loop of track 0..5 (0 Bounce, 1 Smash, 2 Blaster, 3 Slither, 4 Stack, 5 menu) if it is not playing;
        // another track replaces the current one.
        void startMusic(int track = 0);
        void stopMusic();
        int currentTrack() const { return m_track; }
        void setMusicDucked(bool ducked); // quieter music under pause / game over screens

        // Volumes 0..100 (Settings) and the mute switch
        void setVolumes(int music, int sfx, bool muted);
        void toggleMute();
        bool isMuted() const { return m_muted; }

    private:
        sf::SoundBuffer m_flapBuffer, m_scoreBuffer, m_crashBuffer, m_bestBuffer, m_clickBuffer;
        sf::SoundBuffer m_shootBuffer, m_popBuffer, m_hitBuffer, m_powerBuffer, m_eatBuffer, m_blastBuffer;
        std::array<sf::SoundBuffer, 6> m_musicBuffers;
        std::array<bool, 6> m_musicBuilt{};
        int m_track = 0;
        std::optional<sf::Sound> m_flap, m_score, m_crash, m_best, m_click, m_music, m_hit, m_power, m_eat;
        std::vector<sf::Sound> m_shootPool, m_popPool, m_blastPool;
        std::size_t m_shootNext = 0, m_popNext = 0, m_blastNext = 0;
        int m_musicVolume = 70;
        int m_sfxVolume = 80;
        bool m_muted = false;
        bool m_ducked = false;

        void applyMusicVolume();
        void playPooled(std::vector<sf::Sound> &pool, std::size_t &next, float volume, float pitch);
        void playEffect(std::optional<sf::Sound> &sound, float volume, float pitch = 1.0f);
    };
} // namespace Flappy
