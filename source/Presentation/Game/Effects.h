#pragma once
#include "FlappyCommon.h"
#include <string>
#include <vector>

namespace Flappy
{
    // Particles and one-shot visual effects. Everything is deterministic enough to look good, but uses a
    // small internal random generator (no <random> state shared with the rest of the game).
    class Effects
    {
    public:
        Effects(const sf::Font &font, const Accent &accent);

        void setAccent(const Accent &accent) { m_accent = accent; }
        void clear();
        void update(float dt);

        // --- spawners ---
        // Small yellow/orange star burst + a few sparks, behind the blob when it flaps.
        void spawnFlapBurst(sf::Vector2f pos);
        // Cyan shockwave rings + sparkles + a floating "+N" popup (3D extruded orange text, rises and fades).
        void spawnScore(sf::Vector2f gapCenter, int amount);
        // Small trailing sparkle (call every frame while playing; internally rate limited).
        void spawnTrail(sf::Vector2f pos, float dt);
        // Big crash: 3-layer starburst (red/orange, yellow, white), 3 expanding rings, radial spark lines, jello
        // chunks in the accent color flying out with gravity, smoke puffs. Also adds screen shake.
        void spawnExplosion(sf::Vector2f pos);
        // Medium explosion for enemies / bombs: layered starburst in `tint`, a ring, sparks, shards in the tint
        // colour and a puff of smoke. scale 1 = about 220 px across; also a small screen shake.
        void spawnBlast(sf::Vector2f pos, float scale = 1.0f, sf::Color tint = sf::Color(255, 154, 43));
        // Small shatter (a brick, a fruit, a pickup): `count` shards and sparkles in `color` plus a tiny ring.
        // power scales speeds and sizes (1 = normal).
        void spawnBurst(sf::Vector2f pos, sf::Color color, int count = 14, float power = 1.0f);
        // A floating label (3D extruded orange text, rises and fades), e.g. "COMBO x3" or "+50".
        void spawnText(sf::Vector2f pos, const std::string &text);
        // Confetti rain from the top (for a new best).
        void spawnConfetti();

        void addShake(float intensity) { m_shake = std::max(m_shake, intensity); }
        // Current camera offset in logical pixels (decays automatically), add it to the view center.
        sf::Vector2f shakeOffset() const;

        bool explosionActive() const { return m_explosionTime >= 0.0f && m_explosionTime < 1.2f; }

        // Effects that go behind the blob (explosion burst, rings, trail, popups behind).
        void drawBack(sf::RenderTarget &target) const;
        // Effects in front (sparks, chunks, smoke, confetti, popups).
        void drawFront(sf::RenderTarget &target) const;

    private:
        enum class Kind
        {
            Spark,   // small glowing dot / short line
            Sparkle, // 4-point star
            Chunk,   // jello fragment (shaded ellipse), affected by gravity
            Smoke,   // big soft dark puff, grows and fades
            Confetti // small rotating colored rectangle
        };
        struct Particle
        {
            Kind kind = Kind::Spark;
            sf::Vector2f pos, vel;
            float life = 0.0f, maxLife = 1.0f;
            float size = 4.0f;
            float rotation = 0.0f, spin = 0.0f;
            float gravity = 0.0f;
            sf::Color color = sf::Color::White;
        };
        struct Ring
        {
            sf::Vector2f pos;
            float age = 0.0f, life = 0.6f;
            float startRadius = 10.0f, endRadius = 150.0f;
            float thickness = 6.0f;
            sf::Color color = sf::Color::White;
        };
        struct Popup
        {
            std::string text;
            sf::Vector2f pos;
            float age = 0.0f, life = 0.9f;
        };
        struct Burst
        {
            sf::Vector2f pos;
            float age = 0.0f, life = 0.35f;
            float radius = 70.0f;
            sf::Color inner = sf::Color(255, 240, 140);
            sf::Color outer = sf::Color(255, 154, 43);
        };

        const sf::Font *m_font;
        Accent m_accent;
        std::vector<Particle> m_particles;
        std::vector<Ring> m_rings;
        std::vector<Popup> m_popups;
        std::vector<Burst> m_bursts;
        unsigned int m_rng = 12345u;
        float m_shake = 0.0f;
        float m_time = 0.0f;
        float m_trailTimer = 0.0f;
        float m_explosionTime = -1.0f; // seconds since the explosion started (<0 = none)
        sf::Vector2f m_explosionPos{0.0f, 0.0f};

        float rnd();                       // 0..1
        float rnd(float lo, float hi);     // lo..hi
    };
} // namespace Flappy
