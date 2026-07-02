# The ₹2,000 Studio Mic

## Where This Came From

A few weeks ago, we were filming some content for PCB Cupid's YouTube channel. We had this tiny wireless microphone clipped onto the presenter's collar — one of those expensive ones that every creator swears by. You know the kind. Costs somewhere north of fifteen thousand rupees. Sounds great. Pairs instantly. The whole crew loves it.

And then someone said the obvious thing out loud.

*"Wait. We literally sell a stereo microphone module for ₹499. We have our own Glyph boards with battery management. We have SD card modules. We have audio amplifiers. Why are we using someone else's mic?"*

Silence.

We all looked at each other. Then at the pile of Glyph boards on the workbench. Then at the 2-channel MEMS mic module sitting in its anti-static bag, waiting for someone to give it a real project.

That's how this started. Not with a datasheet. With a question that wouldn't go away.

## The Bet

Here's the thing about building hardware in India. You're always fighting the perception that the expensive imported thing is better. And sometimes it is. That wireless mic we were using is genuinely good engineering. No argument there.

But we started doing the math.

That wireless setup costs fifteen grand and does exactly one thing. Meanwhile, we've got a stereo mic module for five hundred bucks, a Glyph S3 with a built-in battery charger for seven hundred, an SD card breakout for ninety-nine, an OLED display, an audio amplifier — all sitting in our own inventory. Every piece of it open source. Every line of code yours to change.

Now look, I'm not going to pretend a ₹499 MEMS microphone is going to beat a studio condenser. It won't. The physics of a microscopic silicon diaphragm are what they are. The noise floor is higher. The bass rolls off at 60 Hz. If you're recording a platinum album, buy the Neumann.

But here's what we *can* do: build something that costs less than the GST on that wireless mic, records clean 24-bit stereo audio straight to an SD card, shows you live VU meters on an OLED, lets you monitor through headphones with zero latency, runs on a battery for hours, and fits in your pocket. And when it breaks, or you want to change how it works, you open the code and fix it yourself.

That's a different kind of value. That's the PCB Cupid kind.

## What We're Actually Building

A self-contained stereo recording microphone. Portable. Battery powered. All-digital signal path. Made entirely from our own modules.

The heart of it is our 2-channel MEMS microphone module — two matched digital mics on one board, spitting out 24-bit stereo I2S audio. No analog preamp. No ADC noise. No ground loops. Just ones and zeros straight into the Glyph.

The brain is a Glyph S3 — dual-core, 240 MHz, 8 MB of flash, and crucially, a built-in LiPo battery charger. It's got two I2S peripherals, which means we can pull audio in from the mic and push it out to the amplifier at the same time. Real-time monitoring, no lag.

Audio gets written as proper WAV files to a microSD card over SPI. A tiny OLED shows recording time, bouncing VU meter bars, battery level. A few buttons for record, stop, playback. An NAU8325 amplifier drives headphones or a small speaker so you can hear yourself while recording.

All of this goes into a 3D-printed enclosure. The whole thing, end to end, costs around two to two-and-a-half thousand rupees in parts. Every single component is something we already sell on shop.pcbcupid.com.

## Why a Kit

We want to sell this as a kit. Not a finished product. A kit.

Someone builds this, they learn I2S audio, SPI storage, I2C displays, RTOS task design, WAV file formats, real-time DSP. Those are real embedded engineering skills. It's a walking catalog of PCB Cupid products working together — Glyph board, G-Sense mic, G-Mod SD card, GLINK accessories.

But honestly, the bigger reason is simpler. There's something different about recording a podcast on gear you assembled and programmed yourself versus something you unboxed. Content creators love that story. Their audience loves that story.

And it's hackable by design. Want Wi-Fi streaming? The Glyph has it built in. Want a web interface for settings? Go ahead. Want to add DSP effects? The processor can handle it. The kit is a starting point, not a finished product.

## How It Actually Sounds

I want to be straight about this, because overselling helps nobody.

For close-mic podcast vocals, it works. If you're six to twelve inches from the mic, the signal-to-noise ratio is totally usable. Add a noise gate — either in post, or running right on the Glyph in real time — and the noise floor disappears between words. Voices come through clean. The chest resonance, the warmth that lives around 100 to 200 Hz, it's all there.

Stereo interviews are where this thing gets interesting. Two mics, two channels, one module. Put it between two people and you get natural stereo separation. That's something even expensive lav mics can't do. Acoustic instruments — guitar, ukulele, violin — the high end is crisp and detailed.

Where it struggles: whisper-quiet recordings, ASMR, anything where the sound you're capturing lives close to the noise floor. The self-noise is around 29 dBA, and in a silent room, you'll hear it. Bass-heavy music is also not its strength — the MEMS element starts rolling off at 60 Hz, so kick drums and bass guitars lose their weight. And no, it won't replace a large-diaphragm condenser. A microscopic silicon diaphragm will never move air the way a one-inch membrane does. Physics is stubborn about these things.

But here's the clever part that makes up for a lot of this. Because the audio is digital from the moment it leaves the mic capsule, and because the Glyph S3 has real DSP muscle, we can do things in software that analog mics need racks of outboard gear for. Noise gating to clean up the silence between words. A touch of EQ to bring out warmth. Compression to even out the levels. Auto gain so you don't clip.

Applied tastefully, these close a surprising amount of the gap between a MEMS mic and a budget condenser. For podcasting and content creation, they can tip the difference between "sounds like a fun DIY project" and "wait, that actually sounds like a real mic."

## Rough Plan

The recording pipeline is straightforward. Mic to Glyph over I2S. Glyph encodes WAV and writes to SD card. Simultaneously, it passes the audio through to the NAU8325 amplifier for headphone monitoring. An OLED display on I2C shows recording time and VU levels pulled from peak detection on the incoming audio. A handful of GPIO buttons handle record, stop, playback, and mode switching. The whole thing runs on a LiPo, managed by the Glyph S3's built-in charger.

That's the core. Everything else — Wi-Fi streaming, DSP effects, web control panel — those are stretch goals. The foundation is solid on its own.

## Kit Parts (All from PCB Cupid)

Glyph S3 at ₹699 handles the brainwork and keeps the battery happy. The G-Sense 2CH MEMS Mic at ₹499 is the microphone itself. A G-Mod Micro SD Card Breakout at ₹99 stores the recordings. The NAU8325 drives headphones for monitoring. A 1.3-inch SH1106 OLED at ₹320 shows you what's going on. The 3D-printed enclosure at ₹199 holds it all together. Toss in a LiPo for a couple hundred rupees, and you're sitting at somewhere between two and two-and-a-half thousand for the whole build.

For perspective: a basic stereo field recorder costs eight to ten thousand. A decent wireless mic setup runs fifteen to twenty-five. This kit costs less than a nice dinner for two, and you walk away understanding how digital audio actually works, from the silicon up.

## What's in This Repo (Eventually)

Firmware for the Glyph S3, the RTOS-based audio pipeline, all of it in Arduino. Wiring diagrams and pin mappings. A proper build guide. And once it's all working, demo videos, sample recordings, comparison tests against the expensive stuff.

## The Spirit of This

We're not trying to beat the audio engineering industry at their own game. We're trying to show what's possible when you look at the components sitting on your own workbench and ask the question that started all of this.

What if we just built it ourselves?

Sometimes the answer is "it won't be as good." And that's fine. But sometimes the answer is "it'll be ninety percent as good for ten percent of the price, and you'll understand every piece of it." That's the sweet spot. That's where PCB Cupid lives.

Let's build a mic.
