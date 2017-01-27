#ifndef PPM_H
#define PPM_H


// --- Include --- //
// --- ==== --- //


// --- Macros --- //
// Maps 0-65535 to 1000-2000.
#define UINT16_TO_CHANNEL_US(x) ((x) * (2000 - 1000) / (UINT16_MAX) + 1000)
// --- ==== --- //


// --- Function declarations --- //
void ICACHE_FLASH_ATTR ppm_init(void);
bool ppm_get_failsafe(void);
void ppm_reset_failsafe(void);
void ppm_force_failsafe(void);
void ppm_set_channel(int channel, uint16_t value);
// --- ==== --- //


#endif