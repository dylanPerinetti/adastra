/****************************************************
 *  ADASTRA (FuséEx) - Data Logger V2
 *  Club NOVA CNAM pour le CSPACE 2026
 *
 *  Capteurs :
 *   - MPU6050 (Accéléromètre + Gyroscope)
 *   - BMP280  (Pression / Altitude / Température)
 *   - HC-SR04 (Distance sol / atterrissage)
 *
 *  Stockage :
 *   - Carte micro-SD via module SPI
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

constexpr float   SEA_LEVEL_HPA   = 1013.25;       // hPa (à ajuster le jour du vol)
constexpr unsigned long LOG_PERIOD_MS = 20;         // ~50 Hz
constexpr unsigned long ECHO_TIMEOUT_US = 30000;    // timeout pulseIn (µs)

static const char FILENAME[] = "ADASTRA.CSV";      // fichier sur la carte SD

/* ===================== OBJETS ===================== */

Adafruit_MPU6050 mpu;
Adafruit_BMP280  bmp;

bool sdReady = false;   // carte SD disponible ?

/* ===================== HC-SR04 ===================== */

uint16_t readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return 0;

  return (uint16_t)(duration * 0.034f / 2.0f);  // cm
}

/* ===================== SETUP ===================== */

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  /* --- Carte micro-SD --- */
  if (SD.begin(SD_CS_PIN)) {
    sdReady = true;
    Serial.println(F("SD OK"));

    // Écrire l'en-tête CSV si le fichier n'existe pas encore
    if (!SD.exists(FILENAME)) {
      File f = SD.open(FILENAME, FILE_WRITE);
      if (f) {
        f.println(F("time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,temp_C,distance_cm"));
        f.close();
      }
    }
  } else {
    Serial.println(F("WARN: carte SD absente — données série uniquement"));
  }

  /* --- MPU6050 --- */
  if (!mpu.begin()) {
    Serial.println(F("ERR: MPU6050 introuvable"));
    while (true) delay(500);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  /* --- BMP280 --- */
  if (!bmp.begin(0x76)) {
    Serial.println(F("ERR: BMP280 introuvable"));
    while (true) delay(500);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // température
                  Adafruit_BMP280::SAMPLING_X16,  // pression
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_1);

  Serial.println(F("================================="));
  Serial.println(F("  ADASTRA V2 - DATA LOGGER PRET"));
  Serial.println(F("  Club NOVA CNAM — micro-SD"));
  Serial.println(F("================================="));

  // En-tête série (même format que le CSV)
  Serial.println(F("time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,temp_C,distance_cm"));
}

/* ===================== LOOP ===================== */

void loop() {
  unsigned long t0 = millis();

  /* --- Lecture capteurs --- */
  sensors_event_t accel, gyro, temp_event;
  mpu.getEvent(&accel, &gyro, &temp_event);

  float pressure = bmp.readPressure();           // Pa
  float altitude = bmp.readAltitude(SEA_LEVEL_HPA); // m
  float tempC    = bmp.readTemperature();         // °C

  uint16_t dist  = readDistanceCm();              // cm

  /* --- Construire la ligne CSV --- */
  char line[160];
  snprintf(line, sizeof(line),
    "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.2f,%.2f,%u",
    t0,
    accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
    gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
    pressure, altitude, tempC,
    dist);

  /* --- Sortie série --- */
  Serial.println(line);

  /* --- Écriture carte SD --- */
  if (sdReady) {
    File f = SD.open(FILENAME, FILE_WRITE);
    if (f) {
      f.println(line);
      f.close();
    }
  }

  /* --- Compenser le temps d'exécution pour garder ~50 Hz --- */
  unsigned long elapsed = millis() - t0;
  if (elapsed < LOG_PERIOD_MS) {
    delay(LOG_PERIOD_MS - elapsed);
  }
}
