//! WiFi, UDP and TCP.
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

#include "fw_config.h"
#include "../wifi_config.h"
// --- ==== --- //


// --- Constants --- //
#define MAX_UDP_LISTENERS 1
#define MAX_TCP_LISTENERS 1
// --- ==== --- //


// --- Types --- //
struct udp_listener {
	struct espconn conn;
	esp_udp proto;
};

struct tcp_listener {
	struct espconn conn;
	esp_tcp proto;
};
// --- ==== --- //


// --- "Globals" --- //
int next_udp_listener = 0;
struct udp_listener udp_listeners[MAX_UDP_LISTENERS];

int next_tcp_listener = 0;
struct tcp_listener tcp_listeners[MAX_TCP_LISTENERS];
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
// --- ==== --- //


// --- Public interface --- //
void ICACHE_FLASH_ATTR wifi_setup(void)
{
	wifi_set_opmode(STATION_MODE);

	struct station_config sc;
	strncpy((char *)sc.ssid, SSID, 32);
	strncpy((char *)sc.password, PASSWD, 64);
	sc.bssid_set = 0;

	wifi_station_set_config(&sc);
	wifi_station_dhcpc_start();

	wifi_set_event_handler_cb(wifi_event_callback);
}

bool ICACHE_FLASH_ATTR new_udp_listener(uint32_t port, espconn_recv_callback callback)
{
	if (next_udp_listener == MAX_UDP_LISTENERS)
	{
		return false;
	}

	int id = next_udp_listener++;

	udp_listeners[id].proto.local_port = port;

	udp_listeners[id].conn.type = ESPCONN_UDP;
	udp_listeners[id].conn.state = ESPCONN_NONE;
	udp_listeners[id].conn.proto.udp = &udp_listeners[id].proto;

	espconn_create(&udp_listeners[id].conn);
	espconn_regist_recvcb(&udp_listeners[id].conn, callback);

	return true;
}

bool ICACHE_FLASH_ATTR new_tcp_listener(uint32_t port,
	espconn_connect_callback connect_callback)
{
	if (next_tcp_listener == MAX_TCP_LISTENERS)
	{
		return false;
	}

	int id = next_tcp_listener++;

	tcp_listeners[id].proto.local_port = port;

	tcp_listeners[id].conn.type = ESPCONN_TCP;
	tcp_listeners[id].conn.state = ESPCONN_NONE;
	tcp_listeners[id].conn.proto.tcp = &tcp_listeners[id].proto;
	
	espconn_regist_connectcb(&tcp_listeners[id].conn, connect_callback);
	espconn_accept(&tcp_listeners[id].conn);

	return true;
}
// --- ==== --- //
