#pragma once
#include "../Widget.h"

class WidgetOpenKNXLogo : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetOpenKNXLogo"; }
    WidgetOpenKNXLogo(uint32_t displayTime, WidgetsAction action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override; // Rückgabe der Anzeigedauer in ms
    WidgetsAction getAction() const override; // Rückgabe der Aktion des Widgets

    void setDisplayModule(i2cDisplay *displayModule) override; // Display-Modul setzen
    i2cDisplay *getDisplayModule() const override;             // Display-Modul abrufen

  private:
    void drawOpenKNXLogo();

    WidgetState _state;                  // Aktueller Zustand des Widgets
    uint32_t _displayTime;               // Anzeigedauer des Widgets in ms
    WidgetsAction _action;               // Aktion des Widgets
    i2cDisplay *_display;                // Zeiger auf das Display-Modul
    bool _needsRedraw;                   // Gibt an, ob das Widget neu gezeichnet werden muss
    std::string _name = "DefaultWidget"; // Name des Widgets

    /** state of partial drawing:
     * 0    = no drawing / done,
     * 1..n = drawing step
     */
    uint8_t _drawStep = 0;

    /** the last rendered uptime */
    String _lastUptime = "";
};