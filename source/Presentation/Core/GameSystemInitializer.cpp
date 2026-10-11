#include "GameSystemInitializer.h"
#include "../Rendering/CameraManager.h"
#include "../Rendering/ParticleSystem.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/BackgroundManager.h"
#include "../UI/MenuUI.h"
#include "../UI/UIServices.h"

GameSystemInitializer::GameSystemInitializer()
{
}

void GameSystemInitializer::initializeSystems(
    sf::RenderWindow &window,
    CameraManager &cameraManager,
    ParticleSystem &particleSystem,
    Renderer &renderer,
    MenuUI &menuUI,
    std::shared_ptr<IUIConfigurationService> uiConfigService,
    std::shared_ptr<IGameConfigurationService> gameConfigService,
    std::shared_ptr<IResourceService> resourceService)
{
    sf::Vector2f windowSize = sf::Vector2f(window.getSize());

    // Initialize each system
    initializeCameraManager(cameraManager, window, gameConfigService);
    initializeParticleSystem(particleSystem, gameConfigService);
    initializeRenderer(renderer, windowSize, resourceService);
    initializeMenuUI(menuUI, windowSize, uiConfigService, resourceService);
}

void GameSystemInitializer::initializeCameraManager(
    CameraManager &cameraManager,
    const sf::RenderWindow &window,
    std::shared_ptr<IGameConfigurationService> gameConfigService)
{
    // Initialize camera manager with configuration service
    cameraManager.initialize(window, gameConfigService);
}

void GameSystemInitializer::initializeParticleSystem(
    ParticleSystem &particleSystem,
    std::shared_ptr<IGameConfigurationService> gameConfigService)
{
    // Initialize particle system with configuration service
    particleSystem.initialize(gameConfigService);
}

void GameSystemInitializer::initializeRenderer(
    Renderer &renderer,
    const sf::Vector2f &windowSize,
    std::shared_ptr<IResourceService> resourceService)
{
    // Create and initialize background manager
    auto backgroundManager = std::make_shared<BackgroundManager>();
    backgroundManager->setResourceService(resourceService);
    backgroundManager->generateTwilight();
    backgroundManager->centerBackground(windowSize);

    // Set background manager to renderer
    renderer.setBackground(backgroundManager);
}

void GameSystemInitializer::initializeMenuUI(
    MenuUI &menuUI,
    const sf::Vector2f &windowSize,
    std::shared_ptr<IUIConfigurationService> uiConfigService,
    std::shared_ptr<IResourceService> resourceService)
{
    // Get fonts from resource service
    const sf::Font &exitFont = resourceService->getFont("peter-light");
    const sf::Font &jelloFont = resourceService->getFont("helvetica-ultra-light");

    // Initialize MenuUI with fonts
    menuUI.initialize(exitFont, uiConfigService, resourceService);
    menuUI.setJelloFont(jelloFont);

    // Center Jello text and position circles
    menuUI.centerJelloText(windowSize);
    menuUI.positionCircles(windowSize);
}
