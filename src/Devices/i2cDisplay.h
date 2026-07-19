#pragma once
/**
 * @file        i2cDisplay.h
 * @brief       This module offers a i2c display for the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 **/

#include "OpenKNX.h"

#include "OpenKNX/I2C/Wire1Lock.h" // shared Wire1 (display+PCA9557) mutex; no-op on RP2040
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define i2c_inst_t TwoWire // We will use TwoWire

// SSD1306 and SSD1315 are command-compatible; no panel-specific remapping needed.
#define SSD13XX_SETVCOMDETECT SSD1306_SETVCOMDETECT

class i2cDisplay
{
  public:
    i2cDisplay();  // Constructor
    ~i2cDisplay(); // Destructor

    const std::string logPrefix() { return "i2cDisplay"; }

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

    void setup(); // Setup method for initialization
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
                            pin_size_t sda, pin_size_t scl); // Set all display settings
    void SetDisplayContrast(uint8_t contrast);               // Set the display contrast
    void SetDisplayVCOMDetect(uint8_t vcomh);                // Set the display VCOMH regulator output
    inline void SetDim(bool dim)                             // Dim the display (single Wire1 transaction)
    {
        OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
        display->dim(dim);
    }
    void SetInvertDisplay(bool invert);          // Invert the display
    void SetDisplayStartLine(uint8_t startline); // Set the display start line
    void SetDisplayOffset(uint8_t offset);       // Set the display offset
    void SetDisplayClockDiv(uint8_t clockdiv);   // Set the display clock division
    void SetDisplayPreCharge(uint8_t precharge); // Set the display precharge
    void displayBuff();                          // Funktion, die den Puffer mit dem aktuellen Zustand vergleicht und nur geänderte Bereiche sendet

    inline void __setLoopColumnMethod(bool loopColumnMethod) { __loopColumnMethod = loopColumnMethod; } // Set the loop column method

    void setBrightness(uint8_t brightness); // 0-100%
    void displayOn();
    void displayOff();
    uint8_t getBrightness() const { return _brightness; }
    bool isDisplayOn() const { return _displayOn; }

    // Central font-size control. Level 0/1/2 maps to Adafruit setTextSize(1/2/3).
    void setFontSize(uint8_t level); // 0, 1 or 2 (clamped)
    uint8_t getFontSize() const { return _fontSize; }

    // Live + persistent invert; re-applied by displayOn() after DISPLAYON/contrast changes.
    void setInvert(bool invert); // Track + apply invert state
    bool isInverted() const { return _invert; }

    // 180deg rotation via segment-remap + COM-scan direction (self-locking; safe at runtime, NOT
    // inside the already-locked init block). Applies to the live panel scan -> caller redraws.
    void setRotation(bool flip180);
    bool isRotated() const { return _rotate180; }

    // Raw 1-bit framebuffer (SSD1306 vertical packing) for the screenshot encoder; nullptr if uninit.
    const uint8_t* getFramebuffer() const { return display ? display->getBuffer() : nullptr; }

  private:
    uint8_t _brightness = 100;
    bool _displayOn = true;
    uint8_t _fontSize = 0;   // 0/1/2 -> setTextSize(1/2/3); menu default is 0
    bool _invert = false;    // current invert state, re-applied after DISPLAYON
    bool _rotate180 = false; // current 180deg rotation state

    // #define BUFFER_SIZE (128 * ((64 + 7 ) / 8))
    uint16_t _sizeDispBuff;
    uint8_t* _curDispBuffer;  // Buffer size!
    uint8_t* _prevDispBuffer; // Buffer size!

    // Non-blocking flush: displayBuff() diffs and records dirty page/column ranges (no I2C); loop()
    // pushes at most FLUSH_PAGES_PER_LOOP dirty pages per iteration so a full-frame change never blocks.
    static constexpr uint8_t MAX_PAGES = 8;            // 64px / 8 (also covers 32px displays)
    static constexpr uint8_t FLUSH_PAGES_PER_LOOP = 1; // pages pushed per loop() -> bounds the I2C time
    bool _pageDirty[MAX_PAGES] = {false};
    int16_t _dirtyStartCol[MAX_PAGES] = {0};
    int16_t _dirtyEndCol[MAX_PAGES] = {0};
    uint8_t _flushCursor = 0; // round-robin scan start so a page re-dirtied every frame can't starve lower pages

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
    void sendCommand(uint8_t command);         // locks Wire1 (single-command transaction)
    void sendCommandUnlocked(uint8_t command); // raw command; caller already holds the Wire1 lock
    void displayFullBuffer();
    void updateCols(int startCol, int endCol);
    void updatePage(int page, int startCol, int endCol);
};
