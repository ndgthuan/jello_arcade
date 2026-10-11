#include "SmashGame.h"
#include "UiKit.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numbers>
#include <string>

namespace Flappy
{
    namespace
    {
        // Logical 1280x720 space; the HUD owns the top 130 px
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kLeftWall = 30.0f;
        constexpr float kRightWall = 1250.0f;
        constexpr float kTopWall = 130.0f;
        constexpr float kBottomLine = 740.0f; // a ball below this line is lost
        constexpr float kPaddleY = 650.0f;
        constexpr float kPaddleH = 26.0f;
        constexpr float kPaddleNormalW = 150.0f;
        constexpr float kPaddleWideW = 230.0f;
        constexpr float kPaddleSpeed = 900.0f; // keyboard steering, px/s
        constexpr float kBallRadius = 14.0f;
        constexpr float kBaseSpeed = 480.0f;
        constexpr float kMaxBounceAngle = kPi / 3.0f; // paddle edge bounce: 60 degrees from vertical
        constexpr float kMinUpward = 0.35f;           // the vertical share of the speed never drops below this
        constexpr int kCols = 10;
        constexpr int kRows = 6;
        constexpr int kLayouts = 4;
        constexpr float kBrickW = 112.0f;
        constexpr float kBrickH = 34.0f;
        constexpr float kBrickGap = 6.0f;
        constexpr float kBrickLeft = 61.0f;
        constexpr float kBrickTop = 175.0f;
        constexpr float kBrickRadius = 9.0f;
        constexpr float kBombRadius = 130.0f;
        constexpr float kBombChance = 0.06f;
        constexpr float kDropChance = 0.12f;
        constexpr float kCapsuleFall = 150.0f;
        constexpr float kWideTime = 12.0f;
        constexpr float kSlowTime = 8.0f;
        constexpr int kMaxLives = 5;
        constexpr std::size_t kMaxBalls = 6;
        constexpr int kSubsteps = 4; // physics steps per frame (no tunnelling through bricks)
        constexpr std::size_t kOrbCount = 40;
        constexpr float kBannerTime = 1.8f;

        constexpr sf::Color kRowColors[kRows] = {
            sf::Color(255, 90, 143), // pink
            sf::Color(255, 138, 43), // orange
            sf::Color(255, 210, 63), // yellow
            sf::Color(124, 255, 91), // green
            sf::Color(79, 216, 255), // cyan
            sf::Color(168, 110, 255) // violet
        };
        constexpr sf::Color kBombColor(230, 40, 60);
        constexpr sf::Color kOrbColors[4] = {
            sf::Color(255, 90, 143), sf::Color(79, 216, 255), sf::Color(150, 90, 255), sf::Color(255, 210, 63)};

        std::uint8_t byteOf(float unit)
        {
            return static_cast<std::uint8_t>(std::clamp(unit, 0.0f, 1.0f) * 255.0f + 0.5f);
        }

        sf::Color shade(sf::Color c, float amount)
        {
            return mix(c, sf::Color(20, 10, 60), amount);
        }

        sf::Color brighten(sf::Color c, float amount)
        {
            return mix(c, sf::Color::White, amount);
        }

        sf::Vector2f centerOf(sf::FloatRect r)
        {
            return sf::Vector2f{r.position.x + r.size.x * 0.5f, r.position.y + r.size.y * 0.5f};
        }

        sf::FloatRect inflate(sf::FloatRect r, float d)
        {
            return sf::FloatRect({r.position.x - d, r.position.y - d}, {r.size.x + 2.0f * d, r.size.y + 2.0f * d});
        }

        sf::Vector2f rotated(sf::Vector2f v, float rad)
        {
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            return sf::Vector2f{v.x * c - v.y * s, v.x * s + v.y * c};
        }

        sf::Color capsuleColor(char letter)
        {
            switch (letter)
            {
            case 'W':
                return sf::Color(79, 216, 255);
            case 'M':
                return sf::Color(255, 90, 143);
            case 'S':
                return sf::Color(124, 255, 91);
            default:
                return sf::Color(255, 210, 63);
            }
        }

        // Brick layouts: 0 full rows, 1 checkerboard, 2 pyramid (narrow at the top), 3 hollow frame
        bool brickPresent(int layout, int row, int col)
        {
            switch (layout)
            {
            case 1:
                return (row + col) % 2 == 0;
            case 2:
                return std::abs(2 * col - (kCols - 1)) <= 2 * row + 1;
            case 3:
                return row == 0 || row == kRows - 1 || col == 0 || col == kCols - 1;
            default:
                return true;
            }
        }

        sf::FloatRect brickRect(int row, int col)
        {
            const float x = kBrickLeft + static_cast<float>(col) * (kBrickW + kBrickGap);
            const float y = kBrickTop + static_cast<float>(row) * (kBrickH + kBrickGap);
            return sf::FloatRect({x, y}, {kBrickW, kBrickH});
        }

        // Backdrop gradient: deep indigo at the top, violet glow toward the floor
        sf::Color backdropColor(float t)
        {
            struct Stop
            {
                float at;
                sf::Color color;
            };
            const Stop stops[4] = {
                {0.0f, sf::Color(12, 6, 44)},
                {0.45f, sf::Color(38, 14, 96)},
                {0.62f, sf::Color(90, 28, 146)},
                {1.0f, sf::Color(54, 16, 110)}};
            for (std::size_t i = 1; i < 4; ++i)
            {
                if (t <= stops[i].at)
                {
                    const float k = (t - stops[i - 1].at) / (stops[i].at - stops[i - 1].at);
                    return mix(stops[i - 1].color, stops[i].color, k);
                }
            }
            return stops[3].color;
        }

        // Quad a-b-c-d as two triangles; a and b get the `top` colour, c and d the `bottom` colour
        void addQuad(sf::VertexArray &va, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
                     sf::Color top, sf::Color bottom)
        {
            va.append(sf::Vertex{a, top});
            va.append(sf::Vertex{b, top});
            va.append(sf::Vertex{c, bottom});
            va.append(sf::Vertex{a, top});
            va.append(sf::Vertex{c, bottom});
            va.append(sf::Vertex{d, bottom});
        }

        // Floor line from the horizon (top x, width) to the bottom edge (bottom x, width)
        void addFloorLine(sf::VertexArray &va, float horizonY, float topX, float botX, float topW, float botW,
                          sf::Color top, sf::Color bottom)
        {
            addQuad(va, sf::Vector2f{topX - topW * 0.5f, horizonY}, sf::Vector2f{topX + topW * 0.5f, horizonY},
                    sf::Vector2f{botX + botW * 0.5f, kHeight}, sf::Vector2f{botX - botW * 0.5f, kHeight}, top,
                    bottom);
        }

        // Thin line segment as a quad
        void strokeLine(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color)
        {
            const sf::Vector2f d = b - a;
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 0.001f)
            {
                return;
            }
            const sf::Vector2f n = sf::Vector2f{-d.y, d.x} * (thickness * 0.5f / len);
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 4);
            va[0] = sf::Vertex{a + n, color};
            va[1] = sf::Vertex{a - n, color};
            va[2] = sf::Vertex{b + n, color};
            va[3] = sf::Vertex{b - n, color};
            target.draw(va);
        }

        // Crack lines across a damaged brick
        void drawCracks(sf::RenderTarget &target, sf::FloatRect r)
        {
            const sf::Color line(40, 10, 30, 210);
            const float x = r.position.x;
            const float y = r.position.y;
            const float w = r.size.x;
            const float h = r.size.y;
            const sf::Vector2f p0{x + w * 0.30f, y};
            const sf::Vector2f p1{x + w * 0.42f, y + h * 0.36f};
            const sf::Vector2f p2{x + w * 0.36f, y + h * 0.62f};
            const sf::Vector2f p3{x + w * 0.56f, y + h};
            strokeLine(target, p0, p1, 2.2f, line);
            strokeLine(target, p1, p3, 2.2f, line);
            strokeLine(target, p1, sf::Vector2f{x + w * 0.72f, y + h * 0.42f}, 1.6f, line);
            strokeLine(target, p2, sf::Vector2f{x + w * 0.18f, y + h * 0.74f}, 1.6f, line);
        }

        // Neon bar: wide faint halo, mid glow, bright core
        void drawNeonBar(sf::RenderTarget &target, sf::FloatRect bar, sf::Color neon, float pulse)
        {
            UiKit::drawRoundedRect(target, inflate(bar, 9.0f), 9.0f, withAlpha(neon, byteOf(0.16f * pulse)));
            UiKit::drawRoundedRect(target, inflate(bar, 4.0f), 5.0f, withAlpha(neon, byteOf(0.5f * pulse)));
            UiKit::drawRoundedRect(target, bar, 2.0f, brighten(neon, 0.7f));
        }

        // Neon perspective floor: rays from a vanishing point and rows that roll toward the viewer
        void drawGrid(sf::RenderTarget &target, float offset, sf::Color color)
        {
            constexpr float kHorizon = 430.0f;
            constexpr float kVanishY = 380.0f;
            constexpr float kSpacing = 120.0f;
            const float vpX = kWidth * 0.5f;
            const float factorTop = (kHorizon - kVanishY) / (kHeight - kVanishY);
            const float shift = offset * kSpacing;

            sf::VertexArray va(sf::PrimitiveType::Triangles);
            for (int i = -12; i <= 12; ++i)
            {
                const float xb = vpX + static_cast<float>(i) * kSpacing - shift;
                const float xt = vpX + (xb - vpX) * factorTop;
                // Wide faint glow, then the thin bright core; both brighten toward the bottom edge
                addFloorLine(va, kHorizon, xt, xb, 4.0f, 10.0f, withAlpha(color, 12), withAlpha(color, 45));
                addFloorLine(va, kHorizon, xt, xb, 1.2f, 3.0f, withAlpha(color, 40), withAlpha(color, 220));
            }

            for (int k = 0; k < 40; ++k)
            {
                const float t = static_cast<float>(k) + offset;
                const float y = kHorizon + 1.6f * t * t;
                if (y > kHeight)
                {
                    break;
                }
                const float k01 = std::clamp((y - kHorizon) / (kHeight - kHorizon), 0.0f, 1.0f);
                const sf::Color glow = withAlpha(color, byteOf(0.15f * k01));
                const sf::Color core = withAlpha(color, byteOf(0.9f * k01));
                const float gh = 3.0f + 3.0f * k01;
                const float ch = 0.6f + 0.8f * k01;
                addQuad(va, sf::Vector2f{0.0f, y - gh}, sf::Vector2f{kWidth, y - gh}, sf::Vector2f{kWidth, y + gh},
                        sf::Vector2f{0.0f, y + gh}, glow, glow);
                addQuad(va, sf::Vector2f{0.0f, y - ch}, sf::Vector2f{kWidth, y - ch}, sf::Vector2f{kWidth, y + ch},
                        sf::Vector2f{0.0f, y + ch}, core, core);
            }
            target.draw(va);
        }
    } // namespace

    SmashGame::SmashGame(const sf::Font &font) : ArcadeGame(font, 1, 1)
    {
        // Ambient glow orbs, placed once from the fixed generator (the same layout every run)
        m_orbs.reserve(kOrbCount);
        for (std::size_t i = 0; i < kOrbCount; ++i)
        {
            Orb orb;
            orb.pos = sf::Vector2f{rnd() * kWidth, rnd() * kHeight};
            orb.vel = sf::Vector2f{-22.0f + 44.0f * rnd(), -30.0f + 60.0f * rnd()};
            orb.radius = 30.0f + 80.0f * rnd();
            orb.color = kOrbColors[i % 4];
            orb.alpha = static_cast<std::uint8_t>(40.0f + 50.0f * rnd());
            m_orbs.push_back(orb);
        }
        onStart(); // start() would begin the music at program start-up
    }

    // ---------------------------------------------------------------- hooks

    void SmashGame::onStart()
    {
        m_level = 1;
        m_lives = 3;
        m_combo = 0;
        m_wideTimer = 0.0f;
        m_slowTimer = 0.0f;
        m_speedMul = 1.0f;
        m_flash = 0.0f;
        m_levelBanner = 0.0f;
        m_paddleW = kPaddleNormalW;
        m_paddleSquash = 0.0f;
        m_paddleX = kWidth * 0.5f;
        m_launchSign = 1;
        m_capsules.clear();

        buildLevel();
        m_balls.clear();
        attachBall();

        setLives(m_lives, 3);
        setCombo(0);
        setComboWord("COMBO");
    }

    void SmashGame::onBegin()
    {
        launchBalls();
    }

    void SmashGame::onIdle(float dt)
    {
        animate(std::min(dt, 0.05f));
        placeAttachedBalls();
    }

    void SmashGame::onKeyPressed(sf::Keyboard::Key key)
    {
        if (key == sf::Keyboard::Key::Space)
        {
            launchBalls();
        }
    }

    void SmashGame::onMouseMoved(sf::Vector2f point)
    {
        m_paddleX = clampPaddle(point.x);
    }

    void SmashGame::onMousePressed(sf::Vector2f, sf::Mouse::Button)
    {
        launchBalls();
    }

    void SmashGame::onUpdate(float dt)
    {
        dt = std::min(dt, 0.05f);
        animate(dt);

        // Keyboard steering (the mouse sets the paddle directly in onMouseMoved)
        float dir = 0.0f;
        if (keyDown(sf::Keyboard::Key::Left) || keyDown(sf::Keyboard::Key::A))
        {
            dir -= 1.0f;
        }
        if (keyDown(sf::Keyboard::Key::Right) || keyDown(sf::Keyboard::Key::D))
        {
            dir += 1.0f;
        }
        m_paddleX = clampPaddle(m_paddleX + dir * kPaddleSpeed * dt);

        // Power-up timers, eased paddle width and ball speed
        m_wideTimer = std::max(0.0f, m_wideTimer - dt);
        m_slowTimer = std::max(0.0f, m_slowTimer - dt);
        const float wantWidth = m_wideTimer > 0.0f ? kPaddleWideW : kPaddleNormalW;
        m_paddleW += (wantWidth - m_paddleW) * std::min(1.0f, 10.0f * dt);
        const float wantSpeed = m_slowTimer > 0.0f ? 0.6f : 1.0f;
        m_speedMul += (wantSpeed - m_speedMul) * std::min(1.0f, 4.0f * dt);
        m_paddleX = clampPaddle(m_paddleX);

        placeAttachedBalls();
        for (int s = 0; s < kSubsteps; ++s)
        {
            for (std::size_t i = 0; i < m_balls.size(); ++i)
            {
                if (!m_balls[i].attached)
                {
                    advanceBall(m_balls[i], dt / static_cast<float>(kSubsteps));
                }
            }
        }

        // Balls that fell past the paddle are lost; the last one costs a life
        const std::size_t before = m_balls.size();
        std::erase_if(m_balls, [](const Ball &b)
                      { return !b.attached && b.pos.y > kBottomLine; });
        if (m_balls.empty())
        {
            loseLife();
            if (m_lives <= 0)
            {
                return;
            }
        }
        else if (m_balls.size() != before)
        {
            fx().addShake(3.0f);
        }

        // Trails: shift the history one slot per frame
        for (Ball &ball : m_balls)
        {
            for (std::size_t k = ball.trail.size() - 1; k > 0; --k)
            {
                ball.trail[k] = ball.trail[k - 1];
            }
            ball.trail[0] = ball.pos;
        }

        updateCapsules(dt);

        if (aliveBricks() == 0)
        {
            nextLevel();
        }
    }

    // ---------------------------------------------------------------- drawing

    void SmashGame::drawBackground(sf::RenderTarget &target) const
    {
        const float t = clockTime();

        // Vertical gradient
        sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
        for (int i = 0; i <= 12; ++i)
        {
            const float y = kHeight * static_cast<float>(i) / 12.0f;
            const sf::Color c = backdropColor(y / kHeight);
            sky.append(sf::Vertex{sf::Vector2f{0.0f, y}, c});
            sky.append(sf::Vertex{sf::Vector2f{kWidth, y}, c});
        }
        target.draw(sky);

        drawGrid(target, m_gridOffset, mix(accent().light, sf::Color(90, 245, 255), 0.5f));

        // Drifting glow orbs
        for (const Orb &orb : m_orbs)
        {
            UiKit::drawGlow(target, orb.pos, orb.radius, withAlpha(orb.color, orb.alpha));
        }

        // Pulsing neon frame around the play field (brighter on every brick break)
        const float pulse = std::clamp(0.45f + 0.35f * std::sin(t * 2.4f) + 0.9f * m_flash, 0.0f, 1.0f);
        const sf::Color frameColor = mix(accent().base, sf::Color(170, 90, 255), 0.5f);
        const sf::FloatRect frame({kLeftWall - 12.0f, kTopWall - 12.0f},
                                  {kRightWall - kLeftWall + 24.0f, 640.0f});
        UiKit::drawRoundedRect(target, frame, 26.0f, sf::Color::Transparent, withAlpha(frameColor, byteOf(0.35f * pulse)),
                               14.0f);
        UiKit::drawRoundedRect(target, frame, 26.0f, sf::Color::Transparent,
                               withAlpha(brighten(frameColor, 0.4f), byteOf(0.8f * pulse)), 3.0f);
    }

    void SmashGame::drawWorld(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        const float pulse = std::clamp(0.85f + 0.15f * std::sin(t * 2.0f) + 0.6f * m_flash, 0.0f, 1.0f);
        const sf::Color neon = accent().base;
        const float railH = kBottomLine - kTopWall;

        // Neon rails
        drawNeonBar(target, sf::FloatRect({kLeftWall - 2.0f, kTopWall}, {4.0f, railH}), neon, pulse);
        drawNeonBar(target, sf::FloatRect({kRightWall - 2.0f, kTopWall}, {4.0f, railH}), neon, pulse);
        drawNeonBar(target, sf::FloatRect({kLeftWall - 2.0f, kTopWall - 2.0f}, {kRightWall - kLeftWall + 4.0f, 4.0f}),
                    neon, pulse);

        drawBricks(target);
        drawCapsules(target);
        drawPaddle(target);
        drawBalls(target);

        // Level counter under the HUD
        UiKit::drawLabel(target, font(), "LEVEL " + std::to_string(m_level), 22, sf::Vector2f{640.0f, 140.0f},
                         accent().light, true, 2.0f);

        // Big "LEVEL n" banner after a level clear: pops in, then fades out
        if (m_levelBanner > 0.0f)
        {
            const float life = m_levelBanner / kBannerTime; // 1 -> 0
            const float intro = std::clamp((1.0f - life) * 4.0f, 0.0f, 1.0f);
            const float scale = 1.3f - 0.3f * intro;
            const auto alpha = byteOf(std::min(1.0f, life * 2.0f));
            UiKit::drawExtrudedText(target, font(), "LEVEL " + std::to_string(m_level), 84,
                                    sf::Vector2f{640.0f, 330.0f}, UiKit::orangeStyle(), scale, 0.0f, alpha);
        }
    }

    void SmashGame::drawBricks(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        for (const Brick &b : m_bricks)
        {
            if (!b.alive)
            {
                continue;
            }
            const sf::FloatRect r = b.rect;
            const float x = r.position.x;
            const float y = r.position.y;
            const float w = r.size.x;
            const float h = r.size.y;
            const sf::Color base = brighten(b.bomb ? kBombColor : b.color, 0.5f * b.flash);
            const sf::Color dark = shade(base, 0.6f);

            // Edge slab underneath (the 3D thickness)
            UiKit::drawRoundedRect(target, sf::FloatRect({x, y + 5.0f}, {w, h}), kBrickRadius, dark);
            // Glossy face: light at the top, base colour at the bottom
            UiKit::drawRoundedRectGradient(target, r, kBrickRadius, brighten(base, 0.35f), base);
            // Shade along the lower lip and a white gloss strip on top
            UiKit::drawRoundedRect(target, sf::FloatRect({x + 6.0f, y + h - 7.0f}, {w - 12.0f, 3.0f}), 1.5f,
                                   withAlpha(dark, byteOf(0.5f)));
            UiKit::drawRoundedRect(target, sf::FloatRect({x + 8.0f, y + 4.0f}, {w - 16.0f, 6.0f}), 3.0f,
                                   withAlpha(sf::Color::White, 170));

            if (b.bomb)
            {
                // Burst icon with a pulsing red halo
                const sf::Vector2f c = centerOf(r);
                const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f + x * 0.05f);
                UiKit::drawGlow(target, c, 46.0f, withAlpha(sf::Color(255, 120, 60), byteOf(0.25f + 0.2f * pulse)));
                UiKit::drawStarburst(target, c, 11.0f, 5.0f, 6, t * 40.0f, sf::Color(255, 250, 200),
                                     sf::Color(255, 190, 40), 3u);
            }

            // Damaged (one hit taken): darker shade and cracks
            if (b.hp < b.maxHp)
            {
                UiKit::drawRoundedRect(target, r, kBrickRadius, withAlpha(dark, byteOf(0.35f)));
                drawCracks(target, r);
            }
        }
    }

    void SmashGame::drawPaddle(sf::RenderTarget &target) const
    {
        const float w = m_paddleW;
        const float grow = m_paddleSquash * 3.0f; // a hit makes it briefly thicker
        const float h = kPaddleH + grow;
        const float x = m_paddleX - w * 0.5f;
        const float y = kPaddleY - grow * 0.5f;
        const Accent &a = accent();

        // Glow under the bar, dark slab, then the glossy body
        UiKit::drawGlow(target, sf::Vector2f{m_paddleX, y + h + 8.0f}, w * 0.75f, withAlpha(a.base, 150));
        UiKit::drawRoundedRect(target, sf::FloatRect({x + 3.0f, y + 9.0f}, {w - 6.0f, h}), h * 0.5f, shade(a.base, 0.45f));
        UiKit::drawRoundedRectGradient(target, sf::FloatRect({x, y}, {w, h}), h * 0.5f, a.light, a.base);

        // Cylinder-like highlights: a bright band near the top and a soft one across the middle
        UiKit::drawRoundedRect(target, sf::FloatRect({x + h * 0.5f, y + 4.0f}, {w - h, h * 0.18f}), h * 0.09f,
                               withAlpha(sf::Color::White, 200));
        UiKit::drawRoundedRect(target, sf::FloatRect({x + h * 0.5f, y + h * 0.52f}, {w - h, h * 0.14f}),
                               h * 0.07f, withAlpha(sf::Color::White, 70));
        UiKit::drawSparkle(target, sf::Vector2f{x + h * 0.5f, y + h * 0.5f}, h * 0.36f, withAlpha(sf::Color::White, 230));
        UiKit::drawSparkle(target, sf::Vector2f{x + w - h * 0.5f, y + h * 0.5f}, h * 0.26f,
                           withAlpha(sf::Color::White, 180));
    }

    void SmashGame::drawBalls(sf::RenderTarget &target) const
    {
        const Accent &a = accent();
        for (const Ball &ball : m_balls)
        {
            if (!ball.attached)
            {
                // Fading trail: the oldest sample (last index) is drawn first, smaller and fainter
                for (std::size_t k = ball.trail.size(); k-- > 0;)
                {
                    const float age = static_cast<float>(k) / static_cast<float>(ball.trail.size());
                    const float radius = kBallRadius * (1.9f - 0.9f * age);
                    UiKit::drawGlow(target, ball.trail[k], radius, withAlpha(a.base, byteOf(0.5f * (1.0f - age))));
                }
            }

            // Stretch the jelly along the flight direction
            float angle = 0.0f;
            sf::Vector2f radii{kBallRadius, kBallRadius};
            if (!ball.attached)
            {
                angle = std::atan2(ball.vel.y, ball.vel.x) * 180.0f / kPi;
                radii = sf::Vector2f{kBallRadius * 1.1f, kBallRadius * 0.92f};
            }
            UiKit::drawGlow(target, ball.pos, 44.0f, withAlpha(a.base, 150));
            UiKit::drawShadedEllipse(target, ball.pos, radii, a.light, a.base, a.dark, angle);
            UiKit::drawSparkle(target, sf::Vector2f{ball.pos.x - 5.0f, ball.pos.y - 5.0f}, 4.5f,
                               withAlpha(sf::Color::White, 240));
        }
    }

    void SmashGame::drawCapsules(sf::RenderTarget &target) const
    {
        for (const Capsule &c : m_capsules)
        {
            const sf::Color col = capsuleColor(c.letter);
            // The capsule spins about its vertical axis: its width follows cos(rot)
            const float sx = std::max(0.22f, std::abs(std::cos(c.rot)));
            const float w = 54.0f * sx;
            const float h = 30.0f;
            const float left = c.pos.x - w * 0.5f;
            const float top = c.pos.y - h * 0.5f;

            UiKit::drawGlow(target, c.pos, 50.0f, withAlpha(col, 120));
            UiKit::drawRoundedRect(target, sf::FloatRect({left, top + 4.0f}, {w, h}), 15.0f, shade(col, 0.6f));
            UiKit::drawRoundedRectGradient(target, sf::FloatRect({left, top}, {w, h}), 15.0f, brighten(col, 0.45f), col);
            if (w > 24.0f)
            {
                UiKit::drawRoundedRect(target, sf::FloatRect({left + 8.0f, top + 4.0f}, {w - 16.0f, 5.0f}), 2.5f,
                                       withAlpha(sf::Color::White, 200));
            }
            if (sx > 0.45f)
            {
                UiKit::drawLabel(target, font(), std::string(1, c.letter), 20, c.pos, sf::Color::White, true, 1.0f);
            }
        }
    }

    // ---------------------------------------------------------------- game logic

    void SmashGame::animate(float dt)
    {
        m_gridOffset = std::fmod(m_gridOffset + dt * 0.4f, 1.0f);
        m_flash *= std::exp(-4.0f * dt);
        m_paddleSquash *= std::exp(-9.0f * dt);
        m_levelBanner = std::max(0.0f, m_levelBanner - dt);
        for (Brick &b : m_bricks)
        {
            b.flash *= std::exp(-7.0f * dt);
        }
        for (Orb &orb : m_orbs)
        {
            orb.pos += orb.vel * dt;
            const float m = orb.radius;
            if (orb.pos.x < -m)
            {
                orb.pos.x += kWidth + 2.0f * m;
            }
            else if (orb.pos.x > kWidth + m)
            {
                orb.pos.x -= kWidth + 2.0f * m;
            }
            if (orb.pos.y < -m)
            {
                orb.pos.y += kHeight + 2.0f * m;
            }
            else if (orb.pos.y > kHeight + m)
            {
                orb.pos.y -= kHeight + 2.0f * m;
            }
        }
    }

    void SmashGame::placeAttachedBalls()
    {
        for (Ball &ball : m_balls)
        {
            if (ball.attached)
            {
                ball.pos = sf::Vector2f{m_paddleX, kPaddleY - kBallRadius - 1.0f};
                ball.vel = sf::Vector2f{0.0f, 0.0f};
            }
        }
    }

    void SmashGame::attachBall()
    {
        Ball ball;
        ball.pos = sf::Vector2f{m_paddleX, kPaddleY - kBallRadius - 1.0f};
        ball.trail.fill(ball.pos);
        ball.attached = true;
        m_balls.push_back(ball);
    }

    void SmashGame::launchBalls()
    {
        bool launched = false;
        for (Ball &ball : m_balls)
        {
            if (!ball.attached)
            {
                continue;
            }
            ball.attached = false;
            // Launch tilts a little, alternating left and right
            const float tilt = 0.22f * static_cast<float>(m_launchSign);
            m_launchSign = -m_launchSign;
            const float speed = currentSpeed();
            ball.vel = sf::Vector2f{speed * std::sin(tilt), -speed * std::cos(tilt)};
            launched = true;
        }
        if (launched)
        {
            audio().playEat(1.4f);
        }
    }

    void SmashGame::buildLevel()
    {
        m_bricks.clear();
        m_bricks.reserve(kRows * kCols);
        const int layout = (m_level - 1) % kLayouts;
        for (int row = 0; row < kRows; ++row)
        {
            for (int col = 0; col < kCols; ++col)
            {
                if (!brickPresent(layout, row, col))
                {
                    continue;
                }
                Brick brick;
                brick.rect = brickRect(row, col);
                brick.color = kRowColors[row];
                brick.alive = true;
                // From level 2 the top two rows need two hits
                brick.maxHp = (m_level >= 2 && row < 2) ? 2 : 1;
                brick.hp = brick.maxHp;
                if (rnd() < kBombChance)
                {
                    brick.bomb = true;
                    brick.maxHp = 1;
                    brick.hp = 1;
                }
                m_bricks.push_back(brick);
            }
        }
    }

    void SmashGame::advanceBall(Ball &ball, float h)
    {
        ball.pos += ball.vel * h;

        // Side walls and ceiling
        if (ball.pos.x - kBallRadius < kLeftWall)
        {
            ball.pos.x = kLeftWall + kBallRadius;
            ball.vel.x = std::abs(ball.vel.x);
            bounceFx(ball.pos);
        }
        else if (ball.pos.x + kBallRadius > kRightWall)
        {
            ball.pos.x = kRightWall - kBallRadius;
            ball.vel.x = -std::abs(ball.vel.x);
            bounceFx(ball.pos);
        }
        if (ball.pos.y - kBallRadius < kTopWall)
        {
            ball.pos.y = kTopWall + kBallRadius;
            ball.vel.y = std::abs(ball.vel.y);
            bounceFx(ball.pos);
        }

        collidePaddle(ball);
        collideBricks(ball);
        limitSpeed(ball);
    }

    void SmashGame::bounceFx(sf::Vector2f at)
    {
        fx().spawnBurst(at, accent().light, 4, 0.4f);
        audio().playEat(1.8f);
    }

    void SmashGame::collidePaddle(Ball &ball)
    {
        if (ball.vel.y <= 0.0f)
        {
            return; // only a falling ball can be caught
        }
        const float half = m_paddleW * 0.5f;
        const float top = kPaddleY;
        const float bottom = kPaddleY + kPaddleH;
        if (ball.pos.y + kBallRadius < top || ball.pos.y - kBallRadius > bottom)
        {
            return;
        }
        if (ball.pos.x < m_paddleX - half - kBallRadius * 0.5f || ball.pos.x > m_paddleX + half + kBallRadius * 0.5f)
        {
            return;
        }

        // The bounce angle follows the hit point: centre goes straight up, the edges go 60 degrees out
        const float offset = std::clamp((ball.pos.x - m_paddleX) / half, -1.0f, 1.0f);
        const float angle = offset * kMaxBounceAngle;
        const float speed = currentSpeed();
        ball.pos.y = top - kBallRadius;
        ball.vel = sf::Vector2f{speed * std::sin(angle), -speed * std::cos(angle)};

        m_paddleSquash = 1.0f;
        m_combo = 0;
        setCombo(0);
        audio().playHit();
        fx().spawnBurst(sf::Vector2f{ball.pos.x, top}, accent().light, 8, 0.7f);
    }

    void SmashGame::collideBricks(Ball &ball)
    {
        for (std::size_t i = 0; i < m_bricks.size(); ++i)
        {
            Brick &brick = m_bricks[i];
            if (!brick.alive)
            {
                continue;
            }
            const float left = brick.rect.position.x;
            const float top = brick.rect.position.y;
            const float right = left + brick.rect.size.x;
            const float bottom = top + brick.rect.size.y;
            if (ball.pos.x + kBallRadius < left || ball.pos.x - kBallRadius > right ||
                ball.pos.y + kBallRadius < top || ball.pos.y - kBallRadius > bottom)
            {
                continue;
            }

            // Closest point of the brick to the ball centre
            const sf::Vector2f closest{std::clamp(ball.pos.x, left, right), std::clamp(ball.pos.y, top, bottom)};
            const sf::Vector2f d = ball.pos - closest;
            const float dist2 = d.x * d.x + d.y * d.y;
            sf::Vector2f n{0.0f, 0.0f};
            if (dist2 > 1e-4f)
            {
                const float dist = std::sqrt(dist2);
                if (dist >= kBallRadius)
                {
                    continue;
                }
                n = d / dist;
                ball.pos = closest + n * kBallRadius;
            }
            else
            {
                // Centre inside the brick: push out through the nearest side
                const float dl = ball.pos.x - left;
                const float dr = right - ball.pos.x;
                const float dt = ball.pos.y - top;
                const float db = bottom - ball.pos.y;
                const float m = std::min(std::min(dl, dr), std::min(dt, db));
                if (m == dl)
                {
                    n = sf::Vector2f{-1.0f, 0.0f};
                }
                else if (m == dr)
                {
                    n = sf::Vector2f{1.0f, 0.0f};
                }
                else if (m == dt)
                {
                    n = sf::Vector2f{0.0f, -1.0f};
                }
                else
                {
                    n = sf::Vector2f{0.0f, 1.0f};
                }
                ball.pos += n * (m + kBallRadius);
            }

            // Reflect the velocity when it moves into the brick
            const float vn = ball.vel.x * n.x + ball.vel.y * n.y;
            if (vn < 0.0f)
            {
                ball.vel -= n * (2.0f * vn);
            }

            if (brick.hp > 1)
            {
                --brick.hp;
                brick.flash = 1.0f;
                fx().spawnBurst(closest, brighten(brick.color, 0.5f), 6, 0.6f);
                audio().playPop(0.7f);
            }
            else
            {
                breakBrick(i);
            }
            return; // one brick per substep
        }
    }

    void SmashGame::limitSpeed(Ball &ball) const
    {
        // Keep the speed constant, never let the ball go too horizontal
        const float speed = currentSpeed();
        const float len = std::sqrt(ball.vel.x * ball.vel.x + ball.vel.y * ball.vel.y);
        if (len < 1e-3f)
        {
            ball.vel = sf::Vector2f{0.0f, -speed};
            return;
        }
        ball.vel = ball.vel * (speed / len);

        const float minVy = kMinUpward * speed;
        if (std::abs(ball.vel.y) < minVy)
        {
            ball.vel.y = ball.vel.y < 0.0f ? -minVy : minVy;
        }
        const float sx = ball.vel.x < 0.0f ? -1.0f : 1.0f;
        ball.vel.x = sx * std::sqrt(std::max(0.0f, speed * speed - ball.vel.y * ball.vel.y));
    }

    void SmashGame::breakBrick(std::size_t index)
    {
        Brick &brick = m_bricks[index];
        brick.alive = false;
        const sf::Vector2f c = centerOf(brick.rect);
        const bool bomb = brick.bomb;
        const sf::Color color = bomb ? kBombColor : brick.color;

        fx().spawnBurst(c, color, 16);
        audio().playPop(1.0f + 0.04f * static_cast<float>(std::min(m_combo, 12)));
        addScore(10 * (1 + m_combo / 3));
        ++m_combo;
        setCombo(m_combo);
        setComboWord("COMBO");
        if (m_combo % 5 == 0)
        {
            fx().spawnText(c, "COMBO x" + std::to_string(m_combo));
            fx().addShake(6.0f);
        }
        m_flash = std::min(1.0f, m_flash + 0.35f);
        if (rnd() < kDropChance)
        {
            spawnCapsule(c);
        }

        if (bomb)
        {
            // Chain reaction: every brick within reach goes too
            fx().spawnBlast(c, 1.0f, kBombColor);
            audio().playBlast();
            fx().addShake(5.0f);
            for (std::size_t j = 0; j < m_bricks.size(); ++j)
            {
                if (m_bricks[j].alive && std::hypot(centerOf(m_bricks[j].rect).x - c.x,
                                                    centerOf(m_bricks[j].rect).y - c.y) < kBombRadius)
                {
                    breakBrick(j);
                }
            }
        }
    }

    void SmashGame::spawnCapsule(sf::Vector2f pos)
    {
        const float r = rnd();
        Capsule capsule;
        capsule.pos = pos;
        capsule.letter = r < 0.3f ? 'W' : (r < 0.55f ? 'M' : (r < 0.8f ? 'S' : 'L'));
        capsule.rot = rnd() * 2.0f * kPi;
        m_capsules.push_back(capsule);
    }

    void SmashGame::updateCapsules(float dt)
    {
        const float paddleL = m_paddleX - m_paddleW * 0.5f;
        const float paddleR = m_paddleX + m_paddleW * 0.5f;
        std::size_t i = 0;
        while (i < m_capsules.size())
        {
            Capsule &c = m_capsules[i];
            c.pos.y += kCapsuleFall * dt;
            c.rot += 2.4f * dt;

            // Capsule box (54 x 30) against the paddle box
            const bool caught = c.pos.x + 27.0f >= paddleL && c.pos.x - 27.0f <= paddleR &&
                                c.pos.y + 15.0f >= kPaddleY && c.pos.y - 15.0f <= kPaddleY + kPaddleH;
            if (caught)
            {
                const Capsule taken = c;
                m_capsules.erase(m_capsules.begin() + static_cast<std::ptrdiff_t>(i));
                applyPower(taken.letter, taken.pos);
                continue;
            }
            if (c.pos.y > kBottomLine)
            {
                m_capsules.erase(m_capsules.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
        }
    }

    void SmashGame::applyPower(char letter, sf::Vector2f pos)
    {
        const sf::Color color = capsuleColor(letter);
        std::string text;
        switch (letter)
        {
        case 'W':
            m_wideTimer = kWideTime;
            text = "WIDE!";
            break;
        case 'M':
            multiball();
            text = "MULTIBALL!";
            break;
        case 'S':
            m_slowTimer = kSlowTime;
            text = "SLOW!";
            break;
        default:
            if (m_lives < kMaxLives)
            {
                ++m_lives;
            }
            syncLives();
            text = "+1 LIFE!";
            break;
        }

        audio().playPower();
        fx().spawnBurst(pos, color, 18, 1.0f);
        fx().spawnText(sf::Vector2f{m_paddleX, kPaddleY - 40.0f}, text);
    }

    void SmashGame::multiball()
    {
        if (m_balls.empty())
        {
            return;
        }
        // Two extra balls split off the first ball, flying at +-0.4 rad from its direction
        const Ball source = m_balls.front();
        const float speed = currentSpeed();
        const sf::Vector2f dir = source.attached ? sf::Vector2f{0.0f, -speed} : source.vel;
        const float angles[2] = {-0.4f, 0.4f};
        for (float angle : angles)
        {
            if (m_balls.size() >= kMaxBalls)
            {
                break;
            }
            Ball extra;
            extra.pos = source.pos;
            extra.vel = rotated(dir, angle);
            extra.attached = false;
            extra.trail = source.trail;
            m_balls.push_back(extra);
        }
    }

    void SmashGame::loseLife()
    {
        const sf::Vector2f paddle{m_paddleX, kPaddleY};
        --m_lives;
        m_combo = 0;
        setCombo(0);
        fx().spawnBlast(paddle, 1.0f);
        fx().addShake(8.0f);
        audio().playHit();

        if (m_lives <= 0)
        {
            m_lives = 0;
            syncLives();
            gameOver(paddle, 1.0f);
            return;
        }
        syncLives();
        attachBall();
    }

    void SmashGame::nextLevel()
    {
        ++m_level;
        buildLevel();
        m_capsules.clear();
        m_balls.clear();
        attachBall();

        m_levelBanner = kBannerTime;
        m_flash = 1.0f;
        addScore(500);
        fx().spawnConfetti();
        fx().spawnText(sf::Vector2f{640.0f, 380.0f}, "LEVEL " + std::to_string(m_level));
        fx().addShake(6.0f);
        audio().playNewBest();
    }

    void SmashGame::syncLives()
    {
        setLives(m_lives, std::max(3, m_lives));
    }

    float SmashGame::currentSpeed() const
    {
        const float base = kBaseSpeed + 60.0f * static_cast<float>(difficulty()) +
                           25.0f * static_cast<float>(m_level - 1);
        return base * m_speedMul;
    }

    float SmashGame::clampPaddle(float x) const
    {
        const float half = m_paddleW * 0.5f;
        return std::clamp(x, kLeftWall + half, kRightWall - half);
    }

    int SmashGame::aliveBricks() const
    {
        return static_cast<int>(std::count_if(m_bricks.begin(), m_bricks.end(), [](const Brick &b)
                                              { return b.alive; }));
    }

    float SmashGame::rnd()
    {
        m_seed = m_seed * 1664525u + 1013904223u;
        return static_cast<float>(m_seed >> 8) * (1.0f / 16777216.0f);
    }
} // namespace Flappy
