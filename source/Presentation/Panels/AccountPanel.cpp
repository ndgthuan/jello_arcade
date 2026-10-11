#include "AccountPanel.h"
#include "../../Data/Profile/Achievements.h"
#include "../../Data/Profile/UserData.h"
#include "../Game/UiKit.h"
#include "PanelManager.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Panels
{
    namespace
    {
        namespace ui = Flappy::UiKit;

        const sf::Color kNavy(18, 12, 60);
        const sf::Color kMuted(214, 224, 255);
        const sf::Color kGold(255, 243, 176);
        const sf::Color kRed(255, 120, 120);
        const sf::Color kPink(255, 90, 143);

        int profileCount()
        {
            return static_cast<int>(UserData::instance().profiles().size());
        }

        // Thin bar centered on `center`, rotated by angleDeg (SFML convention: clockwise).
        void drawBar(sf::RenderTarget &target, sf::Vector2f center, float length, float thickness, float angleDeg,
                     sf::Color color)
        {
            sf::RectangleShape bar(sf::Vector2f{length, thickness});
            bar.setOrigin(sf::Vector2f{length / 2.0f, thickness / 2.0f});
            bar.setPosition(center);
            bar.setRotation(sf::degrees(angleDeg));
            bar.setFillColor(color);
            target.draw(bar);
        }

        // Small filled circle with a white check mark, centered on `center`.
        void drawCheckBadge(sf::RenderTarget &target, sf::Vector2f center, sf::Color fill)
        {
            sf::CircleShape circle(12.0f);
            circle.setOrigin(sf::Vector2f{12.0f, 12.0f});
            circle.setPosition(center);
            circle.setFillColor(fill);
            target.draw(circle);
            drawBar(target, sf::Vector2f{center.x - 4.0f, center.y + 2.0f}, 5.7f, 3.0f, 45.0f, sf::Color::White);
            drawBar(target, sf::Vector2f{center.x + 2.0f, center.y}, 11.3f, 3.0f, -45.0f, sf::Color::White);
        }

        // Text whose right edge sits at rightPos.x (rightPos.y is the vertical center).
        void drawTextRight(sf::RenderTarget &target, const sf::Font &font, const std::string &text, unsigned int size,
                           sf::Vector2f rightPos, sf::Color color)
        {
            const sf::Text measure(font, text, size);
            const float width = measure.getLocalBounds().size.x;
            drawText(target, font, text, size, sf::Vector2f{rightPos.x - width, rightPos.y}, color);
        }
    } // namespace

    AccountPanel::AccountPanel(const sf::Font &font)
        : m_font(&font),
          m_nameField(font, "Player name", sf::FloatRect({700.0f, 500.0f}, {250.0f, 56.0f}), UserData::kMaxNameLength)
    {
        m_buttons.emplace_back(font, "Add", sf::FloatRect({962.0f, 496.0f}, {118.0f, 60.0f}),
                               Flappy::UiKit::Button3DStyle::primary(accent()), 26);
        m_buttons.emplace_back(font, "Back", sf::FloatRect({440.0f, 596.0f}, {280.0f, 64.0f}),
                               Flappy::UiKit::Button3DStyle::secondary(), 28);
        m_buttons.emplace_back(font, "Trophies", sf::FloatRect({820.0f, 596.0f}, {280.0f, 64.0f}),
                               Flappy::UiKit::Button3DStyle::secondary(), 28);
        onOpen();
    }

    void AccountPanel::onOpen()
    {
        m_open = 0.0f;
        m_closeRequested = false;
        m_error.clear();
        m_errorTimer = 0.0f;
        m_nameField.clear();
        m_nameField.setFocused(false);
        m_pressed = -1;
        m_hoverRow = -1;
        m_hoverDelete = -1;
        for (auto &button : m_buttons)
        {
            button.setPressed(false);
            button.setHovered(false);
        }
    }

    void AccountPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);
        m_nameField.update(dt);
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }

        if (m_errorTimer > 0.0f)
        {
            m_errorTimer -= dt;
            if (m_errorTimer <= 0.0f)
            {
                m_errorTimer = 0.0f;
                m_error.clear();
            }
        }
    }

    void AccountPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, sf::FloatRect({160.0f, 40.0f}, {960.0f, 640.0f}), "PLAYERS", m_open);
        if (m_open <= 0.3f)
        {
            return;
        }

        const sf::Font &font = *m_font;
        const UserData &data = UserData::instance();
        const auto &profiles = data.profiles();
        const Flappy::Accent acc = accent();
        const int count = static_cast<int>(profiles.size());
        const int active = data.activeIndex();
        const bool canDelete = count > 1;

        // Left column: one row per profile
        for (int i = 0; i < count; ++i)
        {
            const auto idx = static_cast<std::size_t>(i);
            const sf::FloatRect rect = rowRect(i);
            const bool isActive = i == active;
            const bool hovered = i == m_hoverRow;
            const float cy = rect.position.y + rect.size.y / 2.0f;

            if (isActive)
            {
                const sf::Color top = hovered ? Flappy::mix(acc.light, sf::Color::White, 0.15f) : acc.light;
                const sf::Color bottom = hovered ? Flappy::mix(acc.base, sf::Color::White, 0.1f) : acc.base;
                ui::drawRoundedRectGradient(target, rect, 20.0f, top, bottom);
                ui::drawRoundedRect(target, rect, 20.0f, sf::Color::Transparent, acc.dark, 3.0f);
            }
            else
            {
                const sf::Color fill = hovered ? sf::Color(28, 18, 96, 210) : sf::Color(14, 8, 60, 190);
                ui::drawRoundedRect(target, rect, 20.0f, fill, sf::Color(255, 255, 255, 90), 2.0f);
            }

            const sf::Color textColor = isActive ? kNavy : sf::Color::White;
            if (isActive)
            {
                drawCheckBadge(target, sf::Vector2f{rect.position.x + 34.0f, cy}, acc.dark);
            }
            const float nameX = rect.position.x + (isActive ? 56.0f : 22.0f);
            Panels::drawText(target, font, profiles[idx].name, 28, sf::Vector2f{nameX, cy}, textColor);

            // Delete button (dimmed when it cannot be used)
            const sf::FloatRect del = deleteRect(i);
            const sf::Vector2f delCenter{del.position.x + 20.0f, del.position.y + 20.0f};
            const bool delHover = canDelete && i == m_hoverDelete;
            const auto alpha = static_cast<std::uint8_t>(canDelete ? 255 : 80);
            sf::Color pink = delHover ? Flappy::mix(kPink, sf::Color::White, 0.25f) : kPink;
            pink.a = alpha;
            sf::CircleShape circle(20.0f);
            circle.setOrigin(sf::Vector2f{20.0f, 20.0f});
            circle.setPosition(delCenter);
            circle.setFillColor(pink);
            target.draw(circle);
            const sf::Color cross = Flappy::withAlpha(sf::Color::White, alpha);
            drawBar(target, delCenter, 16.0f, 4.0f, 45.0f, cross);
            drawBar(target, delCenter, 16.0f, 4.0f, -45.0f, cross);

            // Games played, right-aligned before the delete button
            drawTextRight(target, font, std::to_string(profiles[idx].gamesPlayed) + " games", 20, sf::Vector2f{del.position.x - 12.0f, cy},
                          isActive ? kNavy : kMuted);
        }

        // Counter below the list
        const std::string counter =
            std::to_string(count) + " / " + std::to_string(UserData::kMaxProfiles) + " players";
        Panels::drawText(target, font, counter, 20,
                         sf::Vector2f{200.0f, 176.0f + static_cast<float>(count) * 66.0f + 4.0f}, kMuted);

        // Right column: stats card for the active profile
        ui::drawRoundedRect(target, sf::FloatRect({700.0f, 176.0f}, {380.0f, 296.0f}), 20.0f,
                            Flappy::withAlpha(sf::Color::White, 28), Flappy::withAlpha(sf::Color::White, 90), 2.0f);

        const Profile &player = data.active();
        ui::drawExtrudedText(target, font, player.name, 44, sf::Vector2f{890.0f, 232.0f}, ui::orangeStyle());

        // Trophies of the active profile, at the top of the card (above the name)
        const int trophies = data.unlockedCount(active);
        ui::drawLabel(target, font, "Trophies " + std::to_string(trophies) + " / " +
                                        std::to_string(static_cast<int>(Achievements::Count)),
                      20, sf::Vector2f{890.0f, 192.0f}, kGold, true, 1.0f);

        // Best scores are for the difficulty chosen in Settings
        const std::size_t level = static_cast<std::size_t>(std::clamp(data.settings().difficulty, 0, 2));
        const std::string labels[6] = {"Games played", "Total score", "Best - Bounce", "Best - Smash", "Best - Blaster", "Best - Slither"};
        const std::string values[6] = {
            std::to_string(player.gamesPlayed), std::to_string(player.totalScore), std::to_string(player.best[0][level]),
            std::to_string(player.best[1][level]), std::to_string(player.best[2][level]), std::to_string(player.best[3][level])};
        for (std::size_t k = 0; k < 6; ++k)
        {
            const float y = 292.0f + static_cast<float>(k) * 30.0f;
            Panels::drawText(target, font, labels[k], 22, sf::Vector2f{722.0f, y}, sf::Color::White);
            drawTextRight(target, font, values[k], 24, sf::Vector2f{1058.0f, y}, kGold);
        }

        // New player form
        m_nameField.draw(target, acc);
        Panels::drawText(target, font, "New player", 20, sf::Vector2f{702.0f, 484.0f}, kMuted);
        if (m_errorTimer > 0.0f)
        {
            Panels::drawText(target, font, m_error, 20, sf::Vector2f{700.0f, 574.0f}, kRed);
        }

        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }
    }

    void AccountPanel::handleMouseMove(sf::Vector2f point)
    {
        m_hoverRow = -1;
        m_hoverDelete = -1;
        for (int i = 0; i < profileCount(); ++i)
        {
            if (deleteRect(i).contains(point))
            {
                m_hoverDelete = i;
            }
            else if (rowRect(i).contains(point))
            {
                m_hoverRow = i;
            }
        }

        for (auto &button : m_buttons)
        {
            button.setHovered(button.contains(point));
        }
    }

    void AccountPanel::handleMouseDown(sf::Vector2f point)
    {
        m_nameField.handleMouseDown(point);

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

    void AccountPanel::handleMouseUp(sf::Vector2f point)
    {
        const int pressed = m_pressed;
        m_pressed = -1;
        for (auto &button : m_buttons)
        {
            button.setPressed(false);
        }

        UserData &data = UserData::instance();
        const int count = profileCount();

        // Delete squares first (they sit inside the row rectangles)
        for (int i = 0; i < count; ++i)
        {
            if (count > 1 && deleteRect(i).contains(point))
            {
                data.deleteProfile(i);
                m_changed = true;
                return;
            }
        }

        // Rows make their profile active
        for (int i = 0; i < count; ++i)
        {
            if (rowRect(i).contains(point))
            {
                data.setActive(i);
                m_changed = true;
                return;
            }
        }

        // Buttons fire only when released over the button that was pressed
        if (pressed >= 0 && pressed < static_cast<int>(m_buttons.size()) &&
            m_buttons[static_cast<std::size_t>(pressed)].contains(point))
        {
            if (pressed == 0)
            {
                submitName();
            }
            else if (pressed == 2)
            {
                m_openPanel = static_cast<int>(PanelId::Trophies);
            }
            else
            {
                m_closeRequested = true;
            }
        }
    }

    void AccountPanel::handleText(char32_t unicode)
    {
        m_nameField.handleText(unicode);
    }

    void AccountPanel::handleKey(sf::Keyboard::Key key)
    {
        m_nameField.handleKey(key);
        if (m_nameField.consumeSubmit())
        {
            submitName();
        }
    }

    Flappy::Accent AccountPanel::accent() const
    {
        return Flappy::Accent::from(Flappy::accentChoice(UserData::instance().settings().accentIndex));
    }

    sf::FloatRect AccountPanel::rowRect(int index) const
    {
        return sf::FloatRect({200.0f, 176.0f + static_cast<float>(index) * 66.0f}, {440.0f, 60.0f});
    }

    sf::FloatRect AccountPanel::deleteRect(int index) const
    {
        const sf::FloatRect row = rowRect(index);
        return sf::FloatRect({row.position.x + row.size.x - 52.0f, row.position.y + 11.0f}, {40.0f, 40.0f});
    }

    void AccountPanel::submitName()
    {
        std::string error;
        if (UserData::instance().addProfile(m_nameField.text(), &error))
        {
            m_nameField.clear();
            m_error.clear();
            m_errorTimer = 0.0f;
            m_changed = true;
        }
        else
        {
            m_error = error;
            m_errorTimer = 3.0f;
        }
    }
} // namespace Panels
