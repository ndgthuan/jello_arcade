#pragma once
#include "FlappyCommon.h"

namespace Flappy
{
    // The jello mascot that lives on the main menu: it sits on the neon floor at the bottom left, breathes, blinks,
    // follows the mouse with its eyes, squishes happily when the mouse comes close and does a big bounce when the
    // menu asks for it (pulse()). It is drawn in WINDOW PIXEL coordinates (the default view), scaled with the window
    // height, so it works at any window size. Colours come from UserData::instance().settings().accentIndex.
    class MenuMascot
    {
    public:
        MenuMascot();

        // mousePixel: the cursor in window pixels. windowSize in pixels.
        void update(float dt, sf::Vector2f mousePixel, sf::Vector2u windowSize);

        // A little bounce with a squash-and-stretch (called when a menu icon is clicked).
        void pulse();

        // Draw order: after the background and the menu, before panels.
        void draw(sf::RenderTarget &target, sf::Vector2u windowSize) const;

    private:
        float m_time = 0.0f;
        float m_blinkTimer = 2.0f;
        float m_blink = 0.0f;       // 0 open .. 1 closed
        float m_hop = 0.0f;         // current hop height in "mascot units"
        float m_hopVel = 0.0f;
        float m_squash = 0.0f;      // spring: + tall, - flat
        float m_squashVel = 0.0f;
        float m_happy = 0.0f;       // 0..1 smoothed "mouse is near"
        sf::Vector2f m_look{0.0f, 0.0f}; // smoothed eye direction (unit-ish vector toward the mouse)
        sf::Vector2f m_center{0.0f, 0.0f}; // last drawn center in pixels (for the look direction)
        float m_scale = 1.0f;
    };
} // namespace Flappy
