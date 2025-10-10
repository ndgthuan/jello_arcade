#include "UIServices.h"
#include "../../Data/Config/UIConfig.h"
#include "../../Data/Resources/ResourceManager.h"

// ===== CONFIGURATION SERVICE IMPLEMENTATION =====

// UI Configuration constants
float ConfigurationService::getTitleCenterOffsetX() const
{
    return UIConfig::TITLE_CENTER_OFFSET_X;
}

float ConfigurationService::getTitleCenterOffsetY() const
{
    return UIConfig::TITLE_CENTER_OFFSET_Y;
}

float ConfigurationService::getTitleCircleSpacing() const
{
    return UIConfig::TITLE_CIRCLE_SPACING;
}

float ConfigurationService::getLabelOffsetX() const
{
    return UIConfig::LABEL_OFFSET_X;
}

float ConfigurationService::getLabelOffsetY() const
{
    return UIConfig::LABEL_OFFSET_Y;
}

float ConfigurationService::getDefaultCircleRadius() const
{
    return UIConfig::DEFAULT_CIRCLE_RADIUS;
}

float ConfigurationService::getDefaultCircleSpacing() const
{
    return UIConfig::DEFAULT_CIRCLE_SPACING;
}

float ConfigurationService::getCircleOutlineThickness() const
{
    return UIConfig::CIRCLE_OUTLINE_THICKNESS;
}

float ConfigurationService::getButtonProgressBarHeight() const
{
    return UIConfig::BUTTON_PROGRESS_BAR_HEIGHT;
}

float ConfigurationService::getButtonAnimationSpeed() const
{
    return UIConfig::BUTTON_ANIMATION_SPEED;
}

float ConfigurationService::getButtonLabelSpacing() const
{
    return UIConfig::BUTTON_LABEL_SPACING;
}

unsigned int ConfigurationService::getTitleFontSize() const
{
    return UIConfig::TITLE_FONT_SIZE;
}

float ConfigurationService::getTitleGlowAnimationSpeed() const
{
    return UIConfig::TITLE_GLOW_ANIMATION_SPEED;
}

float ConfigurationService::getTitleGlowIntensityMin() const
{
    return UIConfig::TITLE_GLOW_INTENSITY_MIN;
}

float ConfigurationService::getTitleGlowIntensityMax() const
{
    return UIConfig::TITLE_GLOW_INTENSITY_MAX;
}

int ConfigurationService::getTitleGlowLayers() const
{
    return UIConfig::TITLE_GLOW_LAYERS;
}

float ConfigurationService::getTitleGlowScaleIncrement() const
{
    return UIConfig::TITLE_GLOW_SCALE_INCREMENT;
}

float ConfigurationService::getTitleGlowAlphaBase() const
{
    return UIConfig::TITLE_GLOW_ALPHA_BASE;
}

float ConfigurationService::getTitleGlowAlphaDecrement() const
{
    return UIConfig::TITLE_GLOW_ALPHA_DECREMENT;
}

float ConfigurationService::getIconScale() const
{
    return UIConfig::ICON_SCALE;
}

float ConfigurationService::getIconHoverScale() const
{
    return UIConfig::ICON_HOVER_SCALE;
}

float ConfigurationService::getIconAnimationSpeed() const
{
    return UIConfig::ICON_ANIMATION_SPEED;
}

float ConfigurationService::getAccountIconOffsetX() const
{
    return UIConfig::ACCOUNT_ICON_OFFSET_X;
}

float ConfigurationService::getAccountIconOffsetY() const
{
    return UIConfig::ACCOUNT_ICON_OFFSET_Y;
}

// Colors
sf::Color ConfigurationService::getTitleColor() const
{
    return UIConfig::TITLE_COLOR;
}

sf::Color ConfigurationService::getTitleGlowColor() const
{
    return UIConfig::TITLE_GLOW_COLOR;
}

sf::Color ConfigurationService::getButtonNormalColor() const
{
    return UIConfig::BUTTON_NORMAL_COLOR;
}

sf::Color ConfigurationService::getButtonHoverColor() const
{
    return UIConfig::BUTTON_HOVER_COLOR;
}

sf::Color ConfigurationService::getCircleColor() const
{
    return UIConfig::CIRCLE_COLOR;
}

sf::Color ConfigurationService::getCircleOutlineColor() const
{
    return UIConfig::CIRCLE_OUTLINE_COLOR;
}

sf::Color ConfigurationService::getLabelColor() const
{
    return UIConfig::LABEL_COLOR;
}

// Game Configuration constants
int ConfigurationService::getDefaultParticleCount() const
{
    return UIConfig::DEFAULT_PARTICLE_COUNT;
}

float ConfigurationService::getCameraSmoothSpeed() const
{
    return UIConfig::CAMERA_SMOOTH_SPEED;
}

float ConfigurationService::getCameraMomentum() const
{
    return UIConfig::CAMERA_MOMENTUM;
}

float ConfigurationService::getCameraMomentumMultiplier() const
{
    return UIConfig::CAMERA_MOMENTUM_MULTIPLIER;
}

// ===== UI CONFIGURATION SERVICE IMPLEMENTATION =====

// UI Positioning constants
float UIConfigurationService::getTitleCenterOffsetX() const
{
    return UIConfig::TITLE_CENTER_OFFSET_X;
}

float UIConfigurationService::getTitleCenterOffsetY() const
{
    return UIConfig::TITLE_CENTER_OFFSET_Y;
}

float UIConfigurationService::getTitleCircleSpacing() const
{
    return UIConfig::TITLE_CIRCLE_SPACING;
}

float UIConfigurationService::getLabelOffsetX() const
{
    return UIConfig::LABEL_OFFSET_X;
}

float UIConfigurationService::getLabelOffsetY() const
{
    return UIConfig::LABEL_OFFSET_Y;
}

// Circle menu constants
float UIConfigurationService::getDefaultCircleRadius() const
{
    return UIConfig::DEFAULT_CIRCLE_RADIUS;
}

float UIConfigurationService::getDefaultCircleSpacing() const
{
    return UIConfig::DEFAULT_CIRCLE_SPACING;
}

float UIConfigurationService::getCircleOutlineThickness() const
{
    return UIConfig::CIRCLE_OUTLINE_THICKNESS;
}

// Button constants
float UIConfigurationService::getButtonProgressBarHeight() const
{
    return UIConfig::BUTTON_PROGRESS_BAR_HEIGHT;
}

float UIConfigurationService::getButtonAnimationSpeed() const
{
    return UIConfig::BUTTON_ANIMATION_SPEED;
}

float UIConfigurationService::getButtonLabelSpacing() const
{
    return UIConfig::BUTTON_LABEL_SPACING;
}

// Title text constants
unsigned int UIConfigurationService::getTitleFontSize() const
{
    return UIConfig::TITLE_FONT_SIZE;
}

float UIConfigurationService::getTitleGlowAnimationSpeed() const
{
    return UIConfig::TITLE_GLOW_ANIMATION_SPEED;
}

float UIConfigurationService::getTitleGlowIntensityMin() const
{
    return UIConfig::TITLE_GLOW_INTENSITY_MIN;
}

float UIConfigurationService::getTitleGlowIntensityMax() const
{
    return UIConfig::TITLE_GLOW_INTENSITY_MAX;
}

int UIConfigurationService::getTitleGlowLayers() const
{
    return UIConfig::TITLE_GLOW_LAYERS;
}

float UIConfigurationService::getTitleGlowScaleIncrement() const
{
    return UIConfig::TITLE_GLOW_SCALE_INCREMENT;
}

float UIConfigurationService::getTitleGlowAlphaBase() const
{
    return UIConfig::TITLE_GLOW_ALPHA_BASE;
}

float UIConfigurationService::getTitleGlowAlphaDecrement() const
{
    return UIConfig::TITLE_GLOW_ALPHA_DECREMENT;
}

// Icon constants
float UIConfigurationService::getIconScale() const
{
    return UIConfig::ICON_SCALE;
}

float UIConfigurationService::getIconHoverScale() const
{
    return UIConfig::ICON_HOVER_SCALE;
}

float UIConfigurationService::getIconAnimationSpeed() const
{
    return UIConfig::ICON_ANIMATION_SPEED;
}

float UIConfigurationService::getAccountIconOffsetX() const
{
    return UIConfig::ACCOUNT_ICON_OFFSET_X;
}

float UIConfigurationService::getAccountIconOffsetY() const
{
    return UIConfig::ACCOUNT_ICON_OFFSET_Y;
}

// UI Colors
sf::Color UIConfigurationService::getTitleColor() const
{
    return UIConfig::TITLE_COLOR;
}

sf::Color UIConfigurationService::getTitleGlowColor() const
{
    return UIConfig::TITLE_GLOW_COLOR;
}

sf::Color UIConfigurationService::getButtonNormalColor() const
{
    return UIConfig::BUTTON_NORMAL_COLOR;
}

sf::Color UIConfigurationService::getButtonHoverColor() const
{
    return UIConfig::BUTTON_HOVER_COLOR;
}

sf::Color UIConfigurationService::getCircleColor() const
{
    return UIConfig::CIRCLE_COLOR;
}

sf::Color UIConfigurationService::getCircleOutlineColor() const
{
    return UIConfig::CIRCLE_OUTLINE_COLOR;
}

sf::Color UIConfigurationService::getLabelColor() const
{
    return UIConfig::LABEL_COLOR;
}

// ===== GAME CONFIGURATION SERVICE IMPLEMENTATION =====

// Game Configuration constants
int GameConfigurationService::getDefaultParticleCount() const
{
    return UIConfig::DEFAULT_PARTICLE_COUNT;
}

float GameConfigurationService::getCameraSmoothSpeed() const
{
    return UIConfig::CAMERA_SMOOTH_SPEED;
}

float GameConfigurationService::getCameraMomentum() const
{
    return UIConfig::CAMERA_MOMENTUM;
}

float GameConfigurationService::getCameraMomentumMultiplier() const
{
    return UIConfig::CAMERA_MOMENTUM_MULTIPLIER;
}

// ===== RESOURCE SERVICE IMPLEMENTATION =====

// Forward declaration wrapper to avoid direct dependency
class ResourceService::ResourceManagerWrapper
{
public:
    ResourceManager &getInstance() { return ResourceManager::getInstance(); }
};

ResourceService::ResourceService()
    : m_resourceManager(std::make_unique<ResourceManagerWrapper>())
{
}

ResourceService::ResourceService(const ResourceService &other)
    : m_resourceManager(std::make_unique<ResourceManagerWrapper>())
{
}

ResourceService::~ResourceService() = default;

const sf::Texture &ResourceService::getTexture(const std::string &id) const
{
    return m_resourceManager->getInstance().getTexture(id);
}

const sf::Font &ResourceService::getFont(const std::string &id) const
{
    return m_resourceManager->getInstance().getFont(id);
}

bool ResourceService::loadConfig(const std::string &filename)
{
    return m_resourceManager->getInstance().loadConfig(filename);
}

// ===== UI POSITIONING SERVICE IMPLEMENTATION =====

void UIPositioningService::centerText(sf::Text &text, const sf::Vector2f &windowSize, const sf::Vector2f &offset)
{
    sf::FloatRect textBounds = text.getGlobalBounds();

    // Calculate center position with offset
    float centerX = (windowSize.x - textBounds.size.x) / 2.0f + offset.x;
    float centerY = (windowSize.y - textBounds.size.y) / 2.0f + offset.y;

    text.setPosition(sf::Vector2f(centerX, centerY));
}

std::vector<sf::Vector2f> UIPositioningService::positionElementsInRow(const std::vector<sf::FloatRect> &elementBounds,
                                                                      const sf::Vector2f &windowSize,
                                                                      float spacing)
{
    std::vector<sf::Vector2f> positions;

    if (elementBounds.empty())
        return positions;

    // Calculate total width needed
    float totalWidth = 0.0f;
    for (const auto &bounds : elementBounds)
    {
        totalWidth += bounds.size.x;
    }
    totalWidth += spacing * (elementBounds.size() - 1);

    // Calculate starting X position to center the row
    float startX = (windowSize.x - totalWidth) / 2.0f;

    // Calculate positions for each element
    float currentX = startX;
    for (const auto &bounds : elementBounds)
    {
        positions.emplace_back(currentX, 0.0f); // Y position will be set separately
        currentX += bounds.size.x + spacing;
    }

    return positions;
}

sf::Vector2f UIPositioningService::positionBelow(const sf::FloatRect &aboveElement,
                                                 const sf::FloatRect &currentElement,
                                                 float spacing)
{
    float x = aboveElement.position.x + (aboveElement.size.x - currentElement.size.x) / 2.0f;
    float y = aboveElement.position.y + aboveElement.size.y + spacing;

    return sf::Vector2f(x, y);
}

// ===== UI LAYOUT SERVICE IMPLEMENTATION =====

UILayoutService::UILayoutService()
    : m_positioningService(nullptr)
{
}

void UILayoutService::initialize(std::shared_ptr<IUIPositioningService> positioningService)
{
    m_positioningService = positioningService;
}

void UILayoutService::centerTitleText(sf::Text &titleText, const sf::Vector2f &windowSize, const sf::Vector2f &offset)
{
    if (m_positioningService)
    {
        m_positioningService->centerText(titleText, windowSize, offset);
    }
    else
    {
        // Fallback to manual calculation
        sf::FloatRect textBounds = titleText.getGlobalBounds();
        float centerX = (windowSize.x - textBounds.size.x) / 2.0f + offset.x;
        float centerY = (windowSize.y - textBounds.size.y) / 2.0f + offset.y;
        titleText.setPosition({centerX, centerY});
    }
}

void UILayoutService::positionMenuElements(const sf::Vector2f &windowSize, const sf::Vector2f &titlePosition, const sf::FloatRect &titleBounds)
{
    // This method can be extended to handle more complex UI layout logic
    // For now, it serves as a placeholder for future UI layout operations
}
