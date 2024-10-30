#pragma once
#include "OpenKNX.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class i2cDisplay 
{
  public:
    i2cDisplay(); // Constructor
    ~i2cDisplay(); // Destructor
  
  private:
    struct ScreenSettings // Struct to hold the display settings
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
    std::unique_ptr<Adafruit_SSD1306> display;  // Display object. Must be a pointer to be able to make it a unique_ptr
    std::unique_ptr<TwoWire> CustomI2C; // I2C object. Must be a pointer to be able to use unique_ptr for it as well

    void setup(); // Setup method for initialization
    bool InitDisplay(); // Initialize the display
    void SetDisplayWidth(uint8_t width); // Set the display width
    void SetDisplayHeight(uint8_t height); // Set the display height
    void SetDisplayI2CAddress(uint8_t i2cadress); // Set the display i2c address
    void SetDisplayReset(int8_t reset); // Set the display reset pin
    void SetDisplayI2C(bool bIsi2c1); // Set the display i2c bus
    void SetDisplaySDA(pin_size_t sda); // Set the display SDA pin
    void SetDisplaySCL(pin_size_t scl); // Set the display SCL pin
    void SetDisplaySettings(uint8_t width, uint8_t height, uint8_t i2cadress, int8_t reset, bool bIsi2c1, pin_size_t sda, pin_size_t scl);  // Set all display settings
};