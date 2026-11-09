#pragma once 

#include "esp_err.h"

esp_err_t interface_init(void);
void interface_verrouiller(void);
void interface_deverrouiller(void);