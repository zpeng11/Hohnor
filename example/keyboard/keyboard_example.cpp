/**
 * Simple keyboard input example using Hohnor EventLoop
 * This example demonstrates how to use EventLoop with interactive keyboard input handling
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/io/FdUtils.h"
#include "hohnor/log/Logging.h"
#include <iostream>
#include <csignal>

using namespace Hohnor;

class KeyboardHandler {
private:
    std::shared_ptr<EventLoop> loop_;
    bool running_;
    
public:
    KeyboardHandler(std::shared_ptr<EventLoop> loop) : loop_(loop), running_(true) {}
    
    void onKeyPress(char key) {
        switch (key) {
            case 'q':
            case 'Q':
                std::cout << "\nQuitting application..." << std::endl;
                running_ = false;
                loop_->endLoop();
                break;
            case 'h':
            case 'H':
                printHelp();
                break;
            case '\n':
            case '\r':
                std::cout << "\nYou pressed Enter!" << std::endl;
                break;
            case 27: // ESC key
                std::cout << "\nESC key pressed!" << std::endl;
                break;
            default:
                if (key >= 32 && key <= 126) { // Printable ASCII characters
                    std::cout << "Key pressed: '" << key << "' (ASCII: " << (int)key << ")" << std::endl;
                } else {
                    std::cout << "Special key pressed (ASCII: " << (int)key << ")" << std::endl;
                }
                break;
        }
        std::cout << "Press 'h' for help, 'q' to quit: ";
        std::cout.flush();
    }
    
    void printHelp() {
        std::cout << "\n=== Keyboard Input Example Help ===" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  h/H - Show this help" << std::endl;
        std::cout << "  q/Q - Quit the application" << std::endl;
        std::cout << "  ESC - ESC key detection" << std::endl;
        std::cout << "  Any other key - Display key and ASCII code" << std::endl;
        std::cout << "===================================" << std::endl;
        std::cout << "Press 'h' for help, 'q' to quit: ";
        std::cout.flush();
    }
    
    bool isRunning() const { return running_; }
};

int main() {
    std::cout << "=== Hohnor EventLoop Keyboard Input Example ===" << std::endl;
    std::cout << "This example demonstrates interactive keyboard input handling" << std::endl;
    std::cout << "using the Hohnor EventLoop framework." << std::endl;
    std::cout << "===============================================" << std::endl;
    
    try {
        // Create the event loop
        auto loop = EventLoop::createEventLoop();
        
        // Create keyboard handler
        KeyboardHandler handler(loop);
        
        // Set up signal handling for graceful shutdown (Ctrl+C)
        loop->handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down gracefully..." << std::endl;
            loop->endLoop();
        });
        
        // Set up keyboard input handling
        loop->handleKeyboard([&handler](char key) {
            handler.onKeyPress(key);
        });
        
        // Show initial help
        handler.printHelp();
        
        // Start the event loop
        std::cout << "Starting event loop..." << std::endl;
        loop->loop();
        
        // Clean up keyboard handling
        loop->handleKeyboard(nullptr);
        
        std::cout << "Event loop ended. Goodbye!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}