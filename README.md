# OFM-DevideDisplay

DeviceDisplay is a flexible library for managing an OLED display on an embedded device. This library supports multiple widgets, scrolling text, and (ToDo) animated transitions . It integrates with the OpenKNX framework and includes modular components to define custom display widgets.

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
   
### 2. **DisplayFacade** (Facade for Display Management) ToDo!
   - Manages active widgets and transitions between them.
   - **Methods**:
      - `setWidget(DisplayWidget* widget, bool animate, const char* direction)`: Sets the active widget with optional animation.
      - `UpdateTextLines()`: Refreshes the display based on the active widget.
      - `animateTransition(DisplayWidget* newWidget, const char* direction)`: Handles sliding animation for widget transitions.

### 3. **Widgets**
   - Widgets encapsulate individual display components and logic.
   - **Available Widgets**:
     - `TextWidget`: Displays customizable text content with options for multiple lines.
     - `StatusWidget`: Shows status information with header and body text.

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

openknx.addModule(1, openknxDisplayModule);

Widget* WidgetNetInfo = new Widget(Widget::DisplayMode::FOUR_LINE);
WidgetNetInfo->SetFourLines("Network", "IP: 11.11.0.123", "Subnet: 255.255.255.0", "Gateway: 11.11.0.1");
WidgetNetInfo->TextHeader->textSize = 2;
WidgetNetInfo->TextLine1->textSize = 2;
WidgetNetInfo->TextLine2->textSize = 2;
WidgetNetInfo->TextLine3->textSize = 2;

openknxDisplayModule.addWidget(WidgetNetInfo,  2000, "NetworkInfo", DeviceDisplay::WidgetAction::NoAction);