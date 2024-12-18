#pragma once
#include "../Widget.h"

class WidgetBootLogo : public Widget {
public:

    const std::string logPrefix() { return "WidgetBootLogo"; }
    WidgetBootLogo(uint32_t displayTime, WidgetsAction action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;

    uint32_t getDisplayTime() const override; // Rückgabe der Anzeigedauer in ms
    WidgetsAction getAction() const override; // Rückgabe der Aktion des Widgets

    void setDisplayModule(i2cDisplay *displayModule) override; // Display-Modul setzen
    i2cDisplay *getDisplayModule() const override;             // Display-Modul abrufen

private:
    void drawBootLogo(); // Boot-Logo zeichnen

    uint32_t _displayTime;    // Anzeigedauer des Widgets in ms
    WidgetsAction _action;    // Aktion des Widgets
    i2cDisplay *_display;     // Zeiger auf das Display-Modul
    bool _needsRedraw;        // Gibt an, ob das Widget neu gezeichnet werden muss
};