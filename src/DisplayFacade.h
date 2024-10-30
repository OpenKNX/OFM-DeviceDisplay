#pragma once



#include "DeviceDisplay.h"

class DisplayFacade {
public:
    // Definiere eine eigene `WidgetAction`-enum class
    enum class WidgetAction {
        Status,         // Is a StatusFlag
        InternalEnable, // Is an InternalEnabled
        AutoRemove,     // Is an AutoRemoveFlag
        ExternalControl // Is an ExternalDisable
    };

    DisplayFacade(DeviceDisplay& display) : deviceDisplay(display) {}

    // Methode zum Hinzufügen eines Widgets mit Fassade-Flags
    void addWidget(Widget* widget, uint32_t duration, const std::string& name, std::initializer_list<WidgetAction> actionFlags) {
        // Interne Konvertierung der Fassade-Flags zu `DeviceDisplay::WidgetAction`
        DeviceDisplay::WidgetAction deviceFlags = convertFacadeFlagsToDeviceFlags(actionFlags);
        deviceDisplay.addWidget(widget, duration, name, deviceFlags);
    }

    // Methoden für externe Steuerung
    void enableWidget(uint8_t flag) {
        deviceDisplay.setWidgetFlag(flag, DeviceDisplay::WidgetAction::InternalEnabled);
    }


    void disableWidget(uint8_t flag) {
        deviceDisplay.clearWidgetFlag(flag, DeviceDisplay::WidgetAction::InternalEnabled);
    }

    void activateExternalWidget( uint8_t flag) {
        deviceDisplay.clearWidgetFlag(flag, DeviceDisplay::WidgetAction::ExternalManaged);
        deviceDisplay.setWidgetFlag(flag, DeviceDisplay::WidgetAction::InternalEnabled);
    }

    void updateDisplay() {
        deviceDisplay.LoopWidgets();
    }

private:
    DeviceDisplay& deviceDisplay;

    // Funktion zur Konvertierung von Fassade-Flags zu `DeviceDisplay`-Flags
    DeviceDisplay::WidgetAction convertFacadeFlagsToDeviceFlags(std::initializer_list<WidgetAction> actionFlags) {
        DeviceDisplay::WidgetAction deviceFlags = DeviceDisplay::WidgetAction::NoAction;
        for (auto flag : actionFlags) {
            switch (flag) {
                //case WidgetAction::Status:
                //    deviceFlags = deviceFlags | DeviceDisplay::WidgetAction::StatusFlag;
                //    break;
                //case WidgetAction::InternalEnable:
                //    deviceFlags = deviceFlags | DeviceDisplay::WidgetAction::InternalEnabled;
                //    break;
                //case WidgetAction::AutoRemove:
                //    deviceFlags = deviceFlags | DeviceDisplay::WidgetAction::AutoRemoveFlag;
                //    break;
                //case WidgetAction::ExternalControl:
                //    deviceFlags = deviceFlags | DeviceDisplay::WidgetAction::ExternalManaged;
                //    break;
            }
        }
        return deviceFlags;
    }
};