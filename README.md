# Vevor Heater Controller

ESP32 / PlatformIO projects for reverse-engineering and controlling a Vevor diesel heater.

| Project | Purpose |
|---|---|
| [Vevor remote control](Vevor%20remote%20control/) | Main controller: heater library, OLED + rotary encoder UI, WiFi web interface, REST API, OTA |
| [Vevor-Serial-Sniffer](Vevor-Serial-Sniffer/) | Passive serial sniffer used to decode the protocol |
| [Vevor-RS485-Sniffer](Vevor-RS485-Sniffer/) | RS485 variant of the sniffer |
| [Vevor-Serial-Transmitter](Vevor-Serial-Transmitter/) | Transmit-side experiments |
| [RS485-TX-Test](RS485-TX-Test/) | RS485 transmit test |
| [WCS1800-CurrentSensor-Test](WCS1800-CurrentSensor-Test/) | Current sensor test |

See [SOLUTION_SUMMARY.md](SOLUTION_SUMMARY.md) and [SHARE_README.md](SHARE_README.md) for an overview.

> **Warning:** This controls a fuel-burning heater. Use at your own risk. Change the default web/OTA PIN (`1234`) before exposing the device on a network.

Licensed under the [MIT License](LICENSE).
