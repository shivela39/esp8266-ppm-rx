#ifndef FW_CONFIG_H
#define FW_CONFIG_H

// OVERCLOCK = 0 - ESP8266 clocked at 80 MHz, PPM resolution is 10us
// OVERCLOCK = 1 - ESP8266 clocked at 160 MHz, PPM resolution is 5us
#define OVERCLOCK 0

// The number of channels to output.
#define N_CHANNELS 8

// The UDP port to listen on for channel updates.
#define PPM_PORT 5620

// Switch to failsafe after this many microseconds of not receiving a valid message.
// Call `ppm_reset_failsafe()` to reset the timer and cancel failsafe.
#define FAILSAFE_TIMEOUT_US (500 * 1000)

#endif