#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PIN_BOUTON GPIO_NUM_0
#define PIN_RETRO  GPIO_NUM_38

static const char *TAG = "main";

// NOUVEAU : la tâche du bouton, avec la boucle qui était dans app_main
static void tache_bouton(void *arg)
{
    int etat_precedent = gpio_get_level(PIN_BOUTON);
    int pourcentage = 0;

    while (1) {
        int etat = gpio_get_level(PIN_BOUTON);
        if (etat != etat_precedent) {
            if (etat == 0) {
                ESP_LOGI(TAG, "Bouton pressé");

                pourcentage = (pourcentage + 25) % 125;
                int valeur = pourcentage * 255 / 100;

                ESP_LOGI(TAG, "Luminosité : %d %% (valeur %d)", pourcentage, valeur);
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, valeur);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            } else {
                ESP_LOGI(TAG, "Bouton relâché");
            }
        }
        etat_precedent = etat;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// TODO 1 : écrire la tâche "battement", qui affiche un message chaque seconde
//          avec un compteur (comme à l'étape 1)
static void tache_battement(void *arg)
{
    int compteur = 0;
    while(1) {
        ESP_LOGI(TAG, "Compteur : %d", compteur);
        compteur ++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Configuration du matériel (inchangée)
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

    // NOUVEAU : création des tâches
    xTaskCreate(tache_bouton, "bouton", 4096, NULL, 5, NULL);
    // TODO 2 : créer la tâche "battement", avec une priorité plus basse que le bouton
    xTaskCreate(tache_battement, "battement", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "Tâches créées, fin de app_main");
}