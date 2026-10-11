#include "GameWindow.h"
#include "GameCoordinator.h"
#include "../UI/UIServices.h"
#include <iostream>
#include "../../Data/Resources/Audio.h"
#include "../../Data/Profile/UserData.h"
#include "../Game/GameAudio.h"

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
    centerWindow(desktop);

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

    // A game was picked in the game chooser
    const int picked = m_panels->consumeLaunchGame();
    if (picked >= 0 && picked < static_cast<int>(m_games.size()))
    {
        m_panels->close();
        m_current = m_games[static_cast<std::size_t>(picked)].get();
        m_current->start();
        m_inGame = true;
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
    // Ensure window is focused before processing events
    ensureFocus();

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

    if (m_inGame && m_current)
    {
        m_current->update(deltaTime);
        m_current->draw(m_window);
        m_window.display();
        return;
    }

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
    m_games.push_back(std::make_unique<Flappy::FlappyGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::SmashGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::BlasterGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::SlitherGame>(gameFont));
    m_games.push_back(std::make_unique<Flappy::StackGame>(gameFont));
    for (auto &game : m_games)
    {
        game->setExitToMenuCallback([this]()
                                    {
            m_inGame = false;
            m_waitEscRelease = true;
            Flappy::GameAudio::instance().startMusic(5); });
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

    // Restore view and ensure window is focused
    m_window.setView(m_window.getDefaultView());

    // Force focus and make sure window is active
    m_window.requestFocus();

    // Wait a moment for window to be fully created and focused
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

    // Force focus again to ensure input events are captured
    m_window.requestFocus();

// Additional focus handling for Windows
#ifdef _WIN32
    // Give extra time for Windows to properly focus the window
    sf::sleep(sf::milliseconds(50));
    m_window.requestFocus();
#endif
}

void GameWindow::ensureFocus()
{
    // Ensure window is focused and active
    if (!m_window.hasFocus())
    {
        m_window.requestFocus();

// Additional focus handling for Windows
#ifdef _WIN32
        sf::sleep(sf::milliseconds(25));
        m_window.requestFocus();
#endif
    }
}
