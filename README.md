# AI Applied Mechanics Assistant

An ESP32 that hosts its own chat page. Ask an applied-mechanics question from any browser on your network and it goes to Groq's `llama-3.3-70b-versatile`. The model can decide to read the live temperature, humidity, light or distance from the board before it answers.

Full write-up: [docs/Report4_AI_Mechanics_Assistant.docx](docs/Report4_AI_Mechanics_Assistant.docx)

## Hardware

| Part | Pin |
|---|---|
| DHT11 | 19 |
| LDR | 35 |
| HC-SR04 | trig 25, echo 26 |

## Setup

1. Install: ArduinoJson, NTPClient, DHT sensor library (plus Adafruit Unified Sensor).
2. Get a key from console.groq.com.
3. Copy `secrets.example.h` to `secrets.h` and fill in Wi-Fi and the Groq key.
4. Upload `ai_mechanics_assistant.ino`, read the IP from the serial monitor, open it in a browser.

## Endpoints

| Route | Purpose |
|---|---|
| `GET /` | the chat page |
| `POST /ask` | body `{"question": "..."}`, returns `{"answer": "..."}` |

## How it works

Each question makes one or two calls to Groq. The first one carries four tool definitions (temperature, humidity, light, distance). If the model asks for one, the ESP32 reads the sensor and sends the result back in a second call to get the final answer.

## Known issues

- Groq calls block the web server, so nothing else is served while waiting.
- Only the first tool call in a response is handled.
- No chat memory, every question starts fresh.
- NTP wait at boot has no timeout.
- The light value is a rough estimate, not real lux.
