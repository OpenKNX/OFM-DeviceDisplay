#ifdef DEVICE_DISPLAY_MODULE
    #include "About.h"
    #include "OpenKNX.h"
    #include <math.h>
    #include <string.h>

namespace
{
    constexpr int16_t BUS_Y = 17;      // y of the horizontal line bus
    constexpr int16_t BUS_X0 = 42;     // bus left end
    constexpr int16_t BUS_X1 = 86;     // bus right end
    constexpr uint32_t CRED_MS = 2600; // ms each credit line is shown

    // Credit lines. The last carries the CP437 byte 0x80 == "Ç" so "Erkan Çolak" renders correctly
    // (Adafruit default font is CP437; we enable cp437(true) in start()).
    const char *const CRED[3] = {"(C) 2026 OpenKNX", "DeviceDisplay v0.1", "Erkan \x80olak"};

    // Scramble glyphs for the decrypt effect (printable ASCII only; the resolved char may be 0x80).
    const char GLY[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789#%&@?/<>*+=";
} // namespace

WidgetAbout::WidgetAbout(uint32_t displayTime, WidgetFlags action)
    : _state(WidgetState::STOPPED), _displayTime(displayTime), _action(action), _display(nullptr)
{
}

void WidgetAbout::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr) logErrorP("Display is NULL!");
}

void WidgetAbout::start()
{
    if (_state == WidgetState::RUNNING) return;
    logDebugP("Starting...");
    restart();
}

void WidgetAbout::restart()
{
    _state = WidgetState::RUNNING;
    _startTime = millis();
    _lastUpdate = _startTime;
    for (uint8_t n = 0; n < NODE_COUNT; ++n)
        _flash[n] = 0.0f;
    for (uint8_t i = 0; i < PKT_COUNT; ++i)
    {
        spawnPacket(_pkt[i]);
        _pkt[i].u = static_cast<float>(random(0, 100)) / 100.0f; // stagger initial progress
    }
    if (_display && _display->display) _display->display->cp437(true); // map 0x80 -> Ç
}

void WidgetAbout::stop()
{
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetAbout::pause()
{
    if (_state == WidgetState::RUNNING) _state = WidgetState::PAUSED;
}

void WidgetAbout::resume()
{
    if (_state == WidgetState::PAUSED) _state = WidgetState::RUNNING;
}

void WidgetAbout::loop()
{
    if (_state != WidgetState::RUNNING || !_display) return;

    const uint32_t now = millis();
    if (now - _lastUpdate < 33) return; // ~30 fps
    _lastUpdate = now;

    // advance packets + decay node flashes
    for (uint8_t i = 0; i < PKT_COUNT; ++i)
    {
        _pkt[i].u += _pkt[i].spd;
        if (_pkt[i].u >= 1.0f)
        {
            if (_pkt[i].dst < NODE_COUNT) _flash[_pkt[i].dst] = 1.0f; // arrival pulse
            spawnPacket(_pkt[i]);
        }
    }
    for (uint8_t n = 0; n < NODE_COUNT; ++n)
        if (_flash[n] > 0.0f) _flash[n] -= 0.06f;

    render(now - _startTime);
}

uint32_t WidgetAbout::getDisplayTime() const { return _displayTime; }
WidgetFlags WidgetAbout::getAction() const { return _action; }

void WidgetAbout::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetAbout::getDisplayModule() const { return _display; }

void WidgetAbout::spawnPacket(Packet &p)
{
    p.src = static_cast<uint8_t>(random(0, NODE_COUNT));
    do
    {
        p.dst = static_cast<uint8_t>(random(0, NODE_COUNT));
    }
    while (p.dst == p.src);
    p.u = 0.0f;
    p.spd = 0.006f + static_cast<float>(random(0, 11)) * 0.001f; // 0.006..0.016 / tick
}

void WidgetAbout::nodeXY(uint8_t n, int16_t &x, int16_t &y) const
{
    switch (n)
    {
        case 0:
            x = 64;
            y = 6;
            break; // coupler on top
        case 1:
            x = 48;
            y = 28;
            break;
        case 2:
            x = 64;
            y = 28;
            break;
        default:
            x = 80;
            y = 28;
            break; // node 3
    }
}

// Point on the src -> (src,bus) -> (dst,bus) -> dst polyline at fraction u (all segments are
// axis-aligned, so segment length is a simple Manhattan distance).
void WidgetAbout::pointAt(const Packet &p, float u, int16_t &outX, int16_t &outY) const
{
    int16_t sx, sy, dx, dy;
    nodeXY(p.src, sx, sy);
    nodeXY(p.dst, dx, dy);
    const float px[4] = {static_cast<float>(sx), static_cast<float>(sx), static_cast<float>(dx), static_cast<float>(dx)};
    const float py[4] = {static_cast<float>(sy), static_cast<float>(BUS_Y), static_cast<float>(BUS_Y), static_cast<float>(dy)};

    float len[3], total = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        len[i] = fabsf(px[i + 1] - px[i]) + fabsf(py[i + 1] - py[i]);
        total += len[i];
    }
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    float d = u * total, acc = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        if (acc + len[i] >= d || i == 2)
        {
            const float f = (len[i] > 0.0f) ? (d - acc) / len[i] : 0.0f;
            outX = static_cast<int16_t>(lroundf(px[i] + (px[i + 1] - px[i]) * f));
            outY = static_cast<int16_t>(lroundf(py[i] + (py[i + 1] - py[i]) * f));
            return;
        }
        acc += len[i];
    }
    outX = dx;
    outY = dy;
}

void WidgetAbout::drawReticle()
{
    Adafruit_SSD1306 *d = _display->display;
    const int16_t W = 127, H = 63, a = 5;
    d->drawFastHLine(0, 0, a, WHITE);
    d->drawFastVLine(0, 0, a, WHITE);
    d->drawFastHLine(W - a + 1, 0, a, WHITE);
    d->drawFastVLine(W, 0, a, WHITE);
    d->drawFastHLine(0, H, a, WHITE);
    d->drawFastVLine(0, H - a + 1, a, WHITE);
    d->drawFastHLine(W - a + 1, H, a, WHITE);
    d->drawFastVLine(W, H - a + 1, a, WHITE);
}

void WidgetAbout::drawTopology()
{
    Adafruit_SSD1306 *d = _display->display;

    // bus line + node stubs
    d->drawFastHLine(BUS_X0, BUS_Y, BUS_X1 - BUS_X0, WHITE);
    for (uint8_t n = 0; n < NODE_COUNT; ++n)
    {
        int16_t nx, ny;
        nodeXY(n, nx, ny);
        if (ny < BUS_Y)
            d->drawFastVLine(nx, ny, BUS_Y - ny, WHITE);
        else
            d->drawFastVLine(nx, BUS_Y, ny - BUS_Y, WHITE);
    }

    // node squares (coupler a touch larger); pulse filled on arrival
    for (uint8_t n = 0; n < NODE_COUNT; ++n)
    {
        int16_t nx, ny;
        nodeXY(n, nx, ny);
        const int16_t sz = (n == 0) ? 5 : 4;
        const int16_t x0 = static_cast<int16_t>(nx - sz / 2), y0 = static_cast<int16_t>(ny - sz / 2);
        d->fillRect(x0, y0, sz, sz, BLACK); // clear the stub/bus under the node
        if (_flash[n] > 0.45f)
            d->fillRect(x0, y0, sz, sz, WHITE);
        else
            d->drawRect(x0, y0, sz, sz, WHITE);
    }
}

void WidgetAbout::drawPackets()
{
    Adafruit_SSD1306 *d = _display->display;
    for (uint8_t i = 0; i < PKT_COUNT; ++i)
    {
        int16_t hx, hy;
        pointAt(_pkt[i], _pkt[i].u, hx, hy);
        d->fillRect(static_cast<int16_t>(hx - 1), static_cast<int16_t>(hy - 1), 2, 2, WHITE); // head
        for (uint8_t s = 1; s < 5; ++s)
        {
            const float uu = _pkt[i].u - s * 0.03f;
            if (uu < 0.0f) break;
            int16_t tx, ty;
            pointAt(_pkt[i], uu, tx, ty);
            d->drawPixel(tx, ty, WHITE); // fading trail (single pixels)
        }
    }
}

void WidgetAbout::drawCredits(uint32_t elapsed)
{
    Adafruit_SSD1306 *d = _display->display;

    const uint8_t idx = static_cast<uint8_t>((elapsed / CRED_MS) % 3);
    const float ph = static_cast<float>(elapsed % CRED_MS) / static_cast<float>(CRED_MS);
    float prog = (ph < 0.35f) ? (ph / 0.35f) : ((ph > 0.82f) ? (1.0f - (ph - 0.82f) / 0.18f) : 1.0f);
    if (prog < 0.0f) prog = 0.0f;

    const char *t = CRED[idx];
    const size_t len = strlen(t);
    const int16_t glyCount = static_cast<int16_t>(sizeof(GLY) - 1);

    int16_t x = static_cast<int16_t>((128 - static_cast<int16_t>(len * 6)) / 2);
    if (x < 0) x = 0;

    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);
    d->setCursor(x, 48);
    for (size_t k = 0; k < len; ++k)
    {
        const char c = t[k];
        uint8_t out;
        if (prog > static_cast<float>(k) / static_cast<float>(len))
            out = static_cast<uint8_t>(c); // resolved (may be 0x80 == Ç)
        else if (c == ' ')
            out = ' ';
        else
            out = static_cast<uint8_t>(GLY[random(0, glyCount)]); // scramble
        d->write(out);
    }

    // progress dots (which of the 3 credits is showing)
    for (uint8_t dot = 0; dot < 3; ++dot)
        d->fillRect(static_cast<int16_t>(58 + dot * 6), 60, (dot == idx) ? 4 : 2, 2, WHITE);
}

void WidgetAbout::render(uint32_t elapsed)
{
    if (!_display || !_display->display) return;
    _display->display->clearDisplay();
    _display->display->setTextWrap(false);

    drawReticle();
    drawTopology();
    drawPackets();
    drawCredits(elapsed);

    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE
