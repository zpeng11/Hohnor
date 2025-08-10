# Keyboard Input Example

This example demonstrates how to use the Hohnor EventLoop framework for interactive keyboard input handling.

## Features

- **Interactive Keyboard Input**: Captures individual keystrokes in real-time without requiring Enter
- **Event Loop Integration**: Shows how to integrate keyboard input with the Hohnor EventLoop
- **Signal Handling**: Demonstrates graceful shutdown with Ctrl+C (SIGINT)
- **Non-blocking I/O**: Uses the EventLoop's non-blocking I/O capabilities for responsive input handling

## What This Example Shows

1. **EventLoop Creation**: How to create and configure an EventLoop instance
2. **Keyboard Handling**: Using `handleKeyboard()` to set up interactive keyboard input
3. **Signal Management**: Using `handleSignal()` for graceful shutdown on Ctrl+C
4. **Callback Functions**: Implementing keyboard and signal callbacks
5. **Resource Cleanup**: Proper cleanup with `unHandleKeyboard()`

## Building the Example

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- The Hohnor library built in the parent directory

### Build Steps

1. Make sure the main Hohnor library is built first:
   ```bash
   cd ../..  # Go to project root
   mkdir -p build
   cd build
   cmake ..
   make
   ```

2. Build the keyboard example:
   ```bash
   cd example/keyboard
   mkdir -p build
   cd build
   cmake ..
   make
   ```

3. Run the example:
   ```bash
   ./keyboard_example
   ```

## Usage

Once the program is running:

- **h/H**: Show help message
- **q/Q**: Quit the application
- **ESC**: Detect ESC key press
- **Enter**: Detect Enter key press
- **Ctrl+C**: Graceful shutdown via signal handling
- **Any other key**: Display the key and its ASCII code

## Code Structure

### KeyboardHandler Class

The `KeyboardHandler` class encapsulates the keyboard input logic:

- `onKeyPress(char key)`: Handles individual key presses
- `printHelp()`: Displays available commands
- `isRunning()`: Tracks application state

### Main Function

1. Creates an EventLoop instance
2. Sets up signal handling for SIGINT (Ctrl+C)
3. Configures keyboard input handling with a callback
4. Starts the event loop
5. Cleans up resources on exit

## Key EventLoop Methods Used

- [`EventLoop::handleKeyboard(KeyboardCallback cb)`](../../include/hohnor/core/EventLoop.h:76): Sets up interactive keyboard input
- [`EventLoop::handleSignal(int signal, SignalAction action, SignalCallback cb)`](../../include/hohnor/core/EventLoop.h:73): Handles system signals
- [`EventLoop::loop()`](../../include/hohnor/core/EventLoop.h:43): Starts the main event loop
- [`EventLoop::endLoop()`](../../include/hohnor/core/EventLoop.h:55): Stops the event loop
- [`EventLoop::unHandleKeyboard()`](../../include/hohnor/core/EventLoop.h:79): Cleans up keyboard handling

## Technical Details

- The keyboard input is handled in **non-canonical mode**, meaning individual keystrokes are captured immediately without waiting for Enter
- The terminal is automatically restored to its original state when the program exits
- The EventLoop uses epoll internally for efficient I/O multiplexing
- Signal handling is integrated into the event loop for consistent event processing

## Error Handling

The example includes basic error handling:
- Exception catching in the main function
- Graceful shutdown on signals
- Proper resource cleanup

This example serves as a foundation for building more complex interactive applications using the Hohnor EventLoop framework.