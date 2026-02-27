# 🛰️ ADASTRA – Data Logger V1 Fusée  
### Club NOVA CNAM

Ce dépôt contient le **data logger embarqué** développé pour la fusée du **club NOVA CNAM**, dans le cadre du projet **ADASTRA CSPACE 2026**.

✅ **Mise à jour :** l’enregistrement des données se fait désormais sur **carte micro‑SD (module lecteur micro‑SD)**, afin de récupérer les logs facilement après vol (plus besoin d’EEPROM externe).

---

## 📁 Arborescence du dépôt

```text
adastra-datalogger/
│
├── src/
│   ├── Sequenceur_Nano.ino       # Code embarqué (Séquenceur de récupération)
│   ├── Sequenceur.md
│   ├── adastra_datalogger.ino    # Code embarqué (enregistrement des données en vol -> micro‑SD)
│   └── read_eeprom.ino           # (Legacy) Lecture EEPROM + export CSV via Serial (si ancien montage)
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
- **Lecteur micro‑SD (module SPI)** + **carte micro‑SD** *(stockage des données de vol)*

> 📝 L’EEPROM 24LC256 n’est plus utilisée dans la version actuelle (restée en “legacy” si besoin).

---

## 🔌 Branchement

### 📡 Bus I2C (commun)
Les modules **MPU6050** et **BMP280** partagent le même bus I2C.

| Arduino Nano | Module |
|-------------|--------|
| A4 (SDA) | SDA MPU6050 / BMP280 |
| A5 (SCL) | SCL MPU6050 / BMP280 |

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

### 💾 Lecteur micro‑SD (SPI)
Le module micro‑SD utilise le bus **SPI**.

| micro‑SD (SPI) | Arduino Nano |
|---|---|
| VCC | 5V *(si module avec régulation/level shifting)* **ou** 3.3V *(si module “nu”)* |
| GND | GND |
| CS (SS) | D10 |
| MOSI | D11 |
| MISO | D12 |
| SCK | D13 |

---

## 🧠 Architecture logicielle

Le système fonctionne comme un **data logger autonome** :

1. Lecture des capteurs
2. Mise en forme des données
3. Écriture en continu sur **carte micro‑SD**
4. Récupération des fichiers après vol (lecture PC)

Le timestamp est basé sur `millis()` (suffisant pour un vol suborbital amateur).

---

## 📦 Format des données enregistrées

Les données peuvent être enregistrées :
- soit en **CSV** (lisible directement),
- soit en **binaire** (plus rapide/robuste) puis converties via Python.

### Champs typiques d’une mesure
| Donnée |
|------|
| Temps (ms) |
| Acc X / Y / Z |
| Gyro X / Y / Z |
| Pression |
| Altitude |
| Distance sol |

> Le choix du format (CSV vs binaire) dépend des contraintes de débit et de robustesse en vol.

---

## 📚 Librairies nécessaires

À installer via le gestionnaire de bibliothèques Arduino :

- `Adafruit MPU6050`
- `Adafruit BMP280`
- `Adafruit Unified Sensor`
- `Wire` (incluse par défaut)
- `SD` *(ou `SdFat` si on veut plus de performance/contrôle)*

---

## 🚀 Fonctionnement du code

- Fréquence d’enregistrement : **~50 Hz**
- Création/écriture d’un fichier de log sur la **micro‑SD**
- Données récupérées après vol en lisant la carte micro‑SD sur PC
- Aucune “suppression auto” des anciens fichiers (sécurité post-vol)

---

## 📈 Exploitation des données

Après récupération de la fusée :
- Récupération du fichier de log sur la micro‑SD
- Analyse sous **Python**
- Exploitation :  
  - Profil altitude  
  - Accélération max  
  - Détection décollage / apogée / impact *(pas le déclenchement du parachute, juste pour les données)*

---

## 🔒 Sécurité & robustesse

- Stockage non volatile sur micro‑SD
- Bus I2C simple et fiable pour les capteurs
- Code minimaliste pour réduire les risques d’erreurs en vol

---

## 🔭 Évolutions prévues

- Gestion multi‑fichiers (log par vol / index auto)
- Redondance (double log / checksum)
- Calcul vitesse verticale / apogée en temps réel ?

---

## 👨‍🚀 Auteurs

Projet développé par le **club NOVA CNAM**  
Data logger V1 par : **Dylan Perinetti**

> *« Je ne perds jamais, soit je gagne, soit j’apprends. »* – Nelson Mandela

---

## 🛰️ Projet ADASTRA
Fusée expérimentale étudiante – CNAM — Pour le CSPACE 2026
