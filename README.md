# 🛰️ ADASTRA – Data Logger Fusée  
### Club NOVA CNAM

Ce dépôt contient le **data logger embarqué** développé pour la fusée du **club NOVA CNAM**, dans le cadre du projet **ADASTRA CSPACE 2026**.

Le système permet d’enregistrer les données de vol (accélérations, vitesse angulaire, altitude, pression et distance sol) sur une **EEPROM externe**, afin de les analyser après récupération de la fusée.

---

## 📁 Arborescence du dépôt

```text
adastra-datalogger/
│
├── src/
│   ├── adastra_datalogger.ino    # Code embarqué (enregistrement des données en vol)
│   └── read_eeprom.ino           # Lecture EEPROM + export CSV via Serial
│
├── analysis/
│   └── analyse_vol.py            # Analyse des données de vol (Python)
│
├── README.md                     # Documentation principale du projet
│
└── docs/
    └── schema_branchement.png    # Schéma de câblage du data logger
```
---

## 🔧 Matériel utilisé

- **Arduino Nano**
- **MPU6050** – Accéléromètre + gyroscope (6 axes)
- **BMP280** – Capteur de pression et altitude
- **HC-SR04** – Capteur ultrason (phase sol / atterrissage)
- **EEPROM 24LC256** – Mémoire I2C (32 Ko)

---

## 🔌 Branchement

### 📡 Bus I2C (commun)
Les modules **MPU6050**, **BMP280** et **EEPROM 24LC256** partagent le même bus I2C.

| Arduino Nano | Module |
|-------------|--------|
| A4 (SDA) | SDA MPU6050 / BMP280 / EEPROM |
| A5 (SCL) | SCL MPU6050 / BMP280 / EEPROM |

---

### 🧭 MPU6050

| MPU6050 | Arduino Nano |
|-------|-------------|
| VCC | 5V *(ou 3.3V selon module)* |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

Adresse I2C par défaut : `0x68`

---

### 🌡️ BMP280

| BMP280 | Arduino Nano |
|-------|-------------|
| VCC | 3.3V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

Adresse I2C utilisée : `0x76`

---

### 📡 HC-SR04

| HC-SR04 | Arduino Nano |
|-------|-------------|
| VCC | 5V |
| GND | GND |
| TRIG | D2 |
| ECHO | D3 |

---

### 💾 EEPROM 24LC256

| EEPROM | Arduino Nano |
|-------|-------------|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |
| A0 | GND |
| A1 | GND |
| A2 | GND |
| WP | GND |

Adresse I2C : `0x50`

---

## 🧠 Architecture logicielle

Le système fonctionne comme un **data logger autonome** :

1. Lecture des capteurs
2. Mise en forme des données
3. Écriture séquentielle en EEPROM
4. Lecture des données après vol via USB (Serial)

Le timestamp est basé sur `millis()` (suffisant pour un vol suborbital amateur).

---

## 📦 Format des données enregistrées

Les données sont **optimisées pour l’embarqué** (pas de `float` en mémoire).

### Structure d’un enregistrement

| Donnée | Type | Taille |
|------|------|-------|
| Temps (ms) | `uint32_t` | 4 |
| Acc X | `int16_t` | 2 |
| Acc Y | `int16_t` | 2 |
| Acc Z | `int16_t` | 2 |
| Gyro X | `int16_t` | 2 |
| Gyro Y | `int16_t` | 2 |
| Gyro Z | `int16_t` | 2 |
| Pression (Pa/10) | `uint16_t` | 2 |
| Altitude (m/10) | `int16_t` | 2 |
| Distance (cm) | `uint16_t` | 2 |

➡️ **22 octets par mesure**  
➡️ Environ **1480 mesures** stockables

---

## 📚 Librairies nécessaires

À installer via le gestionnaire de bibliothèques Arduino :

- `Adafruit MPU6050`
- `Adafruit BMP280`
- `Adafruit Unified Sensor`
- `Wire` (incluse par défaut)

---

## 🚀 Fonctionnement du code

- Fréquence d’enregistrement : **~50 Hz**
- Écriture séquentielle en EEPROM
- Aucun effacement automatique (sécurité post-crash mais ça n'arriveras pas ;))
- Données récupérées après vol via un sketch de lecture (read_eeprom.ino)

---

## 📈 Exploitation des données

Après récupération de la fusée :
- Lecture EEPROM via Serial
- Export CSV
- Analyse sous **Python**
- Exploitation :  
  - Profil altitude  
  - Accélération max  
  - Détection décollage / apogée / impact  (Pas le déclanchement du parachute juste pour les données)

---

## 🔒 Sécurité & robustesse

- EEPROM non volatile (données conservées après crash)
- Bus I2C simple et fiable
- Code minimaliste pour éviter les erreurs en vol

---

## 🔭 Évolutions prévues

- Calcul vitesse verticale ?
- Passage SD Card pour vols longs ?

---

## 👨‍🚀 Auteurs

Projet développé par le **club NOVA CNAM**  
Data logger V1 par : **Dylan Perinetti**

> *« Je ne perds jamais, soit je gagne, soit j’apprends. »* – Nelson Mandela

---

## 🛰️ Projet ADASTRA
Fusée expérimentale étudiante – CNAM  Pour le CSPACE 2026
