/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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
#include "tcp_server.h"


#define NOTCONN_PERIOD          pdMS_TO_TICKS(500)
#define CONN_PERIOD             pdMS_TO_TICKS(10000)
#define RECV_PERIOD             pdMS_TO_TICKS(50)

char test_buffer[128];

QueueHandle_t RotQueueHandler;               
TimerHandle_t LedTimerHandle;

gpio_num_t gpio_led_num = GPIO_NUM_2;       
int LedCounter = 0;     
unsigned char LedStatus = NOTCONNECTED;     
BaseType_t LedTimerStarted;

static const char *TAG = "example";

void Led_Init(void)
{
    gpio_set_direction(gpio_led_num, GPIO_MODE_OUTPUT);     
}

/*
@brief
    Rotator control task, only output a group of data in the same time.
    Recv one and output one.
    Real-Time control.
*/
static void rotator_controller(void *pvParameters)
{
    AntennaRot *rot_ptr;
    BaseType_t RXStatus;
    while (1)
    {
        if (uxQueueMessagesWaiting(RotQueueHandler) != 0)
        {
            rot_ptr = (AntennaRot *) malloc (sizeof(AntennaRot));
            RXStatus = xQueueReceive(RotQueueHandler, &rot_ptr, portMAX_DELAY);      // portMAX_DELAY is equal to 1, means the maximum waiting time
            if (RXStatus == pdPASS && rot_ptr != NULL)
            {
                LedStatus = RECVIVING;
                ESP_LOGI(TAG, "recv done, az: %f el: %f", rot_ptr->az, rot_ptr->el);
                free(rot_ptr);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            // Not receive the data, use delay to block the task, in order not to trigger the wdt
            else
            {
                ESP_LOGI(TAG, "didnt receive");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

        }
        else 
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

}

/*
@brief 
    Led timer callback function.
    When not connected, blink the led slow.
    When is connected, always blowing up.
    When is recviving the az and the el or the some other thing, blink the led really quick.

    prepare to add:
    Check the break up status, then reset the status of the led.
*/
static void LedTimerCallback(TimerHandle_t xTimer)
{
    LedCounter++;
    gpio_set_level(gpio_led_num, LedCounter % 2);
    if (LedStatus == CONNECTED)
    {
        LedCounter = 2;
    }
    else if (LedStatus == RECVIVING)
    {
        xTimerChangePeriod(xTimer, RECV_PERIOD, 0);
    }
    else
    {
        ;
    }
}

void app_main(void)
{
    Led_Init();     // Initialize function

    LedTimerHandle = xTimerCreate("led_controller", NOTCONN_PERIOD, pdTRUE, 0, LedTimerCallback);  // Create the led control timer.
    RotQueueHandler = xQueueCreate(5, sizeof(AntennaRot *));  // Create message queue

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());   // Creat a default event loop

    ESP_ERROR_CHECK(example_connect());  // Connect default AP
    
    if (RotQueueHandler != NULL && LedTimerHandle != NULL)
    {
        xTaskCreate(rotator_controller, "rotator_control", 4096, (void *)RotQueueHandler, 3, NULL);
        xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)RotQueueHandler, 5, NULL);

        LedStatus = CONNECTED;
        LedTimerStarted = xTimerStart(LedTimerHandle, 0);  // Start the timer
    }   
    
}
