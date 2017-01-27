# esp8266-ppm-rx
Use an ESP8266 module as a receiver for your drone!

This firmware has two main features: UDP-to-PPM for remote control, and a serial bridge for telemetry.

WARNING: It is quite possible that this firmware has fatal bugs in it! I can't guarantee the safety of whatever you use this on. Be careful!


## Configuration

- Add `wifi_config.h` to the main project directory with your WiFi details:

```
#define SSID "your_network_ssid"
#define PASSWD "your_network_password"
```

- Check out `src/fw_config.h` for some configuration options.
- PPM timing details are in `src/ppm.c` You probably don't need to change them, but they're there.


## Building

1. Build [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk) at commit `e8d757b` with `STANDALONE=y` (which is the default).
2. Go into the `esp8266-ppm-rx` directory and `make`.


## Flashing

1. Edit `ESPTOOL_PORT` and optionally the other `ESPTOOL_*` variables in the Makefile as required. If you're using esptool v1.x (as is bundled with esp-open-sdk at that commit), comment or remove the line `ESPTOOL_FLAGS ?= --before no_reset --after soft_reset`.
2. Make sure your ESP8266 module is connected via serial and is in flash mode, then `make flash`.


## How to use

### PPM
By default, the firmware listens on UDP port 5620 for PPM channel data. Each channel's value is represented by two bytes (16 bits) big endian, so a PPM update is a single packet of `N_CHANNELS * 2` bytes, with channel one's value being the first two bytes, channel two the next two bytes, and so on.

### Serial bridge
By default, the firmware listens on TCP port 5621. The connection is just a completely transparent serial bridge at 115200 baud. I use this for remote telemetry.


## License
Released into the public domain via CC0 (see `COPYING` for the license text) EXCEPT for: 

- Files in `src/driver`, these are from the Espressif NONOS SDK (although slightly modified).
- `blank.bin` and `esp_init_data_default.bin`, also from the Espressif NONOS SDK.

Check the licenses for these files before using them. The source files mostly have their license at the top.
