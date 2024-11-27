#pragma once
/**
 * @file        i2c-Display.h
 * @brief       This module offers a i2c display for the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "OpenKNX.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class i2cDisplay
{
  public:
    i2cDisplay();  // Constructor
    ~i2cDisplay(); // Destructor

    struct ScreenSettings // Struct to hold the display settings
    {
        uint8_t width = -1;      // 128 or 64 screen pixels wide of the display. Currently tested and supported: 128
        uint8_t height = -1;     // 64 or 32 screen pixels high of the display. Currently tested and supported: 64
                                 // Possible display resolutions: 128x64, 128x32, 96x16, 64x48, 64x32, 64x16, 32x16, 32x8
        uint8_t i2cadress = 0x0; // 0x3D for 128x64, 0x3C for 128x32. Check datasheet for Address or visit: 
                                 //        https://github.com/adafruit/I2C_Addresses/blob/main/0x30-0x3F.md
        int8_t reset = -1;       // Reset pin # (or -1 if sharing Arduino reset pin)
        bool bIsi2c1 = false;    // true:i2c1 false:i2c0
        pin_size_t sda = -1;     // SDA pin on RP2040 for i2c1
        pin_size_t scl = -1;     // SCL pin on RP2040 for i2c1
    } lcdSettings;               // Start with default settings

    std::unique_ptr<Adafruit_SSD1306> display; // Display object. Must be a pointer to be able to make it a unique_ptr
    std::unique_ptr<TwoWire> CustomI2C;        // I2C object. Must be a pointer to be able to use unique_ptr for it as well

    void setup();                                           // Setup method for initialization
    bool InitDisplay();                                     // Initialize the display
    bool InitDisplay(ScreenSettings DeviceDisplaySettings); // Initialize the display with custom settings
    void SetDisplayWidth(uint8_t width);                    // Set the display width
    void SetDisplayHeight(uint8_t height);                  // Set the display height
    uint8_t GetDisplayWidth();                              // Return the display width
    uint8_t GetDisplayHeight();                             // Return the display height

    void SetDisplayI2CAddress(uint8_t i2cadress);                                                                                          // Set the display i2c address
    void SetDisplayReset(int8_t reset);                                                                                                    // Set the display reset pin
    void SetDisplayI2C(bool bIsi2c1);                                                                                                      // Set the display i2c bus
    void SetDisplaySDA(pin_size_t sda);                                                                                                    // Set the display SDA pin
    void SetDisplaySCL(pin_size_t scl);                                                                                                    // Set the display SCL pin
    void SetDisplaySettings(uint8_t width, uint8_t height, uint8_t i2cadress, int8_t reset, bool bIsi2c1, pin_size_t sda, pin_size_t scl); // Set all display settings
};