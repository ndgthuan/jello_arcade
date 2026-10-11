#include "UIComponents.h"
#include "UIServices.h"
#include "../../Data/Config/UIConfig.h"
#include "../../Data/Resources/Audio.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Soft dark shadow under text so it stays readable over bright parts of the background
static void drawTextShadow(sf::RenderWindow &window, const sf::Text &text, float strength = 1.0f)
{
    sf::Text shadow = text;
    const sf::Vector2f base = text.getPosition();
    const float offsets[3][3] = {{0.0f, 2.0f, 90.0f}, {1.0f, 4.0f, 55.0f}, {-1.0f, 6.0f, 30.0f}};
    for (const auto &o : offsets)
    {
        shadow.setFillColor(sf::Color(0, 20, 30, static_cast<std::uint8_t>(o[2] * strength)));
        shadow.setPosition({base.x + o[0], base.y + o[1]});
        window.draw(shadow);
    }
}

// ===== POSITIONING STRATEGIES IMPLEMENTATION =====

CircleMenuPositioningStrategy::CircleMenuPositioningStrategy(float circleRadius, float spacing)
    : m_circleRadius(circleRadius), m_spacing(spacing)
{
}

sf::Vector2f CircleMenuPositioningStrategy::calculatePosition(const sf::Vector2f &windowSize,
                                                              const sf::Vector2f &titlePosition,
                                                              const sf::FloatRect &titleBounds,
                                                              size_t elementIndex,
                                                              size_t totalElements)
{
    // Calculate starting position for circles (below title text)
    float circleY = titlePosition.y + titleBounds.size.y + 104.0f; // Default from UIConfig

    // Calculate total width needed for circles with spacing
    float totalWidth = (totalElements * m_circleRadius * 2) + ((totalElements - 1) * m_spacing);

    // Calculate starting X position to center the row of circles
    float startX = (windowSize.x - totalWidth) / 2.0f;

    // Calculate circle X position
    float circleX = startX + (elementIndex * (m_circleRadius * 2 + m_spacing));

    return sf::Vector2f(circleX, circleY);
}

sf::Vector2f CircleMenuPositioningStrategy::calculateLabelPosition(const sf::Vector2f &circlePosition,
                                                                   const sf::FloatRect &labelBounds) const
{
    // Calculate center of the circle
    float circleCenterX = circlePosition.x + m_circleRadius;

    // Calculate label Y position below circle
    float labelY = circlePosition.y + m_circleRadius * 2 + 20.0f; // Default from UIConfig

    // Calculate label X position - IMPORTANT: This +10.f offset is always applied as requested
    float labelX = circleCenterX - (labelBounds.size.x / 2.0f) + 10.0f; // Default from UIConfig

    return sf::Vector2f(labelX, labelY);
}

sf::Vector2f DefaultIconPositioningStrategy::calculateIconPosition(
    const sf::Vector2f &circleCenter,
    const sf::FloatRect &iconBounds,
    const std::string &iconName) const
{
    // Default positioning - center icon in circle
    float iconX = circleCenter.x - (iconBounds.size.x / 2.0f);
    float iconY = circleCenter.y - (iconBounds.size.y / 2.0f);

    // Special handling for specific icons
    if (iconName == "account")
    {
        // Add custom offset for the account icon
        iconX += UIConfig::ACCOUNT_ICON_OFFSET_X;
        iconY += UIConfig::ACCOUNT_ICON_OFFSET_Y;
    }

    return sf::Vector2f(iconX, iconY);
}

// ===== BUTTON IMPLEMENTATION =====

Button::Button(const sf::Font &font, const std::string &text, unsigned int size, std::shared_ptr<IUIConfigurationService> configService)
    : m_isSelected(false), m_isInitialized(false),
      m_normalColor(configService ? configService->getButtonNormalColor() : sf::Color(255, 255, 255, 140)),
      m_hoverColor(configService ? configService->getButtonHoverColor() : sf::Color(255, 255, 255, 200)),
      m_currentColor(m_normalColor),
      m_progressBarWidth(0.0f), m_targetProgressBarWidth(0.0f),
      m_progressBarAlpha(0.0f), m_targetProgressBarAlpha(0.0f),
      m_configService(configService)
{
    // Create text element
    m_text = std::make_unique<sf::Text>(font, text, size);
    m_text->setFillColor(m_normalColor);
    m_text->setOutlineThickness(0.0f);

    // Create progress bar for hover effect
    float barHeight = m_configService ? m_configService->getButtonProgressBarHeight() : 0.5f;
    m_progressBar = std::make_unique<sf::RectangleShape>(sf::Vector2f(0, barHeight));
    m_progressBar->setFillColor(sf::Color::White);
    m_isInitialized = true;
}

void Button::setSelected(bool selected)
{
    if (!m_isInitialized)
        return;

    m_isSelected = selected;
    if (selected)
    {
        // Set target for animation to show white progress bar
        float textWidth = m_text->getGlobalBounds().size.x;
        m_targetProgressBarWidth = textWidth;
        m_targetProgressBarAlpha = 200.0f;
        m_currentColor = m_hoverColor;
    }
    else
    {
        // Set target for animation to hide progress bar
        m_targetProgressBarWidth = 0.0f;
        m_targetProgressBarAlpha = 0.0f;
        m_currentColor = m_normalColor;
    }

    updateColor();
}

void Button::setPosition(float x, float y)
{
    if (!m_isInitialized)
        return;

    m_text->setPosition(sf::Vector2f(x, y));

    // Position progress bar below text
    sf::FloatRect textBounds = m_text->getGlobalBounds();
    float spacing = m_configService ? m_configService->getButtonLabelSpacing() : 21.0f;
    float barY = y + textBounds.size.y + spacing;
    m_progressBar->setPosition(sf::Vector2f(x, barY));
}

void Button::setColor(const sf::Color &color)
{
    m_normalColor = color;
    if (!m_isSelected)
    {
        m_currentColor = color;
        updateColor();
    }
}

void Button::setHoverColor(const sf::Color &color)
{
    m_hoverColor = color;
    if (m_isSelected)
    {
        m_currentColor = color;
        updateColor();
    }
}

bool Button::isHovered(const sf::Vector2f &mousePos) const
{
    if (!m_isInitialized)
        return false;
    return m_text->getGlobalBounds().contains(mousePos);
}

void Button::setClickCallback(std::function<void()> callback)
{
    m_clickCallback = callback;
}

void Button::handleClick()
{
    // Play click sound
    AudioManager::getInstance().playSound("menu_click");

    if (m_clickCallback)
    {
        m_clickCallback();
    }
}

void Button::update(float deltaTime)
{
    if (!m_isInitialized)
        return;

    updateProgressBar(deltaTime);
}

void Button::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    // Draw progress bar first (behind text)
    if (m_progressBarWidth > 0.1f)
    {
        window.draw(*m_progressBar);
    }

    // Draw text on top
    drawTextShadow(window, *m_text);
    window.draw(*m_text);
}

sf::FloatRect Button::getBounds() const
{
    if (!m_isInitialized)
        return sf::FloatRect();
    return m_text->getGlobalBounds();
}

void Button::updateProgressBar(float deltaTime)
{
    // Smooth animation for progress bar
    float animationSpeed = m_configService ? m_configService->getButtonAnimationSpeed() : 8.0f;

    // Animate width smoothly
    m_progressBarWidth += (m_targetProgressBarWidth - m_progressBarWidth) * animationSpeed * deltaTime;

    // Animate alpha smoothly
    m_progressBarAlpha += (m_targetProgressBarAlpha - m_progressBarAlpha) * animationSpeed * deltaTime;

    // Update progress bar with animation
    if (m_progressBarWidth > 0.1f)
    {
        float barHeight = m_configService ? m_configService->getButtonProgressBarHeight() : 0.5f;
        m_progressBar->setSize(sf::Vector2f(m_progressBarWidth, barHeight));

        // Center progress bar - expand from center outward
        float textCenterX = m_text->getPosition().x + m_text->getGlobalBounds().size.x / 2.0f;
        float barX = textCenterX - m_progressBarWidth / 2.0f;
        m_progressBar->setPosition(sf::Vector2f(barX, m_progressBar->getPosition().y));

        // Update color with alpha transparency
        sf::Color barColor = sf::Color::White;
        barColor.a = static_cast<unsigned char>(m_progressBarAlpha);
        m_progressBar->setFillColor(barColor);
    }
    else
    {
        // Hide progress bar when width is too small
        float barHeight = m_configService ? m_configService->getButtonProgressBarHeight() : 0.5f;
        m_progressBar->setSize(sf::Vector2f(0, barHeight));
    }
}

void Button::updateColor()
{
    if (m_text)
    {
        m_text->setFillColor(m_currentColor);
    }
}

// ===== TITLE TEXT IMPLEMENTATION =====

TitleText::TitleText()
    : m_font(nullptr), m_isInitialized(false), m_glowEnabled(true),
      m_glowIntensity(0.0f), m_animationTime(0.0f),
      m_glowColor(sf::Color(0, 0, 0, 20)), m_centerOffset(0.0f, 0.0f), m_basePosition(0.0f, 0.0f),
      m_positioningService(nullptr)
{
}

void TitleText::setFont(const sf::Font &font)
{
    m_font = &font;

    if (!m_text)
    {
        // Use default values if no configuration service is available
        unsigned int fontSize = 175;                          // Default from UIConfig
        sf::Color titleColor = sf::Color(255, 255, 255, 200); // Default from UIConfig
        m_text = std::make_unique<sf::Text>(font, "Jello", fontSize);
        m_text->setFillColor(titleColor);
        m_text->setOutlineThickness(0.0f);
        m_isInitialized = true;

        // Initialize glow layers
        updateGlowLayers();
    }
    else
    {
        m_text->setFont(font);
        updateGlowLayers();
    }
}

void TitleText::setText(const std::string &content)
{
    if (m_text)
    {
        m_text->setString(content);
        updateGlowLayers();
    }
}

void TitleText::setSize(unsigned int size)
{
    if (m_text)
    {
        m_text->setCharacterSize(size);
        updateGlowLayers();
    }
}

void TitleText::setColor(const sf::Color &color)
{
    if (m_text)
    {
        m_text->setFillColor(color);
    }
}

void TitleText::centerText(const sf::Vector2f &windowSize)
{
    if (!m_isInitialized || !m_text)
        return;

    if (m_positioningService)
    {
        // Use positioning service if available
        m_positioningService->centerText(*m_text, windowSize, m_centerOffset);
    }
    else
    {
        // Fallback to manual calculation
        sf::FloatRect textBounds = m_text->getGlobalBounds();
        float centerX = (windowSize.x - textBounds.size.x) / 2.0f + m_centerOffset.x;
        float centerY = (windowSize.y - textBounds.size.y) / 2.0f + m_centerOffset.y;
        m_text->setPosition({centerX, centerY});
    }

    // Set base position for floating animation
    m_basePosition = m_text->getPosition();

    // Update glow layers positions
    updateGlowLayers();
}

void TitleText::setCenterOffset(float x, float y)
{
    m_centerOffset = sf::Vector2f(x, y);
}

void TitleText::enableGlow(bool enable)
{
    m_glowEnabled = enable;
}

void TitleText::setGlowIntensity(float intensity)
{
    m_glowIntensity = intensity;
}

void TitleText::setGlowColor(const sf::Color &color)
{
    m_glowColor = color;
    updateGlowLayers();
}

void TitleText::setPositioningService(std::shared_ptr<IUIPositioningService> positioningService)
{
    m_positioningService = positioningService;
}

void TitleText::updateGlow(float deltaTime)
{
    if (!m_glowEnabled)
        return;

    // Update animation time
    m_animationTime += deltaTime;

    // Animate glow intensity with smoother curve
    float minIntensity = 0.6f;   // Default from UIConfig
    float maxIntensity = 1.0f;   // Default from UIConfig
    float animationSpeed = 2.5f; // Default from UIConfig
    m_glowIntensity = minIntensity + (maxIntensity - minIntensity) * sin(m_animationTime * animationSpeed);

    // Apply glow effect
    applyGlowEffect();
}

void TitleText::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    // Draw glow layers first (behind the text)
    if (m_glowEnabled)
    {
        for (const auto &glowLayer : m_glowLayers)
        {
            window.draw(glowLayer);
        }
    }

    // Draw title text
    if (m_text)
    {
        drawTextShadow(window, *m_text, 0.8f);
        window.draw(*m_text);
    }
}

sf::FloatRect TitleText::getBounds() const
{
    if (!m_isInitialized || !m_text)
        return sf::FloatRect();
    return m_text->getGlobalBounds();
}

sf::Vector2f TitleText::getPosition() const
{
    if (!m_isInitialized || !m_text)
        return sf::Vector2f();
    return m_text->getPosition();
}

void TitleText::updateGlowLayers()
{
    if (!m_text || !m_font)
        return;

    m_glowLayers.clear();

    // Create glow layers with different sizes and opacities
    int glowLayers = 3;            // Default from UIConfig
    float scaleIncrement = 0.015f; // Default from UIConfig
    float alphaBase = 25.0f;       // Default from UIConfig
    float alphaDecrement = 6.0f;   // Default from UIConfig

    for (int i = 0; i < glowLayers; ++i)
    {
        sf::Text glowLayer = *m_text;
        float scale = 1.0f + (i * scaleIncrement);      // Softer scale increase
        float alpha = alphaBase - (i * alphaDecrement); // Softer opacity decrease
        glowLayer.setScale(sf::Vector2f(scale, scale));
        glowLayer.setFillColor(sf::Color(m_glowColor.r, m_glowColor.g, m_glowColor.b, static_cast<unsigned char>(alpha)));
        glowLayer.setPosition(m_text->getPosition());
        m_glowLayers.push_back(glowLayer);
    }
}

void TitleText::applyGlowEffect()
{
    if (!m_glowEnabled)
        return;

    // Apply glow intensity to all glow layers
    for (auto &glowLayer : m_glowLayers)
    {
        sf::Color currentColor = glowLayer.getFillColor();
        currentColor.a = static_cast<unsigned char>(currentColor.a * m_glowIntensity);
        glowLayer.setFillColor(currentColor);
    }
}

// ===== CIRCLE ITEM IMPLEMENTATION =====

CircleItem::CircleItem()
    : m_circle(), m_isInitialized(false), m_isHovered(false), m_hoverScale(1.0f),
      m_hoverScaleTarget(1.0f), m_hoverScaleSpeed(8.0f), m_iconPositioningStrategy(nullptr)
{
}

void CircleItem::initialize(float radius, const sf::Color &color, const sf::Color &outlineColor, float outlineThickness)
{
    m_circle.setRadius(radius);
    m_circle.setFillColor(color);
    m_circle.setOutlineColor(outlineColor);
    m_circle.setOutlineThickness(outlineThickness);
    m_isInitialized = true;
}

void CircleItem::setIcon(const std::string &iconName, std::shared_ptr<IResourceService> resourceService)
{
    m_iconName = iconName; // Store icon name for identification

    try
    {
        if (resourceService)
        {
            const sf::Texture &texture = resourceService->getTexture(iconName);
            m_icon = sf::Sprite(texture);

            // Scale icon to fit nicely inside the circle
            float scale = 0.1f; // Default from UIConfig
            m_icon->setScale(sf::Vector2f(scale, scale));
        }
    }
    catch (const std::exception &e)
    {
        // If texture not found, create empty sprite
        sf::Texture defaultTexture;
        m_icon = sf::Sprite(defaultTexture);
    }
}

void CircleItem::setLabel(const std::string &labelText, const sf::Font &font)
{
    m_label = sf::Text(font, labelText, 27);
    m_label->setFillColor(sf::Color(255, 255, 255, 255)); // Default from UIConfig
    m_label->setOutlineThickness(0.0f);
}

void CircleItem::setIconPositioningStrategy(std::shared_ptr<IIconPositioningStrategy> strategy)
{
    m_iconPositioningStrategy = strategy;
}

void CircleItem::setPosition(float x, float y)
{
    m_circle.setPosition(sf::Vector2f(x, y));

    // Position icon in center of circle using strategy
    if (m_icon.has_value())
    {
        sf::Vector2f iconPos = calculateIconPosition();
        m_icon->setPosition(iconPos);
    }
}

sf::Vector2f CircleItem::calculateIconPosition() const
{
    if (!m_iconPositioningStrategy || !m_icon.has_value())
    {
        // Fallback to default positioning
        float circleCenterX = m_circle.getPosition().x + m_circle.getRadius();
        float circleCenterY = m_circle.getPosition().y + m_circle.getRadius();
        sf::FloatRect iconBounds = m_icon->getGlobalBounds();

        return sf::Vector2f(
            circleCenterX - (iconBounds.size.x / 2.0f),
            circleCenterY - (iconBounds.size.y / 2.0f));
    }

    // Use strategy for positioning
    sf::Vector2f circleCenter(m_circle.getPosition().x + m_circle.getRadius(),
                              m_circle.getPosition().y + m_circle.getRadius());
    sf::FloatRect iconBounds = m_icon->getGlobalBounds();

    return m_iconPositioningStrategy->calculateIconPosition(circleCenter, iconBounds, m_iconName);
}

void CircleItem::setLabelPosition(float x, float y)
{
    if (m_label.has_value())
    {
        m_label->setPosition(sf::Vector2f(x, y));
    }
}

void CircleItem::setCircleColor(const sf::Color &color)
{
    m_circle.setFillColor(color);
}

void CircleItem::setOutlineColor(const sf::Color &color)
{
    m_circle.setOutlineColor(color);
}

void CircleItem::setOutlineThickness(float thickness)
{
    m_circle.setOutlineThickness(thickness);
}

void CircleItem::updateHover(const sf::Vector2f &mousePos)
{
    sf::FloatRect circleBounds = m_circle.getGlobalBounds();
    m_isHovered = circleBounds.contains(mousePos);

    m_hoverScaleTarget = m_isHovered ? 1.2f : 1.0f; // Default from UIConfig
}

void CircleItem::setPrimary(bool primary, const sf::Color &accent)
{
    m_primary = primary;
    m_accent = accent;
    if (primary)
        m_circle.setFillColor(accent);
}

void CircleItem::update(float deltaTime)
{
    m_pulseTime += deltaTime;
    updateHoverAnimation(deltaTime);
}

void CircleItem::setHoverScale(float scale)
{
    m_hoverScale = scale;
}

void CircleItem::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    // Pulsing ring behind the primary item
    if (m_primary)
    {
        float phase = std::fmod(m_pulseTime * 0.6f, 1.0f);
        float r = m_circle.getRadius() * (1.0f + 0.45f * phase);
        sf::CircleShape ring(r);
        ring.setPosition({m_circle.getPosition().x + m_circle.getRadius() - r, m_circle.getPosition().y + m_circle.getRadius() - r});
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(sf::Color(m_accent.r, m_accent.g, m_accent.b, static_cast<std::uint8_t>(150 * (1.0f - phase))));
        window.draw(ring);
    }

    // Draw circle with hover scale
    sf::CircleShape scaledCircle = m_circle;
    sf::Vector2f originalPos = scaledCircle.getPosition();
    float originalRadius = scaledCircle.getRadius();

    scaledCircle.setPosition(originalPos);
    scaledCircle.setRadius(originalRadius * m_hoverScale);
    window.draw(scaledCircle);

    // Draw icon with hover scale and smooth offset
    if (m_icon.has_value())
    {
        sf::Sprite scaledIcon = *m_icon;
        sf::Vector2f originalScale = scaledIcon.getScale();

        // Get the center of the circle
        float circleX = m_circle.getPosition().x + m_circle.getRadius();
        float circleY = m_circle.getPosition().y + m_circle.getRadius();

        // Calculate the original icon size (before any scaling)
        sf::FloatRect originalBounds = m_icon->getGlobalBounds();
        float originalWidth = originalBounds.size.x / originalScale.x;
        float originalHeight = originalBounds.size.y / originalScale.y;

        // Apply hover scale to icon
        float newScale = originalScale.x * m_hoverScale;
        scaledIcon.setScale(sf::Vector2f(newScale, newScale));

        // Use strategy for custom positioning during hover
        if (m_iconPositioningStrategy)
        {
            sf::FloatRect scaledBounds(sf::Vector2f(0, 0), sf::Vector2f(originalWidth * newScale, originalHeight * newScale));
            sf::Vector2f strategyPos = m_iconPositioningStrategy->calculateIconPosition(
                sf::Vector2f(circleX, circleY),
                scaledBounds,
                m_iconName);

            // Apply strategy position with hover offset
            float offsetAmount = (m_hoverScale - 1.0f) * 50.0f;
            float iconX = strategyPos.x + offsetAmount;
            float iconY = strategyPos.y + offsetAmount;

            scaledIcon.setPosition(sf::Vector2f(iconX, iconY));
        }
        else
        {
            // Fallback: Calculate new position to keep icon centered
            float iconX = circleX - (originalWidth * newScale / 2.0f);
            float iconY = circleY - (originalHeight * newScale / 2.0f);

            // Add smooth offset transition based on hover scale
            float offsetAmount = (m_hoverScale - 1.0f) * 50.0f;
            iconX += offsetAmount;
            iconY += offsetAmount;

            scaledIcon.setPosition(sf::Vector2f(iconX, iconY));
        }

        window.draw(scaledIcon);
    }

    // Labels stay visible (dimmed) so icons are never ambiguous; hover brings them to full strength
    if (m_label.has_value())
    {
        float t = std::clamp((m_hoverScale - 1.0f) / 0.2f, 0.0f, 1.0f);
        m_label->setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(225 + 30 * t)));
        drawTextShadow(window, *m_label, 1.8f);
        window.draw(*m_label);
    }
}

sf::FloatRect CircleItem::getBounds() const
{
    return m_circle.getGlobalBounds();
}

sf::FloatRect CircleItem::getLabelBounds() const
{
    if (m_label.has_value())
    {
        return m_label->getGlobalBounds();
    }
    return sf::FloatRect();
}

void CircleItem::updateHoverAnimation(float deltaTime)
{
    // Smooth animation towards target scale
    m_hoverScale += (m_hoverScaleTarget - m_hoverScale) * m_hoverScaleSpeed * deltaTime;

    // Clamp scale values
    m_hoverScale = std::max(0.8f, std::min(1.5f, m_hoverScale));
}

// ===== CIRCLE MENU IMPLEMENTATION =====

CircleMenu::CircleMenu()
    : m_isInitialized(false), m_circleRadius(50.0f), m_spacing(30.0f),
      m_circleColor(sf::Color(255, 255, 255, 255)), m_outlineColor(sf::Color(255, 255, 255, 30)), m_outlineThickness(1.0f)
{
    // Initialize positioning strategy
    updatePositioningStrategy();
}

void CircleMenu::initialize(int count, std::shared_ptr<IUIConfigurationService> configService, std::shared_ptr<IResourceService> resourceService)
{
    // Store services
    m_configService = configService;
    m_resourceService = resourceService;

    // Update configuration from service if available
    if (m_configService)
    {
        m_circleRadius = m_configService->getDefaultCircleRadius();
        m_spacing = m_configService->getDefaultCircleSpacing();
        m_circleColor = m_configService->getCircleColor();
        m_outlineColor = m_configService->getCircleOutlineColor();
        m_outlineThickness = m_configService->getCircleOutlineThickness();
    }

    m_circleItems.clear();

    // Create circle items with specified properties
    for (int i = 0; i < count; ++i)
    {
        auto circleItem = std::make_unique<CircleItem>();
        circleItem->initialize(m_circleRadius, m_circleColor, m_outlineColor, m_outlineThickness);
        m_circleItems.push_back(std::move(circleItem));
    }

    setupCircleItems();
    m_isInitialized = true;
}

void CircleMenu::setupCircleItems()
{
    if (m_circleItems.empty())
        return;

    // Define icon names and labels
    std::vector<std::string> iconNames = {"play", "settings", "info", "interrogation", "account"};
    std::vector<std::string> labelTexts = {"Play", "Settings", "Credits", "Help", "Alt Manager"};

    // Get font for labels
    if (!m_resourceService)
        return;
    const sf::Font &helveticaFont = m_resourceService->getFont("peter-light"); // heavier than ultra-light, stays legible over the artwork

    // Create icon positioning strategy
    auto iconStrategy = std::make_shared<DefaultIconPositioningStrategy>();

    // Setup each circle item
    for (size_t i = 0; i < m_circleItems.size(); ++i)
    {
        // Set icon positioning strategy
        m_circleItems[i]->setIconPositioningStrategy(iconStrategy);

        if (i < iconNames.size())
        {
            m_circleItems[i]->setIcon(iconNames[i], m_resourceService);
        }

        if (i < labelTexts.size())
        {
            m_circleItems[i]->setLabel(labelTexts[i], helveticaFont);
        }

        // "Play" is the main action
        if (i == 0)
        {
            m_circleItems[i]->setPrimary(true, sf::Color(255, 226, 150));
        }
    }
}

void CircleMenu::setCircleRadius(float radius)
{
    m_circleRadius = radius;
    updatePositioningStrategy();

    if (m_isInitialized)
    {
        for (auto &item : m_circleItems)
        {
            item->initialize(m_circleRadius, m_circleColor, m_outlineColor, m_outlineThickness);
        }
    }
}

void CircleMenu::setSpacing(float spacing)
{
    m_spacing = spacing;
    updatePositioningStrategy();
}

void CircleMenu::setCircleColor(const sf::Color &color)
{
    m_circleColor = color;
    if (m_isInitialized)
    {
        for (auto &item : m_circleItems)
        {
            item->setCircleColor(color);
        }
    }
}

void CircleMenu::setOutlineColor(const sf::Color &color)
{
    m_outlineColor = color;
    if (m_isInitialized)
    {
        for (auto &item : m_circleItems)
        {
            item->setOutlineColor(color);
        }
    }
}

void CircleMenu::setOutlineThickness(float thickness)
{
    m_outlineThickness = thickness;
    if (m_isInitialized)
    {
        for (auto &item : m_circleItems)
        {
            item->setOutlineThickness(thickness);
        }
    }
}

void CircleMenu::positionCircles(const sf::Vector2f &windowSize, const sf::Vector2f &titlePosition, const sf::FloatRect &titleBounds)
{
    if (!m_isInitialized || m_circleItems.empty() || !m_positioningStrategy)
        return;

    // Position each circle item using the positioning strategy
    for (size_t i = 0; i < m_circleItems.size(); ++i)
    {
        // Calculate circle position using strategy
        sf::Vector2f circlePos = m_positioningStrategy->calculatePosition(
            windowSize, titlePosition, titleBounds, i, m_circleItems.size());

        m_circleItems[i]->setPosition(circlePos.x, circlePos.y);

        // Calculate label position using strategy - ensures +10.f offset is always applied
        sf::FloatRect labelBounds = m_circleItems[i]->getLabelBounds();
        sf::Vector2f labelPos = m_positioningStrategy->calculateLabelPosition(circlePos, labelBounds);

        m_circleItems[i]->setLabelPosition(labelPos.x, labelPos.y);
    }
}

void CircleMenu::updatePositioningStrategy()
{
    // Create new positioning strategy with current radius and spacing
    m_positioningStrategy = std::make_unique<CircleMenuPositioningStrategy>(m_circleRadius, m_spacing);
}

void CircleMenu::updateHover(const sf::Vector2f &mousePos)
{
    if (!m_isInitialized)
        return;

    for (auto &item : m_circleItems)
    {
        item->updateHover(mousePos);
    }
}

void CircleMenu::update(float deltaTime)
{
    if (!m_isInitialized)
        return;

    for (auto &item : m_circleItems)
    {
        item->update(deltaTime);
    }
}

void CircleMenu::draw(sf::RenderWindow &window)
{
    if (!m_isInitialized)
        return;

    for (auto &item : m_circleItems)
    {
        item->draw(window);
    }
}

sf::FloatRect CircleMenu::getCircleBounds(size_t index) const
{
    if (index >= m_circleItems.size())
        return sf::FloatRect();
    return m_circleItems[index]->getBounds();
}
