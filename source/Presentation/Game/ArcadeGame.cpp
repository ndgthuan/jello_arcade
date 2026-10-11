#include "ArcadeGame.h"
#include "../../Data/Profile/Achievements.h"
#include <algorithm>
#include <cmath>

namespace Flappy
{
    namespace
    {
        // Stick / POV deflection (out of 100) below which a direction counts as released
        constexpr float kPadDeadZone = 50.0f;
    }

    ArcadeGame::ArcadeGame(const sf::Font &font, int gameId, int musicTrack)
        : m_font(&font),
          m_gameId(gameId),
          m_musicTrack(musicTrack),
          m_accent(Accent::from(accentChoice(UserData::instance().settings().accentIndex))),
          m_effects(font, m_accent),
          m_hud(font),
          m_ready(font),
          m_pause(font, m_accent),
          m_gameOver(font, m_accent),
          m_toast(font),
          m_view(sf::FloatRect({0.0f, 0.0f}, {kWidth, kHeight}))
    {
    }

    void ArcadeGame::applySettings()
    {
        const UserData &data = UserData::instance();
        const Settings &settings = data.settings();

        m_difficulty = settings.difficulty;
        m_best = data.best(m_gameId, m_difficulty);

        m_accent = Accent::from(accentChoice(settings.accentIndex));
        m_effects.setAccent(m_accent);
        m_pause = PauseOverlay(*m_font, m_accent);
        m_gameOver = GameOverOverlay(*m_font, m_accent);

        m_audio.setVolumes(settings.musicVolume, settings.sfxVolume, settings.muted);
        m_hud.setPlayerName(data.active().name);
    }

    void ArcadeGame::start()
    {
        applySettings();

        m_state = State::Ready;
        m_score = 0;
        m_newBest = false;
        m_gameOverTimer = 0.0f;
        m_overlayFade = 0.0f;
        m_readyFade = 1.0f;
        m_runTime = 0.0f;
        m_clock = 0.0f;
        m_fanfareTimer = -1.0f;

        m_effects.clear();
        m_ready.setTexts(readyTitle(), readyKeyLabel(), readyActionText());

        m_hud.setScore(0);
        m_hud.setBest(m_best);
        m_hud.setStreak(0);
        m_hud.setStreakWord("STREAK");
        m_hud.setLives(0, 0);
        m_hud.setPauseHovered(false);

        m_audio.setMusicDucked(false);
        m_audio.startMusic(m_musicTrack);

        // The game sets its own lives / combo word in here
        onStart();
    }

    void ArcadeGame::handleEvent(const sf::Event &event, sf::RenderWindow &window)
    {
        applyViewport(window);
        window.setKeyRepeatEnabled(false);

        // Losing focus pauses a running game; there is no FocusGained handler, so it never auto-resumes
        if (event.is<sf::Event::FocusLost>())
        {
            if (m_state == State::Playing)
            {
                pauseGame();
            }
            return;
        }

        const auto *keyPress = event.getIf<sf::Event::KeyPressed>();
        const auto *keyUp = event.getIf<sf::Event::KeyReleased>();
        const auto *press = event.getIf<sf::Event::MouseButtonPressed>();
        const auto *release = event.getIf<sf::Event::MouseButtonReleased>();
        const auto *move = event.getIf<sf::Event::MouseMoved>();
        const auto *padPress = event.getIf<sf::Event::JoystickButtonPressed>();

        // M mutes / unmutes everything, in any state
        if (keyPress && keyPress->code == sf::Keyboard::Key::M)
        {
            m_audio.toggleMute();
            UserData::instance().settings().muted = m_audio.isMuted();
            UserData::instance().commitSettings();
            return;
        }

        if (move)
        {
            m_mouse = mouseToLogical(window, move->position);
        }

        const bool escKey = keyPress && keyPress->code == sf::Keyboard::Key::Escape;
        const bool pauseKey = keyPress && keyPress->code == sf::Keyboard::Key::P;
        const bool leftPress = press && press->button == sf::Mouse::Button::Left;
        const bool leftRelease = release && release->button == sf::Mouse::Button::Left;
        const bool confirmKey = keyPress && (keyPress->code == sf::Keyboard::Key::Space ||
                                             keyPress->code == sf::Keyboard::Key::Enter);

        // First gamepad only: A = Space, B = Escape, Start = pause / resume
        const bool padOwn = padPress && padPress->joystickId == 0;
        const bool padA = padOwn && padPress->button == 0;
        const bool padB = padOwn && padPress->button == 1;
        const bool padStart = padOwn && padPress->button == 7;

        auto runAction = [this](Action action)
        {
            if (action == Action::None)
            {
                return;
            }
            m_audio.playClick();
            switch (action)
            {
            case Action::Resume:
                resumeGame();
                break;
            case Action::Restart:
                start();
                break;
            case Action::Menu:
                exitToMenu();
                break;
            case Action::None:
                break;
            }
        };

        switch (m_state)
        {
        case State::Ready:
            if (move)
            {
                onMouseMoved(m_mouse);
            }
            else if (keyPress && isStartKey(keyPress->code))
            {
                beginPlaying();
                onKeyPressed(keyPress->code);
            }
            else if (leftPress && startsOnClick())
            {
                beginPlaying();
                onMousePressed(mouseToLogical(window, press->position), press->button);
            }
            else if (padA && isStartKey(sf::Keyboard::Key::Space))
            {
                beginPlaying();
                onKeyPressed(sf::Keyboard::Key::Space);
            }
            else if (escKey || padB)
            {
                exitToMenu();
            }
            break;

        case State::Playing:
            if (escKey || pauseKey || padStart)
            {
                pauseGame();
            }
            else if (keyPress)
            {
                onKeyPressed(keyPress->code);
            }
            else if (padA)
            {
                onKeyPressed(sf::Keyboard::Key::Space);
            }
            else if (keyUp)
            {
                onKeyReleased(keyUp->code);
            }
            else if (move)
            {
                m_hud.setPauseHovered(m_hud.pauseHit(m_mouse));
                onMouseMoved(m_mouse);
            }
            else if (press)
            {
                const sf::Vector2f point = mouseToLogical(window, press->position);
                if (leftPress && m_hud.pauseHit(point))
                {
                    pauseGame();
                }
                else
                {
                    onMousePressed(point, press->button);
                }
            }
            else if (release)
            {
                onMouseReleased(mouseToLogical(window, release->position), release->button);
            }
            break;

        case State::Paused:
            if (escKey || pauseKey || padStart || padB)
            {
                resumeGame();
            }
            else if (move)
            {
                m_pause.handleMouseMove(m_mouse);
            }
            else if (leftPress)
            {
                m_pause.handleMouseDown(mouseToLogical(window, press->position));
            }
            else if (leftRelease)
            {
                runAction(m_pause.handleMouseUp(mouseToLogical(window, release->position)));
            }
            break;

        case State::GameOver:
            if ((confirmKey || padA) && m_gameOver.acceptsInput())
            {
                start();
            }
            else if (escKey || padB)
            {
                exitToMenu();
            }
            else if (move)
            {
                m_gameOver.handleMouseMove(m_mouse);
            }
            else if (leftPress)
            {
                m_gameOver.handleMouseDown(mouseToLogical(window, press->position));
            }
            else if (leftRelease)
            {
                runAction(m_gameOver.handleMouseUp(mouseToLogical(window, release->position)));
            }
            break;
        }
    }

    void ArcadeGame::update(float dt)
    {
        dt = std::clamp(dt, 0.0f, 0.05f);
        pollGamepad();
        m_clock += dt;
        m_effects.update(dt);
        m_hud.update(dt);
        m_toast.update(dt);

        switch (m_state)
        {
        case State::Ready:
            onIdle(dt);
            m_ready.update(dt);
            m_readyFade = 1.0f;
            m_overlayFade = std::max(0.0f, m_overlayFade - 8.0f * dt);
            break;

        case State::Playing:
            m_runTime += dt;
            if (m_readyFade > 0.0f)
            {
                m_readyFade = std::max(0.0f, m_readyFade - 4.0f * dt);
                m_ready.update(dt);
            }
            m_overlayFade = std::max(0.0f, m_overlayFade - 8.0f * dt);
            onUpdate(dt);
            break;

        case State::Paused:
            m_pause.update(dt);
            m_overlayFade = std::min(1.0f, m_overlayFade + 8.0f * dt);
            break;

        case State::GameOver:
            m_gameOverTimer += dt;
            if (m_fanfareTimer >= 0.0f)
            {
                m_fanfareTimer -= dt;
                if (m_fanfareTimer < 0.0f)
                {
                    m_audio.playNewBest();
                    m_fanfareTimer = -1.0f;
                }
            }
            onIdle(dt);
            if (m_gameOverTimer > 0.6f)
            {
                m_overlayFade = std::min(1.0f, m_overlayFade + 3.0f * dt);
            }
            m_gameOver.update(dt);
            break;
        }
    }

    void ArcadeGame::draw(sf::RenderWindow &window)
    {
        applyViewport(window);
        window.clear(sf::Color::Black);

        sf::View view = m_view;
        view.setCenter(sf::Vector2f{kWidth * 0.5f, kHeight * 0.5f} + m_effects.shakeOffset());
        window.setView(view);

        drawBackground(window);
        m_effects.drawBack(window);
        drawWorld(window);
        m_effects.drawFront(window);

        if (m_state != State::GameOver)
        {
            m_hud.draw(window, m_state != State::Ready, 255);
        }

        if (m_state == State::Ready || m_readyFade > 0.01f)
        {
            m_ready.draw(window, sf::Vector2f{kWidth * 0.5f, kHeight * 0.55f}, m_readyFade);
        }
        if (m_state == State::Paused && m_overlayFade > 0.01f)
        {
            m_pause.draw(window, m_overlayFade);
        }
        if (m_state == State::GameOver && m_overlayFade > 0.01f)
        {
            m_gameOver.draw(window, m_overlayFade);
        }

        m_toast.draw(window);

        window.setView(window.getDefaultView());
    }

    void ArcadeGame::applyViewport(const sf::RenderWindow &window)
    {
        const sf::Vector2u size = window.getSize();
        if (size.x == 0 || size.y == 0)
        {
            return;
        }

        const float winW = static_cast<float>(size.x);
        const float winH = static_cast<float>(size.y);
        const float aspect = kWidth / kHeight;

        // Largest 16:9 rectangle that fits, centered (letterbox)
        float w = winW;
        float h = winW / aspect;
        if (h > winH)
        {
            h = winH;
            w = winH * aspect;
        }

        const float fw = w / winW;
        const float fh = h / winH;
        m_view.setViewport(sf::FloatRect({(1.0f - fw) * 0.5f, (1.0f - fh) * 0.5f}, {fw, fh}));
    }

    sf::Vector2f ArcadeGame::mouseToLogical(const sf::RenderWindow &window, sf::Vector2i pixel) const
    {
        return window.mapPixelToCoords(pixel, m_view);
    }

    bool ArcadeGame::keyDown(sf::Keyboard::Key key) const
    {
        if (sf::Keyboard::isKeyPressed(key))
        {
            return true;
        }
        switch (key)
        {
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::A:
            return m_pad.left;
        case sf::Keyboard::Key::Right:
        case sf::Keyboard::Key::D:
            return m_pad.right;
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::W:
            return m_pad.up;
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            return m_pad.down;
        case sf::Keyboard::Key::Space:
            return m_pad.confirm;
        default:
            return false;
        }
    }

    void ArcadeGame::pollGamepad()
    {
        const PadState previous = m_pad;
        PadState now;
        if (sf::Joystick::isConnected(0))
        {
            const float x = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
            const float y = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
            const float povX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovX);
            const float povY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovY);
            now.left = x < -kPadDeadZone || povX < -kPadDeadZone;
            now.right = x > kPadDeadZone || povX > kPadDeadZone;
            // Stick Y is negative upward, POV Y is positive upward
            now.up = y < -kPadDeadZone || povY > kPadDeadZone;
            now.down = y > kPadDeadZone || povY < -kPadDeadZone;
            now.confirm = sf::Joystick::isButtonPressed(0, 0);
        }
        m_pad = now;

        // A direction that newly crosses the dead zone acts like a fresh arrow key press
        if (now.left && !previous.left)
        {
            padKeyPressed(sf::Keyboard::Key::Left);
        }
        if (now.right && !previous.right)
        {
            padKeyPressed(sf::Keyboard::Key::Right);
        }
        if (now.up && !previous.up)
        {
            padKeyPressed(sf::Keyboard::Key::Up);
        }
        if (now.down && !previous.down)
        {
            padKeyPressed(sf::Keyboard::Key::Down);
        }
    }

    void ArcadeGame::padKeyPressed(sf::Keyboard::Key key)
    {
        if (m_state == State::Ready)
        {
            if (isStartKey(key))
            {
                beginPlaying();
                onKeyPressed(key);
            }
        }
        else if (m_state == State::Playing)
        {
            onKeyPressed(key);
        }
    }

    void ArcadeGame::beginPlaying()
    {
        m_state = State::Playing;
        m_runTime = 0.0f;
        onBegin();
    }

    void ArcadeGame::pauseGame()
    {
        m_state = State::Paused;
        m_pause.setScores(m_score, m_best);
        m_audio.setMusicDucked(true);
        m_audio.playClick();
    }

    void ArcadeGame::resumeGame()
    {
        m_state = State::Playing;
        m_audio.setMusicDucked(false);
    }

    void ArcadeGame::exitToMenu()
    {
        m_audio.stopMusic();
        if (m_exitToMenu)
        {
            m_exitToMenu();
        }
    }

    void ArcadeGame::addScore(int points)
    {
        m_score += points;
        m_hud.setScore(m_score);
        m_hud.setBest(std::max(m_best, m_score));
        Achieve::onScore(m_gameId, m_score, m_difficulty, m_toast, m_audio);
    }

    void ArcadeGame::unlockAchievement(int id)
    {
        Achieve::unlock(id, m_toast, m_audio);
    }

    void ArcadeGame::setCombo(int combo)
    {
        m_hud.setStreak(combo);
        if (combo >= 10)
        {
            Achieve::unlock(::Achievements::ComboKing, m_toast, m_audio);
        }
    }

    void ArcadeGame::setComboWord(const std::string &word)
    {
        m_hud.setStreakWord(word);
    }

    void ArcadeGame::setLives(int lives, int maxLives)
    {
        m_hud.setLives(lives, maxLives);
    }

    void ArcadeGame::gameOver(sf::Vector2f where, float scale)
    {
        if (m_state != State::Playing)
        {
            return;
        }

        m_state = State::GameOver;
        m_gameOverTimer = 0.0f;
        m_overlayFade = 0.0f;

        if (scale >= 1.0f)
        {
            m_effects.spawnExplosion(where);
        }
        else
        {
            m_effects.spawnBlast(where, scale * 2.0f, m_accent.base);
            m_effects.addShake(10.0f);
        }
        m_audio.playCrash();
        m_audio.setMusicDucked(true);

        const int previousBest = m_best;
        m_newBest = m_score > m_best;
        if (m_newBest)
        {
            m_best = m_score;
            m_effects.spawnConfetti();
            m_fanfareTimer = 1.0f;
        }
        UserData::instance().recordGame(m_gameId, m_difficulty, m_score);
        Achieve::onScore(m_gameId, m_score, m_difficulty, m_toast, m_audio);
        Achieve::onRunEnd(m_toast, m_audio);
        m_hud.setBest(m_best);
        m_gameOver.show(m_score, previousBest, m_newBest);
    }
} // namespace Flappy
