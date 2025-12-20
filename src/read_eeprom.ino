/****************************************************
 *  ADASTRA - EEPROM Reader
 *  Export CSV
 *  Club NOVA CNAM
 *
 *  Auteur : Dylan Perinetti
 ****************************************************/

#include <Wire.h>

#define EEPROM_ADDR 0x50
#define EEPROM_SIZE 32768

struct LogData {
  uint32_t timeMs;

  int16_t ax;
  int16_t ay;
  int16_t az;

  int16_t gx;
  int16_t gy;
  int16_t gz;

  uint16_t pressure;
  int16_t altitude;
  uint16_t distance;
};

unsigned int addr = 0;

void eepromReadStruct(unsigned int address, LogData &data) {
  byte *ptr = (byte *)&data;

  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(address >> 8);
  Wire.write(address & 0xFF);
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_ADDR, sizeof(LogData));

  for (unsigned int i = 0; i < sizeof(LogData); i++) {
    if (Wire.available()) {
      ptr[i] = Wire.read();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("time_ms,ax,ay,az,gx,gy,gz,pressure_Pa,altitude_m,distance_cm");
}

void loop() {
  if (addr + sizeof(LogData) > EEPROM_SIZE) {
    while (1);
  }

  LogData log;
  eepromReadStruct(addr, log);

  // Arrêt si données vides
  if (log.timeMs == 0xFFFFFFFF || log.timeMs == 0) {
    while (1);
  }

  Serial.print(log.timeMs); Serial.print(",");
  Serial.print(log.ax / 100.0); Serial.print(",");
  Serial.print(log.ay / 100.0); Serial.print(",");
  Serial.print(log.az / 100.0); Serial.print(",");
  Serial.print(log.gx / 100.0); Serial.print(",");
  Serial.print(log.gy / 100.0); Serial.print(",");
  Serial.print(log.gz / 100.0); Serial.print(",");
  Serial.print(log.pressure * 10); Serial.print(",");
  Serial.print(log.altitude / 10.0); Serial.print(",");
  Serial.println(log.distance);

  addr += sizeof(LogData);
  delay(5);
}
