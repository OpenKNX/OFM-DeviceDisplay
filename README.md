# OFM-DevideDisplay

DeviceDisplay is a flexible library for managing an OLED display on an embedded device. This library supports multiple widgets, scrolling text, and animated transitions. It integrates with the OpenKNX framework and includes modular components to define custom display widgets.

## Features

- **Multiple Widget Management**: Easily add and manage different widgets, each with its own properties.
- **Text Scrolling and Alignment**: Customize text alignment, size, color, and scrolling within widgets.
- **Animated Transitions**: Transition widgets with slide animations (horizontal or vertical).
- **Programming Mode**: Toggle an exclusive programming mode widget.
- **Customizable Display Parameters**: Define display resolution, I2C settings, and text properties.

## Components

### 1. **DeviceDisplay** (Main Library Class)
   - Manages widgets and display settings.
   - Controls transitions, programming mode, and display refresh.
   - **Methods**:
      - `setup()`: Initializes the display.
      - `loop(bool configured)`: Updates the display in each loop cycle.
      - `showTextWidget()`: Switches to the `TextWidget` display.
      - `showStatusWidget()`: Switches to the `StatusWidget` display.
      - `addWidget(DisplayWidget* widget, uint32_t duration)`: Adds a widget to the display queue with a display duration.
      - `activateProgMode(bool active)`: Activates or deactivates the programming mode widget.

### 2. **DisplayFacade** (Facade for Display Management)
   - Manages active widgets and transitions between them.
   - **Methods**:
      - `setWidget(DisplayWidget* widget, bool animate, const char* direction)`: Sets the active widget with optional animation.
      - `updateDisplay()`: Refreshes the display based on the active widget.
      - `animateTransition(DisplayWidget* newWidget, const char* direction)`: Handles sliding animation for widget transitions.

### 3. **Widgets**
   - Widgets encapsulate individual display components and logic.
   - **Available Widgets**:
     - `TextWidget`: Displays customizable text content with options for multiple lines.
     - `StatusWidget`: Shows status information with header and body text.
   - **Defining New Widgets**:
     - Extend `DisplayWidget` and implement `draw(Adafruit_SSD1306* display, int x = 0, int y = 0)`.

### 4. **i2cDisplay** (Display Hardware Handler)
   - Handles hardware-specific initialization and communication for the OLED display.
   - **Setup Parameters**:
      - `width`: Display width in pixels.
      - `height`: Display height in pixels.
      - `i2cadress`: I2C address of the display.
      - `sda` / `scl`: SDA and SCL pins for I2C communication.
   
## Example Usage

### Initialization and Setup

```cpp
#include "DeviceDisplay.h"
#include "TextWidget.h"
#include "StatusWidget.h"

DeviceDisplay display;

// Example widgets
TextWidget textWidget("Welcome", "Line 1", "Line 2", "Line 3");
StatusWidget statusWidget("System Status", "All systems go");

void setup() {
    // Initialize display
    display.setup();

    // Add widgets with duration (in milliseconds)
    display.addWidget(&textWidget, 5000);  // Show text widget for 5 seconds
    display.addWidget(&statusWidget, 3000); // Show status widget for 3 seconds

    // Activate programming mode if needed
    display.activateProgMode(true);
}

void loop() {
    display.loop(true); // Regularly update display content
}