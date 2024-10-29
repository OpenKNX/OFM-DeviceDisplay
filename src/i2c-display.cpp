#include "i2c-display.h"

// Initialize the display.
bool i2cDisplay::InitDisplay()
{
    CustomI2C = std::make_unique<TwoWire>(lcdSettings.bIsi2c1 ? i2c1 : i2c0, lcdSettings.sda, lcdSettings.scl);
    display = std::make_unique<Adafruit_SSD1306>(lcdSettings.width, lcdSettings.height, CustomI2C.get(), lcdSettings.reset);

    if (!display->begin(SSD1306_SWITCHCAPVCC, lcdSettings.i2cadress, true, true))
    {
        return false;
    }
    return true;
}

// Setup method for initialization
void i2cDisplay::setup()
{
  //ToDo: Setup OpenKNX Hardware Specific i2c settings, like SDA, SCL, i2c address, etc.
}