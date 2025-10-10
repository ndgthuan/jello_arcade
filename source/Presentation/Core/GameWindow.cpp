#include "GameWindow.h"
#include "GameCoordinator.h"
#include "../UI/UIServices.h"
#include <iostream>

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

    // Create window with only close button
    m_window.create(desktop, "Jello", sf::Style::Close);

    // Note: Window icon will be set after ResourceService is injected
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

void GameWindow::processEvents()
{
    // Ensure window is focused before processing events
    ensureFocus();

    // Process all pending window events - now using interface directly
    m_gameCoordinator->getInputHandler().processEvents(m_window);

    // Check for fullscreen toggle key press
    if (m_gameCoordinator->getInputHandler().isKeyJustPressed("fullscreen_toggle"))
    {
        toggleFullscreen();
    }
}

void GameWindow::display()
{
    // Calculate delta time for smooth animations
    static sf::Clock frameClock;
    float deltaTime = frameClock.restart().asSeconds();

    // Update game systems
    m_gameCoordinator->update(deltaTime);

    // Render everything
    m_gameCoordinator->render(m_window);

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

    // Set window icon now that we have resource service
    setWindowIcon();
}

void GameWindow::toggleFullscreen()
{
    // Get desktop video mode for fullscreen window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Store current window position and size for restoration
    sf::Vector2i windowPosition = m_window.getPosition();
    sf::Vector2u windowSize = m_window.getSize();

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
        m_window.create(desktop, "Jello", sf::Style::Close);

        // Restore window position and size
        m_window.setPosition(windowPosition);
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
