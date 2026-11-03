# CD-OS

Un système d’exploitation fait maison pour appareils à base d’ESP32-S3, écrit en C avec ESP-IDF et FreeRTOS.

Le projet est construit étape par étape, dans un but d’apprentissage : chaque brique (tâches, événements, pilotes, interface) est écrite et comprise avant de passer à la suivante.

## Matériel

| Appareil | Rôle | État |
|---|---|---|
| M5Stack Cardputer ADV | Plateforme de développement | En cours |
| LILYGO T-Watch Ultra | Cible finale (montre) | Plus tard |

Les deux utilisent un ESP32-S3 : le code non lié au matériel sera commun aux deux.

## Prérequis

- ESP-IDF v5.4, installé dans `~/esp/esp-idf`
- Un câble USB-C qui transmet les données (pas seulement la charge)
- Sous Arch : être dans le groupe `uucp` pour accéder à `/dev/ttyACM0`

Charger l’environnement ESP-IDF dans chaque nouveau terminal :

```
get_idf
```

(alias défini dans `~/.bashrc` : `alias get_idf='. $HOME/esp/esp-idf/export.sh'`)

## Compiler et lancer

```
idf.py set-target esp32s3          # une seule fois, à la création du projet
idf.py build                       # compiler
idf.py -p /dev/ttyACM0 flash monitor   # flasher puis afficher les messages
```

Quitter le moniteur : `Ctrl + T` puis `Ctrl + X` (`Ctrl + ]` ne fonctionne pas avec un clavier AZERTY).

Si le flash échoue : maintenir le bouton G0 en allumant le Cardputer (mode téléchargement), puis relancer. Après le flash, éteindre et rallumer **sans** G0 pour démarrer le programme.

## Structure

```
cd-os/
├── CMakeLists.txt      # projet ESP-IDF
├── main/
│   ├── CMakeLists.txt  # composant principal
│   └── main.c          # point d'entrée : app_main()
├── .clang-format       # style du code
├── sdkconfig           # configuration ESP-IDF (généré, à conserver)
└── build/              # fichiers compilés (généré, ignoré par Git)
```

## Feuille de route

- [x] **Étape 1 : hello world.** Projet minimal, messages de log, boucle avec `vTaskDelay`.
- [ ] **Étape 2 : GPIO.** Lire le bouton G0, piloter le rétroéclairage.
- [ ] **Étape 3 : tâches FreeRTOS.** Plusieurs tâches en parallèle, priorités.
- [ ] **Étape 4 : communication entre tâches.** Files de messages, mutex, bus d’événements.
- [ ] **Étape 5 : écran sans LVGL.** Bus SPI, pilote ST7789.
- [ ] **Étape 6 : LVGL.** Interface graphique, widgets, entrées.
- [ ] **Étape 7 : architecture.** Composants, couche matérielle (HAL), assemblage de l’OS.
- [ ] Ensuite : clavier (I2C), carte SD, Wi-Fi, BLE, audio, portage sur la T-Watch Ultra.

## Notes matérielles (Cardputer ADV)

Brochage vérifié dans le code de M5GFX (bibliothèque officielle de M5Stack) :

| Fonction | Broche |
|---|---|
| Écran ST7789 : MOSI / SCLK / DC / CS / RST | 35 / 36 / 34 / 37 / 33 |
| Rétroéclairage | 38 (PWM **256 Hz**, voir ci-dessous) |
| Carte microSD : SCLK / MISO / MOSI / CS | 40 / 39 / 14 / 12 |
| Bus I2C interne : SDA / SCL | 8 / 9 |
| Bouton G0 | 0 (actif à l’état bas) |

- **Rétroéclairage :** à 5 kHz, il ne s’allume qu’à 100 %. Utiliser une fréquence PWM de 256 Hz, comme M5Stack.
- **Écran :** 240 × 135 en paysage, couleurs inversées, décalage de la zone visible (40, 53) dans la mémoire du contrôleur.
- **Flash :** 8 Mo, pas de PSRAM.

## Sauvegarde et restauration du Cardputer

Sauvegarde complète de la flash (en mode téléchargement) :

```
esptool.py -p /dev/ttyACM0 read_flash 0 0x800000 sauvegarde_cardputer.bin
esptool.py -p /dev/ttyACM0 verify_flash 0 sauvegarde_cardputer.bin
```

Restauration :

```
esptool.py -p /dev/ttyACM0 write_flash 0 sauvegarde_cardputer.bin
```

## Références

- [Guide de programmation ESP-IDF (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html)
- [Exemples ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples) (aussi en local : `~/esp/esp-idf/examples`)
- [Documentation LVGL 9.2](https://docs.lvgl.io/9.2/)
- [M5GFX](https://github.com/m5stack/M5GFX) : référence pour le brochage du Cardputer
- [Beej’s Guide to C Programming](https://beej.us/guide/bgc/)
- [cppreference : C](https://en.cppreference.com/w/c)