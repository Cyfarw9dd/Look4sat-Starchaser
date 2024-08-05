#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/gpio.h"


#define NOT_TRACKING_FARSIDE    false       // TODO: When running an actual antenna, be sure that this marco is TRUE
#define DELTA_VALUE             0.01


#define PORT                CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE      CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL  CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT     CONFIG_EXAMPLE_KEEPALIVE_COUNT

typedef struct
{
    float az; // Azimuth    // TODO: need to transform the angle to a pluse
    float el; // Elevation
}AntennaRot;

typedef enum
{
    NOTCONNECTED = 1,
    CONNECTED,
    RECVIVING,
}Connect_Status;

int reconnect(int *sock, int addr_family, int ip_protocol, struct sockaddr_storage *dest_addr); 

void do_retransmit(int *sock, int addr_family, int ip_protocol, struct sockaddr_storage *dest_addr); 

void tcp_server_task(void *pvParameters);

