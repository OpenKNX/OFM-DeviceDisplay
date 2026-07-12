#include "i2cdisplay.h"

#include "OpenKNX/I2C/Wire1Lock.h" // shared Wire1 (display+PCA9557) mutex; no-op on RP2040

#define SSD1306_NO_SPLASH // Suppress the internal display splash screen

/**
 * @brief Construct a new i2c Display::i2c Display object and initialize the display settings.
 *
 */
i2cDisplay::i2cDisplay() : lcdSettings()
{
    lcdSettings.width = -1;        // 128 or 64 screen pixels wide
    lcdSettings.height = -1;       // 64 or 32 screen pixels high
    lcdSettings.i2cadress = 0x0;   // 0x3D for 128x64, 0x3C for 128x32 - See datasheet for Address
    lcdSettings.reset = -1;        // Reset pin # (or -1 if sharing Arduino reset pin)
    lcdSettings.i2cInst = nullptr; // I2C instance (i2c0 or i2c1)
    lcdSettings.sda = -1;          // SDA pin on RP2040 for i2c
    lcdSettings.scl = -1;          // SCL pin on RP2040 for i2c
}

/**
 * @brief Destroy the i2c Display::i2c Display object from the memory.
 *
 */
i2cDisplay::~i2cDisplay()
{
    delete display;                             // Remove the display object from the memory (Adafruit_SSD1306)
    delete CustomI2C;                           // Remove the i2c object from the memory (TwoWire)
    if (_curDispBuffer) free(_curDispBuffer);   // Free the current display buffer if it was allocated
    if (_prevDispBuffer) free(_prevDispBuffer); // Free the previous display buffer if it was allocated
}

/**
 * @brief Initialize the i2c display object with the default settings.
 *        Using the Adafruit_SSD1306 library. CustomI2C and display are unique pointers.
 * @return true if the display was initialized successfully
 */
bool i2cDisplay::InitDisplay()
{
    if (lcdSettings.sda < 0 || lcdSettings.scl < 0 || lcdSettings.i2cInst == nullptr)
    {
        return false; // SDA and SCL and i2c instance must be set
    }
    if (lcdSettings.width < 1 || lcdSettings.height < 1 || !initDisplayBuffer())
    {
        return false; // Width and height must be set
    }

    CustomI2C = lcdSettings.i2cInst;
#ifdef ARDUINO_ARCH_ESP32
    CustomI2C->begin(lcdSettings.sda, lcdSettings.scl); // For ESP32, specify SDA and SCL pins
#else
    CustomI2C->setSDA(lcdSettings.sda);
    CustomI2C->setSCL(lcdSettings.scl);
    CustomI2C->begin();
#endif

    // display = new Adafruit_SSD1306(lcdSettings.width, lcdSettings.height, CustomI2C, lcdSettings.reset, 1000000UL, 1000000UL);
    display = new Adafruit_SSD1306(lcdSettings.width, lcdSettings.height, CustomI2C, lcdSettings.reset, OPENKNX_GPIO_CLOCK, OPENKNX_GPIO_CLOCK);

    // One Wire1 lock for the whole begin+config block. No-op on RP2040.
    {
        OPENKNX_WIRE1_LOCK();
        if (!display->begin(SSD1306_SWITCHCAPVCC, lcdSettings.i2cadress, true, true))
        {
            return false; // Display not found or not initialized. Check the wiring and i2c address
        }

        display->ssd1306_command(SSD1306_SEGREMAP);   // Spiegele die Spaltenanordnung
        display->ssd1306_command(SSD1306_COMSCANINC); // Ändere die Zeilenrichtung
        display->clearDisplay();                      // Clear initialy the display buffer. Previous arcifacts could be displayed
        display->display();                           // Display the cleared buffer
    }

    return true;
}
/**
 * @brief Initialize the i2c display object with the custom settings.
 *        Using the Adafruit_SSD1306 library. CustomI2C and display are unique pointers.
 * @param LCDsettings the display settings
 * @return true if the display was initialized successfully
 */
bool i2cDisplay::InitDisplay(ScreenSettings DeviceDisplaySettings)
{
    lcdSettings = DeviceDisplaySettings;
    return InitDisplay();
}

// Setup method for initialization
void i2cDisplay::setup()
{
    // ToDo: Setup OpenKNX Hardware Specific i2c settings, like SDA, SCL, i2c address, etc.
}

/**
 * @brief Non-blocking partial transfer: push at most FLUSH_PAGES_PER_LOOP dirty pages per call so
 *        a full-frame change never blocks the loop (whole 128x64 push at 400kHz was ~54ms).
 */
void i2cDisplay::loop()
{
    if (!_curDispBuffer || !_prevDispBuffer || !display) return;

    const int pages = lcdSettings.height / 8;
    const int width = lcdSettings.width;
    if (pages <= 0) return;
    uint8_t pushed = 0;
    // Round-robin the scan start so a page re-dirtied every frame never starves the lower body pages.
    for (int i = 0; i < pages && i < MAX_PAGES && pushed < FLUSH_PAGES_PER_LOOP; i++)
    {
        const int page = (_flushCursor + i) % pages;
        if (!_pageDirty[page]) continue;
        const int startCol = _dirtyStartCol[page];
        const int endCol = _dirtyEndCol[page];
        updatePage(page, startCol, endCol); // one bounded I2C transfer for this page's dirty range
        for (int col = startCol; col <= endCol; col++)
        {
            const size_t index = (size_t)page * width + col;
            _prevDispBuffer[index] = _curDispBuffer[index]; // this page's range is now on-screen
        }
        _pageDirty[page] = false;
        pushed++;
        _flushCursor = (uint8_t)((page + 1) % pages); // continue after this page on the next loop()
    }
}

/**
 * @brief Set the display width in pixels. Default is 128.
 *
 * @param width of the display in pixels
 */
void i2cDisplay::SetDisplayWidth(uint8_t width)
{
    lcdSettings.width = width;
}

/**
 * @brief Get the display width in pixels.
 *
 * @return uint8_t width of the display in pixels
 */
uint8_t i2cDisplay::GetDisplayWidth()
{
    return lcdSettings.width;
}

/**
 * @brief Set the display height in pixels. Default is 64.
 *
 * @param height of the display in pixels
 */
void i2cDisplay::SetDisplayHeight(uint8_t height)
{
    lcdSettings.height = height;
}

/**
 * @brief Get the display height in pixels.
 *
 * @return uint8_t height of the display in pixels
 */
uint8_t i2cDisplay::GetDisplayHeight()
{
    return lcdSettings.height;
}

/**
 * @brief Set the display i2c address. Default is 0x3C.
 *
 * @param i2cadress of the display
 */
void i2cDisplay::SetDisplayI2CAddress(uint8_t i2cadress)
{
    lcdSettings.i2cadress = i2cadress;
}

/**
 * @brief Set the display reset pin. Default is -1.
 *
 * @param reset pin number
 */
void i2cDisplay::SetDisplayReset(int8_t reset)
{
    lcdSettings.reset = reset;
}

/**
 * @brief Set the display i2c bus. Default is true.
 *
 * @param i2c_inst_t i2cInst of the display (i2c0 or i2c1)
 */
void i2cDisplay::SetDisplayI2C(i2c_inst_t *i2cInst)
{
    lcdSettings.i2cInst = i2cInst;
}

/**
 * @brief Set the display SDA pin. Default is 26.
 *
 * @param sda pin number
 */
void i2cDisplay::SetDisplaySDA(pin_size_t sda)
{
    lcdSettings.sda = sda;
}

/**
 * @brief Set the display SCL pin. Default is 27.
 *
 * @param scl pin number
 */
void i2cDisplay::SetDisplaySCL(pin_size_t scl)
{
    lcdSettings.scl = scl;
}

/**
 * @brief Set the display settings for the display object.
 *
 * @param width of the display in pixels
 * @param height of the display in pixels
 * @param i2cadress of the display
 * @param reset pin number
 * @param i2cInst of the display (i2c0 or i2c1)
 * @param sda pin number
 * @param scl pin number
 */
void i2cDisplay::SetDisplaySettings(uint8_t width, uint8_t height, uint8_t i2cadress, int8_t reset, i2c_inst_t *i2cInst, pin_size_t sda, pin_size_t scl)
{
    SetDisplayWidth(width);
    SetDisplayHeight(height);
    SetDisplayI2CAddress(i2cadress);
    SetDisplayReset(reset);
    SetDisplayI2C(i2cInst);
    SetDisplaySDA(sda);
    SetDisplaySCL(scl);
}

/**
 * @brief Set the display brightness.
 *        0x00 to 0xFF. Default is 0xFF.
 * @param brightness of the display
 */
void i2cDisplay::SetDisplayContrast(uint8_t contrast) // Set the contrast of the display
{
    // Contrast is 0x81 (SSD1306_SETCONTRAST) on both SSD1306 and SSD1315 (0xD9 is pre-charge).
    OPENKNX_WIRE1_LOCK();                          // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD1306_SETCONTRAST); // 0x81 on SSD1306 AND SSD1315
    display->ssd1306_command(contrast);
}

/**
 * @brief The VCOM regulator output is set to the specified level.
 *        0x00 to 0xFF. Default is 0x20 (0.77*VCC) max. 0xff (0.83*VCC) and min
 *        0x00 (0.65*VCC) which is the reset value. (Page 32)
 *        https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
 * @param vcomh
 */
void i2cDisplay::SetDisplayVCOMDetect(uint8_t vcomh) // Set the VCOMH regulator output
{
    // Check if the VCOM value is in the valid range (0x00 to 0xFF)
    if (vcomh < 0 || vcomh > 0xFF)
        return;

    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD13XX_SETVCOMDETECT);
    display->ssd1306_command(vcomh);
}

/**
 * @brief Invert the display. Default is false. This will invert the display buffer.
 *        This is useful when the display is in a bright environment.
 *@param invert true to invert the display, false to revert the display
 */
void i2cDisplay::SetInvertDisplay(bool invert) // Invert the display
{
    _invert = invert;     // track state so it can be re-applied after DISPLAYON/contrast
    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->invertDisplay(invert);
}

/**
 * @brief Set the invert state (live + persistent); re-applied by displayOn().
 * @param invert true to invert the display, false for normal
 */
void i2cDisplay::setInvert(bool invert)
{
    SetInvertDisplay(invert);
}

/**
 * @brief Central font-size control. Level 0/1/2 maps to Adafruit setTextSize(1/2/3).
 * @param level 0 (Normal), 1 (Groß) or 2 (Größer); values >2 are clamped to 2.
 */
void i2cDisplay::setFontSize(uint8_t level)
{
    if (level > 2) level = 2;
    _fontSize = level;

    if (!display)
    {
        logDebugP("setFontSize(%d) - display not initialized", level);
        return;
    }

    display->setTextSize(level + 1); // 0/1/2 -> 1/2/3
    logDebugP("Display font size set to level %d (textSize %d)", level, level + 1);
}

/**
 * @brief Set the Start Line of the display. 0x00 to 0xFF. Default is 0.
 *       This is the display start line register.
 * @param startline of the display
 */
void i2cDisplay::SetDisplayStartLine(uint8_t startline) // Set the display start line
{
    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD1306_SETSTARTLINE | startline);
}

/**
 * @brief Set the display offset. 0x00 to 0x3F. Default is 0x00.
 *        This is the display offset from the top of the display.
 * @param offset of the display
 */
void i2cDisplay::SetDisplayOffset(uint8_t offset) // Set the display offset
{
    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    display->ssd1306_command(offset);
}

/**
 * @brief Set the display clock division. 0x00 to 0xFF. Default is 0x80.
 *        This is the first phase of the OLED charge pump.
 * @param clockdiv of the display
 */
void i2cDisplay::SetDisplayClockDiv(uint8_t clockdiv) // Set the display clock division
{
    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
    display->ssd1306_command(clockdiv);
}

/**
 * @brief Set the display precharge.0x00 to 0xFF. Default is 0xF1.
 *        This is the second phase of the OLED charge pump.
 *        It is used to adjust the contrast of the display.
 * @param precharge of the display
 */
void i2cDisplay::SetDisplayPreCharge(uint8_t precharge) // Set the display precharge
{
    OPENKNX_WIRE1_LOCK(); // shared Wire1 vs LED flush; no-op on RP2040
    display->ssd1306_command(SSD1306_SETPRECHARGE);
    display->ssd1306_command(precharge);
}

/**
 * @brief Display the buffer on the display. This function will compare the current buffer with the previous buffer
 *        and only send the changes to the display. This will reduce the number of updates and increase the speed.
 */
void i2cDisplay::displayBuff()
{
    if (!_curDispBuffer || !_prevDispBuffer || !display) return;

    // Snapshot + diff only, no I2C; loop() pushes the pixels. prev is synced per page only when that
    // page is actually sent (in loop()), so the diff always reflects what is physically on-screen.
    memcpy(_curDispBuffer, display->getBuffer(), _sizeDispBuff);
    const int pages = lcdSettings.height / 8;
    const int width = lcdSettings.width;
    for (int page = 0; page < pages && page < MAX_PAGES; page++)
    {
        int startCol = width, endCol = -1;
        for (int col = 0; col < width; col++)
        {
            const size_t index = (size_t)page * width + col;
            if (_curDispBuffer[index] != _prevDispBuffer[index])
            {
                if (col < startCol) startCol = col;
                if (col > endCol) endCol = col;
            }
        }
        if (endCol >= 0) // this page changed -> (merge and) flag its dirty column range
        {
            if (_pageDirty[page])
            {
                if (startCol < _dirtyStartCol[page]) _dirtyStartCol[page] = (int16_t)startCol;
                if (endCol > _dirtyEndCol[page]) _dirtyEndCol[page] = (int16_t)endCol;
            }
            else
            {
                _dirtyStartCol[page] = (int16_t)startCol;
                _dirtyEndCol[page] = (int16_t)endCol;
                _pageDirty[page] = true;
            }
        }
    }
}

/**
 * @brief Initialize the display buffer. This function will allocate memory for the current and previous display buffer.
 *        The current display buffer will hold the current display data and the previous display buffer will hold the
 *        previous display data. This will allow the display to only update the changes and not the whole display.
 * @return true if the memory allocation was successful
 */
bool i2cDisplay::initDisplayBuffer()
{
    _sizeDispBuff = lcdSettings.width * ((lcdSettings.height + 7) / 8); // Calculate the buffer size
    _curDispBuffer = (uint8_t *)malloc(_sizeDispBuff);                  // Allocate memory for the current display buffer
    _prevDispBuffer = (uint8_t *)malloc(_sizeDispBuff);                 // Allocate memory for the previous display buffer

    if (!_curDispBuffer || !_prevDispBuffer) // Check if the memory allocation was successful
    {
        if (_curDispBuffer) free(_curDispBuffer);   // Free the memory if it was allocated
        if (_prevDispBuffer) free(_prevDispBuffer); // Free the memory if it was allocated
        _curDispBuffer = nullptr;                   // Set the pointer to null
        _prevDispBuffer = nullptr;                  // Set the pointer to null
        return false;                               // Allocation failed
    }
    memset(_curDispBuffer, 0, _sizeDispBuff);  // Clear the current display buffer
    memset(_prevDispBuffer, 0, _sizeDispBuff); // Clear the previous display buffer

    return true; // Allocation successful
}

/**
 * @brief UPdate the page of the display.
 * @param page to update
 * @param startCol of the page
 * @param endCol of the page
 */
void i2cDisplay::updatePage(int page, int startCol, int endCol)
{
    if (startCol > endCol) return;
    // Push in small column chunks, each in its own Wire1 lock scope, so the bus is held <1ms/chunk and
    // the LED flush isn't starved. Each chunk re-arms the page+column window (the LED write may run in between).
    static constexpr int WIRE1_CHUNK_COLS = 32; // ~<1 ms bus hold @ 400 kHz
    for (int c0 = startCol; c0 <= endCol; c0 += WIRE1_CHUNK_COLS)
    {
        const int c1 = (c0 + WIRE1_CHUNK_COLS - 1 < endCol) ? (c0 + WIRE1_CHUNK_COLS - 1) : endCol;
        OPENKNX_WIRE1_LOCK();                                // released each iteration so the LED can slip in
        sendCommandUnlocked(SSD1306_PAGEADDR);               // Set the page address
        sendCommandUnlocked(page);                           // start page
        sendCommandUnlocked(page);                           // end page (single page)
        sendCommandUnlocked(SSD1306_COLUMNADDR);             // Set the column address
        sendCommandUnlocked(c0);                             // chunk start column
        sendCommandUnlocked(c1);                             // chunk end column
        CustomI2C->beginTransmission(lcdSettings.i2cadress); // Begin the transmission of the changes
        CustomI2C->write(0x40);                              // Set the data mode
        for (int col = c0; col <= c1; col++)                 // Loop through this chunk's columns
        {
            CustomI2C->write(_curDispBuffer[page * lcdSettings.width + col]); // Write the data to the display
        }
        CustomI2C->endTransmission(); // End the transmission for this chunk
    }
}

/**
 * @brief Update the display partially, by columns
 * @param startCol - the first column to update; Allowed values [0; lcdSettings.width - 1]
 * @param endCol - the last (included) column to update; Allowed values [0; lcdSettings.width - 1]
 */
void i2cDisplay::updateCols(int startCol, int endCol)
{
    if (startCol > endCol) return;
    // Whole multi-page column push is one Wire1 critical section. No-op on RP2040.
    OPENKNX_WIRE1_LOCK();
    sendCommandUnlocked(SSD1306_PAGEADDR);                    // Set the page address
    sendCommandUnlocked(0);                                   // Set the page
    sendCommandUnlocked((lcdSettings.height / 8) - 1);        // Set the page
    sendCommandUnlocked(SSD1306_COLUMNADDR);                  // Set the column address
    sendCommandUnlocked(startCol);                            // Set the start column
    sendCommandUnlocked(endCol);                              // Set the end column
    CustomI2C->beginTransmission(lcdSettings.i2cadress);      // Begin the transmission of the changes
    CustomI2C->write(0x40);                                   // Set the data mode
    for (int page = 0; page < lcdSettings.height / 8; page++) // Loop through the pages of the display
    {
        for (int col = startCol; col <= endCol; col++) // Loop through the columns
        {
            CustomI2C->write(_curDispBuffer[page * lcdSettings.width + col]); // Write the data to the display
        }
    }
    CustomI2C->endTransmission(); // End the transmission
}

/**
 * @brief Send a single command to the display (one atomic Wire1 transaction). No-op lock on RP2040.
 * @param command to send
 */
void i2cDisplay::sendCommand(uint8_t command)
{
    OPENKNX_WIRE1_LOCK();
    sendCommandUnlocked(command);
}

/**
 * @brief Raw command send without taking the Wire1 lock. Only call from a context that already holds it.
 * @param command to send
 */
void i2cDisplay::sendCommandUnlocked(uint8_t command)
{
    CustomI2C->beginTransmission(lcdSettings.i2cadress);
    CustomI2C->write(0x00);
    CustomI2C->write(command);
    CustomI2C->endTransmission();
}

/**
 * @brief Update the display area with the current buffer.
 * @param x position of the display
 * @param y position of the display
 * @param byteIndex of the display buffer
 */
void i2cDisplay::updateArea(int x, int y, int byteIndex)
{
    int page = y / 8; // Page from 0 to 7
    int column = x;   // Column from 0 to 127

    // Whole area push is one Wire1 critical section. No-op on RP2040.
    OPENKNX_WIRE1_LOCK();
    sendCommandUnlocked(SSD1306_PAGEADDR); // Set the page address
    sendCommandUnlocked(page);             // Set the page
    sendCommandUnlocked(page);             // Set the page second time, because the display expects two values

    sendCommandUnlocked(SSD1306_COLUMNADDR);    // Set the column address
    sendCommandUnlocked(column);                // Set the column
    sendCommandUnlocked(lcdSettings.width - 1); // Set the last column

    CustomI2C->beginTransmission(lcdSettings.i2cadress); // Send the changes pixel by pixel
    CustomI2C->write(0x40);                              // Data mode
    CustomI2C->write(_curDispBuffer[byteIndex]);         // Write the data to the display
    CustomI2C->endTransmission();                        // End the transmission
}

/**
 * @brief Display the full buffer on the display.
 */
void i2cDisplay::displayFullBuffer()
{
    // Whole-frame push is one Wire1 critical section. No-op on RP2040.
    OPENKNX_WIRE1_LOCK();
    sendCommandUnlocked(SSD1306_PAGEADDR);
    sendCommandUnlocked(0);                            // First page
    sendCommandUnlocked((lcdSettings.height / 8) - 1); // Last page
    sendCommandUnlocked(SSD1306_COLUMNADDR);
    sendCommandUnlocked(0);                     // First column
    sendCommandUnlocked(lcdSettings.width - 1); // Last column

    CustomI2C->beginTransmission(lcdSettings.i2cadress);
    CustomI2C->write(0x40); // Data mode
    for (int i = 0; i < lcdSettings.width * ((lcdSettings.height + 7) / 8); i++)
    {
        CustomI2C->write(_curDispBuffer[i]);
    }
    CustomI2C->endTransmission();
}

/**
 * @brief Sets the display brightness (0-100%)
 *
 */
void i2cDisplay::setBrightness(uint8_t brightness)
{
    _brightness = brightness;

    if (!display)
    {
        logDebugP("setBrightness(%d) - display not initialized", brightness);
        return;
    }

    // Map 0-100% to 0-255 for SSD1306 contrast
    uint8_t contrast = map(brightness, 0, 100, 0, 255);

    // Map 0-100% to VCOM range (0x00 to 0x40)
    // Bei 0% Brightness: VCOM = 0x00 (niedrigste Spannung)
    // Bei 100% Brightness: VCOM = 0x20 (Standard-Wert für volle Helligkeit)
    uint8_t vcom = map(brightness, 0, 100, 0, 0x20);

    // Set contrast and VCOM
    SetDisplayContrast(contrast);
    SetDisplayVCOMDetect(vcom);

    logDebugP("Display brightness set to %d%% (contrast: %d, VCOM: 0x%02X)", brightness, contrast, vcom);
}

/**
 * @brief Turns the display on
 */
void i2cDisplay::displayOn()
{
    _displayOn = true;

    if (!display)
    {
        logDebugP("displayOn() - display not initialized");
        return;
    }

    // Scoped lock on the raw display-> call only; setBrightness()/SetDisplay* self-lock (non-recursive mutex).
    {
        OPENKNX_WIRE1_LOCK();
        display->ssd1306_command(SSD1306_DISPLAYON);
    }

    // Restore last brightness (contrast and VCOM) - SetDisplayContrast/VCOMDetect self-lock Wire1
    setBrightness(_brightness);

    // Re-apply invert; the SSD1306 can reset it after DISPLAYON/contrast changes.
    {
        OPENKNX_WIRE1_LOCK();
        display->invertDisplay(_invert);
    }

    logDebugP("Display turned ON (invert=%d)", _invert);
}

/**
 * @brief Turns the display off
 */
void i2cDisplay::displayOff()
{
    _displayOn = false;

    if (!display)
    {
        logDebugP("displayOff() - display not initialized");
        return;
    }

    // Optional: Set contrast and VCOM to 0 before turning off (these self-lock Wire1)
    SetDisplayContrast(0x00);
    SetDisplayVCOMDetect(0x00);

    // Scoped lock on the raw display-> call only (SetDisplay* helpers self-lock). No-op on RP2040.
    {
        OPENKNX_WIRE1_LOCK();
        display->ssd1306_command(SSD1306_DISPLAYOFF);
    }

    logDebugP("Display turned OFF");
}