//! RC receiver that outputs PPM.
//!
//! 16 bits per channel, default 8 channels.
//!
//! TODO:
//! - Make UART bridge for telemetry.
//! - Allow setting of a failsafe position as well as normal failsafe?
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

#include "fw_config.h"

#include "ppm.h"
#include "net.h"
// --- ==== --- //


// --- PPM --- //
// 16 bits per channel, big endian (I think).
void ICACHE_FLASH_ATTR net_callback_ppm(void *arg, char *data, unsigned short len)
{
	struct espconn *conn = (struct espconn *)arg;

	if (len == N_CHANNELS * sizeof(uint16_t))
	{
		ppm_reset_failsafe();

		for (int channel = 0; channel < N_CHANNELS; ++channel)
		{
			int channel_offset = channel * sizeof(uint16_t);
			uint16_t value = *(data + channel_offset) << 8 | *(data + channel_offset + 1);
			ppm_set_channel(channel, value);
		}
	}
}
// --- ==== --- //


// --- Serial bridge --- //
#define SERIAL_TX_BUFFER_LEN 512

static struct espconn *serial_conn = NULL;

static uint8_t serial_tx_buffer[SERIAL_TX_BUFFER_LEN];

static void ICACHE_FLASH_ATTR net_receive_callback_serial(void *arg, char *data, unsigned short len)
{
	struct espconn *conn = (struct espconn *)arg;
	if (conn == NULL) { return; }
	
	uart0_tx_buffer((uint8_t *)data, len);
}

static void ICACHE_FLASH_ATTR net_disconnect_callback_serial(void *arg)
{
	struct espconn *conn = (struct espconn *)arg;
	serial_conn = NULL;
}

static void ICACHE_FLASH_ATTR net_connect_callback_serial(void *arg)
{
	struct espconn *conn = (struct espconn *)arg;
	espconn_regist_recvcb(conn, net_receive_callback_serial);
	espconn_regist_disconcb(conn, net_disconnect_callback_serial);
	serial_conn = conn;
}

// ---

void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events) {
	if (events->sig == 0) {
		uint8_t rx_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) 
			& UART_RXFIFO_CNT;
		uint16_t buf_i = 0;
		
		for (uint8_t ii=0; ii < rx_len; ii++) {
			serial_tx_buffer[buf_i++] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		}
		
		if (serial_conn != NULL)
		{
			espconn_send(serial_conn, serial_tx_buffer, buf_i);
		}
		
		// ---

		// Clear the interrupt condition flags and re-enable the receive interrupt.
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
		uart_rx_intr_enable(UART0);
	}
}
// --- ==== --- //


// --- Main --- //
void ICACHE_FLASH_ATTR user_init(void)
{
#if OVERCLOCK
	system_update_cpu_freq(160);
#endif

	// I hope this disables debug output.
	system_set_os_print(0);

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	ppm_init();

	wifi_setup();

	new_udp_listener(PPM_PORT, net_callback_ppm);
	new_tcp_listener(SERIAL_BRIDGE_PORT, net_connect_callback_serial);
}
// --- ==== --- //
