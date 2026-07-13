// ============================================================
// PCB Cupid Wireless Studio Mic — Glyph C6
// ============================================================
// Records 24-bit stereo WAV from the G-Sense 2CH MEMS Mic
// to a G-Mod Micro SD Card. Creates an open WiFi hotspot
// with a web interface for controlling recordings and
// downloading files to any device.
// ============================================================

#pragma once

// ---- WiFi Access Point ----
#define AP_SSID       "Glyph Studio Mic"
#define AP_PASS       ""            // open network, no password

// ---- SD Card SPI Pins (G-Mod Micro SD Card → Glyph C6) ----
#define SD_SCK        21
#define SD_MOSI       22            // labelled MO on the module
#define SD_MISO       23            // labelled MI on the module
#define SD_CS         15

// ---- 2CH Mic I2S Pins (G-Sense → Glyph C6) ----
#define I2S_WS        20            // Word Select / LRCK
#define I2S_SD        19            // Serial Data
#define I2S_SCK       18            // Bit Clock / SCLK

// ---- Audio Settings ----
#define SAMPLE_RATE       44100
#define NUM_CHANNELS      2
#define BITS_PER_SAMPLE   16        // CD-quality, practical file sizes

// ---- Recording ----
#define CHUNK_SIZE        4096      // bytes to buffer before SD write
#define RECORD_LED        14        // onboard LED on C6 (blinks when recording)
#define RECORD_BTN        9         // user button — short press toggles record/stop

// ---- Web Server ----
#define WEB_PORT          80
