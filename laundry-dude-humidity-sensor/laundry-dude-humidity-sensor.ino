#include <Wire.h>
#include <SoftwareSerial.h>

// BME280 API
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"


// Serial speed
#define SERIAL_BAUDRATE 9600
#define FETCH_INTERVAL 100

// Struct carrying BME280 data
struct Bme280Data {
  float humidity = NAN;
  float pressure = NAN;
  float temperature = NAN;
};

struct Bme280Data bme280Data;

// Sensor driver for the BME280
Adafruit_BME280 bme;

// Software serial for XBee module
SoftwareSerial xbeeSerial(2, 3); // RX, TX

void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  };

  xbeeSerial.begin(SERIAL_BAUDRATE);
}

void loop() {
  readHumPressTemp();

  if (xbeeSerial.available()) {
    switch (xbeeSerial.read()) {
    case 'h':
      xbeeSerial.print("h=");xbeeSerial.println(bme280Data.humidity);
      break;
    case 't':
      xbeeSerial.print("t=");xbeeSerial.println(bme280Data.temperature);
      break;
    }
  } else
    delay(FETCH_INTERVAL);
}

void readHumPressTemp() {
  bme280Data.humidity = bme.readHumidity();
  // Read and convert to hPa
  bme280Data.pressure = bme.readPressure() / 100;
  bme280Data.temperature = bme.readTemperature();
}
