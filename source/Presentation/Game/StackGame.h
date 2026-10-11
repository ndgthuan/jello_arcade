#pragma once
#include "ArcadeGame.h"
#include <cstdint>
#include <vector>

namespace Flappy
{
    // Jello Stack: glossy slabs slide above the tower and are dropped with one button. The overhang is cut off and
    // falls away; a perfect drop snaps the slab on, builds a combo and widens the tower again. The camera follows
    // the top of the tower up into a starry sky.
    class StackGame : public ArcadeGame
    {
    public:
        explicit StackGame(const sf::Font &font);

    protected:
        void onStart() override;
        void onUpdate(float dt) override;
        void onIdle(float dt) override;
        void onKeyPressed(sf::Keyboard::Key key) override;
        void onMousePressed(sf::Vector2f point, sf::Mouse::Button button) override;
        void drawBackground(sf::RenderTarget &target) const override;
        void drawWorld(sf::RenderTarget &target) const override;

        const char *readyTitle() const override { return "GET READY!"; }
        const char *readyKeyLabel() const override { return "SPACE / CLICK"; }
        const char *readyActionText() const override { return "to drop!"; }
        bool isStartKey(sf::Keyboard::Key key) const override;
        bool startsOnClick() const override { return true; }

    private:
        // A placed slab; y is its world top (the camera offset is added when drawing)
        struct Slab
        {
            float cx = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float age = 2.0f; // seconds since it landed (drives the landing tween and the squash)
        };

        // A piece cut off an overhang, or a whole missed slab, falling with gravity
        struct Shard
        {
            float cx = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float vx = 0.0f;
            float vy = 0.0f;
            float rot = 0.0f; // degrees
            float spin = 0.0f;
            float age = 0.0f;
            sf::Color color = sf::Color::White;
        };

        // Expanding white frame around a perfect drop
        struct Ring
        {
            float cx = 0.0f;
            float y = 0.0f;
            float w = 0.0f;
            float age = 0.0f;
        };

        void drop();
        void animate(float dt);
        void spawnShard(float cx, float y, float w, sf::Color color, float vx, float spin);
        float rnd();             // 0..1
        float rnd(float lo, float hi);

        std::vector<Slab> m_tower; // [0] is the base, the last one is the tower top
        std::vector<Shard> m_shards;
        std::vector<Ring> m_rings;
        float m_moveX = 140.0f; // centre of the sliding slab
        float m_moveDir = 1.0f;
        bool m_moving = true;   // false after a miss
        float m_cameraY = 0.0f;
        int m_combo = 0;
        std::uint32_t m_seed = 7919u;
    };
} // namespace Flappy
