# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

IoT street light monitoring system using an **ESP32** microcontroller with an **ACS712 current sensor**. The ESP32 reads current data and POSTs it over WiFi to a Flask-SocketIO server, which streams it in real-time to a browser dashboard.

## Architecture

```
ESP32 + ACS712 sensor
    │  HTTP POST JSON every 500ms
    ▼
Flask server (server.py :5000)
    │  SocketIO emit "sensor_update"
    ▼
Browser dashboard (templates/index.html)
```

- **`main.ino`** — ESP32 firmware: WiFi connection, ACS712 reading with auto-calibration, HTTP POST to `/api/data`
- **`server.py`** — Flask-SocketIO server: receives sensor JSON at `POST /api/data`, broadcasts to all connected browsers via SocketIO, serves the dashboard at `/`
- **`templates/index.html`** — Real-time dashboard with Tailwind CSS: live current display, color-coded load status badge, ADC/offset stats, 60-point history chart
- **`README.md`** — Contains an extended reference version of the Arduino sketch with serial commands (`status`, `calibrate`, `help`)

## Running the Server

```bash
pip install flask flask-socketio
python server.py
```

Server runs on `http://0.0.0.0:5000` with debug mode enabled.

## Build & Upload (Firmware)

Requires Arduino IDE or Arduino CLI with ESP32 board support installed.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port <PORT> .
arduino-cli monitor --port <PORT> --config baudrate=115200
```

Before flashing, update WiFi credentials and server IP in `main.ino`:
- `ssid` / `password` — WiFi network
- `serverURL` — Flask server IP (e.g., `http://192.168.1.x:5000/api/data`)

## Key Constants

- `SENSOR_PIN 34` — ADC input pin
- `sensitivity = 0.185` — ACS712-5A module (change to 0.100 for 20A or 0.066 for 30A)
- `samples = 50` — averaging window for noise reduction
- Calibration takes 100 samples at startup with no load connected

## API

- `POST /api/data` — ESP32 sends `{ current_mA, current_A, adc, offset, status }`
- `GET /api/latest` — Returns the most recent sensor reading
- SocketIO event `sensor_update` — Broadcasted on each new reading
