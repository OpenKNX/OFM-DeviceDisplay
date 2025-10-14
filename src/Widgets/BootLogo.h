#pragma once
#include "../Widget.h"

class WidgetBootLogo : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetBootLogo"; }
    
    struct LogoBitmap
    {
        const uint8_t *data;
        uint8_t width;
        uint8_t height;
    };

    WidgetBootLogo(uint32_t displayTime, WidgetFlags action, const LogoBitmap *logo = nullptr);
    
    inline void setLogo(const LogoBitmap *logo) { _logoBitmap = logo; }
    
    /**
     * @brief Set the step height for incremental drawing
     * @param stepHeight Number of rows to draw per loop() call (1-64)
     */
    void setStepHeight(uint8_t stepHeight);
    
    /**
     * @brief Calculate optimal step height based on available loop time
     * @param maxLoopTimeUs Maximum loop time in microseconds (default: 2000µs)
     */
    void calculateOptimalStepHeight(uint32_t maxLoopTimeUs = 2000);

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void setup() override;
    void loop() override;
    
    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }

    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }

  private:
    void drawBootLogo();

    WidgetState _state;
    uint32_t _displayTime;
    WidgetFlags _action;
    i2cDisplay *_display;
    bool _needsRedraw;
    std::string _name = "BootLogo";

    /** state of partial drawing:
     * 0=no drawing / done,
     * 1=clear + first stripe,
     * 2=partial draw remaining stripes,
     * 3=send to display
     */
    uint8_t _drawStep = 0;

    /** Number of rows to draw in one loop() call */
    uint8_t _stepHeight = 8;  // Default: 8 rows (~1.6ms/loop), not const anymore!

    /** Row to start drawing in one loop() call */
    uint8_t _yStart = 0;
    
    /** Target loop time in microseconds */
    static constexpr uint32_t TARGET_LOOP_TIME_US = 2000;  // 2ms
    
    /** Estimated time per row in microseconds (empirically determined) */
    static constexpr uint32_t TIME_PER_ROW_US = 200;  // ~200µs per row
    
    // ← OPTIMIZATION: Cache display dimensions (calculated once in setup())
    uint16_t _displayWidth = 0;
    uint16_t _displayHeight = 0;
    
    // ← OPTIMIZATION: Cache centered position (calculated once in setup())
    uint16_t _xCenter = 0;
    uint16_t _yCenter = 0;
    
    // ← OPTIMIZATION: Cache bitmap properties (calculated once in setup())
    uint16_t _bytesPerRow = 0;

  protected:
    const LogoBitmap *_logoBitmap = nullptr;
};