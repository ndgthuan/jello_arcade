#include "BounceGame.h"

#include <cmath>

namespace Flappy
{
    BounceGame::BounceGame(const sf::Font &font) : ArcadeGame(font, 0, 0)
    {
        onStart();
    }

    void BounceGame::onStart()
    {
        m_jello.setAccent(accent());
        m_jello.reset();
        m_pipes.setDifficulty(difficulty());
        m_pipes.reset();
        m_pipes.setScrolling(true);
        m_streak = 0;
        m_skipStartPress = false;
        setCombo(0);
        setComboWord("STREAK");
        setLives(0, 0);
    }

    void BounceGame::onBegin()
    {
        flap();
        m_skipStartPress = true;
    }

    bool BounceGame::isStartKey(sf::Keyboard::Key key) const
    {
        return key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W;
    }

    void BounceGame::onKeyPressed(sf::Keyboard::Key key)
    {
        if (m_skipStartPress)
        {
            m_skipStartPress = false;
            return;
        }
        if (isStartKey(key))
        {
            flap();
        }
    }

    void BounceGame::onMousePressed(sf::Vector2f point, sf::Mouse::Button button)
    {
        (void)point;
        if (m_skipStartPress)
        {
            m_skipStartPress = false;
            return;
        }
        if (button == sf::Mouse::Button::Left)
        {
            flap();
        }
    }

    void BounceGame::onIdle(float dt)
    {
        m_jello.update(dt, false);
        m_scene.update(dt, m_jello.isDead() ? 0.0f : m_pipes.speed() * 0.25f);
    }

    void BounceGame::onUpdate(float dt)
    {
        m_scene.update(dt, m_pipes.speed());
        m_jello.update(dt, true);
        m_pipes.update(dt);

        fx().spawnTrail(m_jello.position(), dt);
        if (m_jello.consumeFlapEvent())
        {
            fx().spawnFlapBurst(m_jello.position());
        }

        // Each newly passed pipe adds a point; passing with the blob near the gap keeps the streak
        const int passed = m_pipes.collectScore(kPlayerX);
        for (int i = 0; i < passed; ++i)
        {
            addScore(1);
            const sf::Vector2f gap = m_pipes.lastScoredGapCenter();
            const bool perfect = std::abs(gap.y - m_jello.position().y) < 45.0f;
            m_streak = perfect ? m_streak + 1 : 0;
            setCombo(m_streak);
            fx().spawnScore(gap, 1);
            audio().playScore(m_streak);
        }

        // Pipes, the ground or flying off the top of the screen
        const sf::FloatRect hitbox = m_jello.hitbox();
        if (m_pipes.collides(hitbox) || hitbox.position.y + hitbox.size.y >= kGroundY ||
            m_jello.position().y < -40.0f)
        {
            m_jello.kill();
            m_pipes.setScrolling(false);
            gameOver(m_jello.position(), 1.0f);
        }
    }

    void BounceGame::flap()
    {
        m_jello.flap();
        audio().playFlap();
    }

    void BounceGame::drawBackground(sf::RenderTarget &target) const
    {
        m_scene.draw(target);
    }

    void BounceGame::drawWorld(sf::RenderTarget &target) const
    {
        m_pipes.draw(target);
        m_jello.drawShadow(target);
        m_jello.drawTrail(target);
        m_jello.draw(target);
        m_scene.drawVignette(target, 0.9f);
    }
} // namespace Flappy
