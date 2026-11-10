#pragma once 

#include "esp_err.h"
#include <stdint.h>

void interface_envoyer_touche(uint32_t touche);

esp_err_t interface_init(void);
void interface_verrouiller(void);
void interface_deverrouiller(void);