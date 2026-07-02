# The ₹2,000 Studio Mic

## Where This Came From

A few weeks ago, we were filming some content for PCB Cupid's YouTube channel. We had this tiny wireless microphone clipped onto the presenter's collar — one of those expensive ones that everyone uses these days. You know the kind. Costs somewhere north of ₹15,000. Sounds great. Pairs instantly. The whole crew loves it.

And then someone said the obvious thing out loud:

*"Wait. We literally sell a stereo microphone module for ₹499. We have our own ESP32 boards with battery management. We have SD card modules. We have audio amplifiers. Why are we using someone else's mic?"*

Silence.

We all looked at each other. Then at the pile of Glyph boards on the workbench. Then at the 2-channel MEMS mic module sitting in its anti-static bag, waiting for someone to give it a real project.

That's how this started. Not with a datasheet. With a question that wouldn't go away.

## The Bet

Here's the thing about building hardware in India: you're always fighting the perception that the expensive imported thing is better. And sometimes it is. That wireless mic we were using? It's genuinely good engineering. No question.

But we started doing the math, and the numbers were kind of ridiculous:

| What we were using | What we could build |
|---|---|
| ₹15,000+ wireless mic | ₹499 stereo MEMS mic module |
| Proprietary closed system | Open source, hackable, yours |
| Can't repair, can't modify | Every line of code is yours |
| One trick pony (just a mic) | ESP32 with Wi-Fi, BLE, DSP, SD card, display... |
| Dead battery? Buy a new one | Dead battery? Swap in any LiPo |

Now, I'm not going to pretend a ₹499 MEMS microphone is going to beat a studio condenser. It won't. The physics of a microscopic silicon diaphragm are what they are. The noise floor is higher. The bass rolls off at 60 Hz. If you're recording a platinum album, buy the Neumann.

But here's what we *can* do: build something that costs less than the tax on that wireless mic, records clean 24-bit stereo audio to an SD card, shows you a VU meter on an OLED display, lets you monitor through headphones in real time, runs on a battery for hours, and fits in your pocket. And when it breaks or you want to change something, you open the code and fix it yourself.

That's a different kind of value. That's the PCB Cupid kind.

## What We're Actually Building

A self-contained stereo recording microphone. Portable. Battery powered. All-digital signal path. Made entirely from PCB Cupid modules.

The heart of it is our **2-channel MEMS microphone module** — two matched ICS-43434 (or IM69D130) digital mics on one board, spitting out 24-bit stereo I2S audio. No analog preamp. No ADC noise. No ground loops. Just ones and zeros straight into the ESP32.

The brain is a **Glyph ESP32-S3** — dual-core 240 MHz, 8 MB of flash, and crucially, a built-in LiPo battery charger. It's got two I2S peripherals, which means we can record from the mic AND send audio to the amplifier at the same time. Real-time monitoring with zero latency.

Audio gets written as proper WAV files to a **microSD card** over SPI. A tiny **OLED display** shows you what's happening — recording time, VU meter bars dancing with your voice, battery level. A few **buttons** for record, stop, playback. A **NAU8325 amplifier** drives headphones or a small speaker so you can hear yourself while recording.

All of this goes into a **3D-printed enclosure**. The whole thing, end to end, costs around **₹2,000-2,500** in parts. Every single component is something we already sell on shop.pcbcupid.com.

## The Kit Vision

We want to sell this as a kit. Not a finished product — a kit. Here's why:

1. **Educational value is the whole point.** Someone builds this, they learn I2S audio, SPI storage, I2C displays, RTOS task design, WAV file formats, and real-time DSP. Those are real embedded engineering skills.

2. **It showcases our entire ecosystem.** Glyph board, G-Sense mic, G-Mod SD card, GLINK I2C accessories — it's a walking catalog of PCB Cupid products working together.

3. **The "I built this" feeling.** There's something different about recording a podcast on gear you assembled and programmed yourself versus something you unboxed. Content creators love that story. Their audience loves that story.

4. **It's hackable by design.** Want to add Wi-Fi streaming? The ESP32-S3 has Wi-Fi. Want to add a web interface for settings? Go ahead. Want to add DSP effects? The dual-core processor can handle it. The kit is a starting point, not a finished product.

## The Honest Audio Quality Assessment

I'm going to be straightforward about what this can and can't do, because we don't want to oversell and underdeliver.

### What it does well:

- **Close-mic podcast vocals.** If you're 6-12 inches from the mic, the signal-to-noise ratio is totally usable. Add a noise gate in post (or in real-time on the ESP32) and the noise floor disappears between words.
- **Stereo interviews.** Two mics, two channels, one module. Put it between two people and you get natural stereo separation. That's something even expensive lav mics can't do.
- **Acoustic instruments.** Guitar, ukulele, violin — the high end is clean and detailed.
- **Field recording (moderate to loud environments).** City streets, markets, nature with birds — fine. Quiet forests at night — the self-noise becomes audible.

### What it doesn't do:

- **Quiet ASMR or whisper recording.** The self-noise floor of ~29 dBA means you'll hear hiss in very quiet recordings.
- **Bass-heavy music production.** The MEMS element rolls off at 60 Hz. Kick drums and bass guitars will sound thin.
- **Replacing a large-diaphragm condenser.** Physics is physics. A microscopic silicon diaphragm will never move air the way a 1-inch gold-sputtered membrane does.

### The DSP Advantage

Here's the clever part: because the audio is digital from the moment it leaves the mic capsule, and because the ESP32-S3 has real DSP horsepower, we can do things in software that analog mics need racks of outboard gear for:

- **Noise gating** — silence the noise floor when nobody's talking
- **EQ** — boost warmth at 150 Hz, add air at 10 kHz
- **Compression** — even out level differences
- **Auto gain control** — adjust input gain on the fly

These aren't gimmicks. Applied tastefully, they can close a significant chunk of the quality gap between a MEMS mic and a budget condenser. For podcast and content creation, they can make the difference between "sounds like a DIY project" and "sounds like a real mic."

## Rough Architecture

```
                    ┌──────────────────────────────────┐
                    │          Glyph ESP32-S3           │
                    │  Dual-core 240MHz, 8MB flash      │
  2CH Mic ──I2S──▶  │  I2S0_RX  ┌──────────┐  I2S0_TX │  ──I2S──▶  NAU8325 ──▶  Headphones
  (Stereo, 24-bit,  │            │ Audio DSP │          │
   44.1kHz)         │            │ - WAV enc │          │  ──I2C──▶  SH1106 OLED
                    │            │ - VU meter│          │           (VU bars, time, battery)
  SD Card ◀──SPI──▶ │  SPI2      │ - EQ/Dyn  │          │
  (WAV storage)     │            └──────────┘          │
                    │                                  │
  Buttons ──GPIO──▶ │  REC ●  STOP ■  PLAY ▶  MODE    │
                    │                                  │
  Battery ──BMS──▶  │  Built-in LiPo charger           │
                    └──────────────────────────────────┘
```

## Kit BOM (Everything from PCB Cupid)

| Module | Price | What It Does |
|--------|-------|--------------|
| Glyph ESP32-S3 | ₹699 | Brain + battery management |
| G-Sense 2CH MEMS Mic I2S | ₹499 | The microphone itself |
| NAU8325 Codec+Amp | TBD | Headphone monitoring output |
| G-Mod Micro SD Card Breakout | ₹99 | WAV file storage |
| 1.3" SH1106 OLED (I2C) | ₹320 | Status display + VU meter |
| 3D Printed Enclosure | ₹199 | Housing |
| LiPo Battery | ~₹200-400 | Portable power |
| **Total** | **~₹2,000-2,500** | |

For context: a basic stereo field recorder costs ₹8,000-10,000. A good wireless mic setup costs ₹15,000-25,000. This kit costs less than a dinner for two at a nice restaurant, and you walk away knowing how digital audio actually works.

## What's in This Repo

This repository will contain:
- **Firmware** — Arduino sketch for the ESP32-S3 with RTOS-based audio pipeline
- **Hardware** — Wiring diagrams, pin mappings, enclosure design notes
- **Documentation** — Build guide, user manual, component reference
- **Content** — Demo videos, sample recordings, comparison tests

## The Spirit of This Project

We're not trying to beat the audio engineering industry at their own game. We're trying to show what's possible when you look at the components sitting on your own workbench and ask: *what if we just built it ourselves?*

Sometimes the answer is "it won't be as good." And that's fine. But sometimes the answer is "it'll be 90% as good for 10% of the price, and you'll understand every piece of it." That's the sweet spot. That's where PCB Cupid lives.

Let's build a mic.
