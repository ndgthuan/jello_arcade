#include "MenuMascot.h"
#include "UiKit.h"
#include "../../Data/Profile/UserData.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kTwoPi = 2.0f * kPi;
        constexpr float kBodyRx = 84.0f;        // body radii at scale 1 (window height 1000)
        constexpr float kBodyRy = 72.0f;
        constexpr float kHopUnit = 84.0f;       // one hop unit in scaled pixels (the body radius)
        constexpr float kHopGravity = 40.0f;    // hop gravity in units per second squared
        constexpr float kBlinkTime = 0.18f;     // seconds for one blink
        constexpr float kNearDistance = 220.0f; // mouse closer than this (scaled) makes the mascot happy
        constexpr float kHoverHop = 6.0f;       // little hop when the mouse first comes close
        constexpr float kPulseHop = 17.0f;      // big bounce from pulse()
        constexpr float kRebound = 10.0f;       // landings faster than this bounce again...
        constexpr float kReboundFactor = 0.35f; // ...at this fraction of the speed (17 -> 6, so only once)
        constexpr float kSquashLimit = 0.45f;

        const sf::Color kInk(27, 15, 74);
        const sf::Color kMouth(74, 15, 58);
        const sf::Color kTongue(255, 122, 168);
        const sf::Color kRim(127, 244, 255, 200);

        // Next random value in [0, 1). A tiny LCG; there is only one mascot, so one shared state is enough.
        float nextRandom01()
        {
            static std::uint32_t state = 2463534242u;
            state = state * 1664525u + 1013904223u;
            return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
        }

        float nextBlinkDelay()
        {
            return 2.0f + 3.0f * nextRandom01();
        }

        // Moves value toward target by at most maxStep
        float approach(float value, float target, float maxStep)
        {
            return value + std::clamp(target - value, -maxStep, maxStep);
        }

        // Point on an ellipse (radii rx, ry) rotated by rotDeg around c, at parametric angle a (radians).
        sf::Vector2f ellipsePoint(sf::Vector2f c, float rx, float ry, float rotDeg, float a)
        {
            const float x = rx * std::cos(a);
            const float y = ry * std::sin(a);
            const float r = rotDeg * kPi / 180.0f;
            return sf::Vector2f{c.x + x * std::cos(r) - y * std::sin(r),
                                c.y + x * std::sin(r) + y * std::cos(r)};
        }

        // Filled ellipse arc as a triangle fan from angle a0 to a1; inner colour at the centre, edge colour on the rim.
        void fillFan(sf::RenderTarget &target, sf::Vector2f c, float rx, float ry, float rotDeg, float a0, float a1,
                     int segments, sf::Color inner, sf::Color edge)
        {
            sf::VertexArray va(sf::PrimitiveType::TriangleFan);
            va.append(sf::Vertex{c, inner});
            for (int i = 0; i <= segments; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(segments);
                va.append(sf::Vertex{ellipsePoint(c, rx, ry, rotDeg, a0 + (a1 - a0) * t), edge});
            }
            target.draw(va);
        }

        void fillEllipse(sf::RenderTarget &target, sf::Vector2f c, float rx, float ry, float rotDeg, sf::Color color)
        {
            fillFan(target, c, rx, ry, rotDeg, 0.0f, kTwoPi, 32, color, color);
        }

        // Thick arc centred on an ellipse outline. With fadeLower the alpha ramps up toward the bottom (rim light).
        void strokeArc(sf::RenderTarget &target, sf::Vector2f c, float rx, float ry, float thickness, float a0,
                       float a1, sf::Color color, bool fadeLower)
        {
            constexpr int kSegments = 40;
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
            target.draw(va);
        }

        // Flat elliptical shadow on the floor: a squashed circle.
        void drawFlatShadow(sf::RenderTarget &target, sf::Vector2f center, float rx, std::uint8_t alpha)
        {
            sf::CircleShape shape(1.0f, 40);
            shape.setOrigin(sf::Vector2f{1.0f, 1.0f});
            shape.setPosition(center);
            shape.setScale(sf::Vector2f{rx, rx * 0.25f});
            shape.setFillColor(sf::Color(12, 8, 40, alpha));
            target.draw(shape);
        }

        // Uniform scale: 1000 logical pixels of window height = 1.0
        float scaleFor(sf::Vector2u windowSize)
        {
            return static_cast<float>(std::max(windowSize.y, 1u)) / 1000.0f;
        }

        // Body radii (window pixels) for the scale and squash-and-stretch
        sf::Vector2f radiiFor(float s, float squash)
        {
            return sf::Vector2f{kBodyRx * s * (1.0f - 0.3f * squash), kBodyRy * s * (1.0f + 0.45f * squash)};
        }

        // Body centre: x at 13% of the width, bottom resting on the floor at 84% of the height
        sf::Vector2f centerFor(sf::Vector2u windowSize, float s, sf::Vector2f radii, float hop)
        {
            const float baseY = 0.84f * static_cast<float>(windowSize.y);
            return sf::Vector2f{0.13f * static_cast<float>(windowSize.x),
                                baseY - radii.y - hop * kHopUnit * s};
        }
    } // namespace

    MenuMascot::MenuMascot() = default;

    void MenuMascot::update(float dt, sf::Vector2f mousePixel, sf::Vector2u windowSize)
    {
        m_time += dt;

        // Blink: m_blinkTimer > 0 counts down to the next blink; <= 0 means blinking, -m_blinkTimer is the time into it.
        m_blinkTimer -= dt;
        if (m_blinkTimer > 0.0f)
        {
            m_blink = 0.0f;
        }
        else
        {
            const float e = -m_blinkTimer;
            if (e >= kBlinkTime)
            {
                m_blinkTimer = nextBlinkDelay();
                m_blink = 0.0f;
            }
            else
            {
                m_blink = std::sin(kPi * e / kBlinkTime);
            }
        }

        // Hop: gravity, and a bounce when the landing happens
        if (m_hop > 0.0f || m_hopVel > 0.0f)
        {
            m_hopVel -= kHopGravity * dt;
            m_hop += m_hopVel * dt;
            if (m_hop <= 0.0f)
            {
                const float impact = -m_hopVel;
                m_hop = 0.0f;
                m_squashVel -= 4.0f; // flat kick on landing
                m_hopVel = impact > kRebound ? impact * kReboundFactor : 0.0f;
            }
        }

        // Squash & stretch: damped spring toward the breathing + happy target
        const float squashTarget = 0.04f * std::sin(m_time * 2.2f) + 0.12f * m_happy;
        m_squashVel += (-90.0f * (m_squash - squashTarget) - 9.0f * m_squashVel) * dt;
        m_squash += m_squashVel * dt;
        if (m_squash > kSquashLimit || m_squash < -kSquashLimit)
        {
            m_squash = std::clamp(m_squash, -kSquashLimit, kSquashLimit);
            m_squashVel = 0.0f;
        }

        // Geometry for this frame (also the look origin)
        m_scale = scaleFor(windowSize);
        m_center = centerFor(windowSize, m_scale, radiiFor(m_scale, m_squash), m_hop);

        // Mouse proximity: happy easing, and a small excited hop on the first approach
        const sf::Vector2f toMouse = mousePixel - m_center;
        const float dist = std::sqrt(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
        const bool near = dist < kNearDistance * m_scale;
        if (near && m_happy < 0.05f && m_hop <= 0.0f)
            m_hopVel = kHoverHop;
        m_happy = approach(m_happy, near ? 1.0f : 0.0f, 6.0f * dt);

        // Eyes follow the mouse: unit vector toward it, eased
        sf::Vector2f target{0.0f, 0.0f};
        if (dist > 1e-3f)
            target = toMouse / dist;
        const float k = std::min(1.0f, 10.0f * dt);
        m_look += (target - m_look) * k;
        const float lookLen = std::sqrt(m_look.x * m_look.x + m_look.y * m_look.y);
        if (lookLen > 1.0f)
            m_look /= lookLen;
    }

    void MenuMascot::pulse()
    {
        m_hopVel = kPulseHop;
        m_squashVel += 5.0f; // stretch up first
    }

    void MenuMascot::draw(sf::RenderTarget &target, sf::Vector2u windowSize) const
    {
        const float s = scaleFor(windowSize);
        const float floorY = 0.84f * static_cast<float>(windowSize.y);
        const float air = std::clamp(m_hop / 3.0f, 0.0f, 1.0f); // 0 on the floor .. 1 high up

        const sf::Vector2f radii = radiiFor(s, m_squash);
        const sf::Vector2f c = centerFor(windowSize, s, radii, m_hop);
        const float rx = radii.x;
        const float ry = radii.y;

        const Accent accent = Accent::from(accentChoice(UserData::instance().settings().accentIndex));

        // Floor shadow: shrinks and fades while the mascot is in the air
        const float shadowRx = kBodyRx * s * 1.15f * std::lerp(1.0f, 0.55f, air);
        const auto shadowAlpha = static_cast<std::uint8_t>(std::lerp(130.0f, 60.0f, air));
        const sf::Vector2f shadowC{c.x, floorY};
        drawFlatShadow(target, shadowC, shadowRx * 1.35f, static_cast<std::uint8_t>(shadowAlpha / 3));
        drawFlatShadow(target, shadowC, shadowRx, shadowAlpha);

        // Soft glow behind the body
        UiKit::drawGlow(target, c, 2.2f * kBodyRx * s, accent.glow);

        // Body: shaded ellipse, dark outline ring, cyan rim light along the lower edge
        UiKit::drawShadedEllipse(target, c, radii, accent.light, accent.base, accent.dark);
        const float outline = 3.0f * s;
        strokeArc(target, c, rx - outline * 0.5f, ry - outline * 0.5f, outline, 0.0f, kTwoPi,
                  withAlpha(accent.dark, 200), false);
        strokeArc(target, c, rx - 3.0f * s, ry - 3.0f * s, 2.5f * s, 0.0f, kPi, kRim, true);

        // Highlights: big white specular top-left, a small bright dot and a sparkle
        fillEllipse(target, sf::Vector2f{c.x - 0.42f * rx, c.y - 0.5f * ry}, 0.3f * rx, 0.15f * ry, -30.0f,
                    withAlpha(sf::Color::White, 190));
        UiKit::drawSparkle(target, sf::Vector2f{c.x - 0.1f * rx, c.y - 0.72f * ry}, 8.0f * s,
                           withAlpha(sf::Color::White, 230));
        fillEllipse(target, sf::Vector2f{c.x + 0.66f * rx, c.y + 0.43f * ry}, 0.13f * rx, 0.07f * ry, 40.0f,
                    withAlpha(sf::Color::White, 90));

        // Face: eyes follow the mouse and blink, pink blush, open smile
        const float R = rx;
        const float open = 1.0f - m_blink;
        const float pupilR = 0.11f * R;
        const float maxOffset = std::min(0.18f * R, 0.25f * R - pupilR); // keeps the pupil inside the white
        for (const float side : {-1.0f, 1.0f})
        {
            const sf::Vector2f eye{c.x + side * 0.28f * R, c.y - 0.1f * R};
            fillEllipse(target, eye, 0.25f * R, 0.29f * R * open, 0.0f, sf::Color::White);
            const sf::Vector2f pupil{eye.x + m_look.x * maxOffset, eye.y + m_look.y * maxOffset};
            fillEllipse(target, pupil, pupilR, pupilR * open, 0.0f, kInk);
            const sf::Vector2f glint{pupil.x + 0.04f * R, pupil.y - 0.05f * R};
            fillEllipse(target, glint, 0.035f * R, 0.035f * R * open, 0.0f, sf::Color::White);
        }

        const auto blushAlpha = static_cast<std::uint8_t>(60.0f + 120.0f * m_happy);
        const sf::Color blush(255, 111, 160, blushAlpha);
        for (const float side : {-1.0f, 1.0f})
            fillEllipse(target, sf::Vector2f{c.x + side * 0.5f * R, c.y + 0.2f * R}, 0.14f * R, 0.07f * R, 0.0f,
                        blush);

        // Open smile: opens wider with happiness, pink tongue at the bottom
        const sf::Vector2f mouthC{c.x, c.y + 0.12f * R};
        const float mouthRy = (0.05f + 0.12f * m_happy) * R;
        fillFan(target, mouthC, 0.14f * R, mouthRy, 0.0f, 0.0f, kPi, 16, kMouth, kMouth);
        fillEllipse(target, sf::Vector2f{mouthC.x, mouthC.y + 0.45f * mouthRy}, 0.08f * R, 0.4f * mouthRy, 0.0f,
                    kTongue);

        // Three orbiting sparkles while happy
        if (m_happy > 0.3f)
        {
            const float fade = std::clamp((m_happy - 0.3f) / 0.3f, 0.0f, 1.0f);
            const auto alpha = static_cast<std::uint8_t>(230.0f * fade);
            for (int i = 0; i < 3; ++i)
            {
                const float a = m_time * 1.6f + kTwoPi * static_cast<float>(i) / 3.0f;
                const sf::Vector2f p{c.x + std::cos(a) * rx * 1.3f, c.y + std::sin(a) * ry * 1.05f};
                const float size = 6.0f * s * (0.8f + 0.2f * std::sin(m_time * 5.0f + static_cast<float>(i)));
                UiKit::drawSparkle(target, p, size, withAlpha(sf::Color::White, alpha));
            }
        }
    }
} // namespace Flappy
