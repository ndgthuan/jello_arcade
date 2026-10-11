#pragma once
#include <SFML/Graphics.hpp>
#include <functional>

namespace Flappy
{
    // Every game in the menu implements this; GameWindow only talks to it.
    class IMiniGame
    {
    public:
        virtual ~IMiniGame() = default;

        // Called each time the game is opened from the menu: back to the Ready screen with fresh settings.
        virtual void start() = 0;
        virtual void handleEvent(const sf::Event &event, sf::RenderWindow &window) = 0;
        virtual void update(float dt) = 0;
        virtual void draw(sf::RenderWindow &window) = 0;
        // Called when the player chooses "Main menu" or presses Esc on the Ready / Game over screen.
        virtual void setExitToMenuCallback(std::function<void()> callback) = 0;
    };

    // Catalog shown by the game picker (index = game id, same as the UserData best-score index)
    struct GameInfo
    {
        const char *name;
        const char *tagline;
    };

    inline const GameInfo &gameInfo(int id)
    {
        static const GameInfo kGames[5] = {
            {"Jello Bounce", "Flap through the glossy pipes"},
            {"Jello Smash", "Break the candy wall"},
            {"Jello Blaster", "Shoot the asteroid storm"},
            {"Jello Slither", "Eat, grow, do not crash"},
            {"Jello Stack", "Drop blocks, build the tower"}};
        return kGames[id < 0 ? 0 : (id > 4 ? 4 : id)];
    }
} // namespace Flappy
