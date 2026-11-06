#include "bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define MAX_ABONNES 16
#define TAILLE_FILE 10

typedef struct {
    evenement_id_t id;
    bus_handler_t handler;
} abonne_t;

static abonne_t s_abonnes[MAX_ABONNES];   // la liste des abonnés
static int s_nb_abonnes;                  // le nombre de cases utilisées
static QueueHandle_t s_file;              // la file des événements publiés

static void tache_distribution(void *arg)
{
    (void)arg;
    evenement_t evt;

    while (1) {
        xQueueReceive(s_file, &evt, portMAX_DELAY);
        for (int i = 0; i < s_nb_abonnes; i++) {
            if (s_abonnes[i].id == evt.id) {
                s_abonnes[i].handler(&evt);
            }
        }
    }
}

esp_err_t bus_init(void)
{
    s_file = xQueueCreate(TAILLE_FILE, sizeof(evenement_t));
    if (s_file == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(tache_distribution, "bus", 4096, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t bus_abonner(evenement_id_t id, bus_handler_t handler)
{
    if (s_nb_abonnes >= MAX_ABONNES) {
        return ESP_ERR_NO_MEM;
    }
    s_abonnes[s_nb_abonnes].id = id;
    s_abonnes[s_nb_abonnes].handler = handler;
    s_nb_abonnes++;
    return ESP_OK;
}

esp_err_t bus_publier(evenement_id_t id, int valeur)
{
    evenement_t evt = {
        .id = id,
        .valeur = valeur,
    };

    if (xQueueSend(s_file, &evt, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;

}