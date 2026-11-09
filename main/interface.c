#include "interface.h"
#include "ecran.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define LIGNES_TAMPON 20

static SemaphoreHandle_t s_mutex_lvgl;
static uint16_t s_tampon[ECRAN_LARGEUR * LIGNES_TAMPON] __attribute__((aligned(4)));

// l'interface de LVGL : le temps écoulé depuis le démarrage, en millisecondes
static uint32_t horloge_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// appelée par LVGl quand une zone de l'écran est prête à être envoyée
static void envoyer_zone(lv_display_t *disp, const lv_area_t *zone, uint8_t *pixels)
{
    int largeur = zone->x2 - zone->x1 + 1;
    int hauteur = zone->y2 - zone->y1 + 1;

    // remettre les octets de chaque pixel dans l'odre attendu par l'écran 
    lv_draw_sw_rgb565_swap(pixels, largeur * hauteur);

    ecran_envoyer_pixels(zone->x1, zone->y1, zone->x2 +1, zone->y2 +1, (const uint16_t *)pixels);

    // prévenir LVGL que son tampon est de nouveau libre
    lv_display_flush_ready(disp);
}

static void tache_lvgl(void *arg)
{
    (void)arg;
    while(1) {
        interface_verrouiller();
        lv_timer_handler();
        interface_deverrouiller();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t interface_init(void) 
{
    s_mutex_lvgl = xSemaphoreCreateMutex();
    if (s_mutex_lvgl == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_init();
    lv_tick_set_cb(horloge_ms);

    lv_display_t *disp = lv_display_create(ECRAN_LARGEUR, ECRAN_HAUTEUR);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_tampon, NULL, sizeof(s_tampon), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, envoyer_zone);

    if (xTaskCreate(tache_lvgl, "lvgl", 8192, NULL, 3, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void interface_verrouiller(void)
{
    xSemaphoreTake(s_mutex_lvgl, portMAX_DELAY);
}

void interface_deverrouiller(void) 
{
    xSemaphoreGive(s_mutex_lvgl);
}