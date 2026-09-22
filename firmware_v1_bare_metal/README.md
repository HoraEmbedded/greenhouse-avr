# Serre automatisée : ATmega2560

[![CI](https://github.com/<utilisateur>/<depot>/actions/workflows/ci.yml/badge.svg)](https://github.com/<utilisateur>/<depot>/actions/workflows/ci.yml)

Firmware bare-metal (Arduino Mega / ATmega2560) pour la gestion automatisée d'une serre : température/humidité (DHT22), humidité du sol (ADC), ventilateur et pompe pilotés par hystérésis, seuils configurables en EEPROM via commandes série, sécurité pompe (niveau d'eau + irrigation diurne uniquement), simulation Wokwi.


## Ce que ce projet démontre

- Logique de décision séparée du matériel — testable sans microcontrôleur (12 suites, 98 cas, `gcc` natif)
- Sûreté de fonctionnement : watchdog matériel, dégradation gracieuse en cas de panne capteur, deux gardes de sécurité indépendantes sur la pompe
- Analyse formelle du pire cas de pile (63/8192 octets, vérifiée par désassemblage réel)
- Preuve algébrique sur la durée de vie EEPROM (pas une estimation à la louche)
- Pipeline de vérification complet en CI (6 jobs) : build, tests, couverture, analyse statique, analyse de pile, télémétrie
- Outillage Python de capture/visualisation, avec simulation Wokwi entièrement automatisée (`wokwi-cli`)

## Composants matériels

| Composant | Rôle | Interface |
|---|---|---|
| DHT22 | Température et humidité de l'air | 1-Wire, broche 2 |
| Potentiomètre | Simule une sonde d'humidité du sol | ADC, A0 |
| DS1307 | Horloge temps réel — irrigation le jour uniquement | I2C (0x68), partagé avec l'écran |
| Interrupteur à flotteur | Niveau d'eau du réservoir — sécurité pompe | Numérique, broche 3 |
| LCD 1602 | Affichage local (température, humidité, états) | I2C (0x27) |
| LED bleue / verte | Simulent les relais pompe / ventilateur | Numérique, broches 8/9 |

## Architecture logicielle

| Module | Rôle |
|---|---|
| `hysteresis.c` | Décision ventilateur/pompe (bande morte) |
| `thresholds.c` | Validation des seuils |
| `command.c` | Analyseur de commandes série (`GET`/`SET`/`RESET`) |
| `eeprom_config.c` | Persistance (octet magique + checksum XOR) |
| `fault_handling.c` | Dégradation après pannes DHT22 répétées |
| `ring_buffer.c` | Réception UART par interruption |
| `water_level.c` / `schedule.c` | Gardes de sécurité pompe (eau, horaire jour/nuit) |
| `rtc_decode.c` / `dht22_decode.c` / `soil.c` | Décodage capteurs |

## Chiffres vérifiés

| | |
|---|---|
| Tests hôte | 12 suites, 98 cas |
| Couverture | 100 % lignes, 100 % fonctions, 93,5 % branches (86/92 — le reste prouvé inatteignable) |
| Pire cas de pile | 63 / 8192 octets (99,2 % de marge) |
| Flash / RAM | ~4,1 Ko / 425 octets sur 253 952 / 8192 |
| Durée de vie EEPROM | > 50 ans en usage normal, 1 an même en cas d'abus délibéré |

## Démarrage rapide

```bash
# Firmware
pio run

# Tests unitaires + couverture
cd test/host && make run && make coverage

# Analyse statique et pile
cppcheck --enable=warning,style,performance,portability --inconclusive --std=c11 -Isrc src/*.c
python tools/stack_analysis.py

# Télémétrie (capture automatisée via Wokwi)
cd tools/telemetry
pip install -r requirements.txt
python generate_drift_scenario.py --output drift_scenario.yaml
python capture_and_plot.py --scenario drift_scenario.yaml --duration 100
```


## Limites connues

- Sonde de sol simulée par un potentiomètre — calibration réelle à refaire sur une vraie sonde capacitive
- Aucune validation sur silicium réel, tout est vérifié en simulation + tests hôte
- Fenêtre horaire jour/nuit en constante de compilation, pas encore configurable en EEPROM
