# Système Embarqué de Gestion de Serre Intelligente (Validation par Simulation)

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
  * La **LED Bleue** valide la tension haute (5V) sur la broche 9, modélisant la fermeture du contacteur du **ventilateur**.
  * La **LED Verte** valide la tension haute (5V) sur la broche 8, modélisant la fermeture du contacteur de la **pompe d'irrigation**.

 <img width="1018" height="783" alt="serre" src="https://github.com/user-attachments/assets/96a32dc9-5000-4b12-9090-7a35cb5f5907" />

  
---

## Architecture Logicielle (Pilotes *Bare-Metal*)
Le projet n'utilise aucune bibliothèque externe. Les pilotes suivants ont été écrits via la manipulation directe des registres :

1. **Machine d'état Asynchrone :** Le Timer 1 est configuré en mode CTC (prescaler de 1024) pour déclencher une interruption `TIMER1_COMPA_vect` toutes les 2 secondes, éliminant tout appel bloquant dans la boucle principale.
2. **UART :** Configuration des registres `UBRR0` et `UCSR0` pour la télémétrie asynchrone (9600 bauds).
3. **I2C (TWI) :** Gestion du registre `TWCR` pour cadencer l'horloge SCL à 100 kHz.
4. **ADC :** Configuration de l'échantillonnage avec un facteur de division de 128 via `ADCSRA`.

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
