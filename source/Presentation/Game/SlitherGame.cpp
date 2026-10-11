#include "SlitherGame.h"
#include "UiKit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numbers>
#include <string>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr int kCols = 32;
        constexpr int kRows = 16;
        constexpr float kCell = 36.0f;
        const sf::Vector2f kOrigin{64.0f, 132.0f};
        constexpr std::size_t kMaxBlocks = 12;
        constexpr int kBlockFromLength = 12;
        constexpr float kComboWindow = 3.0f;
        constexpr float kStarLife = 6.0f;
        constexpr float kPopInterval = 0.04f;
        constexpr float kMinStepInterval = 0.07f;

        const std::array<sf::Color, 5> kFruitColors{{sf::Color(255, 84, 96), sf::Color(255, 150, 52),
                                                     sf::Color(255, 206, 64), sf::Color(255, 110, 160),
                                                     sf::Color(255, 120, 80)}};
        const sf::Color kInk(27, 15, 74);
        const sf::Color kGold(255, 206, 64);
        const sf::Color kStarCore(255, 250, 210);
        const sf::Color kCyan(90, 245, 255);
        const sf::Color kViolet(170, 110, 255);
        const sf::Color kLeafLight(170, 255, 130);
        const sf::Color kLeafMid(70, 200, 80);
        const sf::Color kLeafDark(20, 110, 50);
        const sf::Color kStem(110, 70, 40);
        const sf::Color kTongue(255, 110, 150);

        sf::Vector2f cellCenter(sf::Vector2i c)
        {
            return {kOrigin.x + (static_cast<float>(c.x) + 0.5f) * kCell,
                    kOrigin.y + (static_cast<float>(c.y) + 0.5f) * kCell};
        }

        std::uint8_t toAlpha(float v)
        {
            return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 255.0f));
        }

        // Quad as two triangles (for a TriangleList)
        void appendRect(sf::VertexArray &va, sf::FloatRect r, sf::Color color)
        {
            const sf::Vector2f p = r.position;
            const sf::Vector2f s = r.size;
            va.append(sf::Vertex{p, color});
            va.append(sf::Vertex{{p.x + s.x, p.y}, color});
            va.append(sf::Vertex{{p.x + s.x, p.y + s.y}, color});
            va.append(sf::Vertex{p, color});
            va.append(sf::Vertex{{p.x + s.x, p.y + s.y}, color});
            va.append(sf::Vertex{{p.x, p.y + s.y}, color});
        }

        // Quad strip between two points with half widths and colours at each end (for a TriangleList)
        void appendTube(sf::VertexArray &va, sf::Vector2f a, sf::Vector2f b, float wa, float wb, sf::Color ca,
                        sf::Color cb)
        {
            const sf::Vector2f d = b - a;
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            if (len < 0.001f)
            {
                return;
            }
            const sf::Vector2f n{-d.y / len, d.x / len};
            const sf::Vector2f a0 = a + n * wa;
            const sf::Vector2f a1 = a - n * wa;
            const sf::Vector2f b0 = b + n * wb;
            const sf::Vector2f b1 = b - n * wb;
            va.append(sf::Vertex{a0, ca});
            va.append(sf::Vertex{a1, ca});
            va.append(sf::Vertex{b1, cb});
            va.append(sf::Vertex{a0, ca});
            va.append(sf::Vertex{b1, cb});
            va.append(sf::Vertex{b0, cb});
        }

        // Solid circle (for eyes and pupils)
        void drawDisc(sf::RenderTarget &target, sf::Vector2f c, float r, sf::Color color)
        {
            sf::CircleShape disc(r, 20);
            disc.setOrigin({r, r});
            disc.setPosition(c);
            disc.setFillColor(color);
            target.draw(disc);
        }

        // Thick arc used for the bonus star countdown (from / to in radians)
        void drawArc(sf::RenderTarget &target, sf::Vector2f c, float radius, float thickness, float fromRad,
                     float toRad, sf::Color color)
        {
            if (toRad - fromRad <= 0.001f)
            {
                return;
            }
            constexpr int kSegments = 48;
            const float h = thickness * 0.5f;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= kSegments; ++i)
            {
                const float a = fromRad + (toRad - fromRad) * static_cast<float>(i) / static_cast<float>(kSegments);
                const sf::Vector2f u{std::cos(a), std::sin(a)};
                va.append(sf::Vertex{c + u * (radius + h), color});
                va.append(sf::Vertex{c + u * (radius - h), color});
            }
            target.draw(va);
        }

        // One glossy bead: shaded ellipse with the highlight toward the top-left
        void drawBead(sf::RenderTarget &target, sf::Vector2f c, float r, sf::Color mid, sf::Color dark)
        {
            UiKit::drawShadedEllipse(target, c, {r, r}, mix(mid, sf::Color::White, 0.55f), mid,
                                     mix(mid, dark, 0.5f));
        }

        // Fraction of the way along the snake: 0 at the head, 1 at the tail
        float bodyFraction(std::size_t i, std::size_t n)
        {
            return n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
        }
    } // namespace

    SlitherGame::SlitherGame(const sf::Font &font) : ArcadeGame(font, 3, 3)
    {
        onStart();
    }

    void SlitherGame::onStart()
    {
        m_body.assign({sf::Vector2i{kCols / 2, kRows / 2}, sf::Vector2i{kCols / 2 - 1, kRows / 2},
                       sf::Vector2i{kCols / 2 - 2, kRows / 2}, sf::Vector2i{kCols / 2 - 3, kRows / 2}});
        m_prev = m_body;
        m_turns.clear();
        m_dir = sf::Vector2i{1, 0};
        m_stepTimer = 0.0f;
        m_stepInterval = stepInterval();
        m_running = false;

        m_dying = false;
        m_popNext = 0;
        m_popTimer = 0.0f;

        m_blocks.clear();
        m_starActive = false;
        m_starTimer = 0.0f;
        m_foodEaten = 0;
        m_pulse = 0.0f;

        m_combo = 0;
        m_lastEatTime = -100.0f;
        setCombo(0);
        setComboWord("COMBO");
        setLives(0, 0);

        spawnFood();
    }

    void SlitherGame::onBegin()
    {
        m_running = true;
        m_stepTimer = 0.0f;
    }

    bool SlitherGame::isStartKey(sf::Keyboard::Key key) const
    {
        switch (key)
        {
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::Right:
        case sf::Keyboard::Key::W:
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::S:
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Space:
        case sf::Keyboard::Key::Enter:
            return true;
        default:
            return false;
        }
    }

    void SlitherGame::onKeyPressed(sf::Keyboard::Key key)
    {
        sf::Vector2i dir;
        switch (key)
        {
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::W:
            dir = {0, -1};
            break;
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            dir = {0, 1};
            break;
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::A:
            dir = {-1, 0};
            break;
        case sf::Keyboard::Key::Right:
        case sf::Keyboard::Key::D:
            dir = {1, 0};
            break;
        default:
            return;
        }

        // Compare with the last queued turn (or the current heading): no same-direction or 180-degree turns
        const sf::Vector2i ref = m_turns.empty() ? m_dir : m_turns.back();
        if (dir == ref || dir == sf::Vector2i{-ref.x, -ref.y})
        {
            return;
        }
        if (m_turns.size() < 2)
        {
            m_turns.push_back(dir);
        }
    }

    void SlitherGame::onUpdate(float dt)
    {
        m_pulse = std::max(0.0f, m_pulse - 2.5f * dt);

        // The combo tag goes out after the window without food
        if (m_combo > 0 && runTime() - m_lastEatTime > kComboWindow)
        {
            m_combo = 0;
            setCombo(0);
        }

        if (m_starActive)
        {
            m_starTimer -= dt;
            if (m_starTimer <= 0.0f)
            {
                m_starActive = false;
            }
        }

        m_stepTimer += dt;
        while (!m_dying && m_stepTimer >= m_stepInterval)
        {
            m_stepTimer -= m_stepInterval;
            step();
        }
    }

    void SlitherGame::onIdle(float dt)
    {
        m_pulse = std::max(0.0f, m_pulse - 2.5f * dt);
        if (!m_dying)
        {
            return;
        }

        // Chain reaction: one segment pops from the head to the tail every 40 ms
        m_popTimer += dt;
        const std::size_t total = m_body.size();
        while (m_popTimer >= kPopInterval && m_popNext < total)
        {
            m_popTimer -= kPopInterval;
            const sf::Vector2f where = cellCenter(m_body[m_popNext]);
            const float progress = static_cast<float>(m_popNext + 1) / static_cast<float>(total);
            fx().spawnBurst(where, accent().base, 10, 1.0f);
            audio().playPop(0.8f + progress);
            if (m_popNext % 4 == 0)
            {
                fx().spawnBlast(where, 0.4f, accent().light);
            }
            ++m_popNext;
        }
    }

    void SlitherGame::step()
    {
        if (!m_turns.empty())
        {
            m_dir = m_turns.front();
            m_turns.pop_front();
        }

        const sf::Vector2i head = m_body.front();
        sf::Vector2i next{head.x + m_dir.x, head.y + m_dir.y};
        if (difficulty() == 0)
        {
            // Easy: the field wraps around
            next.x = (next.x + kCols) % kCols;
            next.y = (next.y + kRows) % kRows;
        }
        else if (next.x < 0 || next.x >= kCols || next.y < 0 || next.y >= kRows)
        {
            die();
            return;
        }

        const bool eat = next == m_food;
        const bool bonus = m_starActive && next == m_star;

        // The tail leaves its cell this step, so it only blocks when the snake grows
        const std::size_t blockers = eat ? m_body.size() : m_body.size() - 1;
        for (std::size_t i = 0; i < blockers; ++i)
        {
            if (m_body[i] == next)
            {
                die();
                return;
            }
        }
        for (const Block &b : m_blocks)
        {
            if (b.cell == next)
            {
                die();
                return;
            }
        }

        m_prev = m_body;
        m_body.push_front(next);
        if (!eat)
        {
            m_body.pop_back();
        }
        m_stepInterval = stepInterval();

        if (eat)
        {
            eatFruit();
        }
        if (bonus)
        {
            eatStar();
        }
    }

    void SlitherGame::eatFruit()
    {
        const float now = runTime();
        m_combo = (now - m_lastEatTime <= kComboWindow) ? m_combo + 1 : 0;
        m_lastEatTime = now;
        setCombo(m_combo);

        const int points = 10 * std::max(1, m_combo);
        const sf::Vector2f where = cellCenter(m_body.front());
        addScore(points);
        fx().spawnBurst(where, m_fruitColor, 18, 1.2f);
        audio().playEat(1.0f + 0.03f * static_cast<float>(std::min(m_combo, 15)));
        fx().spawnText(where, "+" + std::to_string(points));
        fx().addShake(2.0f);
        m_pulse = 1.0f;

        ++m_foodEaten;
        // Crystal block every 4th food once the snake is long enough
        if (m_foodEaten % 4 == 0 && m_body.size() >= static_cast<std::size_t>(kBlockFromLength) &&
            m_blocks.size() < kMaxBlocks)
        {
            const sf::Vector2i cell = freeCell(true);
            m_blocks.push_back(Block{cell, m_blocks.size() % 2 == 0 ? kCyan : kViolet});
        }

        spawnFood();
        if (m_foodEaten % 5 == 0)
        {
            spawnStar();
        }
    }

    void SlitherGame::eatStar()
    {
        const sf::Vector2f where = cellCenter(m_star);
        m_starActive = false;
        addScore(100);
        audio().playPower();
        fx().spawnBlast(where, 1.2f, kGold);
        fx().spawnText(where, "+100");
        fx().addShake(3.0f);
        m_pulse = 1.0f;
    }

    void SlitherGame::die()
    {
        m_dying = true;
        m_popNext = 0;
        m_popTimer = kPopInterval; // the head pops on the first idle frame
        m_turns.clear();
        gameOver(cellCenter(m_body.front()), 1.0f);
    }

    void SlitherGame::spawnFood()
    {
        m_food = freeCell(false);
        m_fruitColor = kFruitColors[static_cast<std::size_t>(rndInt(static_cast<int>(kFruitColors.size())))];
    }

    void SlitherGame::spawnStar()
    {
        m_star = freeCell(true);
        m_starActive = true;
        m_starTimer = kStarLife;
    }

    float SlitherGame::stepInterval() const
    {
        const float base = difficulty() == 0 ? 0.15f : (difficulty() == 2 ? 0.095f : 0.12f);
        return std::max(kMinStepInterval, base - 0.002f * static_cast<float>(m_body.size()));
    }

    bool SlitherGame::isFree(sf::Vector2i cell) const
    {
        if (cell == m_food || (m_starActive && cell == m_star))
        {
            return false;
        }
        for (const sf::Vector2i &s : m_body)
        {
            if (s == cell)
            {
                return false;
            }
        }
        for (const Block &b : m_blocks)
        {
            if (b.cell == cell)
            {
                return false;
            }
        }
        return true;
    }

    sf::Vector2i SlitherGame::freeCell(bool farFromHead)
    {
        const sf::Vector2i head = m_body.front();
        auto ok = [this, farFromHead, head](sf::Vector2i c)
        {
            if (!isFree(c))
            {
                return false;
            }
            return !farFromHead || std::abs(c.x - head.x) + std::abs(c.y - head.y) >= 6;
        };

        for (int attempt = 0; attempt < 300; ++attempt)
        {
            const sf::Vector2i c{rndInt(kCols), rndInt(kRows)};
            if (ok(c))
            {
                return c;
            }
        }
        // Fallback: first free cell in scan order
        for (int y = 0; y < kRows; ++y)
        {
            for (int x = 0; x < kCols; ++x)
            {
                if (ok(sf::Vector2i{x, y}))
                {
                    return sf::Vector2i{x, y};
                }
            }
        }
        return head;
    }

    float SlitherGame::rnd()
    {
        m_seed = m_seed * 1664525u + 1013904223u;
        return static_cast<float>(m_seed >> 8) / 16777216.0f;
    }

    int SlitherGame::rndInt(int n)
    {
        return std::min(n - 1, static_cast<int>(rnd() * static_cast<float>(n)));
    }

    // ------------------------------------------------------------------ drawing

    void SlitherGame::drawBackground(sf::RenderTarget &target) const
    {
        // Vertical teal -> indigo gradient
        sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
        const sf::Color rows[3] = {sf::Color(6, 50, 62), sf::Color(12, 36, 78), sf::Color(24, 14, 70)};
        for (int i = 0; i < 3; ++i)
        {
            const float y = kHeight * 0.5f * static_cast<float>(i);
            sky.append(sf::Vertex{{0.0f, y}, rows[i]});
            sky.append(sf::Vertex{{kWidth, y}, rows[i]});
        }
        target.draw(sky);

        // Drifting glow motes around the field
        const float t = clockTime();
        for (int i = 0; i < 30; ++i)
        {
            const float fi = static_cast<float>(i);
            const float bx = std::fmod(fi * 263.0f + 90.0f, kWidth);
            const float by = std::fmod(fi * 157.0f + 60.0f, kHeight);
            const sf::Vector2f p{bx + 40.0f * std::sin(t * 0.25f + fi * 1.7f),
                                 by + 30.0f * std::cos(t * 0.19f + fi * 2.3f)};
            const float tw = 0.5f + 0.5f * std::sin(t * 2.0f + fi);
            const sf::Color c = (i % 3 == 0) ? kViolet : kCyan;
            UiKit::drawGlow(target, p, 7.0f + 5.0f * static_cast<float>(i % 4), withAlpha(c, toAlpha(30.0f + 50.0f * tw)));
        }

        // Field panel: dark checkerboard (red-tinted on hard)
        const bool hard = difficulty() == 2;
        const sf::Color shadeA = hard ? sf::Color(40, 16, 30) : sf::Color(14, 38, 58);
        const sf::Color shadeB = hard ? sf::Color(33, 12, 26) : sf::Color(11, 31, 50);
        sf::VertexArray checks(sf::PrimitiveType::Triangles);
        for (int r = 0; r < kRows; ++r)
        {
            for (int c = 0; c < kCols; ++c)
            {
                const sf::Vector2f p{kOrigin.x + static_cast<float>(c) * kCell, kOrigin.y + static_cast<float>(r) * kCell};
                appendRect(checks, {p, {kCell, kCell}}, ((c + r) % 2 == 0) ? shadeA : shadeB);
            }
        }
        target.draw(checks);

        const sf::FloatRect field{kOrigin, {kCols * kCell, kRows * kCell}};
        UiKit::drawRoundedRectGradient(target, field, 0.0f, sf::Color(255, 255, 255, 30), sf::Color(255, 255, 255, 0));

        // Neon grid lines: a faint wide glow and a thin bright core
        sf::VertexArray grid(sf::PrimitiveType::Triangles);
        for (int i = 0; i <= kCols; ++i)
        {
            const float x = kOrigin.x + static_cast<float>(i) * kCell;
            appendRect(grid, {{x - 1.5f, field.position.y}, {3.0f, field.size.y}}, withAlpha(kCyan, 18));
            appendRect(grid, {{x - 0.5f, field.position.y}, {1.0f, field.size.y}}, withAlpha(kCyan, 70));
        }
        for (int j = 0; j <= kRows; ++j)
        {
            const float y = kOrigin.y + static_cast<float>(j) * kCell;
            appendRect(grid, {{field.position.x, y - 1.5f}, {field.size.x, 3.0f}}, withAlpha(kCyan, 18));
            appendRect(grid, {{field.position.x, y - 0.5f}, {field.size.x, 1.0f}}, withAlpha(kCyan, 70));
        }
        target.draw(grid);

        // Pulsing neon frame: walls are deadly outside normal / hard, so the frame reads as a hazard on hard
        const sf::Color frameColor = hard ? sf::Color(255, 80, 100) : kCyan;
        const float glow = std::min(1.0f, 0.55f + 0.2f * std::sin(t * 3.0f) + 0.6f * m_pulse);
        const sf::FloatRect frame{{field.position.x - 8.0f, field.position.y - 8.0f},
                                  {field.size.x + 16.0f, field.size.y + 16.0f}};
        const sf::Color none = sf::Color::Transparent;
        UiKit::drawRoundedRect(target, frame, 14.0f, none, withAlpha(frameColor, toAlpha(40.0f * glow)), 12.0f);
        UiKit::drawRoundedRect(target, frame, 14.0f, none, withAlpha(frameColor, toAlpha(110.0f * glow)), 6.0f);
        UiKit::drawRoundedRect(target, frame, 14.0f, none, withAlpha(frameColor, toAlpha(255.0f * glow)), 2.5f);
    }

    void SlitherGame::drawWorld(sf::RenderTarget &target) const
    {
        drawBlocks(target);
        drawFruit(target);
        if (m_starActive)
        {
            drawStar(target);
        }
        drawSnake(target);
    }

    void SlitherGame::drawBlocks(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        for (std::size_t i = 0; i < m_blocks.size(); ++i)
        {
            const Block &b = m_blocks[i];
            const sf::Vector2f p = cellCenter(b.cell);
            const float glow = 0.75f + 0.25f * std::sin(t * 2.5f + static_cast<float>(i) * 1.3f);
            const sf::Color dark = mix(b.color, sf::Color(10, 6, 40), 0.6f);
            const sf::Color light = mix(b.color, sf::Color::White, 0.5f);

            UiKit::drawGlow(target, p, 42.0f, withAlpha(b.color, toAlpha(70.0f * glow)));
            // Contact shadow, side slab, top face, rim light and a specular streak
            UiKit::drawRoundedRect(target, {{p.x - 14.0f, p.y - 9.0f}, {30.0f, 30.0f}}, 8.0f, withAlpha(sf::Color(0, 0, 20), 110));
            UiKit::drawRoundedRect(target, {{p.x - 15.0f, p.y - 13.0f}, {30.0f, 30.0f}}, 8.0f, dark);
            UiKit::drawRoundedRectGradient(target, {{p.x - 15.0f, p.y - 18.0f}, {30.0f, 28.0f}}, 8.0f, light, b.color);
            UiKit::drawRoundedRect(target, {{p.x - 15.0f, p.y - 18.0f}, {30.0f, 28.0f}}, 8.0f, sf::Color::Transparent,
                            withAlpha(sf::Color::White, toAlpha(170.0f * glow)), 1.6f);
            UiKit::drawRoundedRect(target, {{p.x - 9.0f, p.y - 14.0f}, {12.0f, 3.0f}}, 1.5f, withAlpha(sf::Color::White, 150));
            UiKit::drawSparkle(target, {p.x + 7.0f, p.y - 8.0f}, 4.0f, withAlpha(sf::Color::White, toAlpha(200.0f * glow)));
        }
    }

    void SlitherGame::drawFruit(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        const sf::Vector2f c = cellCenter(m_food);
        const float pulse = 1.0f + 0.07f * std::sin(t * 6.0f);
        const Accent fruit = Accent::from(m_fruitColor);

        UiKit::drawGlow(target, c, 44.0f * pulse, withAlpha(m_fruitColor, 120));
        // Contact shadow, shaded body, leaf, stem, glossy highlight
        UiKit::drawShadedEllipse(target, {c.x + 1.0f, c.y + 12.0f}, {12.0f, 5.0f}, withAlpha(kInk, 90),
                                 withAlpha(kInk, 90), withAlpha(kInk, 0));
        UiKit::drawShadedEllipse(target, c, {14.0f * pulse, 14.0f * pulse}, fruit.light, fruit.base, fruit.dark);
        UiKit::drawShadedEllipse(target, {c.x + 6.0f, c.y - 15.0f}, {8.0f, 4.0f}, kLeafLight, kLeafMid, kLeafDark, -30.0f);
        UiKit::drawRoundedRect(target, {{c.x - 1.5f, c.y - 20.0f}, {3.0f, 7.0f}}, 1.5f, kStem);
        UiKit::drawGlow(target, {c.x - 5.0f, c.y - 6.0f}, 6.0f, sf::Color(255, 255, 255, 230));

        // Sparkles orbiting the fruit
        for (int i = 0; i < 3; ++i)
        {
            const float fi = static_cast<float>(i);
            const float a = t * 2.5f + fi * kTwoPi / 3.0f;
            const float r = 26.0f + 3.0f * std::sin(t * 4.0f + fi);
            const sf::Vector2f p{c.x + std::cos(a) * r, c.y + std::sin(a) * r};
            UiKit::drawSparkle(target, p, 4.5f, withAlpha(sf::Color(255, 250, 220), 200), t * 90.0f);
        }
    }

    void SlitherGame::drawStar(sf::RenderTarget &target) const
    {
        const float t = clockTime();
        const sf::Vector2f c = cellCenter(m_star);
        const float pulse = 1.0f + 0.08f * std::sin(t * 7.0f);

        UiKit::drawGlow(target, c, 58.0f * pulse, withAlpha(kGold, 130));
        UiKit::drawGlow(target, c, 30.0f, withAlpha(kStarCore, 120));
        UiKit::drawStarburst(target, c, 22.0f * pulse, 9.0f, 5, t * 40.0f, kStarCore, kGold, 7u);
        UiKit::drawStarburst(target, c, 12.0f, 5.0f, 5, -t * 30.0f, sf::Color::White, kStarCore, 11u);
        UiKit::drawSparkle(target, {c.x + 14.0f, c.y - 16.0f}, 6.0f, sf::Color::White, t * 60.0f);

        // Countdown ring: the arc shrinks as the star disappears; it blinks in the last two seconds
        const float left = std::clamp(m_starTimer / kStarLife, 0.0f, 1.0f);
        const float blink = m_starTimer < 2.0f ? 0.5f + 0.5f * std::sin(t * 12.0f) : 1.0f;
        UiKit::drawRing(target, c, 34.0f, 3.0f, withAlpha(kGold, 50));
        drawArc(target, c, 34.0f, 4.5f, -kPi * 0.5f, -kPi * 0.5f + kTwoPi * left,
                withAlpha(kStarCore, toAlpha(255.0f * blink)));
    }

    void SlitherGame::drawSnake(sf::RenderTarget &target) const
    {
        const std::size_t n = m_body.size();
        const std::size_t first = m_dying ? m_popNext : 0; // popped segments are hidden
        if (n == 0 || first >= n)
        {
            return;
        }

        const float t = clockTime();
        const float frac = m_dying ? 1.0f : std::clamp(m_stepTimer / m_stepInterval, 0.0f, 1.0f);
        const Accent &acc = accent();

        // Positions interpolated from the previous step, colours and sizes from head to tail
        std::vector<sf::Vector2f> pos(n);
        std::vector<float> rad(n);
        std::vector<sf::Color> col(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            const sf::Vector2i cur = m_body[i];
            const sf::Vector2i prev = i < m_prev.size() ? m_prev[i] : (m_prev.empty() ? cur : m_prev.back());
            const sf::Vector2f b = cellCenter(cur);
            const sf::Vector2f a = cellCenter(prev);
            const bool jump = std::abs(cur.x - prev.x) > 1 || std::abs(cur.y - prev.y) > 1; // wrapped around
            pos[i] = jump ? b : a + (b - a) * frac;

            const float k = bodyFraction(i, n);
            rad[i] = i == 0 ? 21.0f : std::lerp(17.0f, 11.0f, k);
            // Saturated body colour with a bright energy wave running from the head to the tail
            const float wave = 0.5f + 0.5f * std::sin(t * 6.0f - static_cast<float>(i) * 0.55f);
            col[i] = mix(mix(acc.base, acc.dark, k * 0.65f), mix(acc.light, sf::Color::White, 0.3f), wave * 0.45f);
        }

        // Soft glow under every bead
        for (std::size_t i = first; i < n; ++i)
        {
            const float k = bodyFraction(i, n);
            UiKit::drawGlow(target, pos[i], rad[i] * 2.1f, withAlpha(acc.base, toAlpha(150.0f - 60.0f * k)));
        }

        // Jelly connections between the beads (under the beads)
        sf::VertexArray tube(sf::PrimitiveType::Triangles);
        for (std::size_t i = first; i + 1 < n; ++i)
        {
            appendTube(tube, pos[i], pos[i + 1], rad[i] * 0.8f, rad[i + 1] * 0.8f, mix(col[i], acc.dark, 0.3f),
                       mix(col[i + 1], acc.dark, 0.3f));
        }
        target.draw(tube);

        // Beads from the tail to the neck, with a faint sparkle trail
        for (std::size_t i = n; i-- > 1;)
        {
            if (i < first)
            {
                continue;
            }
            const float k = bodyFraction(i, n);
            drawBead(target, pos[i], rad[i], col[i], acc.dark);
            if (i % 4 == 2)
            {
                UiKit::drawSparkle(target, pos[i], 4.0f - 1.5f * k, withAlpha(acc.light, toAlpha(150.0f * (1.0f - k))));
            }
        }

        if (first != 0)
        {
            return;
        }

        // Head: bigger, two eyes and a tongue flick once per second
        const sf::Vector2f d{static_cast<float>(m_dir.x), static_cast<float>(m_dir.y)};
        const sf::Vector2f p{-d.y, d.x};
        const sf::Vector2f h = pos[0];

        if (!m_dying && std::fmod(t, 1.0f) < 0.3f)
        {
            const float angle = std::atan2(d.y, d.x) * 180.0f / kPi;
            const sf::Vector2f base = h + d * 19.0f;
            const sf::Vector2f tip = base + d * 12.0f;
            sf::RectangleShape stem({12.0f, 2.5f});
            stem.setOrigin({0.0f, 1.25f});
            stem.setPosition(base);
            stem.setRotation(sf::degrees(angle));
            stem.setFillColor(kTongue);
            target.draw(stem);
            for (const float s : {-1.0f, 1.0f})
            {
                sf::RectangleShape fork({6.0f, 2.0f});
                fork.setOrigin({0.0f, 1.0f});
                fork.setPosition(tip);
                fork.setRotation(sf::degrees(angle + s * 30.0f));
                fork.setFillColor(kTongue);
                target.draw(fork);
            }
        }

        drawBead(target, h, rad[0], col[0], acc.dark);
        UiKit::drawGlow(target, h + sf::Vector2f{-7.0f, -8.0f} - d * 4.0f, 6.0f, sf::Color(255, 255, 255, 200));

        for (const float s : {-1.0f, 1.0f})
        {
            const sf::Vector2f e = h + d * 7.0f + p * (8.0f * s);
            drawDisc(target, e, 6.5f, sf::Color::White);
            drawDisc(target, e + d * 2.5f, 3.6f, kInk);
            drawDisc(target, e + d * 2.5f + sf::Vector2f{-1.2f, -1.2f}, 1.2f, sf::Color::White);
        }
    }
} // namespace Flappy
