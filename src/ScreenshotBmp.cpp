#include "ScreenshotBmp.h"
#ifdef DEVICE_DISPLAY_MODULE

namespace
{
    inline void put16(uint8_t*& p, uint16_t v)
    {
        *p++ = static_cast<uint8_t>(v & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 8) & 0xFF);
    }
    inline void put32(uint8_t*& p, uint32_t v)
    {
        *p++ = static_cast<uint8_t>(v & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 8) & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 16) & 0xFF);
        *p++ = static_cast<uint8_t>((v >> 24) & 0xFF);
    }
    constexpr uint16_t MAX_ROW_BYTES = 128 / 8; // supported width upper bound (row buffer sizing)
} // namespace

namespace ScreenshotBmp
{
    uint32_t fileSize(uint16_t w, uint16_t h)
    {
        const uint32_t stride = ((static_cast<uint32_t>(w) + 31) / 32) * 4; // 4-byte aligned
        return HEADER_BYTES + stride * h;
    }

    void writeHeader(Print& out, uint16_t w, uint16_t h, bool invert)
    {
        const uint32_t stride = ((static_cast<uint32_t>(w) + 31) / 32) * 4;
        const uint32_t imageBytes = stride * h;

        uint8_t hdr[HEADER_BYTES];
        uint8_t* p = hdr;
        *p++ = 'B';
        *p++ = 'M';
        put32(p, HEADER_BYTES + imageBytes);            // bfSize
        put32(p, 0);                                    // bfReserved
        put32(p, HEADER_BYTES);                         // bfOffBits (pixel data start)
        put32(p, 40);                                   // biSize (INFOHEADER)
        put32(p, w);                                    // biWidth
        put32(p, static_cast<uint32_t>(-static_cast<int32_t>(h))); // biHeight < 0 -> top-down
        put16(p, 1);                                    // biPlanes
        put16(p, 1);                                    // biBitCount (1bpp)
        put32(p, 0);                                    // biCompression (BI_RGB)
        put32(p, imageBytes);                           // biSizeImage
        put32(p, 2835);                                 // biXPelsPerMeter (~72 dpi)
        put32(p, 2835);                                 // biYPelsPerMeter
        put32(p, 2);                                    // biClrUsed
        put32(p, 2);                                    // biClrImportant

        // 2-colour palette (BGRA): index 0 then index 1 (the lit bit). invert swaps them.
        const uint8_t black[4] = {0x00, 0x00, 0x00, 0x00};
        const uint8_t white[4] = {0xFF, 0xFF, 0xFF, 0x00};
        const uint8_t* c0 = invert ? white : black;
        const uint8_t* c1 = invert ? black : white;
        for (uint8_t i = 0; i < 4; ++i) *p++ = c0[i];
        for (uint8_t i = 0; i < 4; ++i) *p++ = c1[i];

        out.write(hdr, HEADER_BYTES);
    }

    void writeRows(Print& out, const uint8_t* fb, uint16_t w, uint16_t h, uint16_t yStart, uint16_t yCount)
    {
        if (fb == nullptr || w == 0 || w > 128) return; // row buffer bounds (memory-safety)

        const uint16_t rowBytes = static_cast<uint16_t>((w + 7) / 8);
        const uint16_t stride = static_cast<uint16_t>(((rowBytes + 3) / 4) * 4);
        const uint16_t pad = static_cast<uint16_t>(stride - rowBytes);
        uint8_t row[MAX_ROW_BYTES + 3]; // packed bytes + up to 3 alignment-pad bytes

        uint32_t yEnd = static_cast<uint32_t>(yStart) + yCount;
        if (yEnd > h) yEnd = h;

        for (uint16_t y = yStart; y < yEnd; ++y)
        {
            const uint16_t page = static_cast<uint16_t>(y >> 3);
            const uint8_t bit = static_cast<uint8_t>(y & 7);
            uint16_t idx = 0;
            for (uint16_t bx = 0; bx < rowBytes; ++bx)
            {
                uint8_t acc = 0;
                for (uint8_t k = 0; k < 8; ++k)
                {
                    const uint16_t col = static_cast<uint16_t>(bx * 8 + k);
                    if (col < w && ((fb[static_cast<uint32_t>(page) * w + col] >> bit) & 1))
                        acc |= static_cast<uint8_t>(1 << (7 - k));
                }
                row[idx++] = acc;
            }
            for (uint16_t i = 0; i < pad; ++i) row[idx++] = 0;
            out.write(row, idx);
        }
    }

    void write(Print& out, const uint8_t* fb, uint16_t w, uint16_t h, bool invert)
    {
        writeHeader(out, w, h, invert);
        writeRows(out, fb, w, h, 0, h);
    }
} // namespace ScreenshotBmp

#endif // DEVICE_DISPLAY_MODULE
