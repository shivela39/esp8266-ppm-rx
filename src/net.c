//! WiFi and UDP.
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
#define MAX_UDP_LISTENERS 2
// --- ==== --- //


// --- Types --- //
struct udp_listener {
	struct espconn conn;
	esp_udp proto;
};
// --- ==== --- //


// --- "Globals" --- //
int next_udp_listener;
struct udp_listener udp_listeners[MAX_UDP_LISTENERS];
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
	wifi_set_opmode_current(STATION_MODE);

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
// --- ==== --- //
