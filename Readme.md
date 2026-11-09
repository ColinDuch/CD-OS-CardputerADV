# CD-OS

Un système d’exploitation fait maison pour appareils à base d’ESP32-S3, écrit en C avec ESP-IDF et FreeRTOS.

Le projet est construit étape par étape, dans un but d’apprentissage : chaque brique (tâches, événements, pilotes, interface) est écrite et comprise avant de passer à la suivante.

## Matériel

| Appareil | Rôle | Outils | État |
|---|---|---|---|
| M5Stack Cardputer ADV | Plateforme de développement | ESP-IDF (`idf.py`) | En cours |
| LILYGO T-Watch Ultra | Cible finale (montre) | PlatformIO (à confirmer) | Plus tard |

Organisation prévue : deux OS avec une base commune. Le code indépendant du matériel (bus d’événements, services, apps communes) sera partagé, chaque appareil gardant ses pilotes, son interface et ses apps spécifiques.

## Fonctionnalités actuelles

- **Bouton G0** lu par scrutation toutes les 20 ms, avec détection des changements d’état.
- **Rétroéclairage** réglé en PWM (LEDC, 256 Hz) : chaque appui augmente la luminosité par pas de 5 %, de 0 à 100 %.
- **Tâches FreeRTOS** : lecture du bouton, battement de cœur (journal chaque seconde), distribution du bus.
- **Bus d’événements** (publication / abonnement) : `EVT_LUMINOSITE`, `EVT_BOUTON_APPUYE`.
- **État partagé** (`s_etat`) protégé par un mutex.
- **Pilote d’écran maison** (ST7789 via `esp_lcd`) : remplissage, rectangles avec rognage, caractères et texte avec facteur d’échelle (1 à 4).
- **Barre de luminosité** à l’écran, redessinée à chaque événement `EVT_LUMINOSITE`.

## Prérequis

- ESP-IDF v5.4, installé dans `~/esp/esp-idf`
- Un câble USB-C qui transmet les données (pas seulement la charge)
- Sous Arch : être dans le groupe `uucp` pour accéder à `/dev/ttyACM0`
- Après une mise à jour du noyau Linux, redémarrer (sinon le pilote `cdc_acm` ne se charge pas et `/dev/ttyACM0` n’apparaît pas)

Charger l’environnement ESP-IDF dans chaque nouveau terminal :

```
get_idf
```

(alias défini dans `~/.bashrc` : `alias get_idf='. $HOME/esp/esp-idf/export.sh'`)

## Compiler et lancer

```
idf.py set-target esp32s3              # une seule fois, à la création du projet
idf.py build                           # compiler
idf.py -p /dev/ttyACM0 flash monitor   # flasher puis afficher les messages
idf.py menuconfig                      # modifier la configuration (sdkconfig)
```

Quitter le moniteur : `Ctrl + T` puis `Ctrl + X` (`Ctrl + ]` ne fonctionne pas avec un clavier AZERTY).

Si le flash échoue : maintenir le bouton G0 en allumant le Cardputer (mode téléchargement), puis relancer. Après le flash, éteindre et rallumer **sans** G0 pour démarrer le programme.

## Éditeur (VS Code)

Pour que l’analyse du code trouve les en-têtes d’ESP-IDF, `.vscode/settings.json` pointe vers le fichier généré à chaque compilation :

```json
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

Compiler au moins une fois, puis recharger la fenêtre. Ne pas activer clangd en même temps que l’extension C/C++ de Microsoft.

## Structure

```
cd-os/
├── CMakeLists.txt        # projet ESP-IDF
├── sdkconfig.defaults    # réglages personnalisés (flash 8 Mo)
├── sdkconfig             # configuration complète (générée, à conserver)
├── .clang-format         # style du code
├── main/
│   ├── CMakeLists.txt    # liste des sources : main.c, bus.c, ecran.c
│   ├── main.c            # app_main, tâches, abonnés du bus, barre de luminosité
│   ├── bus.h / bus.c     # bus d'événements (file + tâche de distribution)
│   ├── ecran.h / ecran.c # pilote d'écran : SPI, ST7789, dessin, texte
│   └── police8x8.h       # police bitmap 8 × 8 (ASCII)
└── build/                # fichiers compilés (généré, ignoré par Git)
```

## Architecture

```
tâche bouton ── bus_publier ──▶ [ file du bus ] ──▶ tâche de distribution
                                                     │
                                       appelle les abonnés concernés
                                                     │
                     ┌───────────────────┬───────────┴───────────┬──────────────────────┐
                     ▼                   ▼                       ▼                      ▼
            journal_luminosite   alerte_luminosite      afficher_luminosite      journal_appui
```

Règles retenues :

- Une tâche qui attend doit toujours rendre la main à FreeRTOS (`vTaskDelay`, attente d’une file ou d’un sémaphore).
- Priorités : plus le nombre est grand, plus la tâche est prioritaire. Bouton 5, bus 4, battement 2, `app_main` 1.
- Files et mutex sont créés **avant** les tâches qui les utilisent (préemption dès `xTaskCreate`).
- Sections critiques aussi courtes que possible ; on copie sous le mutex, on affiche après.
- Les abonnés du bus s’exécutent dans la tâche du bus : ils doivent rester courts, sans `vTaskDelay`.
- On s’abonne uniquement au démarrage (la liste des abonnés n’est pas protégée par un mutex).
- L’écran est protégé par un mutex : on peut dessiner depuis n’importe quelle tâche.

## Feuille de route

- [x] **Étape 1 : hello world.** Projet minimal, messages de log, boucle avec `vTaskDelay`.
- [x] **Étape 2 : GPIO.** Lecture du bouton G0, sortie GPIO, PWM avec le LEDC.
- [x] **Étape 3 : tâches FreeRTOS.** Plusieurs tâches en parallèle, priorités, préemption.
- [x] **Étape 4 : communication entre tâches.** Files de messages, mutex, bus d’événements.
- [ ] Partie 4 : sortir le bus dans son propre composant (reportée à l’étape 7).
- [x] **Étape 5 : écran sans LVGL.** Bus SPI, pilote ST7789, rectangles, texte, mutex d’écran.
- [ ] **Étape 6 : LVGL.** Interface graphique, widgets, entrées.
- [ ] **Étape 7 : architecture.** Composants, couche matérielle (HAL), assemblage de l’OS.
- [ ] Ensuite : clavier (I2C), carte SD, Wi-Fi, BLE, audio, portage sur la T-Watch Ultra.

Idées en attente : retour à la ligne et défilement du texte, texte transparent, accents (police `font8x8_latin`), seuil minimal de luminosité.

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
- **Seuil de luminosité :** contenu contrasté visible dès environ 10 % (valeur 25 sur 255). Écran non initialisé : visible vers 15 % (valeur 38).
- **Écran :** 240 × 135 en paysage, couleurs inversées, `swap_xy` + miroir horizontal, décalage de la zone visible (40, 53) dans la mémoire du contrôleur.
- **Couleurs RGB565 :** 5 bits rouge, 6 bits vert, 5 bits bleu. Octets à inverser avant l’envoi (l’ESP32 est petit-boutiste, le ST7789 attend l’octet fort en premier).
- **Mémoire :** flash de 8 Mo (`CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y`), pas de PSRAM. La T-Watch Ultra a 16 Mo de flash et 8 Mo de PSRAM.

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

## Crédits

- Police `police8x8.h` : font8x8 de Daniel Hepper, domaine public, d’après les polices VGA d’IBM.

## Références

- [Guide de programmation ESP-IDF (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html)
- [FreeRTOS dans ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/system/freertos_idf.html)
- [API GPIO](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-reference/peripherals/gpio.html)
- [Exemples ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples) (aussi en local : `~/esp/esp-idf/examples`)
- [Documentation LVGL 9.2](https://docs.lvgl.io/9.2/)
- [M5GFX](https://github.com/m5stack/M5GFX) : référence pour le brochage du Cardputer
- [font8x8](https://github.com/dhepper/font8x8) : police bitmap utilisée
- [Beej’s Guide to C Programming](https://beej.us/guide/bgc/)
- [cppreference : C](https://en.cppreference.com/w/c)