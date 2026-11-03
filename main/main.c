#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdbool.h>

#define PIN_BOUTON GPIO_NUM_0
#define PIN_RETRO GPIO_NUM_38

static const char *TAG = "main";

void app_main(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_BOUTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cfg);

    gpio_config_t cfg_bl = {
        .pin_bit_mask = 1ULL << PIN_RETRO,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&cfg_bl);

    bool allume = false;
    gpio_set_level(PIN_RETRO, allume);

    int etat_precedent = gpio_get_level(PIN_BOUTON);

    gpio_set_level(PIN_RETRO, 0);

    while(1) {
        int etat = gpio_get_level(PIN_BOUTON); // donne 0 ou 1
        if (etat != etat_precedent){
            if (etat == 0) {
                ESP_LOGI(TAG, "Bouton pressé");
                allume = !allume;
                gpio_set_level(PIN_RETRO, allume);
                ESP_LOGI(TAG, "Rétroéclairage %s", allume ? "allumé" : "éteint");
            }else {
                ESP_LOGI(TAG, "Bouton relaché");
            }
        }
        etat_precedent = etat;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}