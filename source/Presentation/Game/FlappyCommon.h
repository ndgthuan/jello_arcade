#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdint>

// Shared constants and small helpers for the "Jello Bounce" (flappy-style) game.
// The game is drawn in a fixed logical resolution of 1280x720; ArcadeGame maps it to the window.
namespace Flappy
{
    inline constexpr float kWidth = 1280.0f;
    inline constexpr float kHeight = 720.0f;
    inline constexpr float kHorizonY = 470.0f; // where the sky meets the neon floor
    inline constexpr float kGroundY = 640.0f;  // pipes stand here; touching it is fatal

    // Player physics (pixels / second)
    inline constexpr float kGravity = 1750.0f;
    inline constexpr float kFlapSpeed = -570.0f;
    inline constexpr float kMaxFallSpeed = 950.0f;
    inline constexpr float kPlayerX = 360.0f;
    inline constexpr float kPlayerHalfW = 34.0f; // hitbox half size (a bit smaller than the sprite)
    inline constexpr float kPlayerHalfH = 29.0f;

    // Pipes
    inline constexpr float kPipeWidth = 112.0f;
    inline constexpr float kRimWidth = 136.0f;
    inline constexpr float kRimHeight = 46.0f;
    inline constexpr float kGapSize = 240.0f;     // vertical opening between top and bottom rim
    inline constexpr float kGapMargin = 90.0f;    // min distance from gap edge to ceiling / ground
    inline constexpr float kPipeSpeed = 240.0f;   // horizontal scroll speed
    inline constexpr float kPipeSpacing = 360.0f; // distance between two consecutive pipes (x to x)

    enum class State
    {
        Ready,
        Playing,
        Paused,
        GameOver
    };

    // What an overlay asks the game to do after a click / key press
    enum class Action
    {
        None,
        Resume,
        Restart,
        Menu
    };

    inline sf::Color withAlpha(sf::Color c, std::uint8_t a)
    {
        c.a = a;
        return c;
    }

    // Linear interpolation between two colors (t in 0..1), alpha included
    inline sf::Color mix(sf::Color a, sf::Color b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        auto m = [t](std::uint8_t x, std::uint8_t y)
        { return static_cast<std::uint8_t>(x + (y - x) * t + 0.5f); };
        return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
    }

    // Jello color family derived from one accent color (matches the design mockup)
    struct Accent
    {
        sf::Color base = sf::Color(124, 255, 91);
        sf::Color light = sf::Color(124, 255, 91);
        sf::Color dark = sf::Color(124, 255, 91);
        sf::Color glow = sf::Color(124, 255, 91, 140);

        static Accent from(sf::Color c)
        {
            Accent a;
            a.base = c;
            a.light = mix(c, sf::Color::White, 0.6f);
            a.dark = mix(c, sf::Color(20, 10, 60), 0.62f);
            a.glow = withAlpha(c, 140);
            return a;
        }
    };

    // The four jello colours the player can pick in Settings (index = UserData settings accentIndex)
    inline sf::Color accentChoice(int index)
    {
        switch (std::clamp(index, 0, 3))
        {
        case 1:
            return sf::Color(255, 90, 143); // pink
        case 2:
            return sf::Color(255, 210, 63); // gold
        case 3:
            return sf::Color(79, 216, 255); // sky blue
        default:
            return sf::Color(124, 255, 91); // lime
        }
    }

    // Difficulty (0 easy, 1 normal, 2 hard): vertical opening and scroll speed
    struct DifficultyParams
    {
        float gap;
        float speed;
        const char *name;
    };

    inline DifficultyParams difficultyParams(int difficulty)
    {
        switch (std::clamp(difficulty, 0, 2))
        {
        case 0:
            return {270.0f, 215.0f, "Easy"};
        case 2:
            return {210.0f, 270.0f, "Hard"};
        default:
            return {kGapSize, kPipeSpeed, "Normal"};
        }
    }
} // namespace Flappy
