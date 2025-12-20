/****************************************************
 *  ADASTRA (FuséEx) - Data Logger V1
 *  Club NOVA CNAM pour le CSPACE 2026
 *
 *  Capteurs :
 *   - MPU6050 (Accéléromètre + Gyroscope)
 *   - BMP280  (Pression / Altitude)
 *   - HC-SR04 (Distance sol / atterrissage)
 *
 *  Stockage :
 *   - EEPROM I2C 24LC256 (32 Ko)
 *
 *  Auteur : Dylan Perinetti
 ****************************************************/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

/* ===================== CONFIG ===================== */

#define EEPROM_ADDR 0x50

#define TRIG_PIN 2
#define ECHO_PIN 3

#define SEA_LEVEL_PRESSURE 1013.25 // hPa
#define LOG_FREQUENCY_MS 20        // ~50 Hz

/* ===================== OBJETS ===================== */

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

/* ===================== STRUCTURE DATA ===================== */
/*
 * Taille totale : 22 octets
 */
struct LogData {
  uint32_t timeMs;

  int16_t ax;
  int16_t ay;
  int16_t az;

  int16_t gx;
  int16_t gy;
  int16_t gz;

  uint16_t pressure; // Pa / 10
  int16_t altitude;  // m / 10
  uint16_t distance; // cm
};

unsigned int eepromAddress = 0;

/* ===================== EEPROM ===================== */

void eepromWriteStruct(unsigned int addr, LogData &data) {
  byte *ptr = (byte *)&data;

  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(addr >> 8);
  Wire.write(addr & 0xFF);

  for (unsigned int i = 0; i < sizeof(LogData); i++) {
    Wire.write(ptr[i]);
  }

  Wire.endTransmission();
  delay(10); // temps d'écriture EEPROM
}

/* ===================== HC-SR04 ===================== */

uint16_t readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30 ms
  if (duration == 0) return 0;

  return duration * 0.034 / 2;
}

/* ===================== SETUP ===================== */

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  /* MPU6050 */
  if (!mpu.begin()) {
    Serial.println("ERREUR : MPU6050 non detecte");
    while (1);
  }

  /* BMP280 */
  if (!bmp.begin(0x76)) {
    Serial.println("ERREUR : BMP280 non detecte");
    while (1);
  }

  Serial.println("=================================");
  Serial.println("  ADASTRA - DATA LOGGER PRET");
  Serial.println("  Club NOVA CNAM");
  Serial.println("=================================");
}

/* ===================== LOOP ===================== */

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  LogData log;

  log.timeMs = millis();

  // Accélération (m/s² * 100)
  log.ax = accel.acceleration.x * 100;
  log.ay = accel.acceleration.y * 100;
  log.az = accel.acceleration.z * 100;

  // Gyroscope (rad/s * 100)
  log.gx = gyro.gyro.x * 100;
  log.gy = gyro.gyro.y * 100;
  log.gz = gyro.gyro.z * 100;

  // Pression & altitude
  log.pressure = bmp.readPressure() / 10; // Pa / 10
  log.altitude = bmp.readAltitude(SEA_LEVEL_PRESSURE) * 10;

  // Distance sol
  log.distance = readDistance();

  // Écriture EEPROM
  eepromWriteStruct(eepromAddress, log);
  eepromAddress += sizeof(LogData);

  // Debug sol uniquement
  Serial.print("LOG @ ");
  Serial.print(log.timeMs);
  Serial.print(" ms | Alt: ");
  Serial.print(log.altitude / 10.0);
  Serial.println(" m");

  delay(LOG_FREQUENCY_MS);
}
