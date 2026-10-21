# RS485 TX Test - Hardware Verification

Simple test program to verify RS485 TTL board is receiving data from ESP32.

## Purpose
Verify the RS485 board RX LED blinks when ESP32 sends data, confirming proper hardware connection before attempting to sniff heater protocol.

## Hardware Setup

### Connections
```
ESP32          →    RS485 TTL Module
GPIO17 (TX2)   →    DI (Data In / TX)
GPIO16 (RX2)   →    RO (Receive Out / RX)
GND            →    GND
3.3V or 5V     →    VCC
```

### RS485 Board Terminals
- **A+** and **B-**: Leave disconnected for this test (or can connect to control panel later)

## Expected Behavior

1. Upload firmware to ESP32
2. Open serial monitor (115200 baud)
3. **Watch the RS485 board RX LED** - it should **blink every second**
4. Serial monitor shows: `Sent: TEST_DATA_0 + binary pattern (RX LED should blink now)`

## Troubleshooting

### RX LED not blinking?
- Check TX/RX pin connections (GPIO17 → DI, GPIO16 → RO)
- Verify GND connection between ESP32 and RS485 board
- Check RS485 board power (VCC connected, LED power indicator on)
- Verify correct GPIO pins in code match your wiring

### Next Steps
Once RX LED blinks correctly:
1. Connect A+/B- to heater control panel
2. Modify code to listen (sniff) instead of transmit
3. Capture actual heater protocol data
