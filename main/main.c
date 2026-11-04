#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

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

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 256,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t canal = {
        .gpio_num = PIN_RETRO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&canal);

    int etat_precedent = gpio_get_level(PIN_BOUTON);
    int pourcentage = 0;

    while(1) {
        int etat = gpio_get_level(PIN_BOUTON); // donne 0 ou 1
        if (etat != etat_precedent){
            if (etat == 0) {
                ESP_LOGI(TAG, "Bouton pressé");
                pourcentage = (pourcentage + 25) % 125;     // pas de 25 %, 0, 25, 50, 75, 100 puis de nouveau 0
                int valeur = pourcentage * 255 / 100;   // conversion pour la luminosité 
                ESP_LOGI(TAG, "Luminosité est : %d %% (valeur %d)", pourcentage, valeur);
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, valeur);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            }else {
                ESP_LOGI(TAG, "Bouton relaché");
            }
        }
        etat_precedent = etat;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}