#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    #include "../Widget.h"

// WidgetAbout: the "Über" / About screen — a SciFi HUD that represents the developer. A live KNX
// topology (coupler on a line bus with three devices) sits inside reticle corner brackets while
// data PACKETS travel node -> bus -> node with a fading trail; the destination node pulses on
// arrival. Below it the credits DECRYPT into place, cycling
//   (C) 2026 OpenKNX  /  DeviceDisplay v0.1  /  Erkan Colak
// (the last with the correct "Ç" via the CP437 glyph). Pure Adafruit-GFX (points/lines/text), no
// bitmaps, so it runs smoothly on the 128x64 OLED. Design: "DataPackets Stil 1" (flat bus). :)
class WidgetAbout : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetAbout"; }
    WidgetAbout(uint32_t displayTime, WidgetFlags action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;

    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

    uint32_t getDisplayTime() const override;
    WidgetFlags getAction() const override;
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }

    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

    // Restart the animation from the top (fresh credit cycle + packets). Called by DeviceDisplay
    // each time the About screen is opened so it always begins on "(C) 2026 OpenKNX".
    void restart();

  private:
    static constexpr uint8_t NODE_COUNT = 4; // coupler + 3 devices
    static constexpr uint8_t PKT_COUNT = 5;  // concurrent packets on the bus

    struct Packet
    {
        uint8_t src; // source node index
        uint8_t dst; // destination node index
        float u;     // progress along the src->bus->dst polyline [0..1]
        float spd;   // progress per tick
    };

    WidgetState _state;
    uint32_t _displayTime;
    WidgetFlags _action;
    i2cDisplay *_display;
    uint32_t _lastUpdate = 0;
    uint32_t _startTime = 0;
    std::string _name = "About";

    Packet _pkt[PKT_COUNT];
    float _flash[NODE_COUNT] = {0, 0, 0, 0}; // per-node arrival pulse (decays each tick)

    void spawnPacket(Packet &p);                                          // pick random src != dst, reset progress
    void nodeXY(uint8_t n, int16_t &x, int16_t &y) const;                 // node centre
    void pointAt(const Packet &p, float u, int16_t &x, int16_t &y) const; // point on the polyline at u
    void drawReticle();                                                   // SciFi corner brackets
    void drawTopology();                                                  // bus + stubs + node squares (+ flashes)
    void drawPackets();                                                   // moving packets with trail
    void drawCredits(uint32_t elapsed);                                   // decrypting credit line + progress dots
    void render(uint32_t elapsed);
};
#endif // DEVICE_DISPLAY_MODULE
