#include "UiKit.h"

#include <cmath>
#include <numbers>
#include <utility>

namespace Flappy::UiKit
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kDegToRad = kPi / 180.0f;
        constexpr int kArcSegments = 8; // per corner of a rounded rect
        constexpr int kGlowSegments = 32;

        // Point on a circle around `c` at angle `rad` (radians, y down)
        sf::Vector2f polar(sf::Vector2f c, float r, float rad)
        {
            return {c.x + r * std::cos(rad), c.y + r * std::sin(rad)};
        }

        // Rotates a vector around the origin by `deg` degrees
        sf::Vector2f rotateVec(sf::Vector2f v, float deg)
        {
            const float rad = deg * kDegToRad;
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            return {v.x * c - v.y * s, v.x * s + v.y * c};
        }

        // Multiplies a color's alpha by alpha / 255
        sf::Color modulate(sf::Color c, std::uint8_t alpha)
        {
            c.a = static_cast<std::uint8_t>(c.a * alpha / 255);
            return c;
        }

        sf::Vector2f rectCenter(sf::FloatRect r)
        {
            return {r.position.x + r.size.x * 0.5f, r.position.y + r.size.y * 0.5f};
        }

        float clampRadius(sf::FloatRect rect, float radius)
        {
            return std::max(0.0f, std::min(radius, std::min(rect.size.x, rect.size.y) * 0.5f));
        }

        sf::FloatRect inset(sf::FloatRect rect, float d)
        {
            return {{rect.position.x + d, rect.position.y + d}, {rect.size.x - 2.0f * d, rect.size.y - 2.0f * d}};
        }

        // Perimeter points of a rounded rect, clockwise on screen (closed implicitly)
        std::vector<sf::Vector2f> roundedContour(sf::FloatRect rect, float radius)
        {
            const float r = clampRadius(rect, radius);
            const float x0 = rect.position.x;
            const float y0 = rect.position.y;
            const float x1 = x0 + rect.size.x;
            const float y1 = y0 + rect.size.y;

            // Corner arc centers and start angles: top-left, top-right, bottom-right, bottom-left
            const sf::Vector2f centers[4] = {{x0 + r, y0 + r}, {x1 - r, y0 + r}, {x1 - r, y1 - r}, {x0 + r, y1 - r}};
            const float starts[4] = {kPi, 1.5f * kPi, 0.0f, 0.5f * kPi};

            std::vector<sf::Vector2f> pts;
            pts.reserve(4 * (kArcSegments + 1));
            for (int corner = 0; corner < 4; ++corner)
            {
                for (int k = 0; k <= kArcSegments; ++k)
                {
                    const float a = starts[corner] + 0.5f * kPi * static_cast<float>(k) / kArcSegments;
                    pts.push_back(polar(centers[corner], r, a));
                }
            }
            return pts;
        }

        // Solid convex fill: fan from `center` to the contour
        void fillContour(sf::RenderTarget &target, const std::vector<sf::Vector2f> &pts, sf::Vector2f center,
                         sf::Color color)
        {
            sf::VertexArray va(sf::PrimitiveType::TriangleFan, pts.size() + 2);
            va[0] = sf::Vertex{center, color};
            for (std::size_t i = 0; i <= pts.size(); ++i)
            {
                va[i + 1] = sf::Vertex{pts[i % pts.size()], color};
            }
            target.draw(va);
        }

        // Band between two contours with the same point count (used for borders)
        void drawBand(sf::RenderTarget &target, const std::vector<sf::Vector2f> &outer,
                      const std::vector<sf::Vector2f> &inner, sf::Color color)
        {
            const std::size_t n = outer.size();
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 2 * (n + 1));
            for (std::size_t i = 0; i <= n; ++i)
            {
                va[2 * i] = sf::Vertex{outer[i % n], color};
                va[2 * i + 1] = sf::Vertex{inner[i % n], color};
            }
            target.draw(va);
        }

        // Inner border of `thickness` drawn inside the rounded rect
        void drawBorder(sf::RenderTarget &target, sf::FloatRect rect, float radius, float thickness, sf::Color color)
        {
            const float t = std::max(0.0f, std::min(thickness, std::min(rect.size.x, rect.size.y) * 0.5f));
            if (t <= 0.0f || color.a == 0)
            {
                return;
            }
            const float r = clampRadius(rect, radius);
            drawBand(target, roundedContour(rect, r), roundedContour(inset(rect, t), std::max(0.0f, r - t)), color);
        }

        // Ellipse filled as a fan whose center is `fanCenter`, with color `centerColor` at the center and `rimColor` at the edge
        void fanEllipse(sf::RenderTarget &target, const sf::RenderStates &states, sf::Vector2f ellipseCenter,
                        sf::Vector2f radii, sf::Vector2f fanCenter, sf::Color centerColor, sf::Color rimColor)
        {
            constexpr int kSegs = 32;
            sf::VertexArray va(sf::PrimitiveType::TriangleFan, kSegs + 2);
            va[0] = sf::Vertex{fanCenter, centerColor};
            for (int i = 0; i <= kSegs; ++i)
            {
                const float a = 2.0f * kPi * static_cast<float>(i) / kSegs;
                const sf::Vector2f p{ellipseCenter.x + radii.x * std::cos(a), ellipseCenter.y + radii.y * std::sin(a)};
                va[i + 1] = sf::Vertex{p, rimColor};
            }
            target.draw(va, states);
        }

        // Transform that places local coordinates at `center` and rotates them around it
        sf::RenderStates placed(sf::Vector2f center, float rotationDeg)
        {
            sf::RenderStates states;
            states.transform.translate(center).rotate(sf::degrees(rotationDeg));
            return states;
        }

        // Small house icon (roof + body) used by the home button
        void drawHouseIcon(sf::RenderTarget &target, sf::Vector2f center, float size, sf::Color color)
        {
            const float cx = center.x;
            const float cy = center.y;

            sf::VertexArray roof(sf::PrimitiveType::TriangleFan, 3);
            roof[0] = sf::Vertex{{cx, cy - 0.62f * size}, color};
            roof[1] = sf::Vertex{{cx - 0.7f * size, cy - 0.02f * size}, color};
            roof[2] = sf::Vertex{{cx + 0.7f * size, cy - 0.02f * size}, color};
            target.draw(roof);

            drawRoundedRect(target, {{cx - 0.46f * size, cy - 0.04f * size}, {0.92f * size, 0.66f * size}},
                            0.06f * size, color);
        }
    } // namespace

    ExtrudeStyle orangeStyle()
    {
        ExtrudeStyle s;
        s.face = sf::Color::White;
        s.layers = {sf::Color(255, 243, 176), sf::Color(255, 210, 74), sf::Color(255, 154, 43),
                    sf::Color(229, 96, 26),   sf::Color(176, 46, 12),  sf::Color(107, 20, 8)};
        s.glow = sf::Color(255, 170, 60, 50);
        s.shadowAlpha = 0.55f;
        return s;
    }

    ExtrudeStyle pinkStyle()
    {
        ExtrudeStyle s;
        s.face = sf::Color::White;
        s.layers = {sf::Color(255, 208, 230), sf::Color(255, 122, 184), sf::Color(224, 51, 138),
                    sf::Color(168, 28, 104),  sf::Color(110, 15, 72),   sf::Color(60, 6, 48)};
        s.glow = sf::Color(255, 90, 170, 50);
        s.shadowAlpha = 0.55f;
        return s;
    }

    ExtrudeStyle redStyle()
    {
        ExtrudeStyle s;
        s.face = sf::Color::White;
        s.layers = {sf::Color(255, 208, 192), sf::Color(255, 122, 90), sf::Color(224, 46, 46),
                    sf::Color(158, 21, 32),   sf::Color(90, 10, 20)};
        s.glow = sf::Color(255, 70, 70, 50);
        s.shadowAlpha = 0.55f;
        return s;
    }

    ExtrudeStyle blueStyle()
    {
        ExtrudeStyle s;
        s.face = sf::Color::White;
        s.layers = {sf::Color(214, 228, 255), sf::Color(143, 176, 255), sf::Color(74, 107, 224),
                    sf::Color(42, 58, 168),   sf::Color(21, 31, 110)};
        s.glow = sf::Color(120, 160, 255, 50);
        s.shadowAlpha = 0.55f;
        return s;
    }

    ExtrudeStyle goldStyle()
    {
        ExtrudeStyle s;
        s.face = sf::Color(255, 251, 208);
        s.layers = {sf::Color(255, 243, 176), sf::Color(255, 194, 71), sf::Color(184, 107, 12)};
        s.shadowAlpha = 0.4f;
        return s;
    }

    void drawExtrudedText(sf::RenderTarget &target, const sf::Font &font, const std::string &text,
                          unsigned int characterSize, sf::Vector2f center, const ExtrudeStyle &style,
                          float scale, float rotationDeg, std::uint8_t alpha)
    {
        sf::Text label(font, text, characterSize);
        const sf::FloatRect bounds = label.getLocalBounds();
        const sf::Vector2f origin{bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f};
        label.setOrigin(origin);
        label.setScale({scale, scale});
        label.setRotation(sf::degrees(rotationDeg));
        label.setOutlineThickness(static_cast<float>(characterSize) * 0.035f);

        // Stack depth along the rotated downward axis
        const float step = style.step * scale;
        const std::size_t layerCount = style.layers.size();
        const float stackDepth = static_cast<float>(layerCount) * step;

        // Glow behind everything
        if (style.glow.a > 0)
        {
            const float radius = std::max(bounds.size.x * 0.55f, bounds.size.y * 1.2f) * scale;
            drawGlow(target, center, radius, modulate(style.glow, alpha));
        }

        // Soft drop shadow below the whole stack
        if (style.shadowAlpha > 0.0f)
        {
            const auto shadowA = static_cast<std::uint8_t>(std::clamp(style.shadowAlpha * alpha * 0.6f, 0.0f, 255.0f));
            const sf::Color shadow(0, 0, 0, shadowA);
            label.setFillColor(shadow);
            label.setOutlineColor(shadow);
            for (int k = 0; k < 3; ++k)
            {
                const float d = stackDepth + (2.0f + 2.0f * static_cast<float>(k)) * scale;
                label.setPosition(center + rotateVec({0.0f, d}, rotationDeg));
                target.draw(label);
            }
        }

        // Extrusion: farthest (largest offset) first, nearest last
        for (std::size_t j = layerCount; j-- > 0;)
        {
            const sf::Color c = modulate(style.layers[j], alpha);
            label.setFillColor(c);
            label.setOutlineColor(c);
            const float d = static_cast<float>(j + 1) * step;
            label.setPosition(center + rotateVec({0.0f, d}, rotationDeg));
            target.draw(label);
        }

        // Face on top
        const sf::Color face = modulate(style.face, alpha);
        label.setFillColor(face);
        label.setOutlineColor(face);
        label.setPosition(center);
        target.draw(label);
    }

    void drawLabel(sf::RenderTarget &target, const sf::Font &font, const std::string &text,
                   unsigned int characterSize, sf::Vector2f center, sf::Color color, bool shadow, float letterSpacing)
    {
        sf::Text label(font, text, characterSize);
        label.setLetterSpacing(letterSpacing);
        const sf::FloatRect bounds = label.getLocalBounds();
        label.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});

        if (shadow)
        {
            label.setFillColor(sf::Color(0, 20, 30, 110));
            label.setPosition({center.x, center.y + 2.0f});
            target.draw(label);
            label.setPosition({center.x, center.y + 4.0f});
            target.draw(label);
        }

        label.setFillColor(color);
        label.setPosition(center);
        target.draw(label);
    }

    void drawRoundedRect(sf::RenderTarget &target, sf::FloatRect rect, float radius, sf::Color fill,
                         sf::Color outline, float outlineThickness)
    {
        const float r = clampRadius(rect, radius);
        const float t = std::max(0.0f, std::min(outlineThickness, std::min(rect.size.x, rect.size.y) * 0.5f));
        if (t > 0.0f && outline.a > 0)
        {
            drawBorder(target, rect, r, t, outline);
            const sf::FloatRect in = inset(rect, t);
            fillContour(target, roundedContour(in, std::max(0.0f, r - t)), rectCenter(in), fill);
            return;
        }
        fillContour(target, roundedContour(rect, r), rectCenter(rect), fill);
    }

    void drawRoundedRectGradient(sf::RenderTarget &target, sf::FloatRect rect, float radius, sf::Color top,
                                 sf::Color bottom)
    {
        const std::vector<sf::Vector2f> pts = roundedContour(rect, radius);
        const float height = std::max(rect.size.y, 1.0f);

        sf::VertexArray va(sf::PrimitiveType::TriangleFan, pts.size() + 2);
        va[0] = sf::Vertex{rectCenter(rect), mix(top, bottom, 0.5f)};
        for (std::size_t i = 0; i <= pts.size(); ++i)
        {
            const sf::Vector2f p = pts[i % pts.size()];
            const float t = (p.y - rect.position.y) / height;
            va[i + 1] = sf::Vertex{p, mix(top, bottom, t)};
        }
        target.draw(va);
    }

    void drawGlow(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color)
    {
        sf::VertexArray va(sf::PrimitiveType::TriangleFan, kGlowSegments + 2);
        const sf::Color rim = withAlpha(color, 0);
        va[0] = sf::Vertex{center, color};
        for (int i = 0; i <= kGlowSegments; ++i)
        {
            const float a = 2.0f * kPi * static_cast<float>(i) / kGlowSegments;
            va[i + 1] = sf::Vertex{polar(center, radius, a), rim};
        }
        target.draw(va);
    }

    void drawShadedEllipse(sf::RenderTarget &target, sf::Vector2f center, sf::Vector2f radii,
                           sf::Color highlight, sf::Color mid, sf::Color edge, float rotationDeg)
    {
        // Bright spot sits toward the top-left of the ellipse
        const sf::Vector2f offset{-0.30f * radii.x, -0.35f * radii.y};
        const sf::RenderStates states = placed(center, rotationDeg);
        const sf::Vector2f origin{0.0f, 0.0f};

        // Outer ring: edge color at the rim, mid color at the offset center
        fanEllipse(target, states, origin, radii, offset, mid, edge);

        // Inner core: mid color at 60% size, highlight at the offset center
        fanEllipse(target, states, offset, {radii.x * 0.6f, radii.y * 0.6f}, offset, highlight, mid);
    }

    void drawStarburst(sf::RenderTarget &target, sf::Vector2f center, float outerRadius, float innerRadius,
                       int spikes, float rotationDeg, sf::Color colorInner, sf::Color colorOuter, unsigned int seed)
    {
        spikes = std::max(spikes, 3);
        const int count = spikes * 2;

        // Per-tip radius jitter (+-25%) from a tiny LCG so the burst looks the same every time
        std::vector<float> tipRadius(static_cast<std::size_t>(spikes));
        std::uint32_t state = seed;
        for (float &r : tipRadius)
        {
            state = state * 1664525u + 1013904223u;
            const float j = static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
            r = outerRadius * (0.75f + 0.5f * j);
        }

        const sf::Color inner = mix(colorInner, colorOuter, 0.35f);
        const float start = rotationDeg * kDegToRad - 0.5f * kPi;

        sf::VertexArray va(sf::PrimitiveType::TriangleFan, static_cast<std::size_t>(count) + 2);
        va[0] = sf::Vertex{center, colorInner};
        for (int k = 0; k <= count; ++k)
        {
            const int idx = k % count;
            const bool tip = idx % 2 == 0;
            const float r = tip ? tipRadius[static_cast<std::size_t>(idx / 2)] : innerRadius;
            const float a = start + static_cast<float>(idx) * kPi / static_cast<float>(spikes);
            va[k + 1] = sf::Vertex{polar(center, r, a), tip ? colorOuter : inner};
        }
        target.draw(va);
    }

    void drawRing(sf::RenderTarget &target, sf::Vector2f center, float radius, float thickness, sf::Color color)
    {
        // Outline grows outward from the shape, so shrink the shape to keep the band centered on `radius`
        const float t = std::max(thickness, 0.0f);
        sf::CircleShape ring(std::max(radius - t * 0.5f, 0.0f), 64);
        ring.setOrigin({ring.getRadius(), ring.getRadius()});
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineColor(color);
        ring.setOutlineThickness(t);
        target.draw(ring);
    }

    void drawSparkle(sf::RenderTarget &target, sf::Vector2f center, float size, sf::Color color, float rotationDeg)
    {
        // Eight points: four long tips at `size`, four concave corners at 0.25 * size
        constexpr int kPoints = 8;
        const float start = rotationDeg * kDegToRad;
        sf::VertexArray va(sf::PrimitiveType::TriangleFan, kPoints + 2);
        sf::Color core = mix(color, sf::Color::White, 0.6f);
        core.a = color.a;
        va[0] = sf::Vertex{center, core};
        for (int k = 0; k <= kPoints; ++k)
        {
            const int idx = k % kPoints;
            const float r = (idx % 2 == 0) ? size : size * 0.25f;
            const float a = start + static_cast<float>(idx) * kPi / 4.0f;
            va[k + 1] = sf::Vertex{polar(center, r, a), color};
        }
        target.draw(va);
    }

    void drawSunburstRays(sf::RenderTarget &target, sf::Vector2f center, float radius, int rayCount,
                          float rotationDeg, sf::Color color)
    {
        // 2 * rayCount slices around the circle; the even ones are drawn as rays
        const int n = std::max(rayCount, 1);
        const float slice = kPi / static_cast<float>(n);
        const float start = rotationDeg * kDegToRad;
        const sf::Color rim = withAlpha(color, 0);

        sf::VertexArray va(sf::PrimitiveType::Triangles, static_cast<std::size_t>(n) * 3);
        for (int i = 0; i < n; ++i)
        {
            const float a0 = start + static_cast<float>(2 * i) * slice;
            const float a1 = a0 + slice;
            const std::size_t base = static_cast<std::size_t>(i) * 3;
            va[base] = sf::Vertex{center, color};
            va[base + 1] = sf::Vertex{polar(center, radius, a0), rim};
            va[base + 2] = sf::Vertex{polar(center, radius, a1), rim};
        }
        target.draw(va);
    }

    Button3DStyle Button3DStyle::primary(const Accent &accent)
    {
        Button3DStyle s;
        s.top = accent.light;
        s.bottom = accent.base;
        s.edge = accent.dark;
        s.text = sf::Color(26, 10, 58);
        s.glow = withAlpha(accent.glow, 60);
        s.depth = 10.0f;
        s.radius = 26.0f;
        return s;
    }

    Button3DStyle Button3DStyle::secondary()
    {
        Button3DStyle s;
        s.top = sf::Color::White;
        s.bottom = sf::Color(207, 224, 255);
        s.edge = sf::Color(111, 134, 214);
        s.text = sf::Color(27, 31, 92);
        s.depth = 10.0f;
        s.radius = 26.0f;
        return s;
    }

    Button3D::Button3D(const sf::Font &font, std::string label, sf::FloatRect rect, Button3DStyle style,
                       unsigned int characterSize)
        : m_font(&font), m_label(std::move(label)), m_rect(rect), m_style(style), m_characterSize(characterSize)
    {
    }

    void Button3D::update(float dt)
    {
        const float target = m_hovered ? 1.0f : 0.0f;
        m_lift += (target - m_lift) * std::min(1.0f, 12.0f * dt);
    }

    void Button3D::draw(sf::RenderTarget &target) const
    {
        const float x = m_rect.position.x;
        const float w = m_rect.size.x;
        const float h = m_rect.size.y;
        const float r = m_style.radius;
        const float lift = 3.0f * m_lift;
        const float sink = m_pressed ? m_style.depth * 0.8f : 0.0f;

        // The face moves down when pressed; the slab bottom stays put, so the visible edge thins out
        const float faceY = m_rect.position.y - lift + sink;
        const float slabBottom = m_rect.position.y - lift + h + m_style.depth;
        const sf::FloatRect face{{x, faceY}, {w, h}};
        const sf::FloatRect slab{{x, faceY}, {w, slabBottom - faceY}};
        const sf::Vector2f faceCenter = rectCenter(face);

        if (m_style.glow.a > 0)
        {
            drawGlow(target, faceCenter, std::max(w, h) * 0.55f, m_style.glow);
        }

        // Edge slab under the face
        drawRoundedRect(target, slab, r, m_style.edge);

        // Face with a slight brighten on hover
        const float brighten = 0.12f * m_lift;
        const sf::Color top = mix(m_style.top, sf::Color::White, brighten);
        const sf::Color bottom = mix(m_style.bottom, sf::Color::White, brighten);
        drawRoundedRectGradient(target, face, r, top, bottom);
        drawBorder(target, face, r, 3.0f, m_style.edge);

        // Thin highlight near the top of the face
        const float hlInset = r * 0.5f;
        if (w > r * 2.0f)
        {
            drawRoundedRect(target, {{x + hlInset, faceY + 5.0f}, {w - 2.0f * hlInset, 4.0f}}, 2.0f,
                            sf::Color(255, 255, 255, 170));
        }

        if (m_iconHome)
        {
            drawHouseIcon(target, faceCenter, h * 0.42f, m_style.text);
        }
        else
        {
            drawLabel(target, *m_font, m_label, m_characterSize, faceCenter, m_style.text, false);
        }
    }
} // namespace Flappy::UiKit
