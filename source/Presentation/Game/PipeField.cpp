#include "PipeField.h"
#include "UiKit.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = 3.14159265358979f;
        constexpr float kFirstPipeOffset = 200.0f; // first pipe spawns this far past the right edge
        constexpr float kMaxGapStep = 260.0f;      // max change of gap center between neighbours
        constexpr float kScoredShade = 0.82f;      // scored pipes are darker
        constexpr float kRimOffset = (kRimWidth - kPipeWidth) / 2.0f; // rim overhang on each side
        constexpr float kRimRadius = 14.0f;
        constexpr float kOutline = 3.0f;           // navy outline thickness
        constexpr float kCullMargin = 200.0f;      // draw pipes that are this far off screen

        const sf::Color kNavy(6, 32, 92);

        // Horizontal color stops across the width (t in 0..1)
        struct PipeGradient
        {
            std::array<float, 7> stops;
            std::array<sf::Color, 7> colors;
        };

        const PipeGradient kBodyGradient{
            {0.0f, 0.16f, 0.34f, 0.43f, 0.52f, 0.78f, 1.0f},
            {sf::Color(11, 47, 122), sf::Color(26, 127, 224), sf::Color(99, 220, 255), sf::Color(234, 252, 255),
             sf::Color(99, 220, 255), sf::Color(26, 127, 224), sf::Color(11, 47, 122)}};

        const PipeGradient kRimGradient{
            {0.0f, 0.14f, 0.32f, 0.42f, 0.52f, 0.78f, 1.0f},
            {sf::Color(11, 47, 122), sf::Color(42, 147, 238), sf::Color(127, 230, 255), sf::Color(255, 255, 255),
             sf::Color(127, 230, 255), sf::Color(42, 147, 238), sf::Color(11, 47, 122)}};

        sf::Color sampleGradient(const PipeGradient &g, float t)
        {
            for (std::size_t i = 1; i < g.stops.size(); ++i)
            {
                if (t <= g.stops[i])
                {
                    const float span = g.stops[i] - g.stops[i - 1];
                    const float k = span > 0.0f ? (t - g.stops[i - 1]) / span : 0.0f;
                    return mix(g.colors[i - 1], g.colors[i], k);
                }
            }
            return g.colors.back();
        }

        sf::Color scaleRgb(sf::Color c, float k)
        {
            auto s = [k](std::uint8_t v)
            { return static_cast<std::uint8_t>(v * k); };
            return sf::Color(s(c.r), s(c.g), s(c.b), c.a);
        }

        // Vertical distance from the rectangle's top (or bottom) edge at horizontal offset dx
        float cornerInset(float dx, float width, float radius)
        {
            float d = 0.0f;
            if (dx < radius)
                d = radius - dx;
            else if (dx > width - radius)
                d = dx - (width - radius);
            else
                return 0.0f;
            return radius - std::sqrt(std::max(0.0f, radius * radius - d * d));
        }

        // Vertical strip with a horizontal gradient and rounded top / bottom corners
        sf::VertexArray gradientStrip(sf::FloatRect rect, float radius, const PipeGradient &g, float scale)
        {
            constexpr int kColumns = 34;
            sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 2 * kColumns);
            for (int i = 0; i < kColumns; ++i)
            {
                const float t = static_cast<float>(i) / (kColumns - 1);
                const float dx = t * rect.size.x;
                const float inset = cornerInset(dx, rect.size.x, radius);
                const sf::Color c = scaleRgb(sampleGradient(g, t), scale);
                const float x = rect.position.x + dx;
                va[2 * i].position = sf::Vector2f{x, rect.position.y + inset};
                va[2 * i].color = c;
                va[2 * i + 1].position = sf::Vector2f{x, rect.position.y + rect.size.y - inset};
                va[2 * i + 1].color = c;
            }
            return va;
        }

        std::array<sf::FloatRect, 4> pipeRects(const Pipe &p, float gapSize)
        {
            const float gapTop = p.gapCenterY - gapSize / 2.0f;
            const float gapBottom = p.gapCenterY + gapSize / 2.0f;
            return {
                sf::FloatRect(sf::Vector2f{p.x, 0.0f}, sf::Vector2f{kPipeWidth, gapTop - kRimHeight}),
                sf::FloatRect(sf::Vector2f{p.x - kRimOffset, gapTop - kRimHeight}, sf::Vector2f{kRimWidth, kRimHeight}),
                sf::FloatRect(sf::Vector2f{p.x - kRimOffset, gapBottom}, sf::Vector2f{kRimWidth, kRimHeight}),
                sf::FloatRect(sf::Vector2f{p.x, gapBottom + kRimHeight},
                              sf::Vector2f{kPipeWidth, kGroundY - gapBottom - kRimHeight})};
        }

        // Soft dark ellipse on the ground under a pipe
        void drawContactShadow(sf::RenderTarget &target, float centerX)
        {
            constexpr int kSegments = 40;
            const float cy = kGroundY - 2.0f;
            sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
            fan.append(sf::Vertex{sf::Vector2f{centerX, cy}, sf::Color(0, 0, 20, 150)});
            for (int i = 0; i <= kSegments; ++i)
            {
                const float a = 2.0f * kPi * static_cast<float>(i) / kSegments;
                fan.append(sf::Vertex{sf::Vector2f{centerX + 80.0f * std::cos(a), cy + 14.0f * std::sin(a)},
                                      sf::Color(0, 0, 20, 0)});
            }
            target.draw(fan);
        }

        void drawBody(sf::RenderTarget &target, sf::FloatRect rect, float scale)
        {
            target.draw(gradientStrip(rect, 0.0f, kBodyGradient, scale));

            sf::RectangleShape edge(sf::Vector2f{kOutline, rect.size.y});
            edge.setFillColor(kNavy);
            edge.setPosition(rect.position);
            target.draw(edge);
            edge.setPosition(sf::Vector2f{rect.position.x + rect.size.x - kOutline, rect.position.y});
            target.draw(edge);
        }

        // Blurred drop shadow below a rim, built from three growing translucent rounded rects
        void drawRimShadow(sf::RenderTarget &target, sf::FloatRect rect)
        {
            constexpr float kOffsetY = 10.0f;
            for (int i = 0; i < 3; ++i)
            {
                const float e = 3.0f * static_cast<float>(i);
                const sf::FloatRect r(sf::Vector2f{rect.position.x - e, rect.position.y + kOffsetY - e},
                                      sf::Vector2f{rect.size.x + 2.0f * e, rect.size.y + 2.0f * e});
                UiKit::drawRoundedRect(target, r, kRimRadius + e, withAlpha(kNavy, 48));
            }
        }

        void drawRim(sf::RenderTarget &target, sf::FloatRect rect, float scale)
        {
            // Navy rounded base acts as the outline; the gradient is inset by the outline width
            UiKit::drawRoundedRect(target, rect, kRimRadius, kNavy);

            const sf::FloatRect inner(sf::Vector2f{rect.position.x + kOutline, rect.position.y + kOutline},
                                      sf::Vector2f{rect.size.x - 2.0f * kOutline, rect.size.y - 2.0f * kOutline});
            target.draw(gradientStrip(inner, kRimRadius - kOutline, kRimGradient, scale));

            // Bright highlight along the top edge
            sf::RectangleShape highlight(sf::Vector2f{rect.size.x - 2.0f * kRimRadius, kOutline});
            highlight.setFillColor(sf::Color(255, 255, 255, 150));
            highlight.setPosition(sf::Vector2f{rect.position.x + kRimRadius, rect.position.y + kOutline});
            target.draw(highlight);
        }

        void drawPipe(sf::RenderTarget &target, const Pipe &pipe, float gapSize)
        {
            const float scale = pipe.scored ? kScoredShade : 1.0f;
            const auto rects = pipeRects(pipe, gapSize);
            const sf::FloatRect &topBody = rects[0];
            const sf::FloatRect &topRim = rects[1];
            const sf::FloatRect &bottomRim = rects[2];
            const sf::FloatRect &bottomBody = rects[3];

            drawContactShadow(target, pipe.x + kPipeWidth / 2.0f);

            // Bodies first so the rim shadows fall on them
            drawBody(target, topBody, scale);
            drawBody(target, bottomBody, scale);

            drawRimShadow(target, topRim);
            drawRimShadow(target, bottomRim);

            drawRim(target, topRim, scale);
            drawRim(target, bottomRim, scale);
        }
    } // namespace

    PipeField::PipeField()
    {
        reset(0);
    }

    void PipeField::reset(unsigned int seed)
    {
        if (seed == 0u)
        {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            seed = static_cast<unsigned int>(now);
        }
        m_rngState = seed != 0u ? seed : 1u;

        m_pipes.clear();
        m_lastScored = sf::Vector2f{0.0f, 0.0f};

        spawn(kWidth + kFirstPipeOffset);
        while (m_pipes.back().x < kWidth + kPipeSpacing)
            spawn(m_pipes.back().x + kPipeSpacing);
    }

    void PipeField::update(float dt)
    {
        if (m_scrolling)
        {
            const float dx = m_speed * dt;
            for (Pipe &p : m_pipes)
                p.x -= dx;
        }

        std::erase_if(m_pipes, [](const Pipe &p)
                      { return p.x + kPipeWidth + kRimOffset < -20.0f; });

        if (m_pipes.empty())
            spawn(kWidth + kFirstPipeOffset);
        while (m_pipes.back().x < kWidth + kPipeSpacing)
            spawn(m_pipes.back().x + kPipeSpacing);
    }

    bool PipeField::collides(const sf::FloatRect &box) const
    {
        for (const Pipe &p : m_pipes)
        {
            for (const sf::FloatRect &r : pipeRects(p, m_gapSize))
            {
                if (box.findIntersection(r))
                    return true;
            }
        }
        return false;
    }

    int PipeField::collectScore(float playerX)
    {
        int count = 0;
        for (Pipe &p : m_pipes)
        {
            if (!p.scored && p.x + kPipeWidth < playerX)
            {
                p.scored = true;
                m_lastScored = sf::Vector2f{p.x + kPipeWidth / 2.0f, p.gapCenterY};
                ++count;
            }
        }
        return count;
    }

    float PipeField::nextGapCenterY() const
    {
        for (const Pipe &p : m_pipes)
        {
            if (!p.scored)
                return p.gapCenterY;
        }
        return kHeight / 2.0f;
    }

    void PipeField::draw(sf::RenderTarget &target) const
    {
        for (const Pipe &p : m_pipes)
        {
            if (p.x + kPipeWidth + kRimOffset < -kCullMargin || p.x - kRimOffset > kWidth + kCullMargin)
                continue;
            drawPipe(target, p, m_gapSize);
        }
    }

    void PipeField::setDifficulty(int difficulty)
    {
        const DifficultyParams params = difficultyParams(difficulty);
        m_gapSize = params.gap;
        m_speed = params.speed;
    }

    void PipeField::spawn(float x)
    {
        const float minGapCenter = kGapMargin + m_gapSize / 2.0f;
        const float maxGapCenter = kGroundY - kGapMargin - m_gapSize / 2.0f;
        const float prev = m_pipes.empty() ? (minGapCenter + maxGapCenter) / 2.0f : m_pipes.back().gapCenterY;
        const float lo = std::max(minGapCenter, prev - kMaxGapStep);
        const float hi = std::min(maxGapCenter, prev + kMaxGapStep);
        const float gapY = lo + nextRandom01() * (hi - lo);
        m_pipes.push_back(Pipe{x, gapY, false});
    }

    float PipeField::nextRandom01()
    {
        m_rngState = m_rngState * 1664525u + 1013904223u;
        return static_cast<float>(m_rngState >> 8) / 16777216.0f;
    }
} // namespace Flappy
