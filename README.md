# ADASTRA - Data Logger V2 Fusee

### Club NOVA CNAM - CSPACE 2026

Electronique embarquee pour la fusee experimentale du **club NOVA CNAM**, dans le cadre du projet **ADASTRA**.

Le systeme est compose de **deux Arduino Nano independants** :

| Arduino | Role | Fichier source |
|---|---|---|
| **Arduino #1 – Data Logger** | Acquisition capteurs + enregistrement CSV sur micro-SD | `adastra_datalogger-v2.ino` |
| **Arduino #2 – Sequenceur** | Declenchement parachute (solenoide + ressort) | `Sequenceur_Nano.ino` |

---

## Arborescence du depot

```text
adastra/
├── src/
│   ├── adastra_datalogger-v2.ino   # Arduino #1 – Data logger micro-SD
│   ├── Sequenceur_Nano.ino         # Arduino #2 – Sequenceur parachute
│   └── Sequenceur.md               # Documentation sequenceur
│
├── analysis/
│   └── analyse_vol.py              # Analyse post-vol (Python)
│
├── docs/
│   └── schema_branchement.png      # Schema de cablage
│
└── README.md
```

---

## Materiel

### Arduino #1 – Data Logger

| Composant | Role |
|---|---|
| **Arduino Nano** | Microcontroleur data logger |
| **MPU6050** | Accelerometre + gyroscope 6 axes (I2C) |
| **BMP280** | Pression, altitude, temperature (I2C) |
| **HC-SR04** | Capteur ultrason - distance sol (digital) |
| **Module micro-SD (SPI)** | Stockage des donnees de vol |

### Arduino #2 – Sequenceur parachute

| Composant | Role |
|---|---|
| **Arduino Nano** | Microcontroleur sequenceur |
| **Solenoide 24 V** | Liberation du verrou de trappe |
| **MOSFET N logic-level** (IRLZ44N) | Commande de puissance solenoide |
| **Jack de rampe** | Detection decollage (arrachement) |
| **LEDs** | Indication d'etat (arme / tempo / fire / done) |
| **Diode de roue libre** (1N5408) | Protection bobine solenoide |

> Voir [src/Sequenceur.md](src/Sequenceur.md) pour le detail du sequenceur.

---

## Branchement

### Bus I2C (MPU6050 + BMP280)

| Arduino Nano | Module |
|---|---|
| A4 (SDA) | SDA MPU6050 / BMP280 |
| A5 (SCL) | SCL MPU6050 / BMP280 |

### MPU6050

| MPU6050 | Arduino Nano |
|---|---|
| VCC | 5 V *(ou 3.3 V selon module)* |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

Adresse I2C : `0x68`

### BMP280

| BMP280 | Arduino Nano |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

Adresse I2C : `0x76`

### HC-SR04

| HC-SR04 | Arduino Nano |
|---|---|
| VCC | 5 V |
| GND | GND |
| TRIG | D2 |
| ECHO | D3 |

### Module micro-SD (SPI)

| micro-SD | Arduino Nano |
|---|---|
| VCC | 5 V *(avec regul.)* ou 3.3 V |
| GND | GND |
| CS | D10 |
| MOSI | D11 |
| MISO | D12 |
| SCK | D13 |

---

## Architecture logicielle

```
          ┌──────────┐
          │  SETUP   │
          └────┬─────┘
               │
        ┌──────▼──────┐
        │  Init SD ?  │──Non──▶ mode serie seul
        └──────┬──────┘
               │ Oui
        ┌──────▼──────┐
        │  Init I2C   │  MPU6050 + BMP280
        └──────┬──────┘
               │
        ┌──────▼──────┐
        │   LOOP      │◀────────────────┐
        │  50 Hz      │                 │
        ├─────────────┤                 │
        │ Lecture      │                │
        │ capteurs     │                │
        ├─────────────┤                │
        │ Ligne CSV    │                │
        │ → serie      │                │
        │ → SD         │                │
        └──────┬──────┘                 │
               └────────────────────────┘
```

1. Lecture des capteurs (MPU6050, BMP280, HC-SR04)
2. Construction d'une ligne CSV en memoire
3. Ecriture simultanee sur **serie** et **carte micro-SD**
4. Compensation du temps d'execution pour maintenir ~50 Hz

---

## Format CSV

Le fichier `ADASTRA.CSV` est cree automatiquement sur la carte SD au premier demarrage.

| Colonne | Unite | Description |
|---|---|---|
| `time_ms` | ms | Timestamp (`millis()`) |
| `ax`, `ay`, `az` | m/s² | Accelerations (MPU6050) |
| `gx`, `gy`, `gz` | rad/s | Vitesses angulaires (MPU6050) |
| `pressure_Pa` | Pa | Pression atmospherique (BMP280) |
| `altitude_m` | m | Altitude barometrique (BMP280) |
| `temp_C` | °C | Temperature (BMP280) |
| `distance_cm` | cm | Distance au sol (HC-SR04) |

---

## Librairies Arduino

A installer via le gestionnaire de bibliotheques :

| Librairie | Usage |
|---|---|
| `Adafruit MPU6050` | Accelerometre / gyroscope |
| `Adafruit BMP280` | Pression / altitude |
| `Adafruit Unified Sensor` | Dependance Adafruit |
| `SD` | Lecture / ecriture carte micro-SD |
| `Wire` | Bus I2C *(incluse)* |
| `SPI` | Bus SPI *(incluse)* |

---

## Analyse post-vol (Python)

Le script `analysis/analyse_vol.py` fournit :

- **10 graphiques** : altitude, accelerations (XYZ + norme, m/s² et g), gyroscope (rad/s et °/s + norme), pression / temperature, distance sol, vitesse verticale, taux de montee/descente, dashboard recapitulatif
- **Detection automatique** du decollage, de l'apogee et de l'atterrissage
- **Rapport synthetique** : duree de vol, apogee, acceleration max, pression min/max, temperatures
- **Filtrage Butterworth** passe-bas pour lisser les donnees bruitees
- **Estimation de la vitesse verticale** par integration de l'acceleration

### Dependances Python

```bash
pip install numpy pandas matplotlib scipy
```

### Utilisation

```bash
# Afficher les graphiques (mode interactif)
python analysis/analyse_vol.py --file ADASTRA.CSV

# Sauvegarder les graphiques en PNG
python analysis/analyse_vol.py --file ADASTRA.CSV --save --outdir plots/
```

### Sortie type

```
====================================================
   RAPPORT DE VOL – ADASTRA - Club NOVA CNAM
====================================================
  Duree totale enregistrement : 42.56 s
  Nombre d'echantillons       : 2128
  Frequence moyenne           : 50.0 Hz

  Apogee                      : 312.45 m
  Temps apogee                : 8.72 s
  Acceleration max            : 78.53 m/s² (8.01 g)
  Pression min                : 977.32 hPa
  Pression max                : 1012.87 hPa
  Temperature min / max       : 18.2 / 22.7 °C

  [  Decollage  ]  t = 1.24 s
  [   Apogee    ]  t = 8.72 s
  [Atterrissage ]  t = 38.90 s
====================================================
```

---

## Securite & robustesse

- Stockage non volatile sur micro-SD (pas de perte si coupure)
- Si la carte SD est absente, les donnees restent visibles sur le port serie
- Bus I2C simple et fiable pour les capteurs
- Compensation de la duree de boucle pour une frequence d'echantillonnage stable
- Code minimaliste pour reduire les risques d'erreurs en vol

---

## Evolutions prevues

- Gestion multi-fichiers (index auto par vol)
- Redondance / checksum des donnees
- Calcul d'apogee en temps reel a bord
- Integration avec le sequenceur de recuperation

---

## Auteurs

Projet developpe par le **club NOVA CNAM**
Data logger par **Dylan Perinetti**

> *« Je ne perds jamais, soit je gagne, soit j'apprends. »* – Nelson Mandela

---

## Projet ADASTRA

Fusee experimentale etudiante – CNAM — CSPACE 2026
