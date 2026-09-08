# Système Embarqué de Gestion de Serre Intelligente (Validation par Simulation)

[![CI](https://github.com/<utilisateur>/<depot>/actions/workflows/ci.yml/badge.svg)](https://github.com/<utilisateur>/<depot>/actions/workflows/ci.yml)

## Description du Projet
Ce projet consiste en la conception et l'implémentation d'un micrologiciel de contrôle automatisé pour une serre agricole. Le code est développé intégralement en C pur (approche *bare-metal*) pour l'architecture AVR (ATmega2560), garantissant une exécution déterministe et non-bloquante via une gestion par interruptions matérielles.

Afin de valider la logique de commande (hystérésis) et l'intégrité des communications bas niveau (I2C, 1-Wire, ADC) avant le déploiement matériel, l'intégralité du système a été modélisée et testée sur l'environnement de simulation industrielle **Wokwi**.

---

## Modélisation de l'Environnement Virtuel (Wokwi)
Dans le cadre de la simulation, les composants physiques de puissance ont été substitués par des modèles virtuels équivalents pour valider les signaux électriques générés par le microcontrôleur. La topologie est définie dans le fichier `diagram.json`.

### Rôle des Composants Simulés
* **ATmega2560 (`wokwi-arduino-mega2560`) :** Cœur du système. Il exécute le fichier binaire `.hex` généré par le compilateur AVR-GCC et gère les registres matériels virtuels.
* **Afficheur LCD I2C (`wokwi-lcd1602-i2c`) :** Modélise l'écran physique et son module PCF8574. Permet de valider l'implémentation logicielle du protocole TWI/I2C (adressage `0x27`) et l'affichage de l'IHM.
* **Capteur DHT22 (`wokwi-dht22`) :** Simule les variations environnementales de l'air. Il répond aux requêtes du microcontrôleur en générant les chronogrammes stricts du protocole 1-Wire (impulsions de 40 µs à 80 µs).
* **Potentiomètre Linéaire (`wokwi-potentiometer`) :** Remplace la sonde capacitive d'humidité du sol. Il agit comme un diviseur de tension modifiant le signal de 0V à 5V sur la broche `A0`, permettant de valider la configuration du convertisseur analogique-numérique (ADC).
* **LEDs de Signalisation (`wokwi-led`) et Résistances :** Remplacent les modules relais électromécaniques 5V. 
  * La **LED Bleue** (broche 8) valide la tension haute (5V) modélisant la fermeture du contacteur de la **pompe d'irrigation**.
  * La **LED Verte** (broche 9) valide la tension haute (5V) modélisant la fermeture du contacteur du **ventilateur**.

 <img width="1018" height="783" alt="serre" src="https://github.com/user-attachments/assets/96a32dc9-5000-4b12-9090-7a35cb5f5907" />

  
---

## Architecture Logicielle (Pilotes *Bare-Metal*)
Le projet n'utilise aucune bibliothèque externe. Les pilotes suivants ont été écrits via la manipulation directe des registres :

1. **Cadencement par interruption :** Le Timer 1 est configuré en mode CTC (prescaler de 1024) pour déclencher une interruption `TIMER1_COMPA_vect` toutes les 2 secondes, qui ne fait que lever un drapeau (`measure_flag`). C'est cette étape de cadencement qui est non-bloquante ; le traitement qui suit (lecture DHT22, écriture LCD) reste volontairement séquentiel, voir "Décisions de conception" plus bas.
2. **UART :** Configuration des registres `UBRR0` et `UCSR0` pour la télémétrie asynchrone (9600 bauds).
3. **I2C (TWI) :** Gestion du registre `TWCR` pour cadencer l'horloge SCL à 100 kHz.
4. **ADC :** Configuration de l'échantillonnage avec un facteur de division de 128 via `ADCSRA`.

---

## Décisions de conception

- **ATmega2560, pas un Uno/328P** : le projet utilise trois périphériques matériels simultanément (UART, TWI/I2C, ADC) plus un timer dédié — le 2560 offre plus de RAM et de broches pour étendre le système (plusieurs capteurs, écran) sans reconfiguration.
- **Lecture bloquante déclenchée par interruption, pas 100 % asynchrone** : l'interruption Timer1 ne fait que lever un drapeau toutes les 2 s, ce qui est la partie réellement non-bloquante. La lecture du DHT22 et l'écriture LCD qui suivent sont volontairement bloquantes : rien d'autre ne doit s'exécuter pendant ces quelques millisecondes, donc l'attente active est un choix simple et suffisant, pas une architecture temps réel complète.
- **Hystérésis à seuils dissociés (ON/OFF différents)** plutôt qu'un seuil unique : un seuil unique ferait claquer le relais à chaque mesure quand la valeur oscille autour de lui. La bande morte (2 °C, 30 points de %) absorbe le bruit de mesure normal du DHT22.
- **Logique de décision extraite dans `hysteresis.c`**, sans aucun accès registre : elle compile et se teste avec un `gcc` hôte ordinaire (`test/host/`), indépendamment du firmware AVR. Un bug de seuil se détecte donc sans carte, sans simulateur, en une fraction de seconde.

## Comportement en cas de panne capteur

- **La pompe ne dépend plus jamais du DHT22.** L'humidité du sol vient de l'ADC (potentiomètre en simulation, sonde capacitive en réel), un circuit totalement indépendant du capteur d'air. Avant cette correction, une panne du DHT22 gelait aussi silencieusement l'irrigation -- ce n'était jamais nécessaire.
- **Le ventilateur, lui, dépend réellement de la température.** Une lecture DHT22 manquée isolée ne change rien (probable glitch de timing sur le bus 1-Wire) : le dernier état connu est conservé. Mais après **5 échecs consécutifs** (~10 s sans donnée), le firmware ne peut plus faire confiance à cet état et **force le ventilateur en marche**, plutôt que de le laisser dans un état qui pourrait dater d'avant une vraie surchauffe.
- **Ce choix (forcer ON, pas OFF) est un compromis de sûreté assumé** : une serre qui surchauffe peut tuer les plantes en moins d'une heure ; un ventilateur qui tourne un peu plus que nécessaire ne coûte que de l'électricité. Entre les deux modes de défaillance, celui-ci privilégie le réversible.
- Le seuil (`DHT_FAILURE_SAFETY_THRESHOLD`, 5 cycles) est isolé dans `fault_handling.h` et testé indépendamment (`test/host/test_fault_handling.c`).

## Limites connues

- **Le capteur d'humidité du sol est simulé par un potentiomètre linéaire** (`diagram.json`). La conversion (`soil.c`) est calibrée sur ce comportement idéal (0 → 0 %, 1023 → 100 %). Une vraie sonde capacitive n'est pas linéaire sur toute sa plage et est souvent de polarité inverse (valeur brute plus haute quand c'est plus sec). Les deux constantes de calibration sont isolées dans `soil.h` précisément pour être remesurées sur le vrai capteur avant tout déploiement.
- **Aucun test sur silicium réel** : tout est validé en simulation Wokwi et par tests hôte sur la logique pure. Le comportement électrique réel (bruit sur l'ADC, timing DHT22 sur un vrai bus) reste à vérifier sur carte.
- **La lecture DHT22 désactive les interruptions pendant quelques millisecondes** (`cli()`/`sei()` dans `dht_read()`) pour garantir un timing fiable sur le protocole 1-Wire. Sur ce projet à une seule source d'interruption, l'effet est nul ; ça deviendrait un point d'attention si d'autres interruptions à échéance courte étaient ajoutées plus tard.

## Seuils configurables (EEPROM + commandes série)

Les seuils d'hystérésis (allumage/extinction ventilateur et pompe) ne sont plus figés dans le code : ils sont chargés depuis l'EEPROM au démarrage, et modifiables à chaud via le port série (9600 bauds), sans reflasher le firmware.

**Commandes disponibles :**

| Commande | Effet |
|---|---|
| `GET` | Affiche les 4 seuils actuels |
| `SET FAN_ON <dixièmes de °C>` | Ex. `SET FAN_ON 275` pour 27,5 °C |
| `SET FAN_OFF <dixièmes de °C>` | |
| `SET PUMP_ON <%>` | |
| `SET PUMP_OFF <%>` | |
| `RESET` | Restaure les valeurs d'usine et les sauvegarde |

Toute commande `SET` qui rendrait la bande morte invalide (seuil ON du ventilateur ≤ seuil OFF, par exemple) est **rejetée** avant d'atteindre les actionneurs — la validation (`thresholds.c`) est testée indépendamment de la persistance et de la lecture série.

**Robustesse EEPROM** : un octet magique et un checksum XOR détectent une puce vierge ou une écriture interrompue par une coupure de courant ; dans les deux cas, le firmware retombe sur les valeurs d'usine plutôt que de faire confiance à des données partielles.

## Outil de télémétrie (tools/telemetry/)

Le firmware envoie déjà sa télémétrie sur le port série toutes les 2 secondes. Ces scripts la capturent, l'enregistrent, et la visualisent -- rien côté firmware n'a changé.

```bash
pip install -r tools/telemetry/requirements.txt

# Capture en continu vers un CSV (Ctrl+C pour arrêter)
python3 tools/telemetry/logger.py --port COM5 --output telemetry.csv

# Génère un graphique à partir du CSV
python3 tools/telemetry/plot.py --input telemetry.csv --output telemetry.png
```

Le graphique superpose la température et l'humidité du sol avec des zones grisées montrant quand le ventilateur et la pompe étaient actifs -- utile pour voir la bande morte de l'hystérésis en action sur des données réelles, pas seulement en théorie.

**Note :** l'image d'exemple ci-dessous (si présente dans le dépôt) est générée à partir de données synthétiques pour illustrer le format, pas d'une vraie session de mesure -- à remplacer par un graphique issu d'une capture réelle une fois le montage physique disponible.

**Tests** : `logger.py` sépare le parsing (`parser.py`, pur, sans port série) de la boucle de lecture (`logger.run()`, testée avec un faux port dans `test_logger_integration.py`). Lancer :
```bash
cd tools/telemetry
python3 test_parser.py
python3 test_logger_integration.py
```

## Couverture de code

```bash
pip install gcovr
cd test/host
make coverage
```

| Métrique | Résultat mesuré |
|---|---|
| Lignes | 100 % (91/91) |
| Fonctions | 100 % (12/12) |
| **Branches** | **92,7 % (76/82)** |

La couverture de **lignes** est trompeuse seule : une ligne `if (x < 0) return 0;` apparaît "exécutée" dès que la comparaison tourne, que le `return` ait lieu ou non. La couverture de **branches** est la mesure qui compte, et c'est celle-ci qu'on rapporte.

**Deux branches restent non couvertes dans `thresholds.c`, et c'est volontaire, pas un trou de test** : `fan_off_decidegC > MAX` et `pump_off_percent < 0` sont **structurellement inatteignables**, prouvé par le calcul directement en commentaire dans `thresholds.c` -- la contrainte d'ordre (`fan_on > fan_off`) combinée à la vérification de plage sur `fan_on` rend la première mathématiquement impossible à déclencher ; argument symétrique pour la seconde. Écrire un test pour ces cas forcerait une entrée qui ne peut pas survenir, ce qui ne prouverait rien -- exactement le travers que `NC-015` du projet Sentinelle met en garde (un chiffre de couverture n'a de valeur que si on sait ce qu'il mesure réellement).

## Tests


```bash
cd test/host
make run
```

Huit suites, 51 cas au total : hystérésis, validation des seuils, analyseur de commandes série, dégradation en cas de panne capteur, conversion du capteur de sol (calibration par défaut **et** une calibration réaliste testée dans les deux polarités -- normale et inversée, comme le fera probablement une vraie sonde capacitive), et décodage DHT22 (checksum avec repli modulo 256, bit de signe négatif) -- toutes sur l'hôte, sans AVR ni simulateur.

---

## Instructions de Déploiement et de Simulation

### 1. Compilation du Firmware
Le projet est configuré pour l'environnement de développement PlatformIO.

```bash
git clone <url-du-depot>
cd <nom-du-dossier>
pio run
```

> **Note :** Cette commande génère le fichier exécutable `firmware.hex` dans le répertoire `.pio/build/megaatmega2560/`.

### 2. Exécution de la Simulation
1. Ouvrir l'environnement de simulation **Wokwi**.
2. Importer le fichier de routage `diagram.json` pour générer le circuit virtuel.
3. Importer les fichiers sources (`main.c`) ou charger directement le fichier `firmware.hex` compilé.
4. Lancer la simulation. Le comportement des actionneurs (LEDs) peut être observé en modifiant interactivement les valeurs du DHT22 et du potentiomètre via l'interface graphique.

### Télémétrie
Les données d'état et les mesures environnementales sont retransmises en temps réel sur le terminal série virtuel, permettant le profilage des algorithmes d'hystérésis.

---
*Projet réalisé dans le cadre d'un cursus en ingénierie des systèmes embarqués.*
