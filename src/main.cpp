#include <Arduino.h>

#include "driver/gpio.h"
#include "driver/twai.h"

//Initialize configuration structures using macro initializers
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NO_ACK);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();


void setup() {
    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }
}

void loop() {

    // Configure message to transmit
    twai_message_t message;
        // Message type and format settings
    message.extd = 1;              // Standard vs extended format
    message.rtr = 0;               // Data vs RTR frame
    message.ss = 0;                // Whether the message is single shot (i.e.; does not repeat on error)
    message.self = 0;              // Whether the message is a self reception request (loopback)
    message.dlc_non_comp = 0;      // DLC is less than 8
    // Message ID and payload
    message.identifier = 0xAAAA;
    message.data_length_code = 4;
    message.data[0] = 0;
    message.data[1] = 1;
    message.data[2] = 2;
    message.data[3] = 3;

    //Queue message for transmission
    if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
        printf("Message queued for transmission\n");
    } else {
        printf("Failed to queue message for transmission\n");
    }

    delay(1000);

    twai_status_info_t status_info;
    twai_get_status_info(&status_info);
    printf("state: %d\n", status_info.state);
}