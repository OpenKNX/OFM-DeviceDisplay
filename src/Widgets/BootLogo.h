#pragma once
#include "../Widget.h"

class WidgetBootLogo : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetBootLogo"; }
    WidgetBootLogo(uint32_t displayTime, WidgetsAction action);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override; // Rückgabe der Anzeigedauer in ms
    WidgetsAction getAction() const override; // Rückgabe der Aktion des Widgets

    void setDisplayModule(i2cDisplay *displayModule) override; // Display-Modul setzen
    i2cDisplay *getDisplayModule() const override;             // Display-Modul abrufen

  private:
    void drawBootLogo(); // Boot-Logo zeichnen

    WidgetState _state;             // Aktueller Zustand des Widgets
    uint32_t _displayTime;          // Anzeigedauer des Widgets in ms
    WidgetsAction _action;          // Aktion des Widgets
    i2cDisplay *_display;           // Zeiger auf das Display-Modul
    bool _needsRedraw;              // Gibt an, ob das Widget neu gezeichnet werden muss
    std::string _name = "BootLogo"; // Name des Widgets

    /** state of partial drawing:
     * 0=no drawing / done,
     * 1=clear,
     * 2=partial draw image,
     * 3=start sending to display,
     */
    uint8_t _drawStep = 0; // TODO check using enum

    /** Number of rows to draw in one loop() call */
    const uint8_t _stepHeight = 8; // TODO find "best" size (note: 8 results in ~1.6ms max loop time)

    /** Row to start drawing in one loop() call */
    uint8_t _yStart = 0;

};