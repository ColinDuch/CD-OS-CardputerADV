#pragma once

#include "esp_err.h"

typedef enum {
    EVT_BOUTON_APPUYE,   // valeur : nombre total d'appuis
    EVT_LUMINOSITE,      // valeur : nouvelle luminosité en %
    EVT_COUNT            // toujours en dernier : nombre de types d'événements
} evenement_id_t;

typedef struct {
    evenement_id_t id;
    int valeur;
} evenement_t;

typedef void (*bus_handler_t)(const evenement_t *evt);

esp_err_t bus_init(void);
esp_err_t bus_abonner(evenement_id_t id, bus_handler_t handler);
esp_err_t bus_publier(evenement_id_t id, int valeur);