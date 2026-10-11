#include "BlasterGame.h"
#include "../../Data/Profile/Achievements.h"
#include "UiKit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr float kDegToRad = kPi / 180.0f;

        constexpr float kShipSpeed = 520.0f;  // keyboard flight, px/s
        constexpr float kShotSpeed = 1100.0f; // player bullets, px/s
        constexpr float kShipRadius = 24.0f;
        constexpr float kMinX = 70.0f;
        constexpr float kMaxX = 1210.0f;
        constexpr float kMinY = 150.0f;
        constexpr float kMaxY = 690.0f;
        constexpr float kChainWindow = 1.5f; // kills this close together extend the combo
        constexpr float kInvulnTime = 1.5f;
        constexpr float kPowerTime = 10.0f;
        constexpr float kDropChance = 0.10f;
        constexpr int kBossEvery = 40;
        constexpr int kBossHp = 60;
        constexpr int kBaseHearts = 3;
        constexpr int kMaxHearts = 5;
        constexpr std::size_t kMaxEnemies = 60;
        constexpr std::size_t kMaxShots = 120;
        constexpr std::size_t kMaxEnemyShots = 120;
        constexpr std::size_t kMaxPickups = 6;

        constexpr std::array<float, 3> kRockRadius = {20.0f, 36.0f, 58.0f};   // small, medium, large
        constexpr std::array<float, 3> kSpawnInterval = {1.7f, 1.25f, 0.95f}; // easy, normal, hard
        constexpr std::array<float, 3> kStarSpeed = {0.2f, 0.45f, 0.9f};      // parallax per layer
        constexpr std::array<int, 3> kStarCount = {80, 48, 26};

        // Colour from 0..255 ints (clamped)
        sf::Color rgba(int r, int g, int b, int a = 255)
        {
            auto c8 = [](int v)
            { return static_cast<std::uint8_t>(std::clamp(v, 0, 255)); };
            return sf::Color(c8(r), c8(g), c8(b), c8(a));
        }

        // Same colour with another alpha (0..255)
        sf::Color fade(sf::Color c, float alpha)
        {
            c.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 255.0f));
            return c;
        }

        // White hit flash alpha: full strength right after a hit, fading out over 0.12 s
        int flashAlpha(float flash, float maxAlpha)
        {
            return static_cast<int>(std::clamp(flash / 0.12f, 0.0f, 1.0f) * maxAlpha);
        }

        float vecLen(sf::Vector2f v)
        {
            return std::sqrt(v.x * v.x + v.y * v.y);
        }

        float distSq(sf::Vector2f a, sf::Vector2f b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        // Wraps v into [0, period)
        float wrap(float v, float period)
        {
            const float r = std::fmod(v, period);
            return r < 0.0f ? r + period : r;
        }

        // Point on an ellipse (radii rx, ry) rotated by rotDeg around c, at parametric angle t (radians)
        sf::Vector2f ellipsePt(sf::Vector2f c, float rx, float ry, float rotDeg, float t)
        {
            const float x = rx * std::cos(t);
            const float y = ry * std::sin(t);
            const float r = rotDeg * kDegToRad;
            return sf::Vector2f{c.x + x * std::cos(r) - y * std::sin(r), c.y + x * std::sin(r) + y * std::cos(r)};
        }

        // Points of an elliptical arc from angle a0 to a1 (radians), segments + 1 points
        std::vector<sf::Vector2f> arcPoints(sf::Vector2f c, float rx, float ry, float rotDeg, float a0, float a1,
                                            int segments)
        {
            std::vector<sf::Vector2f> pts;
            pts.reserve(static_cast<std::size_t>(segments) + 1);
            for (int i = 0; i <= segments; ++i)
            {
                const float t = a0 + (a1 - a0) * static_cast<float>(i) / static_cast<float>(segments);
                pts.push_back(ellipsePt(c, rx, ry, rotDeg, t));
            }
            return pts;
        }

        // Convex capsule outline in local coordinates, centred on the origin and lying along +x
        std::vector<sf::Vector2f> capsulePoints(float halfLen, float radius)
        {
            const float straight = std::max(halfLen - radius, 0.0f);
            constexpr int kCap = 6;
            std::vector<sf::Vector2f> pts;
            for (int k = 0; k <= kCap; ++k)
            {
                const float a = -0.5f * kPi + kPi * static_cast<float>(k) / static_cast<float>(kCap);
                pts.push_back(sf::Vector2f{straight + radius * std::cos(a), radius * std::sin(a)});
            }
            for (int k = 0; k <= kCap; ++k)
            {
                const float a = 0.5f * kPi + kPi * static_cast<float>(k) / static_cast<float>(kCap);
                pts.push_back(sf::Vector2f{-straight + radius * std::cos(a), radius * std::sin(a)});
            }
            return pts;
        }

        // Filled polygon as a fan from its centroid; colour goes from `top` (smallest y) to `bottom` (largest y)
        void fillPolyGradient(sf::RenderTarget &target, const sf::RenderStates &states,
                              const std::vector<sf::Vector2f> &pts, sf::Color top, sf::Color bottom)
        {
            if (pts.size() < 3)
            {
                return;
            }
            float minY = pts[0].y;
            float maxY = pts[0].y;
            sf::Vector2f c{0.0f, 0.0f};
            for (const sf::Vector2f &p : pts)
            {
                minY = std::min(minY, p.y);
                maxY = std::max(maxY, p.y);
                c.x += p.x;
                c.y += p.y;
            }
            const float n = static_cast<float>(pts.size());
            c.x /= n;
            c.y /= n;
            const float span = std::max(maxY - minY, 1.0f);
            auto shade = [&](sf::Vector2f p)
            { return mix(top, bottom, (p.y - minY) / span); };

            sf::VertexArray va(sf::PrimitiveType::TriangleFan, pts.size() + 2);
            va[0] = sf::Vertex{c, shade(c)};
            for (std::size_t i = 0; i <= pts.size(); ++i)
            {
                const sf::Vector2f p = pts[i % pts.size()];
                va[i + 1] = sf::Vertex{p, shade(p)};
            }
            target.draw(va, states);
        }

        // Thick polyline (or closed loop) as a triangle strip
        void strokePath(sf::RenderTarget &target, const sf::RenderStates &states, const std::vector<sf::Vector2f> &pts,
                        float thickness, sf::Color color, bool closed)
        {
            const std::size_t n = pts.size();
            if (n < 2)
            {
                return;
            }
            const float h = thickness * 0.5f;
            const std::size_t count = closed ? n + 1 : n;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip, count * 2);
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t cur = i % n;
                const std::size_t prev = closed ? (cur + n - 1) % n : (i > 0 ? i - 1 : 0);
                const std::size_t next = closed ? (cur + 1) % n : std::min(i + 1, n - 1);
                const sf::Vector2f d{pts[next].x - pts[prev].x, pts[next].y - pts[prev].y};
                const float len = std::max(vecLen(d), 0.0001f);
                const sf::Vector2f nrm{-d.y / len * h, d.x / len * h};
                va[2 * i] = sf::Vertex{sf::Vector2f{pts[cur].x + nrm.x, pts[cur].y + nrm.y}, color};
                va[2 * i + 1] = sf::Vertex{sf::Vector2f{pts[cur].x - nrm.x, pts[cur].y - nrm.y}, color};
            }
            target.draw(va, states);
        }

        sf::Color pickupColor(char letter)
        {
            switch (letter)
            {
            case 'R':
                return rgba(255, 140, 40);
            case 'T':
                return rgba(80, 230, 255);
            case 'S':
                return rgba(120, 160, 255);
            default:
                return rgba(255, 90, 170);
            }
        }
    } // namespace

    BlasterGame::BlasterGame(const sf::Font &font) : ArcadeGame(font, 2, 2)
    {
        // Deterministic starfield in three parallax layers
        std::uint32_t seed = 9001u;
        auto next = [&seed]()
        {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>(seed >> 8) / 16777216.0f;
        };
        for (int layer = 0; layer < 3; ++layer)
        {
            const float baseRadius = layer == 0 ? 1.0f : (layer == 1 ? 1.5f : 2.2f);
            for (int i = 0; i < kStarCount[static_cast<std::size_t>(layer)]; ++i)
            {
                Star s;
                s.pos = sf::Vector2f{next() * kWidth, 20.0f + next() * (kHeight - 20.0f)};
                s.radius = baseRadius * (0.7f + 0.5f * next());
                s.phase = next() * kTwoPi;
                s.layer = layer;
                m_stars.push_back(s);
            }
        }
    }

    float BlasterGame::rnd()
    {
        m_rng = m_rng * 1664525u + 1013904223u;
        return static_cast<float>(m_rng >> 8) / 16777216.0f;
    }

    float BlasterGame::rnd(float lo, float hi)
    {
        return lo + (hi - lo) * rnd();
    }

    void BlasterGame::onStart()
    {
        m_enemies.clear();
        m_spawned.clear();
        m_shots.clear();
        m_enemyShots.clear();
        m_pickups.clear();

        m_ship = sf::Vector2f{220.0f, 420.0f};
        m_mouseTarget = m_ship;
        m_mouseActive = false;
        m_mouseFire = false;
        m_shipAlive = true;
        m_tilt = 0.0f;
        m_prevY = m_ship.y;
        m_throttle = 0.5f;
        m_fireTimer = 0.0f;

        m_hp = kBaseHearts;
        m_invuln = 0.0f;
        m_flash = 0.0f;
        m_rapid = 0.0f;
        m_triple = 0.0f;
        m_shield = false;

        m_kills = 0;
        m_nextBoss = kBossEvery;
        m_bossAlive = false;
        m_chain = 0;
        m_chainTimer = 0.0f;
        m_spawnTimer = 1.0f;

        m_scrollSpeed = 40.0f;

        setCombo(0);
        setComboWord("COMBO");
        setLives(kBaseHearts, kBaseHearts);
    }

    void BlasterGame::onIdle(float dt)
    {
        scrollBackdrop(dt, 40.0f);
    }

    void BlasterGame::onUpdate(float dt)
    {
        scrollBackdrop(dt, 150.0f);
        m_flash = std::max(0.0f, m_flash - 2.2f * dt);
        m_invuln = std::max(0.0f, m_invuln - dt);
        m_rapid = std::max(0.0f, m_rapid - dt);
        m_triple = std::max(0.0f, m_triple - dt);

        if (m_chainTimer > 0.0f)
        {
            m_chainTimer -= dt;
            if (m_chainTimer <= 0.0f)
            {
                m_chain = 0;
                setCombo(0);
            }
        }
        // A release may have been eaten by the pause overlay, so check the real button state too
        if (m_mouseFire && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        {
            m_mouseFire = false;
        }

        updateShip(dt);
        spawnWave(dt);
        updateEnemies(dt);
        updateShots(dt);
        updateEnemyShots(dt);
        updateContacts();
        updatePickups(dt);

        // Add this frame's new enemies, then drop everything that is finished
        m_enemies.insert(m_enemies.end(), m_spawned.begin(), m_spawned.end());
        m_spawned.clear();
        std::erase_if(m_enemies, [](const Enemy &e)
                      { return !e.alive; });
        std::erase_if(m_shots, [](const Shot &s)
                      { return !s.alive; });
        std::erase_if(m_enemyShots, [](const EnemyShot &s)
                      { return !s.alive; });
        std::erase_if(m_pickups, [](const Pickup &p)
                      { return !p.alive; });
    }

    bool BlasterGame::isStartKey(sf::Keyboard::Key key) const
    {
        using K = sf::Keyboard::Key;
        return key == K::Space || key == K::W || key == K::A || key == K::S || key == K::D || key == K::Up ||
               key == K::Down || key == K::Left || key == K::Right || key == K::Enter;
    }

    void BlasterGame::onMouseMoved(sf::Vector2f point)
    {
        m_mouseTarget = point;
        m_mouseActive = true;
    }

    void BlasterGame::onMousePressed(sf::Vector2f point, sf::Mouse::Button button)
    {
        if (button == sf::Mouse::Button::Left)
        {
            m_mouseFire = true;
            m_mouseTarget = point;
            m_mouseActive = true;
        }
    }

    void BlasterGame::onMouseReleased(sf::Vector2f point, sf::Mouse::Button button)
    {
        (void)point;
        if (button == sf::Mouse::Button::Left)
        {
            m_mouseFire = false;
        }
    }

    void BlasterGame::scrollBackdrop(float dt, float targetSpeed)
    {
        m_scrollSpeed += (targetSpeed - m_scrollSpeed) * std::min(1.0f, 3.0f * dt);
        m_scroll += m_scrollSpeed * dt;
    }

    // ---------------------------------------------------------------- ship

    void BlasterGame::updateShip(float dt)
    {
        using K = sf::Keyboard::Key;
        const float dx = (keyDown(K::D) || keyDown(K::Right) ? 1.0f : 0.0f) -
                         (keyDown(K::A) || keyDown(K::Left) ? 1.0f : 0.0f);
        const float dy = (keyDown(K::S) || keyDown(K::Down) ? 1.0f : 0.0f) -
                         (keyDown(K::W) || keyDown(K::Up) ? 1.0f : 0.0f);

        const sf::Vector2f before = m_ship;
        if (dx != 0.0f || dy != 0.0f)
        {
            // Keys take over until the mouse moves again
            m_mouseActive = false;
            const float len = vecLen(sf::Vector2f{dx, dy});
            m_ship.x += dx / len * kShipSpeed * dt;
            m_ship.y += dy / len * kShipSpeed * dt;
        }
        else if (m_mouseActive)
        {
            const float k = 1.0f - std::exp(-11.0f * dt);
            m_ship.x += (m_mouseTarget.x - m_ship.x) * k;
            m_ship.y += (m_mouseTarget.y - m_ship.y) * k;
        }
        m_ship.x = std::clamp(m_ship.x, kMinX, kMaxX);
        m_ship.y = std::clamp(m_ship.y, kMinY, kMaxY);

        // Flame strength follows the speed; the nose tilts with the vertical speed
        const float step = dt > 0.0f ? dt : 0.0001f;
        const float moved = vecLen(sf::Vector2f{m_ship.x - before.x, m_ship.y - before.y}) / step;
        m_throttle += (std::clamp(moved / 420.0f, 0.2f, 1.0f) - m_throttle) * std::min(1.0f, 10.0f * dt);
        const float vy = (m_ship.y - m_prevY) / step;
        m_prevY = m_ship.y;
        m_tilt += (std::clamp(vy * 0.05f, -22.0f, 22.0f) - m_tilt) * std::min(1.0f, 8.0f * dt);

        // Rapid fire doubles the rate; releasing the trigger re-arms the first shot instantly
        const bool firing = m_shipAlive && (m_mouseFire || keyDown(K::Space));
        const float interval = 1.0f / (m_rapid > 0.0f ? 11.0f : 7.0f);
        if (!firing)
        {
            m_fireTimer = interval;
            return;
        }
        m_fireTimer += dt;
        if (m_fireTimer >= interval)
        {
            m_fireTimer -= interval;
            fireVolley();
        }
    }

    void BlasterGame::fireVolley()
    {
        const sf::Vector2f muzzle{m_ship.x + 46.0f, m_ship.y};
        const bool triple = m_triple > 0.0f;
        const int count = triple ? 3 : 1;
        for (int i = 0; i < count; ++i)
        {
            if (m_shots.size() >= kMaxShots)
            {
                break;
            }
            const float a = triple ? (static_cast<float>(i) - 1.0f) * 9.0f * kDegToRad : 0.0f;
            Shot s;
            s.pos = muzzle;
            s.vel = sf::Vector2f{std::cos(a) * kShotSpeed, std::sin(a) * kShotSpeed};
            m_shots.push_back(s);
        }
        audio().playShoot();
        fx().spawnBurst(muzzle, rgba(255, 250, 200), 3, 0.5f);
    }

    // ---------------------------------------------------------------- enemies

    void BlasterGame::spawnWave(float dt)
    {
        m_spawnTimer -= dt;
        if (m_spawnTimer > 0.0f)
        {
            return;
        }

        const int diff = std::clamp(difficulty(), 0, 2);
        const float ramp = std::clamp(1.0f - runTime() / 160.0f, 0.4f, 1.0f);
        m_spawnTimer = kSpawnInterval[static_cast<std::size_t>(diff)] * ramp * rnd(0.75f, 1.25f) *
                       (m_bossAlive ? 1.5f : 1.0f);

        if (!m_bossAlive && m_kills >= m_nextBoss)
        {
            spawnBoss();
            return;
        }
        if (m_enemies.size() + m_spawned.size() >= kMaxEnemies)
        {
            return;
        }

        int bombers = 0;
        for (const Enemy &e : m_enemies)
        {
            if (e.alive && e.kind == Kind::Bomber)
            {
                ++bombers;
            }
        }

        const float t = runTime();
        const float roll = rnd();
        if (t >= 60.0f && bombers < 2 && roll < 0.12f)
        {
            Enemy e;
            e.kind = Kind::Bomber;
            e.pos = sf::Vector2f{1360.0f, rnd(230.0f, 600.0f)};
            e.baseY = e.pos.y;
            e.vel = sf::Vector2f{-55.0f, 0.0f};
            e.radius = 64.0f;
            e.hp = 8;
            e.maxHp = 8;
            e.phase = rnd(0.0f, kTwoPi);
            e.shootTimer = 2.0f;
            m_spawned.push_back(e);
        }
        else if (t >= 12.0f && roll < 0.40f + 0.05f * static_cast<float>(diff))
        {
            Enemy e;
            e.kind = Kind::Drone;
            e.pos = sf::Vector2f{1340.0f, rnd(190.0f, 620.0f)};
            e.baseY = e.pos.y;
            e.vel = sf::Vector2f{-(150.0f + 20.0f * static_cast<float>(diff)), 0.0f};
            e.radius = 32.0f;
            e.hp = 2;
            e.maxHp = 2;
            e.phase = rnd(0.0f, kTwoPi);
            e.shootTimer = rnd(1.0f, 2.0f);
            m_spawned.push_back(e);
        }
        else
        {
            const float s = rnd();
            const int size = s < 0.45f ? 2 : (s < 0.8f ? 1 : 0);
            const sf::Vector2f pos{1340.0f, rnd(170.0f, 650.0f)};
            const sf::Vector2f vel{-rnd(90.0f, 150.0f) - 10.0f * static_cast<float>(diff), rnd(-30.0f, 30.0f)};
            m_spawned.push_back(makeAsteroid(size, pos, vel));
        }
    }

    void BlasterGame::spawnBoss()
    {
        Enemy e;
        e.kind = Kind::Mothership;
        e.pos = sf::Vector2f{1450.0f, 420.0f};
        e.baseY = 420.0f;
        e.vel = sf::Vector2f{-150.0f, 0.0f};
        e.radius = 120.0f;
        e.hp = kBossHp;
        e.maxHp = kBossHp;
        e.shootTimer = 1.6f;
        m_spawned.push_back(e);
        m_bossAlive = true;

        fx().spawnText(sf::Vector2f{640.0f, 260.0f}, "MOTHERSHIP!");
        audio().playPower();
    }

    BlasterGame::Enemy BlasterGame::makeAsteroid(int size, sf::Vector2f pos, sf::Vector2f vel)
    {
        Enemy e;
        e.kind = Kind::Asteroid;
        e.size = size;
        e.pos = pos;
        e.vel = vel;
        e.radius = kRockRadius[static_cast<std::size_t>(size)];
        e.hp = size + 1;
        e.maxHp = e.hp;
        e.rot = rnd(0.0f, 360.0f);
        e.spin = rnd(-60.0f, 60.0f);

        // Jittered polygon with 7 to 9 vertices
        const int n = 7 + static_cast<int>(rnd() * 3.0f);
        for (int i = 0; i < n; ++i)
        {
            const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(n) + rnd(-0.18f, 0.18f);
            const float r = e.radius * rnd(0.74f, 1.0f);
            e.shape.push_back(sf::Vector2f{std::cos(a) * r, std::sin(a) * r});
        }
        return e;
    }

    void BlasterGame::fireAimed(sf::Vector2f from, float speed, float spreadDeg, int count, sf::Color color)
    {
        const sf::Vector2f d{m_ship.x - from.x, m_ship.y - from.y};
        const float base = std::atan2(d.y, d.x);
        for (int i = 0; i < count; ++i)
        {
            if (m_enemyShots.size() >= kMaxEnemyShots)
            {
                return;
            }
            const float offset = (static_cast<float>(i) - static_cast<float>(count - 1) * 0.5f) * spreadDeg * kDegToRad;
            const float a = base + offset;
            EnemyShot s;
            s.pos = from;
            s.vel = sf::Vector2f{std::cos(a) * speed, std::sin(a) * speed};
            s.color = color;
            m_enemyShots.push_back(s);
        }
    }

    void BlasterGame::updateEnemies(float dt)
    {
        for (Enemy &e : m_enemies)
        {
            if (!e.alive)
            {
                continue;
            }
            e.age += dt;
            e.flash = std::max(0.0f, e.flash - dt);

            switch (e.kind)
            {
            case Kind::Asteroid:
                e.pos.x += e.vel.x * dt;
                e.pos.y += e.vel.y * dt;
                e.rot += e.spin * dt;
                if ((e.pos.y < 165.0f && e.vel.y < 0.0f) || (e.pos.y > 675.0f && e.vel.y > 0.0f))
                {
                    e.vel.y = -e.vel.y;
                }
                break;

            case Kind::Drone:
                e.pos.x += e.vel.x * dt;
                e.pos.y = e.baseY + std::sin(e.age * 2.2f + e.phase) * 70.0f;
                e.shootTimer -= dt;
                if (e.shootTimer <= 0.0f && e.pos.x < 1230.0f && e.pos.x > 40.0f)
                {
                    fireAimed(e.pos, 270.0f, 0.0f, 1, rgba(255, 150, 60));
                    e.shootTimer = rnd(1.75f, 2.25f);
                }
                break;

            case Kind::Bomber:
                e.pos.x += e.vel.x * dt;
                e.pos.y = e.baseY + std::sin(e.age * 1.1f + e.phase) * 90.0f;
                e.shootTimer -= dt;
                if (e.shootTimer <= 0.0f && e.pos.x < 1200.0f)
                {
                    fireAimed(e.pos, 230.0f, 0.0f, 1, rgba(255, 90, 170));
                    e.shootTimer = 2.6f;
                }
                break;

            case Kind::Mothership:
                if (e.pos.x > 980.0f)
                {
                    e.pos.x = std::max(980.0f, e.pos.x - 150.0f * dt);
                }
                e.pos.y = 420.0f + std::sin(e.age * 0.7f) * 170.0f;
                if (e.pos.x < 1100.0f)
                {
                    e.shootTimer -= dt;
                    if (e.shootTimer <= 0.0f)
                    {
                        fireAimed(e.pos, 260.0f, 12.0f, 3, rgba(255, 70, 90));
                        e.shootTimer = 1.6f;
                    }
                }
                break;
            }

            if (e.kind != Kind::Mothership && e.pos.x < -160.0f)
            {
                e.alive = false;
            }
        }
    }

    void BlasterGame::updateShots(float dt)
    {
        for (Shot &s : m_shots)
        {
            if (!s.alive)
            {
                continue;
            }
            s.pos.x += s.vel.x * dt;
            s.pos.y += s.vel.y * dt;
            s.life -= dt;
            if (s.life <= 0.0f || s.pos.x > kWidth + 40.0f || s.pos.y < 0.0f || s.pos.y > kHeight)
            {
                s.alive = false;
                continue;
            }

            for (Enemy &e : m_enemies)
            {
                if (!e.alive)
                {
                    continue;
                }
                const float reach = e.radius + 6.0f;
                if (distSq(s.pos, e.pos) > reach * reach)
                {
                    continue;
                }
                s.alive = false;
                e.hp -= 1;
                e.flash = 0.12f;
                fx().spawnBurst(s.pos, rgba(255, 240, 170), 6, 0.6f);
                if (e.hp <= 0)
                {
                    destroyEnemy(e);
                }
                else
                {
                    audio().playPop(1.4f);
                }
                break;
            }
        }
    }

    void BlasterGame::updateEnemyShots(float dt)
    {
        for (EnemyShot &s : m_enemyShots)
        {
            if (!s.alive)
            {
                continue;
            }
            s.pos.x += s.vel.x * dt;
            s.pos.y += s.vel.y * dt;
            s.life -= dt;
            if (s.life <= 0.0f || s.pos.x < -60.0f || s.pos.x > kWidth + 60.0f || s.pos.y < 0.0f || s.pos.y > kHeight)
            {
                s.alive = false;
                continue;
            }
            const float reach = kShipRadius + 6.0f;
            if (m_shipAlive && distSq(s.pos, m_ship) < reach * reach)
            {
                s.alive = false;
                fx().spawnBurst(s.pos, s.color, 10, 0.8f);
                hurt();
            }
        }
    }

    // Enemies that crash into the ship explode (the boss only hurts)
    void BlasterGame::updateContacts()
    {
        for (Enemy &e : m_enemies)
        {
            if (!e.alive || !m_shipAlive)
            {
                continue;
            }
            const float reach = e.radius * 0.85f + kShipRadius;
            if (distSq(e.pos, m_ship) > reach * reach)
            {
                continue;
            }
            if (e.kind == Kind::Mothership)
            {
                hurt();
                continue;
            }
            fx().spawnBlast(e.pos, 0.9f, tintFor(e));
            audio().playBlast();
            e.alive = false;
            hurt();
            if (!m_shipAlive)
            {
                return;
            }
        }
    }

    void BlasterGame::updatePickups(float dt)
    {
        for (Pickup &p : m_pickups)
        {
            if (!p.alive)
            {
                continue;
            }
            p.age += dt;
            p.rot += 90.0f * dt;
            p.pos.x -= 90.0f * dt;
            p.pos.y += std::sin(p.age * 3.0f) * 30.0f * dt;
            if (p.pos.x < -60.0f)
            {
                p.alive = false;
                continue;
            }
            if (m_shipAlive && distSq(p.pos, m_ship) < 46.0f * 46.0f)
            {
                collectPickup(p);
                p.alive = false;
            }
        }
    }

    sf::Color BlasterGame::tintFor(const Enemy &e) const
    {
        switch (e.kind)
        {
        case Kind::Asteroid:
            return rgba(190, 165, 150);
        case Kind::Drone:
            return rgba(255, 120, 60);
        case Kind::Bomber:
            return rgba(255, 80, 120);
        default:
            return rgba(200, 120, 255);
        }
    }

    void BlasterGame::destroyEnemy(Enemy &e)
    {
        e.alive = false;
        ++m_kills;
        const sf::Color tint = tintFor(e);
        float scale = 1.0f;
        int points = 0;

        switch (e.kind)
        {
        case Kind::Asteroid:
            // Large splits into two medium, medium into two small, small is gone
            if (e.size > 0)
            {
                for (int i = 0; i < 2; ++i)
                {
                    const float side = i == 0 ? -1.0f : 1.0f;
                    m_spawned.push_back(makeAsteroid(e.size - 1, e.pos, sf::Vector2f{e.vel.x - 30.0f, e.vel.y + side * 70.0f}));
                }
            }
            scale = 0.55f + 0.4f * static_cast<float>(e.size);
            points = e.size == 2 ? 20 : (e.size == 1 ? 40 : 80);
            break;

        case Kind::Drone:
            scale = 1.0f;
            points = 100;
            break;

        case Kind::Bomber:
            // Armoured ship: releases a ring of eight bullets
            scale = 1.6f;
            points = 300;
            for (int i = 0; i < 8; ++i)
            {
                if (m_enemyShots.size() >= kMaxEnemyShots)
                {
                    break;
                }
                const float a = static_cast<float>(i) * kPi / 4.0f;
                EnemyShot s;
                s.pos = e.pos;
                s.vel = sf::Vector2f{std::cos(a) * 240.0f, std::sin(a) * 240.0f};
                s.life = 4.0f;
                s.color = rgba(255, 140, 90);
                m_enemyShots.push_back(s);
            }
            break;

        case Kind::Mothership:
            fx().spawnExplosion(e.pos);
            fx().spawnConfetti();
            m_bossAlive = false;
            m_nextBoss = m_kills + kBossEvery;
            addScore(1000);
            unlockAchievement(::Achievements::BossSlayer);
            fx().spawnText(e.pos, "+1000");
            audio().playBlast();
            dropPickup(e.pos);
            return;
        }

        fx().spawnBlast(e.pos, scale, tint);
        audio().playBlast();
        registerKill(e.pos, points);
        if (rnd() < kDropChance)
        {
            dropPickup(e.pos);
        }
    }

    // Kills that follow each other within kChainWindow raise the chain; the chain is the multiplier source
    void BlasterGame::registerKill(sf::Vector2f pos, int points)
    {
        m_chain = m_chainTimer > 0.0f ? m_chain + 1 : 1;
        m_chainTimer = kChainWindow;
        setCombo(m_chain);

        const int total = points * (1 + m_chain / 5);
        addScore(total);
        if (total >= 50)
        {
            fx().spawnText(pos, "+" + std::to_string(total));
        }
    }

    void BlasterGame::dropPickup(sf::Vector2f pos)
    {
        if (m_pickups.size() >= kMaxPickups)
        {
            return;
        }
        const float r = rnd();
        char letter = r < 0.35f ? 'R' : (r < 0.60f ? 'T' : (r < 0.80f ? 'S' : 'H'));
        if (letter == 'H' && m_hp >= kMaxHearts)
        {
            letter = 'S';
        }
        Pickup p;
        p.pos = pos;
        p.letter = letter;
        p.rot = rnd(0.0f, 360.0f);
        m_pickups.push_back(p);
    }

    void BlasterGame::collectPickup(const Pickup &p)
    {
        const char *label = "";
        switch (p.letter)
        {
        case 'R':
            m_rapid = kPowerTime;
            label = "RAPID FIRE";
            break;
        case 'T':
            m_triple = kPowerTime;
            label = "TRIPLE SHOT";
            break;
        case 'S':
            m_shield = true;
            label = "SHIELD";
            break;
        default:
            m_hp = std::min(m_hp + 1, kMaxHearts);
            setLives(m_hp, std::max(kBaseHearts, m_hp));
            label = "+1 HEART";
            break;
        }
        audio().playPower();
        fx().spawnBurst(p.pos, pickupColor(p.letter), 18, 1.2f);
        fx().spawnText(p.pos, label);
    }

    void BlasterGame::hurt()
    {
        if (!m_shipAlive || m_invuln > 0.0f)
        {
            return;
        }
        if (m_shield)
        {
            // The bubble soaks up one hit
            m_shield = false;
            m_invuln = 0.8f;
            fx().spawnBurst(m_ship, rgba(140, 230, 255), 22, 1.4f);
            fx().addShake(4.0f);
            audio().playPop(0.6f);
            return;
        }

        --m_hp;
        m_invuln = kInvulnTime;
        m_flash = 1.0f;
        fx().addShake(10.0f);
        audio().playHit();
        setLives(m_hp, std::max(kBaseHearts, m_hp));
        if (m_hp <= 0)
        {
            m_shipAlive = false;
            gameOver(m_ship, 1.0f);
        }
    }

    // ---------------------------------------------------------------- drawing: backdrop

    void BlasterGame::drawBackground(sf::RenderTarget &target) const
    {
        // Vertical gradient: deep navy at the top, purple at the bottom
        const sf::Color navy = rgba(8, 10, 40);
        const sf::Color violet = rgba(64, 22, 104);
        sf::VertexArray sky(sf::PrimitiveType::TriangleStrip, 4);
        sky[0] = sf::Vertex{sf::Vector2f{0.0f, 0.0f}, navy};
        sky[1] = sf::Vertex{sf::Vector2f{kWidth, 0.0f}, navy};
        sky[2] = sf::Vertex{sf::Vector2f{0.0f, kHeight}, violet};
        sky[3] = sf::Vertex{sf::Vector2f{kWidth, kHeight}, violet};
        target.draw(sky);

        // Wrapped drift: objects leave the screen completely before they reappear, so nothing pops
        auto drift = [this](float base, float speed, float period, float offset)
        {
            return wrap(base - m_scroll * speed, period) - offset;
        };

        // Three big coloured nebulae
        UiKit::drawGlow(target, sf::Vector2f{drift(300.0f, 0.03f, 3000.0f, 900.0f), 260.0f}, 420.0f,
                        rgba(120, 60, 220, 70));
        UiKit::drawGlow(target, sf::Vector2f{drift(1300.0f, 0.03f, 3000.0f, 900.0f), 520.0f}, 380.0f,
                        rgba(255, 80, 170, 55));
        UiKit::drawGlow(target, sf::Vector2f{drift(2100.0f, 0.03f, 3000.0f, 900.0f), 150.0f}, 300.0f,
                        rgba(60, 200, 255, 45));

        auto drawStars = [&](int layer)
        {
            const sf::Color base = layer == 0 ? rgba(150, 170, 255) : (layer == 1 ? rgba(200, 215, 255) : rgba(255, 255, 255));
            sf::VertexArray va(sf::PrimitiveType::Triangles);
            for (const Star &s : m_stars)
            {
                if (s.layer != layer)
                {
                    continue;
                }
                const float x = wrap(s.pos.x - m_scroll * kStarSpeed[static_cast<std::size_t>(layer)], kWidth);
                const float tw = 0.55f + 0.45f * std::sin(clockTime() * 2.3f + s.phase);
                const sf::Color c = fade(base, (110.0f + 120.0f * tw));
                const float r = s.radius;
                const sf::Vector2f left{x - r, s.pos.y};
                const sf::Vector2f top{x, s.pos.y - r};
                const sf::Vector2f right{x + r, s.pos.y};
                const sf::Vector2f bottom{x, s.pos.y + r};
                va.append(sf::Vertex{left, c});
                va.append(sf::Vertex{top, c});
                va.append(sf::Vertex{right, c});
                va.append(sf::Vertex{left, c});
                va.append(sf::Vertex{right, c});
                va.append(sf::Vertex{bottom, c});
            }
            target.draw(va);
        };

        drawStars(0);

        // Big planet with a lit rim, bands and a ring that passes behind and in front of it
        const sf::Vector2f pc{drift(1300.0f, 0.05f, 2400.0f, 600.0f), 330.0f};
        const float ringTilt = -16.0f;
        const sf::RenderStates none;
        const std::vector<sf::Vector2f> ringBack = arcPoints(pc, 255.0f, 58.0f, ringTilt, kPi, kTwoPi, 48);
        strokePath(target, none, ringBack, 12.0f, rgba(255, 150, 210, 90), false);
        strokePath(target, none, ringBack, 4.0f, rgba(255, 235, 250, 150), false);

        UiKit::drawGlow(target, pc, 230.0f, rgba(110, 150, 255, 46));
        UiKit::drawShadedEllipse(target, pc, sf::Vector2f{150.0f, 150.0f}, rgba(180, 225, 255), rgba(92, 112, 236),
                                 rgba(40, 28, 124), 0.0f);
        fillPolyGradient(target, none, arcPoints(sf::Vector2f{pc.x, pc.y - 34.0f}, 128.0f, 9.0f, 0.0f, 0.0f, kTwoPi, 28),
                         rgba(60, 40, 150, 80), rgba(60, 40, 150, 80));
        fillPolyGradient(target, none, arcPoints(sf::Vector2f{pc.x, pc.y + 46.0f}, 112.0f, 9.0f, 0.0f, 0.0f, kTwoPi, 28),
                         rgba(60, 40, 150, 80), rgba(60, 40, 150, 80));
        strokePath(target, none, arcPoints(pc, 146.0f, 146.0f, 0.0f, kPi * 1.05f, kPi * 1.5f, 24), 5.0f,
                   rgba(235, 248, 255, 170), false);

        const std::vector<sf::Vector2f> ringFront = arcPoints(pc, 255.0f, 58.0f, ringTilt, 0.0f, kPi, 48);
        strokePath(target, none, ringFront, 12.0f, rgba(255, 150, 210, 110), false);
        strokePath(target, none, ringFront, 4.0f, rgba(255, 235, 250, 170), false);

        drawStars(1);
        drawStars(2);
    }

    // ---------------------------------------------------------------- drawing: play field

    void BlasterGame::drawWorld(sf::RenderTarget &target) const
    {
        drawPickups(target);

        for (const Enemy &e : m_enemies)
        {
            if (!e.alive)
            {
                continue;
            }
            switch (e.kind)
            {
            case Kind::Asteroid:
                drawAsteroid(target, e);
                break;
            case Kind::Drone:
                drawDrone(target, e);
                break;
            case Kind::Bomber:
                drawBomber(target, e);
                break;
            case Kind::Mothership:
                drawMothership(target, e);
                break;
            }
        }
        drawBossBar(target);

        drawEnemyShots(target);
        drawShots(target);

        if (m_shipAlive)
        {
            drawShip(target);
        }

        if (m_flash > 0.001f)
        {
            sf::RectangleShape veil(sf::Vector2f{kWidth + 120.0f, kHeight + 120.0f});
            veil.setPosition(sf::Vector2f{-60.0f, -60.0f});
            veil.setFillColor(rgba(255, 40, 60, static_cast<int>(m_flash * 80.0f)));
            target.draw(veil);
        }
    }

    void BlasterGame::drawShip(sf::RenderTarget &target) const
    {
        // Blink while invulnerable
        if (m_invuln > 0.0f && std::fmod(clockTime() * 12.0f, 2.0f) < 1.0f)
        {
            return;
        }

        const Accent &acc = accent();
        const float t = clockTime();
        sf::RenderStates local;
        local.transform.translate(m_ship).rotate(sf::degrees(m_tilt));

        UiKit::drawGlow(target, m_ship, 96.0f, fade(acc.base, 58.0f));

        // Engine flame: flickering cone with a hot white core, behind the body
        const float flicker = 0.85f + 0.15f * std::sin(t * 41.0f) + 0.05f * std::sin(t * 23.0f);
        const float len = (18.0f + 34.0f * m_throttle) * flicker;
        UiKit::drawGlow(target, local.transform.transformPoint(sf::Vector2f{-34.0f, 0.0f}),
                        26.0f + 10.0f * m_throttle, rgba(255, 150, 60, 110));
        sf::VertexArray flameOuter(sf::PrimitiveType::Triangles);
        flameOuter.append(sf::Vertex{sf::Vector2f{-28.0f, -10.0f}, rgba(255, 205, 90, 235)});
        flameOuter.append(sf::Vertex{sf::Vector2f{-28.0f, 10.0f}, rgba(255, 205, 90, 235)});
        flameOuter.append(sf::Vertex{sf::Vector2f{-28.0f - len, 0.0f}, rgba(255, 90, 40, 0)});
        target.draw(flameOuter, local);
        sf::VertexArray flameCore(sf::PrimitiveType::Triangles);
        flameCore.append(sf::Vertex{sf::Vector2f{-28.0f, -5.0f}, rgba(255, 255, 240, 240)});
        flameCore.append(sf::Vertex{sf::Vector2f{-28.0f, 5.0f}, rgba(255, 255, 240, 240)});
        flameCore.append(sf::Vertex{sf::Vector2f{-28.0f - len * 0.55f, 0.0f}, rgba(255, 220, 160, 0)});
        target.draw(flameCore, local);

        // Two angular metal wings: navy gradient with a light leading edge
        const std::vector<sf::Vector2f> wingUp = {sf::Vector2f{10.0f, -16.0f}, sf::Vector2f{-6.0f, -46.0f},
                                                  sf::Vector2f{-44.0f, -42.0f}, sf::Vector2f{-30.0f, -10.0f}};
        std::vector<sf::Vector2f> wingDown;
        for (const sf::Vector2f &p : wingUp)
        {
            wingDown.push_back(sf::Vector2f{p.x, -p.y});
        }
        auto drawWing = [&](const std::vector<sf::Vector2f> &w)
        {
            fillPolyGradient(target, local, w, rgba(104, 124, 210), rgba(18, 20, 58));
            strokePath(target, local, std::vector<sf::Vector2f>{w[0], w[1]}, 2.5f, rgba(190, 225, 255, 235), false);
            strokePath(target, local, std::vector<sf::Vector2f>{w[1], w[2]}, 1.5f, rgba(120, 150, 230, 160), false);
        };
        drawWing(wingUp);
        drawWing(wingDown);

        // Glossy jello pod in the accent colours
        UiKit::drawShadedEllipse(target, m_ship, sf::Vector2f{34.0f, 26.0f}, acc.light, acc.base, acc.dark, m_tilt);
        strokePath(target, local, arcPoints(sf::Vector2f{0.0f, 0.0f}, 32.5f, 24.5f, 0.0f, 0.0f, kTwoPi, 40), 2.5f,
                   withAlpha(acc.dark, 200), true);
        strokePath(target, local, arcPoints(sf::Vector2f{0.0f, 0.0f}, 30.0f, 22.0f, 0.0f, 0.1f * kPi, 0.9f * kPi, 24),
                   2.5f, withAlpha(sf::Color(127, 244, 255), 200), false);
        fillPolyGradient(target, local, arcPoints(sf::Vector2f{-10.0f, -11.0f}, 14.0f, 6.5f, -20.0f, 0.0f, kTwoPi, 20),
                         rgba(255, 255, 255, 235), rgba(255, 255, 255, 30));
        fillPolyGradient(target, local, arcPoints(sf::Vector2f{11.0f, -2.0f}, 10.0f, 7.0f, 0.0f, 0.0f, kTwoPi, 20),
                         rgba(220, 255, 255, 235), rgba(40, 130, 210, 220));
        UiKit::drawSparkle(target, local.transform.transformPoint(sf::Vector2f{-22.0f, -13.0f}), 6.0f,
                           rgba(255, 255, 255, 240));

        if (m_shield)
        {
            drawShield(target);
        }
    }

    void BlasterGame::drawShield(sf::RenderTarget &target) const
    {
        const float pulse = 1.0f + 0.04f * std::sin(clockTime() * 6.0f);
        const float r = 58.0f * pulse;
        UiKit::drawGlow(target, m_ship, r + 14.0f, rgba(120, 220, 255, 60));
        UiKit::drawShadedEllipse(target, m_ship, sf::Vector2f{r, r}, rgba(220, 250, 255, 70), rgba(120, 220, 255, 38),
                                 rgba(60, 160, 255, 90), 0.0f);
        UiKit::drawRing(target, m_ship, r, 4.0f, rgba(170, 240, 255, 210));
        UiKit::drawRing(target, m_ship, r - 7.0f, 1.5f, rgba(255, 255, 255, 120));
        strokePath(target, sf::RenderStates{}, arcPoints(m_ship, r - 3.0f, r - 3.0f, 0.0f, kPi * 1.05f, kPi * 1.45f, 16),
                   3.0f, rgba(255, 255, 255, 150), false);
    }

    void BlasterGame::drawAsteroid(sf::RenderTarget &target, const Enemy &e) const
    {
        const std::size_t n = e.shape.size();
        if (n < 3)
        {
            return;
        }
        sf::RenderStates s;
        s.transform.translate(e.pos).rotate(sf::degrees(e.rot));

        const sf::Color dark = rgba(58, 46, 60);
        const sf::Color light = rgba(226, 210, 192);
        const sf::Color mid = rgba(146, 128, 124);

        // Lit from the top-left: each rim vertex takes its shade from its direction
        auto shade = [&](sf::Vector2f p)
        {
            const float r = std::max(vecLen(p), 0.001f);
            const float d = (p.x * -0.6f + p.y * -0.8f) / r;
            return mix(dark, light, 0.5f + 0.5f * d);
        };
        sf::VertexArray body(sf::PrimitiveType::TriangleFan, n + 2);
        body[0] = sf::Vertex{sf::Vector2f{0.0f, 0.0f}, mid};
        for (std::size_t i = 0; i <= n; ++i)
        {
            const sf::Vector2f p = e.shape[i % n];
            body[i + 1] = sf::Vertex{p, shade(p)};
        }
        target.draw(body, s);

        // Faceted highlights and shadows
        sf::VertexArray facets(sf::PrimitiveType::Triangles);
        for (std::size_t i = 0; i < n; ++i)
        {
            const std::size_t k = i % 3;
            if (k == 1)
            {
                continue;
            }
            const sf::Color c = k == 0 ? rgba(255, 255, 255, 34) : rgba(20, 10, 20, 60);
            facets.append(sf::Vertex{sf::Vector2f{0.0f, 0.0f}, c});
            facets.append(sf::Vertex{e.shape[i], c});
            facets.append(sf::Vertex{e.shape[(i + 1) % n], c});
        }
        target.draw(facets, s);

        // Craters
        for (std::size_t i = 0; i < n && i < 6; i += 3)
        {
            const float cr = e.radius * 0.13f;
            sf::CircleShape crater(cr, 10);
            crater.setOrigin(sf::Vector2f{cr, cr});
            crater.setPosition(sf::Vector2f{e.shape[i].x * 0.45f, e.shape[i].y * 0.45f});
            crater.setFillColor(rgba(40, 30, 44, 120));
            target.draw(crater, s);
        }

        // Dark rim
        strokePath(target, s, e.shape, 3.0f, dark, true);

        if (e.flash > 0.0f)
        {
            const int a = flashAlpha(e.flash, 170.0f);
            fillPolyGradient(target, s, e.shape, rgba(255, 255, 255, a), rgba(255, 255, 255, a));
        }
    }

    void BlasterGame::drawDrone(sf::RenderTarget &target, const Enemy &e) const
    {
        const float tilt = std::sin(e.age * 2.2f + e.phase) * 8.0f;
        const float t = clockTime();
        UiKit::drawGlow(target, e.pos, 64.0f, rgba(255, 110, 40, 70));

        // Saucer body and glass dome, both shaded
        UiKit::drawShadedEllipse(target, e.pos, sf::Vector2f{36.0f, 12.0f}, rgba(255, 206, 150), rgba(255, 96, 48),
                                 rgba(140, 22, 36), tilt);
        UiKit::drawShadedEllipse(target, sf::Vector2f{e.pos.x, e.pos.y - 10.0f}, sf::Vector2f{16.0f, 14.0f},
                                 rgba(240, 255, 255, 235), rgba(110, 220, 255, 215), rgba(30, 110, 200, 230), tilt);

        // Blinking lights along the rim
        sf::RenderStates s;
        s.transform.translate(e.pos).rotate(sf::degrees(tilt));
        for (int i = 0; i < 4; ++i)
        {
            const bool on = (static_cast<int>(t * 8.0f) + i) % 2 == 0;
            sf::CircleShape bulb(2.6f, 8);
            bulb.setOrigin(sf::Vector2f{2.6f, 2.6f});
            bulb.setPosition(sf::Vector2f{-24.0f + 16.0f * static_cast<float>(i), 6.0f});
            bulb.setFillColor(on ? rgba(255, 245, 140) : rgba(255, 150, 60, 120));
            target.draw(bulb, s);
        }

        if (e.flash > 0.0f)
        {
            UiKit::drawGlow(target, e.pos, 44.0f, rgba(255, 255, 255, flashAlpha(e.flash, 170.0f)));
        }
    }

    void BlasterGame::drawBomber(sf::RenderTarget &target, const Enemy &e) const
    {
        const float t = clockTime();
        const float pulse = 0.5f + 0.5f * std::sin(t * 5.0f + e.phase);
        const float tilt = std::sin(e.age * 1.1f + e.phase) * 6.0f;
        sf::RenderStates s;
        s.transform.translate(e.pos).rotate(sf::degrees(tilt));

        UiKit::drawGlow(target, e.pos, 96.0f, rgba(255, 70, 120, static_cast<int>(40.0f + 30.0f * pulse)));

        // Armoured hull with plates and a bright rim
        const std::vector<sf::Vector2f> hull = {
            sf::Vector2f{-64.0f, -6.0f}, sf::Vector2f{-40.0f, -38.0f}, sf::Vector2f{14.0f, -48.0f},
            sf::Vector2f{58.0f, -18.0f}, sf::Vector2f{66.0f, 6.0f},    sf::Vector2f{30.0f, 36.0f},
            sf::Vector2f{-30.0f, 40.0f}, sf::Vector2f{-62.0f, 22.0f}};
        fillPolyGradient(target, s, hull, rgba(170, 180, 210), rgba(44, 40, 78));
        strokePath(target, s, std::vector<sf::Vector2f>{hull[1], hull[2], hull[3]}, 2.5f, rgba(230, 236, 255, 210), false);
        strokePath(target, s, std::vector<sf::Vector2f>{sf::Vector2f{-20.0f, -36.0f}, sf::Vector2f{-12.0f, 38.0f}}, 1.5f,
                   rgba(20, 16, 40, 170), false);
        strokePath(target, s, std::vector<sf::Vector2f>{sf::Vector2f{22.0f, -40.0f}, sf::Vector2f{28.0f, 30.0f}}, 1.5f,
                   rgba(20, 16, 40, 170), false);
        strokePath(target, s, hull, 3.0f, rgba(20, 14, 40), true);

        // Pulsing core
        const float cr = 15.0f + 3.0f * pulse;
        fillPolyGradient(target, s, arcPoints(sf::Vector2f{0.0f, 0.0f}, cr, cr, 0.0f, 0.0f, kTwoPi, 24),
                         rgba(255, 236, 190), rgba(255, 60, 96));

        if (e.flash > 0.0f)
        {
            const int a = flashAlpha(e.flash, 170.0f);
            fillPolyGradient(target, s, hull, rgba(255, 255, 255, a), rgba(255, 255, 255, a));
        }
    }

    void BlasterGame::drawMothership(sf::RenderTarget &target, const Enemy &e) const
    {
        const float t = clockTime();
        const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
        const sf::RenderStates none;

        UiKit::drawGlow(target, e.pos, 260.0f, rgba(170, 90, 255, 46));

        // Underside shadow, shaded hull, rim
        fillPolyGradient(target, none,
                         arcPoints(sf::Vector2f{e.pos.x, e.pos.y + 14.0f}, 186.0f, 52.0f, 0.0f, 0.0f, kTwoPi, 48),
                         rgba(40, 20, 80, 160), rgba(40, 20, 80, 160));
        UiKit::drawShadedEllipse(target, e.pos, sf::Vector2f{190.0f, 64.0f}, rgba(232, 214, 255), rgba(120, 84, 196),
                                 rgba(44, 24, 96), 0.0f);
        const std::vector<sf::Vector2f> rim = arcPoints(e.pos, 190.0f, 64.0f, 0.0f, 0.0f, kTwoPi, 48);
        strokePath(target, none, rim, 4.0f, rgba(240, 206, 255, 150), true);

        // Dome
        UiKit::drawShadedEllipse(target, sf::Vector2f{e.pos.x, e.pos.y - 40.0f}, sf::Vector2f{84.0f, 50.0f},
                                 rgba(236, 255, 255, 240), rgba(110, 220, 255, 220), rgba(34, 96, 190, 230), 0.0f);

        // Ring of blinking lights
        for (int i = 0; i < 14; ++i)
        {
            const float a = kTwoPi * static_cast<float>(i) / 14.0f;
            const sf::Vector2f p = ellipsePt(e.pos, 170.0f, 54.0f, 0.0f, a);
            const bool on = (static_cast<int>(t * 6.0f) + i) % 3 == 0;
            sf::CircleShape bulb(on ? 4.5f : 3.0f, 10);
            bulb.setOrigin(sf::Vector2f{bulb.getRadius(), bulb.getRadius()});
            bulb.setPosition(p);
            bulb.setFillColor(on ? rgba(255, 220, 120) : rgba(255, 90, 170));
            target.draw(bulb);
        }

        // Pulsing reactor core under the hull
        const sf::Vector2f core{e.pos.x, e.pos.y + 22.0f};
        UiKit::drawGlow(target, core, 56.0f + 10.0f * pulse, rgba(255, 130, 90, static_cast<int>(120.0f + 80.0f * pulse)));
        UiKit::drawShadedEllipse(target, core, sf::Vector2f{22.0f, 10.0f}, rgba(255, 240, 200), rgba(255, 120, 80),
                                 rgba(200, 30, 70), 0.0f);

        if (e.flash > 0.0f)
        {
            const int a = flashAlpha(e.flash, 170.0f);
            fillPolyGradient(target, none, rim, rgba(255, 255, 255, a), rgba(255, 255, 255, a));
        }
    }

    void BlasterGame::drawBossBar(sf::RenderTarget &target) const
    {
        for (const Enemy &e : m_enemies)
        {
            if (!e.alive || e.kind != Kind::Mothership)
            {
                continue;
            }
            const float x = 380.0f;
            const float y = 152.0f;
            const float w = 520.0f;
            const float frac = std::clamp(static_cast<float>(e.hp) / static_cast<float>(e.maxHp), 0.0f, 1.0f);
            UiKit::drawRoundedRect(target, sf::FloatRect{{x - 4.0f, y - 4.0f}, {w + 8.0f, 22.0f}}, 10.0f,
                                   rgba(20, 8, 36, 220), rgba(255, 255, 255, 90), 2.0f);
            UiKit::drawRoundedRectGradient(target, sf::FloatRect{{x, y}, {w * frac, 14.0f}}, 7.0f,
                                           rgba(255, 120, 120), rgba(255, 200, 60));
            UiKit::drawLabel(target, font(), "MOTHERSHIP", 20, sf::Vector2f{640.0f, 186.0f}, rgba(255, 240, 250));
            return;
        }
    }

    void BlasterGame::drawShots(sf::RenderTarget &target) const
    {
        const Accent &acc = accent();
        const std::vector<sf::Vector2f> outer = capsulePoints(13.0f, 5.0f);
        const std::vector<sf::Vector2f> inner = capsulePoints(11.5f, 2.6f);
        for (const Shot &s : m_shots)
        {
            if (!s.alive)
            {
                continue;
            }
            const float angle = std::atan2(s.vel.y, s.vel.x) / kDegToRad;
            sf::RenderStates st;
            st.transform.translate(s.pos).rotate(sf::degrees(angle));
            UiKit::drawGlow(target, s.pos, 22.0f, fade(acc.base, 120.0f));
            fillPolyGradient(target, st, outer, acc.light, acc.base);
            fillPolyGradient(target, st, inner, rgba(255, 255, 255), rgba(255, 255, 255));
        }
    }

    void BlasterGame::drawEnemyShots(sf::RenderTarget &target) const
    {
        for (const EnemyShot &s : m_enemyShots)
        {
            if (!s.alive)
            {
                continue;
            }
            UiKit::drawGlow(target, s.pos, 24.0f, fade(s.color, 110.0f));
            sf::CircleShape core(6.0f, 12);
            core.setOrigin(sf::Vector2f{6.0f, 6.0f});
            core.setPosition(s.pos);
            core.setFillColor(mix(s.color, sf::Color::White, 0.7f));
            target.draw(core);
        }
    }

    void BlasterGame::drawPickups(sf::RenderTarget &target) const
    {
        for (const Pickup &p : m_pickups)
        {
            if (!p.alive)
            {
                continue;
            }
            const sf::Color col = pickupColor(p.letter);
            UiKit::drawGlow(target, p.pos, 52.0f, fade(col, 120.0f));

            // Glowing hexagon with a dark extrusion underneath
            sf::CircleShape under(24.0f, 6);
            under.setOrigin(sf::Vector2f{24.0f, 24.0f});
            under.setPosition(sf::Vector2f{p.pos.x, p.pos.y + 4.0f});
            under.setRotation(sf::degrees(p.rot));
            under.setFillColor(mix(col, sf::Color(20, 10, 60), 0.6f));
            target.draw(under);

            sf::CircleShape hex(24.0f, 6);
            hex.setOrigin(sf::Vector2f{24.0f, 24.0f});
            hex.setPosition(p.pos);
            hex.setRotation(sf::degrees(p.rot));
            hex.setFillColor(col);
            hex.setOutlineThickness(3.0f);
            hex.setOutlineColor(rgba(255, 255, 255, 200));
            target.draw(hex);

            UiKit::drawLabel(target, font(), std::string(1, p.letter), 24, p.pos, rgba(255, 255, 255), true);
        }
    }
} // namespace Flappy
