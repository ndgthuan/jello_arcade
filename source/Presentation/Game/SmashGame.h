#pragma once
#include "ArcadeGame.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Flappy
{
    // Jello Smash: a breakout game. A jello ball bounces between the neon walls, the paddle and a candy wall of
    // bricks. Bombs chain-explode their neighbours, combos score more, and broken bricks sometimes drop capsules
    // (W wide paddle, M multiball, S slow balls, L extra life).
    class SmashGame : public ArcadeGame
    {
    public:
        explicit SmashGame(const sf::Font &font);

    protected:
        void onStart() override;
        void onUpdate(float dt) override;
        void onIdle(float dt) override;
        void drawBackground(sf::RenderTarget &target) const override;
        void drawWorld(sf::RenderTarget &target) const override;

        const char *readyTitle() const override { return "GET READY!"; }
        const char *readyKeyLabel() const override { return "SPACE"; }
        const char *readyActionText() const override { return "to launch!"; }

        void onBegin() override;
        void onKeyPressed(sf::Keyboard::Key key) override;
        void onMouseMoved(sf::Vector2f point) override;
        void onMousePressed(sf::Vector2f point, sf::Mouse::Button button) override;

    private:
        struct Brick
        {
            sf::FloatRect rect;
            sf::Color color = sf::Color::White;
            int hp = 1;         // hits left
            int maxHp = 1;      // 2 on the top rows from level 2; the first hit cracks it
            bool alive = false;
            bool bomb = false;  // red bomb: blasts the bricks around it when destroyed
            float flash = 0.0f; // 1 right after a hit, decays (brightness)
        };

        struct Ball
        {
            sf::Vector2f pos;
            sf::Vector2f vel;
            bool attached = true;                // rides on the paddle until launched
            std::array<sf::Vector2f, 8> trail{}; // 8 previous positions, [0] = newest
        };

        struct Capsule
        {
            sf::Vector2f pos;
            char letter = 'W';
            float rot = 0.0f; // spin angle; the capsule width follows cos(rot)
        };

        struct Orb
        {
            sf::Vector2f pos;
            sf::Vector2f vel;
            float radius = 60.0f;
            sf::Color color = sf::Color::White;
            std::uint8_t alpha = 60;
        };

        void animate(float dt);
        void placeAttachedBalls();
        void attachBall();
        void launchBalls();
        void buildLevel();
        void advanceBall(Ball &ball, float h);
        void bounceFx(sf::Vector2f at);
        void collidePaddle(Ball &ball);
        void collideBricks(Ball &ball);
        void limitSpeed(Ball &ball) const;
        void breakBrick(std::size_t index);
        void spawnCapsule(sf::Vector2f pos);
        void updateCapsules(float dt);
        void applyPower(char letter, sf::Vector2f pos);
        void multiball();
        void loseLife();
        void nextLevel();
        void syncLives();
        float currentSpeed() const;
        float clampPaddle(float x) const;
        int aliveBricks() const;
        float rnd();

        void drawPaddle(sf::RenderTarget &target) const;
        void drawBalls(sf::RenderTarget &target) const;
        void drawCapsules(sf::RenderTarget &target) const;
        void drawBricks(sf::RenderTarget &target) const;

        std::vector<Brick> m_bricks;
        std::vector<Ball> m_balls;
        std::vector<Capsule> m_capsules;
        std::vector<Orb> m_orbs;

        float m_paddleX = 640.0f;
        float m_paddleW = 150.0f;
        float m_paddleSquash = 0.0f; // 1 right after a paddle hit, decays
        float m_wideTimer = 0.0f;
        float m_slowTimer = 0.0f;
        float m_speedMul = 1.0f; // eases to 0.6 while the slow power-up runs
        float m_flash = 0.0f;    // neon frame flash, set on every brick break
        float m_levelBanner = 0.0f;
        float m_gridOffset = 0.0f;
        int m_level = 1;
        int m_lives = 3;
        int m_combo = 0;
        int m_launchSign = 1; // alternates the launch tilt left / right
        std::uint32_t m_seed = 2024u;
    };
} // namespace Flappy
