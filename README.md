# Jello

A beautiful, interactive menu application built with C++ and SFML 3.0 featuring smooth camera panning and modern UI effects.

## ✨ Features

- **Interactive Menu**: Main menu with "Jello" text featuring dynamic glow effects
- **Circle Menu**: 5 interactive icons (Play, Settings, Info, Account, Exit)
- **Camera Panning**: Mouse-following camera with momentum and smooth movement
- **Particle Effects**: Dynamic background particles
- **Audio Feedback**: Click sounds and audio effects
- **Responsive Design**: Automatically adjusts to window size
- **Modern UI**: Clean, minimalist design with hover effects

## What is in it

- **Play**: a game chooser with five arcade games, each with glossy 3D visuals, explosions, synthesized sound and its own music:
  **Jello Bounce** (flap through pipes), **Jello Smash** (breakout with power-ups and chain-reaction bombs),
  **Jello Blaster** (side-scrolling space shooter with a boss), **Jello Slither** (snake that blows up chain by chain)
  and **Jello Stack** (drop sliding blocks, build the tower, chase PERFECT drops).
- **Settings**: music / effects volume, mute, jello colour (4 choices), difficulty (Easy / Normal / Hard), screen shake, fullscreen.
- **Credits**, **Help**: who made it, and a Help page with tabs for every game.
- **Trophies**: 20 achievements per profile (open them from Alt Manager); a banner slides in when one unlocks.
- **Alt Manager**: local player profiles (up to 6) with their own best score per game and difficulty, games played and total score.
- The main menu has its own music and a jello mascot that watches your mouse.
- Settings and profiles are saved in `userdata.json` next to the game (created on first run, ignored by git).
- The menu background and every picture in the game are drawn in code, and all sounds and music are synthesized in code.

## 🎮 Controls

- **Mouse**: Move to pan camera, click to interact with menu items
- **F11**: Toggle fullscreen mode
- **Escape**: Exit application
- **M** (in game): mute / unmute

## 🚀 Quick Start

### Prerequisites
- C++23 compatible compiler
- SFML 3.0 libraries
- CMake 3.16+

### Build & Run

```bash
# Clone repository
git clone <repository-url>
cd ModernMenuGame

# Build (Windows)
build.bat

# Build (Linux/Mac)
chmod +x build.sh
./build.sh

# Run (from the repository root, asset paths are relative)
./release/Jello.exe
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<path to SFML 3 install>
cmake --build . --config Release
```

SFML DLLs are copied next to `Jello.exe` automatically after the build.

## 📁 Project Structure

```
ModernMenuGame/
├── source/                   # Source code
│   ├── main.cpp             # Entry point
│   ├── Data/                # Data Layer
│   │   ├── Config/          # Configuration files
│   │   ├── Logging/         # Logging system
│   │   └── Resources/       # Asset management
│   └── Presentation/        # Presentation Layer
│       ├── Core/            # Core game systems
│       ├── Input/           # Input handling
│       ├── Rendering/       # Rendering systems
│       └── UI/              # User interface
├── release/                 # Built executable
└── game.log                 # Log file
```

## 🎯 Design Patterns

- **Singleton**: Logger, ResourceManager, AudioManager
- **Dependency Injection**: Services and configuration
- **Facade**: MenuUI, GameCoordinator
- **Observer**: Callback system
- **Factory**: KeyBindingFactory

## ⚙️ Configuration

### Configuration Files

#### `resource_config.json`
```json
{
  "textures": {
    "TitleLogo": "source/Data/Resources/assets/textures/logo.png",
    "MenuBackground": "source/Data/Resources/assets/images/MenuBackground.png",
    "play": "source/Data/Resources/assets/textures/play.png",
    "settings": "source/Data/Resources/assets/textures/settings.png",
    "info": "source/Data/Resources/assets/textures/info.png",
    "interrogation": "source/Data/Resources/assets/textures/interrogation.png",
    "account": "source/Data/Resources/assets/textures/account.png"
  },
  "fonts": {
    "main": "source/Data/Resources/assets/fonts/font.ttf",
    "peter-light": "source/Data/Resources/assets/fonts/corbel-light.ttf",
    "helvetica-ultra-light": "source/Data/Resources/assets/fonts/helvetica-neue-lt-std-ultra-light.otf"
  },
  "audio": {
    "menu_click": "source/Data/Resources/assets/audio/MenuClickSound.ogg"
  }
}
```

#### `Keybinds.json`
Only `fullscreen_toggle` and `exit` are used so far; `pause` and `menu` are reserved.
```json
{
  "keyboard": {
    "fullscreen_toggle": "F11",
    "exit": "Escape",
    "pause": "P",
    "menu": "M"
  },
  "mouse": {
    "left_click": "Left",
    "right_click": "Right",
    "middle_click": "Middle"
  }
}
```

## 🎨 UI Components

### MenuUI
- **TitleText**: "Jello" text with glow effects
- **CircleMenu**: Circular menu with 5 interactive icons
- **Button**: Exit button with hover effects

### Visual Effects
- **Glow Animation**: Dynamic glow effects on title text
- **Hover Effects**: Icon scaling and color changes
- **Smooth Transitions**: Fluid animations
- **Particle System**: Background particle effects

## 🔧 Development

### Logging System
- **Thread-safe**: Mutex protection for concurrent access
- **Multiple Levels**: DEBUG, INFO, WARNING, ERROR, CRITICAL
- **File Output**: Logs written to `game.log`
- **Console Output**: Warnings+ displayed on console

### Audio System
- **Sound Effects**: Menu click sounds
- **Volume Control**: Adjustable audio levels
- **Resource Management**: Load and cache audio files

### Resource Management
- **Singleton Pattern**: Centralized resource access
- **JSON Configuration**: Load resources from config files
- **Error Handling**: Graceful fallback when loading fails

## 🎮 Game Loop

```cpp
while (window.isOpen()) {
    // 1. Process events
    window.processEvents();
    
    // 2. Update systems
    coordinator.update(deltaTime);
    
    // 3. Render
    coordinator.render(window);
    
    // 4. Display
    window.display();
}
```

## 🐛 Troubleshooting

### Common Issues
1. **SFML not found**: Install SFML 3.0 libraries
2. **Assets not loading**: Check file paths in config files
3. **Audio issues**: Check audio drivers and file formats

### Debug
- Check log file `game.log` for debugging
- Use console output for warnings
- Verify resource loading in logs

## 🤝 Contributing

### Code Standards
- **C++23**: Modern C++ features
- **Clean Code**: Readable and maintainable
- **Documentation**: Comment important functions
- **Error Handling**: Proper exception handling

### Development Workflow
1. Fork repository
2. Create feature branch
3. Implement changes
4. Test thoroughly
5. Submit pull request

## 📄 License

This project is released under the MIT License.

## 👥 Authors

- **Developer**: Nguyễn Dương Gia Thuận
- **Architecture**: Clean Architecture with SOLID principles
- **Framework**: SFML 3.0

## 🔗 Links

- **SFML Documentation**: https://www.sfml-dev.org/
- **C++ Reference**: https://en.cppreference.com/
- **Game Development**: Best practices and patterns

---

**Jello** - a menu built with C++ and SFML 3.0.