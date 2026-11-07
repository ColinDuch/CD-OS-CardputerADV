#pragma once 

#include <stdint.h>
#include "esp_err.h"

#define ECRAN_LARGEUR 240
#define ECRAN_HAUTEUR 135

#define COULEUR_NOIR 0x0000
#define COULEUR_BLANC 0xFFFF
#define COULEUR_ROUGE 0xF800
#define COULEUR_VERT 0x07E0
#define COULEUR_BLEU 0x001F

esp_err_t ecran_init(void);
esp_err_t ecran_remplir(uint16_t couleur);