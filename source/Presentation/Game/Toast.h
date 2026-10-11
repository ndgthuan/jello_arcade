#pragma once
#include "FlappyCommon.h"
#include "GameAudio.h"
#include <deque>
#include <string>

namespace Flappy
{
    // A banner that slides up from the bottom of the screen ("ACHIEVEMENT UNLOCKED - Pipe Dreamer"), queued so
    // several unlocks in the same moment show one after another. Logical 1280x720 coordinates.
    class Toast
    {
    public:
        explicit Toast(const sf::Font &font) : m_font(&font) {}

        void show(const std::string &heading, const std::string &name);
        void update(float dt);
        void draw(sf::RenderTarget &target) const;
        bool active() const { return m_showing; }

    private:
        struct Item
        {
            std::string heading;
            std::string name;
        };
        const sf::Font *m_font;
        std::deque<Item> m_queue;
        Item m_current;
        bool m_showing = false;
        float m_age = 0.0f;
    };

    // Trophy bookkeeping shared by every game.
    namespace Achieve
    {
        // Unlocks `id` for the active profile; the first time it shows a toast and plays the pickup sound.
        void unlock(int id, Toast &toast, GameAudio &audio);
        // Call whenever the score changes: score-tier trophies of `game` and the Hard Hero trophy.
        void onScore(int game, int score, int difficulty, Toast &toast, GameAudio &audio);
        // Call after UserData::recordGame: Explorer and Veteran.
        void onRunEnd(Toast &toast, GameAudio &audio);
    } // namespace Achieve
} // namespace Flappy
