#include "Jello.h"
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
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr float kHistoryStep = 0.035f; // seconds between trail samples
        constexpr std::size_t kHistoryMax = 6;
        constexpr float kBodyRx = 68.0f; // body radii before squash & stretch
        constexpr float kBodyRy = 58.0f;
        constexpr float kScale = 0.68f; // overall sprite scale (keeps the blob small next to the pipe gap)

        const sf::Color kInk(27, 15, 74);
        const sf::Color kMouth(74, 15, 58);
        const sf::Color kTongue(255, 122, 168);
        const sf::Color kBlush(255, 111, 160, 130);
        const sf::Color kRim(127, 244, 255, 200);
        const sf::Color kSweat(127, 244, 255, 230);
        const sf::Color kShadowInk(0, 0, 20);

        // Point on an ellipse (radii rx, ry) rotated by rotDeg around c, at parametric angle a (radians).
        sf::Vector2f ellipsePoint(sf::Vector2f c, float rx, float ry, float rotDeg, float a)
        {
            const float x = rx * std::cos(a);
            const float y = ry * std::sin(a);
            const float r = rotDeg * kPi / 180.0f;
            return sf::Vector2f{c.x + x * std::cos(r) - y * std::sin(r),
                                c.y + x * std::sin(r) + y * std::cos(r)};
        }

        // Filled ellipse as a triangle fan; inner color at the center, edge color on the rim.
        void fillEllipse(sf::RenderTarget &target, const sf::RenderStates &states, sf::Vector2f c, float rx, float ry,
                         float rotDeg, sf::Color inner, sf::Color edge)
        {
            constexpr int kSegments = 36;
            sf::VertexArray va(sf::PrimitiveType::TriangleFan);
            va.append(sf::Vertex{c, inner});
            for (int i = 0; i <= kSegments; ++i)
            {
                const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(kSegments);
                va.append(sf::Vertex{ellipsePoint(c, rx, ry, rotDeg, a), edge});
            }
            target.draw(va, states);
        }

        void fillEllipse(sf::RenderTarget &target, const sf::RenderStates &states, sf::Vector2f c, float rx, float ry,
                         float rotDeg, sf::Color color)
        {
            fillEllipse(target, states, c, rx, ry, rotDeg, color, color);
        }

        // Thick arc along an ellipse outline (centered on the outline). With fadeLower the alpha ramps
        // up toward the bottom of the shape, which gives the rim light its fading look.
        void strokeArc(sf::RenderTarget &target, const sf::RenderStates &states, sf::Vector2f c, float rx, float ry,
                       float thickness, float a0, float a1, sf::Color color, bool fadeLower)
        {
            constexpr int kSegments = 48;
            const float h = thickness * 0.5f;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip);
            for (int i = 0; i <= kSegments; ++i)
            {
                const float a = a0 + (a1 - a0) * static_cast<float>(i) / static_cast<float>(kSegments);
                sf::Color col = color;
                if (fadeLower)
                {
                    const float k = std::clamp((std::sin(a) - 0.1f) / 0.7f, 0.0f, 1.0f);
                    col.a = static_cast<std::uint8_t>(color.a * k);
                }
                va.append(sf::Vertex{ellipsePoint(c, rx + h, ry + h, 0.0f, a), col});
                va.append(sf::Vertex{ellipsePoint(c, rx - h, ry - h, 0.0f, a), col});
            }
            target.draw(va, states);
        }

        // Thick polyline (quads joined into a strip).
        void strokePolyline(sf::RenderTarget &target, const sf::RenderStates &states,
                            const std::vector<sf::Vector2f> &pts, float thickness, sf::Color color)
        {
            if (pts.size() < 2)
                return;
            const float h = thickness * 0.5f;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip);
            for (std::size_t i = 0; i < pts.size(); ++i)
            {
                const sf::Vector2f a = pts[i > 0 ? i - 1 : 0];
                const sf::Vector2f b = pts[i + 1 < pts.size() ? i + 1 : i];
                const float dx = b.x - a.x;
                const float dy = b.y - a.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                const float nx = len > 0.0f ? -dy / len * h : 0.0f;
                const float ny = len > 0.0f ? dx / len * h : h;
                va.append(sf::Vertex{sf::Vector2f{pts[i].x + nx, pts[i].y + ny}, color});
                va.append(sf::Vertex{sf::Vector2f{pts[i].x - nx, pts[i].y - ny}, color});
            }
            target.draw(va, states);
        }
    } // namespace

    Jello::Jello(const Accent &accent) : m_accent(accent) {}

    void Jello::reset(float y)
    {
        m_pos = sf::Vector2f{kPlayerX, y};
        m_baseY = y;
        m_velY = 0.0f;
        m_time = 0.0f;
        m_dead = false;
        m_flapEvent = false;
        m_stretch = 0.0f;
        m_stretchVel = 0.0f;
        m_tilt = 0.0f;
        m_history.clear();
        m_historyTimer = 0.0f;
    }

    void Jello::flap()
    {
        if (m_dead)
            return;
        m_velY = kFlapSpeed;
        m_stretchVel = 9.0f; // springy kick: stretch tall, then bounce back
        m_flapEvent = true;
    }

    void Jello::kill()
    {
        if (m_dead)
            return;
        m_dead = true;
        m_velY = 0.0f;
        m_stretchVel = -6.0f; // small splat recoil
        m_history.clear();
        m_historyTimer = 0.0f;
    }

    void Jello::update(float dt, bool physicsActive)
    {
        m_time += dt;

        // Only a living blob under physics moves; a dead one stays where it fell.
        const bool moving = physicsActive && !m_dead;

        // Damped spring for squash & stretch. Rest value is 0, nudged while falling fast or idling.
        float rest = 0.0f;
        if (moving)
            rest = std::clamp(m_velY / kMaxFallSpeed, 0.0f, 1.0f) * 0.12f;
        else if (!m_dead)
            rest = 0.06f * std::sin(m_time * 3.0f);
        m_stretchVel += (-120.0f * (m_stretch - rest) - 12.0f * m_stretchVel) * dt;
        m_stretch += m_stretchVel * dt;

        if (moving)
        {
            m_velY = std::min(m_velY + kGravity * dt, kMaxFallSpeed);
            m_pos.y += m_velY * dt;
        }
        else if (!m_dead)
        {
            m_pos.y = m_baseY + std::sin(m_time * 3.0f) * 10.0f;
        }

        // Tilt follows the vertical speed (nose down when falling); dead pose tilts to 18 degrees.
        float tiltTarget = 0.0f;
        if (m_dead)
            tiltTarget = 18.0f;
        else if (physicsActive)
            tiltTarget = std::clamp(m_velY * 0.075f, -26.0f, 55.0f);
        m_tilt += (tiltTarget - m_tilt) * (1.0f - std::exp(-10.0f * dt));

        // Trail samples are recorded only while flying.
        if (moving)
        {
            m_historyTimer += dt;
            if (m_historyTimer >= kHistoryStep)
            {
                m_historyTimer -= kHistoryStep;
                m_history.push_front(m_pos);
                if (m_history.size() > kHistoryMax)
                    m_history.pop_back();
            }
        }
        else
        {
            m_history.clear();
            m_historyTimer = 0.0f;
        }
    }

    sf::FloatRect Jello::hitbox() const
    {
        return sf::FloatRect(sf::Vector2f{m_pos.x - kPlayerHalfW, m_pos.y - kPlayerHalfH},
                             sf::Vector2f{2.0f * kPlayerHalfW, 2.0f * kPlayerHalfH});
    }

    bool Jello::consumeFlapEvent()
    {
        const bool event = m_flapEvent;
        m_flapEvent = false;
        return event;
    }

    void Jello::drawShadow(sf::RenderTarget &target) const
    {
        // Higher = smaller and fainter shadow
        const float t = std::clamp((kGroundY - m_pos.y) / kGroundY, 0.0f, 1.0f);
        const float width = std::lerp(110.0f, 55.0f, t) * kScale;
        const auto alpha = static_cast<std::uint8_t>(std::lerp(150.0f, 50.0f, t));
        const sf::Vector2f c{m_pos.x, kGroundY - 6.0f};
        fillEllipse(target, sf::RenderStates{}, c, width * 0.5f, 9.0f, 0.0f,
                    sf::Color(kShadowInk.r, kShadowInk.g, kShadowInk.b, alpha),
                    sf::Color(kShadowInk.r, kShadowInk.g, kShadowInk.b, 0));
    }

    void Jello::drawTrail(sf::RenderTarget &target) const
    {
        // m_history[0] is the newest sample; draw the oldest (fainter, smaller) first.
        const std::size_t n = m_history.size();
        for (std::size_t k = n; k-- > 0;)
        {
            const float radius = (46.0f - 8.0f * static_cast<float>(k)) * kScale;
            const float alpha = std::max(0.15f, 0.55f - 0.08f * static_cast<float>(k));
            sf::Color c = m_accent.glow;
            c.a = static_cast<std::uint8_t>(alpha * 255.0f);
            UiKit::drawGlow(target, m_history[k], radius, c);
        }

        // Speed lines trailing to the left of the blob
        if (n == 0 || m_dead)
            return;

        struct SpeedLine
        {
            float dy;
            float len;
            float h;
        };
        const SpeedLine lines[] = {{-30.0f, 150.0f, 3.0f}, {-12.0f, 200.0f, 2.0f}, {8.0f, 110.0f, 2.0f}, {24.0f, 80.0f, 2.5f}};

        const float frontX = m_pos.x - 72.0f * kScale;
        std::size_t i = 0;
        for (const SpeedLine &ln : lines)
        {
            const float len = ln.len * (0.85f + 0.15f * std::sin(m_time * 12.0f + static_cast<float>(i)));
            const float y = m_pos.y + ln.dy;
            const float x0 = frontX - len;
            const float x1 = frontX;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip);
            va.append(sf::Vertex{sf::Vector2f{x0, y}, sf::Color(255, 255, 255, 0)});
            va.append(sf::Vertex{sf::Vector2f{x0, y + ln.h}, sf::Color(255, 255, 255, 0)});
            va.append(sf::Vertex{sf::Vector2f{x1, y}, sf::Color(255, 255, 255, 170)});
            va.append(sf::Vertex{sf::Vector2f{x1, y + ln.h}, sf::Color(255, 255, 255, 170)});
            target.draw(va);
            ++i;
        }
    }

    void Jello::draw(sf::RenderTarget &target) const
    {
        const float sx = (1.0f - 0.30f * m_stretch) * (m_dead ? 1.25f : 1.0f);
        const float sy = (1.0f + 0.45f * m_stretch) * (m_dead ? 0.70f : 1.0f);

        // Body: shaded ellipse in absolute coordinates
        UiKit::drawShadedEllipse(target, m_pos, sf::Vector2f{kBodyRx * sx * kScale, kBodyRy * sy * kScale},
                                 m_accent.light, m_accent.base, m_accent.dark, m_tilt);

        // Everything else is drawn in body-local space (origin = body center) with one transform
        sf::Transform tr;
        tr.translate(m_pos);
        tr.rotate(sf::degrees(m_tilt));
        tr.scale(sf::Vector2f{sx * kScale, sy * kScale});
        sf::RenderStates states;
        states.transform = tr;

        // Subsurface glow, outline and cyan rim light along the lower edge
        fillEllipse(target, states, sf::Vector2f{1.0f, 35.0f}, 46.0f, 16.0f, 0.0f, withAlpha(m_accent.light, 90));
        strokeArc(target, states, sf::Vector2f{0.0f, 0.0f}, kBodyRx - 1.5f, kBodyRy - 1.5f, 3.0f, 0.0f, kTwoPi,
                  withAlpha(m_accent.dark, 180), false);
        strokeArc(target, states, sf::Vector2f{0.0f, 0.0f}, 65.5f, 56.5f, 3.0f, 0.0f, kPi, kRim, true);

        // Highlights
        fillEllipse(target, states, sf::Vector2f{-30.0f, -30.0f}, 26.0f, 13.0f, -30.0f,
                    withAlpha(sf::Color::White, 190));
        UiKit::drawSparkle(target, tr.transformPoint(sf::Vector2f{-6.0f, -44.0f}), 7.0f,
                           withAlpha(sf::Color::White, 230));
        fillEllipse(target, states, sf::Vector2f{45.0f, 25.0f}, 9.0f, 4.0f, 40.0f, withAlpha(sf::Color::White, 90));

        // Blush
        fillEllipse(target, states, sf::Vector2f{5.0f, 15.0f}, 7.0f, 4.0f, 0.0f, kBlush);
        fillEllipse(target, states, sf::Vector2f{49.0f, 15.0f}, 7.0f, 4.0f, 0.0f, kBlush);

        if (!m_dead)
        {
            // Eyes: white, dark pupil slightly lower-right, tiny glint
            for (const float ex : {13.0f, 39.0f})
            {
                fillEllipse(target, states, sf::Vector2f{ex, -9.0f}, 11.0f, 13.0f, 0.0f, sf::Color::White);
                const sf::Vector2f pupil{ex + 3.0f, -7.0f};
                fillEllipse(target, states, pupil, 6.5f, 6.5f, 0.0f, kInk);
                fillEllipse(target, states, sf::Vector2f{pupil.x + 2.5f, pupil.y - 3.0f}, 2.3f, 2.3f, 0.0f,
                            sf::Color::White);
            }

            // Open mouth with a pink tongue
            sf::VertexArray mouth(sf::PrimitiveType::TriangleFan);
            const sf::Vector2f mouthC{28.0f, 15.0f};
            mouth.append(sf::Vertex{mouthC, kMouth});
            constexpr int kMouthSegments = 12;
            for (int i = 0; i <= kMouthSegments; ++i)
            {
                const float a = kPi * static_cast<float>(i) / static_cast<float>(kMouthSegments);
                mouth.append(sf::Vertex{ellipsePoint(mouthC, 8.0f, 6.5f, 0.0f, a), kMouth});
            }
            target.draw(mouth, states);
            fillEllipse(target, states, sf::Vector2f{28.0f, 19.5f}, 4.0f, 2.0f, 0.0f, kTongue);
        }
        else
        {
            // Dead: X eyes, frown, sweat drop
            for (const float ex : {12.0f, 38.0f})
            {
                const sf::Vector2f c{ex, -7.0f};
                strokePolyline(target, states, {sf::Vector2f{c.x - 8.0f, c.y - 8.0f}, sf::Vector2f{c.x + 8.0f, c.y + 8.0f}},
                               5.0f, kInk);
                strokePolyline(target, states, {sf::Vector2f{c.x + 8.0f, c.y - 8.0f}, sf::Vector2f{c.x - 8.0f, c.y + 8.0f}},
                               5.0f, kInk);
            }

            std::vector<sf::Vector2f> frown;
            constexpr int kFrownSegments = 12;
            for (int i = 0; i <= kFrownSegments; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(kFrownSegments);
                const float u = 1.0f - t;
                frown.push_back(sf::Vector2f{u * u * 14.0f + 2.0f * u * t * 26.0f + t * t * 38.0f,
                                             u * u * 18.0f + 2.0f * u * t * 8.0f + t * t * 18.0f});
            }
            strokePolyline(target, states, frown, 5.0f, kMouth);

            fillEllipse(target, states, sf::Vector2f{57.0f, 8.0f}, 4.0f, 4.0f, 0.0f, kSweat);
            sf::VertexArray drop(sf::PrimitiveType::Triangles);
            drop.append(sf::Vertex{sf::Vector2f{57.0f, 1.0f}, kSweat});
            drop.append(sf::Vertex{sf::Vector2f{53.5f, 8.0f}, kSweat});
            drop.append(sf::Vertex{sf::Vector2f{60.5f, 8.0f}, kSweat});
            target.draw(drop, states);
        }
    }
} // namespace Flappy
