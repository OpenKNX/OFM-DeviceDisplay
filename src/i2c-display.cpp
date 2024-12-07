#include "i2c-display.h"

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
    delete display;                            // Remove the display object from the memory (Adafruit_SSD1306)
    delete CustomI2C;                          // Remove the i2c object from the memory (TwoWire)
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

    CustomI2C = new TwoWire(lcdSettings.i2cInst, lcdSettings.sda, lcdSettings.scl);
    display = new Adafruit_SSD1306(lcdSettings.width, lcdSettings.height, CustomI2C, lcdSettings.reset, 1000000UL, 1000000UL);

    if (!display->begin(SSD1306_SWITCHCAPVCC, lcdSettings.i2cadress, true, true))
    {
        return false; // Display not found or not initialized. Check the wiring and i2c address
    }

    display->clearDisplay(); // Clear initialy the display buffer. Previous arcifacts could be displayed
    display->display();      // Display the cleared buffer

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

// update
void i2cDisplay::loop()
{
    if (_loopColumn < lcdSettings.width && openknx.freeLoopTime())
    {
        updateCols(_loopColumn, _loopColumn+4-1);
        _loopColumn += 4;
    }
    else if (_loopColumn == 0xff)
    {
        _loopColumn = 0;
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
    display->ssd1306_command(SSD1306_SETCONTRAST);
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

    display->ssd1306_command(SSD1306_SETVCOMDETECT);
    display->ssd1306_command(vcomh);
}

/**
 * @brief Invert the display. Default is false. This will invert the display buffer.
 *        This is useful when the display is in a bright environment.
 *@param invert true to invert the display, false to revert the display
 */
void i2cDisplay::SetInvertDisplay(bool invert) // Invert the display
{
    // display->ssd1306_command(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
    display->invertDisplay(invert);
}

/**
 * @brief Set the Start Line of the display. 0x00 to 0xFF. Default is 0.
 *       This is the display start line register.
 * @param startline of the display
 */
void i2cDisplay::SetDisplayStartLine(uint8_t startline) // Set the display start line
{
    display->ssd1306_command(SSD1306_SETSTARTLINE | startline);
}

/**
 * @brief Set the display offset. 0x00 to 0x3F. Default is 0x00.
 *        This is the display offset from the top of the display.
 * @param offset of the display
 */
void i2cDisplay::SetDisplayOffset(uint8_t offset) // Set the display offset
{
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
    display->ssd1306_command(SSD1306_SETPRECHARGE);
    display->ssd1306_command(precharge);
}

/**
 * @brief Display the buffer on the display. This function will compare the current buffer with the previous buffer
 *        and only send the changes to the display. This will reduce the number of updates and increase the speed.
 */
void i2cDisplay::displayBuff()
{
    memcpy(_curDispBuffer, display->getBuffer(), _sizeDispBuff); // Copy the display buffer to the current buffer
    _loopColumn = 0xff;

    /*
    for (int page = 0; page < lcdSettings.height / 8; page++)    // Loop through the pages of the display
    {
        int startColumn = lcdSettings.width; // Set the start column to the width of the display
        int endColumn = -1;                  // Set the end column to -1
        bool pageChanged = false;            // Set the page changed flag to false.

        for (int col = 0; col < lcdSettings.width; col++) // Loop through the columns of the display
        {
            size_t index = page * lcdSettings.width + col;       // Calculate the index of the current byte
            if (_curDispBuffer[index] != _prevDispBuffer[index]) // Check if the current byte is different from the previous byte
            {
                pageChanged = true;
                if (col < startColumn) startColumn = col;       // Set the start column
                if (col > endColumn) endColumn = col;           // Set the end column
                _prevDispBuffer[index] = _curDispBuffer[index]; // Update the previous buffer
            }
        }
        if (pageChanged) // Update only if the page has changed. This will reduce the number of updates
            updatePage(page, startColumn, endColumn);
    }
    */
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
    if (startCol > endCol) return;                       // Return if the start column is greater than the end column
    sendCommand(SSD1306_PAGEADDR);                       // Set the page address
    sendCommand(page);                                   // Set the page
    sendCommand(page);                                   // Set the page
    sendCommand(SSD1306_COLUMNADDR);                     // Set the column address
    sendCommand(startCol);                               // Set the start column
    sendCommand(endCol);                                 // Set the end column
    CustomI2C->beginTransmission(lcdSettings.i2cadress); // Begin the transmission of the changes
    CustomI2C->write(0x40);                              // Set the data mode
    for (int col = startCol; col <= endCol; col++)       // Loop through the columns
    {
        CustomI2C->write(_curDispBuffer[page * lcdSettings.width + col]); // Write the data to the display
    }
    CustomI2C->endTransmission(); // End the transmission
}

/**
 * @brief UPdate the page of the display.
 * @param page to update
 * @param startCol of the page
 * @param endCol of the page
 */
void i2cDisplay::updateCols(int startCol, int endCol)
{
    if (startCol > endCol) return;                       // Return if the start column is greater than the end column
    sendCommand(SSD1306_PAGEADDR);                       // Set the page address
    sendCommand(0);                                      // Set the page
    sendCommand((lcdSettings.height / 8) - 1);           // Set the page
    sendCommand(SSD1306_COLUMNADDR);                     // Set the column address
    sendCommand(startCol);                               // Set the start column
    sendCommand(endCol);                                 // Set the end column
    CustomI2C->beginTransmission(lcdSettings.i2cadress); // Begin the transmission of the changes
    CustomI2C->write(0x40);                              // Set the data mode
    for (int page = 0; page < lcdSettings.height / 8; page++)    // Loop through the pages of the display
    {
        for (int col = startCol; col <= endCol; col++)       // Loop through the columns
        {
            CustomI2C->write(_curDispBuffer[page * lcdSettings.width + col]); // Write the data to the display
        }
    }
    CustomI2C->endTransmission(); // End the transmission
}

/**
 * @brief Send a command to the display.
 * @param command to send
 */
void i2cDisplay::sendCommand(uint8_t command)
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

    sendCommand(SSD1306_PAGEADDR); // Set the page address
    sendCommand(page);             // Set the page
    sendCommand(page);             // Set the page second time, because the display expects two values

    sendCommand(SSD1306_COLUMNADDR);    // Set the column address
    sendCommand(column);                // Set the column
    sendCommand(lcdSettings.width - 1); // Set the last column

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
    sendCommand(SSD1306_PAGEADDR);
    sendCommand(0);                            // First page
    sendCommand((lcdSettings.height / 8) - 1); // Last page
    sendCommand(SSD1306_COLUMNADDR);
    sendCommand(0);                     // First column
    sendCommand(lcdSettings.width - 1); // Last column

    CustomI2C->beginTransmission(lcdSettings.i2cadress);
    CustomI2C->write(0x40); // Data mode
    for (int i = 0; i < lcdSettings.width * ((lcdSettings.height + 7) / 8); i++)
    {
        CustomI2C->write(_curDispBuffer[i]);
    }
    CustomI2C->endTransmission();
}
