# Serre connectée : ESP32 + FreeRTOS

[![CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml)

Évolution IoT du firmware V1 (ATmega2560 bare-metal) vers une cible ESP32
32-bits. Architecture multitâche préemptive FreeRTOS, régulation locale
par hystérésis conservée à l'identique, télémétrie sécurisée MQTT/TLS vers
un broker Cloud, écran LCD 1602 I2C partagé sur le bus protégé par mutex.

## Ce que ce projet démontre

- Migration Bare-metal vers RTOS : découpage en 4 tâches préemptives
  (Sensors, Logic, Network, Display), communication par Queues, exclusion
  mutuelle sur le bus I2C
- Portabilité de la logique métier : les modules de décision (hystérésis,
  gardes pompe, dégradation capteur) restent en C pur, testables sur PC
  indépendamment de la cible
- Sécurité Edge-to-Cloud : Wi-Fi WPA2, MQTT sur TLS 1.2/1.3 (port 8883),
  authentification par certificat Root CA, payload JSON structuré
- Double environnement de build : simulation Wokwi (publisher série) et
  carte réelle (publisher MQTT/TLS), sélection par filtre de sources
- Séparation des secrets : `secrets.h` git-ignoré, template `.example`
  fourni pour la CI

## Composants matériels

| Composant | Rôle | Interface |
|---|---|---|
| DHT22 | Température et humidité de l'air | GPIO 4 |
| Potentiomètre | Simule une sonde d'humidité du sol | ADC, GPIO 34 |
| DS1307 | Horloge temps réel, irrigation le jour uniquement | I2C (0x68) |
| Interrupteur à flotteur | Niveau d'eau du réservoir, sécurité pompe | GPIO 27 |
| LCD 1602 | Affichage local (T°, sol, états actionneurs) | I2C (0x27) |
| LED bleue / verte | Simulent les relais pompe / ventilateur | GPIO 26 / 25 |

## Architecture logicielle

| Tâche | Priorité | Coeur | Rôle |
|---|---|---|---|
| `Task_Sensors` | 3 | 0 | Lecture capteurs à 20 Hz, mutex I2C |
| `Task_Logic` | 2 | 0 | Hystérésis, gardes pompe, encodeur JSON |
| `Task_Network` | 1 | 1 | Publication télémétrie à 1 Hz |
| `Task_Display` | 1 | 0 | Rafraîchissement LCD à 2 Hz |

| Module `core/` | Rôle |
|---|---|
| `hysteresis.c` | Décision ventilateur/pompe (bande morte) |
| `thresholds.c` | Structure de configuration et validation |
| `fault_handling.c` | Dégradation ventilateur après pannes DHT22 |
| `water_level.c` / `schedule.c` | Gardes pompe (eau, jour/nuit) |
| `rtc_decode.c` / `dht22_decode.c` / `soil.c` | Décodage capteurs |
| `telemetry_json.c` | Encodeur JSON portable (testé sur hôte) |

## Chiffres vérifiés

| | |
|---|---|
| Tests hôte | 4 suites, 11 cas (`gcc` natif) |
| Tâches FreeRTOS | 4 (réparties sur 2 coeurs) |
| Environnements PlatformIO | 2 (simulation Wokwi, carte réelle) |
| Jobs CI | 4 (tests hôte, build Wokwi, build hardware, contrôle secrets) |
| Fréquence capteurs | 20 Hz |
| Fréquence publication | 1 Hz |

## Démarrage rapide

### Simulation Wokwi

```bash
pio run -e esp32dev_wokwi
wokwi-cli .
```

Le LCD affiche `System Active` pendant 2 s, puis les valeurs temps réel.
Bouger les curseurs DHT22 et potentiomètre dans Wokwi pour tester
l'hystérésis et les gardes pompe.

### Carte ESP32 réelle

```bash
cp include/secrets.h.example include/secrets.h
# Remplir Wi-Fi, broker MQTT, identifiants et Root CA
pio run -e esp32dev_hw -t upload
pio device monitor -e esp32dev_hw
```

### Tests hôte

```bash
cd test/host && make run
```

## Limites connues

- Validation Cloud (HiveMQ) à effectuer sur silicium réel, la simulation
  Wokwi gratuite ne ponte pas le Wi-Fi vers Internet
- Sonde sol simulée par potentiomètre, calibration réelle à refaire sur
  sonde capacitive
- `PubSubClient` ne supporte pas QoS 1/2, envisager ESP-MQTT pour une
  fiabilité industrielle
- Fenêtre horaire jour/nuit en constante de compilation
