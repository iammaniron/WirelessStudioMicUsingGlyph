// ============================================================
// PCB Cupid — Wireless Studio Mic using Glyph C6
// ============================================================
// Turns a Glyph C6, 2CH MEMS Mic, and Micro SD Card module
// into a portable stereo recorder with its own WiFi hotspot
// and web-based control panel.
//
// Dependencies (install via Arduino Library Manager):
//   - AudioTools by Phil Schatzmann
// ============================================================

#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <AudioTools.h>
#include "config.h"

// ============================================================
// GLOBALS
// ============================================================

WebServer server(WEB_PORT);

AudioInfo audioInfo(SAMPLE_RATE, NUM_CHANNELS, BITS_PER_SAMPLE);
I2SStream i2sStream;

bool isRecording       = false;
unsigned long recordStartMs = 0;
unsigned long samplesWritten = 0;     // per channel
File currentFile;
String currentFileName;

// ---- File counter for unique names ----
int fileCounter = 0;

// ---- Button debounce ----
unsigned long lastBtnCheck = 0;
bool lastBtnState         = HIGH;  // unpressed (pullup)
bool btnPressed           = false;

// ============================================================
// SD CARD
// ============================================================

bool initSD() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] Card init failed — is the card inserted?");
    return false;
  }

  uint8_t type = SD.cardType();
  if (type == CARD_NONE) {
    Serial.println("[SD] No card detected");
    return false;
  }

  Serial.print("[SD] Card ready — ");
  Serial.print(SD.cardSize() / (1024 * 1024));
  Serial.println(" MB");

  // Find next recording number by scanning existing files
  File root = SD.open("/");
  while (true) {
    File f = root.openNextFile();
    if (!f) break;
    String name = f.name();
    if (name.startsWith("/recording_") && name.endsWith(".wav")) {
      int n = name.substring(11, name.length() - 4).toInt();
      if (n >= fileCounter) fileCounter = n + 1;
    }
    f.close();
  }
  root.close();
  return true;
}

// List recordings on the SD card, returns JSON array fragment
String listRecordingsJSON() {
  String json = "[";
  File root = SD.open("/");
  bool first = true;

  while (true) {
    File f = root.openNextFile();
    if (!f) break;

    String name = f.name();
    if (name.endsWith(".wav")) {
      if (!first) json += ",";
      first = false;

      // Estimate duration from file size
      size_t dataSize = (f.size() > 44) ? (f.size() - 44) : 0;
      float seconds = (float)dataSize /
                      (SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8));

      json += "{";
      json += "\"name\":\"" + name + "\",";
      json += "\"size\":" + String(f.size()) + ",";
      json += "\"duration\":" + String(seconds, 1);
      json += "}";
    }
    f.close();
  }
  root.close();
  json += "]";
  return json;
}

// Human-readable file size
String formatSize(size_t bytes) {
  if (bytes < 1024)        return String(bytes) + " B";
  if (bytes < 1048576)     return String(bytes / 1024.0, 1) + " KB";
  return String(bytes / 1048576.0, 1) + " MB";
}

// ============================================================
// WAV FILE HELPERS
// ============================================================

void writeWavHeader(File &f, uint32_t dataBytes) {
  uint32_t sampleRate    = SAMPLE_RATE;
  uint16_t numChannels   = NUM_CHANNELS;
  uint16_t bitsPerSample = BITS_PER_SAMPLE;
  uint16_t bytesPerSample = bitsPerSample / 8;
  uint32_t byteRate      = sampleRate * numChannels * bytesPerSample;
  uint16_t blockAlign    = numChannels * bytesPerSample;
  uint32_t riffSize      = 36 + dataBytes;

  // Helper lambda to write little-endian values
  auto w32 = [&](uint32_t v) { f.write((uint8_t*)&v, 4); };
  auto w16 = [&](uint16_t v) { f.write((uint8_t*)&v, 2); };

  f.write((const uint8_t*)"RIFF", 4);  w32(riffSize);
  f.write((const uint8_t*)"WAVE", 4);

  f.write((const uint8_t*)"fmt ", 4);  w32(16);       // fmt chunk size
  w16(1);                               // PCM format
  w16(numChannels);
  w32(sampleRate);
  w32(byteRate);
  w16(blockAlign);
  w16(bitsPerSample);

  f.write((const uint8_t*)"data", 4);  w32(dataBytes);
}

// ============================================================
// RECORDING
// ============================================================

bool startRecording() {
  if (isRecording) return false;

  currentFileName = "/recording_" + String(fileCounter++) + ".wav";
  currentFile = SD.open(currentFileName, FILE_WRITE);
  if (!currentFile) {
    Serial.println("[REC] Cannot create file: " + currentFileName);
    return false;
  }

  // Placeholder header — will be re-written on stop
  writeWavHeader(currentFile, 0);

  // Configure I2S for capture
  auto cfg = i2sStream.defaultConfig(RX_MODE);
  cfg.copyFrom(audioInfo);
  cfg.i2s_format   = I2S_STD_FORMAT;
  cfg.pin_ws       = I2S_WS;
  cfg.pin_bck      = I2S_SCK;
  cfg.pin_data_rx  = I2S_SD;
  cfg.is_master    = true;
  cfg.use_apll     = false;

  if (!i2sStream.begin(cfg)) {
    Serial.println("[REC] I2S init failed");
    currentFile.close();
    SD.remove(currentFileName);
    return false;
  }

  isRecording    = true;
  recordStartMs  = millis();
  samplesWritten = 0;

  pinMode(RECORD_LED, OUTPUT);
  digitalWrite(RECORD_LED, HIGH);

  Serial.println("[REC] Started → " + currentFileName);
  return true;
}

void stopRecording() {
  if (!isRecording) return;

  i2sStream.end();

  // Calculate actual data size and re-write header
  uint32_t dataBytes = samplesWritten * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
  currentFile.seek(0);
  writeWavHeader(currentFile, dataBytes);
  currentFile.close();

  float duration = (float)samplesWritten / SAMPLE_RATE;

  Serial.println("[REC] Stopped → " + currentFileName);
  Serial.printf("[REC] %.1f sec, %s\n", duration, formatSize(dataBytes + 44).c_str());

  digitalWrite(RECORD_LED, LOW);
  isRecording = false;
}

// Called every loop iteration — drains I2S DMA buffer into SD card
void recordingLoop() {
  if (!isRecording) return;

  static uint8_t buffer[CHUNK_SIZE];
  size_t bytesRead = i2sStream.readBytes(buffer, CHUNK_SIZE);

  if (bytesRead > 0) {
    currentFile.write(buffer, bytesRead);
    samplesWritten += bytesRead / (NUM_CHANNELS * (BITS_PER_SAMPLE / 8));
  }

  // Blink LED while recording (on/off every 500ms based on sample count)
  digitalWrite(RECORD_LED, (samplesWritten / (SAMPLE_RATE / 2)) % 2);
}

// ---- Onboard BOOT key as record/stop button (GPIO 9) ----
// After boot, the BOOT pin is free. Short press toggles recording.
// Debounced at ~30ms. Uses internal pullup — no external wiring needed.
void handleButton() {
  unsigned long now = millis();
  if (now - lastBtnCheck < 30) return;   // debounce
  lastBtnCheck = now;

  bool state = digitalRead(RECORD_BTN);
  if (state == LOW && lastBtnState == HIGH) {
    // falling edge — button pressed
    if (isRecording) {
      stopRecording();
      Serial.println("[BTN] Recording stopped via button");
    } else {
      startRecording();
      Serial.println("[BTN] Recording started via button");
    }
  }
  lastBtnState = state;
}

// ============================================================
// WEB SERVER — HTML PAGE
// ============================================================

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Glyph Studio Mic</title>
<style>
  *, *::before, *::after { box-sizing:border-box; margin:0; padding:0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    background: #0f0f0f; color: #e0e0e0; min-height: 100vh;
    display: flex; justify-content: center; padding: 20px;
  }
  .container { max-width: 520px; width: 100%; }
  h1 { font-size: 1.3rem; font-weight: 600; margin-bottom: 4px; color: #fff; }
  .subtitle { font-size: 0.8rem; color: #888; margin-bottom: 24px; }

  .status-bar {
    display: flex; align-items: center; gap: 10px; margin-bottom: 20px;
    padding: 14px 18px; border-radius: 12px; background: #1a1a1a;
  }
  .status-dot {
    width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0;
    background: #444; transition: background 0.3s;
  }
  .status-dot.recording { background: #ff3b30; box-shadow: 0 0 8px #ff3b30; animation: pulse 1s infinite; }
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }
  .status-text { font-size: 0.95rem; flex: 1; }
  .status-time { font-size: 1.4rem; font-weight: 700; font-variant-numeric: tabular-nums; color: #fff; }

  .controls { display: flex; gap: 10px; margin-bottom: 24px; }
  button {
    flex: 1; padding: 14px 0; border: none; border-radius: 10px;
    font-size: 0.95rem; font-weight: 600; cursor: pointer; transition: all 0.2s;
  }
  button:active { transform: scale(0.97); }
  #btnRecord {
    background: #ff3b30; color: #fff;
  }
  #btnRecord.recording { background: #1a1a1a; color: #ff3b30; border: 1.5px solid #ff3b30; }
  #btnRecord:disabled { opacity: 0.3; pointer-events: none; }

  .section-title {
    font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.5px;
    color: #666; margin-bottom: 10px;
  }
  .file-list { list-style: none; }
  .file-item {
    display: flex; align-items: center; gap: 12px;
    padding: 12px 14px; background: #1a1a1a; border-radius: 10px;
    margin-bottom: 6px;
  }
  .file-info { flex: 1; min-width: 0; }
  .file-name { font-size: 0.9rem; color: #fff; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
  .file-meta { font-size: 0.75rem; color: #777; margin-top: 2px; }
  .btn-download {
    background: #2d6ff7; color: #fff; padding: 8px 16px; border-radius: 8px;
    border: none; font-size: 0.8rem; font-weight: 600; cursor: pointer;
    text-decoration: none; flex-shrink: 0;
  }
  .btn-download:hover { background: #1a5ce5; }

  .empty-state {
    text-align: center; padding: 40px 20px; color: #555;
  }
  .empty-state .icon { font-size: 2rem; margin-bottom: 8px; }

  .footer {
    text-align: center; margin-top: 30px; font-size: 0.7rem; color: #444;
  }

  .toast {
    position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%);
    background: #2d6ff7; color: #fff; padding: 10px 24px; border-radius: 20px;
    font-size: 0.85rem; opacity: 0; transition: opacity 0.3s; pointer-events: none;
  }
  .toast.show { opacity: 1; }
</style>
</head>
<body>
<div class="container">

  <h1>Glyph Studio Mic</h1>
  <p class="subtitle">Wireless Stereo Recorder — PCB Cupid</p>

  <div class="status-bar">
    <div class="status-dot" id="statusDot"></div>
    <span class="status-text" id="statusText">Ready</span>
    <span class="status-time" id="statusTime">00:00</span>
  </div>

  <div class="controls">
    <button id="btnRecord" onclick="toggleRecord()">Start Recording</button>
  </div>

  <p class="section-title">Recordings on SD Card</p>
  <ul class="file-list" id="fileList"></ul>
  <div class="empty-state" id="emptyState">
    <div class="icon">🎙️</div>
    <p>No recordings yet. Press record to start.</p>
  </div>

  <p class="footer">PCB Cupid &bull; Glyph C6 &bull; G-Sense 2CH Mic</p>

</div>
<div class="toast" id="toast"></div>

<script>
  const btnRecord  = document.getElementById('btnRecord');
  const statusDot  = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const statusTime = document.getElementById('statusTime');
  const fileList   = document.getElementById('fileList');
  const emptyState = document.getElementById('emptyState');
  const toast      = document.getElementById('toast');

  function fmtTime(sec) {
    const m = Math.floor(sec / 60);
    const s = Math.floor(sec % 60);
    return String(m).padStart(2,'0') + ':' + String(s).padStart(2,'0');
  }

  function fmtSize(b) {
    if (b < 1024) return b + ' B';
    if (b < 1048576) return (b/1024).toFixed(1) + ' KB';
    return (b/1048576).toFixed(1) + ' MB';
  }

  function toggleRecord() {
    const url = btnRecord.classList.contains('recording')
      ? '/api/record/stop'
      : '/api/record/start';
    fetch(url).then(r => r.text()).then(txt => {
      if (txt === 'OK') poll(); // immediate refresh
    });
  }

  function downloadFile(name) {
    window.location.href = '/download?file=' + encodeURIComponent(name);
  }

  function showToast(msg) {
    toast.textContent = msg;
    toast.classList.add('show');
    setTimeout(() => toast.classList.remove('show'), 2000);
  }

  function poll() {
    fetch('/api/status')
      .then(r => r.json())
      .then(data => {
        // Recording state
        if (data.recording) {
          statusDot.classList.add('recording');
          statusText.textContent = 'Recording...';
          statusTime.textContent = fmtTime(data.elapsed);
          btnRecord.textContent = 'Stop Recording';
          btnRecord.classList.add('recording');
        } else {
          statusDot.classList.remove('recording');
          statusText.textContent = 'Ready';
          statusTime.textContent = '00:00';
          btnRecord.textContent = 'Start Recording';
          btnRecord.classList.remove('recording');
        }

        // File list
        fileList.innerHTML = '';
        if (data.files.length === 0) {
          emptyState.style.display = 'block';
        } else {
          emptyState.style.display = 'none';
          data.files.forEach(f => {
            const li = document.createElement('li');
            li.className = 'file-item';
            li.innerHTML =
              '<div class="file-info">' +
                '<div class="file-name">' + f.name + '</div>' +
                '<div class="file-meta">' + fmtTime(f.duration) + ' &middot; ' + fmtSize(f.size) + '</div>' +
              '</div>' +
              '<button class="btn-download" onclick="downloadFile(\'' + f.name + '\')">Download</button>';
            fileList.appendChild(li);
          });
        }
      });
  }

  // Poll every second
  setInterval(poll, 1000);
  poll();
</script>
</body>
</html>
)rawliteral";

// ============================================================
// WEB SERVER — ROUTES
// ============================================================

void setupWebServer() {

  // ---- Main page ----
  server.on("/", []() {
    server.send(200, "text/html", PAGE_HTML);
  });

  // ---- API: Status (JSON) ----
  server.on("/api/status", []() {
    String json = "{";
    json += "\"recording\":" + String(isRecording ? "true" : "false") + ",";
    json += "\"elapsed\":" + String(isRecording ? (millis() - recordStartMs) / 1000 : 0) + ",";
    json += "\"files\":" + listRecordingsJSON();
    json += "}";
    server.send(200, "application/json", json);
  });

  // ---- API: Start recording ----
  server.on("/api/record/start", []() {
    if (startRecording()) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "SD card error");
    }
  });

  // ---- API: Stop recording ----
  server.on("/api/record/stop", []() {
    stopRecording();
    server.send(200, "text/plain", "OK");
  });

  // ---- Download a file ----
  server.on("/download", []() {
    if (!server.hasArg("file")) {
      server.send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filename = server.arg("file");

    // Security: prevent directory traversal
    if (filename.indexOf("..") != -1 || !filename.startsWith("/")) {
      server.send(403, "text/plain", "Forbidden");
      return;
    }

    if (!SD.exists(filename)) {
      server.send(404, "text/plain", "File not found");
      return;
    }

    File downloadFile = SD.open(filename, FILE_READ);
    if (!downloadFile) {
      server.send(500, "text/plain", "Cannot open file");
      return;
    }

    size_t fileSize = downloadFile.size();
    server.sendHeader("Content-Type", "audio/wav");
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename.substring(1) + "\"");
    server.sendHeader("Content-Length", String(fileSize));
    server.streamFile(downloadFile, "audio/wav");
    downloadFile.close();
  });

  // ---- 404 ----
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("[WEB] Server started on http://" + WiFi.softAPIP().toString());
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("══════════════════════════════════════");
  Serial.println("  PCB Cupid — Wireless Studio Mic");
  Serial.println("  Glyph C6 + G-Sense 2CH Mic + SD Card");
  Serial.println("══════════════════════════════════════");

  // ---- SD Card ----
  Serial.print("[INIT] SD Card... ");
  if (!initSD()) {
    Serial.println("FATAL — check card and wiring, then reset.");
    while (1) { delay(1000); }
  }

  // ---- WiFi Access Point ----
  Serial.print("[INIT] WiFi AP... ");
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println(WiFi.softAPIP());

  // ---- Web Server ----
  setupWebServer();

  // ---- Onboard LED & Button ----
  pinMode(RECORD_LED, OUTPUT);
  digitalWrite(RECORD_LED, LOW);
  pinMode(RECORD_BTN, INPUT_PULLUP);   // GPIO 9 — onboard BOOT key, acts as user button after boot

  Serial.println("[READY] Connect to WiFi: \"" AP_SSID "\"");
  Serial.println("[READY] Web panel → http://" + WiFi.softAPIP().toString());
  Serial.println("[READY] Or press the onboard BOOT button to record/stop");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  server.handleClient();
  recordingLoop();
  handleButton();
}
