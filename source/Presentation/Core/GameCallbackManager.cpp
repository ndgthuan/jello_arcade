#include "GameCallbackManager.h"
#include "../Input/InputHandler.h"
#include "../Rendering/BackgroundManager.h"
#include "../Rendering/CameraManager.h"
#include "../UI/MenuUI.h"

GameCallbackManager::GameCallbackManager()
{
}

void GameCallbackManager::setupInputCallbacks(
    InputHandler &inputHandler,
    std::function<void(const sf::Vector2f &, const sf::Vector2f &)> mouseMoveCallback,
    std::function<void(const sf::Vector2f &)> mouseClickCallback,
    std::function<void()> exitCallback)
{
    // Store exit callback
    m_exitCallback = exitCallback;

    // Setup specific callbacks
    setupMouseMoveCallback(inputHandler, mouseMoveCallback);
    setupCloseCallback(inputHandler);
    setupMouseClickCallback(inputHandler, mouseClickCallback);
}

void GameCallbackManager::setExitCallback(std::function<void()> callback)
{
    m_exitCallback = callback;
}

void GameCallbackManager::setupMouseMoveCallback(
    InputHandler &inputHandler,
    std::function<void(const sf::Vector2f &, const sf::Vector2f &)> mouseMoveCallback)
{
    // Setup mouse move callback using provided callback function
    inputHandler.setMouseMoveCallback(mouseMoveCallback);
}

void GameCallbackManager::setupCloseCallback(InputHandler &inputHandler)
{
    // Setup window close callback
    inputHandler.setCloseCallback([this]()
                                  {
                                      // This will be handled by GameWindow
                                  });
}

void GameCallbackManager::setupMouseClickCallback(
    InputHandler &inputHandler,
    std::function<void(const sf::Vector2f &)> mouseClickCallback)
{
    // Setup mouse click callback using provided callback function
    inputHandler.setMouseClickCallback(mouseClickCallback);
}
