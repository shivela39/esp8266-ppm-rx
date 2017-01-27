//! PPM output.
//!


// --- Include --- //
#include <stdint.h>
#include <limits.h>

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"

#include "driver/hw_timer.h"

#include "fw_config.h"
#include "ppm.h"
// --- ==== --- //


// --- Customizable constants --- //
// Logical level of the PPM pulses.
#define PPM_HIGH 1

// The length of the pulse terminating a frame.
#define FRAME_GAP_US 4000

// The length of a pulse between channels.
#define PULSE_US 300
// --- ==== --- //


// --- Other constants --- //
// If you change this, you also need to change the constant
// in the call to PIN_FUNC_SELECT in ppm_init().
#define PPM_GPIO 2 // TX1

#define PPM_LOW !PPM_HIGH // Logical level of channel width and frame gap.

#if OVERCLOCK
#define PPM_RESOLUTION_US 5
#else
#define PPM_RESOLUTION_US 10
#endif
// --- ==== --- //


// --- "Globals" --- //
// Channel values. Change these!
static uint16_t channels[N_CHANNELS] = {0};

// ---

// Updated with values from `channels` at the end of each PPM frame.
static uint16_t _channels_us[N_CHANNELS] = {0};

// Current channel we're outputting.
static int current_channel = 0;

// How long we've been outputting this channel, in microseconds.
static uint32_t channel_timer = 0;

// ---

// How long we've been going since last `ppm_reset_failsafe()`.
// If this is >= FAILSAFE_TIMEOUT_US, failsafe is active.
static uint32_t failsafe_timer = 0;
// --- ==== --- //


// --- Timer callback --- //
static bool gpio_level = 0;

// Abstraction to avoid writing to the GPIO registers unecessarily.
static inline void set_gpio_level(bool level)
{
	if (gpio_level != level)
	{
		gpio_level = level;
		GPIO_OUTPUT_SET(PPM_GPIO, level ? PPM_HIGH : PPM_LOW);
	}
}

// ---

static inline void update_channels(void)
{
	for (int i = 0; i < N_CHANNELS; ++i)
	{
		_channels_us[i] = UINT16_TO_CHANNEL_US(channels[i]);
	}
}

// ---

void hw_timer_callback(void)
{
	if (failsafe_timer >= FAILSAFE_TIMEOUT_US)
	{
		// Failsafe!
		set_gpio_level(false);

		current_channel = 0;
		channel_timer = 0;

		return;
	}

	// ---

	// Each channel has a 0.3ms pulse, followed by 0.7-1.7 ms.
	// The final channel is followed by another 0.3ms pulse, then the frame gap.
	// We can think of the frame gap as just a really long channel.

	if (channel_timer < PULSE_US) // Start-of-channel pulse.
	{
		set_gpio_level(true);
	}
	else
	{
		if (current_channel != N_CHANNELS + 1) // Normal channel.
		{
			if (channel_timer < _channels_us[current_channel])
			{
				// Waiting...
				set_gpio_level(false);
			}
			else // Next channel!
			{
				// Begin the pulse immediately.
				set_gpio_level(true);

				current_channel += 1;
				channel_timer = 0;
			}
		}
		else // Frame gap.
		{
			if (channel_timer < FRAME_GAP_US)
			{
				// We're in the frame gap.
				set_gpio_level(false);
			}
			else // Frame gap is over, new frame!
			{
				// Begin the pulse immediately.
				set_gpio_level(true);

				current_channel = 0;
				channel_timer = 0;

				update_channels();
			}
		}
	}

	failsafe_timer += PPM_RESOLUTION_US;
	channel_timer += PPM_RESOLUTION_US;

}
// --- ==== --- //


// --- Public interface --- //
void ICACHE_FLASH_ATTR ppm_init(void)
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_output_set(0, 0, (1 << PPM_GPIO), 0);

	hw_timer_init(FRC1_SOURCE, 1);
	hw_timer_set_func(hw_timer_callback);
	hw_timer_arm(PPM_RESOLUTION_US);
}

void ppm_reset_failsafe(void)
{
	failsafe_timer = 0;
}

void ppm_force_failsafe(void)
{
	failsafe_timer = FAILSAFE_TIMEOUT_US;
}

uint16_t ppm_get_channel(int channel)
{
	return channels[channel];
}

void ppm_set_channel(int channel, uint16_t value)
{
	channels[channel] = value;
}
// --- ==== --- //
