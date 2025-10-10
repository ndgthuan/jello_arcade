# Jello Game Menu - PanningMenu Project

## 📖 Overview

Jello is a game menu application built with C++ and SFML using modern object-oriented architecture. The project features camera panning that follows mouse movement to create smooth and engaging interactive experiences.

## ✨ Key Features

### 🎮 Menu System
- **Interactive Menu**: Main menu with "Jello" text featuring dynamic glow effects
- **Circle Menu**: 5 interactive icons (Play, Settings, Info, Account, Exit)
- **Hover Effects**: Smooth hover animations with transitions
- **Click Sounds**: Audio feedback on interactions
- **Responsive Design**: Automatically adjusts to window size

### 📷 Camera System
- **Mouse Panning**: Camera follows mouse movement with momentum
- **Smooth Movement**: Fluid motion with interpolation
- **Boundary Awareness**: Camera constrained within background bounds
- **Configurable**: Customizable speed and momentum settings

### 🎨 Rendering System
- **Background Rendering**: Background display with panning support
- **Particle System**: Dynamic particle effects
- **UI Rendering**: UI unaffected by camera movement
- **Teal Theme**: Beautiful teal color scheme

### ⌨️ Input System
- **Mouse Tracking**: Precise mouse movement tracking
- **Click Handling**: Comprehensive click event processing
- **Keyboard Shortcuts**: 
  - `F11`: Toggle fullscreen
  - `Escape`: Exit application
- **Key Binding**: JSON-configurable key bindings

## 🏗️ Project Architecture

### 📁 Directory Structure
```
PanningMenu/
├── source/
│   ├── Data/                    # Data Layer
│   │   ├── Config/             # Configuration files
│   │   ├── Logging/            # Logging system
│   │   └── Resources/          # Asset management
│   └── Presentation/           # Presentation Layer
│       ├── Core/              # Core game systems
│       ├── Input/             # Input handling
│       ├── Rendering/         # Rendering systems
│       └── UI/                # User interface
├── release/                    # Executable files
└── game.log                   # Log file
```

### 🎯 Design Patterns
- **Singleton**: Logger, ResourceManager, AudioManager
- **Dependency Injection**: Services and configuration
- **Facade**: MenuUI, GameCoordinator
- **Observer**: Callback system
- **Factory**: KeyBindingFactory

## 🚀 Installation and Running

### System Requirements
- **Compiler**: C++17 compatible (GCC, Clang, MSVC)
- **Libraries**: SFML 2.5+ (Graphics, Audio, System, Window)
- **OS**: Windows, Linux, macOS

### Build from Source

#### Method 1: Using CMake (Recommended)
```bash
# Clone repository
git clone <repository-url>
cd PanningMenu

# Option A: Use build script (Windows)
build.bat

# Option B: Use build script (Linux/Mac)
chmod +x build.sh
./build.sh

# Option C: Manual CMake build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# The executable will be created in release/ directory
# Output: release/Jello.exe (Windows) or release/Jello (Linux/Mac)
```

#### Method 2: Direct Compilation (Windows)
```bash
# Navigate to project directory
cd PanningMenu

# Compile directly with g++
g++ -std=c++23 -I"C:\Program Files\SFML-3.0.0\include" -L"C:\Program Files\SFML-3.0.0\lib" source\main.cpp source\Data\Resources\*.cpp .\source\Presentation\Rendering\*.cpp source\Presentation\Core\*.cpp .\source\Presentation\Input\*.cpp .\source\Presentation\UI\*.cpp .\source\Data\Config\*.cpp .\source\Data\Logging\*.cpp -o .\release\Jello.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
```

#### Method 3: Using Visual Studio
```bash
# Open CMakeLists.txt in Visual Studio
# Or generate Visual Studio project files:
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
# Open the generated .sln file in Visual Studio
```

### Run Application
```bash
# Run executable from release directory
.\release\Jello.exe

# Or from build directory (if using CMake)
.\build\Release\Jello.exe
```

## ⚙️ Configuration

### 📄 Configuration Files

#### `games_config.json`
```json
{
  "games": {
    "tetris": {
      "name": "Tetris",
      "description": "Classic block stacking puzzle game",
      "enabled": true
    }
  }
}
```

#### `resource_config.json`
```json
{
  "textures": {
    "TitleLogo": "path/to/logo.png",
    "MenuBackground": "path/to/background.png"
  },
  "fonts": {
    "main": "path/to/font.ttf"
  },
  "audio": {
    "menu_click": "path/to/click.ogg"
  }
}
```

#### `Keybinds.json`
```json
{
  "keyboard": {
    "fullscreen_toggle": "F11",
    "exit": "Escape",
    "pause": "P"
  }
}
```

## 🎨 UI Components

### 🎯 MenuUI
- **TitleText**: "Jello" text with glow effects
- **CircleMenu**: Circular menu with icons
- **Button**: Exit button with hover effects

### 🎨 Visual Effects
- **Glow Animation**: Dynamic glow effects on title text
- **Hover Effects**: Icon scaling and color changes
- **Smooth Transitions**: Fluid animations
- **Particle System**: Background particle effects

## 🔧 Development

### 📝 Logging System
- **Thread-safe**: Mutex protection for concurrent access
- **Multiple Levels**: DEBUG, INFO, WARNING, ERROR, CRITICAL
- **File Output**: Logs written to file
- **Console Output**: Warnings+ displayed on console

### 🎵 Audio System
- **Sound Effects**: Menu click sounds
- **Volume Control**: Adjustable audio levels
- **Resource Management**: Load and cache audio files

### 🖼️ Resource Management
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
1. **SFML not found**: Install SFML libraries
2. **Assets not loading**: Check file paths in config files
3. **Audio issues**: Check audio drivers and file formats

### Debug
- Check log file `game.log` for debugging
- Use console output for warnings
- Verify resource loading in logs

## 📊 Performance

### 🎯 Optimizations
- **Smart Pointers**: Automatic memory management
- **Efficient Rendering**: Batch rendering operations
- **Smooth Animations**: 60 FPS target
- **Resource Caching**: Load once, use many times

### 📈 Metrics
- **Memory Usage**: Optimized with smart pointers
- **Frame Rate**: Smooth 60 FPS
- **Load Time**: Fast resource loading
- **Responsiveness**: Low input latency

## 🤝 Contributing

### 📋 Code Standards
- **C++17**: Modern C++ features
- **Clean Code**: Readable and maintainable
- **Documentation**: Comment important functions
- **Error Handling**: Proper exception handling

### 🔄 Development Workflow
1. Fork repository
2. Create feature branch
3. Implement changes
4. Test thoroughly
5. Submit pull request

## 📄 License

This project is released under the [MIT License](LICENSE).

## 👥 Authors

- **Developer**: Nguyễn Dương Gia Thuận
- **Architecture**: Clean Architecture with SOLID principles
- **Framework**: SFML 2.5+

## 🔗 Links

- **SFML Documentation**: https://www.sfml-dev.org/
- **C++ Reference**: https://en.cppreference.com/
- **Game Development**: Best practices and patterns

---

**Jello Game Menu** - An excellent example of modern game architecture with C++ and SFML! 🎮✨
