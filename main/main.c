#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Bonjour depuis le Cardputer !");

    int compteur = 0;
    while (1) {
        if (compteur % 2 == 0) {
            ESP_LOGI(TAG, "Compteur : %d", compteur);
        } else {
            ESP_LOGW(TAG, "Compteur : %d", compteur);
        }
        compteur++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}