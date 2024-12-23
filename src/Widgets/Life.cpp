#include "Life.h"
#include "OpenKNX.h"

// Constructor
WidgetLife::WidgetLife(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED), _lastUpdateTime(0), _display(nullptr)
{
    memset(grid, 0, sizeof(grid));
    memset(nextGrid, 0, sizeof(nextGrid));
}

void WidgetLife::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP(" Display is NULL.");
        return;
    }
    initializeGrid();
}

void WidgetLife::start()
{
    if (_state == WidgetState::RUNNING) return;

    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetLife::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetLife::pause()
{
    if (_state == WidgetState::RUNNING)
    {
        logInfoP("Pause...");
        _state = WidgetState::PAUSED;
    }
}

void WidgetLife::resume()
{
    if (_state == WidgetState::PAUSED)
    {
        logInfoP("Resume...");
        _state = WidgetState::RUNNING;
    }
}

void WidgetLife::loop()
{
    if (_state != WidgetState::RUNNING || !_display) return;

    const uint32_t UPDATE_INTERVAL = 100; // Update speed in milliseconds
    uint32_t currentTime = millis();

    if (currentTime - _lastUpdateTime >= UPDATE_INTERVAL)
    {
        updateGrid();
        drawGrid();
        _lastUpdateTime = currentTime;
    }
}

uint32_t WidgetLife::getDisplayTime() const { return _displayTime; }
WidgetsAction WidgetLife::getAction() const { return _action; }

void WidgetLife::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetLife::getDisplayModule() const { return _display; }

// Initialize grid randomly
void WidgetLife::initializeGrid()
{
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            grid[y][x] = random(0, 2); // Random 0 (dead) or 1 (alive)
        }
    }
}

// Update grid based on Game of Life rules
void WidgetLife::updateGrid()
{
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            uint8_t neighbors = 0;

            // Count neighbors
            for (int8_t dy = -1; dy <= 1; dy++)
            {
                for (int8_t dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0) continue;

                    int nx = (x + dx + GRID_WIDTH) % GRID_WIDTH;
                    int ny = (y + dy + GRID_HEIGHT) % GRID_HEIGHT;
                    neighbors += grid[ny][nx];
                }
            }

            // Apply rules
            if (grid[y][x] == 1 && (neighbors < 2 || neighbors > 3))
            {
                nextGrid[y][x] = 0; // Cell dies
            }
            else if (grid[y][x] == 0 && neighbors == 3)
            {
                nextGrid[y][x] = 1; // Cell becomes alive
            }
            else
            {
                nextGrid[y][x] = grid[y][x];
            }
        }
    }
    memcpy(grid, nextGrid, sizeof(grid)); // Copy next state to current state
}

// Draw grid to display
void WidgetLife::drawGrid()
{
    _display->display->clearDisplay();
    for (uint8_t y = 0; y < GRID_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < GRID_WIDTH; x++)
        {
            if (grid[y][x] == 1)
            {
                _display->display->drawPixel(x, y, WHITE);
            }
        }
    }
    _display->displayBuff();
}