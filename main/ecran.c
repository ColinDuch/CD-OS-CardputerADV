#include "ecran.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define ECRAN_SPI SPI2_HOST
#define PIN_MOSI 35
#define PIN_SCLK 36
#define PIN_CS 37
#define PIN_DC 34
#define PIN_RST 33
#define ECRAN_DECALAGE_X 40
#define ECRAN_DECALAGE_Y 53

static const char *TAG = "ecran";
static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_fin_envoi;
static SemaphoreHandle_t s_mutex_ecran;

// appelée par le pilote, en interruption, quand un envoi de pixels est terminé
static bool fin_envoi(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *donnees, void *ctx) 
{
    (void)io;
    (void)donnees;
    (void)ctx;

    BaseType_t reveil = pdFALSE;
    xSemaphoreGiveFromISR(s_fin_envoi, &reveil);
    return reveil == pdTRUE;
}

esp_err_t ecran_init(void)
{
    s_fin_envoi = xSemaphoreCreateBinary();
    if (s_fin_envoi == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_mutex_ecran = xSemaphoreCreateMutex();
    if (s_mutex_ecran == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // 1. le bus SPI
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ECRAN_LARGEUR * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(ECRAN_SPI, &bus, SPI_DMA_CH_AUTO), TAG, "bus SPI");

    // 2. le panel IO : commandes et données, avec le fil DC(data/commands)
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = fin_envoi,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ECRAN_SPI, &io_cfg, &io), TAG, "panel IO");

    // 3. le pilote du contrôleur ST7789 (puce écran cardputer)
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel), TAG, "ST7789");

    // 4. la mise en route de l'écran 
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel, true), TAG, "inversion");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(s_panel, true), TAG, "rotation");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_panel, true, false), TAG, "miroir");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_panel, ECRAN_DECALAGE_X, ECRAN_DECALAGE_Y), TAG, "décalage");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "allumage");

    return ESP_OK;
}

// le ST7789 attend l'octet fort de chaque pixel en premier,
// alors l'esp32 range l'octet faible en premier dans la mémoire.
static uint16_t inverser_octets(uint16_t couleur)
{
    return (couleur >> 8) | (couleur << 8);
}

esp_err_t ecran_rectangle(int x, int y, int largeur, int hauteur, uint16_t couleur)
{
    static uint16_t ligne[ECRAN_LARGEUR];

    // 1. rogner la partie qui dépasse à gauche ou à droite
    if (x < 0) {
        largeur += x;
        x = 0;
    }

    if (x + largeur > ECRAN_LARGEUR) {
        largeur = ECRAN_LARGEUR - x;
    }

    if (y < 0) {
        hauteur += y;
        y = 0;
    }

    if (y + hauteur > ECRAN_HAUTEUR) {
        hauteur = ECRAN_HAUTEUR - y;
    }


    // 2. rien de visible : rien à dessiner
    if (largeur <= 0 || hauteur <= 0) {
        return ESP_OK;
    }

    xSemaphoreTake(s_mutex_ecran, portMAX_DELAY);

    for (int i = 0; i < largeur; i++) {
        ligne[i] = inverser_octets(couleur);
    }

    for (int j = 0; j < hauteur; j++) {
        esp_lcd_panel_draw_bitmap(s_panel, x, y + j, x + largeur, y + j + 1, ligne);
        xSemaphoreTake(s_fin_envoi, portMAX_DELAY);
    }

    xSemaphoreGive(s_mutex_ecran);

    return ESP_OK;
}

esp_err_t ecran_remplir(uint16_t couleur)
{
    return ecran_rectangle(0, 0, ECRAN_LARGEUR, ECRAN_HAUTEUR, couleur);
}