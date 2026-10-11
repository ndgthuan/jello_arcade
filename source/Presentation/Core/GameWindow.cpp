#include "GameWindow.h"
#include "GameCoordinator.h"
#include "../UI/UIServices.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include "../../Data/Logging/Logger.h"
#include "../../Data/Resources/Audio.h"
#include "../../Data/Profile/UserData.h"
#include "../Game/GameAudio.h"

namespace
{
    // Fade timings in seconds
    constexpr float kFadeOutTime = 0.2f;
    constexpr float kFadeInTime = 0.25f;
    // Period of the FPS log line in seconds
    constexpr float kLogPeriod = 10.f;
}

GameWindow::GameWindow() : m_isFullscreen(false)
{
    // Initialize window
    initializeWindow();

    // Create services for dependency injection
    auto uiConfigService = std::make_shared<UIConfigurationService>();
    auto gameConfigService = std::make_shared<GameConfigurationService>();
    auto resourceService = std::make_shared<ResourceService>();

    // Create game coordinator with dependency injection
    m_gameCoordinator = std::make_unique<GameCoordinator>(
        uiConfigService,
        gameConfigService,
        resourceService);
}

GameWindow::~GameWindow()
{
    // All resources are managed by smart pointers, no manual cleanup needed
}

void GameWindow::initializeWindow()
{
    // Get desktop video mode for fullscreen window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Windowed mode: 90% of the desktop so the title bar and taskbar fit on screen
    m_window.create(windowedMode(desktop), "Jello", sf::Style::Close);
    m_window.setVerticalSyncEnabled(true); // without it the menu runs at 1500+ FPS and burns the GPU
    centerWindow(desktop);

    // Request focus once on the first frame
    m_needsFocus = true;

    // Note: Window icon will be set after ResourceService is injected
}

sf::VideoMode GameWindow::windowedMode(const sf::VideoMode &desktop)
{
    return sf::VideoMode({desktop.size.x * 9 / 10, desktop.size.y * 9 / 10});
}

void GameWindow::centerWindow(const sf::VideoMode &desktop)
{
    sf::Vector2u size = m_window.getSize();
    m_window.setPosition({static_cast<int>(desktop.size.x - size.x) / 2, static_cast<int>(desktop.size.y - size.y) / 2 - 20});
}

void GameWindow::setWindowIcon()
{
    if (!m_resourceService)
    {
        // Fallback to creating ResourceService if not injected
        m_resourceService = std::make_shared<ResourceService>();
    }

    // Get icon texture from resource service and set as window icon
    const sf::Texture &iconTex = m_resourceService->getTexture("TitleLogo");

    sf::Image icon = iconTex.copyToImage();
    m_window.setIcon(icon);
}

void GameWindow::processGameEvents()
{
    while (auto event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        else if (const auto *key = event->getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::F11)
        {
            toggleFullscreen();
        }
        else
        {
            m_current->handleEvent(*event, m_window);
        }
    }
}

void GameWindow::processPanelEvents()
{
    while (auto event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        else if (const auto *key = event->getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::F11)
        {
            toggleFullscreen();
            return; // the window was recreated
        }
        else
        {
            m_panels->handleEvent(*event, m_window);
        }
    }

    if (!m_panels->isOpen())
    {
        m_waitEscRelease = true;
    }

    if (m_panels->consumeChanged())
    {
        applyAudioSettings();
    }

    // A game was picked in the game chooser: fade out, then open it at full black
    const int picked = m_panels->consumeLaunchGame();
    if (picked >= 0 && picked < static_cast<int>(m_games.size()))
    {
        startFade([this, picked]()
                  {
            m_panels->close();
            m_current = m_games[static_cast<std::size_t>(picked)].get();
            m_current->start();
            m_inGame = true; });
        return;
    }
    if (m_panels->consumeFullscreenRequest())
    {
        toggleFullscreen();
    }
}

void GameWindow::applyAudioSettings()
{
    // The menu click follows the effects volume and the mute switch
    const Settings &settings = UserData::instance().settings();
    AudioManager::getInstance().setSoundVolume(settings.muted ? 0.0f : static_cast<float>(settings.sfxVolume));
    Flappy::GameAudio::instance().setVolumes(settings.musicVolume, settings.sfxVolume, settings.muted);
}

void GameWindow::processEvents()
{
    // Focus is requested once after the window was (re)created
    ensureFocus();

    // F3 toggles the FPS overlay; polled so it works in the menu, the games and the panels
    const bool f3Down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F3);
    if (f3Down && !m_prevF3)
    {
        m_showFps = !m_showFps;
    }
    m_prevF3 = f3Down;

    // While a panel is open it reads the window events itself
    if (m_panels && m_panels->isOpen())
    {
        processPanelEvents();
        return;
    }

    // While a game is open it reads the window events itself
    if (m_inGame && m_current)
    {
        processGameEvents();
        return;
    }

    // Process all pending window events - now using interface directly
    m_gameCoordinator->getInputHandler().processEvents(m_window);

    // Check for fullscreen toggle key press
    if (m_gameCoordinator->getInputHandler().isKeyJustPressed("fullscreen_toggle"))
    {
        toggleFullscreen();
    }

    // Check for exit key press (not while the Esc that closed a panel or left the game is still held)
    if (m_waitEscRelease && !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
    {
        m_waitEscRelease = false;
    }
    if (!m_waitEscRelease && m_gameCoordinator->getInputHandler().isKeyJustPressed("exit"))
    {
        m_window.close();
    }
}

void GameWindow::display()
{
    // Calculate delta time for smooth animations
    static sf::Clock frameClock;
    float deltaTime = frameClock.restart().asSeconds();

    updateFrameStats(deltaTime);
    updateFade(deltaTime);

    if (m_inGame && m_current)
    {
        m_current->update(deltaTime);
        m_current->draw(m_window);
    }
    else
    {
        // Update game systems
        m_gameCoordinator->update(deltaTime);
        m_mascot.update(deltaTime, sf::Vector2f(sf::Mouse::getPosition(m_window)), m_window.getSize());
        if (m_panels && m_panels->isOpen())
        {
            m_panels->update(deltaTime);
        }

        // Render everything
        m_gameCoordinator->render(m_window);
        m_mascot.draw(m_window, m_window.getSize());
        if (m_panels && m_panels->isOpen())
        {
            m_panels->draw(m_window);
        }
    }

    // Fade and FPS overlay are drawn last, in pixel coordinates
    m_window.setView(m_window.getDefaultView());
    drawFade();
    drawFpsOverlay();

    // Display everything to the screen
    m_window.display();
}

bool GameWindow::isOpen() const
{
    return m_window.isOpen();
}

sf::RenderWindow &GameWindow::getWindow()
{
    return m_window;
}

void GameWindow::setResourceService(std::shared_ptr<IResourceService> resourceService)
{
    m_resourceService = resourceService;

    // Initialize game coordinator now that we have resource service
    m_gameCoordinator->initialize(m_window);

    // Setup window close callback
    m_gameCoordinator->setWindowCloseCallback([this]()
                                              { m_window.close(); });

    // Setup exit button callback
    m_gameCoordinator->setExitCallback([this]()
                                       { m_window.close(); });

    // Create the game and let the menu's Play icon open it
    const sf::Font &gameFont = m_resourceService->getFont("main");
    m_games.clear();
    m_games.push_back(std::make_unique<Flappy::BounceGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::SmashGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::BlasterGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::SlitherGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::StackGame>(gameFont));
    for (auto &game : m_games)
    {
        game->setExitToMenuCallback([this]()
                                    {
            // Fade out, then return to the menu at full black
            startFade([this]()
                      {
                m_inGame = false;
                m_waitEscRelease = true;
                Flappy::GameAudio::instance().startMusic(5); }); });
    }
    m_panels = std::make_unique<Panels::PanelManager>(m_resourceService->getFont("main"));
    connectIconCallback();
    applyAudioSettings();
    Flappy::GameAudio::instance().startMusic(5); // menu music

    // Set window icon now that we have resource service
    setWindowIcon();
}

void GameWindow::connectIconCallback()
{
    m_gameCoordinator->setIconCallback([this](int icon)
                                       {
        m_mascot.pulse();
        switch (icon)
        {
        case 0: // Play: choose a game
            m_panels->open(Panels::PanelId::Games);
            break;
        case 1:
            m_panels->open(Panels::PanelId::Settings);
            break;
        case 2:
            m_panels->open(Panels::PanelId::Credits);
            break;
        case 3:
            m_panels->open(Panels::PanelId::Help);
            break;
        case 4:
            m_panels->open(Panels::PanelId::Accounts);
            break;
        default:
            break;
        } });
}

void GameWindow::toggleFullscreen()
{
    // Get desktop video mode for fullscreen window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Toggle window state
    m_isFullscreen = !m_isFullscreen;

    if (m_isFullscreen)
    {
        // Switch to fullscreen mode
        m_window.create(desktop, "Jello", sf::Style::None);
    }
    else
    {
        // Switch to windowed mode - only close button
        m_window.create(windowedMode(desktop), "Jello", sf::Style::Close);
        centerWindow(desktop);
    }

    m_window.setVerticalSyncEnabled(true);
    // Restore view and let ensureFocus() request focus once on the next frame
    m_window.setView(m_window.getDefaultView());
    m_needsFocus = true;

    // Wait a moment for the window to be fully created
    sf::sleep(sf::milliseconds(100));

    // Reinitialize game coordinator after window recreation
    m_gameCoordinator->initialize(m_window);

    // Re-setup callbacks after reinitialization
    m_gameCoordinator->setWindowCloseCallback([this]()
                                              { m_window.close(); });
    m_gameCoordinator->setExitCallback([this]()
                                       { m_window.close(); });
    connectIconCallback();

    // Reapply window icon after recreation
    setWindowIcon();
}

void GameWindow::ensureFocus()
{
    // Focus is requested only once after the window was created or re-created, never every frame
    if (!m_needsFocus)
    {
        return;
    }
    m_needsFocus = false;
    m_window.requestFocus();

// Additional focus handling for Windows
#ifdef _WIN32
    sf::sleep(sf::milliseconds(25));
    m_window.requestFocus();
#endif
}

void GameWindow::startFade(std::function<void()> action)
{
    // Ignore new switch requests while a fade is running
    if (m_fade != FadeState::None)
    {
        return;
    }
    m_fadeAction = std::move(action);
    m_fade = FadeState::Out;
    m_fadeTimer = 0.f;
}

void GameWindow::updateFade(float dt)
{
    if (m_fade == FadeState::None)
    {
        return;
    }

    m_fadeTimer += dt;
    if (m_fade == FadeState::Out && m_fadeTimer >= kFadeOutTime)
    {
        // Screen is fully black: switch now, then fade back in
        m_fade = FadeState::In;
        m_fadeTimer = 0.f;
        std::function<void()> action = std::move(m_fadeAction);
        m_fadeAction = nullptr;
        if (action)
        {
            action();
        }
    }
    else if (m_fade == FadeState::In && m_fadeTimer >= kFadeInTime)
    {
        m_fade = FadeState::None;
        m_fadeTimer = 0.f;
    }
}

void GameWindow::drawFade()
{
    if (m_fade == FadeState::None)
    {
        return;
    }

    // 0 -> 1 while fading out, 1 -> 0 while fading in
    float alpha = (m_fade == FadeState::Out) ? m_fadeTimer / kFadeOutTime : 1.f - m_fadeTimer / kFadeInTime;
    alpha = std::clamp(alpha, 0.f, 1.f);

    sf::RectangleShape shade(m_window.getDefaultView().getSize());
    shade.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha * 255.f)));
    m_window.draw(shade);
}

void GameWindow::updateFrameStats(float dt)
{
    if (dt <= 0.f)
    {
        return;
    }

    // Smoothed frame time for the overlay: 10 % new sample each frame
    m_avgFrameTime = (m_avgFrameTime <= 0.f) ? dt : m_avgFrameTime * 0.9f + dt * 0.1f;

    // Average and lowest instantaneous FPS for the 10 s log line
    const float fps = 1.f / dt;
    if (m_logFrames == 0 || fps < m_logMinFps)
    {
        m_logMinFps = fps;
    }
    m_logFrames++;
    m_logTime += dt;

    if (m_logTime >= kLogPeriod)
    {
        char line[128];
        std::snprintf(line, sizeof(line), "FPS avg %.1f min %.1f over 10 s", m_logFrames / m_logTime, m_logMinFps);
        Logger::getInstance().info(line);
        m_logFrames = 0;
        m_logTime = 0.f;
        m_logMinFps = 0.f;
    }
}

void GameWindow::drawFpsOverlay()
{
    if (!m_showFps || !m_resourceService || m_avgFrameTime <= 0.f)
    {
        return;
    }

    char label[64];
    std::snprintf(label, sizeof(label), "FPS %d  %.1f ms",
                  static_cast<int>(1.f / m_avgFrameTime + 0.5f), m_avgFrameTime * 1000.f);

    const sf::Font &font = m_resourceService->getFont("main");
    sf::Text shadow(font, label, 18);
    shadow.setFillColor(sf::Color(0, 0, 0));
    sf::Text text(font, label, 18);
    text.setFillColor(sf::Color::White);

    // Right-aligned in the top-right corner, 12 px from the edges
    const sf::FloatRect bounds = text.getLocalBounds();
    const float x = m_window.getDefaultView().getSize().x - 12.f - (bounds.position.x + bounds.size.x);
    const float y = 12.f - bounds.position.y;
    text.setPosition({x, y});
    shadow.setPosition({x + 2.f, y + 2.f});

    m_window.draw(shadow);
    m_window.draw(text);
}
