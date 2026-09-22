# Centrale de régulation : de l'ATmega2560 à l'ESP32 IoT

[![V1 CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/ci.yml)
[![V2 CI](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml/badge.svg)](https://github.com/HoraEmbedded/greenhouse-avr/actions/workflows/v2_esp32_ci.yml)

Monorepo retraçant l'évolution complète d'une centrale de régulation critique pour serre.

Le projet présente deux générations d'architecture. La V1 repose sur un firmware bare-metal en C pour ATmega2560, validé par 98 tests unitaires. La V2 fait évoluer le système vers une architecture IoT basée sur ESP32 et FreeRTOS, avec communication MQTT sur TLS et télémétrie vers un broker Cloud.

## Contenu du dépôt

| Dossier                                              | Description                                                                                                                     |
| ---------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| [`firmware_v1_bare_metal/`](firmware_v1_bare_metal/) | V1 : firmware C pur sur ATmega2560, drivers développés à partir de la datasheet et régulation locale par hystérésis             |
| [`firmware_v2_rtos_iot/`](firmware_v2_rtos_iot/)     | V2 : migration vers ESP32 et FreeRTOS, télémétrie MQTT/TLS vers le Cloud, écran LCD et double environnement simulation/hardware |

## Démarche

La V1 se concentre sur la maîtrise des contraintes d'un système embarqué temps réel.

Elle met en œuvre :

* une super-boucle non bloquante
* le mode sommeil IDLE
* un watchdog
* une analyse de pile par désassemblage
* une preuve algébrique de la durée de vie de l'EEPROM
* une régulation locale par hystérésis
* des tests unitaires exécutés sur l'hôte

La V2 conserve cette approche et l'étend avec une architecture connectée et multitâche.

Elle introduit :

* FreeRTOS et la gestion de tâches préemptives
* la communication inter-tâches par queues et mutex
* la connectivité Wi-Fi
* MQTT sur TLS 1.2/1.3
* la gestion d'un Root CA
* l'isolation des secrets
* la télémétrie vers un broker Cloud
* la séparation des responsabilités entre logique métier, communication et matériel

Le cœur de décision reste implémenté en C pur. Les fonctions de régulation, les gardes de pompe et la gestion de la dégradation des capteurs sont ainsi portables et testables sur PC, indépendamment de la cible matérielle.

## Comparaison V1 / V2

|                 | V1 Bare-metal                 | V2 RTOS / IoT                          |
| --------------- | ----------------------------- | -------------------------------------- |
| Cible           | ATmega2560, 8 bits            | ESP32, 32 bits dual-core               |
| Architecture    | Super-boucle non bloquante    | 4 tâches FreeRTOS préemptives          |
| Communication   | Modules C appelés en séquence | Queues et mutex inter-tâches           |
| Réseau          | Aucun                         | Wi-Fi WPA2, MQTT sur TLS 1.2/1.3       |
| Sécurité        | Watchdog, checksum, EEPROM    | Watchdog, TLS, Root CA, secrets isolés |
| Tests hôte      | 12 suites, 98 cas             | 4 suites, 11 cas                       |
| CI              | 6 jobs                        | 4 jobs                                 |
| Simulation      | Wokwi CLI                     | Wokwi CLI                              |
| Sortie locale   | LCD 1602 I2C                  | LCD 1602 I2C                           |
| Sortie distante | Port série                    | Broker Cloud HiveMQ                    |

## Démarrage rapide

### V1

```bash
cd firmware_v1_bare_metal
pio run
```

### V2, simulation

```bash
cd firmware_v2_rtos_iot
pio run -e esp32dev_wokwi
wokwi-cli .
```

La documentation détaillée de chaque version est disponible dans son dossier respectif.

## Auteur

Projet personnel réalisé par Horacia Azonhoumon.
