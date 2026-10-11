#include "GameCoordinator.h"
#include "../UI/UIServices.h"
#include "../UI/UIManager.h"

GameCoordinator::GameCoordinator(
    std::shared_ptr<IUIConfigurationService> uiConfigService,
    std::shared_ptr<IGameConfigurationService> gameConfigService,
    std::shared_ptr<IResourceService> resourceService)
    : m_uiConfigService(uiConfigService), m_gameConfigService(gameConfigService), m_resourceService(resourceService),
      m_currentGameIndex(0), m_isGameRunning(false), m_isGamePaused(false),
      m_isInitialized(false), m_isPaused(false), m_deltaTime(0.0f), m_totalTime(0.0f)
{
    // Initialize helper classes
    m_systemInitializer = std::make_unique<GameSystemInitializer>();
    m_callbackManager = std::make_unique<GameCallbackManager>();
}

GameCoordinator::~GameCoordinator()
{
}

void GameCoordinator::initialize(sf::RenderWindow &window)
{
    // Reset input states first to prevent stuck keys after window recreation
    m_inputHandler.resetKeyStates();

    initializeUIServices();
    initializeGameState();

    // Initialize UI manager
    sf::Vector2f windowSize = sf::Vector2f(window.getSize());
    m_uiManager.initialize(windowSize, m_uiConfigService, m_resourceService);

    // Setup exit callback for UI manager
    m_uiManager.setExitCallback([this]()
                                {
                                    // This will be set by GameWindow later
                                });

    // Use system initializer to setup all systems
    m_systemInitializer->initializeSystems(
        window,
        m_cameraManager,
        m_particleSystem,
        m_renderer,
        m_uiManager.getMenuUI(),
        m_uiConfigService,
        m_gameConfigService,
        m_resourceService);

    // Use callback manager to setup all callbacks
    m_callbackManager->setupInputCallbacks(
        m_inputHandler,
        [this](const sf::Vector2f &mousePos, const sf::Vector2f &windowSize)
        {
            // Handle mouse move for camera and UI
            if (m_renderer.isInitialized())
            {
                sf::FloatRect bgBounds = m_renderer.getBounds();
                m_cameraManager.updateCamera(bgBounds, mousePos, windowSize);
            }
            m_uiManager.updateHover(mousePos);
        },
        [this](const sf::Vector2f &clickPos)
        {
            // Handle mouse click for UI interaction
            m_uiManager.handleClick(clickPos);
            m_uiManager.isExitButtonClicked(clickPos);
        },
        [this]() { /* Exit callback will be set later */ });
}

void GameCoordinator::initializeUIServices()
{
    // Initialize UI services
    // This follows Dependency Inversion Principle
}

void GameCoordinator::initializeGameState()
{
    m_isInitialized = true;
    m_isPaused = false;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
}

void GameCoordinator::processEvents()
{
    // This will be called by GameWindow
}

void GameCoordinator::update(float deltaTime)
{
    // Update game state
    updateGameState(deltaTime);

    // Update camera with smooth movement
    m_cameraManager.updateCameraSmooth();

    // Update particle system
    m_particleSystem.update(deltaTime);

    // Update UI
    m_uiManager.update(deltaTime);
}

void GameCoordinator::render(sf::RenderWindow &window)
{
    // Clear window with teal background color
    window.clear(sf::Color(0, 128, 128));

    // Render background and particles with camera view
    m_renderer.setView(window, m_cameraManager.getCamera());
    m_renderer.drawBackground(window);
    m_renderer.drawParticles(window, m_particleSystem);

    // Reset to default view for UI rendering (UI not affected by camera)
    window.setView(window.getDefaultView());

    // Draw UI on default view (not affected by camera movement)
    m_uiManager.draw(window);
}

void GameCoordinator::handleWindowResize(const sf::Vector2f &windowSize)
{
    // Recenter UI elements after window resize
    m_uiManager.handleWindowResize(windowSize);
}

void GameCoordinator::setWindowCloseCallback(std::function<void()> callback)
{
    m_inputHandler.setCloseCallback(callback);
}

void GameCoordinator::setExitCallback(std::function<void()> callback)
{
    m_callbackManager->setExitCallback(callback);
    // Also set the callback for UI manager
    m_uiManager.setExitCallback(callback);
}

void GameCoordinator::setIconCallback(std::function<void(int)> callback)
{
    m_uiManager.getMenuUI().setIconCallback(std::move(callback));
}

// Game management methods implementation
void GameCoordinator::switchToGame(int gameIndex)
{
    if (gameIndex >= 0 && gameIndex < 4)
    { // 4 games: Tetris, Snake, Pong, Flappy
        m_currentGameIndex = gameIndex;
        m_isGameRunning = true;
        m_isGamePaused = false;
        // TODO: Initialize specific game based on index
    }
}

int GameCoordinator::getCurrentGame() const
{
    return m_currentGameIndex;
}

void GameCoordinator::pauseGame()
{
    if (m_isGameRunning)
    {
        m_isGamePaused = true;
        m_isPaused = true;
    }
}

void GameCoordinator::resumeGame()
{
    if (m_isGameRunning)
    {
        m_isGamePaused = false;
        m_isPaused = false;
    }
}

bool GameCoordinator::isGamePaused() const
{
    return m_isGamePaused;
}

void GameCoordinator::updateGameState(float deltaTime)
{
    m_deltaTime = deltaTime;
    m_totalTime += deltaTime;
}

void GameCoordinator::resetGameState()
{
    m_isInitialized = false;
    m_isPaused = false;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
}
