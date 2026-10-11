#pragma once
#include "ArcadeGame.h"
#include <cstdint>
#include <deque>
#include <vector>

namespace Flappy
{
    // Jello Slither: a snake on a 32x16 grid. Glossy jello beads, fruit (every 5th one brings a bonus star) and
    // crystal blocks from length 12 on. Easy wraps around the edges, normal and hard have deadly walls.
    class SlitherGame : public ArcadeGame
    {
    public:
        explicit SlitherGame(const sf::Font &font);

    protected:
        void onStart() override;
        void onBegin() override;
        void onUpdate(float dt) override;
        void onIdle(float dt) override;
        void onKeyPressed(sf::Keyboard::Key key) override;
        void drawBackground(sf::RenderTarget &target) const override;
        void drawWorld(sf::RenderTarget &target) const override;

        const char *readyTitle() const override { return "GET READY!"; }
        const char *readyKeyLabel() const override { return "ARROWS / WASD"; }
        const char *readyActionText() const override { return "to slither!"; }
        bool isStartKey(sf::Keyboard::Key key) const override;

    private:
        struct Block
        {
            sf::Vector2i cell;
            sf::Color color;
        };

        // Logic
        void step();
        void eatFruit();
        void eatStar();
        void die();
        void spawnFood();
        void spawnStar();
        float stepInterval() const;
        bool isFree(sf::Vector2i cell) const;
        sf::Vector2i freeCell(bool farFromHead);
        float rnd();     // 0..1
        int rndInt(int n); // 0..n-1

        // Drawing
        void drawBlocks(sf::RenderTarget &target) const;
        void drawFruit(sf::RenderTarget &target) const;
        void drawStar(sf::RenderTarget &target) const;
        void drawSnake(sf::RenderTarget &target) const;

        // Snake: body[0] is the head. prev holds the body before the last step (for smooth motion).
        std::deque<sf::Vector2i> m_body;
        std::deque<sf::Vector2i> m_prev;
        std::deque<sf::Vector2i> m_turns; // pending turns, at most 2
        sf::Vector2i m_dir{1, 0};
        float m_stepTimer = 0.0f;
        float m_stepInterval = 0.15f;
        bool m_running = false; // true once the run has begun (left the Ready screen)

        // Death chain: segments pop from the head to the tail on the Game over screen
        bool m_dying = false;
        std::size_t m_popNext = 0;
        float m_popTimer = 0.0f;

        // Food, bonus star, obstacles
        sf::Vector2i m_food{0, 0};
        sf::Color m_fruitColor{255, 84, 96};
        int m_foodEaten = 0;
        bool m_starActive = false;
        sf::Vector2i m_star{0, 0};
        float m_starTimer = 0.0f;
        std::vector<Block> m_blocks;

        // Score / combo / feedback
        int m_combo = 0;
        float m_lastEatTime = -100.0f;
        float m_pulse = 0.0f; // decaying 0..1 kick on every food, drives the border glow
        std::uint32_t m_seed = 2463534242u;
    };
} // namespace Flappy
