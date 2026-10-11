#pragma once
#include "ArcadeGame.h"
#include "Jello.h"
#include "PipeField.h"
#include "Scene.h"

namespace Flappy
{
    // Jello Bounce: flap through the glossy pipes. Tap to flap, every pipe passed adds a point and passing with the
    // blob near the gap builds a streak. The base class handles the state machine, HUD and overlays.
    class BounceGame : public ArcadeGame
    {
    public:
        explicit BounceGame(const sf::Font &font);

    protected:
        void onStart() override;
        void onBegin() override;
        void onUpdate(float dt) override;
        void onIdle(float dt) override;
        void onKeyPressed(sf::Keyboard::Key key) override;
        void onMousePressed(sf::Vector2f point, sf::Mouse::Button button) override;
        void drawBackground(sf::RenderTarget &target) const override;
        void drawWorld(sf::RenderTarget &target) const override;

        const char *readyTitle() const override { return "GET READY!"; }
        const char *readyKeyLabel() const override { return "SPACE"; }
        const char *readyActionText() const override { return "to bounce!"; }
        bool isStartKey(sf::Keyboard::Key key) const override;
        bool startsOnClick() const override { return true; }

    private:
        void flap();

        Jello m_jello;
        PipeField m_pipes;
        Scene m_scene;
        int m_streak = 0;
        // onBegin() already flapped for the press that started the run; the base then sends that same press to
        // onKeyPressed / onMousePressed, which must not flap a second time.
        bool m_skipStartPress = false;
    };
} // namespace Flappy
