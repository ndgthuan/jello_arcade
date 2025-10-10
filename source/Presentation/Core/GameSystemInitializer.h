#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

// Forward declarations
class CameraManager;
class ParticleSystem;
class Renderer;
class MenuUI;
class IUIConfigurationService;
class IGameConfigurationService;
class IResourceService;

/**
 * Game System Initializer class
 * Follows Single Responsibility Principle - only handles system initialization
 * Separates initialization logic from GameCoordinator
 */
class GameSystemInitializer
{
public:
    GameSystemInitializer();
    ~GameSystemInitializer() = default;

    // Initialize all game systems
    void initializeSystems(
        sf::RenderWindow &window,
        CameraManager &cameraManager,
        ParticleSystem &particleSystem,
        Renderer &renderer,
        MenuUI &menuUI,
        std::shared_ptr<IUIConfigurationService> uiConfigService,
        std::shared_ptr<IGameConfigurationService> gameConfigService,
        std::shared_ptr<IResourceService> resourceService);

private:
    // Helper methods for specific system initialization
    void initializeCameraManager(
        CameraManager &cameraManager,
        const sf::RenderWindow &window,
        std::shared_ptr<IGameConfigurationService> gameConfigService);

    void initializeParticleSystem(
        ParticleSystem &particleSystem,
        std::shared_ptr<IGameConfigurationService> gameConfigService);

    void initializeRenderer(
        Renderer &renderer,
        const sf::Vector2f &windowSize,
        std::shared_ptr<IResourceService> resourceService);

    void initializeMenuUI(
        MenuUI &menuUI,
        const sf::Vector2f &windowSize,
        std::shared_ptr<IUIConfigurationService> uiConfigService,
        std::shared_ptr<IResourceService> resourceService);
};
