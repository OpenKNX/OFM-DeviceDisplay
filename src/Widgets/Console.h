#ifdef DEVICE_DISPLAY_MODULE
#pragma once

#include "../Widget.h"
#include "../devices/i2cDisplay.h"
#include <deque>
#include <string>

/**
 * @brief Console widget for displaying scrolling log messages
 */
class WidgetConsole : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetConsole"; }
    
    enum LogLevel
    {
        ALL = 0,
        DEBUG = 1,
        INFO = 2,
        WARNING = 3,
        ERROR = 4,
        FATAL = 5
    };

    WidgetConsole(  uint32_t displayTime = 60000, 
                    //WidgetFlags action = WidgetFlags::DefaultWidget,
                    WidgetFlags action = WidgetFlags::NoAction, // Default action
                    uint8_t maxLines = 8
                  );

    // Widget interface implementation
    void setDisplayModule(i2cDisplay* displayModule) override;
    i2cDisplay* getDisplayModule() const override;
    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;
    uint32_t getDisplayTime() const override;
    void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }
    void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }               // Set the widget action
    void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget
    WidgetFlags getAction() const override;
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string& name) override { _name = name; } // Set the name of the widget


    // Console-specific methods
    void addLine(const std::string& text, LogLevel level = INFO);
    void clear();
    void setAutoScroll(bool enabled) { _autoScroll = enabled; }
    void setTextSize(uint8_t size) { _textSize = (size >= 1 && size <= 2) ? size : 1; }
    void setLogLevel(LogLevel minLevel) { _minLogLevel = minLevel; }
    void toggleTimestamps(bool enabled) { _showTimestamps = enabled; }
    void scrollUp();
    void scrollDown();

    // Callback hook for external log systems (prepared for future use)
    static WidgetConsole* instance;  // Singleton for callback access

  private:
    void drawConsole();
    void fetchTime(uint16_t& days, uint16_t& hours, uint16_t& minutes, uint16_t& seconds);
    const char* getLevelPrefix(LogLevel level);

    uint32_t _displayTime;
    WidgetFlags _action;
    WidgetState _state;
    i2cDisplay* _display;
    std::string _name = "Console";

    std::deque<std::string> _lines;  // Log lines buffer
    uint8_t _maxLines;               // Max lines to display on screen
    uint8_t _scrollOffset;           // Current scroll position
    uint8_t _textSize;               // Text size (1 or 2)
    bool _autoScroll;                // Auto-scroll to newest
    bool _showTimestamps;            // Show timestamps prefix
    LogLevel _minLogLevel;           // Minimum log level to display
    uint32_t _lastUpdateTime;        // For periodic refresh
};
#endif // DEVICE_DISPLAY_MODULE