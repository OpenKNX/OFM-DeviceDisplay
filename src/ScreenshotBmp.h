#pragma once
#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        ScreenshotBmp.h
 * @brief       Sink-agnostic 1-bit BMP encoder for the OLED framebuffer (SD file now, HTTP later)
 * @version     0.0.1
 * @date        2026-07-12
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 *
 * Encodes an SSD1306 framebuffer (8 pixels packed vertically per byte) into a top-down 1-bit BMP,
 * streamed to any Print& sink: an SD FSFILE today, an HTTP image/bmp response for the web display
 * later - same encoder, no changes. Header and rows are split so the SD path can chunk rows across
 * loop() ticks; write() is the one-shot convenience for a streaming sink.
 **/
#include <Arduino.h> // Print
#include <cstdint>

namespace ScreenshotBmp
{
    constexpr uint32_t HEADER_BYTES = 62; // 14 file header + 40 info header + 8 palette (2 colors)

    // Total 1-bit BMP file size for a w x h image (rows 4-byte aligned).
    uint32_t fileSize(uint16_t w, uint16_t h);

    // 62-byte header. invert=false -> lit pixel (bit 1) = white (OLED look); true swaps the palette
    // (paper look). biHeight is negative (top-down) so rows stream top->bottom with no seek.
    void writeHeader(Print& out, uint16_t w, uint16_t h, bool invert);

    // Emit yCount rows from yStart (top-down). Transposes SSD1306 vertical packing (fb[page*w+col],
    // LSB=top) into BMP horizontal bytes (MSB=leftmost). Only w <= 128 is supported (row buffer).
    void writeRows(Print& out, const uint8_t* fb, uint16_t w, uint16_t h, uint16_t yStart, uint16_t yCount);

    // One-shot (header + all rows) for a streaming sink such as the web image/bmp response.
    void write(Print& out, const uint8_t* fb, uint16_t w, uint16_t h, bool invert);
} // namespace ScreenshotBmp
#endif // DEVICE_DISPLAY_MODULE
