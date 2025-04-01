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

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#ifdef ARDUINO_ARCH_ESP32
  #define i2c_inst_t TwoWire
#endif

// Define SSD13xx controller commands ToDo: Check if SSD1315 is correct supported
#ifdef DEVICE_DISPLAY_MODULE_SSD1315
#define SSD13XX_SETCONTRAST 0xD9
#define SSD13XX_SETVCOMDETECT 0xDB
#else 
#define SSD13XX_SETCONTRAST SSD1306_SETCONTRAST
#define SSD13XX_SETVCOMDETECT SSD1306_SETVCOMDETECT
#endif

class i2cDisplay
{
  public:
    i2cDisplay();  // Constructor
    ~i2cDisplay(); // Destructor

    struct ScreenSettings // Struct to hold the display settings
    {
        uint8_t width = -1;            // 128 or 64 screen pixels wide of the display. Currently tested and supported: 128
        uint8_t height = -1;           // 64 or 32 screen pixels high of the display. Currently tested and supported: 64
                                       // Possible display resolutions: 128x64, 128x32, 96x16, 64x48, 64x32, 64x16, 32x16, 32x8
        uint8_t i2cadress = 0x0;       // 0x3D for 128x64, 0x3C for 128x32. Check datasheet for Address or visit:
                                       //        https://github.com/adafruit/I2C_Addresses/blob/main/0x30-0x3F.md
        int8_t reset = -1;             // Reset pin # (or -1 if sharing Arduino reset pin)
        i2c_inst_t* i2cInst = nullptr; // I2C instance (i2c0 or i2c1)
        pin_size_t sda = -1;           // SDA pin on RP2040 for i2c1
        pin_size_t scl = -1;           // SCL pin on RP2040 for i2c1
    } lcdSettings;                     // Start with default settings

    Adafruit_SSD1306* display; // Display object. Must be a pointer to be able to make it a unique_ptr
    TwoWire* CustomI2C;        // I2C object. Must be a pointer to be able to use unique_ptr for it as well

    void setup();                                           // Setup method for initialization
    void loop();
    bool InitDisplay();                                     // Initialize the display
    bool InitDisplay(ScreenSettings DeviceDisplaySettings); // Initialize the display with custom settings
    void SetDisplayWidth(uint8_t width);                    // Set the display width
    void SetDisplayHeight(uint8_t height);                  // Set the display height
    uint8_t GetDisplayWidth();                              // Return the display width
    uint8_t GetDisplayHeight();                             // Return the display height

    void SetDisplayI2CAddress(uint8_t i2cadress); // Set the display i2c address
    void SetDisplayReset(int8_t reset);           // Set the display reset pin
    void SetDisplayI2C(i2c_inst_t* i2cInst);      // Set the display i2c bus
    void SetDisplaySDA(pin_size_t sda);           // Set the display SDA pin
    void SetDisplaySCL(pin_size_t scl);           // Set the display SCL pin
    void SetDisplaySettings(uint8_t width, uint8_t height, uint8_t i2cadress,
                            int8_t reset, i2c_inst_t* i2cInst,
                            pin_size_t sda, pin_size_t scl);   // Set all display settings
    void SetDisplayContrast(uint8_t contrast);                 // Set the display contrast
    void SetDisplayVCOMDetect(uint8_t vcomh);                  // Set the display VCOMH regulator output
    inline void SetDim(bool dim) { return display->dim(dim); } // Dim the display
    void SetInvertDisplay(bool invert);                        // Invert the display
    void SetDisplayStartLine(uint8_t startline);               // Set the display start line
    void SetDisplayOffset(uint8_t offset);                     // Set the display offset
    void SetDisplayClockDiv(uint8_t clockdiv);                 // Set the display clock division
    void SetDisplayPreCharge(uint8_t precharge);               // Set the display precharge
    void displayBuff();                                        // Funktion, die den Puffer mit dem aktuellen Zustand vergleicht und nur geänderte Bereiche sendet


    inline void __setLoopColumnMethod(bool loopColumnMethod) { __loopColumnMethod = loopColumnMethod; } // Set the loop column method
  private:
    // #define BUFFER_SIZE (128 * ((64 + 7 ) / 8))
    uint16_t _sizeDispBuff;   
    uint8_t* _curDispBuffer;  // Buffer size!
    uint8_t* _prevDispBuffer; // Buffer size!

    // __TESTING__
    bool __loopColumnMethod = false; // Enable the loop column for partial display updates. Default is false. 
    /**
     * Number of Columns to partial transfer to display
     * Values 2^n only, value >32 will not update all pages!
     */
    const uint8_t _loopColumnCount = 4;

    /** 
     * Define next start-column for partial transfer to display.
     * `0xff` for scheduling restart of transfer *after* next `loop()`, to reduce overall display loop-time on change
     * Allowed values `0` to `lcdSettings.width - _loopColumnCount`, all other values will be ignored.
     */
    uint8_t _loopColumn = 0xfe;
    
    bool initDisplayBuffer();
    void updateArea(int x, int y, int byteIndex);
    void sendCommand(uint8_t command);
    void displayFullBuffer();
    void updateCols(int startCol, int endCol);
    void updatePage(int page, int startCol, int endCol);
};