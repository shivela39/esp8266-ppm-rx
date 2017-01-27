#ifndef PPM_H
#define PPM_H


// --- Include --- //
// --- ==== --- //


// --- Macros --- //
// Maps 0-65535 to 1000-2000.
#define UINT16_TO_CHANNEL_US(x) ((x) * (2000 - 1000) / (UINT16_MAX) + 1000)
// --- ==== --- //


// --- Function declarations --- //
/// Intialize and start the PPM system.
void ICACHE_FLASH_ATTR ppm_init(void);

/// Get whether failsafe is active.
bool ppm_get_failsafe(void);

/// Clear failsafe and reset timeout timer.
void ppm_reset_failsafe(void);

/// Enable failsafe immediately.
void ppm_force_failsafe(void);

/// Get a channel's current value.
uint16_t ppm_get_channel(int channel);

/// Set a channel to a value.
void ppm_set_channel(int channel, uint16_t value);
// --- ==== --- //


#endif