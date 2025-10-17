#include "Console.h"
#include "OpenKNX.h"

// Static instance for callback hook
WidgetConsole* WidgetConsole::instance = nullptr;

WidgetConsole::WidgetConsole(uint32_t displayTime, WidgetFlags action, uint8_t maxLines)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED), _display(nullptr),
      _maxLines(maxLines), _scrollOffset(0), _textSize(1), _autoScroll(true), _showTimestamps(true),
      _minLogLevel(INFO), _lastUpdateTime(0)
{
    instance = this;  // Set singleton instance
}

void WidgetConsole::setDisplayModule(i2cDisplay* displayModule)
{
    _display = displayModule;
}

i2cDisplay* WidgetConsole::getDisplayModule() const
{
    return _display;
}

void WidgetConsole::setup()
{
    if (_state == WidgetState::RUNNING)
        return;

    logDebugP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }

    // Initialize with system info
    addLine("=== OpenKNX Console ===", INFO);
#ifdef DEVICE_ID
    addLine(std::string("Device: ") + DEVICE_ID, INFO);
#endif
    addLine("Addr: " + openknx.info.humanIndividualAddress(), INFO);
    addLine("SN: " + openknx.info.humanSerialNumber(), INFO);
    addLine("Ready.", INFO);
}

void WidgetConsole::start()
{
    if (_state == WidgetState::STOPPED)
    {
        logDebugP("Start...");
        _state = WidgetState::RUNNING;
        _lastUpdateTime = millis();
        drawConsole();
    }
}

void WidgetConsole::stop()
{
    if (_state != WidgetState::STOPPED)
    {
        logDebugP("Stop...");
        _state = WidgetState::STOPPED;
    }
}

void WidgetConsole::pause()
{
    if (_state == WidgetState::RUNNING)
    {
        logDebugP("Pause...");
        _state = WidgetState::PAUSED;
    }
}

void WidgetConsole::resume()
{
    if (_state == WidgetState::PAUSED)
    {
        logDebugP("Resume...");
        _state = WidgetState::RUNNING;
        _lastUpdateTime = millis();
    }
}

void WidgetConsole::loop()
{
    if (_state != WidgetState::RUNNING)
        return;

    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime >= 100)  // Update 10x per second
    {
        drawConsole();
        _lastUpdateTime = currentTime;
    }
}

uint32_t WidgetConsole::getDisplayTime() const
{
    return _displayTime;
}

WidgetFlags WidgetConsole::getAction() const
{
    return _action;
}

void WidgetConsole::addLine(const std::string& text, LogLevel level)
{
    if (level < _minLogLevel)
        return;  // Filter by log level

    std::string formattedLine;

    // Add timestamp if enabled
    if (_showTimestamps)
    {
        uint16_t days = 0, hours = 0, minutes = 0, seconds = 0;
        fetchTime(days, hours, minutes, seconds);

        char timestamp[12];
        snprintf(timestamp, sizeof(timestamp), "%02u:%02u:%02u", hours, minutes, seconds);
        formattedLine = std::string(timestamp) + " ";
    }

    // Add level prefix
    formattedLine += getLevelPrefix(level);
    formattedLine += text;

    // Add to buffer
    _lines.push_back(formattedLine);

    // Limit buffer size (keep max 100 lines)
    if (_lines.size() > 100)
    {
        _lines.pop_front();
    }

    // Auto-scroll to newest
    if (_autoScroll && _lines.size() > _maxLines)
    {
        _scrollOffset = _lines.size() - _maxLines;
    }

    // Trigger redraw if running
    if (_state == WidgetState::RUNNING)
    {
        drawConsole();
    }
}

void WidgetConsole::clear()
{
    _lines.clear();
    _scrollOffset = 0;
    if (_state == WidgetState::RUNNING)
    {
        drawConsole();
    }
}

void WidgetConsole::scrollUp()
{
    if (_scrollOffset > 0)
    {
        _scrollOffset--;
        drawConsole();
    }
}

void WidgetConsole::scrollDown()
{
    if (_scrollOffset + _maxLines < _lines.size())
    {
        _scrollOffset++;
        drawConsole();
    }
}

void WidgetConsole::fetchTime(uint16_t& days, uint16_t& hours, uint16_t& minutes, uint16_t& seconds)
{
    if (openknx.time.isValid())
    {
        auto time = openknx.time.getUtcTime();
        hours = time.hour;
        minutes = time.minute;
        seconds = time.second;
        days = 0;
    }
    else
    {
        uint32_t uptimeSecs = uptime();
        seconds = uptimeSecs % 60;
        uptimeSecs /= 60;
        minutes = uptimeSecs % 60;
        uptimeSecs /= 60;
        hours = uptimeSecs % 24;
        uptimeSecs /= 24;
        days = uptimeSecs;
    }
}

const char* WidgetConsole::getLevelPrefix(LogLevel level)
{
    switch (level)
    {
        case DEBUG:
            return "[D] ";
        case INFO:
            return "[I] ";
        case WARNING:
            return "[W] ";
        case ERROR:
            return "[E] ";
        case FATAL:
            return "[F] ";
        default:
            return "";
    }
}

void WidgetConsole::drawConsole()
{
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }

    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();
    const uint8_t FONT_HEIGHT = 8 * _textSize;
    const uint8_t CHAR_WIDTH = 6 * _textSize;

    _display->display->clearDisplay();
    _display->display->setTextSize(_textSize);
    _display->display->setTextColor(WHITE);
    _display->display->setTextWrap(false);

    // Calculate how many lines fit on screen
    uint8_t visibleLines = SCREEN_HEIGHT / FONT_HEIGHT;
    if (visibleLines > _maxLines)
        visibleLines = _maxLines;

    // Draw lines from buffer
    for (uint8_t i = 0; i < visibleLines && (_scrollOffset + i) < _lines.size(); i++)
    {
        std::string line = _lines[_scrollOffset + i];

        // Truncate line if too long
        uint8_t maxChars = SCREEN_WIDTH / CHAR_WIDTH;
        if (line.length() > maxChars)
        {
            line = line.substr(0, maxChars - 3) + "...";
        }

        _display->display->setCursor(0, i * FONT_HEIGHT);
        _display->display->print(line.c_str());
    }

    // Draw scroll indicator (bottom right) if more lines available
    if (_lines.size() > visibleLines)
    {
        char scrollInfo[16];
        snprintf(scrollInfo, sizeof(scrollInfo), "%u/%u", _scrollOffset + visibleLines,
                 (uint16_t)_lines.size());

        int16_t textWidth = strlen(scrollInfo) * CHAR_WIDTH;
        _display->display->setCursor(SCREEN_WIDTH - textWidth, SCREEN_HEIGHT - FONT_HEIGHT);
        _display->display->print(scrollInfo);
    }

    _display->displayBuff();
}