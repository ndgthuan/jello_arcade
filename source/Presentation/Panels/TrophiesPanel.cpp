#include "TrophiesPanel.h"
#include "../../Data/Profile/Achievements.h"
#include "../../Data/Profile/UserData.h"
#include "../Game/UiKit.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace Panels
{
    namespace
    {
        namespace ui = Flappy::UiKit;
        using Buttons = std::vector<ui::Button3D>;

        const sf::FloatRect kCard({130.0f, 40.0f}, {1020.0f, 640.0f});
        const sf::FloatRect kBack({500.0f, 612.0f}, {280.0f, 56.0f});
        // Chip under the title: its dark-blue track is filled with the accent colour in proportion to the progress
        const sf::FloatRect kChip({400.0f, 156.0f}, {480.0f, 28.0f});

        constexpr float kPi = 3.14159265f;
        constexpr int kTileCount = static_cast<int>(Achievements::Count);
        constexpr std::size_t kColumns = 4;
        constexpr std::size_t kWrapLength = 19; // descriptions longer than this are split in two lines
        constexpr float kLift = 4.0f;           // hovered tile moves up by this much

        const sf::Color kNavy(18, 12, 60);
        const sf::Color kGoldBorder(255, 210, 74);
        const sf::Color kMedalHighlight(255, 243, 176);
        const sf::Color kMedalMid(255, 194, 71);
        const sf::Color kMedalEdge(184, 107, 12);
        const sf::Color kLockedMedal(90, 95, 130);
        const sf::Color kMetal(214, 218, 240);

        Flappy::Accent currentAccent()
        {
            return Flappy::Accent::from(Flappy::accentChoice(UserData::instance().settings().accentIndex));
        }

        Buttons makeBackButton(const sf::Font &font, const Flappy::Accent &accent)
        {
            Buttons buttons;
            buttons.emplace_back(font, std::string("Back"), kBack, ui::Button3DStyle::primary(accent), 30u);
            return buttons;
        }

        // Filled disc centered on `center`
        void drawDisc(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color)
        {
            sf::CircleShape disc(radius, 40);
            disc.setOrigin({radius, radius});
            disc.setPosition(center);
            disc.setFillColor(color);
            target.draw(disc);
        }

        // Five-point star drawn as a triangle fan around its center. A star is concave, so ConvexShape
        // would not triangulate it correctly; a fan from the center always does.
        void drawStar(sf::RenderTarget &target, sf::Vector2f center, float outer, float inner, sf::Color color)
        {
            constexpr std::size_t kSpikes = 5;
            constexpr std::size_t kPoints = kSpikes * 2;
            sf::VertexArray fan(sf::PrimitiveType::TriangleFan, kPoints + 2);
            fan[0].position = center;
            fan[0].color = color;
            for (std::size_t i = 0; i <= kPoints; ++i)
            {
                const float radius = (i % 2 == 0) ? outer : inner;
                const float angle = -kPi / 2.0f + static_cast<float>(i) * kPi / static_cast<float>(kSpikes);
                sf::Vertex &v = fan[i + 1];
                v.position = center + sf::Vector2f{radius * std::cos(angle), radius * std::sin(angle)};
                v.color = color;
            }
            target.draw(fan);
        }

        // Small padlock (shackle, body, keyhole) centered on `c`
        void drawPadlock(sf::RenderTarget &target, sf::Vector2f c)
        {
            ui::drawRing(target, {c.x, c.y - 4.0f}, 5.0f, 2.0f, kMetal);
            ui::drawRoundedRect(target, {{c.x - 7.0f, c.y - 1.0f}, {14.0f, 12.0f}}, 3.0f, kMetal);
            drawDisc(target, {c.x, c.y + 4.0f}, 1.7f, kLockedMedal);
        }

        // Round medal: shaded gold with a star when unlocked, flat grey with a padlock when locked
        void drawMedal(sf::RenderTarget &target, sf::Vector2f c, float radius, bool unlocked)
        {
            if (unlocked)
            {
                ui::drawShadedEllipse(target, c, {radius, radius}, kMedalHighlight, kMedalMid, kMedalEdge);
                drawStar(target, c, radius * 0.5f, radius * 0.23f, sf::Color::White);
            }
            else
            {
                drawDisc(target, c, radius, kLockedMedal);
                drawPadlock(target, c);
            }
        }

        // Splits a long text into two lines at the space nearest to the middle
        std::vector<std::string> wrapText(const std::string &text)
        {
            if (text.size() <= kWrapLength)
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
    } // namespace

    TrophiesPanel::TrophiesPanel(const sf::Font &font)
        : m_font(&font), m_buttons(makeBackButton(font, currentAccent()))
    {
    }

    void TrophiesPanel::onOpen()
    {
        m_open = 0.0f;
        m_time = 0.0f;
        m_pressed = -1;
        m_hover = -1;
        m_closeRequested = false;
        m_buttons = makeBackButton(*m_font, accent()); // picks up the current accent colour
    }

    void TrophiesPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }
    }

    void TrophiesPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, kCard, "TROPHIES", m_open);
        if (m_open <= 0.3f)
        {
            return;
        }

        const sf::Font &font = *m_font;
        const Flappy::Accent a = accent();
        const UserData &data = UserData::instance();
        const int profile = data.activeIndex();
        const int unlocked = data.unlockedCount(profile);
        const std::string &playerName = data.active().name;

        // Chip: dark track, accent fill for the unlocked share, "<name>  -  <n> / 20" on top
        ui::drawRoundedRect(target, kChip, 14.0f, sf::Color(214, 224, 255, 210), sf::Color(255, 255, 255, 140),
                            2.0f);
        const float share = static_cast<float>(unlocked) / static_cast<float>(kTileCount);
        if (unlocked > 0)
        {
            const sf::FloatRect fill(kChip.position, {kChip.size.x * share, kChip.size.y});
            ui::drawRoundedRectGradient(target, fill, 14.0f, a.light, a.base);
        }
        const std::string chip = playerName + "  -  " + std::to_string(unlocked) + " / " +
                                 std::to_string(kTileCount);
        ui::drawLabel(target, font, chip, 18, {kChip.position.x + kChip.size.x / 2.0f, kChip.position.y + 14.0f},
                      kNavy, false, 1.0f);

        // 4 x 5 grid of trophies in id order
        for (int i = 0; i < kTileCount; ++i)
        {
            const bool won = data.hasAchievement(profile, i);
            const Achievements::Info &info = Achievements::info(i);

            sf::FloatRect r = tileRect(i);
            if (i == m_hover)
            {
                r.position.y -= kLift;
            }

            if (won)
            {
                ui::drawRoundedRect(target, r, 16.0f, Flappy::withAlpha(sf::Color::White, 38), kGoldBorder, 2.0f);
            }
            else
            {
                ui::drawRoundedRect(target, r, 16.0f, Flappy::withAlpha(sf::Color::White, 16),
                                    Flappy::withAlpha(sf::Color::White, 50), 2.0f);
            }

            drawMedal(target, {r.position.x + 44.0f, r.position.y + 39.0f}, 22.0f, won);

            const sf::Color nameColor = won ? sf::Color::White : sf::Color(150, 160, 200);
            const sf::Color descColor = won ? sf::Color(200, 210, 245) : sf::Color(120, 130, 170);
            const float textX = r.position.x + 80.0f;
            drawText(target, font, info.name, 20, {textX, r.position.y + 24.0f}, nameColor);

            const std::vector<std::string> lines = wrapText(info.description);
            if (lines.size() == 1)
            {
                drawText(target, font, lines[0], 15, {textX, r.position.y + 50.0f}, descColor, false);
            }
            else
            {
                for (std::size_t k = 0; k < lines.size(); ++k)
                {
                    const float y = r.position.y + 46.0f + 18.0f * static_cast<float>(k);
                    drawText(target, font, lines[k], 15, {textX, y}, descColor, false);
                }
            }
        }

        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }
    }

    void TrophiesPanel::handleMouseMove(sf::Vector2f point)
    {
        m_hover = -1;
        for (int i = 0; i < kTileCount; ++i)
        {
            // Include the lifted area so the tile does not flicker at its top edge
            sf::FloatRect hit = tileRect(i);
            hit.position.y -= kLift;
            hit.size.y += kLift;
            if (hit.contains(point))
            {
                m_hover = i;
            }
        }

        for (auto &button : m_buttons)
        {
            button.setHovered(button.contains(point));
        }
    }

    void TrophiesPanel::handleMouseDown(sf::Vector2f point)
    {
        m_pressed = -1;
        for (std::size_t i = 0; i < m_buttons.size(); ++i)
        {
            const bool hit = m_buttons[i].contains(point);
            m_buttons[i].setPressed(hit);
            if (hit)
            {
                m_pressed = static_cast<int>(i);
            }
        }
    }

    void TrophiesPanel::handleMouseUp(sf::Vector2f point)
    {
        // Back fires only when released over the button that was pressed
        bool clicked = false;
        if (m_pressed >= 0 && static_cast<std::size_t>(m_pressed) < m_buttons.size())
        {
            clicked = m_buttons[static_cast<std::size_t>(m_pressed)].contains(point);
        }
        for (auto &button : m_buttons)
        {
            button.setPressed(false);
        }
        m_pressed = -1;

        if (clicked)
        {
            m_closeRequested = true;
        }
    }

    Flappy::Accent TrophiesPanel::accent() const
    {
        return currentAccent();
    }

    sf::FloatRect TrophiesPanel::tileRect(int index) const
    {
        const auto col = static_cast<std::size_t>(index) % kColumns;
        const auto row = static_cast<std::size_t>(index) / kColumns;
        return sf::FloatRect({168.0f + static_cast<float>(col) * 244.0f, 190.0f + static_cast<float>(row) * 84.0f},
                             {232.0f, 78.0f});
    }
} // namespace Panels
