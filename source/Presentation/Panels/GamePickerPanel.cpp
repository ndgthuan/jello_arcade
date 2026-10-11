#include "GamePickerPanel.h"
#include "../../Data/Profile/UserData.h"
#include "../Game/MiniGame.h"
#include "../Game/UiKit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Panels
{
    namespace
    {
        namespace ui = Flappy::UiKit;

        const sf::FloatRect kCard({90.0f, 40.0f}, {1100.0f, 640.0f});
        const sf::FloatRect kBack({500.0f, 600.0f}, {280.0f, 60.0f});

        constexpr float kCardLeft = 148.0f; // 90 + (1100 - 984) / 2
        constexpr float kCardTop = 190.0f;
        constexpr float kCardWidth = 184.0f;
        constexpr float kCardHeight = 330.0f;
        constexpr float kCardStep = 200.0f; // card width + gap (16)
        constexpr float kRadius = 22.0f;
        constexpr float kRise = 8.0f;  // hover lift in pixels
        constexpr float kSink = 6.0f;  // pressed sink in pixels
        constexpr float kSlab = 10.0f; // thickness of the dark slab under a card
        constexpr int kCards = 5;

        // Illustrations are drawn for a 230 px wide card (centre x 115) and scaled down to fit the narrower card
        constexpr float kArtScale = 0.85f;
        constexpr float kArtCenter = 115.0f;

        constexpr unsigned int kNameSize = 21u;
        constexpr float kNameMaxWidth = 172.0f; // card width minus 12
        constexpr unsigned int kTaglineSize = 15u;
        constexpr unsigned int kTopSize = 13u;
        constexpr std::size_t kTopNameLength = 8;

        struct Theme
        {
            sf::Color top;
            sf::Color bottom;
        };

        const std::array<Theme, 5> kThemes{{
            {sf::Color(31, 111, 176), sf::Color(10, 15, 60)},
            {sf::Color(200, 60, 160), sf::Color(40, 10, 80)},
            {sf::Color(90, 60, 200), sf::Color(10, 10, 50)},
            {sf::Color(40, 180, 140), sf::Color(5, 40, 50)},
            {sf::Color(255, 150, 80), sf::Color(120, 30, 90)},
        }};

        const sf::Color kMuted(214, 224, 255);
        const sf::Color kGold(255, 243, 176);
        const sf::Color kInk(30, 20, 50);

        sf::FloatRect rectAt(float x, float y, float w, float h)
        {
            return sf::FloatRect({x, y}, {w, h});
        }

        std::uint8_t toByte(float value)
        {
            return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 255.0f));
        }

        // Design length -> card length
        float sc(float value)
        {
            return value * kArtScale;
        }

        // Design point (x relative to a 230 px card, y from the top of the card) -> point in this card
        sf::Vector2f toCard(sf::Vector2f o, float dx, float dy)
        {
            return {o.x + kCardWidth * 0.5f + sc(dx - kArtCenter), o.y + sc(dy)};
        }

        // Offsets a card point by design units
        sf::Vector2f nudge(sf::Vector2f p, float dx, float dy)
        {
            return {p.x + sc(dx), p.y + sc(dy)};
        }

        // Rectangle with a design top-left corner and design size
        sf::FloatRect boxAt(sf::Vector2f o, float dx, float dy, float w, float h)
        {
            const sf::Vector2f p = toCard(o, dx, dy);
            return rectAt(p.x, p.y, sc(w), sc(h));
        }

        // HSV to colour (hue in degrees, sat and val 0..1)
        sf::Color hsvColor(float hue, float sat, float val)
        {
            const float h = std::fmod(hue, 360.0f) / 60.0f;
            const float c = val * sat;
            const float x = c * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
            const float m = val - c;

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

        // Card rectangle without hover / press offsets (used for hit tests)
        sf::FloatRect baseRect(int id)
        {
            return rectAt(kCardLeft + kCardStep * static_cast<float>(id), kCardTop, kCardWidth, kCardHeight);
        }

        int cardAt(sf::Vector2f point)
        {
            for (int i = 0; i < kCards; ++i)
            {
                if (baseRect(i).contains(point))
                {
                    return i;
                }
            }
            return -1;
        }

        // Splits a tagline longer than 20 characters into two lines at the space nearest to the middle
        std::vector<std::string> wrapTagline(const std::string &text)
        {
            if (text.size() <= 20)
            {
                return {text};
            }

            const std::size_t middle = text.size() / 2;
            std::size_t best = std::string::npos;
            std::size_t bestDistance = 0;
            for (std::size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] != ' ')
                {
                    continue;
                }
                const std::size_t distance = i > middle ? i - middle : middle - i;
                if (best == std::string::npos || distance < bestDistance)
                {
                    best = i;
                    bestDistance = distance;
                }
            }

            if (best == std::string::npos)
            {
                return {text};
            }
            return {text.substr(0, best), text.substr(best + 1)};
        }

        void drawDisc(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color)
        {
            sf::CircleShape disc(radius);
            disc.setOrigin({radius, radius});
            disc.setPosition(center);
            disc.setFillColor(color);
            target.draw(disc);
        }

        void drawTriangle(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color)
        {
            sf::ConvexShape shape(3);
            shape.setPoint(0, a);
            shape.setPoint(1, b);
            shape.setPoint(2, c);
            shape.setFillColor(color);
            target.draw(shape);
        }

        // Glossy blue pipe piece: vertical gradient body, dark rim and a light strip down the middle
        void drawPipe(sf::RenderTarget &target, sf::Vector2f o, float dx, float dy, float w, float h)
        {
            const sf::FloatRect rect = boxAt(o, dx, dy, w, h);
            ui::drawRoundedRectGradient(target, rect, sc(9.0f), sf::Color(120, 200, 255), sf::Color(31, 111, 200));
            ui::drawRoundedRect(target, rect, sc(9.0f), sf::Color::Transparent, sf::Color(10, 40, 110), sc(2.0f));
            const sf::FloatRect strip = rectAt(rect.position.x + rect.size.x * 0.22f, rect.position.y + sc(5.0f),
                                               rect.size.x * 0.14f, rect.size.y - sc(10.0f));
            ui::drawRoundedRect(target, strip, sc(3.0f), sf::Color(190, 235, 255, 210));
        }

        // Glossy rainbow slab of the Stack tower: dark underside, gradient face and a highlight strip
        void drawSlab(sf::RenderTarget &target, sf::Vector2f o, float dx, float dy, float w, sf::Color base)
        {
            ui::drawRoundedRect(target, boxAt(o, dx, dy + 4.0f, w, 15.0f), sc(6.0f),
                                Flappy::mix(base, sf::Color(0, 0, 0), 0.45f));
            ui::drawRoundedRectGradient(target, boxAt(o, dx, dy, w, 15.0f), sc(6.0f),
                                        Flappy::mix(base, sf::Color::White, 0.35f), base);
            ui::drawRoundedRect(target, boxAt(o, dx + 6.0f, dy + 3.0f, w - 12.0f, 4.0f), sc(2.0f),
                                Flappy::withAlpha(sf::Color::White, 160));
        }

        // Illustrations: `o` is the top-left corner of the card face, `t` the running time (used for motion)

        // Two glossy pipes with a lime jello blob flying through the gap
        void drawBounceArt(sf::RenderTarget &target, sf::Vector2f o, float t)
        {
            const float bob = std::sin(t * 2.0f) * 4.0f;

            drawPipe(target, o, 89.0f, 2.0f, 52.0f, 28.0f);
            drawPipe(target, o, 83.0f, 30.0f, 64.0f, 14.0f);
            drawPipe(target, o, 83.0f, 100.0f, 64.0f, 14.0f);
            drawPipe(target, o, 89.0f, 114.0f, 52.0f, 26.0f);

            const sf::Vector2f jello = toCard(o, 115.0f, 72.0f + bob);
            ui::drawShadedEllipse(target, jello, sf::Vector2f{sc(22.0f), sc(18.0f)}, sf::Color(235, 255, 200),
                                  sf::Color(124, 255, 91), sf::Color(46, 170, 40));
            drawDisc(target, nudge(jello, -7.0f, -3.0f), sc(4.5f), sf::Color::White);
            drawDisc(target, nudge(jello, 7.0f, -3.0f), sc(4.5f), sf::Color::White);
            drawDisc(target, nudge(jello, -6.0f, -2.0f), sc(2.2f), kInk);
            drawDisc(target, nudge(jello, 8.0f, -2.0f), sc(2.2f), kInk);
        }

        // Row of four candy bricks, a bouncing ball and a paddle
        void drawSmashArt(sf::RenderTarget &target, sf::Vector2f o, float t)
        {
            const std::array<sf::Color, 4> colors{{
                sf::Color(255, 90, 143),
                sf::Color(255, 160, 60),
                sf::Color(255, 230, 90),
                sf::Color(80, 220, 255),
            }};

            for (std::size_t i = 0; i < colors.size(); ++i)
            {
                const float x = 26.0f + 46.0f * static_cast<float>(i);
                const float y = 22.0f;
                const sf::Color shade = Flappy::mix(colors[i], sf::Color(0, 0, 0), 0.35f);
                ui::drawRoundedRect(target, boxAt(o, x, y + 4.0f, 40.0f, 22.0f), sc(5.0f), shade);
                ui::drawRoundedRect(target, boxAt(o, x, y, 40.0f, 22.0f), sc(5.0f), colors[i]);
                ui::drawRoundedRect(target, boxAt(o, x + 4.0f, y + 3.0f, 32.0f, 5.0f), sc(2.5f),
                                    Flappy::withAlpha(sf::Color::White, 150));
            }

            const sf::Vector2f ball = toCard(o, 115.0f, 84.0f + std::sin(t * 3.0f) * 6.0f);
            ui::drawShadedEllipse(target, ball, sf::Vector2f{sc(10.0f), sc(10.0f)}, sf::Color::White,
                                  sf::Color(200, 210, 255), sf::Color(90, 90, 150));

            ui::drawRoundedRectGradient(target, boxAt(o, 78.0f, 122.0f, 74.0f, 14.0f), sc(7.0f),
                                        sf::Color(150, 240, 255), sf::Color(40, 170, 230));
        }

        // Small ship firing two glowing capsules at a faceted asteroid with an orange starburst on impact
        void drawBlasterArt(sf::RenderTarget &target, sf::Vector2f o, float t)
        {
            const sf::Color wing(255, 90, 143);
            const float shipY = 90.0f + std::sin(t * 2.0f) * 4.0f;
            const sf::Vector2f ship = toCard(o, 56.0f, shipY);

            // Wings first, so the body covers their roots
            drawTriangle(target, nudge(ship, -4.0f, -6.0f), nudge(ship, -16.0f, -20.0f), nudge(ship, 10.0f, -6.0f),
                         wing);
            drawTriangle(target, nudge(ship, -4.0f, 6.0f), nudge(ship, -16.0f, 20.0f), nudge(ship, 10.0f, 6.0f),
                         wing);
            ui::drawShadedEllipse(target, ship, sf::Vector2f{sc(22.0f), sc(11.0f)}, sf::Color(235, 235, 255),
                                  sf::Color(150, 190, 255), sf::Color(60, 80, 200));

            // Two bullets that loop from the nose toward the asteroid
            for (int b = 0; b < 2; ++b)
            {
                const float travel = std::fmod(t * 140.0f + 28.0f * static_cast<float>(b), 56.0f);
                const sf::Vector2f c = toCard(o, 56.0f + 30.0f + travel, shipY - 8.0f + 8.0f * static_cast<float>(b));
                ui::drawGlow(target, c, sc(20.0f), sf::Color(255, 220, 90, 120));
                ui::drawRoundedRect(target, rectAt(c.x - sc(10.0f), c.y - sc(3.0f), sc(20.0f), sc(6.0f)), sc(3.0f),
                                    sf::Color(255, 230, 120));
                ui::drawRoundedRect(target, rectAt(c.x - sc(6.0f), c.y - sc(1.2f), sc(12.0f), sc(2.4f)), sc(1.2f),
                                    sf::Color::White);
            }

            // Asteroid: convex polygon with darker facets
            const sf::Vector2f rock = toCard(o, 168.0f, 64.0f);
            const std::array<float, 8> radii{{24.0f, 21.0f, 25.0f, 20.0f, 24.0f, 22.0f, 19.0f, 25.0f}};
            std::array<sf::Vector2f, 8> points{};
            for (std::size_t k = 0; k < points.size(); ++k)
            {
                const float angle = 0.1745f + 0.7854f * static_cast<float>(k);
                points[k] = sf::Vector2f{rock.x + std::cos(angle) * sc(radii[k]), rock.y + std::sin(angle) * sc(radii[k])};
            }
            sf::ConvexShape asteroid(points.size());
            for (std::size_t k = 0; k < points.size(); ++k)
            {
                asteroid.setPoint(k, points[k]);
            }
            asteroid.setFillColor(sf::Color(150, 150, 165));
            asteroid.setOutlineColor(sf::Color(60, 60, 80));
            asteroid.setOutlineThickness(sc(2.0f));
            target.draw(asteroid);
            for (std::size_t k = 0; k < points.size(); k += 2)
            {
                drawTriangle(target, rock, points[k], points[(k + 1) % points.size()], sf::Color(90, 92, 112, 110));
            }

            // Orange burst where the bullets hit
            const float pulse = 1.0f + 0.12f * std::sin(t * 8.0f);
            ui::drawStarburst(target, toCard(o, 146.0f, 70.0f), sc(22.0f * pulse), sc(11.0f), 9, 15.0f,
                              sf::Color(255, 240, 140), sf::Color(255, 120, 30), 7u);
        }

        // Snake of five shaded beads curving toward a red fruit
        void drawSlitherArt(sf::RenderTarget &target, sf::Vector2f o, float t)
        {
            std::array<sf::Vector2f, 5> beads{};
            for (std::size_t k = 0; k < beads.size(); ++k)
            {
                const float fk = static_cast<float>(k);
                beads[k] = toCard(o, 34.0f + 30.0f * fk,
                                  98.0f - 20.0f * std::sin(fk * 0.9f) + std::sin(t * 2.5f - fk * 0.9f) * 4.0f);
            }

            // Tail first, so the head is drawn last; beads grow toward the head
            for (std::size_t k = 0; k < beads.size(); ++k)
            {
                const float r = 13.0f + static_cast<float>(k);
                ui::drawShadedEllipse(target, beads[k], sf::Vector2f{sc(r), sc(r * 0.92f)}, sf::Color(230, 255, 220),
                                      sf::Color(90, 235, 130), sf::Color(16, 120, 70));
            }

            const sf::Vector2f head = beads.back();
            drawDisc(target, nudge(head, 4.0f, -5.0f), sc(3.6f), sf::Color::White);
            drawDisc(target, nudge(head, 10.0f, -4.0f), sc(3.6f), sf::Color::White);
            drawDisc(target, nudge(head, 5.5f, -4.5f), sc(1.8f), kInk);
            drawDisc(target, nudge(head, 11.5f, -3.5f), sc(1.8f), kInk);

            // Red fruit with a little stalk
            const sf::Vector2f fruit = toCard(o, 192.0f, 100.0f + std::sin(t * 3.0f) * 3.0f);
            ui::drawRoundedRect(target, rectAt(fruit.x - sc(1.5f), fruit.y - sc(17.0f), sc(3.0f), sc(8.0f)), sc(1.5f),
                                sf::Color(110, 70, 40));
            ui::drawShadedEllipse(target, fruit, sf::Vector2f{sc(11.0f), sc(11.0f)}, sf::Color(255, 190, 190),
                                  sf::Color(235, 40, 60), sf::Color(120, 10, 30));
            drawDisc(target, nudge(fruit, -4.0f, -4.0f), sc(2.5f), sf::Color(255, 255, 255, 180));
        }

        // Tower of five rainbow slabs with a sixth slab hovering above and drifting left and right
        void drawStackArt(sf::RenderTarget &target, sf::Vector2f o, float t)
        {
            const std::array<float, 5> widths{{116.0f, 100.0f, 84.0f, 68.0f, 52.0f}};
            for (std::size_t k = 0; k < widths.size(); ++k)
            {
                const float fk = static_cast<float>(k);
                drawSlab(target, o, 115.0f - widths[k] * 0.5f, 110.0f - 18.0f * fk, widths[k],
                         hsvColor(60.0f * fk, 0.75f, 1.0f));
            }

            const float slide = std::sin(t * 1.6f) * 8.0f;
            const float bob = std::sin(t * 2.4f) * 1.5f;
            drawSlab(target, o, 89.0f + slide, 20.0f + bob, 52.0f, hsvColor(300.0f, 0.75f, 1.0f));

            // Two small sparkles that twinkle at different phases
            const float sizeA = 8.0f + 2.0f * std::sin(t * 4.0f);
            const float sizeB = 6.0f + 2.0f * std::sin(t * 4.0f + 1.5f);
            ui::drawSparkle(target, toCard(o, 40.0f, 70.0f), sc(sizeA), sf::Color::White, t * 30.0f);
            ui::drawSparkle(target, toCard(o, 188.0f, 26.0f), sc(sizeB), sf::Color::White, -t * 30.0f);
        }

        void drawIllustration(sf::RenderTarget &target, int id, sf::Vector2f origin, float t)
        {
            switch (id)
            {
            case 0:
                drawBounceArt(target, origin, t);
                break;
            case 1:
                drawSmashArt(target, origin, t);
                break;
            case 2:
                drawBlasterArt(target, origin, t);
                break;
            case 3:
                drawSlitherArt(target, origin, t);
                break;
            default:
                drawStackArt(target, origin, t);
                break;
            }
        }

        // One game card: glow (when hovered), slab, gradient face, border, illustration, name, tagline, chips
        void drawGameCard(sf::RenderTarget &target, const sf::Font &font, int id, float lift, float sink, float t,
                          int difficulty)
        {
            const Theme &theme = kThemes[static_cast<std::size_t>(id)];
            const sf::FloatRect base = baseRect(id);
            const sf::FloatRect face =
                rectAt(base.position.x, base.position.y - kRise * lift + kSink * sink, kCardWidth, kCardHeight);
            const sf::Vector2f center = face.position + face.size * 0.5f;

            if (lift > 0.01f)
            {
                ui::drawGlow(target, center, 170.0f, Flappy::withAlpha(theme.top, toByte(150.0f * lift)));
            }

            // Dark slab stays put: it shows more while the card lifts and less while it sinks
            ui::drawRoundedRect(target, rectAt(base.position.x, base.position.y + kSlab, kCardWidth, kCardHeight),
                                kRadius, Flappy::mix(theme.bottom, sf::Color(0, 0, 0), 0.3f));
            ui::drawRoundedRectGradient(target, face, kRadius, theme.top, theme.bottom);
            ui::drawRoundedRect(target, face, kRadius, sf::Color::Transparent,
                                sf::Color(255, 255, 255, toByte(120.0f + 135.0f * lift)), 3.0f);

            drawIllustration(target, id, face.position, t);

            const float cx = face.position.x + kCardWidth * 0.5f;
            const Flappy::GameInfo &info = Flappy::gameInfo(id);

            // Game name, shrunk when it is wider than the card
            const std::string name = info.name;
            const float nameWidth = sf::Text(font, name, kNameSize).getLocalBounds().size.x;
            const float nameScale = nameWidth > kNameMaxWidth ? kNameMaxWidth / nameWidth : 1.0f;
            ui::drawExtrudedText(target, font, name, kNameSize, {cx, face.position.y + 170.0f}, ui::orangeStyle(),
                                 nameScale);

            // Tagline, one or two lines
            const std::vector<std::string> lines = wrapTagline(info.tagline);
            const float firstY = lines.size() > 1 ? 208.0f : 217.0f;
            for (std::size_t k = 0; k < lines.size(); ++k)
            {
                ui::drawLabel(target, font, lines[k], kTaglineSize,
                              {cx, face.position.y + firstY + 18.0f * static_cast<float>(k)}, kMuted);
            }

            // Best score chip for the current difficulty
            const int best = UserData::instance().best(id, difficulty);
            ui::drawRoundedRect(target, rectAt(face.position.x + 32.0f, face.position.y + 280.0f, 120.0f, 30.0f),
                                15.0f, Flappy::withAlpha(sf::Color(14, 8, 60), 220), sf::Color(255, 210, 63, 200),
                                2.0f);
            ui::drawLabel(target, font, "BEST " + std::to_string(best), 18u,
                          {cx, face.position.y + 295.0f}, kGold, false, 1.0f);

            // Best score of any profile, with the owner's name (only when somebody has scored)
            std::string owner;
            const int top = UserData::instance().globalBest(id, difficulty, &owner);
            if (top > 0)
            {
                const std::string line = "TOP " + std::to_string(top) + " " + owner.substr(0, kTopNameLength);
                ui::drawLabel(target, font, line, kTopSize, {cx, face.position.y + 320.0f}, kGold, false, 1.0f);
            }
        }
    } // namespace

    GamePickerPanel::GamePickerPanel(const sf::Font &font)
        : m_font(&font),
          m_back(font, std::string("Back"), kBack, Flappy::UiKit::Button3DStyle::secondary(), 28u)
    {
    }

    void GamePickerPanel::onOpen()
    {
        m_open = 0.0f;
        m_time = 0.0f;
        m_closeRequested = false;
        m_launchGame = -1;
        m_hoverCard = -1;
        m_pressedCard = -1;
        m_backPressed = false;
        m_lift.fill(0.0f);
        m_sink.fill(0.0f);
        m_back.setHovered(false);
        m_back.setPressed(false);
    }

    void GamePickerPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);

        // Ease the hover lift and the press sink of every card toward its target
        const float ease = std::min(1.0f, 12.0f * dt);
        for (int i = 0; i < kCards; ++i)
        {
            const auto k = static_cast<std::size_t>(i);
            m_lift[k] += ((i == m_hoverCard ? 1.0f : 0.0f) - m_lift[k]) * ease;
            m_sink[k] += ((i == m_pressedCard ? 1.0f : 0.0f) - m_sink[k]) * ease;
        }
        m_back.update(dt);
    }

    void GamePickerPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, kCard, "CHOOSE A GAME", m_open);
        if (m_open <= 0.3f)
        {
            return;
        }

        const UserData &data = UserData::instance();
        const int difficulty = std::clamp(data.settings().difficulty, 0, 2);
        const std::string status = std::string("Playing as ") + data.active().name + "  -  " +
                                   Flappy::difficultyParams(difficulty).name;
        Flappy::UiKit::drawLabel(target, *m_font, status, 24, {640.0f, 556.0f}, kMuted, true, 1.0f);

        auto drawOne = [&](int i)
        {
            const auto k = static_cast<std::size_t>(i);
            drawGameCard(target, *m_font, i, m_lift[k], m_sink[k], m_time, difficulty);
        };

        // The hovered card is drawn last so its glow sits on top of its neighbours
        for (int i = 0; i < kCards; ++i)
        {
            if (i != m_hoverCard)
            {
                drawOne(i);
            }
        }
        if (m_hoverCard >= 0)
        {
            drawOne(m_hoverCard);
        }

        m_back.draw(target);
    }

    void GamePickerPanel::handleMouseMove(sf::Vector2f point)
    {
        m_hoverCard = cardAt(point);
        m_back.setHovered(m_back.contains(point));
    }

    void GamePickerPanel::handleMouseDown(sf::Vector2f point)
    {
        m_pressedCard = cardAt(point);
        m_backPressed = m_back.contains(point);
        m_back.setPressed(m_backPressed);
    }

    void GamePickerPanel::handleMouseUp(sf::Vector2f point)
    {
        // A card launches only when released over the same card it was pressed on
        const int released = cardAt(point);
        if (m_pressedCard >= 0 && released == m_pressedCard)
        {
            m_launchGame = released;
        }
        m_pressedCard = -1;

        // Back closes only when released over the button that was pressed
        const bool clicked = m_backPressed && m_back.contains(point);
        m_backPressed = false;
        m_back.setPressed(false);
        if (clicked)
        {
            m_closeRequested = true;
        }
    }

    void GamePickerPanel::handleKey(sf::Keyboard::Key key)
    {
        switch (key)
        {
        case sf::Keyboard::Key::Num1:
            m_launchGame = 0;
            break;
        case sf::Keyboard::Key::Num2:
            m_launchGame = 1;
            break;
        case sf::Keyboard::Key::Num3:
            m_launchGame = 2;
            break;
        case sf::Keyboard::Key::Num4:
            m_launchGame = 3;
            break;
        case sf::Keyboard::Key::Num5:
            m_launchGame = 4;
            break;
        default:
            break;
        }
    }
} // namespace Panels
