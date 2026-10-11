#include "PanelManager.h"
#include <algorithm>
#include <cstdint>

namespace Panels
{
    namespace
    {
        // Moves a panel's pending flags into the manager's flags. Both are read so neither is left set.
        void harvest(Panel *panel, bool &fullscreen, bool &changed, int &launch)
        {
            if (!panel)
            {
                return;
            }
            const bool fs = panel->consumeFullscreenRequest();
            const bool ch = panel->consumeChanged();
            const int game = panel->consumeLaunchGame();
            fullscreen = fullscreen || fs;
            changed = changed || ch;
            if (game >= 0)
            {
                launch = game;
            }
        }
    } // namespace

    PanelManager::PanelManager(const sf::Font &font)
        : m_settings(std::make_unique<SettingsPanel>(font)),
          m_credits(std::make_unique<CreditsPanel>(font)),
          m_help(std::make_unique<HelpPanel>(font)),
          m_accounts(std::make_unique<AccountPanel>(font)),
          m_games(std::make_unique<GamePickerPanel>(font)),
          m_trophies(std::make_unique<TrophiesPanel>(font))
    {
        m_view = sf::View(sf::FloatRect({0.0f, 0.0f}, {Flappy::kWidth, Flappy::kHeight}));
    }

    void PanelManager::open(PanelId id)
    {
        Panel *panel = nullptr;
        switch (id)
        {
        case PanelId::Settings:
            panel = m_settings.get();
            break;
        case PanelId::Credits:
            panel = m_credits.get();
            break;
        case PanelId::Help:
            panel = m_help.get();
            break;
        case PanelId::Accounts:
            panel = m_accounts.get();
            break;
        case PanelId::Games:
            panel = m_games.get();
            break;
        case PanelId::Trophies:
            panel = m_trophies.get();
            break;
        }
        if (!panel)
        {
            return;
        }

        // Drop stale requests left over from the panel's previous session
        panel->clearCloseRequest();
        panel->consumeFullscreenRequest();
        panel->consumeChanged();
        panel->consumeLaunchGame();

        m_current = panel;
        m_current->onOpen();
        m_dim = 0.0f;
    }

    void PanelManager::close()
    {
        if (m_current)
        {
            m_current->clearCloseRequest();
        }
        m_current = nullptr;
    }

    void PanelManager::handleEvent(const sf::Event &event, sf::RenderWindow &window)
    {
        if (!m_current)
        {
            return;
        }
        applyViewport(window);

        if (const auto *move = event.getIf<sf::Event::MouseMoved>())
        {
            m_current->handleMouseMove(window.mapPixelToCoords(move->position, m_view));
        }
        else if (const auto *press = event.getIf<sf::Event::MouseButtonPressed>())
        {
            if (press->button == sf::Mouse::Button::Left)
            {
                m_current->handleMouseDown(window.mapPixelToCoords(press->position, m_view));
            }
        }
        else if (const auto *release = event.getIf<sf::Event::MouseButtonReleased>())
        {
            if (release->button == sf::Mouse::Button::Left)
            {
                m_current->handleMouseUp(window.mapPixelToCoords(release->position, m_view));
            }
        }
        else if (const auto *text = event.getIf<sf::Event::TextEntered>())
        {
            m_current->handleText(text->unicode);
        }
        else if (const auto *key = event.getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Escape)
            {
                harvest(m_current, m_fullscreenRequested, m_changed, m_launchGame);
                close();
                return;
            }
            m_current->handleKey(key->code);
        }

        harvest(m_current, m_fullscreenRequested, m_changed, m_launchGame);
        if (m_current)
        {
            const int request = m_current->consumeOpenPanel();
            if (request >= 0)
            {
                open(static_cast<PanelId>(request));
                return;
            }
        }
        if (m_current && m_current->closeRequested())
        {
            close();
        }
    }

    void PanelManager::update(float dt)
    {
        if (!m_current)
        {
            return;
        }
        m_dim = std::min(1.0f, m_dim + dt * 6.0f);
        m_current->update(dt);

        // Grab the flags before a close request can drop the panel
        harvest(m_current, m_fullscreenRequested, m_changed, m_launchGame);
        const int request = m_current->consumeOpenPanel();
        if (request >= 0)
        {
            open(static_cast<PanelId>(request));
            return;
        }
        if (m_current->closeRequested())
        {
            close();
        }
    }

    void PanelManager::draw(sf::RenderWindow &window)
    {
        if (!m_current)
        {
            return;
        }

        // Full-window dim in default view coordinates (window pixels)
        window.setView(window.getDefaultView());
        const sf::Vector2u size = window.getSize();
        sf::RectangleShape dim(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
        const auto alpha = static_cast<std::uint8_t>(175.0f * m_dim);
        dim.setFillColor(sf::Color(14, 6, 60, alpha));
        window.draw(dim);

        // Letterboxed logical view for the panel itself
        applyViewport(window);
        window.setView(m_view);
        m_current->draw(window);
        window.setView(window.getDefaultView());
    }

    bool PanelManager::consumeFullscreenRequest()
    {
        const bool requested = m_fullscreenRequested;
        m_fullscreenRequested = false;
        return requested;
    }

    int PanelManager::consumeLaunchGame()
    {
        const int game = m_launchGame;
        m_launchGame = -1;
        return game;
    }

    bool PanelManager::consumeChanged()
    {
        const bool changed = m_changed;
        m_changed = false;
        return changed;
    }

    void PanelManager::applyViewport(const sf::RenderWindow &window)
    {
        const sf::Vector2u size = window.getSize();
        if (size.x == 0 || size.y == 0)
        {
            return;
        }

        const float winW = static_cast<float>(size.x);
        const float winH = static_cast<float>(size.y);
        const float aspect = Flappy::kWidth / Flappy::kHeight;

        // Largest 16:9 rectangle that fits, centered (letterbox)
        float w = winW;
        float h = winW / aspect;
        if (h > winH)
        {
            h = winH;
            w = winH * aspect;
        }

        const float fw = w / winW;
        const float fh = h / winH;
        m_view.setViewport(sf::FloatRect({(1.0f - fw) * 0.5f, (1.0f - fh) * 0.5f}, {fw, fh}));
    }
} // namespace Panels
