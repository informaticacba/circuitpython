/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"

#define DELAY 0x80


// display init sequence according to Adafruit_CircuitPython_ST7735R
// https://github.com/adafruit/Adafruit_CircuitPython_ST7735R
uint8_t display_init_sequence[] = {
    // sw reset
    0x01, 0 | DELAY, 0x96,
    // SLPOUT and Delay
    0x11, 0 | DELAY, 0xFF,
    0xB1, 0x03, 0x01, 0x2C, 0x2D,      // _FRMCTR1
    0xB3, 0x06, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,     // _FRMCTR3
    0xB4, 0x01, 0x07, // _INVCTR line inversion
    0xC0, 0x03, 0xA2, 0x02, 0x84, // _PWCTR1 GVDD = 4.7V, 1.0uA
    0xC1, 0x01, 0xC5, // _PWCTR2 VGH=14.7V, VGL=-7.35V
    0xC2, 0x02, 0x0A, 0x00, // _PWCTR3 Opamp current small, Boost frequency
    0xC3, 0x02, 0x8A, 0x2A,
    0xC4, 0x02, 0x8A, 0xEE,
    0xC5, 0x01, 0x0E, // _VMCTR1 VCOMH = 4V, VOML = -1.1V
    0x20, 0x00, // _INVOFF
    0x36, 0x01, 0x18, // _MADCTL bottom to top refresh
    // 1 clk cycle nonoverlap, 2 cycle gate rise, 3 cycle osc equalie,
    // fix on VTL
    0x3A, 0x01, 0x05, // COLMOD - 16bit color
    0xE0, 0x10, 0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10, // _GMCTRP1 Gamma
    0xE1, 0x10, 0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10, // _GMCTRN1
    0x13, 0 | DELAY, 0x0A, // _NORON
    0x29, 0 | DELAY, 0x64, // _DISPON
    // 0x36, 0x01, 0xC0,  // _MADCTL Default rotation plus BGR encoding
    0x36, 0x01, 0xC8,      // _MADCTL Default rotation plus RGB encoding
    0x21, 0x00,      // _INVON
};

static void display_init(void) {
    busio_spi_obj_t *spi = &displays[0].fourwire_bus.inline_bus;
    common_hal_busio_spi_construct(
        spi,
        &pin_GPIO10,    // CLK
        &pin_GPIO11,    // MOSI
        NULL,           // MISO not connected
        false           // Not half-duplex
        );

    common_hal_busio_spi_never_reset(spi);

    displayio_fourwire_obj_t *bus = &displays[0].fourwire_bus;
    bus->base.type = &displayio_fourwire_type;

    common_hal_displayio_fourwire_construct(
        bus,
        spi,
        &pin_GPIO8,     // DC
        &pin_GPIO9,     // CS
        &pin_GPIO12,    // RST
        40000000,       // baudrate
        0,              // polarity
        0               // phase
        );

    displayio_display_obj_t *display = &displays[0].display;
    display->base.type = &displayio_display_type;
    common_hal_displayio_display_construct(
        display,
        bus,
        160,            // width (after rotation)
        80,             // height (after rotation)
        26,             // column start
        1,              // row start
        90,             // rotation
        16,             // color depth
        false,          // grayscale
        false,          // pixels in a byte share a row. Only valid for depths < 8
        1,              // bytes per cell. Only valid for depths < 8
        false,          // reverse_pixels_in_byte. Only valid for depths < 8
        true,           // reverse_pixels_in_word
        MIPI_COMMAND_SET_COLUMN_ADDRESS,    // set column command
        MIPI_COMMAND_SET_PAGE_ADDRESS,      // set row command
        MIPI_COMMAND_WRITE_MEMORY_START,    // write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        &pin_GPIO25,    // backlight pin
        NO_BRIGHTNESS_COMMAND,
        1.0f,           // brightness
        false,          // single_byte_bounds
        false,          // data_as_commands
        true,           // auto_refresh
        60,             // native_frames_per_second
        true,           // backlight_on_high
        false,          // SH1107_addressing
        50000           // backlight pwm frequency
        );
}

void board_init(void) {
    // Display
    display_init();
}
// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
