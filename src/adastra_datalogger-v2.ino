/****************************************************
 *  ADASTRA (FuséEx) - Data Logger V2
 *  Club NOVA CNAM pour le CSPACE 2026
 *
 *  Capteurs :
 *   - MPU6050 (Accéléromètre + Gyroscope)  → I2C (SDA=A4, SCL=A5)
 *   - BMP280  (Pression / Altitude / Temp)  → I2C (SDA=A4, SCL=A5)
 *   - HC-SR04 (Vitesse du son)              → TRIG=D2, ECHO=D3
 *
 *  Stockage :
 *   - Carte micro-SD (SPI) → CS=D10, MOSI=D11, MISO=D12, SCK=D13
 *
 *  LEDs d'état :
 *   - LED_SD     (D4) : carte SD prête
 *   - LED_SENSOR (D5) : capteurs initialisés
 *   - LED_LOG    (D6) : clignote à chaque écriture
 *   - LED_ERROR  (D7) : erreur détectée
 *
 *  Auteur : Dylan Perinetti
 ****************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

/* ===================== CONFIG ===================== */

constexpr uint8_t SD_CS_PIN       = 10;            // Chip select micro-SD (Nano = D10)
constexpr uint8_t TRIG_PIN        = 2;
constexpr uint8_t ECHO_PIN        = 3;

/* --- LEDs d'état --- */
constexpr uint8_t LED_SD_PIN      = 4;             // SD prête (allumée = OK)
constexpr uint8_t LED_SENSOR_PIN  = 5;             // Capteurs OK (allumée = OK)
constexpr uint8_t LED_LOG_PIN     = 6;             // Blink à chaque écriture
constexpr uint8_t LED_ERROR_PIN   = 7;             // Erreur détectée

constexpr float   SEA_LEVEL_HPA   = 1013.25;       // hPa (à ajuster le jour du vol)
constexpr unsigned long LOG_PERIOD_MS = 20;         // ~50 Hz
constexpr unsigned long ECHO_TIMEOUT_US = 30000;    // timeout pulseIn (µs)

/* --- HC-SR04 : distance fixe connue --- */
constexpr float FIXED_DISTANCE_CM = 50.0f;          // distance fixe capteur-cible (cm) — À AJUSTER

static const char FILENAME[] = "ADASTRA.CSV";      // fichier sur la carte SD

/* ===================== OBJETS ===================== */

Adafruit_MPU6050 mpu;
Adafruit_BMP280  bmp;

bool sdReady    = false;   // carte SD disponible ?
bool sensorsOk  = false;   // capteurs initialisés ?

/* ===================== HC-SR04 ===================== */
// Distance fixe connue → on mesure le temps aller-retour
// pour en déduire la vitesse de propagation du signal (m/s).

float readSoundSpeed() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration_us = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration_us == 0) return 0.0f;            // pas d'écho reçu

  // vitesse = 2 * distance / temps   (aller-retour)
  // distance en m, temps en s
  float distance_m = FIXED_DISTANCE_CM / 100.0f;
  float time_s     = duration_us / 1000000.0f;
  return (2.0f * distance_m) / time_s;           // m/s
}

/* ===================== LEDs ===================== */

void blinkLed(uint8_t pin) {
  digitalWrite(pin, HIGH);
  // La LED s'éteindra au prochain cycle
}

void errorBlink() {
  // Clignotement rapide de la LED erreur
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(LED_ERROR_PIN, HIGH);
    delay(100);
    digitalWrite(LED_ERROR_PIN, LOW);
    delay(100);
  }
}

/* ===================== SETUP ===================== */

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  /* --- LEDs --- */
  pinMode(LED_SD_PIN,     OUTPUT);
  pinMode(LED_SENSOR_PIN, OUTPUT);
  pinMode(LED_LOG_PIN,    OUTPUT);
  pinMode(LED_ERROR_PIN,  OUTPUT);

  // Toutes éteintes au départ
  digitalWrite(LED_SD_PIN,     LOW);
  digitalWrite(LED_SENSOR_PIN, LOW);
  digitalWrite(LED_LOG_PIN,    LOW);
  digitalWrite(LED_ERROR_PIN,  LOW);

  /* --- Carte micro-SD --- */
  if (SD.begin(SD_CS_PIN)) {
    sdReady = true;
    digitalWrite(LED_SD_PIN, HIGH);               // SD OK → LED allumée
    Serial.println(F("SD OK"));

    // Écrire l'en-tête CSV si le fichier n'existe pas encore
    if (!SD.exists(FILENAME)) {
      File f = SD.open(FILENAME, FILE_WRITE);
      if (f) {
        f.println(F("time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,temp_C,sound_speed_ms"));
        f.close();
      }
    }
  } else {
    Serial.println(F("WARN: carte SD absente — données série uniquement"));
    errorBlink();
  }

  /* --- MPU6050 --- */
  if (!mpu.begin()) {
    Serial.println(F("ERR: MPU6050 introuvable"));
    digitalWrite(LED_ERROR_PIN, HIGH);
    while (true) delay(500);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  /* --- BMP280 --- */
  if (!bmp.begin(0x76)) {
    Serial.println(F("ERR: BMP280 introuvable"));
    digitalWrite(LED_ERROR_PIN, HIGH);
    while (true) delay(500);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // température
                  Adafruit_BMP280::SAMPLING_X16,  // pression
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_1);

  sensorsOk = true;
  digitalWrite(LED_SENSOR_PIN, HIGH);             // Capteurs OK → LED allumée

  Serial.println(F("================================="));
  Serial.println(F("  ADASTRA V2 - DATA LOGGER PRET"));
  Serial.println(F("  Club NOVA CNAM — micro-SD"));
  Serial.print(F("  Distance fixe HC-SR04 : "));
  Serial.print(FIXED_DISTANCE_CM);
  Serial.println(F(" cm"));
  Serial.println(F("================================="));

  // En-tête série (même format que le CSV)
  Serial.println(F("time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,temp_C,sound_speed_ms"));
}

/* ===================== LOOP ===================== */

void loop() {
  unsigned long t0 = millis();

  digitalWrite(LED_LOG_PIN, LOW);                 // éteindre le blink précédent

  /* --- Lecture capteurs --- */
  sensors_event_t accel, gyro, temp_event;
  mpu.getEvent(&accel, &gyro, &temp_event);

  float pressure   = bmp.readPressure();           // Pa
  float altitude   = bmp.readAltitude(SEA_LEVEL_HPA); // m
  float tempC      = bmp.readTemperature();         // °C

  float soundSpeed = readSoundSpeed();              // m/s (0 si pas d'écho)

  /* --- Construire la ligne CSV --- */
  char line[180];
  snprintf(line, sizeof(line),
    "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.2f,%.2f,%.2f",
    t0,
    accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
    gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
    pressure, altitude, tempC,
    soundSpeed);

  /* --- Sortie série --- */
  Serial.println(line);

  /* --- Écriture carte SD --- */
  if (sdReady) {
    File f = SD.open(FILENAME, FILE_WRITE);
    if (f) {
      f.println(line);
      f.close();
      blinkLed(LED_LOG_PIN);                       // flash à chaque écriture SD
    }
  }

  /* --- Compenser le temps d'exécution pour garder ~50 Hz --- */
  unsigned long elapsed = millis() - t0;
  if (elapsed < LOG_PERIOD_MS) {
    delay(LOG_PERIOD_MS - elapsed);
  }
}
