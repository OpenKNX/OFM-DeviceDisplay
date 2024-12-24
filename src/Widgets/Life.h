#pragma once
#include "../Widget.h"

// WidgetLife class: Displays Conway's Game of Life as a screensaver.
class WidgetLife : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetLife"; }
    WidgetLife(uint32_t displayTime, WidgetsAction action);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return display time
    WidgetsAction getAction() const override;                                                            // Return widget action
    inline uint32_t setDisplayTime(uint32_t displayTime) override { return _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetsAction>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action & ~action); } // Remove an action from the widget


    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;

  private:
    WidgetState _state;         // Current state
    uint32_t _displayTime;      // Time to display widget
    WidgetsAction _action;      // Widget action
    uint32_t _lastUpdateTime;   // Last time the game was updated
    i2cDisplay *_display;       // Display module
    std::string _name = "Life"; // Name of the widget

    // Game of Life grid parameters
    static const uint8_t GRID_WIDTH = 128;     // Width of the grid (screen width)
    static const uint8_t GRID_HEIGHT = 64;     // Height of the grid (screen height)
    uint8_t grid[GRID_HEIGHT][GRID_WIDTH];     // Current grid state
    uint8_t nextGrid[GRID_HEIGHT][GRID_WIDTH]; // Next state

    void initializeGrid(); // Randomly initialize the grid
    void updateGrid();     // Update the grid based on Game of Life rules
    void drawGrid();       // Draw the grid to the display
};