#include "Effects.h"
#include "../../Data/Profile/UserData.h"
#include "UiKit.h"
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <utility>

namespace Flappy
{
    namespace
    {
        constexpr std::size_t kMaxParticles = 800;
        constexpr float kPi = std::numbers::pi_v<float>;

        constexpr sf::Color kConfettiColors[] = {
            sf::Color(255, 90, 143), sf::Color(255, 210, 63), sf::Color(79, 216, 255),
            sf::Color(124, 255, 91), sf::Color(255, 255, 255), sf::Color(255, 138, 43)};

        // Adds an item unless the list is already full
        template <typename T>
        void pushCapped(std::vector<T> &items, T item)
        {
            if (items.size() < kMaxParticles)
                items.push_back(std::move(item));
        }

        float clamp01(float v)
        {
            return std::clamp(v, 0.0f, 1.0f);
        }

        float lerpf(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        float easeOut(float t)
        {
            float u = 1.0f - clamp01(t);
            return 1.0f - u * u * u;
        }

        float easeOutBack(float t)
        {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            float u = t - 1.0f;
            return 1.0f + c3 * u * u * u + c1 * u * u;
        }

        // Multiplies the alpha of a color by f (0..1)
        sf::Color scaleAlpha(sf::Color c, float f)
        {
            return withAlpha(c, static_cast<std::uint8_t>(c.a * clamp01(f) + 0.5f));
        }

        std::uint8_t toByte(float a)
        {
            return static_cast<std::uint8_t>(clamp01(a) * 255.0f + 0.5f);
        }

        sf::Vector2f rotateVec(sf::Vector2f v, float deg)
        {
            float r = deg * kPi / 180.0f;
            float c = std::cos(r);
            float s = std::sin(r);
            return sf::Vector2f{v.x * c - v.y * s, v.x * s + v.y * c};
        }
    } // namespace

    Effects::Effects(const sf::Font &font, const Accent &accent)
        : m_font(&font), m_accent(accent)
    {
    }

    void Effects::clear()
    {
        m_particles.clear();
        m_rings.clear();
        m_popups.clear();
        m_bursts.clear();
        m_shake = 0.0f;
        m_trailTimer = 0.0f;
        m_explosionTime = -1.0f;
    }

    void Effects::update(float dt)
    {
        m_time += dt;

        for (Particle &p : m_particles)
        {
            p.pos += p.vel * dt;
            p.vel.y += p.gravity * dt;
            p.rotation += p.spin * dt;
            p.life += dt;

            float drag = 0.0f;
            switch (p.kind)
            {
            case Kind::Spark:
                drag = 1.8f;
                break;
            case Kind::Sparkle:
                drag = 2.5f;
                break;
            case Kind::Smoke:
                drag = 1.2f;
                p.size += p.size * 0.35f * dt; // smoke keeps spreading
                break;
            default:
                break;
            }
            if (drag > 0.0f)
                p.vel = p.vel * std::exp(-drag * dt);
        }
        std::erase_if(m_particles, [](const Particle &p)
                      { return p.life >= p.maxLife; });

        for (Ring &r : m_rings)
            r.age += dt;
        std::erase_if(m_rings, [](const Ring &r)
                      { return r.age >= r.life; });

        for (Popup &p : m_popups)
        {
            p.age += dt;
            p.pos.y -= 60.0f * dt;
        }
        std::erase_if(m_popups, [](const Popup &p)
                      { return p.age >= p.life; });

        for (Burst &b : m_bursts)
            b.age += dt;
        std::erase_if(m_bursts, [](const Burst &b)
                      { return b.age >= b.life; });

        m_shake *= std::exp(-6.0f * dt);
        if (m_shake < 0.2f)
            m_shake = 0.0f;

        if (m_explosionTime >= 0.0f)
        {
            m_explosionTime += dt;
            if (m_explosionTime > 2.0f) // the core is invisible after 1.2 s, stop counting
                m_explosionTime = -1.0f;
        }
    }

    sf::Vector2f Effects::shakeOffset() const
    {
        if (!UserData::instance().settings().screenShake)
        {
            return sf::Vector2f{0.0f, 0.0f};
        }
        // Different frequencies per axis, so the shake looks random without using the rng
        return sf::Vector2f{m_shake * std::sin(m_time * 60.0f), m_shake * std::cos(m_time * 47.0f)};
    }

    float Effects::rnd()
    {
        m_rng = m_rng * 1664525u + 1013904223u;
        return static_cast<float>(m_rng >> 8) * (1.0f / 16777216.0f);
    }

    float Effects::rnd(float lo, float hi)
    {
        return lo + (hi - lo) * rnd();
    }

    void Effects::spawnFlapBurst(sf::Vector2f pos)
    {
        Burst b;
        b.pos = pos + sf::Vector2f{-20.0f, 30.0f};
        b.age = 0.0f;
        b.life = 0.35f;
        b.radius = 70.0f;
        m_bursts.push_back(b);

        for (int i = 0; i < 5; ++i)
        {
            Particle p;
            p.kind = Kind::Spark;
            p.pos = b.pos;
            p.vel = sf::Vector2f{rnd(-420.0f, -180.0f), rnd(40.0f, 260.0f)}; // down-left
            p.gravity = 300.0f;
            p.maxLife = rnd(0.3f, 0.5f);
            p.size = rnd(2.0f, 3.0f);
            p.color = rnd() < 0.5f ? sf::Color(255, 210, 74) : sf::Color(255, 154, 43);
            pushCapped(m_particles, p);
        }
    }

    void Effects::spawnScore(sf::Vector2f gapCenter, int amount)
    {
        const sf::Color cyan(127, 244, 255);
        m_rings.push_back(Ring{gapCenter, 0.0f, 0.6f, 40.0f, 150.0f, 6.0f, cyan});
        m_rings.push_back(Ring{gapCenter, 0.0f, 0.6f, 20.0f, 120.0f, 3.0f, withAlpha(cyan, 160)});

        for (int i = 0; i < 8; ++i)
        {
            float angle = static_cast<float>(i) * 2.0f * kPi / 8.0f + rnd(-0.3f, 0.3f);
            float dirX = std::cos(angle);
            float dirY = std::sin(angle);
            float dist = rnd(30.0f, 60.0f);

            Particle p;
            p.kind = Kind::Sparkle;
            p.pos = gapCenter + sf::Vector2f{dirX * dist, dirY * dist};
            p.vel = sf::Vector2f{dirX, dirY} * rnd(80.0f, 220.0f);
            p.maxLife = rnd(0.5f, 0.8f);
            p.size = rnd(8.0f, 14.0f);
            p.rotation = rnd(0.0f, 360.0f);
            p.spin = rnd(-240.0f, 240.0f);
            p.color = rnd() < 0.5f ? sf::Color(255, 236, 130) : sf::Color(255, 255, 255);
            pushCapped(m_particles, p);
        }

        m_popups.push_back(Popup{"+" + std::to_string(amount), gapCenter, 0.0f, 0.9f});
    }

    void Effects::spawnTrail(sf::Vector2f pos, float dt)
    {
        m_trailTimer += dt;
        if (m_trailTimer > 0.2f)
            m_trailTimer = 0.0f;
        if (m_trailTimer < 0.05f)
            return;
        m_trailTimer -= 0.05f;

        Particle p;
        p.kind = rnd() < 0.5f ? Kind::Sparkle : Kind::Spark;
        p.pos = sf::Vector2f{pos.x - 40.0f + rnd(-8.0f, 8.0f), pos.y + rnd(-12.0f, 12.0f)};
        p.vel = sf::Vector2f{rnd(-90.0f, -30.0f), rnd(-14.0f, 14.0f)}; // slow drift left
        p.maxLife = rnd(0.4f, 0.6f);
        p.size = p.kind == Kind::Sparkle ? rnd(5.0f, 9.0f) : rnd(2.0f, 3.0f);
        p.rotation = rnd(0.0f, 360.0f);
        p.spin = rnd(-180.0f, 180.0f);
        p.color = rnd() < 0.5f ? m_accent.base : sf::Color(255, 250, 200);
        pushCapped(m_particles, p);
    }

    void Effects::spawnText(sf::Vector2f pos, const std::string &text)
    {
        Popup popup;
        popup.text = text;
        popup.pos = pos;
        popup.life = 0.9f;
        m_popups.push_back(popup);
    }

    void Effects::spawnBlast(sf::Vector2f pos, float scale, sf::Color tint)
    {
        Burst b;
        b.pos = pos;
        b.life = 0.45f;
        b.radius = 110.0f * scale;
        b.inner = sf::Color(255, 246, 200);
        b.outer = tint;
        m_bursts.push_back(b);

        m_rings.push_back(Ring{pos, 0.0f, 0.5f, 20.0f * scale, 170.0f * scale, 6.0f, mix(tint, sf::Color::White, 0.5f)});

        const int sparks = 10 + static_cast<int>(10.0f * scale);
        for (int i = 0; i < sparks; ++i)
        {
            const float angle = rnd(0.0f, 2.0f * kPi);
            Particle p;
            p.kind = Kind::Spark;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle), std::sin(angle)} * rnd(250.0f, 650.0f) * scale;
            p.maxLife = rnd(0.3f, 0.7f);
            p.size = rnd(2.0f, 3.2f);
            p.color = mix(tint, sf::Color::White, rnd(0.3f, 1.0f));
            pushCapped(m_particles, p);
        }

        const int shards = 3 + static_cast<int>(4.0f * scale);
        for (int i = 0; i < shards; ++i)
        {
            const float angle = rnd(0.0f, 2.0f * kPi);
            Particle p;
            p.kind = Kind::Chunk;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle), std::sin(angle)} * rnd(150.0f, 420.0f) * scale;
            p.gravity = 900.0f;
            p.maxLife = rnd(0.7f, 1.1f);
            p.size = rnd(8.0f, 18.0f) * scale;
            p.rotation = rnd(0.0f, 360.0f);
            p.spin = rnd(-540.0f, 540.0f);
            p.color = tint;
            pushCapped(m_particles, p);
        }

        for (int i = 0; i < 2; ++i)
        {
            Particle p;
            p.kind = Kind::Smoke;
            p.pos = pos + sf::Vector2f{rnd(-15.0f, 15.0f), rnd(-10.0f, 10.0f)} * scale;
            p.vel = sf::Vector2f{rnd(-30.0f, 30.0f), rnd(-60.0f, -20.0f)};
            p.maxLife = 0.9f;
            p.size = rnd(30.0f, 50.0f) * scale;
            p.color = sf::Color(20, 6, 40, 170);
            pushCapped(m_particles, p);
        }

        addShake(4.0f * scale);
    }

    void Effects::spawnBurst(sf::Vector2f pos, sf::Color color, int count, float power)
    {
        m_rings.push_back(Ring{pos, 0.0f, 0.3f, 8.0f, 60.0f * power, 3.0f, mix(color, sf::Color::White, 0.5f)});

        for (int i = 0; i < count; ++i)
        {
            const float angle = rnd(0.0f, 2.0f * kPi);
            Particle p;
            p.kind = Kind::Chunk;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle), std::sin(angle)} * rnd(150.0f, 450.0f) * power;
            p.gravity = 1100.0f;
            p.maxLife = rnd(0.6f, 1.0f);
            p.size = rnd(4.0f, 9.0f) * power;
            p.rotation = rnd(0.0f, 360.0f);
            p.spin = rnd(-720.0f, 720.0f);
            p.color = color;
            pushCapped(m_particles, p);
        }
        for (int i = 0; i < count / 2; ++i)
        {
            const float angle = rnd(0.0f, 2.0f * kPi);
            Particle p;
            p.kind = Kind::Sparkle;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle), std::sin(angle)} * rnd(60.0f, 260.0f) * power;
            p.maxLife = rnd(0.4f, 0.7f);
            p.size = rnd(5.0f, 9.0f);
            p.color = mix(color, sf::Color::White, 0.6f);
            pushCapped(m_particles, p);
        }
    }

    void Effects::spawnExplosion(sf::Vector2f pos)
    {
        m_explosionTime = 0.0f;
        m_explosionPos = pos;

        m_rings.push_back(Ring{pos, 0.0f, 0.7f, 60.0f, 360.0f, 9.0f, sf::Color(255, 243, 176)});
        m_rings.push_back(Ring{pos, 0.0f, 0.9f, 40.0f, 520.0f, 5.0f, sf::Color(127, 244, 255)});
        m_rings.push_back(Ring{pos, 0.0f, 1.0f, 30.0f, 700.0f, 3.0f, sf::Color(255, 255, 255, 160)});

        // Radial spark lines
        const sf::Color hot[] = {sf::Color(255, 243, 176), sf::Color(255, 255, 255), sf::Color(255, 210, 74)};
        for (int i = 0; i < 22; ++i)
        {
            float angle = static_cast<float>(i) * 2.0f * kPi / 22.0f + rnd(-0.12f, 0.12f);
            float speed = rnd(400.0f, 900.0f);

            Particle p;
            p.kind = Kind::Spark;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle), std::sin(angle)} * speed;
            p.maxLife = rnd(0.5f, 0.9f);
            p.size = rnd(2.0f, 3.2f);
            p.color = hot[static_cast<int>(rnd() * 3.0f)];
            pushCapped(m_particles, p);
        }

        // Jello chunks
        int chunkCount = 8 + static_cast<int>(rnd(0.0f, 3.0f));
        for (int i = 0; i < chunkCount; ++i)
        {
            float angle = rnd(0.0f, 2.0f * kPi);
            float speed = rnd(250.0f, 650.0f);

            Particle p;
            p.kind = Kind::Chunk;
            p.pos = pos;
            p.vel = sf::Vector2f{std::cos(angle) * speed, std::sin(angle) * speed - 200.0f};
            p.gravity = 1500.0f;
            p.maxLife = rnd(1.4f, 1.8f);
            p.size = rnd(12.0f, 30.0f);
            p.rotation = rnd(0.0f, 360.0f);
            p.spin = rnd(-720.0f, 720.0f);
            p.color = m_accent.base;
            pushCapped(m_particles, p);
        }

        // Dark smoke puffs
        for (int i = 0; i < 5; ++i)
        {
            Particle p;
            p.kind = Kind::Smoke;
            p.pos = pos + sf::Vector2f{rnd(-40.0f, 40.0f), rnd(-20.0f, 20.0f)};
            p.vel = sf::Vector2f{rnd(-40.0f, 40.0f), rnd(-120.0f, -40.0f)};
            p.maxLife = 1.4f;
            p.size = rnd(40.0f, 90.0f);
            p.spin = rnd(-30.0f, 30.0f);
            p.color = sf::Color(20, 6, 40, 200);
            pushCapped(m_particles, p);
        }

        addShake(14.0f);
    }

    void Effects::spawnConfetti()
    {
        for (int i = 0; i < 90; ++i)
        {
            Particle p;
            p.kind = Kind::Confetti;
            p.pos = sf::Vector2f{rnd(0.0f, kWidth), rnd(-300.0f, -20.0f)};
            p.vel = sf::Vector2f{rnd(-60.0f, 60.0f), rnd(120.0f, 320.0f)};
            p.gravity = 80.0f;
            p.spin = rnd(-540.0f, 540.0f);
            p.rotation = rnd(0.0f, 360.0f);
            p.size = rnd(8.0f, 16.0f);
            p.maxLife = rnd(3.0f, 4.5f);
            p.color = kConfettiColors[static_cast<int>(rnd() * 6.0f)];
            pushCapped(m_particles, p);
        }
    }

    void Effects::drawBack(sf::RenderTarget &target) const
    {
        // Explosion core: three starburst layers that pop out and then fade
        if (explosionActive())
        {
            const float t = m_explosionTime;
            const float s = clamp01((t - 0.25f) / 0.95f);
            const float alpha = t < 0.25f ? clamp01(t / 0.04f) : 1.0f - s;
            const float spin = t * 60.0f;

            struct Layer
            {
                float radius;
                int spikes;
                float spinOffset;
                sf::Color inner;
                sf::Color outer;
                unsigned int seed;
            };
            const Layer layers[] = {
                {330.0f, 16, 0.0f, sf::Color(255, 42, 74), sf::Color(255, 154, 43), 11u},
                {230.0f, 14, 8.0f, sf::Color(255, 250, 200), sf::Color(255, 210, 74), 23u},
                {125.0f, 10, 4.0f, sf::Color(255, 255, 255), sf::Color(255, 246, 214), 37u}};

            for (const Layer &layer : layers)
            {
                float radius = t < 0.25f ? layer.radius * easeOutBack(t / 0.25f)
                                         : layer.radius * (1.0f - 0.6f * s);
                if (radius < 1.0f)
                    continue;

                UiKit::drawStarburst(target, m_explosionPos, radius, radius * 0.5f, layer.spikes,
                                     spin + layer.spinOffset, scaleAlpha(layer.inner, alpha),
                                     scaleAlpha(layer.outer, alpha), layer.seed);
            }
        }

        // Flap bursts
        for (const Burst &b : m_bursts)
        {
            float t = clamp01(b.age / b.life);
            float radius = b.radius * lerpf(0.6f, 1.0f, easeOut(t));
            float alpha = 1.0f - t;
            UiKit::drawStarburst(target, b.pos, radius, radius * 0.45f, 12, b.age * 240.0f,
                                 scaleAlpha(b.inner, alpha),
                                 scaleAlpha(b.outer, alpha), 5u);
        }

        // Shockwave rings (the oldest one gets a soft halo)
        for (std::size_t i = 0; i < m_rings.size(); ++i)
        {
            const Ring &r = m_rings[i];
            float t = clamp01(r.age / r.life);
            float e = easeOut(t);
            float radius = std::max(lerpf(r.startRadius, r.endRadius, e), 0.5f);
            float thickness = std::max(r.thickness * (1.0f - 0.6f * e), 0.5f);
            float alpha = 1.0f - t;

            if (i == 0)
                UiKit::drawRing(target, r.pos, radius, thickness * 4.0f, scaleAlpha(r.color, alpha * 0.25f));
            UiKit::drawRing(target, r.pos, radius, thickness, scaleAlpha(r.color, alpha));
        }

        // Smoke puffs
        for (const Particle &p : m_particles)
        {
            if (p.kind != Kind::Smoke)
                continue;
            float t = clamp01(p.life / p.maxLife);
            float alpha = (1.0f - t) * std::min(1.0f, t * 6.0f);
            UiKit::drawGlow(target, p.pos, p.size, scaleAlpha(p.color, alpha));
        }
    }

    void Effects::drawFront(sf::RenderTarget &target) const
    {
        // Spark lines, batched into one vertex array (tail fades to transparent)
        sf::VertexArray sparks(sf::PrimitiveType::Triangles);
        for (const Particle &p : m_particles)
        {
            if (p.kind != Kind::Spark)
                continue;

            float t = clamp01(p.life / p.maxLife);
            float alpha = 1.0f - t;

            float speed = std::sqrt(p.vel.x * p.vel.x + p.vel.y * p.vel.y);
            sf::Vector2f dir = speed > 0.001f ? sf::Vector2f{p.vel.x / speed, p.vel.y / speed}
                                              : sf::Vector2f{1.0f, 0.0f};
            float len = std::clamp(speed * 0.06f, 6.0f, 40.0f);
            sf::Vector2f head = p.pos;
            sf::Vector2f tail = p.pos - dir * len;
            sf::Vector2f perp{-dir.y * p.size * 0.5f, dir.x * p.size * 0.5f};

            sf::Color headColor = scaleAlpha(mix(p.color, sf::Color::White, 0.4f), alpha);
            sf::Color tailColor = withAlpha(mix(p.color, sf::Color::White, 0.4f), 0);

            sparks.append(sf::Vertex{tail + perp, tailColor});
            sparks.append(sf::Vertex{tail - perp, tailColor});
            sparks.append(sf::Vertex{head - perp, headColor});
            sparks.append(sf::Vertex{tail + perp, tailColor});
            sparks.append(sf::Vertex{head - perp, headColor});
            sparks.append(sf::Vertex{head + perp, headColor});

            UiKit::drawGlow(target, p.pos, p.size * 3.0f + 2.0f, scaleAlpha(p.color, alpha * 0.5f));
        }
        target.draw(sparks);

        for (const Particle &p : m_particles)
        {
            float t = clamp01(p.life / p.maxLife);
            switch (p.kind)
            {
            case Kind::Sparkle:
                UiKit::drawSparkle(target, p.pos, p.size * (0.6f + 0.4f * (1.0f - t)),
                                   scaleAlpha(p.color, 1.0f - t), p.rotation);
                break;

            case Kind::Chunk:
            {
                float alpha = t > 0.8f ? (1.0f - t) / 0.2f : 1.0f;
                UiKit::drawShadedEllipse(target, p.pos, sf::Vector2f{p.size, p.size * 0.8f},
                                         scaleAlpha(mix(p.color, sf::Color::White, 0.6f), alpha), scaleAlpha(p.color, alpha),
                                         scaleAlpha(mix(p.color, sf::Color(20, 10, 60), 0.55f), alpha), p.rotation);
                sf::Vector2f highlight = p.pos + rotateVec(sf::Vector2f{-p.size * 0.35f, -p.size * 0.3f}, p.rotation);
                UiKit::drawShadedEllipse(target, highlight, sf::Vector2f{p.size * 0.3f, p.size * 0.16f},
                                         scaleAlpha(sf::Color::White, alpha * 0.9f),
                                         scaleAlpha(sf::Color::White, alpha * 0.5f),
                                         withAlpha(sf::Color::White, 0), p.rotation);
                break;
            }

            case Kind::Confetti:
            {
                float alpha = t > 0.8f ? (1.0f - t) / 0.2f : 1.0f;
                sf::RectangleShape rect(sf::Vector2f{p.size, p.size * 0.5f});
                rect.setOrigin(sf::Vector2f{p.size * 0.5f, p.size * 0.25f});
                rect.setPosition(p.pos);
                rect.setRotation(sf::degrees(p.rotation));
                rect.setFillColor(scaleAlpha(p.color, alpha));
                target.draw(rect);
                break;
            }

            default:
                break; // sparks drawn above, smoke drawn behind
            }
        }

        // "+N" popups: pop in, settle, then fade over the last 35 % of their life
        for (const Popup &p : m_popups)
        {
            float scale = 1.0f;
            if (p.age < 0.12f)
                scale = lerpf(0.6f, 1.15f, p.age / 0.12f);
            else if (p.age < 0.2f)
                scale = lerpf(1.15f, 1.0f, (p.age - 0.12f) / 0.08f);

            float fadeStart = p.life * 0.65f;
            float alpha = p.age <= fadeStart ? 1.0f : 1.0f - (p.age - fadeStart) / (p.life - fadeStart);
            if (alpha <= 0.0f)
                continue;

            UiKit::drawExtrudedText(target, *m_font, p.text, 64, p.pos, UiKit::orangeStyle(), scale, 0.0f,
                                    toByte(alpha));
        }
    }
} // namespace Flappy
