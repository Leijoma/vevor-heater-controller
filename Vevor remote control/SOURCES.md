# Sources and References

Comprehensive list of all sources, references, and related projects used in developing this Vevor heater smart controller.

## GitHub Repositories

### Primary Protocol Sources

**1. zatakon/esphome-vevor-heater**
- **URL:** [github.com/zatakon/esphome-vevor-heater](https://github.com/zatakon/esphome-vevor-heater)
- **Type:** ESPHome custom component
- **Status:** Active, working implementation
- **Content:**
  - Complete working serial protocol implementation
  - Packet encoding/decoding
  - State machine logic
  - Temperature control algorithms
  - Hardware connection details (signal inversion, level shifting)
- **Usage:** Referenced for protocol confirmation, tested approach validation
- **License:** Check repository for license terms

**2. zatakon/vevor_heater_control**
- **URL:** [github.com/zatakon/vevor_heater_control](https://github.com/zatakon/vevor_heater_control)
- **Type:** Protocol reverse engineering documentation
- **Status:** Original research, foundational
- **Content:**
  - Detailed packet structures with byte-by-byte breakdown
  - Command codes and state machines
  - Data encoding formats (temperature, voltage, etc.)
  - Checksum algorithms
  - Confidence levels for each field
- **Usage:** Primary reference for protocol specification
- **Note:** Most detailed protocol documentation available

### Related Heater Projects

**3. BluetoothHeater (Ray Jones)**
- **URL:** [gitlab.com/mrjones.id.au/bluetoothheater](https://gitlab.com/mrjones.id.au/bluetoothheater)
- **Type:** Complete heater controller firmware
- **Platform:** ESP32
- **Content:**
  - Similar diesel heater protocol (Chinese heaters)
  - TX gate transistor circuit design
  - Half-duplex communication handling
  - State machine implementation
  - OLED display integration
- **Usage:** Hardware design inspiration (TX gating), similar protocol patterns
- **License:** Open source (check repository)

**4. Hackaday - Vevor Diesel Heater Protocol**
- **URL:** [hackaday.io/project/195170-vevor-diesel-heater-protocol](https://hackaday.io/project/195170-vevor-diesel-heater-protocol)
- **Type:** Project documentation and community discussion
- **Content:**
  - Original protocol reverse engineering
  - Community contributions
  - Testing results
  - RS485 vs simple UART clarification
- **Usage:** Historical context, community validation
- **Note:** Referenced in early v1.0 roadmap, confirmed not RS485 but simple UART

## Local Projects (This Repository)

### Related PlatformIO Projects

**5. Vevor-Serial-Transmitter**
- **Path:** `../Vevor-Serial-Transmitter/`
- **Purpose:** Heater impersonation testing (ESP32 acts as heater)
- **Status:** Working, bidirectional communication verified
- **Content:**
  - 470Ω resistor + level shifter approach (proven)
  - Software TX gating implementation
  - Automatic response to controller packets
  - Heater status packet generation (56 bytes)
  - Testing procedures and validation
- **Usage:** Hardware validation, protocol testing, proof of concept
- **Key Achievement:** Confirmed 470Ω resistor approach works without transistor

**6. Vevor-Serial-Sniffer**
- **Path:** `../Vevor-Serial-Sniffer/`
- **Purpose:** Protocol monitoring and packet capture
- **Status:** Working packet capture
- **Content:**
  - Complete protocol documentation (PROTOCOL.md)
  - Packet decoder
  - Real-time monitoring
  - Checksum verification
  - Example captured packets
- **Usage:** Protocol verification, live traffic analysis
- **Key Documentation:** PROTOCOL.md - most complete local protocol spec

**7. WCS1800-CurrentSensor-Test**
- **Path:** `../WCS1800-CurrentSensor-Test/`
- **Purpose:** Current sensor calibration and testing
- **Status:** Calibration procedures documented
- **Content:**
  - Zero calibration procedure
  - Sensitivity calibration
  - Heater current profiling (startup, running, shutdown)
  - WCS1800 configuration
  - Voltage divider requirements
- **Usage:** Current sensor setup, heater state detection
- **Note:** Helps detect heater operation without serial protocol

## Hardware Components and Datasheets

### Level Shifters

**8. Bi-Directional Logic Level Converter (BSS138-based)**
- **Manufacturers:** SparkFun, Adafruit, generic Chinese modules
- **Datasheet:** BSS138 N-Channel MOSFET
- **Voltage Range:** 1.8V - 5.5V
- **Purpose:** 3.3V (ESP32) ↔ 5V (heater) translation
- **Tutorial:** [SparkFun Tutorial](https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide)
- **Usage:** Serial communication voltage translation
- **Note:** Avoid TXS0108E (has issues with UART at 4800 baud)

### Microcontrollers

**9. ESP32 (Espressif)**
- **Documentation:** [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- **Features Used:**
  - UART signal inversion (`setRxInvert()`, `setTxInvert()`)
  - Hardware UART (Serial2)
  - WiFi (AsyncWebServer)
  - ADC (current sensor)
  - OneWire (temperature sensors)
  - PWM (buzzer, future)
- **Platform:** PlatformIO with espressif32 framework
- **Datasheets:** ESP32 Technical Reference Manual

### Sensors

**10. WCS1800 Hall-Effect Current Sensor (0-35A)**
- **Manufacturer:** Winson (China)
- **Type:** Hall-effect linear current sensor
- **Range:** 0-35A AC/DC
- **Sensitivity:** 66.7mV/A
- **Supply:** 5V
- **Output:** Analog 0.5V-4.5V (2.5V at 0A)
- **Usage:** Heater power detection, state verification
- **Note:** Requires voltage divider (10kΩ/10kΩ) to protect ESP32

**11. DS18B20 Digital Temperature Sensor**
- **Manufacturer:** Maxim Integrated (Dallas Semiconductor)
- **Interface:** OneWire (single data wire)
- **Range:** -55°C to +125°C
- **Accuracy:** ±0.5°C (-10°C to +85°C)
- **Resolution:** 9-12 bits (configurable)
- **Libraries:**
  - OneWire by Paul Stoffregen
  - DallasTemperature by Miles Burton
- **Usage:** Duct temperature and room temperature sensing

## Software Libraries

### Arduino/PlatformIO Libraries

**12. RCSwitch (RF 433MHz)**
- **Repository:** [github.com/sui77/rc-switch](https://github.com/sui77/rc-switch)
- **Version:** 2.6.4
- **Purpose:** RF remote control (433MHz transmitter)
- **Usage:** v1.0/v2.0 RF control (backup method for v3.0+)
- **Status:** Working, tested with Vevor remote commands

**13. ESPAsyncWebServer**
- **Repository:** [github.com/me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- **Version:** 1.2.3
- **Purpose:** Async HTTP web server for ESP32
- **Usage:** Web interface, REST API
- **Status:** Working in v2.0

**14. AsyncTCP**
- **Repository:** [github.com/me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
- **Version:** 1.1.1
- **Purpose:** Async TCP library (required by ESPAsyncWebServer)
- **Usage:** Network communication
- **Status:** Working in v2.0

**15. ArduinoJson**
- **Repository:** [github.com/bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- **Version:** 7.0.4
- **Purpose:** JSON serialization/deserialization
- **Usage:** REST API responses, status data
- **Status:** Working in v2.0

**16. OneWire**
- **Author:** Paul Stoffregen
- **Repository:** [github.com/PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire)
- **Version:** 2.3.7
- **Purpose:** OneWire bus communication protocol
- **Usage:** DS18B20 temperature sensor communication
- **Status:** Working in v1.0+

**17. DallasTemperature**
- **Author:** Miles Burton
- **Repository:** [github.com/milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- **Version:** 3.11.0
- **Purpose:** DS18B20 temperature sensor library (uses OneWire)
- **Usage:** Temperature reading, sensor management
- **Status:** Working in v1.0+

## Protocol Standards and Documentation

### UART/Serial Communication

**18. UART Protocol Basics**
- **Standard:** Universal Asynchronous Receiver-Transmitter
- **Format:** 8N1 (8 data bits, no parity, 1 stop bit)
- **Baud Rate:** 4800 bps (Vevor specific)
- **Signal Inversion:** Non-standard (idle=LOW instead of idle=HIGH)
- **References:**
  - [Wikipedia: UART](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter)
  - ESP32 UART documentation

### Half-Duplex Communication

**19. Half-Duplex Serial Bus**
- **Concept:** Single wire for bidirectional communication
- **Timing:** One device transmits at a time
- **Approaches:**
  - TX gate transistor (BluetoothHeater approach)
  - Current-limiting resistor (our 470Ω approach)
  - Software TX gating (pinMode switching)
- **References:**
  - BluetoothHeater hardware schematics
  - Vevor-Serial-Transmitter testing results

## Community Resources

### Forums and Discussion

**20. Reddit - r/chinaglass / r/ESP32**
- **Topics:** Chinese diesel heater modifications, ESP32 projects
- **Usage:** Community troubleshooting, similar projects

**21. EEVBlog Forum**
- **Topics:** Diesel heater control, protocol reverse engineering
- **Usage:** Technical discussions, circuit design help

### Video Tutorials

**22. YouTube - Diesel Heater Control Projects**
- Various creators documenting similar projects
- Hardware setups and testing procedures
- Real-world installation examples

## Development Tools

**23. PlatformIO**
- **Website:** [platformio.org](https://platformio.org)
- **Version:** Core 6.1+
- **IDE:** VS Code extension
- **Purpose:** Development platform, library management, build system
- **Usage:** All project development

**24. Serial Monitor Tools**
- PlatformIO device monitor
- Arduino IDE Serial Monitor
- CoolTerm, PuTTY (alternatives)
- **Usage:** Debugging, protocol monitoring

**25. Protocol Analysis Tools**
- Hex editors (for packet analysis)
- Logic analyzers (for signal debugging)
- Oscilloscopes (for voltage level verification)
- **Usage:** Protocol verification, hardware debugging

## Design and CAD

**26. 3D Modeling for Enclosure**
- **Options:**
  - FreeCAD (open source)
  - Fusion 360 (free for hobbyists)
  - TinkerCAD (web-based, simple)
- **Usage:** Enclosure design for physical UI
- **Status:** Future (v4.0+)

**27. Circuit Simulation**
- **Options:**
  - Falstad Circuit Simulator (web-based)
  - LTspice (free, professional)
- **Usage:** Level shifter verification, voltage divider design

## Testing and Validation

### Testing Projects (Local)

All testing projects located in: `~/Documents/PlatformIO/Projects/`

- **Vevor-Serial-Transmitter:** Bidirectional communication testing
- **Vevor-Serial-Sniffer:** Passive monitoring
- **WCS1800-CurrentSensor-Test:** Sensor calibration

### Validation Methods

**28. Protocol Validation**
- Packet capture and comparison with known good packets
- Checksum verification
- State machine transition testing
- Real heater response testing

**29. Hardware Validation**
- Voltage level measurements
- Signal quality (oscilloscope)
- Current consumption
- Temperature sensor accuracy

## Safety and Standards

**30. Diesel Heater Safety**
- **Standards:** Various regional standards for diesel heaters
- **Key Safety Features:**
  - Flame sensor (prevents fuel flooding)
  - Overheat protection (automatic shutdown)
  - Low voltage cutoff
  - Exhaust temperature monitoring
- **Note:** Controller modifications do not override built-in safety features

**31. Carbon Monoxide Safety**
- **Requirement:** CO detector mandatory for indoor diesel heater use
- **Standards:** UL 2034, EN 50291 (CO detector standards)
- **Reference:** Manufacturer safety guidelines

## Related Commercial Products

**32. Aftermarket Controllers**
- Various commercial controllers available for Chinese diesel heaters
- Often based on similar protocols
- **Usage:** Feature inspiration, validation of control methods

## Documentation and Markdown

**33. Markdown Reference**
- **CommonMark:** [commonmark.org](https://commonmark.org)
- **GitHub Flavored Markdown:** For documentation formatting
- **Usage:** All project documentation files

## Version History and Changelog

**34. Project Development History**
- v1.0: Basic RF control (2023)
- v2.0: WiFi + web interface + power levels (2024)
- v3.0: Serial protocol integration (current - 2024)
- v4.0: Physical UI (planned)
- v5.0: Enhanced features (future)

## Acknowledgments

**Contributors and Inspirations:**
- **zatakon** - Original protocol reverse engineering and ESPHome implementation
- **Ray Jones** - BluetoothHeater project and TX gate circuit design
- **Hackaday community** - Protocol research and documentation
- **ESP32 community** - Libraries, tutorials, troubleshooting support

## Future References

**To Be Added:**
- OLED display libraries (when Phase 3 implemented)
- Rotary encoder libraries (when Phase 3 implemented)
- MQTT libraries (if Home Assistant integration added)
- OTA update libraries (for remote firmware updates)

## License and Legal

**License Notes:**
- This project: MIT License (or specify your license)
- Referenced projects: Check individual repository licenses
- Commercial use: Verify license compatibility
- Vevor trademark: Belongs to respective owner
- **Disclaimer:** Use at your own risk, no warranty provided

## Contact and Support

**Project Repository:**
- GitHub: (if you decide to publish)
- Issues: (if public repository)
- Discussions: (if public repository)

**Community Support:**
- ESP32 forums
- Reddit: r/ESP32, r/arduino
- Discord: Various ESP32/Arduino servers

---

## Quick Reference Links

**Essential Documentation:**
- [SERIAL_PROTOCOL.md](SERIAL_PROTOCOL.md) - Complete protocol specification
- [HARDWARE_SERIAL.md](HARDWARE_SERIAL.md) - Wiring guide
- [HARDWARE_UI.md](HARDWARE_UI.md) - Physical UI planning
- [README.md](README.md) - Project overview
- [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) - Implementation roadmap

**External Projects:**
- [ESPHome Vevor](https://github.com/zatakon/esphome-vevor-heater)
- [Protocol Documentation](https://github.com/zatakon/vevor_heater_control)
- [BluetoothHeater](https://gitlab.com/mrjones.id.au/bluetoothheater)

---

**Last Updated:** 2024
**Document Version:** 1.0
