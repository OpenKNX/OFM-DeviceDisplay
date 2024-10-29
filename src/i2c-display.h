#pragma once
#include "OpenKNX.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class i2cDisplay
{
  private:
    struct ScreenSettings
    {
        u8_t width = 128;         // 128 or 64 screen pixels wide
        uint8_t height = 64;      // 64 or 32 screen pixels high
        uint8_t i2cadress = 0x3C; // 0x3D for 128x64, 0x3C for 128x32 - See datasheet for Address
        int8_t reset = -1;        // Reset pin # (or -1 if sharing Arduino reset pin)
        bool bIsi2c1 = true;      // true:i2c1 false:i2c0
        pin_size_t sda = 26;      // SDA pin on RP2040 for i2c1
        pin_size_t scl = 27;      // SCL pin on RP2040 for i2c1
    };
    ScreenSettings lcdSettings; // Start with default settings

  public:
    std::unique_ptr<Adafruit_SSD1306> display;
    std::unique_ptr<TwoWire> CustomI2C;

    bool InitDisplay();
    void setup();
};