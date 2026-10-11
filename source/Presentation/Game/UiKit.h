#pragma once
#include "FlappyCommon.h"
#include <string>
#include <vector>

// Drawing helpers that give flat SFML shapes a chunky, "3D" look. All coordinates are logical (1280x720).
namespace Flappy::UiKit
{
    // Text with a stacked extrusion (classic 3D lettering): `layers` are drawn from the farthest
    // (darkest) to the nearest, each shifted `step` pixels further down; the face color is drawn last.
    struct ExtrudeStyle
    {
        sf::Color face = sf::Color::White;
        std::vector<sf::Color> layers;          // nearest -> farthest
        float step = 3.0f;                      // vertical offset between layers (before scaling)
        sf::Color glow = sf::Color(0, 0, 0, 0); // soft halo behind the text (alpha 0 = none)
        float shadowAlpha = 0.55f;              // drop shadow strength below the stack (0 = none)
    };

    // Ready-made styles used by the design
    ExtrudeStyle orangeStyle(); // score numbers: white face, yellow -> orange -> brown extrusion
    ExtrudeStyle pinkStyle();   // "GET READY!"
    ExtrudeStyle redStyle();    // "GAME OVER"
    ExtrudeStyle blueStyle();   // "PAUSED"
    ExtrudeStyle goldStyle();   // small gold labels

    // Draws `text` centered on `center`. `scale` scales the whole stack; rotationDeg rotates around center;
    // `alpha` (0..255) multiplies every layer's alpha (for fades).
    void drawExtrudedText(sf::RenderTarget &target, const sf::Font &font, const std::string &text,
                          unsigned int characterSize, sf::Vector2f center, const ExtrudeStyle &style,
                          float scale = 1.0f, float rotationDeg = 0.0f, std::uint8_t alpha = 255);

    // Plain centered text with an optional soft drop shadow (used for small labels)
    void drawLabel(sf::RenderTarget &target, const sf::Font &font, const std::string &text,
                   unsigned int characterSize, sf::Vector2f center, sf::Color color,
                   bool shadow = true, float letterSpacing = 1.0f);

    // Rounded rectangle, solid fill with optional outline
    void drawRoundedRect(sf::RenderTarget &target, sf::FloatRect rect, float radius, sf::Color fill,
                         sf::Color outline = sf::Color::Transparent, float outlineThickness = 0.0f);

    // Rounded rectangle with a vertical gradient (top -> bottom)
    void drawRoundedRectGradient(sf::RenderTarget &target, sf::FloatRect rect, float radius,
                                 sf::Color top, sf::Color bottom);

    // Radial glow: `color` at the center fading to alpha 0 at `radius`
    void drawGlow(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color);

    // Ellipse filled with a radial gradient whose bright spot is offset toward the top-left
    // (this is how the jello body is shaded): highlight -> mid -> edge.
    void drawShadedEllipse(sf::RenderTarget &target, sf::Vector2f center, sf::Vector2f radii,
                           sf::Color highlight, sf::Color mid, sf::Color edge, float rotationDeg = 0.0f);

    // Spiky explosion shape: `spikes` points alternating between outerRadius and innerRadius,
    // gradient from colorInner (center) to colorOuter (tips). Deterministic jitter from `seed`.
    void drawStarburst(sf::RenderTarget &target, sf::Vector2f center, float outerRadius, float innerRadius,
                       int spikes, float rotationDeg, sf::Color colorInner, sf::Color colorOuter,
                       unsigned int seed = 1);

    // Thin ring (CircleShape with outline)
    void drawRing(sf::RenderTarget &target, sf::Vector2f center, float radius, float thickness, sf::Color color);

    // Four-point sparkle star
    void drawSparkle(sf::RenderTarget &target, sf::Vector2f center, float size, sf::Color color, float rotationDeg = 0.0f);

    // Rays radiating from `center` (alternating bright / transparent wedges), faded toward the edge
    void drawSunburstRays(sf::RenderTarget &target, sf::Vector2f center, float radius, int rayCount,
                          float rotationDeg, sf::Color color);

    // ---- Chunky 3D button ----
    struct Button3DStyle
    {
        sf::Color top;    // face gradient top
        sf::Color bottom; // face gradient bottom
        sf::Color edge;   // the "thickness" under the face and the border
        sf::Color text;
        sf::Color glow = sf::Color(0, 0, 0, 0); // alpha 0 = no glow
        float depth = 10.0f;                    // thickness in pixels
        float radius = 26.0f;                   // corner radius

        static Button3DStyle primary(const Accent &accent); // accent-colored candy button, dark text
        static Button3DStyle secondary();                   // white / light-blue button, navy text
    };

    class Button3D
    {
    public:
        Button3D(const sf::Font &font, std::string label, sf::FloatRect rect, Button3DStyle style,
                 unsigned int characterSize = 30);

        void setRect(sf::FloatRect rect) { m_rect = rect; }
        sf::FloatRect rect() const { return m_rect; }
        bool contains(sf::Vector2f point) const { return m_rect.contains(point); }

        void setHovered(bool hovered) { m_hovered = hovered; }
        void setPressed(bool pressed) { m_pressed = pressed; }
        void setLabel(std::string label) { m_label = std::move(label); }
        void setIconHome(bool on) { m_iconHome = on; } // draw a house icon instead of the label

        void update(float dt); // animates the hover lift / press sink
        void draw(sf::RenderTarget &target) const;

    private:
        const sf::Font *m_font;
        std::string m_label;
        sf::FloatRect m_rect;
        Button3DStyle m_style;
        unsigned int m_characterSize;
        bool m_hovered = false;
        bool m_pressed = false;
        bool m_iconHome = false;
        float m_lift = 0.0f; // 0..1 smoothed hover amount
    };
} // namespace Flappy::UiKit
