#include "CameraManager.h"
#include <algorithm>

CameraManager::CameraManager() : m_cameraCenter(0, 0), m_lastMousePos(0, 0), m_mouseVelocity(0, 0)
{
    // Initialize camera center and mouse tracking variables
}

void CameraManager::initialize(const sf::RenderWindow &window, std::shared_ptr<IGameConfigurationService> configService)
{
    // Initialize camera with default window view
    m_camera = window.getDefaultView();
    m_cameraCenter = m_camera.getCenter();

    // Store configuration service
    m_configService = configService;
}

void CameraManager::updateCamera(const sf::FloatRect &backgroundBounds, const sf::Vector2f &mousePos, const sf::Vector2f &windowSize)
{
    // Remember where the camera may go so momentum cannot push the view past the background
    m_panLimits = sf::FloatRect(
        {backgroundBounds.position.x + windowSize.x / 2.0f, backgroundBounds.position.y + windowSize.y / 2.0f},
        {std::max(0.0f, backgroundBounds.size.x - windowSize.x), std::max(0.0f, backgroundBounds.size.y - windowSize.y)});
    m_hasPanLimits = true;

    // Calculate window center for coordinate calculations
    sf::Vector2f windowCenter = windowSize / 2.0f;

    // Calculate mouse velocity for smooth movement
    if (m_lastMousePos != sf::Vector2f(0, 0))
    {
        m_mouseVelocity = mousePos - m_lastMousePos;
    }
    m_lastMousePos = mousePos;

    // Calculate panning ratio based on background and screen width
    float backgroundWidth = backgroundBounds.size.x;
    float screenWidth = windowSize.x;

    // Only pan when background is wider than screen
    if (backgroundWidth > screenWidth)
    {
        float normalizedMouseX = (mousePos.x - windowCenter.x) / (screenWidth / 2.0f);
        float targetCameraX = backgroundBounds.position.x + (backgroundWidth / 2.0f) + (normalizedMouseX * (backgroundWidth - screenWidth) / 2.0f);

        float smoothSpeed = m_configService ? m_configService->getCameraSmoothSpeed() : 0.2f;
        m_cameraCenter.x += (targetCameraX - m_cameraCenter.x) * smoothSpeed;
    }
    else
    {
        // Limited horizontal pan for smaller backgrounds
        float maxHorizontalPan = 100.0f;
        float normalizedMouseX = (mousePos.x - windowCenter.x) / (screenWidth / 2.0f);
        float targetCameraX = backgroundBounds.position.x + (backgroundBounds.size.x / 2.0f) + (normalizedMouseX * maxHorizontalPan);

        float smoothSpeed = m_configService ? m_configService->getCameraSmoothSpeed() : 0.2f;
        m_cameraCenter.x += (targetCameraX - m_cameraCenter.x) * smoothSpeed;
    }

    // Vertical panning (Y-axis)
    if (backgroundBounds.size.y > windowSize.y)
    {
        float normalizedMouseY = (mousePos.y - windowCenter.y) / (windowSize.y / 2.0f);
        float targetCameraY = backgroundBounds.position.y + (backgroundBounds.size.y / 2.0f) + (normalizedMouseY * (backgroundBounds.size.y - windowSize.y) / 2.0f);

        float smoothSpeed = m_configService ? m_configService->getCameraSmoothSpeed() : 0.2f;
        m_cameraCenter.y += (targetCameraY - m_cameraCenter.y) * smoothSpeed;
    }
    else
    {
        // Limited vertical pan for smaller backgrounds
        float maxVerticalPan = 100.0f;
        float normalizedMouseY = (mousePos.y - windowCenter.y) / (windowSize.y / 2.0f);
        float targetCameraY = backgroundBounds.position.y + (backgroundBounds.size.y / 2.0f) + (normalizedMouseY * maxVerticalPan);

        float smoothSpeed = m_configService ? m_configService->getCameraSmoothSpeed() : 0.2f;
        m_cameraCenter.y += (targetCameraY - m_cameraCenter.y) * smoothSpeed;
    }

    updateCamera();
}

void CameraManager::updateCameraSmooth()
{
    // Smooth camera movement with momentum
    float momentum = m_configService ? m_configService->getCameraMomentum() : 0.95f;
    m_mouseVelocity *= momentum;

    // Apply momentum to camera movement
    float momentumMultiplier = m_configService ? m_configService->getCameraMomentumMultiplier() : 0.02f;
    m_cameraCenter.x += m_mouseVelocity.x * momentumMultiplier;
    m_cameraCenter.y += m_mouseVelocity.y * momentumMultiplier;

    updateCamera();
}

void CameraManager::setView(sf::RenderWindow &window)
{
    // Apply camera view to the window
    window.setView(m_camera);
}

void CameraManager::updateCamera()
{
    // Keep the view inside the background when it is larger than the window
    if (m_hasPanLimits)
    {
        if (m_panLimits.size.x > 0.0f)
            m_cameraCenter.x = std::clamp(m_cameraCenter.x, m_panLimits.position.x, m_panLimits.position.x + m_panLimits.size.x);
        if (m_panLimits.size.y > 0.0f)
            m_cameraCenter.y = std::clamp(m_cameraCenter.y, m_panLimits.position.y, m_panLimits.position.y + m_panLimits.size.y);
    }

    // Update camera center position
    m_camera.setCenter(m_cameraCenter);
}
