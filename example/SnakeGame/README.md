# Snake Game Example

A classical snake game implementation using the Hohnor EventLoop framework and ncurses for terminal-based graphics.

## Overview

This example demonstrates how to create a real-time game using the Hohnor EventLoop framework. It showcases:

- **Event-driven architecture**: Using [`EventLoop`](../../include/hohnor/core/EventLoop.h:33) for managing game events
- **Keyboard input handling**: Using [`EventLoop::handleKeyboard()`](../../include/hohnor/core/EventLoop.h:76) for responsive controls
- **Timer-based game loop**: Using [`EventLoop::addTimer()`](../../include/hohnor/core/EventLoop.h:70) for consistent game timing
- **Signal handling**: Graceful shutdown with Ctrl+C using [`EventLoop::handleSignal()`](../../include/hohnor/core/EventLoop.h:73)
- **ncurses integration**: Terminal-based graphics for a retro gaming experience

## Game Features

- **Classic Snake Gameplay**: Control a growing snake to eat food and avoid collisions
- **Real-time Controls**: Responsive WASD movement controls
- **Score Tracking**: Points awarded for each food item consumed
- **Collision Detection**: Game ends when snake hits walls or itself
- **Pause/Resume**: Press 'P' to pause the game
- **Restart**: Press 'R' to restart after game over
- **Graceful Exit**: Press 'Q' to quit or use Ctrl+C

## Controls

| Key | Action |
|-----|--------|
| `W` | Move Up |
| `S` | Move Down |
| `A` | Move Left |
| `D` | Move Right |
| `P` | Pause/Resume |
| `Q` | Quit Game |
| `R` | Restart (when game over) |
| `Ctrl+C` | Force quit |

## Game Elements

- `@` - Snake head
- `o` - Snake body
- `*` - Food
- `#` - Wall/Border

## Building and Running

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- Built Hohnor library (libhohnor.a)
- ncurses library (automatically downloaded and built)

### Build Instructions

1. **Navigate to the SnakeGame directory:**
   ```bash
   cd example/SnakeGame
   ```

2. **Create build directory and compile:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Run the game:**
   ```bash
   ./SnakeGame
   ```

### Terminal Requirements

- Minimum terminal size: 42x25 characters
- Terminal must support ncurses
- Works best in full-screen terminal

## Implementation Details

### Architecture

The game uses the Hohnor EventLoop framework's event-driven architecture:

1. **Main Event Loop**: [`EventLoop`](../../include/hohnor/core/EventLoop.h:33) manages all events
2. **Keyboard Handler**: Processes user input through [`EventLoop::handleKeyboard()`](../../include/hohnor/core/EventLoop.h:76)
3. **Game Timer**: Updates game state at regular intervals using [`EventLoop::addTimer()`](../../include/hohnor/core/EventLoop.h:70)
4. **Signal Handler**: Handles system signals for graceful shutdown

### Key Classes

- **`SnakeGame`**: Main game logic and state management
- **`Point`**: Simple 2D coordinate structure
- **`Direction`**: Enumeration for snake movement directions

### Game Loop

The game runs at approximately 6.67 FPS (150ms per frame) for smooth gameplay:

1. Process keyboard input
2. Update snake position based on direction
3. Check for collisions (walls, self, food)
4. Update score and generate new food if needed
5. Render the game state using ncurses

### Memory Management

- Uses RAII principles for automatic cleanup
- Smart pointers for timer management
- Automatic ncurses cleanup in destructor

## Troubleshooting

### Common Issues

1. **Terminal too small**: Resize terminal to at least 42x25 characters
2. **Build errors**: Ensure Hohnor library is built in `../../build/libhohnor.a`
3. **ncurses not found**: The CMake script automatically downloads and builds ncurses
4. **Game not responsive**: Check terminal supports ncurses and keyboard input

### Debug Mode

The game includes error handling and will display helpful messages for common issues like terminal size problems.

## Educational Value

This example is perfect for learning:

- Event-driven programming patterns
- Real-time game development
- Terminal-based user interfaces
- Timer-based game loops
- Input handling in C++
- Memory management with smart pointers

## Extending the Game

Potential enhancements:

- Multiple difficulty levels (speed adjustment)
- High score persistence
- Different food types with varying points
- Obstacles and maze-like levels
- Multiplayer support
- Sound effects (terminal bell)
- Color support with ncurses color pairs

## License

This example is part of the Hohnor framework and follows the same licensing terms.