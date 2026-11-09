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
#define COULEUR_GRIS 0x7BEF

esp_err_t ecran_init(void);
esp_err_t ecran_remplir(uint16_t couleur);
esp_err_t ecran_rectangle(int x, int y, int largeur, int hauteur, uint16_t couleur);
esp_err_t ecran_caractere(int x, int y, char c, uint16_t couleur, uint16_t fond, int echelle);
esp_err_t ecran_texte(int x, int y, const char *texte, uint16_t couleur, uint16_t fond, int echelle);
esp_err_t ecran_envoyer_pixels(int x_debut, int y_debut, int x_fin, int y_fin, const uint16_t *pixels);