#include "UIManager.h"
#include "../../Data/Resources/Audio.h"
#include <iostream>

UIManager::UIManager()
    : m_isInitialized(false)
{
}

void UIManager::initialize(
    const sf::Vector2f &windowSize,
    std::shared_ptr<IUIConfigurationService> uiConfigService,
    std::shared_ptr<IResourceService> resourceService)
{
    m_uiConfigService = uiConfigService;
    m_resourceService = resourceService;

    // Get fonts from resource service
    const sf::Font &exitFont = resourceService->getFont("peter-light");
    const sf::Font &jelloFont = resourceService->getFont("helvetica-ultra-light");

    // Initialize MenuUI with fonts
    m_menuUI.initialize(exitFont, uiConfigService, resourceService);
    m_menuUI.setJelloFont(jelloFont);

    // Center Jello text and position circles
    m_menuUI.centerJelloText(windowSize);
    m_menuUI.positionCircles(windowSize);

    m_isInitialized = true;
}

void UIManager::update(float deltaTime)
{
    if (!m_isInitialized)
        return;

    m_menuUI.update(deltaTime);

    // If exit was clicked, wait a short delay so the click sound can play
    if (m_pendingExit)
    {
        if (m_exitClock.getElapsedTime().asMilliseconds() >= m_exitDelayMs)
        {
            if (m_exitCallback)
            {
                m_exitCallback();
            }
            m_pendingExit = false;
        }
    }
}

void UIManager::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    m_menuUI.draw(window);
}

void UIManager::handleWindowResize(const sf::Vector2f &windowSize)
{
    if (!m_isInitialized)
        return;

    m_menuUI.centerJelloText(windowSize);
    m_menuUI.positionCircles(windowSize);
}

void UIManager::updateHover(const sf::Vector2f &mousePos)
{
    if (!m_isInitialized)
        return;

    m_menuUI.updateHover(mousePos);
}

void UIManager::handleClick(const sf::Vector2f &clickPos)
{
    if (!m_isInitialized)
        return;

    m_menuUI.handleClick(clickPos);
}

bool UIManager::isExitButtonClicked(const sf::Vector2f &clickPos)
{
    if (!m_isInitialized)
        return false;

    bool clicked = m_menuUI.getBounds().contains(clickPos);
    if (clicked)
    {
        // Play click sound for Exit
        AudioManager::getInstance().playSound("menu_click");
        // Defer actual exit to allow sound to play
        m_pendingExit = true;
        m_exitClock.restart();
    }
    return clicked;
}

void UIManager::setExitCallback(std::function<void()> callback)
{
    m_exitCallback = callback;
}
