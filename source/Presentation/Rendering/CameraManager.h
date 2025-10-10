#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "../UI/UIServices.h"

/**
 * Camera manager class that handles view transformations and camera movement
 * Provides smooth camera panning with momentum and boundary awareness
 * Supports both automatic and manual camera control
 * Follows Dependency Inversion Principle by depending on IConfigurationService
 */
class CameraManager
{
public:
    // Constructor - initializes camera state
    CameraManager();

    // Initialize camera with window reference and configuration service
    void initialize(const sf::RenderWindow &window, std::shared_ptr<IGameConfigurationService> configService);
    // Update camera position based on mouse movement and background bounds
    void updateCamera(const sf::FloatRect &backgroundBounds, const sf::Vector2f &mousePos, const sf::Vector2f &windowSize);
    // Apply smooth camera movement with momentum
    void updateCameraSmooth();
    // Apply current camera view to the window
    void setView(sf::RenderWindow &window);

    // Getter methods for camera state
    const sf::View &getCamera() const { return m_camera; }
    const sf::Vector2f &getCameraCenter() const { return m_cameraCenter; }

private:
    // Helper methods for camera management
    void updateCamera(); // Update camera view with current center

    // Camera state variables
    sf::View m_camera;            // Current camera view
    sf::Vector2f m_cameraCenter;  // Camera center position
    sf::Vector2f m_lastMousePos;  // Previous mouse position for velocity calculation
    sf::Vector2f m_mouseVelocity; // Mouse movement velocity for momentum

    // Configuration service - follows DIP
    std::shared_ptr<IGameConfigurationService> m_configService;
};
