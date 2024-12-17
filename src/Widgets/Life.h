#pragma once
#include "../Widget.h"

// WidgetLife class: Displays Conway's Game of Life as a screensaver.
class WidgetLife : public Widget
{
  public:
    WidgetLife(uint32_t displayTime, WidgetsAction action);

    void start() override;  // Start the Game of Life widget
    void stop() override;   // Stop the Game of Life and clear the display
    void pause() override;  // Pause the Game of Life
    void resume() override; // Resume the Game of Life
    void setup() override;  // Setup the widget
    void loop() override;   // Update the game logic and display

    uint32_t getDisplayTime() const override; // Return display time
    WidgetsAction getAction() const override; // Return widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;

  private:
    enum WidgetLifeState
    {
        STOPPED,
        RUNNING,
        PAUSED
    };

    WidgetLifeState _state;      // Current state
    uint32_t _displayTime;       // Time to display widget
    WidgetsAction _action;       // Widget action
    uint32_t _lastUpdateTime;    // Last time the game was updated
    i2cDisplay *_display;        // Display module

    // Game of Life grid parameters
    static const uint8_t GRID_WIDTH = 128; // Width of the grid (screen width)
    static const uint8_t GRID_HEIGHT = 64; // Height of the grid (screen height)
    uint8_t grid[GRID_HEIGHT][GRID_WIDTH]; // Current grid state
    uint8_t nextGrid[GRID_HEIGHT][GRID_WIDTH]; // Next state

    void initializeGrid();  // Randomly initialize the grid
    void updateGrid();      // Update the grid based on Game of Life rules
    void drawGrid();        // Draw the grid to the display
};