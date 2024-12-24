#include "Fireworks.h"
#include "OpenKNX.h"
#include <cstdlib>

WidgetFireworks::WidgetFireworks(uint32_t displayTime, WidgetFlags action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(constrain(intensity, 1, 10)),
      _display(nullptr), _lastUpdateTime(0), _state(WidgetState::STOPPED), _fireworkCount(_intensity)
{
    _updateInterval = map(_intensity, 1, 10, 50, 20);
    _fireworks = new Firework[_fireworkCount];

    for (uint8_t i = 0; i < _fireworkCount; i++)
    {
        _fireworks[i].x = 0;
        _fireworks[i].y = 0;
        _fireworks[i].properties = 0;
        _fireworks[i].activeParticles = 0;
        for (uint8_t j = 0; j < Firework::MAX_PARTICLES; j++)
        {
            _fireworks[i].particles[j].active = false;
        }
    }
}

WidgetFireworks::~WidgetFireworks()
{
    delete[] _fireworks;
}

void WidgetFireworks::setup()
{
    if (_display == nullptr)
    {
        logErrorP("Display module is NULL. Setup failed.");
        return;
    }
    logInfoP("Setup...");
}

void WidgetFireworks::start()
{
    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetFireworks::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;

    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetFireworks::pause()
{
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
}

void WidgetFireworks::resume()
{
    logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetFireworks::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval) return;

    _lastUpdateTime = currentTime;

    launchFirework();
    updateFireworks();
    drawFireworks();
}

void WidgetFireworks::launchFirework()
{
    for (uint8_t i = 0; i < _fireworkCount; i++)
    {
        if (_fireworks[i].activeParticles == 0)
        {
            uint8_t margin = 10;
            uint8_t startX = random(margin, _display->GetDisplayWidth() - margin);
            uint8_t startY = random(margin, _display->GetDisplayHeight() - margin);

            _fireworks[i].x = startX;
            _fireworks[i].y = startY;
            setActive(_fireworks[i], true);

            for (uint8_t j = 0; j < Firework::MAX_PARTICLES; j++)
            {
                Particle& p = _fireworks[i].particles[j];
                float angle = (360.0f / Firework::MAX_PARTICLES * j) + random(-15, 15);
                float radians = angle * PI / 180.0f;
                uint8_t speed = random(30, 50);
                p.x = startX << SUBPIXEL_BITS;
                p.y = startY << SUBPIXEL_BITS;
                p.vx = (int8_t)(cos(radians) * speed);
                p.vy = (int8_t)(sin(radians) * speed);
                p.life = random(25, 35);
                p.active = true;
            }

            _fireworks[i].activeParticles = Firework::MAX_PARTICLES;
            break;
        }
    }
}

void WidgetFireworks::updateFireworks()
{
    for (uint8_t i = 0; i < _fireworkCount; i++)
    {
        if (_fireworks[i].activeParticles > 0)
        {
            uint8_t activeCount = 0;

            for (uint8_t j = 0; j < Firework::MAX_PARTICLES; j++)
            {
                Particle& p = _fireworks[i].particles[j];

                if (p.active && p.life > 0)
                {
                    p.x += p.vx;
                    p.y += p.vy;
                    p.vy += GRAVITY;
                    if (p.vx > 0) p.vx += DRAG;
                    if (p.vx < 0) p.vx -= DRAG;
                    if (p.vy > 0) p.vy += DRAG;
                    if (p.vy < 0) p.vy -= DRAG;
                    p.life--;
                    if (p.life > 0)
                    {
                        activeCount++;
                    }
                    else
                    {
                        p.active = false;
                    }
                }
            }

            _fireworks[i].activeParticles = activeCount;
            if (activeCount == 0)
            {
                setActive(_fireworks[i], false);
            }
        }
    }
}

void WidgetFireworks::drawFireworks()
{
    if (!_display) return;

    bool hasActive = false;
    for (uint8_t i = 0; i < _fireworkCount; i++)
    {
        if (_fireworks[i].activeParticles > 0)
        {
            hasActive = true;
            break;
        }
    }

    if (!hasActive) return;

    _display->display->clearDisplay();
    for (uint8_t i = 0; i < _fireworkCount; i++)
    {
        if (_fireworks[i].activeParticles > 0)
        {
            for (uint8_t j = 0; j < Firework::MAX_PARTICLES; j++)
            {
                const Particle& p = _fireworks[i].particles[j];

                if (p.active && p.life > 0)
                {
                    int16_t drawX = p.x >> SUBPIXEL_BITS;
                    int16_t drawY = p.y >> SUBPIXEL_BITS;
                    if (drawX >= 0 && drawX < _display->GetDisplayWidth() &&
                        drawY >= 0 && drawY < _display->GetDisplayHeight())
                    {
                        _display->display->drawPixel(drawX, drawY, WHITE);
                        int16_t tailX = drawX - (p.vx >> 5);
                        int16_t tailY = drawY - (p.vy >> 5);

                        if (tailX >= 0 && tailX < _display->GetDisplayWidth() &&
                            tailY >= 0 && tailY < _display->GetDisplayHeight())
                        {
                            _display->display->drawPixel(tailX, tailY, WHITE);
                        }
                        if (p.life > 20 && random(2) == 0)
                        {
                            int16_t sparkX = drawX + random(-1, 2);
                            int16_t sparkY = drawY + random(-1, 2);

                            if (sparkX >= 0 && sparkX < _display->GetDisplayWidth() &&
                                sparkY >= 0 && sparkY < _display->GetDisplayHeight())
                            {
                                _display->display->drawPixel(sparkX, sparkY, WHITE);
                            }
                        }
                    }
                }
            }
        }
    }
    _display->displayBuff();
}