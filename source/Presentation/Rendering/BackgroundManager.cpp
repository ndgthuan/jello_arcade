#include "BackgroundManager.h"
#include "../Game/Scene.h"
#include "../UI/UIServices.h"
#include "../../Data/Logging/Logger.h"

BackgroundManager::BackgroundManager() : m_isInitialized(false), m_resourceService(nullptr)
{
}

BackgroundManager::BackgroundManager(std::shared_ptr<IResourceService> resourceService)
    : m_isInitialized(false), m_resourceService(resourceService)
{
}

BackgroundManager::BackgroundManager(const BackgroundManager &other)
    : m_generatedTexture(other.m_generatedTexture), m_cover(other.m_cover), m_isInitialized(other.m_isInitialized),
      m_textureId(other.m_textureId), m_resourceService(other.m_resourceService)
{
    if (other.m_background)
    {
        m_background = std::make_unique<sf::Sprite>(*other.m_background);
    }
}

void BackgroundManager::loadBackground(const std::string &textureId)
{
    m_textureId = textureId;

    try
    {
        if (!m_resourceService)
        {
            // Fallback to creating ResourceService if not injected
            m_resourceService = std::make_shared<ResourceService>();
        }

        // Get texture from resource service
        const sf::Texture &bgTex = m_resourceService->getTexture(textureId);

        // Create background sprite
        m_background = std::make_unique<sf::Sprite>(bgTex);
        m_isInitialized = true;

        // Set default scale
        setBackgroundScale(1.16883f, 1.3f);
    }
    catch (const std::exception &e)
    {
        // Handle texture loading error
        m_isInitialized = false;
        Logger::getInstance().error("Failed to load background texture: " + textureId + " - " + e.what());
    }
}

void BackgroundManager::generateTwilight()
{
    // Draw the game scene once at 1.8x its logical size (2304x1296) into a texture
    constexpr unsigned int kWidth = 2304;
    constexpr unsigned int kHeight = 1296;
    sf::RenderTexture target(sf::Vector2u{kWidth, kHeight});
    target.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, {Flappy::kWidth, Flappy::kHeight})));
    target.clear(sf::Color(10, 15, 60));

    Flappy::Scene scene;
    scene.draw(target);
    scene.drawVignette(target, 0.7f);
    target.display();

    m_generatedTexture = std::make_shared<sf::Texture>(target.getTexture());
    m_generatedTexture->setSmooth(true);
    m_background = std::make_unique<sf::Sprite>(*m_generatedTexture);
    m_cover = true;
    m_isInitialized = true;
}

void BackgroundManager::setBackgroundScale(float scaleX, float scaleY)
{
    if (m_background)
    {
        m_background->setScale(sf::Vector2f(scaleX, scaleY));
    }
}

void BackgroundManager::setBackgroundPosition(float x, float y)
{
    if (m_background)
    {
        m_background->setPosition(sf::Vector2f(x, y));
    }
}

void BackgroundManager::centerBackground(const sf::Vector2f &windowSize)
{
    if (!m_background)
        return;

    // Generated background: scale (uniformly) to cover the window with a 20% margin for camera panning
    if (m_cover)
    {
        const sf::Vector2u size = m_background->getTexture().getSize();
        const float s = std::max(windowSize.x * 1.2f / static_cast<float>(size.x), windowSize.y * 1.2f / static_cast<float>(size.y));
        m_background->setScale(sf::Vector2f(s, s));
    }

    // Get texture dimensions
    sf::Vector2u texSize = m_background->getTexture().getSize();

    // Get current scale
    sf::Vector2f scale = m_background->getScale();

    // Calculate scaled dimensions
    float scaledWidth = texSize.x * scale.x;
    float scaledHeight = texSize.y * scale.y;

    // Center background on screen
    float posY = (windowSize.y - scaledHeight) / 2.0f;
    float posX = (windowSize.x - scaledWidth) / 2.0f;

    m_background->setPosition(sf::Vector2f(posX, posY));
}

void BackgroundManager::draw(sf::RenderWindow &window)
{
    if (m_background && m_isInitialized)
    {
        window.draw(*m_background);
    }
}

void BackgroundManager::setResourceService(std::shared_ptr<IResourceService> resourceService)
{
    m_resourceService = resourceService;
}

sf::FloatRect BackgroundManager::getBounds() const
{
    if (!m_background || !m_isInitialized)
        return sf::FloatRect();
    return m_background->getGlobalBounds();
}
