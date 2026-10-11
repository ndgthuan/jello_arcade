#include "SettingsPanel.h"
#include "../../Data/Profile/UserData.h"
#include <algorithm>
#include <cstddef>

namespace Panels
{
    namespace
    {
        // Row rect for the settings list: x 300, width 700, height 56
        sf::FloatRect rowRect(float y)
        {
            return sf::FloatRect(sf::Vector2f{300.0f, y}, sf::Vector2f{700.0f, 56.0f});
        }

        // Back button is an accent-coloured candy button, so it is rebuilt when the accent changes
        Flappy::UiKit::Button3D makeBackButton(const sf::Font &font, const Flappy::Accent &accent)
        {
            return Flappy::UiKit::Button3D(font, "Back", sf::FloatRect(sf::Vector2f{640.0f, 580.0f}, sf::Vector2f{300.0f, 62.0f}),
                                           Flappy::UiKit::Button3DStyle::primary(accent), 30);
        }

        int buttonIndexAt(const std::vector<Flappy::UiKit::Button3D> &buttons, sf::Vector2f point)
        {
            for (std::size_t i = 0; i < buttons.size(); ++i)
            {
                if (buttons[i].contains(point))
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }
    } // namespace

    SettingsPanel::SettingsPanel(const sf::Font &font)
        : m_font(&font),
          m_music(font, "Music volume", rowRect(150.0f), UserData::instance().settings().musicVolume),
          m_sfx(font, "Effects volume", rowRect(218.0f), UserData::instance().settings().sfxVolume),
          m_mute(font, "Mute all sound", rowRect(286.0f), UserData::instance().settings().muted),
          m_color(font, "Jello colour", rowRect(354.0f),
                  {Flappy::accentChoice(0), Flappy::accentChoice(1), Flappy::accentChoice(2), Flappy::accentChoice(3)},
                  UserData::instance().settings().accentIndex),
          m_difficulty(font, "Difficulty", rowRect(422.0f), {"Easy", "Normal", "Hard"},
                       UserData::instance().settings().difficulty),
          m_shake(font, "Screen shake", rowRect(490.0f), UserData::instance().settings().screenShake)
    {
        m_buttons.emplace_back(font, "Fullscreen", sf::FloatRect(sf::Vector2f{300.0f, 580.0f}, sf::Vector2f{300.0f, 62.0f}),
                               Flappy::UiKit::Button3DStyle::secondary(), 28);
        m_buttons.emplace_back(makeBackButton(font, accent()));
        onOpen();
    }

    void SettingsPanel::onOpen()
    {
        m_open = 0.0f;

        const Settings &s = UserData::instance().settings();
        m_music.setValue(s.musicVolume);
        m_sfx.setValue(s.sfxVolume);
        m_mute.setOn(s.muted);
        m_shake.setOn(s.screenShake);
        m_color.setSelected(s.accentIndex);
        m_difficulty.setSelected(s.difficulty);

        // Accent may have changed since the last open
        if (m_buttons.size() == 2)
        {
            m_buttons[1] = makeBackButton(*m_font, accent());
        }

        m_pressed = -1;
        m_closeRequested = false;
    }

    void SettingsPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);
        m_mute.update(dt);
        m_shake.update(dt);
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }
    }

    void SettingsPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, sf::FloatRect(sf::Vector2f{240.0f, 40.0f}, sf::Vector2f{800.0f, 640.0f}), "SETTINGS",
                 m_open);

        if (m_open <= 0.3f)
        {
            return;
        }

        const Flappy::Accent a = accent();

        // Faint dividers between rows
        for (float y : {214.0f, 282.0f, 350.0f, 418.0f, 486.0f})
        {
            sf::RectangleShape line(sf::Vector2f{700.0f, 2.0f});
            line.setPosition(sf::Vector2f{300.0f, y});
            line.setFillColor(sf::Color(255, 255, 255, 40));
            target.draw(line);
        }

        m_music.draw(target, a);
        m_sfx.draw(target, a);
        m_mute.draw(target, a);
        m_shake.draw(target, a);
        m_color.draw(target);
        m_difficulty.draw(target, a);

        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }

        drawText(target, *m_font, "Changes are saved automatically", 18, sf::Vector2f{300.0f, 662.0f},
                 sf::Color(214, 224, 255));
    }

    void SettingsPanel::handleMouseMove(sf::Vector2f point)
    {
        // A dragging slider reports a value change; it is applied live, saved on mouse up
        if (m_music.handleMouseMove(point))
        {
            UserData::instance().settings().musicVolume = m_music.value();
            m_changed = true;
        }
        if (m_sfx.handleMouseMove(point))
        {
            UserData::instance().settings().sfxVolume = m_sfx.value();
            m_changed = true;
        }

        m_mute.handleMouseMove(point);
        m_shake.handleMouseMove(point);
        m_color.handleMouseMove(point);
        m_difficulty.handleMouseMove(point);

        for (auto &button : m_buttons)
        {
            button.setHovered(button.contains(point));
        }
    }

    void SettingsPanel::handleMouseDown(sf::Vector2f point)
    {
        Settings &s = UserData::instance().settings();

        // Writes the setting and flags the change; the disk write happens on mouse up or on discrete changes
        auto commit = [this]()
        {
            UserData::instance().commitSettings();
            m_changed = true;
        };

        if (m_music.handleMouseDown(point))
        {
            s.musicVolume = m_music.value();
            m_changed = true;
        }
        if (m_sfx.handleMouseDown(point))
        {
            s.sfxVolume = m_sfx.value();
            m_changed = true;
        }
        if (m_mute.handleMouseDown(point))
        {
            s.muted = m_mute.on();
            commit();
        }
        if (m_shake.handleMouseDown(point))
        {
            s.screenShake = m_shake.on();
            commit();
        }
        if (m_color.handleMouseDown(point))
        {
            s.accentIndex = m_color.selected();
            if (m_buttons.size() == 2)
            {
                m_buttons[1] = makeBackButton(*m_font, accent());
            }
            commit();
        }
        if (m_difficulty.handleMouseDown(point))
        {
            s.difficulty = m_difficulty.selected();
            commit();
        }

        // Press the button under the cursor
        for (auto &button : m_buttons)
        {
            button.setPressed(false);
        }
        m_pressed = buttonIndexAt(m_buttons, point);
        if (m_pressed >= 0)
        {
            m_buttons[static_cast<std::size_t>(m_pressed)].setPressed(true);
        }
    }

    void SettingsPanel::handleMouseUp(sf::Vector2f point)
    {
        m_music.handleMouseUp();
        m_sfx.handleMouseUp();

        // One disk write after the drag / click ends
        UserData::instance().commitSettings();
        m_changed = true;

        // A button fires only when released over the same button it was pressed on
        const int released = buttonIndexAt(m_buttons, point);
        if (m_pressed >= 0 && released == m_pressed)
        {
            if (m_pressed == 0)
            {
                m_fullscreenRequested = true;
            }
            else if (m_pressed == 1)
            {
                m_closeRequested = true;
            }
        }

        for (auto &button : m_buttons)
        {
            button.setPressed(false);
        }
        m_pressed = -1;
    }

    Flappy::Accent SettingsPanel::accent() const
    {
        return Flappy::Accent::from(Flappy::accentChoice(UserData::instance().settings().accentIndex));
    }
} // namespace Panels
