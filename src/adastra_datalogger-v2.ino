/****************************************************
 *  ADASTRA (FuséEx) - Data Logger V2  (MPU9250)
 *  Club NOVA CNAM pour le CSPACE 2026
 *
 *  Capteurs :
 *   - MPU9250 (Accél.+Gyro+Magnéto) → I2C (SDA=A4, SCL=A5), VCC=3.3V*, GND
 *   - BMP280  (Press./Alt.)         → I2C (SDA=A4, SCL=A5), VCC=3.3V, GND
 *   - HC-SR04 (Vitesse son)         → TRIG=D2, ECHO=D3, VCC=5V, GND
 *
 *   * Attention : le chip MPU9250 est 3.3V. La plupart des modules
 *     (GY-9250 / GY-91) intègrent un régulateur + adaptateur de niveau
 *     et acceptent 3.3-5V — vérifier votre carte avant d'alimenter en 5V.
 *
 *  Stockage :
 *   - Carte micro-SD (SPI) → CS=D10, MOSI=D11, MISO=D12, SCK=D13,
 *                             VCC=5V, GND
 *   - Version "vol" : fichier gardé ouvert + flush périodique + rotation
 *     de fichier (VOL00.CSV, VOL01.CSV, ...) + reconnexion automatique.
 *
 *  LEDs d'état :
 *   - LED_SD     (D4) : carte SD prête + fichier ouvert (allumée = OK)
 *   - LED_SENSOR (D5) : capteurs initialisés (allumée = OK)
 *   - LED_LOG    (D6) : "heartbeat" — bascule à chaque flush (~250 ms)
 *   - LED_ERROR  (D7) : erreur (capteur fatal, ou SD absente/éjectée)
 *
 *  Bibliothèque IMU : "MPU9250" by Bolder Flight Systems
 *    (Arduino IDE → Gérer les bibliothèques → chercher "Bolder Flight MPU9250")
 *
 *  Auteur : Dylan Perinetti
 ****************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MPU9250.h>            // Bolder Flight Systems
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

/* ===================== CONFIG ===================== */

constexpr uint8_t SD_CS_PIN       = 10;            // Chip select micro-SD (Nano = D10)
constexpr uint8_t TRIG_PIN        = 2;
constexpr uint8_t ECHO_PIN        = 3;

constexpr uint8_t MPU9250_ADDR    = 0x68;          // 0x68 (AD0=GND) ou 0x69 (AD0=VCC)

/* --- LEDs d'état --- */
constexpr uint8_t LED_SD_PIN      = 4;             // SD prête (allumée = OK)
constexpr uint8_t LED_SENSOR_PIN  = 5;             // Capteurs OK (allumée = OK)
constexpr uint8_t LED_LOG_PIN     = 6;             // Heartbeat (bascule à chaque flush)
constexpr uint8_t LED_ERROR_PIN   = 7;             // Erreur détectée

constexpr float   SEA_LEVEL_HPA   = 1013.25;       // hPa (à ajuster le jour du vol)
constexpr unsigned long LOG_PERIOD_MS = 20;         // ~50 Hz
constexpr unsigned long ECHO_TIMEOUT_US = 30000;    // timeout pulseIn (µs)

/* --- HC-SR04 : distance fixe connue --- */
constexpr float FIXED_DISTANCE_CM = 50.0f;          // distance fixe capteur-cible (cm) — À AJUSTER

/* --- Stockage "vol" --- */
constexpr unsigned long FLUSH_PERIOD_MS    = 250;   // commit sur la carte tous les 250 ms
constexpr unsigned long SD_RETRY_PERIOD_MS = 1000;  // tentative de reconnexion SD
static const char LOG_PREFIX[] = "VOL";             // → VOL00.CSV, VOL01.CSV, ...

// En-tête CSV (utilisé pour le fichier ET la sortie série, défini une seule fois)
#define CSV_HEADER "time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,temp_C,sound_speed_ms,mag_x_uT,mag_y_uT,mag_z_uT"

/* ===================== OBJETS ===================== */

MPU9250          mpu(Wire, MPU9250_ADDR);          // IMU 9 axes sur le bus I2C
Adafruit_BMP280  bmp;

File          logFile;                  // fichier de log gardé ouvert pendant tout le vol
char          logFilename[13] = "";     // nom 8.3 choisi (ex: "VOL00.CSV")

bool sdReady    = false;   // carte SD + fichier ouvert ?
bool sensorsOk  = false;   // capteurs initialisés ?

unsigned long lastFlushMs   = 0;
unsigned long lastSdRetryMs = 0;
bool          logLedState   = false;    // état courant du heartbeat LED_LOG

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

/* ===================== STOCKAGE SD ===================== */

// Cherche le premier nom de fichier libre (VOL00.CSV .. VOL99.CSV),
// l'ouvre et écrit l'en-tête CSV. Renvoie true si OK.
bool startNewLog() {
  for (uint8_t i = 0; i < 100; i++) {
    snprintf(logFilename, sizeof(logFilename), "%s%02u.CSV", LOG_PREFIX, (unsigned)i);
    if (!SD.exists(logFilename)) {
      logFile = SD.open(logFilename, FILE_WRITE);
      if (!logFile) return false;
      logFile.println(F(CSV_HEADER));
      logFile.flush();
      return true;
    }
  }
  // Les 100 noms sont pris : on rouvre le dernier en ajout
  logFile = SD.open(logFilename, FILE_WRITE);
  return (bool)logFile;
}

// Rouvre le fichier courant en mode ajout (après une reconnexion).
// Ne réécrit PAS l'en-tête : on continue le même vol.
bool reopenLog() {
  logFile = SD.open(logFilename, FILE_WRITE);   // FILE_WRITE = ajout
  return (bool)logFile;
}

// Bascule l'état "logging OK" / "erreur SD" et pilote les LEDs.
void setSdOk(bool ok) {
  sdReady = ok;
  digitalWrite(LED_SD_PIN,    ok ? HIGH : LOW);
  digitalWrite(LED_ERROR_PIN, ok ? LOW  : HIGH);
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
  if (SD.begin(SD_CS_PIN) && startNewLog()) {
    setSdOk(true);                                // LED_SD allumée, LED_ERROR éteinte
    Serial.print(F("SD OK -> "));
    Serial.println(logFilename);
  } else {
    setSdOk(false);                               // LED_ERROR allumée (erreur SD visible)
    Serial.println(F("WARN: SD indisponible — donnees serie uniquement"));
  }
  lastFlushMs   = millis();
  lastSdRetryMs = millis();

  /* --- MPU9250 --- */
  // begin() renvoie un statut : > 0 = OK, < 0 = erreur
  int imuStatus = mpu.begin();
  if (imuStatus < 0) {
    Serial.print(F("ERR: MPU9250 introuvable (status="));
    Serial.print(imuStatus);
    Serial.println(F(")"));
    digitalWrite(LED_ERROR_PIN, HIGH);
    while (true) delay(500);
  }
  // Pleines échelles équivalentes à l'ancienne config MPU6050
  mpu.setAccelRange(MPU9250::ACCEL_RANGE_16G);     // ±16 g
  mpu.setGyroRange(MPU9250::GYRO_RANGE_2000DPS);   // ±2000 °/s
  mpu.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ); // ~21 Hz (valeur la plus proche)
  // SRD : 1000 Hz / (1 + srd). srd=19 → 50 Hz interne (cohérent avec ~50 Hz de log)
  mpu.setSrd(19);

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
  Serial.println(F(CSV_HEADER));
}

/* ===================== LOOP ===================== */

void loop() {
  unsigned long t0 = millis();

  /* --- Lecture capteurs --- */
  mpu.readSensor();                                // met à jour accél., gyro et magnéto

  float ax = mpu.getAccelX_mss();                  // m/s²
  float ay = mpu.getAccelY_mss();
  float az = mpu.getAccelZ_mss();
  float gx = mpu.getGyroX_rads();                  // rad/s
  float gy = mpu.getGyroY_rads();
  float gz = mpu.getGyroZ_rads();
  float mx = mpu.getMagX_uT();                     // µT
  float my = mpu.getMagY_uT();
  float mz = mpu.getMagZ_uT();

  float pressure   = bmp.readPressure();           // Pa
  float altitude   = bmp.readAltitude(SEA_LEVEL_HPA); // m
  float tempC      = bmp.readTemperature();         // °C

  float soundSpeed = readSoundSpeed();              // m/s (0 si pas d'écho)

  /* --- Construire la ligne CSV --- */
  char line[220];
  snprintf(line, sizeof(line),
    "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
    t0,
    ax, ay, az,
    gx, gy, gz,
    pressure, altitude, tempC,
    soundSpeed,
    mx, my, mz);

  /* --- Sortie série (toujours, même sans carte) --- */
  Serial.println(line);

  /* --- Écriture carte SD (fichier gardé ouvert) --- */
  if (sdReady) {
    size_t n = logFile.println(line);
    if (n == 0) {
      // Écriture rejetée → carte probablement éjectée
      logFile.close();
      setSdOk(false);                              // LED_ERROR allumée
      Serial.println(F("ERR: ecriture SD echouee (carte ejectee ?)"));
      lastSdRetryMs = millis();
    } else if (millis() - lastFlushMs >= FLUSH_PERIOD_MS) {
      logFile.flush();                             // commit périodique sur la carte
      lastFlushMs = millis();
      logLedState = !logLedState;                  // heartbeat visible
      digitalWrite(LED_LOG_PIN, logLedState);
    }
  } else {
    // Pas de SD : tentative de reconnexion périodique
    if (millis() - lastSdRetryMs >= SD_RETRY_PERIOD_MS) {
      lastSdRetryMs = millis();
      if (SD.begin(SD_CS_PIN) && (logFilename[0] ? reopenLog() : startNewLog())) {
        setSdOk(true);                             // LED_SD allumée, LED_ERROR éteinte
        Serial.print(F("SD (re)connectee -> "));
        Serial.println(logFilename);
        lastFlushMs = millis();
      }
    }
  }

  /* --- Compenser le temps d'exécution pour garder ~50 Hz --- */
  unsigned long elapsed = millis() - t0;
  if (elapsed < LOG_PERIOD_MS) {
    delay(LOG_PERIOD_MS - elapsed);
  }
}
