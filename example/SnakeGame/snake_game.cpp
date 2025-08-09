/**
 * Simple Snake Game using Hohnor EventLoop and ncurses
 * This example demonstrates a classical snake game with keyboard input handling
 */

#include "hohnor/core/EventLoop.h"
#include "hohnor/core/Signal.h"
#include "hohnor/log/Logging.h"
#include "hohnor/core/Timer.h"
#include <ncurses.h>
#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <csignal>

using namespace Hohnor;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

enum Direction {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

class SnakeGame {
private:
    std::shared_ptr<EventLoop> loop_;
    std::deque<Point> snake_;
    Point food_;
    Direction direction_;
    Direction nextDirection_;
    int score_;
    int gameWidth_;
    int gameHeight_;
    bool gameOver_;
    bool paused_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> widthDist_;
    std::uniform_int_distribution<int> heightDist_;
    
    // Game timing
    std::shared_ptr<TimerHandler> gameTimer_;
    static constexpr double GAME_SPEED = 0.15; // seconds between moves
    
public:
    SnakeGame(std::shared_ptr<EventLoop> loop) 
        : loop_(loop), direction_(RIGHT), nextDirection_(RIGHT), 
          score_(0), gameWidth_(30), gameHeight_(15), gameOver_(false), paused_(false),
          rng_(std::random_device{}()), 
          widthDist_(1, gameWidth_ - 2), 
          heightDist_(1, gameHeight_ - 2) {
        
        // Initialize snake in the middle
        snake_.push_back(Point(gameWidth_ / 2, gameHeight_ / 2));
        snake_.push_back(Point(gameWidth_ / 2 - 1, gameHeight_ / 2));
        snake_.push_back(Point(gameWidth_ / 2 - 2, gameHeight_ / 2));
        
        generateFood();
    }
    
    ~SnakeGame() {
        cleanup();
    }
    
    void initialize() {
        // Initialize ncurses
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        keypad(stdscr, TRUE);

        // Initialize colors
        if (has_colors()) {
            start_color();
            use_default_colors(); // Use terminal's default background
            init_pair(1, COLOR_GREEN, -1); // Snake body
            init_pair(2, COLOR_YELLOW, -1); // Snake head
            init_pair(3, COLOR_RED, -1);    // Food
            init_pair(4, COLOR_BLUE, -1);   // Border
            init_pair(5, COLOR_CYAN, -1);   // Text
        }
        
        // Check terminal size
        int termHeight, termWidth;
        getmaxyx(stdscr, termHeight, termWidth);
        
        if (termWidth < gameWidth_ + 2 || termHeight < gameHeight_ + 5) {
            cleanup();
            std::cerr << "Terminal too small! Need at least "
                      << (gameWidth_ + 2) << "x" << (gameHeight_ + 5) << std::endl;
            loop_->endLoop();
            return;
        }
        
        // Set up game timer
        gameTimer_ = loop_->addTimer([this]() {
            if (!gameOver_ && !paused_) {
                gameLoop();
            }
        }, Timestamp::now(), GAME_SPEED);
        
        draw();
    }
    
    void cleanup() {
        if (gameTimer_) {
            gameTimer_->disable();
        }
        endwin();
    }
    
    void onKeyPress(char key) {
        switch (key) {
            case 'q':
            case 'Q':
                gameOver_ = true;
                loop_->endLoop();
                break;
            case 'p':
            case 'P':
                paused_ = !paused_;
                break;
            case 'r':
            case 'R':
                if (gameOver_) {
                    restart();
                }
                break;
            // Arrow keys and WASD
            case 'w':
            case 'W':
                if (direction_ != DOWN) nextDirection_ = UP;
                break;
            case 's':
            case 'S':
                if (direction_ != UP) nextDirection_ = DOWN;
                break;
            case 'a':
            case 'A':
                if (direction_ != RIGHT) nextDirection_ = LEFT;
                break;
            case 'd':
            case 'D':
                if (direction_ != LEFT) nextDirection_ = RIGHT;
                break;
        }
    }
    
    void gameLoop() {
        if (gameOver_) return;
        
        direction_ = nextDirection_;
        
        // Calculate new head position
        Point newHead = snake_.front();
        switch (direction_) {
            case UP:    newHead.y--; break;
            case DOWN:  newHead.y++; break;
            case LEFT:  newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }
        
        // Check wall collision
        if (newHead.x <= 0 || newHead.x >= gameWidth_ - 1 || 
            newHead.y <= 0 || newHead.y >= gameHeight_ - 1) {
            gameOver_ = true;
            draw();
            return;
        }
        
        // Check self collision
        for (const auto& segment : snake_) {
            if (newHead == segment) {
                gameOver_ = true;
                draw();
                return;
            }
        }
        
        // Add new head
        snake_.push_front(newHead);
        
        // Check food collision
        if (newHead == food_) {
            score_++;
            generateFood();
        } else {
            // Remove tail if no food eaten
            snake_.pop_back();
        }
        
        draw();
    }
    
    void generateFood() {
        do {
            food_.x = widthDist_(rng_);
            food_.y = heightDist_(rng_);
        } while (isSnakePosition(food_));
    }
    
    bool isSnakePosition(const Point& pos) {
        for (const auto& segment : snake_) {
            if (segment == pos) {
                return true;
            }
        }
        return false;
    }
    
    void restart() {
        snake_.clear();
        snake_.push_back(Point(gameWidth_ / 2, gameHeight_ / 2));
        snake_.push_back(Point(gameWidth_ / 2 - 1, gameHeight_ / 2));
        snake_.push_back(Point(gameWidth_ / 2 - 2, gameHeight_ / 2));
        
        direction_ = RIGHT;
        nextDirection_ = RIGHT;
        score_ = 0;
        gameOver_ = false;
        paused_ = false;
        
        generateFood();
        draw();
    }
    
    void draw() {
        clear();
        
        // Draw border
        if (has_colors()) attron(COLOR_PAIR(4)); // Blue for border
        for (int x = 0; x < gameWidth_; x++) {
            mvaddch(0, x, '#');
            mvaddch(gameHeight_ - 1, x, '#');
        }
        for (int y = 0; y < gameHeight_; y++) {
            mvaddch(y, 0, '#');
            mvaddch(y, gameWidth_ - 1, '#');
        }
        if (has_colors()) attroff(COLOR_PAIR(4));

        // Draw snake
        for (size_t i = 0; i < snake_.size(); i++) {
            const Point& segment = snake_[i];
            if (i == 0) {
                if (has_colors()) attron(COLOR_PAIR(2)); // Yellow for head
                mvaddch(segment.y, segment.x, '@'); // Head
                if (has_colors()) attroff(COLOR_PAIR(2));
            } else {
                if (has_colors()) attron(COLOR_PAIR(1)); // Green for body
                mvaddch(segment.y, segment.x, 'o'); // Body
                if (has_colors()) attroff(COLOR_PAIR(1));
            }
        }
        
        // Draw food
        if (has_colors()) attron(COLOR_PAIR(3)); // Red for food
        mvaddch(food_.y, food_.x, '*');
        if (has_colors()) attroff(COLOR_PAIR(3));
        
        // Draw score and status
        if (has_colors()) attron(COLOR_PAIR(5)); // Cyan for text
        mvprintw(gameHeight_ + 1, 0, "Score: %d", score_);
        mvprintw(gameHeight_ + 2, 0, "Controls: WASD to move, P to pause, Q to quit");
        
        if (gameOver_) {
            mvprintw(gameHeight_ + 3, 0, "GAME OVER! Press R to restart or Q to quit");
        } else if (paused_) {
            mvprintw(gameHeight_ + 3, 0, "PAUSED - Press P to continue");
        }
        if (has_colors()) attroff(COLOR_PAIR(5));
        
        refresh();
    }
    
    bool isGameOver() const { return gameOver_; }
};

int main() {
    std::cout << "=== Hohnor Snake Game ===" << std::endl;
    std::cout << "Starting snake game..." << std::endl;
    
    try {
        // Create the event loop
        auto loop = EventLoop::createEventLoop();

        // Create snake game
        SnakeGame game(loop);

        // Set up signal handling for graceful shutdown (Ctrl+C)
        loop->handleSignal(SIGINT, SignalAction::Handled, [&]() {
            std::cout << "\nReceived SIGINT (Ctrl+C), shutting down gracefully..." << std::endl;
            loop->endLoop();
        });
        
        // Set up keyboard input handling
        loop->handleKeyboard([&game](char key) {
            game.onKeyPress(key);
        });
        
        // Initialize the game
        game.initialize();
        
        // Start the event loop
        loop->loop();

        // Clean up keyboard handling
        loop->handleKeyboard(nullptr);

        std::cout << "Game ended. Thanks for playing!" << std::endl;

        usleep(10000);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}