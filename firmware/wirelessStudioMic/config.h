// ============================================================
// PCB Cupid Wireless Studio Mic — Glyph C6
// ============================================================
// Records stereo WAV from the G-Sense 2CH MEMS Mic
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
// The ICS-43434 mic outputs 24-bit audio in 32-bit I2S slots.
// We capture at 32-bit then convert to 16-bit for the WAV file.
// This gives CD-quality audio at half the file size.
#define SAMPLE_RATE          44100   // 44.1 kHz — CD quality. Drop to 22050 for smaller files.
#define NUM_CHANNELS         2       // stereo
#define I2S_BITS_PER_SAMPLE  32      // matches the mic's I2S output format (do not change)
#define WAV_BITS_PER_SAMPLE  16      // CD-quality WAV, half the size of 24/32-bit

// ---- Recording ----
#define CHUNK_SIZE        2048       // smaller chunks = more frequent WDT feeding
#define RECORD_LED        14         // onboard LED on C6 (blinks when recording)
#define RECORD_BTN        9          // onboard BOOT key (user button after boot)

// ---- Web Server ----
#define WEB_PORT          80
