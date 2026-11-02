/*
 * Tiny text renderer for the HarmonyOS-on-sweet boot verbose overlay.
 * Draws lines of text onto a 32bpp buffer (any RGBX/BGRX order: text is
 * white, the background behind the text is darkened).  GPL-2.0 (font).
 */
#ifndef HMOS_TEXT_H
#define HMOS_TEXT_H
#include <stdint.h>
#include <string.h>
#include "font_8x16.h"

#define HMOS_TEXT_SCALE 2   /* 16x32 px glyphs: readable on a 1080 px wide panel */
#define HMOS_GLYPH_W (8 * HMOS_TEXT_SCALE)
#define HMOS_GLYPH_H (16 * HMOS_TEXT_SCALE)

static inline void hmos_draw_char_s(uint8_t *base, int stride, int w, int h, int x, int y, unsigned char c,
                                    uint32_t color, int scale)
{
    const unsigned char *g = &hmos_font_8x16[c * 16];
    for (int row = 0; row < 16; row++) {
        unsigned char bits = g[row];
        for (int sy = 0; sy < scale; sy++) {
            int py = y + row * scale + sy;
            if (py < 0 || py >= h)
                continue;
            uint32_t *line = (uint32_t *)(base + (size_t)py * stride);
            for (int col = 0; col < 8; col++) {
                if (!(bits & (0x80 >> col)))
                    continue;
                for (int sx = 0; sx < scale; sx++) {
                    int px = x + col * scale + sx;
                    if (px >= 0 && px < w)
                        line[px] = color;
                }
            }
        }
    }
}

static inline void hmos_draw_char(uint8_t *base, int stride, int w, int h, int x, int y, unsigned char c,
                                  uint32_t color)
{
    hmos_draw_char_s(base, stride, w, h, x, y, c, color, HMOS_TEXT_SCALE);
}

/* darken a rectangle (halve every channel, keep alpha opaque) */
static inline void hmos_dim_rect(uint8_t *base, int stride, int w, int h, int x0, int y0, int x1, int y1)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (int y = y0; y < y1; y++) {
        uint32_t *line = (uint32_t *)(base + (size_t)y * stride);
        for (int x = x0; x < x1; x++)
            line[x] = ((line[x] >> 1) & 0x7f7f7f7f) | 0xff000000; /* 50%: what is behind stays visible */
    }
}

/*
 * Draw the tail of 'text' (lines separated by '\n') in the lower part of the
 * buffer, wrapping long lines.  'top' is the first usable pixel row.
 */
static inline void hmos_draw_log_s(uint8_t *base, int stride, int w, int h, const char *text, int top,
                                   int scale, int dim)
{
    const int margin = 8;
    const int gw = 8 * scale, gh = 16 * scale;
    int cols = (w - 2 * margin) / gw;
    int rows = (h - top - margin) / gh;
    if (cols < 10 || rows < 1 || text == NULL)
        return;
    /* split into wrapped display rows, keep only the last 'rows' */
    enum { MAXR = 256 };
    const char *rs[MAXR];
    int rl[MAXR];
    int n = 0;
    const char *p = text;
    while (*p) {
        const char *e = strchr(p, '\n');
        int len = e ? (int)(e - p) : (int)strlen(p);
        int off = 0;
        do {
            int chunk = len - off > cols ? cols : len - off;
            if (n == MAXR) {
                memmove(rs, rs + 1, sizeof(rs[0]) * (MAXR - 1));
                memmove(rl, rl + 1, sizeof(rl[0]) * (MAXR - 1));
                n--;
            }
            rs[n] = p + off;
            rl[n] = chunk;
            n++;
            off += chunk;
        } while (off < len);
        if (!e)
            break;
        p = e + 1;
    }
    int first = n > rows ? n - rows : 0;
    int y = top;
    if (dim)
        hmos_dim_rect(base, stride, w, h, 0, top - 4, w, top + (n - first) * gh + 4);
    for (int i = first; i < n; i++, y += gh) {
        for (int c = 0; c < rl[i]; c++) {
            unsigned char ch = (unsigned char)rs[i][c];
            uint32_t color = 0xffffffff;
            if (dim) /* drop shadow keeps the text readable over bright animation frames */
                hmos_draw_char_s(base, stride, w, h, margin + c * gw + 1, y + 1, ch < 32 ? '?' : ch, 0xff000000, scale);
            hmos_draw_char_s(base, stride, w, h, margin + c * gw, y, ch < 32 ? '?' : ch, color, scale);
        }
    }
}

static inline void hmos_draw_log(uint8_t *base, int stride, int w, int h, const char *text, int top)
{
    hmos_draw_log_s(base, stride, w, h, text, top, HMOS_TEXT_SCALE, 1);
}

/*
 * Full-screen console: a fixed title line, then the log at console size
 * (8x16 glyphs).  The log fills from the top and, once the screen is full,
 * keeps only the newest lines, so it scrolls up like a normal boot console.
 */
#define HMOS_TITLE "HarmonyOS 5.1 on Redmi Note 10 Pro (sweet) - boot verbose"
static inline void hmos_draw_screen(uint8_t *base, int stride, int w, int h, const char *text, int dim)
{
    int top = 96; /* stay below the camera cut-out */
    hmos_draw_log_s(base, stride, w, h, HMOS_TITLE, top, 2, dim);
    top += 32 + 16;
    for (int x = 8; x < w - 8; x++)
        ((uint32_t *)(base + (size_t)(top - 8) * stride))[x] = 0xff808080;
    hmos_draw_log_s(base, stride, w, h, text, top, 1, dim);
}
#endif
