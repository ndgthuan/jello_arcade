# Jello

A small C++ game launcher with five arcade games, written with SFML 3. There are no image or sound files for
the games: the pictures are drawn in code and the sounds and music are generated when the program starts.

Games: Jello Bounce (flappy-style), Jello Smash (breakout), Jello Blaster (shooter), Jello Slither (snake) and
Jello Stack (stacking blocks). The menu also has settings, a help page, local player profiles with their own
best scores, and a list of 20 trophies.

## Build

You need a C++23 compiler and CMake 3.21 or newer. If SFML 3 is not installed, CMake downloads and builds it
(the first configure takes a minute or two).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This has only been tried on Windows 11 with MinGW-w64 (g++ 15, CMake 4.1). The exe lands in `release/`. Run it
from the repository root, because assets are loaded with relative paths:

```
./release/Jello.exe
```

## Controls

- Menu: mouse. Esc closes a panel.
- Bounce: Space, Up, W or click to flap.
- Smash: mouse or A/D (or the arrow keys) to move the paddle, Space or click to launch the ball.
- Blaster: mouse or WASD to fly, hold Space or the left mouse button to shoot.
- Slither: arrow keys or WASD.
- Stack: Space, Enter, Up, Down or click to drop the block.
- In a game: Esc or P pauses, M mutes.
- Anywhere: F11 fullscreen, F3 shows the FPS.

A gamepad works too (stick or D-pad to move, A to confirm, B to go back, Start to pause).

## Saved data

Settings, profiles, scores and trophies are in `%APPDATA%\Jello\userdata.json`. If there is an old
`userdata.json` next to the exe it is read once and the next save goes to the new place.

## Layout

```
source/Data/          config, logger, user data and trophies, resource loading
source/Presentation/  window and menu code, the games (Game/), menu panels (Panels/), rendering
```

## Fonts

The fonts in `source/Data/Resources/assets/fonts` (Geomanist, Corbel Light, Helvetica Neue UltraLight) belong to
their owners and are only here for use in this project.

## Author

Nguyen Duong Gia Thuan
