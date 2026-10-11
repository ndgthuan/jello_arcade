#include "MenuUI.h"
#include "../../Data/Resources/ResourceManager.h"
#include "../../Data/Config/UIConfig.h"
#include "../../Data/Resources/Audio.h"
#include <iostream>

MenuUI::MenuUI() : m_font(nullptr), m_isInitialized(false)
{
    // Lazy initialization - components will be created in initialize() method
}

void MenuUI::initialize(const sf::Font &font, std::shared_ptr<IUIConfigurationService> configService, std::shared_ptr<IResourceService> resourceService)
{
    // Store font pointer for text rendering
    m_font = &font;

    // Store services
    m_configService = configService;
    m_resourceService = resourceService;

    // Initialize all UI components
    initializeComponents(font);

    // Set initialized to true
    m_isInitialized = true;

    // Set position - place at top-left corner
    setPosition(37, 33);
}

void MenuUI::initializeComponents(const sf::Font &font)
{
    // Create Exit button with configuration service
    m_exitButton = std::make_unique<Button>(font, "Exit", 24, m_configService);
    m_exitButton->setPosition(37, 33);

    // Set click callback for exit button
    m_exitButton->setClickCallback([this]()
                                   {
                                       // Exit callback will be handled by UIManager
                                   });

    // Create title text
    m_titleText = std::make_unique<TitleText>();
    m_titleText->setFont(font);
    m_titleText->setText("Jello");
    if (m_configService)
    {
        m_titleText->setSize(m_configService->getTitleFontSize());
        m_titleText->setColor(m_configService->getTitleColor());
    }

    // Create circle menu
    m_circleMenu = std::make_unique<CircleMenu>();
    setupCircleMenu();
}

void MenuUI::setupCircleMenu()
{
    // Initialize circles (icons and labels are handled internally)
    // Initialize circles with services
    m_circleMenu->initialize(5, m_configService, m_resourceService);
}

void MenuUI::setJelloFont(const sf::Font &font)
{
    if (!m_isInitialized || !m_titleText)
        return;

    // Update title text with new font
    m_titleText->setFont(font);
}

void MenuUI::setPosition(float x, float y)
{
    if (!m_isInitialized)
        return;

    // Set Exit button position
    if (m_exitButton)
    {
        m_exitButton->setPosition(x, y);
    }
}

void MenuUI::centerJelloText(const sf::Vector2f &windowSize)
{
    if (!m_isInitialized || !m_titleText)
        return;

    // Center title text with offset
    if (m_configService)
    {
        m_titleText->setCenterOffset(m_configService->getTitleCenterOffsetX(), m_configService->getTitleCenterOffsetY());
    }
    m_titleText->centerText(windowSize);
}

void MenuUI::positionCircles(const sf::Vector2f &windowSize)
{
    if (!m_isInitialized || !m_circleMenu || !m_titleText)
        return;

    // Get title position and bounds for circle positioning
    sf::Vector2f titlePos = m_titleText->getPosition();
    sf::FloatRect titleBounds = m_titleText->getBounds();

    // Position circles relative to title
    m_circleMenu->positionCircles(windowSize, titlePos, titleBounds);
}

void MenuUI::setSelected(bool selected)
{
    if (!m_isInitialized || !m_exitButton)
        return;

    // Set button selection state
    m_exitButton->setSelected(selected);
}

void MenuUI::update(float deltaTime)
{
    if (!m_isInitialized)
        return;

    // Update all components
    if (m_exitButton)
    {
        m_exitButton->update(deltaTime);
    }

    if (m_titleText)
    {
        m_titleText->updateGlow(deltaTime);
    }

    if (m_circleMenu)
    {
        m_circleMenu->update(deltaTime);
    }
}

void MenuUI::updateHover(const sf::Vector2f &mousePos)
{
    if (!m_isInitialized || !m_circleMenu)
        return;

    // Update hover state for Exit button
    if (m_exitButton)
    {
        bool isHovered = m_exitButton->isHovered(mousePos);
        m_exitButton->setSelected(isHovered);
    }

    // Update hover state for circle menu
    m_circleMenu->updateHover(mousePos);
}

void MenuUI::handleClick(const sf::Vector2f &clickPos)
{
    if (!m_isInitialized)
        return;

    // Handle exit button click
    if (m_exitButton && m_exitButton->isHovered(clickPos))
    {
        m_exitButton->handleClick();
    }

    // Handle circle menu item clicks
    if (m_circleMenu && m_circleMenu->isInitialized())
    {
        size_t count = m_circleMenu->getCircleCount();
        for (size_t i = 0; i < count; ++i)
        {
            sf::FloatRect bounds = m_circleMenu->getCircleBounds(i);
            if (bounds.contains(clickPos))
            {
                AudioManager::getInstance().playSound("menu_click");
                if (m_iconCallback)
                {
                    m_iconCallback(static_cast<int>(i));
                }
                break;
            }
        }
    }
}

void MenuUI::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    // Draw all components in order
    if (m_titleText)
    {
        m_titleText->draw(window);
    }

    if (m_circleMenu)
    {
        m_circleMenu->draw(window);
    }

    if (m_exitButton)
    {
        m_exitButton->draw(window);
    }
}

bool MenuUI::isSelected() const
{
    if (!m_isInitialized || !m_exitButton)
        return false;
    return m_exitButton->isSelected();
}

sf::FloatRect MenuUI::getBounds() const
{
    if (!m_isInitialized || !m_exitButton)
        return sf::FloatRect();
    return m_exitButton->getBounds();
}

void MenuUI::recenterJelloText(const sf::Vector2f &windowSize)
{
    centerJelloText(windowSize);
}
