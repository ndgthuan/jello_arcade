#include "GameOverOverlay.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = 3.14159265358979f;

        // Multiplies the alpha of `c` by `a` (0..1)
        sf::Color faded(sf::Color c, float a)
        {
            c.a = static_cast<std::uint8_t>(std::lround(c.a * std::clamp(a, 0.0f, 1.0f)));
            return c;
        }

        float smooth01(float x)
        {
            x = std::clamp(x, 0.0f, 1.0f);
            return x * x * (3.0f - 2.0f * x);
        }

        float easeOutBack(float t)
        {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            const float u = t - 1.0f;
            return 1.0f + c3 * u * u * u + c1 * u * u;
        }

        // Slide + scale of the whole card. Content is laid out in design coordinates (1280x720)
        // and mapped through this frame.
        struct CardFrame
        {
            sf::Vector2f pivot{930.0f, 360.0f};
            float scale = 1.0f;
            float dx = 0.0f;

            sf::Vector2f map(sf::Vector2f p) const
            {
                return {pivot.x + dx + (p.x - pivot.x) * scale, pivot.y + (p.y - pivot.y) * scale};
            }

            sf::FloatRect rect(sf::FloatRect r) const
            {
                return {map(r.position), r.size * scale};
            }

            sf::Transform transform() const
            {
                sf::Transform t;
                t.translate({pivot.x + dx, pivot.y}).scale({scale, scale}).translate(-pivot);
                return t;
            }
        };

        CardFrame frameAt(float time)
        {
            const float p = easeOutBack(std::clamp(time / 0.45f, 0.0f, 1.0f));
            CardFrame f;
            f.scale = 0.94f + 0.06f * p;
            f.dx = (1.0f - p) * 120.0f;
            return f;
        }

        sf::Text makeText(const sf::Font &font, const std::string &str, unsigned int size, float spacingPx,
                          sf::Color color)
        {
            sf::Text text(font, str, size);
            text.setLetterSpacing(1.0f + spacingPx / (0.5f * static_cast<float>(size)));
            text.setFillColor(color);
            return text;
        }

        // Draws text with its left edge at x and its vertical middle at cy
        void placeText(sf::RenderTarget &target, const sf::RenderStates &st, sf::Text text, float x, float cy)
        {
            const sf::FloatRect b = text.getLocalBounds();
            text.setOrigin({b.position.x, b.position.y + b.size.y * 0.5f});
            text.setPosition({x, cy});
            target.draw(text, st);
        }

        float textWidth(const sf::Text &text)
        {
            return text.getLocalBounds().size.x;
        }

        void drawCircle(sf::RenderTarget &target, const sf::RenderStates &st, sf::Vector2f c, float r,
                        sf::Color fill, sf::Color outline = sf::Color::Transparent, float thickness = 0.0f)
        {
            sf::CircleShape circle(r);
            circle.setOrigin({r, r});
            circle.setPosition(c);
            circle.setFillColor(fill);
            circle.setOutlineColor(outline);
            circle.setOutlineThickness(thickness);
            target.draw(circle, st);
        }

        // Convex polygon (the caller splits concave shapes into convex pieces)
        void drawPoly(sf::RenderTarget &target, const sf::RenderStates &st, std::initializer_list<sf::Vector2f> pts,
                      sf::Color fill, sf::Color outline)
        {
            sf::ConvexShape poly(pts.size());
            std::size_t i = 0;
            for (const sf::Vector2f &p : pts)
                poly.setPoint(i++, p);
            poly.setFillColor(fill);
            poly.setOutlineColor(outline);
            poly.setOutlineThickness(3.0f);
            target.draw(poly, st);
        }

        // Five-point star as a triangle fan around `c`, first point at the top
        void drawStar(sf::RenderTarget &target, const sf::RenderStates &st, sf::Vector2f c, float outer, float inner,
                      sf::Color color)
        {
            sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
            fan.append(sf::Vertex{c, color});
            for (int i = 0; i <= 10; ++i)
            {
                const float a = (-90.0f + 36.0f * static_cast<float>(i)) * kPi / 180.0f;
                const float r = (i % 2 == 0) ? outer : inner;
                fan.append(sf::Vertex{{c.x + r * std::cos(a), c.y + r * std::sin(a)}, color});
            }
            target.draw(fan, st);
        }

        // Rounded rectangle with a vertical gradient, drawn as a fan so it can be rotated
        void drawRoundedPoly(sf::RenderTarget &target, const sf::RenderStates &st, sf::FloatRect r, float radius,
                             sf::Color top, sf::Color bottom)
        {
            const float rad = std::min(radius, std::min(r.size.x, r.size.y) * 0.5f);
            const float x0 = r.position.x;
            const float y0 = r.position.y;
            const float x1 = x0 + r.size.x;
            const float y1 = y0 + r.size.y;
            const sf::Vector2f corners[4] = {
                {x0 + rad, y0 + rad}, // top-left
                {x1 - rad, y0 + rad}, // top-right
                {x1 - rad, y1 - rad}, // bottom-right
                {x0 + rad, y1 - rad}  // bottom-left
            };
            const float startDeg[4] = {180.0f, 270.0f, 0.0f, 90.0f};
            auto colorAt = [&](float y)
            { return mix(top, bottom, (y - y0) / r.size.y); };

            sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
            fan.append(sf::Vertex{{x0 + r.size.x * 0.5f, y0 + r.size.y * 0.5f}, mix(top, bottom, 0.5f)});
            constexpr int kSeg = 8;
            for (int c = 0; c < 4; ++c)
            {
                for (int i = 0; i <= kSeg; ++i)
                {
                    const float a = (startDeg[c] + 90.0f * static_cast<float>(i) / kSeg) * kPi / 180.0f;
                    const sf::Vector2f p{corners[c].x + rad * std::cos(a), corners[c].y + rad * std::sin(a)};
                    fan.append(sf::Vertex{p, colorAt(p.y)});
                }
            }
            const sf::Vertex first = fan[1]; // close the outline
            fan.append(first);
            target.draw(fan, st);
        }

        sf::FloatRect offsetRect(sf::FloatRect r, float dx, float dy, float grow)
        {
            return {{r.position.x - grow + dx, r.position.y - grow + dy}, {r.size.x + 2.0f * grow, r.size.y + 2.0f * grow}};
        }
    } // namespace

    GameOverOverlay::GameOverOverlay(const sf::Font &font, const Accent &accent)
        : m_font(&font), m_accent(accent)
    {
        m_buttons.emplace_back(font, "Play again", sf::FloatRect({716.0f, 568.0f}, {336.0f, 76.0f}),
                               UiKit::Button3DStyle::primary(accent), 32);
        m_buttons.emplace_back(font, "", sf::FloatRect({1068.0f, 568.0f}, {76.0f, 76.0f}),
                               UiKit::Button3DStyle::secondary());
        m_buttons[1].setIconHome(true);
    }

    void GameOverOverlay::show(int score, int previousBest, bool newBest)
    {
        m_score = score;
        m_previousBest = previousBest;
        m_newBest = newBest;
        m_time = 0.0f;
        m_pressedIndex = -1;
        for (UiKit::Button3D &b : m_buttons)
            b.setPressed(false);
    }

    void GameOverOverlay::update(float dt)
    {
        m_time += dt;
        for (UiKit::Button3D &b : m_buttons)
            b.update(dt);
    }

    void GameOverOverlay::handleMouseMove(sf::Vector2f mouse)
    {
        const CardFrame frame = frameAt(m_time);
        for (UiKit::Button3D &b : m_buttons)
            b.setHovered(frame.rect(b.rect()).contains(mouse));
    }

    void GameOverOverlay::handleMouseDown(sf::Vector2f mouse)
    {
        m_pressedIndex = -1;
        for (UiKit::Button3D &b : m_buttons)
            b.setPressed(false);
        if (!acceptsInput())
            return;

        const CardFrame frame = frameAt(m_time);
        for (std::size_t i = 0; i < m_buttons.size(); ++i)
        {
            if (frame.rect(m_buttons[i].rect()).contains(mouse))
            {
                m_pressedIndex = static_cast<int>(i);
                m_buttons[i].setPressed(true);
                return;
            }
        }
    }

    Action GameOverOverlay::handleMouseUp(sf::Vector2f mouse)
    {
        const int pressed = m_pressedIndex;
        m_pressedIndex = -1;

        const CardFrame frame = frameAt(m_time);
        for (UiKit::Button3D &b : m_buttons)
        {
            b.setPressed(false);
            b.setHovered(frame.rect(b.rect()).contains(mouse));
        }

        if (!acceptsInput() || pressed < 0 || pressed >= static_cast<int>(m_buttons.size()))
            return Action::None;
        if (!frame.rect(m_buttons[pressed].rect()).contains(mouse))
            return Action::None;
        return pressed == 0 ? Action::Restart : Action::Menu;
    }

    void GameOverOverlay::draw(sf::RenderTarget &target, float fade) const
    {
        const float a = std::clamp(fade, 0.0f, 1.0f) * smooth01(m_time / 0.25f);
        if (a <= 0.0f)
            return;

        const float p = easeOutBack(std::clamp(m_time / 0.45f, 0.0f, 1.0f));
        CardFrame frame;
        frame.scale = 0.94f + 0.06f * p;
        frame.dx = (1.0f - p) * 120.0f;
        const float cs = frame.scale;
        const auto alpha8 = static_cast<std::uint8_t>(std::lround(255.0f * a));

        // Local transform helper: applies `local` around its own pivot, then the card frame
        auto place = [&frame](const sf::Transform &local)
        {
            sf::RenderStates st;
            st.transform = frame.transform() * local;
            return st;
        };
        const sf::RenderStates cardSt = place(sf::Transform());

        // 1) Dim
        sf::RectangleShape dim(sf::Vector2f{kWidth, kHeight});
        dim.setFillColor(faded(sf::Color(40, 6, 50, 128), a));
        target.draw(dim);

        // 2) Card: soft shadow, edge slab, gradient face, border
        const sf::FloatRect cardRect({640.0f, 40.0f}, {580.0f, 640.0f});
        for (int i = 0; i < 7; ++i)
        {
            const float g = 60.0f - 9.0f * static_cast<float>(i);
            UiKit::drawRoundedRect(target, frame.rect(offsetRect(cardRect, 0.0f, 28.0f, g)), (48.0f + g) * cs,
                                   faded(sf::Color(5, 0, 40, 41), a));
        }
        UiKit::drawRoundedRect(target, frame.rect(offsetRect(cardRect, 0.0f, 14.0f, 0.0f)), 48.0f * cs,
                               faded(sf::Color(10, 4, 50, 204), a));
        UiKit::drawRoundedRectGradient(target, frame.rect(cardRect), 48.0f * cs,
                                       faded(sf::Color(60, 40, 150, 205), a), faded(sf::Color(24, 12, 80, 230), a));
        UiKit::drawRoundedRect(target, frame.rect(cardRect), 48.0f * cs, sf::Color::Transparent,
                               faded(sf::Color(255, 255, 255, 140), a), 3.0f * cs);

        // 3) Medal (ribbons first, then the disc)
        const sf::Vector2f medalC{930.0f, 66.0f};
        sf::Transform medalLocal;
        const float medalS = p < 0.6f ? 0.4f + 0.75f * (p / 0.6f) : 1.15f - 0.15f * ((p - 0.6f) / 0.4f);
        medalLocal.translate(medalC).scale({medalS, medalS}).translate(-medalC);
        const sf::RenderStates medalSt = place(medalLocal);

        // Maps the medal's SVG-local coordinates (origin at its top-left) to design space
        auto sv = [](float x, float y)
        { return sf::Vector2f{x + 860.0f, y + 6.0f}; };

        const bool gold = m_newBest;
        const sf::Color discBase = gold ? sf::Color(194, 106, 8) : sf::Color(160, 170, 200);
        const sf::Color discMid = gold ? sf::Color(255, 194, 71) : sf::Color(220, 226, 240);
        const sf::Color discHi = gold ? sf::Color(255, 251, 208) : sf::Color(255, 255, 255);
        const sf::Color discBorder = gold ? sf::Color(168, 90, 6) : sf::Color(120, 128, 160);
        const sf::Color discRing = gold ? sf::Color(255, 243, 176) : sf::Color(240, 244, 252);

        drawCircle(target, medalSt, {medalC.x, medalC.y + 6.0f}, 50.0f, faded(sf::Color(122, 58, 6, 128), a));

        const sf::Color pink(255, 63, 134), pinkEdge(138, 31, 85);
        const sf::Color orange(255, 138, 43), orangeEdge(162, 66, 8);
        drawPoly(target, medalSt, {sv(38, 70), sv(14, 150), sv(42, 138)}, faded(pink, a), faded(pinkEdge, a));
        drawPoly(target, medalSt, {sv(38, 70), sv(42, 138), sv(58, 156), sv(74, 78)}, faded(pink, a),
                 faded(pinkEdge, a));
        drawPoly(target, medalSt, {sv(102, 70), sv(126, 150), sv(98, 138)}, faded(orange, a), faded(orangeEdge, a));
        drawPoly(target, medalSt, {sv(102, 70), sv(98, 138), sv(82, 156), sv(66, 78)}, faded(orange, a),
                 faded(orangeEdge, a));

        drawCircle(target, medalSt, medalC, 50.0f, faded(discBase, a));
        drawCircle(target, medalSt, medalC, 44.0f, faded(discMid, a));
        drawCircle(target, medalSt, {medalC.x - 9.0f, medalC.y - 9.0f}, 22.0f, faded(discHi, a));
        drawCircle(target, medalSt, medalC, 46.0f, sf::Color::Transparent, faded(discBorder, a), 4.0f);
        drawCircle(target, medalSt, medalC, 38.0f, sf::Color::Transparent, faded(discRing, a), 3.0f);

        sf::CircleShape gloss(16.0f);
        gloss.setOrigin({16.0f, 16.0f});
        gloss.setScale({1.0f, 0.44f});
        gloss.setPosition(sv(50, 32));
        gloss.setRotation(sf::degrees(-35.0f));
        gloss.setFillColor(faded(sf::Color(255, 255, 255, 178), a));
        target.draw(gloss, medalSt);

        // Star: brown edge drawn slightly larger underneath
        drawStar(target, medalSt, medalC, 26.0f, 11.5f, faded(sf::Color(184, 107, 12), a));
        drawStar(target, medalSt, medalC, 24.0f, 10.0f, faded(sf::Color(255, 251, 208), a));

        // 4) Title
        UiKit::drawExtrudedText(target, *m_font, "GAME OVER", 76, frame.map({930.0f, 204.0f}), UiKit::redStyle(), cs,
                                0.0f, alpha8);

        // 5) NEW BEST tag (rotated, so it is drawn with its own transform)
        if (m_newBest)
        {
            const float rot = -3.0f + 1.5f * std::sin(m_time * 3.5f);
            const sf::Vector2f tagC{930.0f, 285.0f};
            sf::Transform tagLocal;
            tagLocal.translate(tagC).rotate(sf::degrees(rot)).translate(-tagC);
            const sf::RenderStates tagSt = place(tagLocal);

            const sf::FloatRect tagFace({815.0f, 262.0f}, {230.0f, 46.0f});
            const sf::Color edge(168, 90, 6);
            // soft drop shadow
            drawRoundedPoly(target, tagSt, offsetRect(tagFace, 0.0f, 12.0f, 0.0f), 16.0f, faded(sf::Color(5, 0, 50, 110), a),
                            faded(sf::Color(5, 0, 50, 110), a));
            // 3px border with a 6px bottom edge
            const sf::FloatRect tagOuter({tagFace.position.x - 3.0f, tagFace.position.y - 3.0f},
                                         {tagFace.size.x + 6.0f, tagFace.size.y + 9.0f});
            drawRoundedPoly(target, tagSt, tagOuter, 19.0f, faded(edge, a), faded(edge, a));
            drawRoundedPoly(target, tagSt, tagFace, 16.0f, faded(sf::Color(255, 226, 122), a),
                            faded(sf::Color(255, 176, 32), a));

            const sf::Color ink(90, 42, 0);
            sf::Text label = makeText(*m_font, "NEW BEST!", 24, 2.0f, faded(ink, a));
            const float contentW = 22.0f + 10.0f + textWidth(label);
            const float startX = 930.0f - contentW * 0.5f;
            drawStar(target, tagSt, {startX + 11.0f, 285.0f}, 10.0f, 4.5f, faded(ink, a));
            placeText(target, tagSt, label, startX + 32.0f, 285.0f);
        }

        // 6) Score count-up with a pop whenever the shown number changes
        const float t01 = std::clamp((m_time - 0.25f) / 0.7f, 0.0f, 1.0f);
        const float raw = static_cast<float>(m_score) * t01;
        const int shown = static_cast<int>(std::lround(raw));
        float pop = 1.0f;
        if (shown > 0 && m_score > 0)
        {
            // time since the last change (raw crossed shown - 0.5), converted from score units to seconds
            const float sinceUnits = raw - (static_cast<float>(shown) - 0.5f);
            const float secs = sinceUnits * 0.7f / static_cast<float>(m_score);
            pop = 1.0f + 0.2f * (1.0f - std::clamp(secs / 0.12f, 0.0f, 1.0f));
        }
        UiKit::drawExtrudedText(target, *m_font, std::to_string(shown), 152, frame.map({930.0f, 388.0f}),
                                UiKit::orangeStyle(), cs * pop, 0.0f, alpha8);

        // 7) Chip row: previous best (+ green delta on a new best)
        const sf::Color prevLabelColor(207, 216, 255), prevNumColor(255, 243, 176);
        sf::Text prevLabel = makeText(*m_font, "PREVIOUS BEST", 14, 2.0f, faded(prevLabelColor, a));
        sf::Text prevNum = makeText(*m_font, std::to_string(m_previousBest), 26, 0.0f, faded(prevNumColor, a));
        const float prevW = 20.0f + textWidth(prevLabel) + 10.0f + textWidth(prevNum) + 20.0f;

        const int delta = m_score - m_previousBest;
        sf::Text deltaText = makeText(*m_font, "+" + std::to_string(delta), 26, 0.0f, faded(sf::Color(7, 61, 26), a));
        const float greenW = 20.0f + textWidth(deltaText) + 20.0f;

        const float gap = 14.0f;
        const float total = prevW + (m_newBest ? gap + greenW : 0.0f);
        const float x0 = 930.0f - total * 0.5f;
        const float chipTop = 496.0f;
        const float chipH = 46.0f;

        UiKit::drawRoundedRect(target, frame.rect({{x0, chipTop}, {prevW, chipH}}), 23.0f * cs,
                               faded(sf::Color(14, 8, 60, 180), a), faded(sf::Color(255, 255, 255, 90), a),
                               2.0f * cs);
        placeText(target, cardSt, prevLabel, x0 + 20.0f, 519.0f);
        placeText(target, cardSt, prevNum, x0 + 20.0f + textWidth(prevLabel) + 10.0f, 519.0f);

        if (m_newBest)
        {
            const float gx = x0 + prevW + gap;
            UiKit::drawRoundedRect(target, frame.rect({{gx, chipTop + 4.0f}, {greenW, chipH}}), 23.0f * cs,
                                   faded(sf::Color(14, 122, 53), a));
            UiKit::drawRoundedRectGradient(target, frame.rect({{gx, chipTop}, {greenW, chipH}}), 23.0f * cs,
                                           faded(sf::Color(140, 255, 107), a), faded(sf::Color(47, 209, 90), a));
            UiKit::drawRoundedRect(target, frame.rect({{gx, chipTop}, {greenW, chipH}}), 23.0f * cs,
                                   sf::Color::Transparent, faded(sf::Color(14, 122, 53), a), 2.0f * cs);
            placeText(target, cardSt, deltaText, gx + 20.0f, 519.0f);
        }

        // 8) Buttons (copied so the card transform can be applied without changing their state)
        for (UiKit::Button3D button : m_buttons)
        {
            button.setRect(frame.rect(button.rect()));
            button.draw(target);
        }

        // 9) Hint
        UiKit::drawLabel(target, *m_font, "Press SPACE to play again", 17, frame.map({930.0f, 664.0f}),
                         faded(sf::Color(214, 224, 255), a), false, 1.0f);
    }
} // namespace Flappy
