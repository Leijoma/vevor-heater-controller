# Vevor Heater Controller

ESP32 / PlatformIO projects for reverse-engineering and controlling a Vevor diesel heater.

> ## ⚠️ Disclaimer: use at your own risk
>
> - This project is provided **"as is", without warranty of any kind**. **Use it entirely at your own risk.** The authors accept no liability for damage, injury, fire, carbon monoxide poisoning or any other loss.
> - The **hardware setup is a test / prototype setup**, built on breadboards for experimentation. It is **not a finished product** and **lacks many safety features** (no independent over-temperature or flame monitoring, no watchdog that shuts the heater off if the controller hangs, no fuse or isolation by default, no tested enclosure, weak network security).
> - The heater protocol was **reverse-engineered** and may behave differently on other models or firmware versions.
> - Never leave a fuel-burning heater running unattended, always ensure ventilation, install a CO alarm, and keep the heater's original controller available as a way to turn it off.
>
> See [Safety and limitations](Vevor%20remote%20control/HARDWARE_SETUP.md#safety-and-limitations) for details.

| Project | Purpose |
|---|---|
| [Vevor remote control](Vevor%20remote%20control/) | Main controller: heater library, OLED + rotary encoder UI, WiFi web interface, REST API, OTA |
| [Vevor-Serial-Sniffer](Vevor-Serial-Sniffer/) | Passive serial sniffer used to decode the protocol |
| [Vevor-RS485-Sniffer](Vevor-RS485-Sniffer/) | RS485 variant of the sniffer |
| [Vevor-Serial-Transmitter](Vevor-Serial-Transmitter/) | Transmit-side experiments |
| [RS485-TX-Test](RS485-TX-Test/) | RS485 transmit test |
| [WCS1800-CurrentSensor-Test](WCS1800-CurrentSensor-Test/) | Current sensor test |

See [SOLUTION_SUMMARY.md](SOLUTION_SUMMARY.md) and [SHARE_README.md](SHARE_README.md) for an overview.

Licensed under the [MIT License](LICENSE).
