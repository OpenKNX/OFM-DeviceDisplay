#ifdef DEVICE_DISPLAY_MODULE
    #include "WidgetTime.h"
    #include "OpenKNX.h"

// German weekday names, indexed by DateOnly::dayOfWeek (0 = Sunday .. 6 = Saturday).
// Matches the authoritative mock (reg2-menu-mockup.html: WD[]).
static const char *const WT_WEEKDAYS_DE[7] = {
    "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};

// Constructor
WidgetTime::WidgetTime(uint32_t displayTime, WidgetFlags action)
    : _displayTime(displayTime), _action(action), _display(nullptr),
      _state(WidgetState::STOPPED), _lastSecondKey(0), _forceRedraw(true) {}

// Sets the display module
void WidgetTime::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;

    logInfoP("Set display module...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
    }
}

// Retrieves the display module
i2cDisplay *WidgetTime::getDisplayModule() const
{
    return _display;
}

// Returns the display time in milliseconds
uint32_t WidgetTime::getDisplayTime() const
{
    return _displayTime;
}

// Returns the widget action
WidgetFlags WidgetTime::getAction() const
{
    return _action;
}

// True when no valid time source is available (used for the "keine Zeit" badge)
bool WidgetTime::isTimeMissing() const
{
    return !openknx.time.isValid();
}

// Setup method for widget initialization
void WidgetTime::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }
}

// Starts the widget
void WidgetTime::start()
{
    logDebugP("Start...");
    _state = WidgetState::RUNNING;
    _forceRedraw = true; // Redraw immediately on first loop tick
}

// Stops the widget
void WidgetTime::stop()
{
    logDebugP("Stop...");
    _state = WidgetState::STOPPED;
}

// Pauses the widget. Note: the clock keeps ticking while paused (see loop()).
void WidgetTime::pause()
{
    logDebugP("Pause...");
    if (_state == WidgetState::RUNNING) _state = WidgetState::PAUSED;
}

// Resumes the widget
void WidgetTime::resume()
{
    logDebugP("Resume...");
    if (_state == WidgetState::PAUSED)
    {
        _state = WidgetState::RUNNING;
        _forceRedraw = true; // Refresh immediately after resume
    }
}

// Returns the "second key" that gates the per-second partial redraw:
// wall-clock second when time is valid, otherwise the uptime second.
uint32_t WidgetTime::currentSecondKey() const
{
    if (openknx.time.isValid())
    {
        auto now = openknx.time.getLocalTime();
        return static_cast<uint32_t>(now.hour) * 3600u +
               static_cast<uint32_t>(now.minute) * 60u +
               static_cast<uint32_t>(now.second);
    }
    // No valid time: tick on the uptime second so the uptime line still updates.
    return uptime();
}

// Main loop for widget operation.
// The clock ticks even while PAUSED so the displayed time never freezes; only
// STOPPED / BACKGROUND fully suspend rendering.
void WidgetTime::loop()
{
    if (_state != WidgetState::RUNNING && _state != WidgetState::PAUSED) return;

    uint32_t secKey = currentSecondKey();
    if (_forceRedraw || secKey != _lastSecondKey)
    {
        _lastSecondKey = secKey;
        _forceRedraw = false;
        drawTime();
    }
}

// Renders the current frame into the display buffer.
// displayBuff() diffs against the previous buffer and only transmits changed
// pixels, so a full logical redraw here results in a partial physical update.
void WidgetTime::drawTime()
{
    if (_display == nullptr || _display->display == nullptr)
    {
        logErrorP("Display or display driver is NULL.");
        return;
    }

    _display->display->clearDisplay();
    _display->display->setTextColor(WHITE);
    _display->display->setTextWrap(false);

    if (openknx.time.isValid())
        drawWithTime();
    else
        drawNoTime();

    _display->displayBuff();
}

// Horizontally centered single line of text at vertical position y.
void WidgetTime::drawCenteredText(int16_t y, uint8_t textSize, const char *text)
{
    const int16_t screenWidth = _display->GetDisplayWidth();
    int16_t x1, y1;
    uint16_t w, h;

    _display->display->setTextSize(textSize);
    _display->display->getTextBounds(text, 0, y, &x1, &y1, &w, &h);

    int16_t x = (int16_t)((screenWidth - (int16_t)w) / 2) - x1;
    if (x < 0) x = 0;

    _display->display->setCursor(x, y);
    _display->display->print(text);
}

// Frame variant when a valid time is available:
//   big  HH:MM   (+ smaller :SS appended, baseline aligned)
//   Wochentag, DD.MM.YYYY
//   Uptime  <uptime>
void WidgetTime::drawWithTime()
{
    const int16_t screenWidth = _display->GetDisplayWidth();

    auto now = openknx.time.getLocalTime();

    char hm[6]; // "HH:MM"
    char ss[4]; // ":SS"
    snprintf(hm, sizeof(hm), "%02u:%02u", now.hour, now.minute);
    snprintf(ss, sizeof(ss), ":%02u", now.second);

    // --- Big time line: "HH:MM" at size 3 with ":SS" appended at size 1 ---
    const uint8_t BIG_SIZE = 3; // 5x7 glyph -> 6*3 x 8*3 = 18x24 per char
    const uint8_t SEC_SIZE = 1; // small trailing seconds
    const int16_t BIG_CHAR_W = 6 * BIG_SIZE;
    const int16_t BIG_H = 8 * BIG_SIZE;
    const int16_t SEC_CHAR_W = 6 * SEC_SIZE;

    int16_t hmWidth = (int16_t)strlen(hm) * BIG_CHAR_W;
    int16_t ssWidth = (int16_t)strlen(ss) * SEC_CHAR_W;
    const int16_t GAP = 3; // gap between HH:MM and :SS

    int16_t totalWidth = hmWidth + GAP + ssWidth;
    int16_t startX = (int16_t)((screenWidth - totalWidth) / 2);
    if (startX < 0) startX = 0;

    const int16_t bigY = 6; // top margin for the big time line

    // Big HH:MM
    _display->display->setTextSize(BIG_SIZE);
    _display->display->setCursor(startX, bigY);
    _display->display->print(hm);

    // Smaller :SS, baseline-aligned to the bottom of the big glyphs
    _display->display->setTextSize(SEC_SIZE);
    _display->display->setCursor(startX + hmWidth + GAP, bigY + BIG_H - (8 * SEC_SIZE));
    _display->display->print(ss);

    // --- Date line: "Wochentag, DD.MM.YYYY" ---
    const char *weekday = (now.dayOfWeek <= 6) ? WT_WEEKDAYS_DE[now.dayOfWeek] : "";
    char dateStr[36];
    snprintf(dateStr, sizeof(dateStr), "%s, %02u.%02u.%04u",
             weekday, now.day, now.month, now.year);

    const int16_t dateY = bigY + BIG_H + 6;
    drawCenteredText(dateY, 1, dateStr);

    // --- Uptime line (small, near the bottom) ---
    char uptimeStr[40];
    snprintf(uptimeStr, sizeof(uptimeStr), "Uptime  %s", openknx.logger.buildUptime().c_str());

    const int16_t screenHeight = _display->GetDisplayHeight();
    int16_t upY = screenHeight - 8; // last text row (size 1)
    if (upY < dateY + 10) upY = dateY + 10;
    drawCenteredText(upY, 1, uptimeStr);
}

// Centered "keine Zeit" fallback: no left labels, no "Uhrzeit: —" row.
// Shows a clock glyph, the two-line info text and the uptime.
void WidgetTime::drawNoTime()
{
    const int16_t screenHeight = _display->GetDisplayHeight();

    // Clock glyph (CP437 0x9B "◯"-style is unreliable across fonts; use a small
    // drawn circle with hands instead, centered above the text).
    const int16_t cx = _display->GetDisplayWidth() / 2;
    const int16_t glyphCy = 12;
    const int16_t r = 8;
    _display->display->drawCircle(cx, glyphCy, r, WHITE);
    _display->display->drawLine(cx, glyphCy, cx, glyphCy - (r - 3), WHITE); // minute hand up
    _display->display->drawLine(cx, glyphCy, cx + (r - 4), glyphCy, WHITE); // hour hand right

    // Two centered info lines (mock: "Kein Datum / Uhrzeit\ngesetzt").
    const int16_t line1Y = glyphCy + r + 6;
    drawCenteredText(line1Y, 1, "Kein Datum / Uhrzeit");
    drawCenteredText(line1Y + 11, 1, "gesetzt");

    // Uptime line near the bottom.
    char uptimeStr[40];
    snprintf(uptimeStr, sizeof(uptimeStr), "Uptime  %s", openknx.logger.buildUptime().c_str());
    int16_t upY = screenHeight - 8;
    if (upY < line1Y + 22) upY = line1Y + 22;
    drawCenteredText(upY, 1, uptimeStr);
}
#endif // DEVICE_DISPLAY_MODULE
