#ifndef NET_H
#define NET_H


// --- Function declarations --- //
void ICACHE_FLASH_ATTR wifi_setup(void);
bool ICACHE_FLASH_ATTR new_udp_listener(uint32_t port, espconn_recv_callback callback);
// --- ==== --- //


#endif