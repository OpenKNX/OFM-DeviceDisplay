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
    display.reset();   // Remove the display object from the memory (Adafruit_SSD1306)
    CustomI2C.reset(); // Remove the i2c object from the memory (TwoWire)
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
    CustomI2C = std::make_unique<TwoWire>(lcdSettings.i2cInst, lcdSettings.sda, lcdSettings.scl);
    display = std::make_unique<Adafruit_SSD1306>(lcdSettings.width, lcdSettings.height, CustomI2C.get(), lcdSettings.reset);

    if (!display->begin(SSD1306_SWITCHCAPVCC, lcdSettings.i2cadress, true, true))
    {
        return false; // Display not found or not initialized. Check the wiring and i2c address
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
void i2cDisplay::SetDisplayI2C(i2c_inst_t* i2cInst)
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
void i2cDisplay::SetDisplaySettings(uint8_t width, uint8_t height, uint8_t i2cadress, int8_t reset, i2c_inst_t* i2cInst, pin_size_t sda, pin_size_t scl)
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
    //display->ssd1306_command(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
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


