#include "Scene.h"
#include "UiKit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Flappy
{
    namespace
    {
        constexpr float kTau = 6.2831853f;
        constexpr float kHillPeriod = 640.0f;      // hill crests repeat every 640px, so the scroll wraps seamlessly
        constexpr float kGridVpY = 448.0f;         // vanishing point of the floor lines (just above the horizon)
        constexpr float kGridBottomSpacing = 100.0f;
        constexpr float kGridRowScale = 1.3f;      // horizontal line k sits at horizon + c * (k + frac)^2
        constexpr float kGridScrollCell = 60.0f;   // world px per one grid row step
        constexpr std::size_t kStarCount = 60;
        constexpr float kSunX = 880.0f;
        constexpr float kSunRadius = 145.0f;
        constexpr std::size_t kSunSegments = 96;
        constexpr std::size_t kCloudSegments = 32;
        constexpr float kCloudAlpha = 0.8f;
        const sf::Color kGridCyan(90, 245, 255);

        struct Stop
        {
            float at;
            sf::Color color;
        };

        const std::array<Stop, 6> kSkyStops{{
            {0.0f, sf::Color(10, 15, 60)},
            {0.24f * kHeight, sf::Color(26, 42, 122)},
            {0.44f * kHeight, sf::Color(31, 111, 176)},
            {0.60f * kHeight, sf::Color(39, 196, 192)},
            {0.69f * kHeight, sf::Color(255, 212, 121)},
            {0.74f * kHeight, sf::Color(255, 138, 92)}}};

        const std::array<Stop, 4> kFloorStops{{
            {kHorizonY, sf::Color(255, 138, 92)},
            {kHorizonY + 0.05f * (kHeight - kHorizonY), sf::Color(138, 47, 149)},
            {kHorizonY + 0.28f * (kHeight - kHorizonY), sf::Color(28, 14, 88)},
            {kHeight, sf::Color(10, 6, 50)}}};

        // Piecewise-linear color ramp through `stops` (sorted by `at`)
        sf::Color ramp(std::span<const Stop> stops, float at)
        {
            if (at <= stops.front().at)
            {
                return stops.front().color;
            }
            for (std::size_t i = 1; i < stops.size(); ++i)
            {
                if (at <= stops[i].at)
                {
                    const float t = (at - stops[i - 1].at) / (stops[i].at - stops[i - 1].at);
                    return mix(stops[i - 1].color, stops[i].color, t);
                }
            }
            return stops.back().color;
        }

        // Smooth 0..1 transition between e0 and e1
        float ease(float e0, float e1, float x)
        {
            const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        // Full-width horizontal band of the sky / floor gradient
        void appendRow(sf::VertexArray &va, float y, sf::Color color)
        {
            va.append(sf::Vertex{{0.0f, y}, color});
            va.append(sf::Vertex{{kWidth, y}, color});
        }

        // Convex quad a-b-c-d as two triangles
        void appendQuad(sf::VertexArray &va, const sf::Vertex &a, const sf::Vertex &b, const sf::Vertex &c,
                        const sf::Vertex &d)
        {
            va.append(a);
            va.append(b);
            va.append(c);
            va.append(a);
            va.append(c);
            va.append(d);
        }

        // Quad from the horizon (topX, width topW) down to the bottom edge (botX, width botW)
        void appendFloorLine(sf::VertexArray &va, float topX, float botX, float topW, float botW, sf::Color top,
                             sf::Color bottom)
        {
            appendQuad(va, sf::Vertex{{topX - topW * 0.5f, kHorizonY}, top},
                       sf::Vertex{{topX + topW * 0.5f, kHorizonY}, top},
                       sf::Vertex{{botX + botW * 0.5f, kHeight}, bottom},
                       sf::Vertex{{botX - botW * 0.5f, kHeight}, bottom});
        }

        // Hill silhouette: crest sampled every 8px, filled down to the horizon
        void appendHill(sf::VertexArray &va, float offset, float (*crest)(float), sf::Color color)
        {
            for (int i = 0; i <= 160; ++i)
            {
                const float x = static_cast<float>(i) * 8.0f;
                va.append(sf::Vertex{{x, crest(x + offset)}, color});
                va.append(sf::Vertex{{x, kHorizonY}, color});
            }
        }

        float farCrest(float x)
        {
            return 370.0f + 22.0f * std::sin(kTau * x / kHillPeriod + 0.6f) +
                   12.0f * std::sin(2.0f * kTau * x / kHillPeriod + 2.1f) +
                   6.0f * std::sin(3.0f * kTau * x / kHillPeriod + 0.3f);
        }

        float nearCrest(float x)
        {
            return 434.0f + 10.0f * std::sin(kTau * x / kHillPeriod + 1.3f) +
                   6.0f * std::sin(2.0f * kTau * x / kHillPeriod + 0.4f) +
                   3.0f * std::sin(3.0f * kTau * x / kHillPeriod + 2.0f);
        }

        // Soft elliptical cloud: radial gradient centred low (warm underside), violet body, faded outline.
        // Built as concentric rings around the gradient centre, clipped to the cloud outline.
        void appendCloud(sf::VertexArray &va, float x, float y, float w, float h)
        {
            const sf::Vector2f shapeCenter{x + w * 0.5f, y + h * 0.5f};
            const float ax = w * 0.5f;
            const float ay = h * 0.5f;
            const sf::Vector2f glowCenter{x + w * 0.5f, y + h * 0.85f};
            const float gx = ax / 0.74f; // gradient reaches its transparent stop (d = 0.74) at the sides...
            const float gy = (h * 0.85f) / 0.74f; // ...and at the top edge
            const sf::Vector2f q{glowCenter.x - shapeCenter.x, glowCenter.y - shapeCenter.y};

            const std::array<Stop, 3> stops{{
                {0.0f, sf::Color(255, 180, 130, 230)},
                {0.55f, sf::Color(150, 90, 210, 150)},
                {0.74f, sf::Color(90, 60, 200, 0)}}};

            auto colorAt = [&](sf::Vector2f p) -> sf::Color
            {
                const float dx = (p.x - glowCenter.x) / gx;
                const float dy = (p.y - glowCenter.y) / gy;
                const float ex = (p.x - shapeCenter.x) / ax;
                const float ey = (p.y - shapeCenter.y) / ay;
                const float edge = 1.0f - ease(0.7f, 1.0f, std::sqrt(ex * ex + ey * ey));
                sf::Color c = ramp(stops, std::sqrt(dx * dx + dy * dy));
                c.a = static_cast<std::uint8_t>(static_cast<float>(c.a) * kCloudAlpha * edge);
                return c;
            };

            // Distance from the glow centre to the cloud outline along unit direction u
            auto outlineDist = [&](sf::Vector2f u)
            {
                const float A = (u.x / ax) * (u.x / ax) + (u.y / ay) * (u.y / ay);
                const float B = 2.0f * (q.x * u.x / (ax * ax) + q.y * u.y / (ay * ay));
                const float C = (q.x / ax) * (q.x / ax) + (q.y / ay) * (q.y / ay) - 1.0f;
                return (-B + std::sqrt(B * B - 4.0f * A * C)) / (2.0f * A);
            };

            constexpr std::array<float, 3> levels{0.55f, 0.74f, 1000.0f}; // last one = outline
            std::array<std::array<sf::Vector2f, kCloudSegments>, levels.size()> rings{};
            for (std::size_t i = 0; i < kCloudSegments; ++i)
            {
                const float ang = kTau * static_cast<float>(i) / static_cast<float>(kCloudSegments);
                const sf::Vector2f u{std::cos(ang), std::sin(ang)};
                const float k = std::sqrt((u.x / gx) * (u.x / gx) + (u.y / gy) * (u.y / gy));
                const float outline = outlineDist(u);
                for (std::size_t r = 0; r < levels.size(); ++r)
                {
                    rings[r][i] = glowCenter + u * std::min(levels[r] / k, outline);
                }
            }

            auto emit = [&](sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2)
            {
                va.append(sf::Vertex{p0, colorAt(p0)});
                va.append(sf::Vertex{p1, colorAt(p1)});
                va.append(sf::Vertex{p2, colorAt(p2)});
            };
            for (std::size_t i = 0; i < kCloudSegments; ++i)
            {
                const std::size_t j = (i + 1) % kCloudSegments;
                emit(glowCenter, rings[0][i], rings[0][j]);
                for (std::size_t r = 0; r + 1 < levels.size(); ++r)
                {
                    emit(rings[r][i], rings[r + 1][i], rings[r][j]);
                    emit(rings[r][j], rings[r + 1][i], rings[r + 1][j]);
                }
            }
        }
    } // namespace

    Scene::Scene()
    {
        // Deterministic LCG, so the starfield is identical every run
        std::uint32_t seed = 12345u;
        auto nextUnit = [&seed]()
        {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>(seed >> 8) / 16777216.0f; // [0, 1)
        };

        m_stars.reserve(kStarCount);
        for (std::size_t i = 0; i < kStarCount; ++i)
        {
            Star s{};
            s.pos = {nextUnit() * kWidth, nextUnit() * 400.0f};
            s.radius = 0.7f + 1.1f * nextUnit();
            s.phase = kTau * nextUnit();
            m_stars.push_back(s);
        }

        // Base positions; they drift left with m_cloudOffset
        m_clouds = {
            {60.0f, 200.0f, 420.0f, 120.0f},
            {860.0f, 110.0f, 480.0f, 130.0f},
            {380.0f, 330.0f, 380.0f, 90.0f},
            {520.0f, 170.0f, 240.0f, 70.0f},
            {1160.0f, 300.0f, 240.0f, 70.0f},
            {-80.0f, 360.0f, 260.0f, 64.0f}};
    }

    void Scene::update(float dt, float scrollSpeed)
    {
        m_time += dt;
        m_farOffset = std::fmod(m_farOffset + scrollSpeed * 0.12f * dt, kHillPeriod);
        m_nearOffset = std::fmod(m_nearOffset + scrollSpeed * 0.3f * dt, kHillPeriod);
        m_cloudOffset = std::fmod(m_cloudOffset + scrollSpeed * 0.05f * dt, kWidth);
        m_gridOffset = std::fmod(m_gridOffset + scrollSpeed * dt / kGridScrollCell, 1.0f);
    }

    void Scene::draw(sf::RenderTarget &target) const
    {
        // Sky: gradient rows from the top down to the horizon
        sf::VertexArray sky(sf::PrimitiveType::TriangleStrip);
        for (const Stop &s : kSkyStops)
        {
            if (s.at >= kHorizonY)
            {
                break;
            }
            appendRow(sky, s.at, s.color);
        }
        appendRow(sky, kHorizonY, ramp(kSkyStops, kHorizonY));
        target.draw(sky);

        // Stars: tiny twinkling discs, every 15th one is a bright sparkle
        sf::VertexArray stars(sf::PrimitiveType::Triangles);
        constexpr int kStarSegments = 6;
        for (std::size_t i = 0; i < m_stars.size(); ++i)
        {
            const Star &s = m_stars[i];
            const float tw = 0.5f + 0.5f * std::sin(m_time * (1.2f + 0.15f * s.phase) + s.phase);
            const auto a = static_cast<std::uint8_t>(255.0f * (0.25f + 0.75f * tw));
            if (i % 15 == 7)
            {
                UiKit::drawSparkle(target, s.pos, 6.0f + 2.5f * s.radius, sf::Color(191, 246, 255, a));
                continue;
            }
            const sf::Color c(255, 255, 255, a);
            for (int k = 0; k < kStarSegments; ++k)
            {
                const float a0 = kTau * static_cast<float>(k) / kStarSegments;
                const float a1 = kTau * static_cast<float>(k + 1) / kStarSegments;
                const sf::Vector2f p0 = s.pos + sf::Vector2f{std::cos(a0), std::sin(a0)} * s.radius;
                const sf::Vector2f p1 = s.pos + sf::Vector2f{std::cos(a1), std::sin(a1)} * s.radius;
                stars.append(sf::Vertex{s.pos, c});
                stars.append(sf::Vertex{p0, c});
                stars.append(sf::Vertex{p1, c});
            }
        }
        target.draw(stars);

        // Sun: big glow, outer glow, then the disc (the floor hides its lower half)
        const sf::Vector2f sunC{kSunX, kHorizonY};
        UiKit::drawGlow(target, sunC, 350.0f, sf::Color(255, 170, 90, 165));
        UiKit::drawGlow(target, sunC, kSunRadius + 120.0f, sf::Color(255, 150, 80, 140));

        const std::array<Stop, 3> sunStops{{
            {kHorizonY - kSunRadius, sf::Color(255, 247, 176)},
            {kHorizonY, sf::Color(255, 194, 71)},
            {kHorizonY + kSunRadius, sf::Color(255, 90, 138)}}};
        sf::VertexArray sun(sf::PrimitiveType::TriangleFan);
        sun.append(sf::Vertex{sunC, ramp(sunStops, sunC.y)});
        for (std::size_t i = 0; i <= kSunSegments; ++i)
        {
            const float ang = kTau * static_cast<float>(i) / static_cast<float>(kSunSegments);
            const sf::Vector2f p = sunC + sf::Vector2f{std::cos(ang), std::sin(ang)} * kSunRadius;
            sun.append(sf::Vertex{p, ramp(sunStops, p.y)});
        }
        target.draw(sun);

        // Clouds: wrap around the screen, drawn twice so the seam is covered
        sf::VertexArray clouds(sf::PrimitiveType::Triangles);
        for (const Cloud &c : m_clouds)
        {
            float x = std::fmod(c.x - m_cloudOffset, kWidth);
            if (x < 0.0f)
            {
                x += kWidth;
            }
            appendCloud(clouds, x, c.y, c.w, c.h);
            appendCloud(clouds, x - kWidth, c.y, c.w, c.h);
        }
        target.draw(clouds);

        // Hills: far layer, then the near layer in front of it
        sf::VertexArray farHills(sf::PrimitiveType::TriangleStrip);
        appendHill(farHills, m_farOffset, farCrest, sf::Color(74, 63, 176, 255));
        target.draw(farHills);

        sf::VertexArray nearHills(sf::PrimitiveType::TriangleStrip);
        appendHill(nearHills, m_nearOffset, nearCrest, sf::Color(36, 26, 134, 255));
        target.draw(nearHills);

        // Floor base gradient
        sf::VertexArray floorStrip(sf::PrimitiveType::TriangleStrip);
        for (const Stop &s : kFloorStops)
        {
            appendRow(floorStrip, s.at, s.color);
        }
        target.draw(floorStrip);

        // Neon perspective grid
        sf::VertexArray grid(sf::PrimitiveType::Triangles);
        const float vpX = kWidth * 0.5f;
        const float factorTop = (kHorizonY - kGridVpY) / (kHeight - kGridVpY);
        const float shift = m_gridOffset * kGridBottomSpacing;

        // Vertical lines: straight rays from the vanishing point through the bottom edge
        for (int i = -82; i <= 82; ++i)
        {
            const float xb = vpX + static_cast<float>(i) * kGridBottomSpacing - shift;
            const float xt = vpX + (xb - vpX) * factorTop;
            // Wide faint glow, then the thin bright core; both fade in toward the bottom edge
            appendFloorLine(grid, xt, xb, 1.2f * 3.5f, 3.0f * 3.5f, sf::Color(90, 245, 255, 12),
                            sf::Color(90, 245, 255, 45));
            appendFloorLine(grid, xt, xb, 1.2f, 3.0f, sf::Color(90, 245, 255, 40), sf::Color(90, 245, 255, 220));
        }

        // Horizontal lines: rows at horizon + c * (k + frac)^2, moving toward the viewer
        for (int k = 0; k < 40; ++k)
        {
            const float t = static_cast<float>(k) + m_gridOffset;
            const float y = kHorizonY + kGridRowScale * t * t;
            if (y > kHeight)
            {
                break;
            }
            const float fade = ease(kHorizonY, kHorizonY + 140.0f, y);
            const float th = 2.0f + ease(kHorizonY, kHeight, y); // 2..3 px
            const sf::Color glow(kGridCyan.r, kGridCyan.g, kGridCyan.b, static_cast<std::uint8_t>(40.0f * fade));
            const sf::Color core(kGridCyan.r, kGridCyan.g, kGridCyan.b, static_cast<std::uint8_t>(220.0f * fade));
            const float gh = th * 1.5f;
            appendQuad(grid, sf::Vertex{{0.0f, y - gh}, glow}, sf::Vertex{{kWidth, y - gh}, glow},
                       sf::Vertex{{kWidth, y + gh}, glow}, sf::Vertex{{0.0f, y + gh}, glow});
            const float ch = th * 0.5f;
            appendQuad(grid, sf::Vertex{{0.0f, y - ch}, core}, sf::Vertex{{kWidth, y - ch}, core},
                       sf::Vertex{{kWidth, y + ch}, core}, sf::Vertex{{0.0f, y + ch}, core});
        }
        target.draw(grid);

        // Warm glow band at the top of the floor
        sf::VertexArray band(sf::PrimitiveType::Triangles);
        const sf::Color bandTop(255, 150, 100, 140);
        const sf::Color bandBottom(255, 150, 100, 0);
        appendQuad(band, sf::Vertex{{0.0f, kHorizonY}, bandTop}, sf::Vertex{{kWidth, kHorizonY}, bandTop},
                   sf::Vertex{{kWidth, kHorizonY + 70.0f}, bandBottom},
                   sf::Vertex{{0.0f, kHorizonY + 70.0f}, bandBottom});
        target.draw(band);
    }

    void Scene::drawVignette(sf::RenderTarget &target, float strength) const
    {
        strength = std::clamp(strength, 0.0f, 1.0f);
        constexpr std::size_t kCols = 20;
        constexpr std::size_t kRows = 12;

        // Ellipse metric centred on (640, 400): fully transparent inside 55%, dark violet at the edges
        auto colorAt = [strength](sf::Vector2f p) -> sf::Color
        {
            const float dx = (p.x - 640.0f) / 760.0f;
            const float dy = (p.y - 400.0f) / 520.0f;
            const float t = ease(0.55f, 1.0f, std::sqrt(dx * dx + dy * dy));
            return sf::Color(8, 0, 40, static_cast<std::uint8_t>(140.0f * strength * t));
        };
        auto gridPoint = [](std::size_t c, std::size_t r) -> sf::Vector2f
        {
            return sf::Vector2f{kWidth * static_cast<float>(c) / static_cast<float>(kCols),
                                kHeight * static_cast<float>(r) / static_cast<float>(kRows)};
        };

        sf::VertexArray va(sf::PrimitiveType::Triangles);
        for (std::size_t r = 0; r < kRows; ++r)
        {
            for (std::size_t c = 0; c < kCols; ++c)
            {
                const sf::Vector2f a = gridPoint(c, r);
                const sf::Vector2f b = gridPoint(c + 1, r);
                const sf::Vector2f d = gridPoint(c, r + 1);
                const sf::Vector2f e = gridPoint(c + 1, r + 1);
                va.append(sf::Vertex{a, colorAt(a)});
                va.append(sf::Vertex{b, colorAt(b)});
                va.append(sf::Vertex{e, colorAt(e)});
                va.append(sf::Vertex{a, colorAt(a)});
                va.append(sf::Vertex{e, colorAt(e)});
                va.append(sf::Vertex{d, colorAt(d)});
            }
        }
        target.draw(va);
    }
} // namespace Flappy
