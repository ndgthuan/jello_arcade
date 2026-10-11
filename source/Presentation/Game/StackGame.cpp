#include "StackGame.h"
#include "UiKit.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kBlockH = 44.0f;
        constexpr float kFirstWidth = 360.0f;
        constexpr float kMaxWidth = 360.0f;
        constexpr float kWiden = 16.0f;
        constexpr float kBaseTop = 650.0f;   // world y of the base top edge (camera at 0)
        constexpr float kHover = 60.0f;      // gap between the sliding slab and the tower top
        constexpr float kMoveMin = 140.0f;   // centre x limits of the sliding slab
        constexpr float kMoveMax = 1140.0f;
        constexpr float kPerfectTol = 7.0f;
        constexpr float kCameraTop = 430.0f; // screen y of the tower top once the tower is tall
        constexpr float kCameraRate = 6.0f;
        constexpr float kShardGravity = 1500.0f;
        constexpr float kLandTime = 0.14f;
        constexpr float kRingLife = 0.55f;
        constexpr int kMaxHeight = 40;       // block count where the sky reaches full space
        constexpr int kStarCount = 70;
        constexpr int kSparkleCount = 14;
        constexpr int kCloudCount = 6;

        const sf::Color kInk(40, 12, 70);

        std::uint8_t toByte(float v)
        {
            return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 255.0f));
        }

        // HSV -> RGB: hue in degrees, s and v in 0..1
        sf::Color hsvColor(float hue, float s, float v)
        {
            const float h = std::fmod(hue, 360.0f) / 60.0f;
            const float c = v * s;
            const float x = c * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
            const float m = v - c;
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            if (h < 1.0f)
            {
                r = c;
                g = x;
            }
            else if (h < 2.0f)
            {
                r = x;
                g = c;
            }
            else if (h < 3.0f)
            {
                g = c;
                b = x;
            }
            else if (h < 4.0f)
            {
                g = x;
                b = c;
            }
            else if (h < 5.0f)
            {
                r = x;
                b = c;
            }
            else
            {
                r = c;
                b = x;
            }
            return sf::Color(toByte((r + m) * 255.0f), toByte((g + m) * 255.0f), toByte((b + m) * 255.0f));
        }

        // Rainbow colour of block i (hue 14 degrees per block)
        sf::Color slabColor(std::size_t index)
        {
            return hsvColor(static_cast<float>((index * 14) % 360), 0.74f, 1.0f);
        }

        // Stable pseudo random value in 0..1 for an index and a salt (no state, so the sky never changes)
        float hash01(int i, int salt)
        {
            std::uint32_t h = static_cast<std::uint32_t>(i) * 747796405u + static_cast<std::uint32_t>(salt) * 2891336453u + 1u;
            h ^= h >> 15;
            h *= 2246822519u;
            h ^= h >> 13;
            h *= 3266489917u;
            h ^= h >> 16;
            return static_cast<float>(h & 0xFFFFFFu) / 16777216.0f;
        }

        // Quad as two triangles (for a Triangles VertexArray)
        void appendQuad(sf::VertexArray &va, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
                        sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
        {
            va.append(sf::Vertex{a, ca});
            va.append(sf::Vertex{b, cb});
            va.append(sf::Vertex{c, cc});
            va.append(sf::Vertex{a, ca});
            va.append(sf::Vertex{c, cc});
            va.append(sf::Vertex{d, cd});
        }

        void appendQuad(sf::VertexArray &va, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
                        sf::Color color)
        {
            appendQuad(va, a, b, c, d, color, color, color, color);
        }

        // Local -> world transform of a slab: centre c, rotated by cs / sn around it
        struct Frame
        {
            sf::Vector2f c;
            float cs = 1.0f;
            float sn = 0.0f;

            sf::Vector2f operator()(float x, float y) const
            {
                return {c.x + x * cs - y * sn, c.y + x * sn + y * cs};
            }
        };

        // A glossy 3D slab centred on f.c: contact shadow, gradient face, darker right side, bright top bevel,
        // a glossy highlight and a darker bottom edge. `fade` multiplies every alpha (shards fading out).
        void appendSlab(sf::VertexArray &va, const Frame &f, float w, float h, sf::Color base, float fade)
        {
            auto col = [fade](sf::Color c)
            { return withAlpha(c, toByte(static_cast<float>(c.a) * fade)); };

            const sf::Color top = mix(base, sf::Color::White, 0.34f);
            const sf::Color side = mix(base, kInk, 0.45f);
            const sf::Color deep = mix(base, kInk, 0.6f);
            const float x0 = -w * 0.5f;
            const float x1 = w * 0.5f;
            const float y0 = -h * 0.5f;
            const float y1 = h * 0.5f;

            // Dark shadow under the slab, fading out
            appendQuad(va, f(x0 + 4.0f, y1), f(x1 - 2.0f, y1), f(x1 - 2.0f, y1 + 6.0f), f(x0 + 4.0f, y1 + 6.0f),
                       col(sf::Color(20, 6, 40, 120)), col(sf::Color(20, 6, 40, 120)), col(sf::Color(20, 6, 40, 0)),
                       col(sf::Color(20, 6, 40, 0)));

            // Face: lighter at the top, saturated at the bottom
            appendQuad(va, f(x0, y0), f(x1, y0), f(x1, y1), f(x0, y1), col(top), col(top), col(base), col(base));

            // Right-hand side: an 8 px darker parallelogram
            appendQuad(va, f(x1, y0), f(x1 + 8.0f, y0 - 4.0f), f(x1 + 8.0f, y1 - 4.0f), f(x1, y1), col(side), col(side),
                       col(side), col(side));

            // Bottom edge darkening
            appendQuad(va, f(x0, y1 - 6.0f), f(x1, y1 - 6.0f), f(x1, y1), f(x0, y1),
                       col(withAlpha(deep, 0)), col(withAlpha(deep, 0)), col(withAlpha(deep, 110)),
                       col(withAlpha(deep, 110)));

            if (w > 10.0f)
            {
                // Bright 4 px bevel along the top edge and a thin light edge on the left
                appendQuad(va, f(x0 + 3.0f, y0), f(x1 - 3.0f, y0), f(x1 - 3.0f, y0 + 4.0f), f(x0 + 3.0f, y0 + 4.0f),
                           col(sf::Color(255, 255, 255, 150)));
                appendQuad(va, f(x0, y0 + 4.0f), f(x0 + 2.0f, y0 + 4.0f), f(x0 + 2.0f, y1 - 2.0f), f(x0, y1 - 2.0f),
                           col(sf::Color(255, 255, 255, 60)));
            }
            if (w > 40.0f)
            {
                // Glossy highlight line
                appendQuad(va, f(x0 + 10.0f, y0 + 12.0f), f(x1 - 22.0f, y0 + 12.0f), f(x1 - 22.0f, y0 + 14.5f),
                           f(x0 + 10.0f, y0 + 14.5f), col(sf::Color(255, 255, 255, 120)));
            }
        }

        // Palette of the sky at one height: top, middle, horizon and bottom colour
        struct SkyStops
        {
            sf::Color top;
            sf::Color mid;
            sf::Color horizon;
            sf::Color bottom;
        };

        SkyStops blendSky(const SkyStops &a, const SkyStops &b, float k)
        {
            return {mix(a.top, b.top, k), mix(a.mid, b.mid, k), mix(a.horizon, b.horizon, k),
                    mix(a.bottom, b.bottom, k)};
        }

        // Warm dusk at height 0, deep purple halfway up, starry black-violet space near the top
        SkyStops skyAt(float p)
        {
            const SkyStops dusk{sf::Color(24, 40, 112), sf::Color(92, 52, 150), sf::Color(236, 92, 130),
                                sf::Color(255, 150, 80)};
            const SkyStops purple{sf::Color(40, 20, 96), sf::Color(80, 30, 130), sf::Color(150, 50, 150),
                                  sf::Color(110, 40, 120)};
            const SkyStops space{sf::Color(4, 3, 16), sf::Color(12, 6, 34), sf::Color(24, 12, 56),
                                 sf::Color(34, 14, 70)};
            if (p < 0.5f)
            {
                return blendSky(dusk, purple, p * 2.0f);
            }
            return blendSky(purple, space, (p - 0.5f) * 2.0f);
        }
    } // namespace

    StackGame::StackGame(const sf::Font &font) : ArcadeGame(font, 4, 4)
    {
        onStart();
    }

    void StackGame::onStart()
    {
        m_tower.clear();
        m_tower.push_back(Slab{640.0f, kBaseTop, kFirstWidth, 2.0f});
        m_shards.clear();
        m_rings.clear();
        m_moveX = kMoveMin;
        m_moveDir = 1.0f;
        m_moving = true;
        m_cameraY = 0.0f;

        m_combo = 0;
        setCombo(0);
        setComboWord("PERFECT");
        setLives(0, 0);
    }

    bool StackGame::isStartKey(sf::Keyboard::Key key) const
    {
        return key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Enter ||
               key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Down;
    }

    void StackGame::onKeyPressed(sf::Keyboard::Key key)
    {
        if (isStartKey(key))
        {
            drop();
        }
    }

    void StackGame::onMousePressed(sf::Vector2f, sf::Mouse::Button button)
    {
        if (button == sf::Mouse::Button::Left)
        {
            drop();
        }
    }

    void StackGame::onUpdate(float dt)
    {
        if (m_moving)
        {
            const int count = std::min(static_cast<int>(m_tower.size()) - 1, 40);
            const float speed = 380.0f + 14.0f * static_cast<float>(count) + 60.0f * static_cast<float>(difficulty());
            m_moveX += m_moveDir * speed * dt;
            if (m_moveX >= kMoveMax)
            {
                m_moveX = kMoveMax;
                m_moveDir = -1.0f;
            }
            else if (m_moveX <= kMoveMin)
            {
                m_moveX = kMoveMin;
                m_moveDir = 1.0f;
            }
        }
        animate(dt);
    }

    void StackGame::onIdle(float dt)
    {
        animate(dt);
    }

    void StackGame::drop()
    {
        // The start key also reaches here, so ignore drops right after the run begins
        if (!m_moving || runTime() < 0.25f)
        {
            return;
        }

        const Slab top = m_tower.back();
        const float moveTop = top.y - kBlockH - kHover; // world top of the sliding slab
        const float moveMid = moveTop + kBlockH * 0.5f + m_cameraY;
        const float moveLeft = m_moveX - top.w * 0.5f;
        const float moveRight = m_moveX + top.w * 0.5f;
        const float left = std::max(moveLeft, top.cx - top.w * 0.5f);
        const float right = std::min(moveRight, top.cx + top.w * 0.5f);
        const sf::Color color = slabColor(m_tower.size());

        if (right - left <= 0.0f)
        {
            // No overlap: the whole slab falls past the tower
            spawnShard(m_moveX, moveTop, top.w, color, rnd(-40.0f, 40.0f), rnd(-140.0f, 140.0f));
            m_moving = false;
            gameOver({m_moveX, moveMid}, 0.8f);
            return;
        }

        const bool perfect = std::fabs(m_moveX - top.cx) < kPerfectTol;
        Slab placed;
        placed.y = top.y - kBlockH;
        placed.age = 0.0f;

        if (perfect)
        {
            // Snapped exactly on top, nothing is cut
            placed.cx = top.cx;
            placed.w = top.w;
        }
        else
        {
            placed.cx = (left + right) * 0.5f;
            placed.w = right - left;

            // The overhang falls away as shards
            if (left > moveLeft)
            {
                const float cut = (moveLeft + left) * 0.5f;
                spawnShard(cut, moveTop, left - moveLeft, color, rnd(-120.0f, -60.0f), -rnd(50.0f, 110.0f));
                fx().spawnBurst({cut, moveMid}, color, 10, 1.0f);
            }
            if (right < moveRight)
            {
                const float cut = (right + moveRight) * 0.5f;
                spawnShard(cut, moveTop, moveRight - right, color, rnd(60.0f, 120.0f), rnd(50.0f, 110.0f));
                fx().spawnBurst({cut, moveMid}, color, 10, 1.0f);
            }
            audio().playHit();
        }

        bool widened = false;
        if (perfect)
        {
            ++m_combo;
            if (m_combo % 3 == 0 && placed.w < kMaxWidth)
            {
                placed.w = std::min(kMaxWidth, placed.w + kWiden);
                widened = true;
            }
        }
        else
        {
            m_combo = 0;
        }
        setCombo(m_combo);
        m_tower.push_back(placed);

        audio().playPop(1.0f + 0.04f * static_cast<float>(std::min(m_combo, 12)));

        // Effects are drawn without the camera offset, so convert the world position to screen space
        const sf::Vector2f topCentre{placed.cx, placed.y + m_cameraY};
        if (perfect)
        {
            addScore(2);
            setComboWord("PERFECT");
            m_rings.push_back(Ring{placed.cx, placed.y, placed.w, 0.0f});
            audio().playPower();
            fx().spawnText({topCentre.x, topCentre.y - 28.0f}, "PERFECT!");
            fx().spawnBurst(topCentre, sf::Color(255, 250, 225), 24, 1.4f);
            fx().addShake(5.0f);
            if (widened)
            {
                fx().spawnText({topCentre.x, topCentre.y - 64.0f}, "WIDER!");
            }
        }
        else
        {
            addScore(1);
            fx().addShake(1.5f);
        }
    }

    void StackGame::animate(float dt)
    {
        for (Slab &s : m_tower)
        {
            s.age = std::min(s.age + dt, 2.0f);
        }

        // The camera eases so the tower top settles at kCameraTop once the tower is tall enough
        const float target = std::max(0.0f, kCameraTop - m_tower.back().y);
        m_cameraY += (target - m_cameraY) * (1.0f - std::exp(-kCameraRate * dt));

        for (Shard &s : m_shards)
        {
            s.age += dt;
            s.vy += kShardGravity * dt;
            s.cx += s.vx * dt;
            s.y += s.vy * dt;
            s.rot += s.spin * dt;
        }
        std::erase_if(m_shards, [this](const Shard &s)
                      { return s.age > 6.0f || s.y + m_cameraY > kHeight + 200.0f; });

        for (Ring &r : m_rings)
        {
            r.age += dt;
        }
        std::erase_if(m_rings, [](const Ring &r)
                      { return r.age >= kRingLife; });
    }

    void StackGame::spawnShard(float cx, float y, float w, sf::Color color, float vx, float spin)
    {
        Shard s;
        s.cx = cx;
        s.y = y;
        s.w = w;
        s.vx = vx;
        s.spin = spin;
        s.color = color;
        m_shards.push_back(s);
    }

    float StackGame::rnd()
    {
        m_seed = m_seed * 1664525u + 1013904223u;
        return static_cast<float>(m_seed >> 8) / 16777216.0f;
    }

    float StackGame::rnd(float lo, float hi)
    {
        return lo + (hi - lo) * rnd();
    }

    // ------------------------------------------------------------------ drawing

    void StackGame::drawBackground(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        const float p = std::clamp(static_cast<float>(m_tower.size() - 1) / static_cast<float>(kMaxHeight), 0.0f, 1.0f);
        const float par = m_cameraY * 0.2f; // parallax: the backdrop moves a fifth of the camera
        const SkyStops sky = skyAt(p);

        // Vertical sky gradient (a little bigger than the view so screen shake never shows an edge)
        sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
        const float stopY[4] = {-40.0f, 220.0f, kHorizonY, 760.0f};
        const sf::Color stopColor[4] = {sky.top, sky.mid, sky.horizon, sky.bottom};
        for (int i = 0; i < 4; ++i)
        {
            strip.append(sf::Vertex{{-20.0f, stopY[i]}, stopColor[i]});
            strip.append(sf::Vertex{{kWidth + 20.0f, stopY[i]}, stopColor[i]});
        }
        target.draw(strip);

        // Dusk glow on the horizon fades out as the tower climbs
        UiKit::drawGlow(target, {kWidth * 0.5f, kHorizonY}, 620.0f,
                        withAlpha(sf::Color(255, 170, 90), toByte(70.0f * (1.0f - p))));

        // Stars: each one appears once the tower is high enough, and twinkles
        for (int i = 0; i < kStarCount; ++i)
        {
            const float threshold = 0.85f * hash01(i, 4);
            const float visible = std::clamp((p - threshold) * 4.0f, 0.0f, 1.0f);
            if (visible <= 0.0f)
            {
                continue;
            }
            const float r = 0.9f + 1.6f * hash01(i, 3);
            const float twinkle = 0.6f + 0.4f * std::sin(t * 2.2f + 6.28f * hash01(i, 5));
            const sf::Color c = hash01(i, 6) < 0.3f ? sf::Color(190, 215, 255) : sf::Color::White;
            sf::CircleShape star(r, 8);
            star.setOrigin({r, r});
            star.setPosition({hash01(i, 1) * kWidth, hash01(i, 2) * 440.0f + par});
            star.setFillColor(withAlpha(c, toByte(255.0f * visible * twinkle)));
            target.draw(star);
        }

        // Big soft moon
        const sf::Vector2f moon{1000.0f, 150.0f + par};
        UiKit::drawGlow(target, moon, 360.0f, withAlpha(sf::Color(255, 226, 160), 26));
        UiKit::drawGlow(target, moon, 170.0f, withAlpha(sf::Color(255, 240, 200), 70));
        UiKit::drawShadedEllipse(target, moon, {44.0f, 44.0f}, sf::Color(255, 255, 240), sf::Color(250, 236, 196),
                                 sf::Color(222, 196, 150));

        // Translucent clouds drifting at low heights
        const float cloudAlpha = std::clamp((0.6f - p) * 4.0f, 0.0f, 1.0f);
        if (cloudAlpha > 0.0f)
        {
            const float span = kWidth + 400.0f;
            for (int i = 0; i < kCloudCount; ++i)
            {
                const float speed = 10.0f + 14.0f * hash01(i, 21);
                const float x = std::fmod(hash01(i, 22) * span + t * speed, span) - 200.0f;
                const float y = 300.0f + 240.0f * hash01(i, 23) + par;
                const sf::Color c = withAlpha(sf::Color(255, 214, 236), toByte(46.0f * cloudAlpha));
                UiKit::drawGlow(target, {x, y}, 78.0f, c);
                UiKit::drawGlow(target, {x + 56.0f, y - 12.0f}, 58.0f, c);
                UiKit::drawGlow(target, {x - 54.0f, y + 8.0f}, 54.0f, c);
            }
        }

        // Sparkles rising through the sky, more of them the higher the tower
        for (int i = 0; i < kSparkleCount; ++i)
        {
            const float speed = 18.0f + 30.0f * hash01(i, 11);
            const float y = kHeight - std::fmod(hash01(i, 12) * kHeight + t * speed, kHeight);
            const float x = hash01(i, 13) * kWidth;
            const float a = std::clamp(p * 2.5f - 0.1f, 0.0f, 1.0f) *
                            (0.45f + 0.55f * std::sin(t * 3.0f + static_cast<float>(i)));
            if (a <= 0.02f)
            {
                continue;
            }
            const sf::Color c = (i % 2 == 0) ? sf::Color(255, 230, 150) : sf::Color(150, 230, 255);
            UiKit::drawSparkle(target, {x, y}, 4.0f + 5.0f * hash01(i, 14), withAlpha(c, toByte(255.0f * a)),
                               45.0f * static_cast<float>(i));
        }
    }

    void StackGame::drawWorld(sf::RenderTarget &target) const
    {
        const float cam = m_cameraY;

        // Stone pedestal under the base
        UiKit::drawGlow(target, {kWidth * 0.5f, kBaseTop + kBlockH + cam}, 460.0f,
                        withAlpha(sf::Color(160, 90, 255), 40));
        UiKit::drawRoundedRectGradient(target,
                                       sf::FloatRect({180.0f, kBaseTop + kBlockH - 8.0f + cam}, {920.0f, 160.0f}),
                                       30.0f, sf::Color(104, 66, 170), sf::Color(34, 18, 74));

        // Tower and falling shards, one batch
        sf::VertexArray solids(sf::PrimitiveType::Triangles);
        for (std::size_t i = 0; i < m_tower.size(); ++i)
        {
            const Slab &s = m_tower[i];
            // Landing: the slab falls the last few pixels with an ease-in, then squashes and springs back
            const float p = std::min(s.age / kLandTime, 1.0f);
            const float offset = -kHover * (1.0f - p * p);
            const float squash = s.age > kLandTime ? 0.14f * std::exp(-(s.age - kLandTime) * 16.0f) : 0.0f;
            const float h = kBlockH * (1.0f - squash);
            const float w = s.w * (1.0f + 0.6f * squash);
            const float bottom = s.y + kBlockH + offset + cam;
            if (bottom < -120.0f || bottom - kBlockH > kHeight + 120.0f)
            {
                continue; // off screen
            }
            appendSlab(solids, Frame{{s.cx, bottom - h * 0.5f}}, w, h, slabColor(i), 1.0f);
        }
        for (const Shard &s : m_shards)
        {
            const float sy = s.y + kBlockH * 0.5f + cam;
            const float fade = std::clamp((kHeight + 60.0f - sy) / 100.0f, 0.0f, 1.0f); // fades below the screen
            if (fade <= 0.0f)
            {
                continue;
            }
            const float rad = s.rot * kPi / 180.0f;
            appendSlab(solids, Frame{{s.cx, sy}, std::cos(rad), std::sin(rad)}, s.w, kBlockH, s.color, fade);
        }
        target.draw(solids);

        // Perfect-drop frames: a white outline that grows and fades
        for (const Ring &r : m_rings)
        {
            const float t = std::clamp(r.age / kRingLife, 0.0f, 1.0f);
            const float grow = 4.0f + 50.0f * t;
            const sf::FloatRect rect({r.cx - r.w * 0.5f - grow, r.y + cam - grow}, {r.w + grow * 2.0f, kBlockH + grow * 2.0f});
            const float a = std::pow(1.0f - t, 1.5f);
            UiKit::drawRoundedRect(target, rect, 16.0f + grow * 0.5f, sf::Color::Transparent,
                                   sf::Color(255, 255, 255, toByte(255.0f * a)), 2.0f + 4.0f * (1.0f - t));
        }

        // The sliding slab with its glow and a dashed guide down to the tower top
        if (m_moving)
        {
            const Slab &top = m_tower.back();
            const float moveTop = top.y - kBlockH - kHover;
            const sf::Color color = slabColor(m_tower.size());
            const sf::Vector2f centre{m_moveX, moveTop + kBlockH * 0.5f + cam};
            UiKit::drawGlow(target, centre, top.w * 0.5f + 110.0f, withAlpha(color, 90));

            sf::VertexArray moving(sf::PrimitiveType::Triangles);
            const float guideEnd = top.y + cam;
            for (float y = moveTop + kBlockH + cam + 4.0f; y < guideEnd; y += 14.0f)
            {
                const float y2 = std::min(y + 8.0f, guideEnd);
                appendQuad(moving, {m_moveX - 1.0f, y}, {m_moveX + 1.0f, y}, {m_moveX + 1.0f, y2},
                           {m_moveX - 1.0f, y2}, sf::Color(255, 255, 255, 60));
            }
            appendSlab(moving, Frame{centre}, top.w, kBlockH, color, 1.0f);
            target.draw(moving);
        }
    }
} // namespace Flappy
