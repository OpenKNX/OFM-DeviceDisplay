# OFM-DeviceDisplay

`OFM-DeviceDisplay` is a flexible library for managing i2c Displays (Adafruit SSD1306 kompatible) on OpenKNX devices. This library supports multiple widgets, dynamic text, icons and QR codes. It is specifically designed for integration into the OpenKNX framework and provides modular components for defining custom display widgets.

## Features

- **Multiple Widget Management**: Easily add and manage various widgets, each with its own properties.
- **Dynamic Text and Alignment**: Customize text alignment, size, color, and scrolling within widgets.
- **Icons and QR Codes**: Support for displaying icons and QR codes on the display.
- **Animated Transitions**: Transitions between widgets with slide animations (horizontal or vertical).
- **Programming Mode**: Switch to an exclusive programming mode widget.
- **Screen Saver**: Displays a matrix-style screensaver.
- **QR Code**: Displays a QR code on the screen.
- **Customizable Display Parameters**: Define display resolution, I2C settings, and text properties.

## Installation

1. Download the `OFM-DeviceDisplay` library from GitHub.
2. Unzip the library and copy or link it to the `lib` directory of your OpenKNX application project.
3. Include the library in your OpenKNX Application projekt main:
```cpp
#include <DeviceDisplay.h>
```

## Dependencies

Make sure you have the following dependencies installed (which normly be installed using the library.json file):

- Adafruit_SSD1306: Library for OLED display control.
- qrcodegen: QR code generator library.

## Example Code

Here is an example of how to use the OFM-DeviceDisplay library:
In Main.Cpp:
```cpp

#include <DeviceDisplay.h>


void setup() {

    const uint8_t firmwareRevision = 6;
    openknx.init(firmwareRevision);
    openknx.addModule(1, openknxDisplayModule); // Example
    openknx.setup();


   // Create a BootLogo widget
    #define durationShowBootLogo 5000 // 5 Seconds
    Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
    openknxDisplayModule.addWidget(bootLogo, durationShowBootLogo, "BootLogo", DeviceDisplay::WidgetAction::StatusFlag | DeviceDisplay::WidgetAction::InternalEnabled | DeviceDisplay::WidgetAction::AutoRemoveFlag);

    // Create a TextWidget
    TextWidget* textWidget = new TextWidget();
    textWidget->SetDynamicTextLines({"Header", "Line 1", "Line 2"});
    openknxDisplayModule.addWidget(textWidget, 3000, "TextWidget", DeviceDisplay::WidgetAction::NoAction);

    // Create a ProgMode widget
    Widget* progMode = new Widget(Widget::DisplayMode::PROG_MODE);
    openknxDisplayModule.addWidget(progMode, PROG_MODE_BLINK_DELAY, "ProgMode", DeviceDisplay::WidgetAction::StatusFlag | DeviceDisplay::WidgetAction::ExternalManaged);

    // Create a QRCode widget
    Widget* qrCodeWidget = new Widget(Widget::DisplayMode::QR_CODE);
    qrCodeWidget->qrCode.qrText = "https://www.openknx.de";
    openknxDisplayModule.addWidget(qrCodeWidget, 10000, "QRCode", DeviceDisplay::WidgetAction::NoAction);

}

void loop() {
    openknx.loop();
}
```

## Classes and Methods

1. **DeviceDisplay (Main Library Class)**
    - Manages widgets and display settings.
    - Controls transitions, programming mode, and display updates.
    - Methods:
        - `void addWidget(Widget* widget, uint32_t duration, const std::string& name, WidgetAction action)`: Adds a widget to the display.
        - `void removeWidget(const std::string& name)`: Removes a widget from the display.
        - `void updateWidgetStatus(WidgetInfo& widgetInfo, WidgetStatus status)`: Updates the status of a widget.
        - `void printWidgetStatus(const WidgetInfo& widgetInfo) const`: Prints the current status of a widget.

2. **i2cDisplay (Facade for Display Management)**
    - Manages active widgets and transitions between them.
    - Methods:
        - `void initDisplay(int width, int height, int sda, int scl, int reset)`: Initializes the display.
        - `void showText(const std::string& text)`: Displays text on the screen.

3. **Widget (Base Class for All Widgets)**
    - Defines the basic properties and methods for widgets.
    - Methods:
        - `virtual void draw() = 0`: Draws the widget on the display.
        - `virtual void update() = 0`: Updates the state of the widget.

4. **TextWidget (Widget for Text Display)**
    - Extends the base Widget class for displaying text.
    - Methods:
        - `void setText(const std::string& text)`: Sets the text to be displayed.
        - `void setAlignment(TextAlignment alignment)`: Sets the text alignment.
        - `void setScroll(bool scroll)`: Enables or disables text scrolling.


## Configuration in ETS

ToDo: 

## License

This library is licensed under the GNU GENERAL PUBLIC LICENSE. For more information, see the LICENSE file.

