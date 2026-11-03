#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PIN_BOUTON GPIO_NUM_0

static const char *TAG = "main";

void app_main(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_BOUTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);

    int etat_precedent = gpio_get_level(PIN_BOUTON);

    while(1) {
        int etat = gpio_get_level(PIN_BOUTON); // donne 0 ou 1
        if (etat != etat_precedent){
            if (etat == 0) {
                ESP_LOGI(TAG, "Bouton pressé");
            }else {
                ESP_LOGI(TAG, "Bouton relaché");
            }
        }
        etat_precedent = etat;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}