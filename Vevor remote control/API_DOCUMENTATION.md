# Vevor Heater REST API Documentation

Version: 2.0 (heater_ui_v2_web)

## Overview

REST API för fjärrstyrning av Vevor dieselvärmare via WiFi. API:et använder JSON-format och token-baserad autentisering för skyddade operationer.

**Base URL**: `http://<ESP32_IP_ADDRESS>`

**Default PIN**: `1234`

## WiFi Configuration

Innan API:et kan användas måste ESP32:n anslutas till WiFi. Detta görs via seriell terminal:

```bash
# Anslut till serial monitor (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXXX -b 115200

# Konfigurera WiFi (körs EN gång, sparas permanent)
wifi ssid YourNetworkName
wifi pass YourPassword
wifi connect

# Visa status
wifi status
```

Efter lyckad anslutning startar web servern automatiskt och visar:
```
WiFi connected!
IP address: 192.168.X.X

Web server available at: http://192.168.X.X
[OTA] Ready! Hostname: vevor-heater, IP: 192.168.X.X
[OTA] Upload via: pio run -t upload --upload-port 192.168.X.X

REST API ENDPOINTS:
  GET  /api/status        : Full status
  GET  /api/temperature   : Temperature data
  ...
```

## OTA (Over-The-Air) Updates

ESP32 stödjer firmware-uppdatering över WiFi utan USB-kabel.

**Krav**: ESP32 måste vara ansluten till WiFi

### Första gången (via USB)
```bash
# Ladda upp första firmware via USB
pio run -e heater_ui_v2_web -t upload
```

### Efterföljande uppdateringar (via WiFi)
```bash
# Uppdatera firmware över WiFi
pio run -e heater_ui_v2_web -t upload --upload-port 192.168.68.71
```

**OTA Password**: `1234` (samma som REST API PIN)

**Säkerhet under uppdatering**:
- Värmaren stoppas automatiskt vid OTA-uppdatering
- ESP32 startar om automatiskt efter framgångsrik uppdatering
- Vid fel behålls tidigare fungerande firmware

**Tips**:
- Se till att ESP32 har stabil WiFi-anslutning
- Uppdatera inte under kritisk drift
- Uppdateringen tar ~30 sekunder

## Authentication

Skyddade endpoints kräver Bearer-token som fås via `/api/authenticate`.

### POST /api/authenticate

Autentisera med PIN och få en session token.

**Request:**
```bash
POST /api/authenticate
Content-Type: application/json

{
  "pin": "1234"
}
```

**Response (Success):**
```json
{
  "success": true,
  "token": "f6fd49d4fba0af629f8aeba94d82504acaa72afacd031f28cf4f5da1f8955fda",
  "message": "Authentication successful"
}
```

**Response (Error):**
```json
{
  "error": "Invalid PIN",
  "code": 401
}
```

**Token Validity**: 1 timme

**Ändra PIN** (via serial):
```bash
pin set 5678
```

---

## Public Endpoints

### GET /api/status

Hämta fullständig värmare- och temperaturstatus.

**Request:**
```bash
GET /api/status
```

**Response:**
```json
{
  "temperature": {
    "current": 20.4,
    "smoothed": 20.3,
    "trend": 0.05,
    "target": 24
  },
  "heater": {
    "state": "Heating",
    "power": 7,
    "voltage": 12.5,
    "fanSpeed": 3200,
    "temperature": 45.5,
    "running": true,
    "error": {
      "code": 0,
      "message": "No error"
    },
    "hasError": false
  },
  "control": {
    "mode": "auto"
  }
}
```

**Fields:**
- `temperature.current`: Aktuell temperatur (°C)
- `temperature.smoothed`: 30-sekunders glidande medelvärde (°C)
- `temperature.trend`: Temperaturförändring (°C/min)
- `temperature.target`: Måltemperatur (°C)
- `heater.state`: OFF | PREHEAT | IGNITION | Heating | COOLDOWN
- `heater.power`: Effektnivå (1-10)
- `heater.voltage`: Systemspänning (V)
- `heater.fanSpeed`: Fläkthastighet (RPM)
- `heater.temperature`: Värmarens interna temperatur / heat exchanger (°C)
- `heater.running`: true om värmaren är igång (inte OFF)
- `heater.error.code`: Felkod (0 = inget fel)
- `heater.error.message`: Felbeskrivning
- `heater.hasError`: true om det finns ett fel
- `control.mode`: "auto" | "manual"

**Error Codes:**
- `0x00` (0) - No error
- `0x01` (1) - Voltage too low
- `0x02` (2) - Voltage too high
- `0x03` (3) - Ignition failure
- `0x04` (4) - Flame out
- `0x05` (5) - Temperature sensor error
- `0x06` (6) - Glow plug error
- `0x08` (8) - Fan motor error
- `0x09` (9) - Fuel pump error

### GET /api/temperature

Hämta enbart temperaturdata.

**Request:**
```bash
GET /api/temperature
```

**Response:**
```json
{
  "current": 20.4,
  "smoothed": 20.3,
  "trend": 0.05,
  "target": 24
}
```

### GET /api/signalk/values

Hämta data i SignalK-kompatibelt format för båtintegration.

**Request:**
```bash
GET /api/signalk/values
```

**Response:**
```json
{
  "environment": {
    "inside": {
      "cabin": {
        "temperature": {
          "value": 20.4,
          "$source": "vevor-heater",
          "timestamp": 123456
        },
        "heater": {
          "state": {
            "value": "Heating"
          },
          "power": {
            "value": 7
          },
          "targetTemperature": {
            "value": 24
          },
          "$source": "vevor-heater"
        }
      }
    }
  }
}
```

---

## Protected Endpoints

Kräver `Authorization: Bearer <token>` header.

### POST /api/start

Starta värmaren.

**Request:**
```bash
POST /api/start
Authorization: Bearer <token>
Content-Type: application/json
```

**Response:**
```json
{
  "success": true,
  "message": "Heater start command sent"
}
```

**Beteende:**
- Startar värmaren med sparad effektnivå
- Aktiverar automatisk temperaturreglering (AUTO mode)
- Returnerar omedelbart (värmaren går igenom PREHEAT → IGNITION → Heating)

### POST /api/stop

Stoppa värmaren.

**Request:**
```bash
POST /api/stop
Authorization: Bearer <token>
Content-Type: application/json
```

**Response:**
```json
{
  "success": true,
  "message": "Heater stop command sent"
}
```

**Beteende:**
- Initierar nedstängning (COOLDOWN-fas på ~3 minuter)
- Inaktiverar automatisk temperaturreglering

### POST /api/target

Ändra måltemperatur.

**Request:**
```bash
POST /api/target
Authorization: Bearer <token>
Content-Type: application/json

{
  "target": 24
}
```

**Response:**
```json
{
  "success": true,
  "target": 24,
  "message": "Target temperature set"
}
```

**Constraints:**
- Min: 5°C
- Max: 35°C

**Beteende:**
- I AUTO mode justerar värmaren automatiskt effekten för att nå måltemperaturen
- Ändringen sparas och används vid nästa start

### POST /api/power

Ändra effektnivå manuellt.

**Request:**
```bash
POST /api/power
Authorization: Bearer <token>
Content-Type: application/json

{
  "power": 7
}
```

**Response:**
```json
{
  "success": true,
  "power": 7,
  "message": "Power level set"
}
```

**Constraints:**
- Min: 1
- Max: 10

**Beteende:**
- Fungerar endast när värmaren är igång
- **INAKTIVERAR automatisk temperaturreglering**
- Använd för manuell styrning

---

## Error Responses

Alla fel returnerar JSON med `error` och `code` fields:

```json
{
  "error": "Error message",
  "code": 400
}
```

**Common Error Codes:**
- `400` - Bad Request (ogiltig parameter)
- `401` - Unauthorized (saknar token eller felaktig PIN)
- `404` - Not Found (okänd endpoint)
- `500` - Internal Server Error

---

## Auto Temperature Control

När värmaren startas aktiveras automatisk **trend-baserad prediktiv temperaturreglering** som justerar effekten baserat på både temperatur OCH uppvärmningshastighet för att undvika överskjutning:

### Algoritm
- **Smoothing**: 30-sekunders glidande medelvärde för att undvika sensor-brus
- **Trend**: Beräknar temperaturförändring över 5 minuter (°C/min)
- **Prediction**: Förutsäger temperatur 3 minuter framåt: `T_predicted = T + (trend × 3)`
- **Hybrid ON/OFF**: Kör kontinuerligt inom ±2°C från mål, stänger bara av vid >+2°C över mål

### Kontrollzoner

| Zon | Temp vs Mål | Trend | Effekt | Beteende |
|-----|-------------|-------|--------|----------|
| 1 | < -2.0°C | Any | 10 | Max uppvärmning |
| 2 | -2.0 till -1.0°C | >0.4°C/min (snabb) | 5 | Sänk tidigt för att undvika överskjutning |
| 2 | -2.0 till -1.0°C | 0.2-0.4°C/min | 6 | Moderat uppvärmning |
| 2 | -2.0 till -1.0°C | <0.2°C/min (långsam) | 7 | Stark uppvärmning |
| 3 | -1.0 till -0.5°C | Predicted overshoot | 1 | FÖREBYGG överskjutning |
| 3 | -1.0 till -0.5°C | >0.4°C/min | 3 | Finreglering |
| 3 | -1.0 till -0.5°C | <0.4°C/min | 5 | Moderat uppvärmning |
| 4 | -0.5 till -0.2°C | >0.2°C/min | 1 | Minimal (värmer fortfarande snabbt) |
| 4 | -0.5 till -0.2°C | <0.2°C/min | 3 | Finreglering |
| 5 | ±0.5°C | <-0.1°C/min (kyls) | 3 | Underhåll+ (kompensera kyla) |
| 5 | ±0.5°C | >0.1°C/min (värms) | 1 | Underhåll- (minska värme) |
| 5 | ±0.5°C | Stabil | 2 | Normal underhåll |
| 6 | +0.5 till +2.0°C | Any | 1 | Minimal effekt |
| 7 | > +2.0°C | Any | **OFF** | Stäng av tills <-1°C (10 min min) |

### Nyckelfördelar
- **Anpassar sig till yttertemperatur**: Högre trend vid varmt väder → sänker effekt tidigare
- **Undviker överskjutning**: Prediktiv kontroll förutser framtida temperatur
- **Kontinuerlig drift**: Sällan ON/OFF-cykler (endast vid >+2°C över mål)
- **Snabb respons**: Uppdaterar effekt var ~2 sekund baserat på trend

### Tuning-parametrar
- `LOOKAHEAD_TIME = 3.0 min`: Hur långt fram algoritmen förutsäger
- `TREND_FAST = 0.4°C/min`: Tröskel för "snabb uppvärmning"
- `TREND_MEDIUM = 0.2°C/min`: Tröskel för "medel uppvärmning"
- `MIN_CYCLE_TIME = 10 min`: Minimum tid mellan ON/OFF-växlingar

**Inaktivera AUTO mode** (via serial):
```bash
d  # Disable auto mode
a  # Enable auto mode
```

---

## Integration Examples

### cURL

```bash
# Autentisera och spara token
TOKEN=$(curl -s -X POST http://192.168.68.71/api/authenticate \
  -H "Content-Type: application/json" \
  -d '{"pin":"1234"}' | jq -r '.token')

# Starta värmaren
curl -X POST http://192.168.68.71/api/start \
  -H "Authorization: Bearer $TOKEN"

# Ändra måltemperatur till 22°C
curl -X POST http://192.168.68.71/api/target \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"target":22}'

# Övervaka status
watch -n 2 "curl -s http://192.168.68.71/api/temperature | jq"

# Stoppa värmaren
curl -X POST http://192.168.68.71/api/stop \
  -H "Authorization: Bearer $TOKEN"
```

### Python

```python
import requests
import json

BASE_URL = "http://192.168.68.71"

# Autentisera
response = requests.post(
    f"{BASE_URL}/api/authenticate",
    json={"pin": "1234"}
)
token = response.json()["token"]

# Headers för skyddade anrop
headers = {
    "Authorization": f"Bearer {token}",
    "Content-Type": "application/json"
}

# Starta värmaren
requests.post(f"{BASE_URL}/api/start", headers=headers)

# Sätt måltemperatur
requests.post(
    f"{BASE_URL}/api/target",
    headers=headers,
    json={"target": 22}
)

# Hämta status
status = requests.get(f"{BASE_URL}/api/status").json()
print(f"Temp: {status['temperature']['current']}°C")
print(f"State: {status['heater']['state']}")
print(f"Power: {status['heater']['power']}")
```

### Node.js / SignalK Plugin

```javascript
const axios = require('axios');

class VevorHeaterPlugin {
  constructor(app) {
    this.baseUrl = 'http://192.168.68.71';
  }

  async start() {
    // Poll status every 2 seconds
    setInterval(async () => {
      try {
        const { data } = await axios.get(`${this.baseUrl}/api/signalk/values`);

        // Publish to SignalK
        this.app.handleMessage('vevor-heater', {
          updates: [{
            values: [
              {
                path: 'environment.inside.cabin.temperature',
                value: data.environment.inside.cabin.temperature.value
              },
              {
                path: 'environment.inside.cabin.heater.state',
                value: data.environment.inside.cabin.heater.state.value
              }
            ]
          }]
        });
      } catch (err) {
        console.error('Failed to fetch heater data:', err);
      }
    }, 2000);
  }
}
```

---

## CORS Support

API:et stödjer Cross-Origin Resource Sharing (CORS) och kan anropas från webbläsare:

```javascript
// Fetch från JavaScript
fetch('http://192.168.68.71/api/status')
  .then(response => response.json())
  .then(data => console.log(data));
```

**CORS Headers:**
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type, Authorization`

---

## Security Considerations

1. **PIN Storage**: PIN hashas med SHA256 + salt och sparas i ESP32:ns säkra flash
2. **Token Expiry**: Tokens är giltiga i 1 timme och måste förnyas
3. **No HTTPS**: API:et använder HTTP (ej HTTPS) - använd endast i betrodda nätverk
4. **Rate Limiting**: Ingen rate limiting implementerad - var ansvarsfull
5. **Default PIN**: Ändra default PIN (1234) för produktion!

---

## Troubleshooting

### Web servern svarar inte

1. Kontrollera WiFi-anslutning via serial:
   ```bash
   wifi status
   ```

2. Om ansluten men server inte svarar, tryck RESET på ESP32

3. Kontrollera att port 80 inte är blockerad av brandvägg

### Token funkar inte

- Tokens är unika per session
- Hämta ny token med `/api/authenticate` om föregående gått ut
- Kontrollera att `Authorization: Bearer <token>` header är korrekt

### Värmaren startar inte via API

- Kontrollera att värmaren inte redan är igång (`/api/status`)
- Verifiera att värmaren fungerar med fysiska kontroller först
- Kolla serial output för felmeddelanden

### SignalK integration

- Kontrollera att SignalK-servern kan nå ESP32:ns IP
- Verifiera att polling-intervallet inte är för kort (rekommenderat: 2s)
- Använd `/api/signalk/values` endpoint för rätt format

---

## Change Log

### Version 2.0 (Phase 2)
- ✅ REST API med 8 endpoints
- ✅ Token-baserad autentisering (SHA256 + salt)
- ✅ WiFi-konfiguration via serial (permanent lagring)
- ✅ SignalK-kompatibelt format
- ✅ CORS-stöd för webbintegration
- ✅ Automatisk temperaturreglering via API
- ✅ JSON-baserad kommunikation

### Version 1.0 (Phase 1)
- Grundläggande UI med OLED och rotary encoder
- Manuell och automatisk temperaturkontroll
- Seriell kommunikation med Vevor-värmaren

---

## Support

För buggar eller frågor, se projektet på GitHub eller kontakta via Issues.

**Firmware Version**: heater_ui_v2_web
**Build Date**: 2026-04-07
**API Version**: 2.0
