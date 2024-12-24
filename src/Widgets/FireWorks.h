#pragma once
#include "../Widget.h"

class WidgetFireworks : public Widget
{
public:
    const std::string logPrefix() { return "WidgetFireworks"; }
    
    WidgetFireworks(uint32_t displayTime, WidgetFlags action, uint8_t intensity);
    ~WidgetFireworks();
    
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void setup() override;
    void loop() override;
    
    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string& name) override { _name = name; }
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }
    inline void setDisplayModule(i2cDisplay* displayModule) override { _display = displayModule; }
    inline i2cDisplay* getDisplayModule() const override { return _display; }

private:
    struct Particle {
        int16_t x;      // Position (x * 16 for subpixel precision)
        int16_t y;      // Position (y * 16)
        int8_t vx;      // Speed X (-128 to 127)
        int8_t vy;      // Speed Y
        uint8_t life;   // Particle lifetime
        bool active;    // Is the Particle active?
    };

    struct Firework {
        static const uint8_t MAX_PARTICLES = 12;
        Particle particles[MAX_PARTICLES];
        uint8_t x;
        uint8_t y;
        uint16_t properties;
        uint8_t activeParticles;
    };

    inline uint8_t getRadius(const Firework& fw) { return (fw.properties & 0xF); }
    inline void setRadius(Firework& fw, uint8_t radius) { fw.properties = (fw.properties & ~0xF) | (radius & 0xF); }
    inline uint8_t getMaxRadius(const Firework& fw) { return (fw.properties >> 4) & 0xF; }
    inline void setMaxRadius(Firework& fw, uint8_t maxRadius) { fw.properties = (fw.properties & ~(0xF << 4)) | ((maxRadius & 0xF) << 4); }
    inline bool isActive(const Firework& fw) { return (fw.properties >> 8) & 0x1; }
    inline void setActive(Firework& fw, bool active) { fw.properties = (fw.properties & ~(1 << 8)) | (active ? (1 << 8) : 0); }

    void launchFirework();
    void updateFireworks();
    void drawFireworks();
    
    WidgetFlags _action;
    uint8_t _intensity;
    uint32_t _displayTime;
    i2cDisplay* _display;
    Firework* _fireworks;
    uint8_t _fireworkCount;
    unsigned long _lastUpdateTime;
    uint8_t _updateInterval;
    WidgetState _state;
    std::string _name = "Fireworks";

    // Physik-Konstanten
    static constexpr int8_t GRAVITY = 1;
    static constexpr int8_t DRAG = -1;
    static constexpr uint8_t SUBPIXEL_BITS = 4; // For 1/16 subpixel precision
};