#include <Wire.h>
#include <SoftwareSerial.h>

// BME280 API
#include "Adafruit_Sensor.h"
#include "Adafruit_BME280.h"


// Serial speed
#define SERIAL_BAUDRATE 9600

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

  Serial.print("h=");Serial.println(bme280Data.humidity);
  xbeeSerial.print("h=");xbeeSerial.println(bme280Data.humidity);

  delay(10000);
}

void readHumPressTemp() {
  bme280Data.humidity = bme.readHumidity();
  // Read and convert to hPa
  bme280Data.pressure = bme.readPressure() / 100;
  bme280Data.temperature = bme.readTemperature();
}
