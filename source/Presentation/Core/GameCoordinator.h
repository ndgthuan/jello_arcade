#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <memory>
#include "../Rendering/CameraManager.h"
#include "../Rendering/ParticleSystem.h"
#include "../Input/InputHandler.h"
#include "../Rendering/Renderer.h"
#include "../UI/MenuUI.h"
#include "IGameCoordinator.h"
#include "../UI/UIServices.h"
#include "GameSystemInitializer.h"
#include "GameCallbackManager.h"
#include "../UI/UIManager.h"
#include "WindowManager.h"

/**
 * Game coordinator class that manages all game systems and their interactions
 * Separates game logic coordination from window management
 * Follows Single Responsibility Principle - only handles system coordination
 * Implements IGameCoordinator interface to follow Dependency Inversion Principle
 * Includes GameStateManager functionality for state management
 */
class GameCoordinator : public IGameCoordinator
{
public:
    // Constructor with dependency injection - follows DIP
    GameCoordinator(
        std::shared_ptr<IUIConfigurationService> uiConfigService,
        std::shared_ptr<IGameConfigurationService> gameConfigService,
        std::shared_ptr<IResourceService> resourceService);
    // Destructor - cleans up allocated resources
    ~GameCoordinator();

    // Game system management - implementing IGameCoordinator interface
    void initialize(sf::RenderWindow &window) override;
    void processEvents() override;
    void update(float deltaTime) override;
    void render(sf::RenderWindow &window) override;
    void handleWindowResize(const sf::Vector2f &windowSize) override;

    // System access methods - implementing IGameCoordinator interface
    InputHandler &getInputHandler() override { return m_inputHandler; }
    WindowManager &getWindowManager() override { return m_windowManager; }

    // Window control methods - implementing IGameCoordinator interface
    void setWindowCloseCallback(std::function<void()> callback) override;
    void setExitCallback(std::function<void()> callback) override;

    // Game management methods - implementing IGameCoordinator interface
    void switchToGame(int gameIndex) override;
    int getCurrentGame() const override;
    void pauseGame() override;
    void resumeGame() override;
    bool isGamePaused() const override;

private:
    // System managers - each handles specific functionality
    WindowManager m_windowManager;
    CameraManager m_cameraManager;
    ParticleSystem m_particleSystem;
    InputHandler m_inputHandler;
    Renderer m_renderer;

    // Specialized managers - follows SRP
    UIManager m_uiManager;

    // Services - follows DIP by depending on abstractions
    std::shared_ptr<IUIConfigurationService> m_uiConfigService;
    std::shared_ptr<IGameConfigurationService> m_gameConfigService;
    std::shared_ptr<IResourceService> m_resourceService;

    // Helper classes - follows SRP by delegating specific responsibilities
    std::unique_ptr<GameSystemInitializer> m_systemInitializer;
    std::unique_ptr<GameCallbackManager> m_callbackManager;

    // Game management state - from GameStateManager
    bool m_isInitialized;
    bool m_isPaused;
    float m_deltaTime;
    float m_totalTime;

    // Game management state
    int m_currentGameIndex;
    bool m_isGameRunning;
    bool m_isGamePaused;

    // Private helper methods
    void initializeUIServices();
    void initializeGameState();
    void updateGameState(float deltaTime);
    void resetGameState();
};
