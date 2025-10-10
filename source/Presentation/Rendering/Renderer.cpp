#include "Renderer.h"
#include "ParticleSystem.h"
#include "BackgroundManager.h"

Renderer::Renderer() : m_backgroundManager(nullptr)
{
    // Initialize background manager pointer to null
}

void Renderer::setBackground(std::shared_ptr<BackgroundManager> backgroundManager)
{
    // Set the background manager for rendering
    m_backgroundManager = backgroundManager;
}

void Renderer::loadBackground(const std::string &textureId)
{
    if (m_backgroundManager)
    {
        m_backgroundManager->loadBackground(textureId);
    }
}

void Renderer::setBackgroundScale(float scaleX, float scaleY)
{
    if (m_backgroundManager)
    {
        m_backgroundManager->setBackgroundScale(scaleX, scaleY);
    }
}

void Renderer::setBackgroundPosition(float x, float y)
{
    if (m_backgroundManager)
    {
        m_backgroundManager->setBackgroundPosition(x, y);
    }
}

void Renderer::centerBackground(const sf::Vector2f &windowSize)
{
    if (m_backgroundManager)
    {
        m_backgroundManager->centerBackground(windowSize);
    }
}

void Renderer::clear(sf::RenderWindow &window, const sf::Color &color)
{
    // Clear the window with specified background color
    window.clear(color);
}

void Renderer::drawBackground(sf::RenderWindow &window)
{
    // Draw the background using background manager if it exists
    if (m_backgroundManager)
    {
        m_backgroundManager->draw(window);
    }
}

void Renderer::drawParticles(sf::RenderWindow &window, ParticleSystem &particleSystem)
{
    // Delegate particle rendering to the particle system
    particleSystem.draw(window);
}

void Renderer::setView(sf::RenderWindow &window, const sf::View &view)
{
    // Set the rendering view for the window
    window.setView(view);
}

void Renderer::resetView(sf::RenderWindow &window)
{
    // Reset to the default window view
    window.setView(window.getDefaultView());
}

void Renderer::display(sf::RenderWindow &window)
{
    // Display the rendered content to the screen
    window.display();
}

sf::FloatRect Renderer::getBounds() const
{
    if (m_backgroundManager)
    {
        return m_backgroundManager->getBounds();
    }
    return sf::FloatRect();
}

bool Renderer::isInitialized() const
{
    if (m_backgroundManager)
    {
        return m_backgroundManager->isInitialized();
    }
    return false;
}
