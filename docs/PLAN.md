# Jello - plan for the menu screens, profiles and new background

Goal: every icon on the main menu opens something real, the menu looks like the game, and the game honours
what the player chose.

## Scope

| Menu icon | Becomes | Notes |
|---|---|---|
| Play | Jello Bounce (done) | uses the active profile, difficulty and colour |
| Settings | Settings panel | music / effects volume, mute, jello colour, difficulty, fullscreen |
| Credits | Credits panel | who made it, what it is built with, fonts |
| Help | Help panel | goal, controls, scoring, tips |
| Alt Manager | Accounts panel | local player profiles: add, switch, delete, per-profile stats and best scores |

Background: the anime picture (copyright risk) is replaced by an original twilight scene drawn in code
(the same sky / sun / hills / neon floor as the game), rendered once into a texture at startup.

## Architecture

```
Data/Profile/UserData      settings + profiles, stored as userdata.json (nlohmann json)
Presentation/Panels/
    Widgets                card frame, Slider, Segmented, SwatchRow, Toggle, TextField
    Panel (base)           update / draw / mouse / text / key, close + fullscreen requests
    SettingsPanel, HelpPanel, CreditsPanel, AccountPanel
    PanelManager           opens one panel at a time, dim + letterboxed 1280x720 view, routes events
Presentation/Game          Jello Bounce reads UserData (difficulty, accent, volumes, active profile, bests)
Rendering/BackgroundManager  builds the menu background from Flappy::Scene, "cover" scaling
```

Event routing: when a panel is open, `GameWindow` polls events itself and hands them to `PanelManager`
(same trick as the game). The menu icon click is a single `iconCallback(index)`.

## Steps

1. UserData (settings, profiles, JSON save/load, migration of nothing - old best file is dropped).
2. Shared contracts: headers for widgets, panels, manager (so parallel workers cannot drift).
3. Parallel work: Widgets, SettingsPanel, Help + Credits, AccountPanel, PanelManager.
4. Lead work: background generation, icon routing, difficulty in PipeField, FlappyGame <-> UserData, HUD player name.
5. Build, then drive the real app (click every icon, type a name, move sliders) and look at screenshots.
6. Fix what the screenshots show, update README, tidy `.gitignore`.

## Decisions
- Difficulty: Easy gap 270 / speed 215, Normal 240 / 240, Hard 210 / 270. Best score is stored per difficulty per profile.
- Max 6 profiles, names 1-12 printable ASCII characters, unique ignoring case, at least one profile always exists.
- UI language stays English (matches the existing menu).
- Colour choices: lime, pink, gold, sky blue (same four as the design mockup).
