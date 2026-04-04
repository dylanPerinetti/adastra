# PCB Support ADASTRA v2 - Arduino Nano + BMP280 + MPU9250 + SD + HC-SR04

> Adapté pour le projet [ADASTRA (FuséEx)](https://github.com/dylanPerinetti/adastra)
> Club NOVA CNAM — CSPACE 2026
> Firmware : `src/adastra_datalogger-v2.ino`

## Connexions (Pin Mapping)

### Bus I2C (BMP280 + MPU9250)
| Signal | Arduino Nano Pin | BMP280  | MPU9250 |
|--------|-----------------|---------|---------|
| SDA    | A4              | SDA     | SDA     |
| SCL    | A5              | SCL     | SCL     |
| VCC    | 3.3V            | VCC (3.3V) | —    |
| VCC    | 5V              | —       | VCC (5V)|
| GND    | GND             | GND     | GND     |
| CSB    | —               | V3V3 (I2C mode) | — |
| SDO    | —               | GND (addr 0x76) | — |

> **Note :** Le BMP280 est alimenté en 3.3V, le MPU9250 en 5V (comme dans le code ADASTRA v2).
> **BMP280 6 pins :** VCC, GND, SCL, SDA, CSB (→ V3V3 pour mode I2C), SDO (→ GND pour adresse 0x76).

Pull-ups I2C: 2x 4.7k ohm vers 3.3V (R5 sur SDA, R6 sur SCL)
Condensateur de decouplage: C1 = 100nF sur le rail 3.3V

### Bus SPI (Module SD)
| Signal | Arduino Nano Pin | SD Module |
|--------|-----------------|-----------|
| GND    | GND             | GND (pin 1) |
| VCC    | 5V              | VCC (pin 2) |
| MISO   | D12             | MISO (pin 3) |
| MOSI   | D11             | MOSI (pin 4) |
| SCK    | D13             | SCK (pin 5)  |
| CS     | D10             | CS (pin 6)   |

### Capteur ultrason HC-SR04
| Signal | Arduino Nano Pin | HC-SR04 |
|--------|-----------------|---------|
| TRIG   | D2              | TRIG    |
| ECHO   | D3              | ECHO    |
| VCC    | 5V              | VCC     |
| GND    | GND             | GND     |

> Mesure de la vitesse du son (distance fixe connue, 50 cm par défaut).

### LEDs d'état (4x)
| LED              | Arduino Pin | Résistance | Fonction                         |
|------------------|-------------|------------|----------------------------------|
| D1 — LED_SD     | D4          | R1 (220Ω)  | Carte SD prête (allumée = OK)    |
| D2 — LED_SENSOR | D5          | R2 (220Ω)  | Capteurs initialisés (allumée)   |
| D3 — LED_LOG    | D6          | R3 (220Ω)  | Clignote à chaque écriture SD    |
| D4 — LED_ERROR  | D7          | R4 (220Ω)  | Erreur détectée                  |

Chaque LED : Dx -> Rx (220 ohm) -> LED -> GND

### Autres
| Composant              | Connexion                          |
|------------------------|------------------------------------|
| Interrupteur ON/OFF (SW1) | BATT+ -> Switch -> VIN          |
| Batterie (J1 JST)      | + -> SW1, - -> GND                |

## Résumé des pins Arduino Nano utilisées

| Pin  | Fonction           |
|------|--------------------|
| D0   | RX (libre)         |
| D1   | TX (libre)         |
| D2   | HC-SR04 TRIG       |
| D3   | HC-SR04 ECHO       |
| D4   | LED_SD             |
| D5   | LED_SENSOR         |
| D6   | LED_LOG            |
| D7   | LED_ERROR          |
| D8   | —                  |
| D9   | —                  |
| D10  | SD CS (SPI)        |
| D11  | SD MOSI (SPI)      |
| D12  | SD MISO (SPI)      |
| D13  | SD SCK (SPI)       |
| A4   | SDA (I2C)          |
| A5   | SCL (I2C)          |
| VIN  | Batterie +         |
| 5V   | MPU9250, SD, HC-SR04 |
| 3.3V | BMP280             |
| GND  | Commun             |

## Dimensions du PCB
- Taille: 80mm x 65mm (largeur x hauteur)
- 4 trous de montage M3 (3.2mm)
- 2 couches (F.Cu / B.Cu)

## Composants
| Ref  | Valeur          | Empreinte                    |
|------|-----------------|------------------------------|
| A1   | Arduino Nano v3 | 2x15 pin headers (2.54mm)    |
| U1   | BMP280          | 1x6 pin header (2.54mm)      |
| U2   | MPU9250         | 1x10 pin header (2.54mm)     |
| U3   | SD Module       | 1x6 pin header (2.54mm)      |
| U4   | HC-SR04         | 1x4 pin header (2.54mm)      |
| D1   | LED Green (SD)  | LED 3mm THT                  |
| D2   | LED Green (SENS)| LED 3mm THT                  |
| D3   | LED Yellow (LOG)| LED 3mm THT                  |
| D4   | LED Red (ERROR) | LED 3mm THT                  |
| R1   | 220 ohm         | Axial THT (LED_SD)           |
| R2   | 220 ohm         | Axial THT (LED_SENSOR)       |
| R3   | 220 ohm         | Axial THT (LED_LOG)          |
| R4   | 220 ohm         | Axial THT (LED_ERROR)        |
| R5   | 4.7k ohm        | Axial THT (I2C SDA pull-up)  |
| R6   | 4.7k ohm        | Axial THT (I2C SCL pull-up)  |
| C1   | 100nF           | Disc ceramique THT           |
| SW1  | ON/OFF Switch   | Toggle SPST THT                |
| J1   | JST PH 2pin     | Connecteur batterie          |

## Instructions
1. Ouvrir PCB_Support.kicad_pro dans KiCad 7+
2. Vérifier le schéma (PCB_Support.kicad_sch)
3. Ajuster le routage dans le PCB editor si nécessaire
4. Lancer le DRC (Design Rule Check) avant fabrication
5. Générer les fichiers Gerber pour la production

## Correspondance avec le firmware ADASTRA v2
```
constexpr uint8_t SD_CS_PIN       = 10;   // → U3 pin CS
constexpr uint8_t TRIG_PIN        = 2;    // → U4 pin TRIG
constexpr uint8_t ECHO_PIN        = 3;    // → U4 pin ECHO
constexpr uint8_t LED_SD_PIN      = 4;    // → D1 via R1
constexpr uint8_t LED_SENSOR_PIN  = 5;    // → D2 via R2
constexpr uint8_t LED_LOG_PIN     = 6;    // → D3 via R3
constexpr uint8_t LED_ERROR_PIN   = 7;    // → D4 via R4
```

## Notes
- Les modules breakout se soudent directement sur les pin headers
- Le MPU9250 est alimenté en 5V (module breakout avec régulateur intégré)
- Le BMP280 est alimenté en 3.3V (tension de fonctionnement native, 6 pins : VCC/GND/SCL/SDA/CSB/SDO)
- CSB du BMP280 connecté à V3V3 (active le mode I2C)
- SDO du BMP280 connecté à GND (adresse I2C = 0x76)
- Le HC-SR04 nécessite une distance fixe connue (cible réfléchissante à 50 cm)
- Adresse I2C MPU9250 : 0x68 (AD0 = GND, par défaut)
- Adresse I2C BMP280 : 0x76
- Vérifiez le pinout exact de vos modules breakout avant soudure !
