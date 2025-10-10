#pragma once
#include <SFML/Graphics.hpp>

/**
 * UI Configuration class that centralizes all UI-related constants
 * Follows Single Responsibility Principle - only handles UI configuration
 * Eliminates hard-coded values throughout the codebase
 */
class UIConfig
{
public:
    // Positioning constants
    static constexpr float TITLE_CENTER_OFFSET_X = -13.0f;
    static constexpr float TITLE_CENTER_OFFSET_Y = -187.0f;
    static constexpr float TITLE_CIRCLE_SPACING = 104.0f;
    static constexpr float LABEL_OFFSET_X = 10.0f; // IMPORTANT: This +10.f offset is always applied
    static constexpr float LABEL_OFFSET_Y = 20.0f;

    // Circle menu constants
    static constexpr float DEFAULT_CIRCLE_RADIUS = 50.0f;
    static constexpr float DEFAULT_CIRCLE_SPACING = 30.0f;
    static constexpr float CIRCLE_OUTLINE_THICKNESS = 1.0f;

    // Button constants
    static constexpr float BUTTON_PROGRESS_BAR_HEIGHT = 0.5f;
    static constexpr float BUTTON_ANIMATION_SPEED = 8.0f;
    static constexpr float BUTTON_LABEL_SPACING = 21.0f;

    // Title text constants
    static constexpr unsigned int TITLE_FONT_SIZE = 175;
    static constexpr float TITLE_GLOW_ANIMATION_SPEED = 2.5f;
    static constexpr float TITLE_GLOW_INTENSITY_MIN = 0.6f;
    static constexpr float TITLE_GLOW_INTENSITY_MAX = 1.0f;
    static constexpr int TITLE_GLOW_LAYERS = 3;
    static constexpr float TITLE_GLOW_SCALE_INCREMENT = 0.015f;
    static constexpr float TITLE_GLOW_ALPHA_BASE = 25.0f;
    static constexpr float TITLE_GLOW_ALPHA_DECREMENT = 6.0f;

    // Colors
    static const sf::Color TITLE_COLOR;
    static const sf::Color TITLE_GLOW_COLOR;
    static const sf::Color BUTTON_NORMAL_COLOR;
    static const sf::Color BUTTON_HOVER_COLOR;
    static const sf::Color CIRCLE_COLOR;
    static const sf::Color CIRCLE_OUTLINE_COLOR;
    static const sf::Color LABEL_COLOR;

    // Icon scale
    static constexpr float ICON_SCALE = 0.1f;
    static constexpr float ICON_HOVER_SCALE = 1.2f;
    static constexpr float ICON_ANIMATION_SPEED = 8.0f;

    // Account icon specific offsets
    static constexpr float ACCOUNT_ICON_OFFSET_X = 5.0f;
    static constexpr float ACCOUNT_ICON_OFFSET_Y = -3.0f;

    // Game system constants (moved from magic numbers)
    static constexpr int DEFAULT_PARTICLE_COUNT = 400;
    static constexpr float CAMERA_SMOOTH_SPEED = 0.2f;
    static constexpr float CAMERA_MOMENTUM = 0.95f;
    static constexpr float CAMERA_MOMENTUM_MULTIPLIER = 0.02f;
};
