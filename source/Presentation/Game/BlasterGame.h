#pragma once
#include "ArcadeGame.h"
#include <cstdint>
#include <vector>

namespace Flappy
{
    // Jello Blaster: a side-scrolling space shooter. The ship follows the mouse (or flies with WASD / arrows)
    // and fires while Space or the left button is held. Asteroids split, drones shoot back, bombers are armoured
    // and every 40 kills a mothership boss arrives. Enemies drop power-ups: rapid fire, triple shot, shield, heart.
    class BlasterGame : public ArcadeGame
    {
    public:
        explicit BlasterGame(const sf::Font &font);

    protected:
        void onStart() override;
        void onUpdate(float dt) override;
        void onIdle(float dt) override;
        void drawBackground(sf::RenderTarget &target) const override;
        void drawWorld(sf::RenderTarget &target) const override;

        const char *readyTitle() const override { return "GET READY!"; }
        const char *readyKeyLabel() const override { return "MOUSE / WASD"; }
        const char *readyActionText() const override { return "to fly - SPACE to fire!"; }
        bool isStartKey(sf::Keyboard::Key key) const override;
        bool startsOnClick() const override { return true; }

        void onMouseMoved(sf::Vector2f point) override;
        void onMousePressed(sf::Vector2f point, sf::Mouse::Button button) override;
        void onMouseReleased(sf::Vector2f point, sf::Mouse::Button button) override;

    private:
        enum class Kind
        {
            Asteroid,
            Drone,
            Bomber,
            Mothership
        };

        struct Enemy
        {
            Kind kind = Kind::Asteroid;
            sf::Vector2f pos{0.0f, 0.0f};
            sf::Vector2f vel{0.0f, 0.0f};
            float radius = 20.0f;
            int hp = 1;
            int maxHp = 1;
            int size = 0;          // asteroid size class: 0 small, 1 medium, 2 large
            float rot = 0.0f;      // degrees
            float spin = 0.0f;     // degrees per second
            float baseY = 0.0f;    // centre line of the sine motion
            float phase = 0.0f;
            float age = 0.0f;
            float shootTimer = 0.0f;
            float flash = 0.0f;    // white hit flash, seconds left
            bool alive = true;
            std::vector<sf::Vector2f> shape; // asteroid outline in local coordinates
        };

        struct Shot
        {
            sf::Vector2f pos{0.0f, 0.0f};
            sf::Vector2f vel{0.0f, 0.0f};
            float life = 1.8f;
            bool alive = true;
        };

        struct EnemyShot
        {
            sf::Vector2f pos{0.0f, 0.0f};
            sf::Vector2f vel{0.0f, 0.0f};
            float life = 6.0f;
            sf::Color color = sf::Color::White;
            bool alive = true;
        };

        struct Pickup
        {
            sf::Vector2f pos{0.0f, 0.0f};
            char letter = 'R'; // R rapid fire, T triple shot, S shield, H heart
            float rot = 0.0f;
            float age = 0.0f;
            bool alive = true;
        };

        struct Star
        {
            sf::Vector2f pos{0.0f, 0.0f};
            float radius = 1.0f;
            float phase = 0.0f;
            int layer = 0; // 0 far, 1 middle, 2 near
        };

        // ship
        sf::Vector2f m_ship{220.0f, 420.0f};
        sf::Vector2f m_mouseTarget{220.0f, 420.0f};
        bool m_mouseActive = false; // the ship chases the pointer (false while the keys fly it)
        bool m_mouseFire = false;   // left button held
        bool m_shipAlive = true;
        float m_tilt = 0.0f;        // degrees, nose down when positive
        float m_prevY = 420.0f;
        float m_throttle = 0.5f;    // engine flame strength 0..1
        float m_fireTimer = 0.0f;

        // health and power-ups
        int m_hp = 3;
        float m_invuln = 0.0f;
        float m_flash = 0.0f; // red screen flash after a hit
        float m_rapid = 0.0f;
        float m_triple = 0.0f;
        bool m_shield = false;

        // progress
        int m_kills = 0;
        int m_nextBoss = 40;
        bool m_bossAlive = false;
        int m_chain = 0;
        float m_chainTimer = 0.0f;
        float m_spawnTimer = 1.0f;

        // backdrop
        float m_scroll = 0.0f;
        float m_scrollSpeed = 40.0f;

        // world
        std::uint32_t m_rng = 4242u;
        std::vector<Enemy> m_enemies;
        std::vector<Enemy> m_spawned; // created during this frame, added after the loops
        std::vector<Shot> m_shots;
        std::vector<EnemyShot> m_enemyShots;
        std::vector<Pickup> m_pickups;
        std::vector<Star> m_stars;

        float rnd();                   // 0..1
        float rnd(float lo, float hi); // lo..hi
        void scrollBackdrop(float dt, float targetSpeed);

        void updateShip(float dt);
        void fireVolley();
        void spawnWave(float dt);
        void spawnBoss();
        Enemy makeAsteroid(int size, sf::Vector2f pos, sf::Vector2f vel);
        void fireAimed(sf::Vector2f from, float speed, float spreadDeg, int count, sf::Color color);
        void updateEnemies(float dt);
        void updateShots(float dt);
        void updateEnemyShots(float dt);
        void updateContacts();
        void updatePickups(float dt);
        void destroyEnemy(Enemy &e);
        void registerKill(sf::Vector2f pos, int points);
        void dropPickup(sf::Vector2f pos);
        void collectPickup(const Pickup &p);
        void hurt();
        sf::Color tintFor(const Enemy &e) const;

        void drawShip(sf::RenderTarget &target) const;
        void drawShield(sf::RenderTarget &target) const;
        void drawAsteroid(sf::RenderTarget &target, const Enemy &e) const;
        void drawDrone(sf::RenderTarget &target, const Enemy &e) const;
        void drawBomber(sf::RenderTarget &target, const Enemy &e) const;
        void drawMothership(sf::RenderTarget &target, const Enemy &e) const;
        void drawBossBar(sf::RenderTarget &target) const;
        void drawShots(sf::RenderTarget &target) const;
        void drawEnemyShots(sf::RenderTarget &target) const;
        void drawPickups(sf::RenderTarget &target) const;
    };
} // namespace Flappy
