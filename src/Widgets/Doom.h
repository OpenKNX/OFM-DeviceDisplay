#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    #include "../Widget.h"

// WidgetDoom: a Wolfenstein/DOOM-style RAYCASTER screensaver. A bot auto-walks a small maze
// and the first-person 3D view is rendered on the 128x64 OLED — vertical wall slices whose
// height is 1/distance, monochrome depth via dithering (near = solid, far = sparse), and the
// two wall orientations shaded differently so corners read. Pure Adafruit-GFX (drawPixel /
// drawFastVLine), no assets. SciFi easter egg for Erkan. :)
class WidgetDoom : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetDoom"; }
    WidgetDoom(uint32_t displayTime, WidgetFlags action);

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

  private:
    WidgetState _state;
    uint32_t _displayTime;
    WidgetFlags _action;
    i2cDisplay *_display;
    uint32_t _lastUpdate = 0;
    std::string _name = "Doom";

    // Bot state (world units = map cells; sub-cell float position). The bot navigates the maze like a
    // corridor-follower: it walks cell-to-cell toward the target cell centre, prefers going straight,
    // turns into open passages at junctions and only reverses at dead-ends (no more dumb ping-pong).
    float _px = 1.5f, _py = 1.5f; // position
    float _dir = 0.0f;            // camera heading (radians), smoothly turned toward the travel dir
    int _tgtX = 1, _tgtY = 1;     // target cell the bot is currently walking into

    // Chunked (non-blocking) raycaster: rendering all 128 columns in one call is ~68ms. Instead the
    // frame is rendered RENDER_COLS_PER_LOOP columns per loop() against a per-frame SNAPSHOT of the
    // bot geometry, so the walls stay consistent while the frame is drawn across several loops.
    static constexpr int16_t RENDER_COLS_PER_LOOP = 32; // ~17ms/loop instead of ~68ms/frame
    static constexpr uint32_t FRAME_MS = 55;            // ~18 fps cap (throttle between frames)
    int16_t _renderCol = 0;                             // next column to render for the current frame
    bool _frameDone = true;                             // true -> start a fresh frame (step + snapshot) on the next loop
    uint32_t _lastFrameStart = 0;                       // millis() of the current frame start (fps cap)
    float _snapPx = 0, _snapPy = 0;                     // bot position frozen for the frame being drawn
    float _snapDirX = 0, _snapDirY = 0;                 // camera direction vector (frozen)
    float _snapPlaneX = 0, _snapPlaneY = 0;             // camera plane vector (frozen)

    // Hidden-feature Easter egg: a "DOOM" title that ZOOMS to full-screen on start (as if you run into
    // the lettering), and while the bot walks the maze the screensaver periodically flashes full-screen
    // cards -> DOOM, the author name (ERKAN COLAK), a copyright line, and "DeviceDisplay". Self-contained
    // and non-blocking (the diffed flush only pushes what actually changed).
    enum class Phase : uint8_t
    {
        Title,
        Play,
        Banner
    };
    Phase _phase = Phase::Title;
    uint32_t _phaseStart = 0;                     // millis() when the current phase began
    uint32_t _lastAnimFrame = 0;                  // throttle for title/banner redraws
    uint8_t _bannerIdx = 0;                       // which banner card comes next
    static constexpr uint32_t TITLE_MS = 2600;    // intro title duration
    static constexpr uint32_t PLAY_MS = 22000;    // walk time between banner reveals
    static constexpr uint32_t BANNER_MS = 2800;   // banner card on-screen duration
    static constexpr uint32_t ANIM_FRAME_MS = 90; // title/banner redraw throttle (~11 fps)
    static constexpr uint8_t BANNER_COUNT = 4;    // DOOM / name / copyright / DeviceDisplay

    bool isWall(int mx, int my) const;                                          // true if map cell is solid / out of bounds
    void step();                                                                // advance the bot toward its target cell
    void pickNextCell();                                                        // choose the next target cell at a junction
    void renderColumn(int16_t x, int16_t W, int16_t H);                         // raycast + draw ONE column (chunked)
    void drawTitle(uint32_t elapsed, int16_t W, int16_t H);                     // zooming full-screen DOOM
    void drawBanner(uint8_t idx, uint32_t elapsed, int16_t W, int16_t H);       // hidden-feature cards
    void drawCenteredText(const char *txt, uint8_t size, int16_t y, int16_t W); // centered GFX text
};
#endif // DEVICE_DISPLAY_MODULE
