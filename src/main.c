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

#include "fw_config.h"

#include "ppm.h"
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
	uint8_t *addr_array = conn->proto.udp->remote_ip;
}

static void ICACHE_FLASH_ATTR ppm_net_setup(uint32_t port)
{
	ppm_udp_proto.local_port = port;
	
	ppm_udp_conn.type = ESPCONN_UDP;
	ppm_udp_conn.state = ESPCONN_NONE;
	ppm_udp_conn.proto.udp = &ppm_udp_proto;
	
	espconn_create(&ppm_udp_conn);
	espconn_regist_recvcb(&ppm_udp_conn, ppm_net_receive_callback);
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
}
// --- ==== --- //


// --- Main --- //
void ICACHE_FLASH_ATTR user_init(void)
{
#if OVERCLOCK
	system_update_cpu_freq(160);
#endif

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	ppm_init();

	wifi_setup();
	ppm_net_setup(4460);
}
// --- ==== --- //


// --- UART --- //
void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events) {
	if (events->sig == 0) {
		// Sig 0 is a normal receive. Get how many bytes have been received.
		uint8_t rx_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;

		uint8_t rx_char;
		for (uint8_t ii=0; ii < rx_len; ii++) {
			rx_char = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
			
			if (rx_char == 's')
			{
				uint16_t value;
				os_printf("Channels:\n");
				for (int i = 0; i < N_CHANNELS; ++i)
				{
					value = ppm_get_channel(i);
					os_printf("  %d = %d (%d us)\n", i, value, UINT16_TO_CHANNEL_US(value));
				}
			}
			else if (rx_char == 'r')
			{
				const int i = 2;
				
				uint16_t new_value = os_random() / (ULONG_MAX/UINT16_MAX);
				
				ppm_set_channel(i, new_value);
				os_printf("Changed channel %d to %d (%d us)\n", i, 
					new_value, UINT16_TO_CHANNEL_US(new_value));
			}
			else if (rx_char == 'f')
			{
				bool failsafe = !ppm_get_failsafe();
				ppm_set_failsafe(!failsafe);
				os_printf("Failsafe is %s\n", failsafe ? "on" : "off");
			}
		}

		// Clear the interrupt condition flags and re-enable the receive interrupt.
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
		uart_rx_intr_enable(UART0);
	}
}
// --- ==== --- //
