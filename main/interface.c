#include "interface.h"
#include "ecran.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdbool.h>

#define LIGNES_TAMPON 20

static SemaphoreHandle_t s_mutex_lvgl;
static QueueHandle_t s_file_touches;
static uint16_t s_tampon[ECRAN_LARGEUR * LIGNES_TAMPON] __attribute__((aligned(4)));

// l'horloge de LVGL : le temps écoulé depuis le démarrage, en millisecondes
static uint32_t horloge_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// appelée par LVGL quand une zone de l'écran est prête à être envoyée
static void envoyer_zone(lv_display_t *disp, const lv_area_t *zone, uint8_t *pixels)
{
    int largeur = zone->x2 - zone->x1 + 1;
    int hauteur = zone->y2 - zone->y1 + 1;

    // remettre les octets de chaque pixel dans l'ordre attendu par l'écran
    lv_draw_sw_rgb565_swap(pixels, largeur * hauteur);

    ecran_envoyer_pixels(zone->x1, zone->y1, zone->x2 + 1, zone->y2 + 1, (const uint16_t *)pixels);

    // prévenir LVGL que son tampon est de nouveau libre
    lv_display_flush_ready(disp);
}

// appelée par n'importe quelle tâche pour transmettre une touche à LVGL
void interface_envoyer_touche(uint32_t touche)
{
    xQueueSend(s_file_touches, &touche, 0);
}

// appelée régulièrement par LVGL : quelle touche est appuyée ?
static void lire_touches(lv_indev_t *clavier, lv_indev_data_t *donnees)
{
    (void)clavier;
    static uint32_t derniere_touche = 0;
    static bool relacher = false;

    donnees->key = derniere_touche;
    donnees->state = LV_INDEV_STATE_RELEASED;

    if (relacher) {
        relacher = false;
        return;
    }

    uint32_t touche;
    if (xQueueReceive(s_file_touches, &touche, 0) == pdTRUE) {
        derniere_touche = touche;
        donnees->key = touche;
        donnees->state = LV_INDEV_STATE_PRESSED;
        relacher = true;
    }
}

static void tache_lvgl(void *arg)
{
    (void)arg;
    while (1) {
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

    s_file_touches = xQueueCreate(5, sizeof(uint32_t));
    if (s_file_touches == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_init();
    lv_tick_set_cb(horloge_ms);

    lv_display_t *disp = lv_display_create(ECRAN_LARGEUR, ECRAN_HAUTEUR);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, s_tampon, NULL, sizeof(s_tampon), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, envoyer_zone);

    lv_group_t *groupe = lv_group_create();
    lv_group_set_default(groupe);

    lv_indev_t *clavier = lv_indev_create();
    lv_indev_set_type(clavier, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(clavier, lire_touches);
    lv_indev_set_group(clavier, groupe);

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
