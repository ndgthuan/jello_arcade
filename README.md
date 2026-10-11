# Jello

Jello is a menu application with five arcade games. The games are drawn and synthesized entirely in code, and the
project is written in C++23 with SFML 3.

## Features

- **Main menu** with a generated background, a jello mascot that follows the mouse, and its own music.
- **Settings**: music and effects volume, mute, jello colour (4 choices), difficulty (Easy, Normal, Hard),
  screen shake.
- **Credits** and **Help** (Help has a tab for every game).
- **Alt Manager**: local player profiles (up to 6), each with its own best score per game and difficulty,
  games played and total score.
- **Trophies**: 20 achievements per profile, tracked for the active profile.
- **Games**, each with synthesized sound and music:
  - **Jello Bounce**: flap through the pipes.
  - **Jello Smash**: breakout with power-ups and chain-reaction bombs.
  - **Jello Blaster**: side-scrolling space shooter with a boss.
  - **Jello Slither**: snake that blows up chain by chain.
  - **Jello Stack**: drop sliding blocks to build the tower and chase PERFECT drops.
- **Gamepad support** and an optional **screen shake** effect.

## Controls

### Menu

| Input | Action |
| --- | --- |
| Mouse move | Point at menu items and panel buttons |
| Left click | Select menu items and buttons |

### Games

| Game | Controls |
| --- | --- |
| Jello Bounce | Space, Up or W, or left click to flap; Enter confirms |
| Jello Smash | Mouse or Left / Right (A / D) moves the paddle; Space launches the balls |
| Jello Blaster | Arrows or WASD to move; Space or hold left mouse button to fire |
| Jello Slither | Arrows or WASD to steer; Space or Enter starts |
| Jello Stack | Space, Enter, Up or Down, or left click to drop the block |

### Global

| Key | Action |
| --- | --- |
| Esc | Pause, or go back |
| P | Pause |
| M | Mute / unmute |
| F11 | Toggle fullscreen |
| F3 | Toggle the FPS overlay |

## Where data is stored

Settings, profiles, best scores and trophies are saved in one JSON file:

```
%APPDATA%\Jello\userdata.json
```

The folder is created on the first save. If an older `userdata.json` exists in the working directory and the new
file does not exist yet, it is read once; the next save writes the new location and the old file is left in place.
Outside Windows, `$HOME/.local/share/Jello/userdata.json` is used when `APPDATA` is not set. Without either variable,
the file is kept in the working directory.

## Building

Requirements: a C++23 compiler and CMake 3.16 or newer. SFML 3 is downloaded automatically if it is not installed.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is written to `release/Jello.exe`. Run it from the repository root, because assets and config
files are loaded with relative paths:

```bash
./release/Jello.exe
```

## Project structure

```
source/
  main.cpp
  Data/
    Config/          UI configuration and resource_config.json
    Logging/         Logger
    Profile/         UserData (settings, profiles, scores), Achievements
    Resources/       ResourceManager, Audio, bundled assets
  Presentation/
    Core/            Window and game coordination (GameWindow, WindowManager, GameCoordinator)
    Game/            The five games, effects, game audio, UiKit
    Panels/          Menu panels (Settings, Credits, Help, Alt Manager, Trophies, game picker)
    Rendering/       Background, camera and particle rendering
    UI/              Menu UI and UI components
    Input/           Input handling and key bindings
```

## Credits

Made by Nguyen Duong Gia Thuan, built with Claude Code.

The bundled fonts (Geomanist, Corbel Light and Helvetica Neue UltraLight) belong to their respective owners and
are included only for use in this project.
