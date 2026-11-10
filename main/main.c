#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "bus.h"
#include "ecran.h"
#include "interface.h"
#include "lvgl.h"

#define PIN_BOUTON GPIO_NUM_0
#define PIN_RETRO  GPIO_NUM_38

#define PAS_LUMINOSITE 5
#define LUMINOSITE_DEPART 10
#define BARRE_LARGEUR 200
#define BARRE_HAUTEUR 20
#define APPUI_LONG_MS 500

static const char *TAG = "main";

typedef struct {
    int luminosite;
    int nb_appuis;
} etat_systeme_t;

static etat_systeme_t s_etat;
static SemaphoreHandle_t s_mutex_etat;
static lv_obj_t *s_barre;
static lv_obj_t *s_texte_luminosite;
static int s_luminosite = LUMINOSITE_DEPART;

static void tache_bouton(void *arg)
{
    (void)arg;
    int etat_precedent = gpio_get_level(PIN_BOUTON);
    TickType_t debut_appui = 0;

    while (1) {
        int etat = gpio_get_level(PIN_BOUTON);
        if (etat != etat_precedent) {
            if (etat == 0) {
                debut_appui = xTaskGetTickCount();
            } else {
                TickType_t duree_ticks = xTaskGetTickCount() - debut_appui;
                uint32_t duree_ms = duree_ticks * portTICK_PERIOD_MS;

                if (duree_ms >= APPUI_LONG_MS) {
                    interface_envoyer_touche(LV_KEY_ENTER);
                } else {
                    interface_envoyer_touche(LV_KEY_NEXT);
                }

                // compter l'appui, puis l'annoncer sur le bus
                xSemaphoreTake(s_mutex_etat, portMAX_DELAY);
                s_etat.nb_appuis++;
                int nb_appuis_locale = s_etat.nb_appuis;
                xSemaphoreGive(s_mutex_etat);

                bus_publier(EVT_BOUTON_APPUYE, nb_appuis_locale);
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

static void journal_appui(const evenement_t *evt)
{
    ESP_LOGI(TAG, "Appui n° %d", evt->valeur);
}

static void afficher_luminosite(const evenement_t *evt)
{
    interface_verrouiller();
    lv_bar_set_value(s_barre, evt->valeur, LV_ANIM_ON);
    lv_label_set_text_fmt(s_texte_luminosite, "%d %%", evt->valeur);
    interface_deverrouiller();
}

// applique une luminosité : rétroéclairage, état partagé, et événement sur le bus
static void regler_luminosite(int pourcentage)
{
    int valeur = pourcentage * 255 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, valeur);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    xSemaphoreTake(s_mutex_etat, portMAX_DELAY);
    s_etat.luminosite = pourcentage;
    xSemaphoreGive(s_mutex_etat);

    bus_publier(EVT_LUMINOSITE, pourcentage);
}

static void clic_moins(lv_event_t *e)
{
    (void)e;
    s_luminosite -= PAS_LUMINOSITE;
    if (s_luminosite < 0) {
        s_luminosite = 0;
    }
    regler_luminosite(s_luminosite);
}

static void clic_plus(lv_event_t *e)
{
    (void)e;
    s_luminosite += PAS_LUMINOSITE;
    if (s_luminosite > 100) {
        s_luminosite = 100;
    }
    regler_luminosite(s_luminosite);
}

// crée tous les objets de l'interface, à appeler en tenant le verrou de LVGL
static void creer_interface(void)
{
    lv_obj_t *fond = lv_screen_active();
    lv_obj_set_style_bg_color(fond, lv_color_black(), 0);

    // le titre
    lv_obj_t *titre = lv_label_create(fond);
    lv_label_set_text(titre, "CD-OS");
    lv_obj_set_style_text_color(titre, lv_color_white(), 0);
    lv_obj_align(titre, LV_ALIGN_TOP_MID, 0, 10);

    // la barre de luminosité
    s_barre = lv_bar_create(fond);
    lv_obj_set_size(s_barre, BARRE_LARGEUR, BARRE_HAUTEUR);
    lv_obj_align(s_barre, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_bar_set_range(s_barre, 0, 100);
    lv_bar_set_value(s_barre, LUMINOSITE_DEPART, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_barre, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

    // le pourcentage, au-dessus de la barre
    s_texte_luminosite = lv_label_create(fond);
    lv_obj_set_style_text_color(s_texte_luminosite, lv_color_white(), 0);
    lv_obj_set_width(s_texte_luminosite, BARRE_LARGEUR);
    lv_obj_set_style_text_align(s_texte_luminosite, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(s_texte_luminosite, s_barre, LV_ALIGN_OUT_TOP_MID, 0, -5);
    lv_label_set_text_fmt(s_texte_luminosite, "%d %%", LUMINOSITE_DEPART);

    // le bouton "-", à gauche
    lv_obj_t *bouton_moins = lv_button_create(fond);
    lv_obj_set_size(bouton_moins, 50, 30);
    lv_obj_align(bouton_moins, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_add_event_cb(bouton_moins, clic_moins, LV_EVENT_CLICKED, NULL);

    lv_obj_t *texte_moins = lv_label_create(bouton_moins);
    lv_label_set_text(texte_moins, "-");
    lv_obj_center(texte_moins);

    // le bouton "+", à droite
    lv_obj_t *bouton_plus = lv_button_create(fond);
    lv_obj_set_size(bouton_plus, 50, 30);
    lv_obj_align(bouton_plus, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(bouton_plus, clic_plus, LV_EVENT_CLICKED, NULL);

    lv_obj_t *texte_plus = lv_label_create(bouton_plus);
    lv_label_set_text(texte_plus, "+");
    lv_obj_center(texte_plus);
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
        .duty = LUMINOSITE_DEPART * 255 / 100,
        .hpoint = 0,
    };
    ledc_channel_config(&canal);

    // état partagé et bus : créés avant toutes les tâches qui les utilisent
    s_etat.luminosite = LUMINOSITE_DEPART;
    s_mutex_etat = xSemaphoreCreateMutex();
    if (s_mutex_etat == NULL) {
        ESP_LOGE(TAG, "Impossible de créer le mutex de l'état");
        return;
    }
    ESP_ERROR_CHECK(bus_init());

    // écran et interface
    ESP_ERROR_CHECK(ecran_init());
    ESP_ERROR_CHECK(interface_init());

    interface_verrouiller();
    creer_interface();
    interface_deverrouiller();

    // abonnements, avant la création des tâches qui publient
    ESP_ERROR_CHECK(bus_abonner(EVT_LUMINOSITE, journal_luminosite));
    ESP_ERROR_CHECK(bus_abonner(EVT_LUMINOSITE, alerte_luminosite));
    ESP_ERROR_CHECK(bus_abonner(EVT_BOUTON_APPUYE, journal_appui));
    ESP_ERROR_CHECK(bus_abonner(EVT_LUMINOSITE, afficher_luminosite));

    xTaskCreate(tache_bouton, "bouton", 4096, NULL, 5, NULL);
    xTaskCreate(tache_battement, "battement", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "Tâches créées, fin de app_main");
}
