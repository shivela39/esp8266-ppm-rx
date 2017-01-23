//! RC receiver that outputs PPM.
//!
//! 16 bits per channel, default 8 channels.
//!
//! TODO: WiFi. AP or STA?
//! Modularize, make UDP<->UART module?
//!


// --- Include --- //
#include <stdint.h>
#include <limits.h>

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"

#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"

#include "espmissingincludes.h"

#include "driver/uart.h"
#include "driver/hw_timer.h"
// --- ==== --- //


// --- Customizable constants --- //
// OVERCLOCK = 0 - ESP8266 clocked at 80 MHz, PPM resolution is 10us
// OVERCLOCK = 1 - ESP8266 clocked at 160 MHz, PPM resolution is 5us
#define OVERCLOCK 0

// The number of channels to output.
#define N_CHANNELS 8

// ---

// Logical level of the PPM pulses.
#define PPM_HIGH 1

// The length of the pulse terminating a frame.
#define FRAME_GAP_US 4000

// The length of a pulse between channels.
#define PULSE_US 300
// --- ==== --- //


// --- Other constants --- //
// If you change this, you also need to change the constant
// in the call to PIN_FUNC_SELECT in user_main().
#define PPM_GPIO 2 // TX1

#define PPM_LOW !PPM_HIGH // Logical level of channel width and frame gap.

#if OVERCLOCK
#define PPM_RESOLUTION_US 5
#else
#define PPM_RESOLUTION_US 10
#endif
// --- ==== --- //


// --- Function-like macros --- //
// Maps 0-65535 to 1000-2000.
#define UINT16_TO_CHANNEL_US(x) ((x) * (2000 - 1000) / (UINT16_MAX) + 1000)
// --- ==== --- //


// --- Globals --- //
// If false, send PPM as normal.
// If true, PPM pin is kept low to indicate loss of transmitter signal.
static bool failsafe = false;
// --- ==== --- //


// --- PPM --- //
// Channel values. Change these!
static uint16_t channels[N_CHANNELS] = {0};

// Updated with values from `channels` at the end of each PPM frame.
static uint16_t _channels_us[N_CHANNELS] = {0};

// Current channel we're outputting.
static int ppm_current_channel = 0;

// How long we've been outputting this channel, in microseconds.
static uint32_t ppm_channel_timer = 0;

// ---

static bool ppm_gpio_level = 0;

// Abstraction to avoid writing to the GPIO registers unecessarily.
static inline void set_ppm_gpio_level(bool level)
{
	if (ppm_gpio_level != level)
	{
		ppm_gpio_level = level;
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

void hw_timer_callback(void)
{
	if (failsafe)
	{
		set_ppm_gpio_level(false);
		ppm_current_channel = 0;
		ppm_channel_timer = 0;
		return;
	}
	
	// ---
	
	// Each channel has a 0.3ms pulse, followed by 0.7-1.7 ms.
	// The final channel is followed by another 0.3ms pulse, then the frame gap.
	// We can think of the frame gap as just a really long channel.

	if (ppm_channel_timer < PULSE_US) // Start-of-channel pulse.
	{
		set_ppm_gpio_level(true);
	}
	else // Channel wait (or frame gap).
	{
		if (ppm_current_channel == N_CHANNELS + 1) // Frame gap.
		{
			// We add PULSE_US because the frame gap is the part
			// after the pulse.
			if (ppm_channel_timer < PULSE_US + FRAME_GAP_US)
			{
				// We're in the frame gap.
				set_ppm_gpio_level(false);
			}
			else // Frame gap is over, new frame!
			{
				// Begin the pulse immediately.
				set_ppm_gpio_level(true);

				ppm_current_channel = 0;
				ppm_channel_timer = 0;

				update_channels();
			}
		}
		else // Normal channel.
		{
			if (ppm_channel_timer < _channels_us[ppm_current_channel])
			{
				// Waiting...
				set_ppm_gpio_level(false);
			}
			else // Next channel!
			{
				// Begin the pulse immediately.
				set_ppm_gpio_level(true);

				ppm_current_channel += 1;
				ppm_channel_timer = 0;
			}
		}
	}

	ppm_channel_timer += PPM_RESOLUTION_US;
}
// --- ==== --- //


// --- WiFi --- //
static void ICACHE_FLASH_ATTR wifi_event_callback(System_Event_t *event) {
	switch (event->event) {
		case EVENT_STAMODE_CONNECTED: {
		} break;

		case EVENT_STAMODE_DISCONNECTED: {
		} break;

		case EVENT_STAMODE_GOT_IP: {
		} break;

		case EVENT_STAMODE_DHCP_TIMEOUT: {
			wifi_station_disconnect();
			wifi_station_connect();
		} break;
	}
}

// ---

static struct espconn ppm_udp_conn;
static esp_udp ppm_udp_proto;

static void ICACHE_FLASH_ATTR ppm_net_receive_callback(void *arg, char *data, uint16_t len)
{
	struct espconn *conn = (struct espconn *)arg;
}

static void ICACHE_FLASH_ATTR ppm_net_setup(uint32_t port)
{
	ppm_udp_proto.local_port = port;
	
	ppm_udp_conn.type = ESPCONN_UDP;
	ppm_udp_conn.state = ESPCONN_NONE;
	ppm_udp_conn.proto.udp = ppm_udp_proto;
	
	espconn_create(&ppm_udp_conn);
	espconn_regist_recvcb(&udp_conn, recv_cb);
}

// ---

static void ICACHE_FLASH_ATTR udp_receive_callback(void *arg, char *data, uint16_t len) {
	struct espconn *conn = (struct espconn *)arg;
	uint8_t *addr_array = conn->proto.udp->remote_ip;
}

// ---

static void ICACHE_FLASH_ATTR wifi_setup(void)
{
	wifi_set_opmode_current(STATION_MODE);

	struct station_config sc;
	strncpy((char *)sc.ssid, SSID, 32);
	strncpy((char *)sc.password, PASSWD, 64);
	sc.bssid_set = 0;

	wifi_station_set_config(&sc);
	wifi_station_dhcpc_start();

	wifi_set_event_handler_cb(wifi_event_callback);

	// ---

	udp_proto.local_port = 2345;
	udp_conn.type = ESPCONN_UDP;
	udp_conn.state = ESPCONN_NONE;
	udp_conn.proto.udp = &udp_proto;
	espconn_create(&udp_conn);
	espconn_regist_recvcb(&udp_conn, udp_receive_callback);
}
// --- ==== --- //


// --- Main --- //
void ICACHE_FLASH_ATTR user_init(void)
{
#if OVERCLOCK
	system_update_cpu_freq(160);
#endif

	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_output_set(0, 0, (1 << PPM_GPIO), 0);

	//wifi_setup();
	//wifi_set_opmode_current(NULL_MODE);

	hw_timer_init(FRC1_SOURCE, 1);
	hw_timer_set_func(hw_timer_callback);
	hw_timer_arm(PPM_RESOLUTION_US);
}
// --- ==== --- //


// --- UART --- //
void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events) {
	if (events->sig == 0) {
		// Sig 0 is a normal receive. Get how many bytes have been received.
		uint8_t rx_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;

		// Parse the characters, taking any digits as the new timer interval.
		uint8_t rx_char;
		for (uint8_t ii=0; ii < rx_len; ii++) {
			rx_char = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
			
			if (rx_char == 's')
			{
				os_printf("Channels:\n");
				for (int i = 0; i < N_CHANNELS; ++i)
				{
					os_printf("  %d = %d (%d us)\n", i, channels[i], UINT16_TO_CHANNEL_US(channels[i]));
				}
			}
			else if (rx_char == 'r')
			{
				const int i = 2;
				unsigned long new_value_raw = os_random();
				channels[i] = new_value_raw / (ULONG_MAX/UINT16_MAX);
				os_printf("Changed channel %d to %d (%d us)\n", i,  channels[i], 
					UINT16_TO_CHANNEL_US(channels[i]));
			}
			else if (rx_char == 'f')
			{
				failsafe = !failsafe;
				os_printf("Failsafe is %s\n", failsafe ? "on" : "off");
			}
		}

		// Clear the interrupt condition flags and re-enable the receive interrupt.
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
		uart_rx_intr_enable(UART0);
	}
}
// --- ==== --- //
