#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "bus.h"

#define PIN_BOUTON GPIO_NUM_0
#define PIN_RETRO  GPIO_NUM_38

static const char *TAG = "main";

typedef struct {
    int luminosite;
    int nb_appuis;
} etat_systeme_t;

static etat_systeme_t s_etat;
static SemaphoreHandle_t s_mutex_etat;

static void tache_bouton(void *arg)
{
    (void)arg;
    int etat_precedent = gpio_get_level(PIN_BOUTON);
    int pourcentage = 0;

    while (1) {
        int etat = gpio_get_level(PIN_BOUTON);
        if (etat != etat_precedent) {
            if (etat == 0) {
                ESP_LOGI(TAG, "Bouton pressé");

                pourcentage = (pourcentage + 25) % 125;
                int valeur = pourcentage * 255 / 100;
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, valeur);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

                xSemaphoreTake(s_mutex_etat, portMAX_DELAY);
                s_etat.luminosite = pourcentage;
                s_etat.nb_appuis++;
                int nb_appuis_locale = s_etat.nb_appuis;
                xSemaphoreGive(s_mutex_etat);

                bus_publier(EVT_LUMINOSITE, pourcentage);
                bus_publier(EVT_BOUTON_APPUYE, nb_appuis_locale);

            } else {
                ESP_LOGI(TAG, "Bouton relâché");
            }
        }
        etat_precedent = etat;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void tache_battement(void *arg)
{
    (void)arg;
    int compteur = 0;
    while (1) {
        etat_systeme_t copie;
        xSemaphoreTake(s_mutex_etat, portMAX_DELAY);
        copie = s_etat;
        xSemaphoreGive(s_mutex_etat);
        ESP_LOGI(TAG, "Battement %d | luminosité : %d %% | appuis : %d", compteur, copie.luminosite, copie.nb_appuis);
        compteur++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void journal_luminosite(const evenement_t *evt)
{
    ESP_LOGI(TAG, "Nouvelle luminosité : %d %%", evt->valeur);
}

static void alerte_luminosite(const evenement_t *evt)
{
    if (evt->valeur == 100) {
        ESP_LOGW(TAG, "Luminosité au maximum !");
    }
}

static void journal_appui(const evenement_t * evt)
{
    ESP_LOGI(TAG, "Appui n° %d", evt->valeur);
}

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

    s_mutex_etat = xSemaphoreCreateMutex();

    ESP_ERROR_CHECK(bus_init());
    ESP_ERROR_CHECK(bus_abonner(EVT_LUMINOSITE, journal_luminosite));
    ESP_ERROR_CHECK(bus_abonner(EVT_LUMINOSITE, alerte_luminosite));
    ESP_ERROR_CHECK(bus_abonner(EVT_BOUTON_APPUYE, journal_appui));

    xTaskCreate(tache_bouton, "bouton", 4096, NULL, 5, NULL);
    xTaskCreate(tache_battement, "battement", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "Tâches créées, fin de app_main");
}