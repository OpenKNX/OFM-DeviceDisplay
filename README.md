## ToDo:

* [x] Implement centralized button handling system
* [x] Implement PowerSave system with multiple modes
* [x] Improve the Widget Manager with state machine and priority system
* [x] Provide Custom Widget examples
* [x] Finalize the documentation
* [ ] Conduct an overall performance check

### Planned Features (Phase 2)
- [ ] **Button Combinations**: UP+DOWN simultaneously for special actions
- [ ] **Long-Press Actions**: OK 5s → ProgMode toggle, configurable shortcuts
- [ ] **Global Button Patterns**: Konami-code style sequences (LEFT, LEFT, UP, DOWN, OK)
- [ ] **ETS Application**: Configuration via KNX parameters

### **Info:**

- **Implementation State:** Active Development / Testing
- **Documentation State:** DRAFT
---

# OFM-DeviceDisplay

`OFM-DeviceDisplay` is a comprehensive, hardware-agnostic library for managing displays on OpenKNX devices. While currently optimized for i2c displays (Adafruit SSD1306 / SSD1315 compatible), the modular **WidgetsManager** core can be adapted for other display types (TFT, e-Paper, TouchDisplay, etc.).

This library features a sophisticated widget management system, centralized button handling, power-save modes, and support for dynamic content including text, icons, QR codes, and varous animations.


## Architecture Overview

```
┌────────────────────────────────────────────────────────────────────┐
│                         DeviceDisplay                              │
│  (Hardware-Specific: Button GPIO, Display Init, Main Loop)         │
│                                                                    │
│  ┌─────────────────────┐  ┌─────────────────────────────────────┐  │
│  │  Button Manager     │  │  i2cDisplay (Hardware Facade)       │  │
│  │  - GPIO Reading     │  │  - SSD1306 Driver Wrapper           │  │
│  │  - Event Creation   │  │  - Brightness Control (VCOM)        │  │
│  │  - LongPress Track  │  │  - Display On/Off                   │  │
│  └──────────┬──────────┘  └──────────┬──────────────────────────┘  │
│             │                        │                             │
│             │ ButtonEvent            │ Display Commands            │
│             ▼                        ▼                             │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    WidgetsManager                            │  │
│  │         (Hardware-Agnostic Widget Orchestration)             │  │
│  │                                                              │  │
│  │  ┌───────────────────────────────────────────────────────┐   │  │
│  │  │  State Machine                                        │   │  │
│  │  │  STARTUP → IDLE → PRIORITY/BACKGROUND/NORMAL/DEFAULT  │   │  │
│  │  └───────────────────────────────────────────────────────┘   │  │
│  │                                                              │  │
│  │  ┌───────────────────────────────────────────────────────┐   │  │
│  │  │  PowerSave System                                     │   │  │
│  │  │  ACTIVE → DIMMED → SCREENSAVER → SLEEP → OFF          │   │  │
│  │  └───────────────────────────────────────────────────────┘   │  │
│  │                                                              │  │
│  │  ┌───────────────────────────────────────────────────────┐   │  │
│  │  │  Widget Priority System                               │   │  │
│  │  │  PRIORITY > BACKGROUND > NORMAL > DEFAULT             │   │  │
│  │  └───────────────────────────────────────────────────────┘   │  │
│  │                                                              │  │
│  │  ┌───────────────────────────────────────────────────────┐   │  │
│  │  │  Button Routing                                       │   │  │
│  │  │  getActiveButtonWidget() → Forward Events             │   │  │
│  │  └───────────────────────────────────────────────────────┘   │  │
│  └───────────────────────────────┬──────────────────────────────┘  │
│                                  │                                 │
│                                  │ Widget Control                  │
│                                  ▼                                 │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                      Widget Queue                            │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐         │  │
│  │  │ PRIORITY │ │BACKGROUND│ │  NORMAL  │ │ DEFAULT  │         │  │
│  │  │ ProgMode │ │   Menu   │ │SysInfo   │ │Clock/QR  │         │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘         │  │
│  │                                                              │  │
│  │  Screensaver: MatrixClassic (separate, auto-start)           │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
```

### Key Design Principles

1. **Hardware Abstraction**: WidgetsManager is display-agnostic, only requires:
   - `Widget::setDisplayModule(DisplayInterface*)` 
   - Display interface with `clearDisplay()`, `draw*()`, `displayBuff()`

2. **Modular Components**:
   - **DeviceDisplay**: Hardware-specific (GPIO, i2c init, main loop)
   - **WidgetsManager**: Core orchestration (state machine, power-save, widget lifecycle)
   - **i2cDisplay**: Hardware facade (SSD1306 driver wrapper)
   - **Widgets**: Self-contained display logic (Menu, Clock, QR, animations)

3. **Event-Driven**: Button events flow from hardware → manager → active widget

4. **Extensible**: Add new display types by implementing display interface

---

## System Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    User Interaction Flow                        │
└─────────────────────────────────────────────────────────────────┘

  Button Press
       │
       ▼
┌──────────────────┐
│ DeviceDisplay    │
│ processButtons() │
└────────┬─────────┘
         │
         │ Creates ButtonEvent(type, action, timestamp)
         │
         ▼
┌─────────────────────────┐
│ WidgetsManager          │
│ getActiveButtonWidget() │◄─── Priority Check:
└────────┬────────────────┘     1. PRIORITY widgets
         │                      2. BACKGROUND (active)
         │                      3. BACKGROUND (inactive)
         │                      4. NORMAL widgets
         │                      5. None → Wake-Up
         │
         ├─── Widget Found? ───┐
         │                     │
         ▼ YES                 ▼ NO
┌──────────────────┐    ┌──────────────────┐
│ Widget           │    │ Wake-Up Handler  │
│ handleButton     │    │ wakeUpDisplay()  │
│ Event()          │    │ + activateMenu() │
└────────┬─────────┘    │ (if SELECT)      │
         │              └──────────────────┘
         │ Event Handled?
         │
         ▼ YES
┌──────────────────┐
│ WidgetsManager   │
│ userInteraction()│
│ → Reset Timer    │
│ → Wake Display   │
└──────────────────┘


┌─────────────────────────────────────────────────────────────────┐
│                    PowerSave State Flow                         │
└─────────────────────────────────────────────────────────────────┘

     User Active
          │
          ▼
    ┌──────────┐  dimTimeout (30s)
    │  ACTIVE  │─────────────────────┐
    │ 100% Br. │                     │
    └────▲─────┘                     │
         │                           ▼
         │ userInteraction()   ┌──────────┐  screenSaverTimeout (60s)
         │                     │  DIMMED  │─────────────────────┐
         │                     │  30% Br. │                     │
         │                     └──────────┘                     │
         │                                                      ▼
         │                                            ┌──────────────────┐
         │                                            │  SCREENSAVER     │
         │                                            │  Animation runs  │
         │                                            └────────┬─────────┘
         │                                                     │
         │                                   sleepTimeout      │
         │                                   (5 min)           │
         │                                                     ▼
         │                                            ┌──────────────────┐
         │                                            │      SLEEP       │
         │                                            │  Display OFF     │
         │                                            └────────┬─────────┘
         │                                                     │
         │                                   offTimeout        │
         │                                   (disabled)        │
         │                                                     ▼
         │                                            ┌──────────────────┐
         │                                            │       OFF        │
         └────────────────────────────────────────────│  Complete OFF    │
                        Any Button Press              └──────────────────┘


┌─────────────────────────────────────────────────────────────────┐
│                  Widget State Machine Flow                      │
└─────────────────────────────────────────────────────────────────┘

    Power On
        │
        ▼
   ┌─────────┐  BootLogo complete
   │ STARTUP │──────────────────┐
   └─────────┘                  │
                                ▼
                          ┌──────────┐
                          │   IDLE   │◄─────────┐
                          └────┬─────┘          │
                               │                │
        ┌──────────────────────┼────────────────┼────────────────┐
        │                      │                │                │
        │ Priority Widget      │ Background     │ Default        │
        │ (ProgMode)           │ Widget         │ Widget         │
        │                      │ (Menu+         │ Rotation       │
        │                      │ DisplayEnabled)│                │
        ▼                      ▼                ▼                │
   ┌──────────┐          ┌──────────┐    ┌──────────┐            │
   │ PRIORITY │          │BACKGROUND│    │ DEFAULT  │            │
   │          │          │          │    │          │            │
   └────┬─────┘          └────┬─────┘    └────┬─────┘            │
        │                     │               │                  │
        │ Widget stops        │ Timeout       │ No interaction   │
        │                     │               │                  │
        └─────────────────────┴───────────────┴──────────────────┘
```

---

## Features

### Core Features
- **Hardware-Agnostic WidgetsManager**: Core logic can be adapted for any display type
- **State Machine**: STARTUP → IDLE → PRIORITY/BACKGROUND/NORMAL/DEFAULT
- **Widget Priority System**: PRIORITY > BACKGROUND > NORMAL > DEFAULT
- **Centralized Button Handling**: Event-driven button system with PRESS, LONG_PRESS, VERY_LONG_PRESS
- **PowerSave Modes**: Automatic power management (ACTIVE → DIMMED → SCREENSAVER → SLEEP → OFF)
- **Background Widgets**: Widgets that run continuously (e.g., Menu)
- **Priority Widgets**: High-priority widgets that override all others (e.g., ProgMode)
- **Screensaver Support**: Dedicated screensaver widget with automatic activation

### Display Features (i2c SSD1306)
- **Multiple Widget Types**: Boot logo, menu, clock, QR code, system info, animations
- **Dynamic Text and Alignment**: Customizable text alignment, size, color, and scrolling
- **Icons and QR Codes**: Built-in support for icons and QR code generation
- **Animated Widgets**: Matrix, rain, starfield, fireworks, 3D cube, Pong, Game of Life
- **Brightness Control**: Automatic brightness adjustment with VCOM register support (0-100%)

### Button System
- **Smart Wake-Up**: Any button wakes display, SELECT activates menu directly
- **Button Events**: Type-safe event system with timestamps
- **LongPress Detection**: Automatic detection of short (>500ms) and very long (>5000ms) presses
- **Widget Button Input**: Widgets opt-in to receive button events via `WantsButtonInput` flag

### Power Management
- **Configurable Timeouts**: Dim, screensaver, sleep, off (all configurable)
- **Brightness Control**: Smooth transitions between brightness levels
- **User Interaction Tracking**: Automatic reset on any button press or widget action
- **Power Save Callbacks**: Monitor state transitions

---

## Widget Management System

### Widget States
```
STOPPED    → Widget is inactive
BACKGROUND → Widget runs in background (e.g., Menu)
RUNNING    → Widget is active and displayed
PAUSED     → Widget is temporarily paused (by PRIORITY widget)
```

### Widget Priorities
1. **PRIORITY Widgets**: Displayed immediately, pause all other widgets (e.g., ProgMode)
2. **BACKGROUND Widgets**: Run continuously, activated by `DisplayEnabled` flag (e.g., Menu)
3. **NORMAL Widgets**: Standard widgets in queue
4. **DEFAULT Widgets**: Shown when no other widgets are active (e.g., Clock, QRCode)

### Widget Flags
- **`StatusWidget`**: High-priority, shown immediately
- **`Background`**: Runs continuously in background
- **`DisplayEnabled`**: Makes background widget visible
- **`AutoRemove`**: Automatically removed after display
- **`DefaultWidget`**: Shown during idle state
- **`WantsButtonInput`**: Widget receives button events

### Manager States
```
STARTUP    → Initial boot sequence
IDLE       → No active widgets, waiting for events
PRIORITY   → Priority widget active (ProgMode, StatusWidgets)
BACKGROUND → Background widget active (Menu with DisplayEnabled)
NORMAL     → Normal widget queue processing
DEFAULT    → Default widget rotation (Clock/QRCode)
```

---

## PowerSave System

### Power Save Modes
```
ACTIVE      → Normal operation (100% brightness)
DIMMED      → Reduced brightness (configurable, default 30%)
SCREENSAVER → Display shows screensaver animation
SLEEP       → Display off, widget manager paused
OFF         → Complete power off (optional)
```

### Configuration Example
```cpp
PowerSaveConfig config;
config.enabled = true;
config.dimTimeout = 30000;         // 30 seconds
config.screenSaverTimeout = 60000; // 1 minute
config.sleepTimeout = 300000;      // 5 minutes
config.offTimeout = 0;             // disabled
config.dimBrightness = 30;         // 30%
config.normalBrightness = 100;     // 100%

widgetManager->setPowerSaveConfig(config);
```

### Wake-Up Behavior
- **Any Button (except SELECT)**: Wakes display, shows DefaultWidget
- **SELECT Button**: Wakes display AND activates menu directly
- **Priority Widget**: Automatically wakes display
- **Background Widget (DisplayEnabled)**: Automatically wakes display

---

## Button Handling System

### Button Event System
```cpp
// Button Types
enum class ButtonType {
    UP, DOWN, SELECT, LEFT, RIGHT
};

// Button Actions
enum class ButtonAction {
    PRESS,           // Short press
    LONG_PRESS,      // Press > 500ms
    VERY_LONG_PRESS, // Press > 5000ms
    RELEASE          // Button released
};

// Button Event
struct ButtonEvent {
    ButtonType type;
    ButtonAction action;
    uint32_t timestamp;
};
```

### Widget Button Handling
```cpp
class MyWidget : public Widget {
public:
    MyWidget() : Widget(..., WidgetFlags::WantsButtonInput) {}
    
    bool handleButtonEvent(const ButtonEvent& event) override {
        if (event.action == ButtonAction::PRESS) {
            switch (event.type) {
                case ButtonType::UP:
                    doSomething();
                    return true; // Event handled
                case ButtonType::SELECT:
                    doSomethingElse();
                    return true;
            }
        }
        return false; // Event not handled
    }
};
```

### Button Priority System
1. **Global Patterns** (planned): System-wide shortcuts (e.g., OK 5s → ProgMode toggle)
2. **Priority Widgets**: Active PRIORITY widget receives buttons
3. **Background Widgets (Active)**: Menu with DisplayEnabled receives buttons
4. **Background Widgets (Inactive)**: Menu in background receives buttons (for wake-up)
5. **Normal Widgets**: Standard widgets receive buttons
6. **Wake-Up Handler**: If no widget wants buttons, wake display

---

## Installation

1. Clone the `OFM-DeviceDisplay` library from GitHub
2. Copy or link it to the `lib` directory of your OpenKNX application project
3. Include the library in your main application:

```cpp
#include <DeviceDisplay.h>
```

## Dependencies

Installed automatically via `library.json`:

- **Adafruit_SSD1306**: Display driver for SSD1306 OLED
- **Adafruit-GFX-Library**: Graphics primitives
- **Adafruit_BusIO**: I2C/SPI communication
- **qrcode generator**: QR code generation

---

## Example Code

### Basic Setup with PowerSave and Menu

```cpp
#include <DeviceDisplay.h>

void setup() {
    const uint8_t firmwareRevision = 6;
    openknx.init(firmwareRevision);
    openknx.addModule(1, openknxDisplayModule);
    openknx.setup();

    // Get manager references
    auto* widgetManager = openknxDisplayModule.getWidgetManager();
    auto* displayModule = openknxDisplayModule.getDisplayModule();

    // Configure PowerSave
    PowerSaveConfig powerSaveConfig;
    powerSaveConfig.enabled = true;
    powerSaveConfig.dimTimeout = 30000;
    powerSaveConfig.screenSaverTimeout = 60000;
    powerSaveConfig.sleepTimeout = 300000;
    powerSaveConfig.offTimeout = 0;
    powerSaveConfig.dimBrightness = 30;
    powerSaveConfig.normalBrightness = 100;
    widgetManager->setPowerSaveConfig(powerSaveConfig);

    // PowerSave Callback (optional)
    openknxDisplayModule.setPowerSaveCallback([](PowerSaveMode oldMode, PowerSaveMode newMode) {
        logInfoP("PowerSave: %s → %s", 
                 getPowerSaveModeName(oldMode), 
                 getPowerSaveModeName(newMode));
    });

    // Create Menu Widget (Background)
    MenuWidget* menuWidget = new MenuWidget(
        10000,  // Display timeout
        WidgetFlags::Background | WidgetFlags::WantsButtonInput
    );
    widgetManager->addWidget(menuWidget);

    // Create ProgMode Widget (Priority)
    WidgetProgMode* progModeWidget = new WidgetProgMode(
        PROG_MODE_BLINK_DELAY,
        WidgetFlags::StatusWidget
    );
    widgetManager->addWidget(progModeWidget);

    // Create Clock Widget (Default)
    WidgetClock* clockWidget = new WidgetClock(
        10000,
        WidgetFlags::DefaultWidget
    );
    widgetManager->addWidget(clockWidget);

    // Create QRCode Widget (Default)
    WidgetQRCode* qrCodeWidget = new WidgetQRCode(
        10000,
        WidgetFlags::DefaultWidget,
        "https://www.openknx.de"
    );
    widgetManager->addWidget(qrCodeWidget);

    // Set Screensaver
    WidgetMatrixClassic* matrixWidget = new WidgetMatrixClassic(
        5000,
        WidgetFlags::AutoRemove,
        8  // Speed
    );
    widgetManager->setScreenSaverWidget(matrixWidget);

    // Start manager
    widgetManager->start();
}

void loop() {
    openknx.loop();
}
```

### Creating a Custom Widget

```cpp
class MyCustomWidget : public Widget
{
private:
    int _counter = 0;

public:
    MyCustomWidget(uint32_t displayTime, WidgetFlags action)
        : Widget(displayTime, action | WidgetFlags::WantsButtonInput)
    {
    }

    void setup() override
    {
        logInfoP("MyCustomWidget setup");
    }

    void start() override
    {
        logInfoP("MyCustomWidget started");
        _counter = 0;
    }

    void loop() override
    {
        if (_state != WidgetState::RUNNING) return;

        _counter++;
        drawContent();
    }

    void stop() override
    {
        logInfoP("MyCustomWidget stopped");
        clearDisplay();
    }

    bool handleButtonEvent(const ButtonEvent& event) override
    {
        if (event.action == ButtonAction::PRESS)
        {
            switch (event.type)
            {
                case ButtonType::UP:
                    _counter++;
                    return true;
                case ButtonType::DOWN:
                    _counter--;
                    return true;
            }
        }
        return false;
    }

    std::string getName() const override
    {
        return "MyCustomWidget";
    }

private:
    void drawContent()
    {
        if (!_display) return;

        _display->clearDisplay();
        _display->setTextSize(2);
        _display->setCursor(0, 0);
        _display->printf("Counter: %d", _counter);
        _display->displayBuff();
    }
};
```

### Status Widget with Auto-Remove

```cpp
void showNotification(const char* message)
{
    // Create widget
    WidgetText* notification = new WidgetText(
        5000,  // 5 seconds
        WidgetFlags::StatusWidget | WidgetFlags::AutoRemove
    );
    
    notification->setText(message);
    widgetManager->addWidget(notification);
    
    // Widget will be displayed immediately and removed after 5 seconds
}
```

### Menu Configuration

```cpp
void setupMenu(MenuWidget* menu)
{
    // Add menu items
    menu->addMenuItem("System Info", []() {
        showSystemInfo();
    });
    
    menu->addMenuItem("Network", []() {
        showNetworkMenu();
    });
    
    menu->addMenuItem("Settings", []() {
        showSettingsMenu();
    });
    
    // Add submenu
    menu->addSubMenu("Display", {
        {"Brightness", []() { adjustBrightness(); }},
        {"Contrast", []() { adjustContrast(); }},
        {"Screensaver", []() { configureScreensaver(); }}
    });
}
```
---

## Classes and Methods

### DeviceDisplay (Main Class)

**Setup & Configuration:**
- `void setup(bool configured)`: Initialize display system
- `void loop(bool configured)`: Main loop processing
- `void setPowerSaveCallback(PowerSaveCallback callback)`: Set power-save event callback

**Access:**
- `i2cDisplay* getDisplayModule()`: Get display module
- `WidgetsManager* getWidgetManager()`: Get widget manager

### WidgetsManager (Core Widget Orchestration)

**Widget Management:**
- `void addWidget(Widget* widget)`: Add widget to queue
- `void removeWidget(const std::string& name)`: Remove widget by name
- `void setScreenSaverWidget(Widget* widget)`: Set screensaver widget
- `void start()`: Start manager and background widgets

**Power Management:**
- `void setPowerSaveConfig(const PowerSaveConfig& config)`: Configure power-save
- `void userInteraction()`: Reset inactivity timer and wake display
- `void wakeUpDisplay()`: Force wake from sleep/screensaver
- `void activateMenu()`: Activate menu widget directly

**State & Debugging:**
- `Widget* getActiveButtonWidget()`: Get widget receiving button input
- `void printStatus() const`: Print full manager status
- `const char* getStateName() const`: Get current state name
- `const char* getPowerSaveModeName() const`: Get power-save mode name

### Widget (Base Class)

**Lifecycle:**
- `virtual void setup()`: Initialize widget
- `virtual void start()`: Start widget execution
- `virtual void loop()`: Main widget loop
- `virtual void stop()`: Stop widget
- `virtual void pause()`: Pause widget (by PRIORITY)
- `virtual void resume()`: Resume widget
- `virtual void background()`: Send widget to background

**Button Handling:**
- `virtual bool handleButtonEvent(const ButtonEvent& event)`: Handle button events
- `bool wantsButtonInput() const`: Check if widget wants buttons

**Properties:**
- `virtual std::string getName() const`: Get widget name
- `WidgetState getState() const`: Get current state
- `WidgetFlags getAction() const`: Get widget flags
- `void addAction(WidgetFlags flag)`: Add flag
- `void removeAction(WidgetFlags flag)`: Remove flag

### i2cDisplay (Hardware Facade for SSD1306)

**Display Control:**
- `void initDisplay(int width, int height, int sda, int scl, int reset)`: Initialize display
- `void displayOn()`: Turn display on
- `void displayOff()`: Turn display off
- `void setBrightness(uint8_t brightness)`: Set brightness (0-100%)

**Drawing:**
- `void clearDisplay()`: Clear display buffer
- `void displayBuff()`: Update display from buffer
- `void drawPixel(int16_t x, int16_t y, uint16_t color)`: Draw pixel
- `void drawLine(...)`: Draw line
- `void drawRect(...)`: Draw rectangle
- `void fillRect(...)`: Draw filled rectangle
- `void drawCircle(...)`: Draw circle
- `void fillCircle(...)`: Draw filled circle

**Text:**
- `void setTextSize(uint8_t size)`: Set text size
- `void setTextColor(uint16_t color)`: Set text color
- `void setCursor(int16_t x, int16_t y)`: Set cursor position
- `void print(const char* text)`: Print text
- `void printf(const char* format, ...)`: Print formatted text

---

## Included Widgets

| Widget | Type | Description |
|--------|------|-------------|
| **MenuWidget** | Background | Interactive menu with button navigation |
| **WidgetProgMode** | Priority | Programming mode indicator (blinks LED) |
| **WidgetClock** | Default | Digital clock display |
| **WidgetQRCode** | Default | QR code generator |
| **WidgetSysInfoLite** | Normal | System information display |
| **WidgetBootLogo** | StatusWidget | Boot splash screen |
| **WidgetOpenKNXLogo** | Normal | OpenKNX logo animation |
| **WidgetMatrixClassic** | Screensaver | Matrix rain effect |
| **WidgetMatrix** | Screensaver | Alternative matrix effect |
| **WidgetRain** | Screensaver | Rain animation |
| **WidgetStarfield** | Screensaver | Starfield animation |
| **WidgetFireworks** | Screensaver | Fireworks animation |
| **WidgetCube3D** | Screensaver | 3D rotating cube |
| **WidgetPong** | Screensaver | Pong game animation |
| **WidgetLife** | Screensaver | Conway's Game of Life |

---

## Configuration

### PowerSave Configuration

```cpp
struct PowerSaveConfig {
    bool enabled = false;              // Enable power-save
    uint32_t dimTimeout = 30000;       // Time until dim (ms)
    uint32_t screenSaverTimeout = 60000; // Time until screensaver (ms)
    uint32_t sleepTimeout = 300000;    // Time until sleep (ms)
    uint32_t offTimeout = 0;           // Time until off (0=disabled)
    uint8_t dimBrightness = 30;        // Brightness when dimmed (0-100%)
    uint8_t normalBrightness = 100;    // Normal brightness (0-100%)
};
```

### Widget Flags

```cpp
enum WidgetFlags : uint32_t {
    DefaultWidget = 1 << 0,     // Default widget (shown in idle)
    Background = 1 << 1,        // Background widget (always running)
    ManagedExternally = 1 << 2, // Externally controlled
    StatusWidget = 1 << 3,      // High priority, shown immediately
    AutoRemove = 1 << 4,        // Remove after display
    DisplayEnabled = 1 << 5,    // Make background widget visible
    WantsButtonInput = 1 << 6   // Receive button events
};
```

---

## Adapting to Other Display Types

The **WidgetsManager** is hardware-agnostic and can be adapted to other displays:

### Example: TFT Display (ST7789)

```cpp
// 1. Implement display facade
class TFTDisplay : public DisplayInterface
{
private:
    Adafruit_ST7789* _tft;
    
public:
    void initDisplay(int width, int height, int cs, int dc, int rst) {
        _tft = new Adafruit_ST7789(cs, dc, rst);
        _tft->init(width, height);
    }
    
    void clearDisplay() { _tft->fillScreen(ST77XX_BLACK); }
    void displayBuff() { /* Not needed for TFT */ }
    void setBrightness(uint8_t brightness) {
        // Implement PWM backlight control
    }
    // ... implement other methods
};

// 2. Use WidgetsManager as-is
WidgetsManager* widgetManager = new WidgetsManager();
TFTDisplay* tftDisplay = new TFTDisplay();
tftDisplay->initDisplay(240, 320, CS_PIN, DC_PIN, RST_PIN);

// 3. Widgets work without changes!
WidgetClock* clock = new WidgetClock(10000, WidgetFlags::DefaultWidget);
clock->setDisplayModule(tftDisplay);
widgetManager->addWidget(clock);
```

### Example: e-Paper Display (Waveshare)

```cpp
class EPaperDisplay : public DisplayInterface
{
private:
    Epd* _epd;
    uint8_t* _frameBuffer;
    
public:
    void initDisplay(int width, int height) {
        _epd = new Epd();
        _epd->Init();
        _frameBuffer = new uint8_t[width * height / 8];
    }
    
    void clearDisplay() { 
        memset(_frameBuffer, 0xFF, sizeof(_frameBuffer)); 
    }
    
    void displayBuff() { 
        _epd->Display(_frameBuffer); 
        _epd->Sleep(); // Save power
    }
    
    void setBrightness(uint8_t brightness) {
        // e-Paper has no backlight, ignore or implement frontlight
    }
    // ... implement other methods
};
```

**WidgetsManager remains unchanged!** Only display facade needs adaptation.

---


## Dynamic Menu System

### MenuWidget Architecture

The **MenuWidget** is a sophisticated, highly flexible background widget that provides an interactive navigation system with full **runtime extensibility**. Menus can be built dynamically during runtime, modified on-the-fly, and controlled via buttons, KNX, MQTT, or external APIs.

```
┌─────────────────────────────────────────────────────────────────────┐
│                    MenuWidget Runtime Architecture                  │
└─────────────────────────────────────────────────────────────────────┘

    ┌────────────────────────────────────────────────────────┐
    │              MenuWidget (Background Widget)            │
    │  State: BACKGROUND → RUNNING (on button press)         │
    │  Flags: Background | WantsButtonInput                  │
    └────────────────┬───────────────────────────────────────┘
                     │
         ┌───────────┴──────────────────────────────┐
         │                                          │
    ┌────▼────────────────────┐      ┌─────────────▼──────────────┐
    │   Menu Stack            │      │   Action Registry          │
    │   (Navigation History)  │      │   (Named Actions)          │
    │                         │      │                            │
    │   [Root]                │      │   "restart" → lambda()     │
    │   [Root → Network]      │      │   "save"    → lambda()     │
    │   [Root → Net → WiFi]   │      │   "info"    → lambda()     │
    │   [Root → Net → W → Adv]│      │   "custom1" → lambda()     │
    │                         │      │   ...                      │
    │   ← LEFT: Pop stack     │      │                            │
    │   → RIGHT: Push submenu │      │   [x] Registered at setup  │
    └─────────────────────────┘      │   [x] Or at runtime!       │
                                     └────────────────────────────┘
         │
         │ Current Menu
         ▼
    ┌────────────────────────────────────────────────────┐
    │        Current Menu Items (std::vector)            │
    │                                                    │
    │  [0] MenuItem("System Info")     → Action          │
    │  [1] MenuItem("Network")         → Submenu         │
    │  [2] MenuItem("Brightness: 50")  → ValueEdit       │
    │  [3] MenuItem("Firmware: v2.0")  → InfoDisplay     │
    │                                                    │
    │  Selected Index: 1                                 │
    │  Scroll Offset: 0                                  │
    │                                                    │
    │  [x] Add items at runtime: addMenuItem()           │
    │  [x] Remove items: clearMenu()                     │
    │  [x] Dynamic population: Lambda in MenuItem        │
    └────────────────────────────────────────────────────┘
         │
         │ Button Events
         ▼
    ┌────────────────────────────────────────────────────┐
    │         Button Event Handler                       │
    │                                                    │
    │  handleButtonEvent(ButtonEvent)                    │
    │    ├─ UP    → navigateUp()    (selectedIndex--)    │
    │    ├─ DOWN  → navigateDown()  (selectedIndex++)    │
    │    ├─ SELECT→ selectItem()    (execute action)     │
    │    ├─ LEFT  → navigateLeft()  (pop menu stack)     │
    │    └─ RIGHT → navigateRight() (enter submenu)      │
    │                                                    │
    │  External Control (KNX/MQTT):                      │
    │    ├─ externalNavigateUp()                         │
    │    ├─ externalNavigateDown()                       │
    │    └─ externalSelectItem()                         │
    └────────────────────────────────────────────────────┘
         │
         │ Render Pipeline
         ▼
    ┌────────────────────────────────────────────────────┐
    │         Display Rendering                          │
    │                                                    │
    │  drawMenu()                                        │
    │    ├─ Draw menu title (breadcrumb)                 │
    │    ├─ Draw visible items (with scroll)             │
    │    ├─ Highlight selected item                      │
    │    └─ Draw overlay (if active)                     │
    │                                                    │
    │  Info Overlay (temporary):                         │
    │    ┌──────────────────────────┐                    │
    │    │  ╔════════════════════╗  │                    │
    │    │  ║   WiFi Connected   ║  │                    │
    │    │  ║  IP: 192.168.1.10  ║  │                    │
    │    │  ╚════════════════════╝  │                    │
    │    └──────────────────────────┘                    │
    │    Auto-close: Button press or timeout             │
    └────────────────────────────────────────────────────┘
```

### Key Architectural Features

#### 1. **Runtime Menu Construction**
Menus are **not fixed at compile time**. They can be:
- [x] Built dynamically during `setup()`
- [x] Modified during runtime (add/remove items)
- [x] Populated based on device state (WiFi networks, sensors, etc.)
- [x] Loaded from external sources (ETS parameters, JSON config, MQTT)

#### 2. **Stack-Based Navigation**
- Navigation history is stored in a **stack** (breadcrumb trail)
- `LEFT` button pops the stack → go back to parent menu
- `RIGHT`/`SELECT` on submenu pushes to stack → enter submenu
- Example: `Root → Settings → Display → Brightness`

#### 3. **Action Registry Pattern**
- **Reusable actions** registered by name
- Decouples menu structure from implementation
- Enables **dynamic menu loading** from config files
- Example: `addMenuItemWithAction("Restart", "restart")`

#### 4. **External Control API**
- Menu can be controlled **without physical buttons**
- Use cases: KNX group objects, MQTT commands, Web UI, Serial console
- Full navigation API: `externalNavigateUp/Down/Left/Right/Select()`

#### 5. **Value Editing In-Place**
- Edit numeric/string values directly in menu
- No separate edit screen needed
- UP/DOWN changes value, SELECT confirms
- Example: Brightness slider, WiFi password entry

#### 6. **Info Overlays**
- Temporary messages displayed on top of menu
- Auto-close on button press or timeout
- Non-blocking (menu state preserved)
- Use case: "Saved!", "Connected!", "Error!"

---

### Menu Item Types

#### 1. **Action Items** (Execute Function)
```cpp
menu->addMenuItem("Restart Device", []() {
    logInfoP("Restarting...");
    ESP.restart();
});

menu->addMenuItem("Toggle LED", []() {
    static bool state = false;
    state = !state;
    digitalWrite(LED_PIN, state);
    menu->showInfoOverlay("LED", state ? "ON" : "OFF");
});
```

#### 2. **Submenu Items** (Navigate to Child Menu)
```cpp
// Static submenu (defined at compile time)
menu->addSubMenu("Network Settings", {
    {"WiFi Config", []() { configureWiFi(); }},
    {"IP Address", []() { showIPAddress(); }},
    {"MQTT Settings", []() { configureMQTT(); }}
});

// Dynamic submenu (populated at runtime)
menu->addMenuItem("WiFi Networks", [&menu]() {
    MenuConfig wifiMenu;
    
    // Scan WiFi networks at runtime
    std::vector<String> networks = WiFi.scanNetworks();
    
    for (const auto& ssid : networks) {
        wifiMenu.items.push_back({
            ssid.c_str(),
            [ssid]() {
                connectToWiFi(ssid);
                menu->showInfoOverlay("Connecting", ssid.c_str());
            }
        });
    }
    
    menu->addSubMenu("Available WiFi", wifiMenu);
});
```

#### 3. **Value Edit Items** (In-Place Editing)
```cpp
int brightness = 50;

menu->addMenuItem("Brightness", [&]() {
    // Edit mode: UP/DOWN changes value, SELECT confirms
    int newValue = menu->editValue(brightness, 0, 100, 5); // min, max, step
    
    if (newValue != brightness) {
        brightness = newValue;
        setBrightness(brightness);
        knx.write(GA_BRIGHTNESS, brightness);
        menu->showInfoOverlay("Brightness", String(brightness).c_str());
    }
});

std::string deviceName = "MyDevice";

menu->addMenuItem("Device Name", [&]() {
    std::string newName = menu->editString(deviceName, 20); // max length
    
    if (newName != deviceName) {
        deviceName = newName;
        saveConfig();
        menu->showInfoOverlay("Saved", deviceName.c_str());
    }
});
```

#### 4. **Info Display Items** (Read-Only)
```cpp
// Dynamic info (updated on each menu draw)
menu->addMenuItem("Uptime", []() {
    char uptime[32];
    snprintf(uptime, sizeof(uptime), "%lu minutes", millis() / 60000);
    return uptime; // Return string to display
}, MenuItemType::INFO);

menu->addMenuItem("IP Address", []() {
    return WiFi.localIP().toString();
}, MenuItemType::INFO);

// Static info
menu->addMenuItem("Firmware: v2.0.1", nullptr, MenuItemType::INFO);
```

---

### Runtime Menu Construction

#### Example 1: **Sensor Menu (Dynamic Population)**
```cpp
void buildSensorMenu(MenuWidget* menu) {
    MenuConfig sensorMenu;
    
    // Query all connected sensors at runtime
    std::vector<Sensor*> sensors = getSensors();
    
    for (auto* sensor : sensors) {
        sensorMenu.items.push_back({
            sensor->getName(),
            [sensor, menu]() {
                // Show sensor details
                char info[64];
                snprintf(info, sizeof(info), "%.1f %s", 
                         sensor->getValue(), 
                         sensor->getUnit());
                menu->showInfoOverlay(sensor->getName(), info);
            }
        });
    }
    
    menu->addSubMenu("Sensors", sensorMenu);
}
```

#### Example 2: **Device Discovery Menu**
```cpp
void buildDeviceMenu(MenuWidget* menu) {
    menu->addMenuItem("Scan Devices", [menu]() {
        menu->showInfoOverlay("Scanning", "Please wait...");
        
        // Scan I2C bus, KNX devices, MQTT topics, etc.
        std::vector<Device> devices = scanDevices();
        
        MenuConfig deviceMenu;
        for (const auto& device : devices) {
            deviceMenu.items.push_back({
                device.name,
                [device, menu]() {
                    // Device details submenu
                    MenuConfig detailsMenu;
                    detailsMenu.items = {
                        {"Address: " + device.address, nullptr, MenuItemType::INFO},
                        {"Type: " + device.type, nullptr, MenuItemType::INFO},
                        {"Configure", [device]() { configureDevice(device); }},
                        {"Remove", [device]() { removeDevice(device); }}
                    };
                    menu->addSubMenu(device.name, detailsMenu);
                }
            });
        }
        
        menu->clearMenu(); // Remove old menu
        menu->addSubMenu("Devices Found", deviceMenu);
    });
}
```

#### Example 3: **Configuration Menu (ETS Parameters)**
```cpp
void buildConfigMenu(MenuWidget* menu) {
    MenuConfig configMenu;
    
    // Load menu structure from ETS parameters
    for (int i = 0; i < numConfigItems; i++) {
        ParamConfig param = knx.paramData(i);
        
        switch (param.type) {
            case ParamType::INTEGER:
                configMenu.items.push_back({
                    param.name,
                    [param, menu]() {
                        int value = knx.paramInt(param.index);
                        int newValue = menu->editValue(value, param.min, param.max, param.step);
                        knx.paramWrite(param.index, newValue);
                        menu->showInfoOverlay("Saved", param.name);
                    }
                });
                break;
                
            case ParamType::BOOLEAN:
                configMenu.items.push_back({
                    param.name,
                    [param, menu]() {
                        bool value = knx.paramBool(param.index);
                        knx.paramWrite(param.index, !value);
                        menu->showInfoOverlay(param.name, !value ? "ON" : "OFF");
                    }
                });
                break;
        }
    }
    
    menu->addSubMenu("Configuration", configMenu);
}
```

---

### Action Registry System

The **Action Registry** decouples menu structure from implementation, enabling **fully dynamic menu loading**.

#### Register Actions Once
```cpp
void registerMenuActions(MenuWidget* menu) {
    // System actions
    menu->registerAction("restart", []() {
        ESP.restart();
    });
    
    menu->registerAction("save_config", []() {
        saveConfig();
        menu->showInfoOverlay("Success", "Config saved!");
    });
    
    menu->registerAction("factory_reset", []() {
        if (confirmFactoryReset()) {
            factoryReset();
            menu->showInfoOverlay("Reset", "Device reset!");
        }
    });
    
    // Network actions
    menu->registerAction("wifi_connect", []() {
        connectWiFi();
    });
    
    menu->registerAction("mqtt_connect", []() {
        connectMQTT();
    });
    
    // Custom actions (can be loaded from ETS!)
    for (int i = 0; i < numCustomActions; i++) {
        String actionName = knx.paramString(PARAM_CUSTOM_ACTION_NAME + i);
        uint16_t ga = knx.paramWord(PARAM_CUSTOM_ACTION_GA + i);
        
        menu->registerAction(actionName.c_str(), [ga]() {
            knx.write(ga, true); // Trigger KNX group object
        });
    }
}
```

#### Build Menu from Config File (JSON/XML)
```cpp
void loadMenuFromJSON(MenuWidget* menu, const char* json) {
    JsonDocument doc;
    deserializeJson(doc, json);
    
    for (JsonObject item : doc["menu"].as<JsonArray>()) {
        String name = item["name"];
        String action = item["action"];
        
        if (item.containsKey("submenu")) {
            // Recursive submenu loading
            MenuConfig submenu = loadSubmenuFromJSON(item["submenu"]);
            menu->addSubMenu(name.c_str(), submenu);
        } else {
            // Action item
            menu->addMenuItemWithAction(name.c_str(), action.c_str());
        }
    }
}

// Example JSON:
/*
{
  "menu": [
    {
      "name": "System",
      "submenu": [
        {"name": "Restart", "action": "restart"},
        {"name": "Save Config", "action": "save_config"}
      ]
    },
    {
      "name": "Network",
      "submenu": [
        {"name": "Connect WiFi", "action": "wifi_connect"},
        {"name": "Connect MQTT", "action": "mqtt_connect"}
      ]
    }
  ]
}
*/
```

---

### External Control API

Control menu without physical buttons (KNX, MQTT, Serial, Web UI).

#### KNX Group Object Control
```cpp
void setupKNXMenuControl(MenuWidget* menu) {
    // GA 10/0/1: Menu Navigation (UP=1, DOWN=2, SELECT=3, LEFT=4)
    knx.callback("10/0/1", [menu](GroupObject& go) {
        uint8_t cmd = go.value();
        
        switch (cmd) {
            case 1: menu->externalNavigateUp(); break;
            case 2: menu->externalNavigateDown(); break;
            case 3: menu->externalSelectItem(); break;
            case 4: menu->externalNavigateLeft(); break;
        }
        
        // Wake display
        widgetManager->activateMenu();
    });
    
    // GA 10/0/2: Direct Menu Action (by name)
    knx.callback("10/0/2", [menu](GroupObject& go) {
        String actionName = go.valueString();
        menu->executeAction(actionName.c_str());
    });
}
```

#### MQTT Control
```cpp
void setupMQTTMenuControl(MenuWidget* menu) {
    mqtt.subscribe("openknx/menu/navigate", [menu](String payload) {
        if (payload == "UP") menu->externalNavigateUp();
        else if (payload == "DOWN") menu->externalNavigateDown();
        else if (payload == "SELECT") menu->externalSelectItem();
        else if (payload == "BACK") menu->externalNavigateLeft();
    });
    
    mqtt.subscribe("openknx/menu/action", [menu](String payload) {
        menu->executeAction(payload.c_str());
    });
}
```

#### Web UI Control
```cpp
void setupWebMenuControl(MenuWidget* menu) {
    server.on("/api/menu/navigate", [menu]() {
        String direction = server.arg("direction");
        
        if (direction == "up") menu->externalNavigateUp();
        else if (direction == "down") menu->externalNavigateDown();
        else if (direction == "select") menu->externalSelectItem();
        else if (direction == "back") menu->externalNavigateLeft();
        
        server.send(200, "application/json", getMenuState(menu));
    });
    
    server.on("/api/menu/state", [menu]() {
        server.send(200, "application/json", getMenuState(menu));
    });
}
```

---

### Value Change Callbacks

Monitor and react to value changes in menu items.

```cpp
int brightness = 50;

menu->addMenuItem("Brightness", [&]() {
    brightness = menu->editValue(brightness, 0, 100, 5);
});

// Register callback for value changes
menu->setOnValueChanged("Brightness", [](int oldValue, int newValue) {
    logInfoP("Brightness: %d → %d", oldValue, newValue);
    
    // Apply to hardware
    setBrightness(newValue);
    
    // Send to KNX
    knx.write(GA_BRIGHTNESS, newValue);
    
    // Send to MQTT
    mqtt.publish("openknx/brightness", String(newValue));
    
    // Update ETS parameter
    knx.paramWrite(PARAM_BRIGHTNESS, newValue);
});
```

---

### Advanced Examples

#### Multi-Language Menu (Runtime Switch)
```cpp
enum Language { EN, DE, FR };
Language currentLanguage = EN;

void buildLocalizedMenu(MenuWidget* menu, Language lang) {
    menu->clearMenu();
    
    const char* texts[][3] = {
        // EN          DE                 FR
        {"System",    "System",          "Système"},
        {"Network",   "Netzwerk",        "Réseau"},
        {"Settings",  "Einstellungen",   "Paramètres"},
        {"Restart",   "Neustart",        "Redémarrer"}
    };
    
    menu->addMenuItem(texts[0][lang], []() { showSystemInfo(); });
    menu->addMenuItem(texts[1][lang], []() { showNetworkInfo(); });
    menu->addMenuItem(texts[2][lang], []() { showSettings(); });
    menu->addMenuItem(texts[3][lang], []() { ESP.restart(); });
}

// Language switcher
menu->addMenuItem("Language", [&menu]() {
    currentLanguage = (Language)((currentLanguage + 1) % 3);
    buildLocalizedMenu(menu, currentLanguage);
    menu->showInfoOverlay("Language", currentLanguage == EN ? "English" : 
                                      currentLanguage == DE ? "Deutsch" : "Français");
});
```

#### Wizard-Style Menu (Multi-Step Configuration)
```cpp
void startWiFiWizard(MenuWidget* menu) {
    static int step = 0;
    static String ssid, password;
    
    switch (step) {
        case 0: // Step 1: Select SSID
            {
                MenuConfig ssidMenu;
                auto networks = WiFi.scanNetworks();
                
                for (const auto& network : networks) {
                    ssidMenu.items.push_back({
                        network,
                        [network, menu]() {
                            ssid = network;
                            step = 1;
                            startWiFiWizard(menu); // Next step
                        }
                    });
                }
                
                menu->clearMenu();
                menu->addSubMenu("Select WiFi", ssidMenu);
            }
            break;
            
        case 1: // Step 2: Enter Password
            password = menu->editString("", 64);
            step = 2;
            startWiFiWizard(menu);
            break;
            
        case 2: // Step 3: Connect
            menu->showInfoOverlay("Connecting", ssid.c_str());
            
            if (WiFi.begin(ssid.c_str(), password.c_str()) == WL_CONNECTED) {
                saveWiFiConfig(ssid, password);
                menu->showInfoOverlay("Success", "WiFi connected!");
                step = 0;
            } else {
                menu->showInfoOverlay("Error", "Connection failed");
                step = 0;
            }
            break;
    }
}
```

---

### Performance & Memory

**Memory Usage:**
- ~50 bytes per menu item
- Stack depth: ~20 bytes per level
- Action registry: ~40 bytes per action
- Total overhead: ~2-5 KB (depending on menu size)

**Performance:**
- Navigation: O(1) (array index)
- Rendering: ~5-10ms (128x64 OLED)
- Max items: ~100 (ESP32), ~50 (ESP8266)

**Optimization Tips:**
- [x] Use `MenuConfig` for large submenus (avoids copying)
- [x] Register frequently-used actions once in registry
- [x] Clear unused menus with `clearMenu()`
- [x] Use lambda captures by reference `[&]` for large objects
- [x] Lazy-load submenus (populate only when entered)

---

### MenuWidget Complete API

**Construction:**
```cpp
MenuWidget(uint32_t displayTime, WidgetFlags action);
```

**Menu Building:**
```cpp
void addMenuItem(const std::string& name, std::function<void()> action);
void addSubMenu(const std::string& name, const MenuConfig& config);
void addMenuItemWithAction(const std::string& name, const std::string& actionName);
void clearMenu();
void removeMenuItem(const std::string& name);
```

**Action Registry:**
```cpp
void registerAction(const std::string& name, std::function<void()> action);
void unregisterAction(const std::string& name);
bool hasAction(const std::string& name) const;
void executeAction(const std::string& name);
```

**Value Editing:**
```cpp
int editValue(int currentValue, int minValue, int maxValue, int step);
float editValue(float currentValue, float minValue, float maxValue, float step);
std::string editString(const std::string& currentValue, size_t maxLength);
bool editBoolean(bool currentValue);
```

**Info Overlay:**
```cpp
void showInfoOverlay(const std::string& title, const std::string& message);
void hideInfoOverlay();
void setInfoOverlayTimeout(uint32_t timeout);
void setInfoOverlayMaxTimeout(uint32_t timeout);
```

**Navigation (Internal):**
```cpp
void navigateUp();
void navigateDown();
void navigateLeft();   // Go back
void navigateRight();  // Enter submenu
void selectItem();
```

**External Control:**
```cpp
void externalNavigateUp();
void externalNavigateDown();
void externalNavigateLeft();
void externalNavigateRight();
void externalSelectItem();
```

**Callbacks:**
```cpp
void setOnValueChanged(const std::string& itemName, 
                       std::function<void(int oldValue, int newValue)> callback);
void setOnMenuChanged(std::function<void(const std::string& menuName)> callback);
void setOnItemSelected(std::function<void(const std::string& itemName)> callback);
```

**State:**
```cpp
std::string getCurrentMenuName() const;
int getSelectedIndex() const;
int getMenuItemCount() const;
bool isInEditMode() const;
bool isOverlayActive() const;
std::vector<std::string> getMenuStack() const; // Breadcrumb trail
```

---

## License

This library is licensed under the **GNU GENERAL PUBLIC LICENSE Version 3**. See the LICENSE file for details.

---

## Contributing

Contributions are welcome! Please submit pull requests or open issues on GitHub.

## Support

For questions and support, visit the OpenKNX community forum or GitHub issues page.
