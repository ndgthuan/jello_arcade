#pragma once
#include "../Game/FlappyCommon.h"
#include "../Game/UiKit.h"
#include <string>
#include <vector>

// Small interface widgets for the menu panels. Everything is drawn in the 1280x720 logical space with the
// chunky look of Flappy::UiKit. Rows have a label on the left (first 230 px of `rect`) and the control on the right.
namespace Panels
{
    // The big frosted 3D card every panel sits on, with an extruded title centered at the top.
    // `open` (0..1) animates it in (fade + slight scale from the card center).
    void drawCard(sf::RenderTarget &target, const sf::Font &font, sf::FloatRect rect, const std::string &title,
                  float open = 1.0f);

    // Section heading / body text helpers (left aligned at `pos`, 'pos.y' is the vertical center of the text)
    void drawText(sf::RenderTarget &target, const sf::Font &font, const std::string &text, unsigned int size,
                  sf::Vector2f pos, sf::Color color, bool shadow = true);

    // Horizontal 0..100 slider. Row layout: label | track with knob | value text at the right end.
    class Slider
    {
    public:
        Slider(const sf::Font &font, std::string label, sf::FloatRect rect, int value = 50);
        int value() const { return m_value; }
        void setValue(int value);
        // Return true when the value changed.
        bool handleMouseDown(sf::Vector2f point);
        bool handleMouseMove(sf::Vector2f point);
        void handleMouseUp();
        void draw(sf::RenderTarget &target, const Flappy::Accent &accent) const;

    private:
        const sf::Font *m_font;
        std::string m_label;
        sf::FloatRect m_rect;
        int m_value;
        bool m_dragging = false;
        bool m_hovered = false;
        float trackLeft() const { return m_rect.position.x + 250.0f; }
        float trackRight() const { return m_rect.position.x + m_rect.size.x - 70.0f; }
        int valueAt(float x) const;
    };

    // A row of mutually exclusive options (Easy / Normal / Hard). Row layout: label | option pills.
    class Segmented
    {
    public:
        Segmented(const sf::Font &font, std::string label, sf::FloatRect rect, std::vector<std::string> options,
                  int selected = 0);
        int selected() const { return m_selected; }
        void setSelected(int index);
        bool handleMouseDown(sf::Vector2f point); // true when the selection changed
        void handleMouseMove(sf::Vector2f point);
        void draw(sf::RenderTarget &target, const Flappy::Accent &accent) const;

    private:
        const sf::Font *m_font;
        std::string m_label;
        sf::FloatRect m_rect;
        std::vector<std::string> m_options;
        int m_selected;
        int m_hover = -1;
        sf::FloatRect optionRect(int index) const;
    };

    // A row of colour circles. Row layout: label | circles. The selected circle gets a ring and a check mark.
    class SwatchRow
    {
    public:
        SwatchRow(const sf::Font &font, std::string label, sf::FloatRect rect, std::vector<sf::Color> colors,
                  int selected = 0);
        int selected() const { return m_selected; }
        void setSelected(int index);
        bool handleMouseDown(sf::Vector2f point); // true when the selection changed
        void handleMouseMove(sf::Vector2f point);
        void draw(sf::RenderTarget &target) const;

    private:
        const sf::Font *m_font;
        std::string m_label;
        sf::FloatRect m_rect;
        std::vector<sf::Color> m_colors;
        int m_selected;
        int m_hover = -1;
        sf::Vector2f centerOf(int index) const;
    };

    // On / off switch. Row layout: label | pill switch.
    class Toggle
    {
    public:
        Toggle(const sf::Font &font, std::string label, sf::FloatRect rect, bool on = false);
        bool on() const { return m_on; }
        void setOn(bool on) { m_on = on; }
        bool handleMouseDown(sf::Vector2f point); // true when toggled
        void handleMouseMove(sf::Vector2f point);
        void update(float dt); // animates the knob
        void draw(sf::RenderTarget &target, const Flappy::Accent &accent) const;

    private:
        const sf::Font *m_font;
        std::string m_label;
        sf::FloatRect m_rect;
        bool m_on;
        bool m_hovered = false;
        float m_anim; // 0..1 knob position
    };

    // Single line text input. Accepts printable ASCII (32..126) up to maxLength characters.
    class TextField
    {
    public:
        TextField(const sf::Font &font, std::string placeholder, sf::FloatRect rect, std::size_t maxLength = 12);
        const std::string &text() const { return m_text; }
        void clear() { m_text.clear(); }
        void setFocused(bool focused) { m_focused = focused; }
        bool focused() const { return m_focused; }
        void handleMouseDown(sf::Vector2f point); // focuses when the click is inside, unfocuses otherwise
        void handleText(char32_t unicode);        // TextEntered event
        void handleKey(sf::Keyboard::Key key);    // Backspace edits, Enter sets the submit flag
        bool consumeSubmit();                     // true once after Enter was pressed
        void update(float dt);                    // caret blink
        void draw(sf::RenderTarget &target, const Flappy::Accent &accent) const;

    private:
        const sf::Font *m_font;
        std::string m_placeholder;
        sf::FloatRect m_rect;
        std::size_t m_maxLength;
        std::string m_text;
        bool m_focused = false;
        bool m_submit = false;
        float m_time = 0.0f;
    };
} // namespace Panels
