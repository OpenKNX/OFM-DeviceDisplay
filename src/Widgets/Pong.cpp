#include "Pong.h"

WidgetPong::WidgetPong(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _state(STOPPED), _display(nullptr),
      paddleLeftY(0), paddleRightY(0), ballX(0), ballY(0), ballSpeedX(0), ballSpeedY(0) {}

void WidgetPong::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
    logDebugP("WidgetPong: Display-Modul gesetzt.");
}
i2cDisplay *WidgetPong::getDisplayModule() const { return _display; }

void WidgetPong::initSettings() // Initialize the screensaver default settings
{
    logInfoP("WidgetPong: Initialisiere Screensaver...");
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();
    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();

    paddleLeftY = SCREEN_HEIGHT / 2 - 5; // Center the paddle
    paddleRightY = SCREEN_HEIGHT / 2 - 5;
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    ballSpeedX = -1; // Horizontal movement
    ballSpeedY = 1;  // Vertical movement
    logInfoP("WidgetPong: Screensaver initialisiert.");
}

void WidgetPong::setup()
{
    logInfoP("WidgetPong: Setup gestartet...");
    if (_display == nullptr)
    {
        logErrorP("WidgetPong: Display ist NULL.");
        return;
    }
    initSettings();
    logInfoP("WidgetPong: Setup abgeschlossen.");
}

void WidgetPong::start() // Start the widget and display the WidgetPong screensaver
{
    if (_state == STOPPED)
    {
        logInfoP("WidgetPong: Starting...");
        _state = RUNNING;
        _lastUpdateTime = millis();
        initSettings(); // Reset screensaver state
        logInfoP("WidgetPong: Started.");
    }
}

void WidgetPong::stop() // Stop the WidgetPong screensaver and clear the display
{
    if (_state != STOPPED)
    {
        logInfoP("WidgetPong: Stop...");
        _state = STOPPED;
        _display->display->clearDisplay();
        _display->displayBuff(); // Display leeren
        logInfoP("WidgetPong: Gestoppt.");
    }
}

void WidgetPong::pause() // Pause the WidgetPong screensaver
{
    if (_state == RUNNING)
    {
        logInfoP("WidgetPong: Pausieren...");
        _state = PAUSED; // Set the state to paused
                         // No need to reset the current WidgetPong pos and settings. Just pause WidgetPong
        logInfoP("WidgetPong: Pausiert.");
    }
}

void WidgetPong::resume() // Resume the WidgetPong screensaver
{
    if (_state == PAUSED)
    {
        logInfoP("WidgetPong: Fortsetzen...");
        _state = RUNNING;
        _lastUpdateTime = millis(); // Reset last update time
        logInfoP("WidgetPong: Fortgesetzt.");
    }
}

void WidgetPong::loop() // Loop is called every second to update the screensaver
{
    if (_state != RUNNING)
    {
        return; // Noting to do here. WidgetPong is not running
    }

    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime >= 50)
    {                      // Update every second
        drawScreensaver(); // Now draw the screensaver data to send it to the display
        _lastUpdateTime = currentTime;
    }
}

uint32_t WidgetPong::getDisplayTime() const { return _displayTime; } // Retrun the display time in ms
WidgetsAction WidgetPong::getAction() const { return _action; }      // Return the widget action

void WidgetPong::drawScreensaver() // Draw the WidgetPong screensaver
{
    if (_display == nullptr)
    {
        logErrorP("WidgetPong: Display ist NULL.");
        return;
    }
    logDebugP("WidgetPong: Screensaver aktualisieren...");
    const uint8_t PADDLE_WIDTH = 2;   // Paddle-Breite
    const uint8_t PADDLE_HEIGHT = 10; // Paddle-Höhe
    const uint8_t BALL_SIZE = 2;      // Ballgröße
    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();

    // Ball movement
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // Ball collides with top and bottom walls
    if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE)
    {
        ballSpeedY = -ballSpeedY; // Reverse ball direction
    }

    // Ball collides with paddles
    if (ballX <= PADDLE_WIDTH && ballY >= paddleLeftY && ballY <= paddleLeftY + PADDLE_HEIGHT)
    {
        ballSpeedX = -ballSpeedX;
    }
    if (ballX >= SCREEN_WIDTH - PADDLE_WIDTH - BALL_SIZE && ballY >= paddleRightY && ballY <= paddleRightY + PADDLE_HEIGHT)
    {
        ballSpeedX = -ballSpeedX;
    }

    // Ball collides with left and right walls (reset ball)
    if (ballX <= 0 || ballX >= SCREEN_WIDTH)
    {
        ballX = SCREEN_WIDTH / 2;
        ballY = SCREEN_HEIGHT / 2;
        ballSpeedX = -ballSpeedX;
        ballSpeedY = (random(0, 2) == 0 ? 1 : -1) * random(1, 3); // Random direction
    }

    // Paddles follow the ball
    if (ballY < paddleLeftY + PADDLE_HEIGHT / 2)
    {
        paddleLeftY -= 1;
    }
    else if (ballY > paddleLeftY + PADDLE_HEIGHT / 2)
    {
        paddleLeftY += 1;
    }
    if (ballY < paddleRightY + PADDLE_HEIGHT / 2)
    {
        paddleRightY -= 1;
    }
    else if (ballY > paddleRightY + PADDLE_HEIGHT / 2)
    {
        paddleRightY += 1;
    }

    // Constrain paddle movement to the screen area
    paddleLeftY = constrain(paddleLeftY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    paddleRightY = constrain(paddleRightY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);

    // Clear the display
    _display->display->clearDisplay();
    _display->display->fillRect(0, paddleLeftY, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);                            // Left Paddle
    _display->display->fillRect(SCREEN_WIDTH - PADDLE_WIDTH, paddleRightY, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE); // Right Paddle
    _display->display->fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, WHITE);                                     // The Ball
    _display->displayBuff();                                                                                    // Send the data to the display
    logDebugP("WidgetPong: Screensaver aktualisiert.");
}