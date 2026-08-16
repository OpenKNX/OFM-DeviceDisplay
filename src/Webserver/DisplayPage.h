#pragma once
#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        DisplayPage.h
 * @brief       Web page showing the live OLED framebuffer and editing the display settings
 * @version     0.0.1
 * @date        2026-08-15
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Optional companion to OFM-Network: when a webserver is part of the firmware, this registers
 * "/display" with a canvas view of the framebuffer plus the same settings the "ddc config" console
 * command edits. Without OFM-Network the whole unit compiles away.
 *
 * Settings arrive on the HTTP task (ESP32 runs esp_http_server in its own task), where touching
 * the I2C display or the settings flash would race the drawing loop. POSTs therefore only queue
 * the change; loop() applies and persists it in the KNX loop context.
 **/

    // Two-fold guard (like WidgetSysInfo): __has_include AND a network build -- NetworkModule.h wraps its
    // body in KNX_IP_*, so __has_include alone resolves to an empty header (openknxNetwork undefined).
    #if defined(OPENKNX_WEBSERVER) && (defined(KNX_IP_LAN) || defined(KNX_IP_WIFI))
        #if defined(__has_include)
            #if __has_include("OpenKNX/Network/Module.h")
                #define DDC_HAS_WEBPAGE 1
            #endif
        #else
            #define DDC_HAS_WEBPAGE 1
        #endif
    #endif

    #ifdef DDC_HAS_WEBPAGE

namespace OpenKNX
{
    class DisplayPage
    {
      public:
        /** @brief Register routes, assets and the menu entry. Call once from the module setup().
         *
         * Safe to call long before the webserver starts: the route/menu/asset lists are plain
         * vectors read at request time and are never cleared, so registration order does not
         * matter. Guarded against a second call anyway. */
        static void setup();

        /** @brief Apply the queued requests. Call from the display loop(). */
        static void loop();
    };
} // namespace OpenKNX

    #endif // DDC_HAS_WEBPAGE
#endif     // DEVICE_DISPLAY_MODULE
