#include "MatrixClassic.h"
#include "OpenKNX.h"

// CP437 character set: Classic Matrix-style characters
const char WidgetMatrixClassic::cp437[] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0xC4, 0xB1,
    0xB0, 0xB1, 0xB2, 0xB3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9};

WidgetMatrixClassic::WidgetMatrixClassic(uint32_t displayTime, WidgetFlags action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(intensity), 
      _display(nullptr), _lastUpdateScreenSaver(0), FallSpeed(0), _state(WidgetState::STOPPED)
{
    FallSpeed = map(_intensity, 1, 10, 150, 30); // Map intensity to speed
}

void WidgetMatrixClassic::setup()
{
    // logDebugP("Setup...");
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }
}

void WidgetMatrixClassic::start()
{
    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    randomSeed(analogRead(0));
    initMatrix();
}

void WidgetMatrixClassic::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetMatrixClassic::pause()
{
    logDebugP("Pause...");
    _state = WidgetState::PAUSED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetMatrixClassic::resume()
{
    logDebugP("Resume...");
    _state = WidgetState::RUNNING;
    randomSeed(analogRead(0));
    initMatrix();
}

void WidgetMatrixClassic::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateScreenSaver >= FallSpeed)
    {
        _lastUpdateScreenSaver = currentTime;
        updateMatrix();
    }
}

uint32_t WidgetMatrixClassic::getDisplayTime() const { return _displayTime; }
WidgetFlags WidgetMatrixClassic::getAction() const { return _action; }

void WidgetMatrixClassic::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetMatrixClassic::getDisplayModule() const { return _display; }

void WidgetMatrixClassic::initMatrix()
{
    for (int col = 0; col < maxColumns; col++)
    {
        for (int drop = 0; drop < maxDrops; drop++)
        {
            _MatrixDropPos[col][drop] = random(0, _display->GetDisplayHeight());
        }
    }
}

void WidgetMatrixClassic::updateMatrix()
{
    _display->display->clearDisplay();

    for (uint16_t x = 0; x < (maxColumns); x++)
    {
        for (uint16_t drop = 0; drop < maxDrops; drop++)
        {
            if (_MatrixDropPos[x][drop] < _display->GetDisplayHeight())
            {
                _MatrixDropPos[x][drop] += columnWidth;
            }
            else
            {
                _MatrixDropPos[x][drop] = 0; // Reset drop to top
            }

            // Draw character
            _display->display->setCursor(x * columnWidth, _MatrixDropPos[x][drop]);
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextSize(1);
            _display->display->write(cp437[random(0, sizeof(cp437))]);
        }
    }

    _display->displayBuff(); // Update display
}