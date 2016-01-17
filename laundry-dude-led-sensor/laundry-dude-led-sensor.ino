#include <SoftwareSerial.h>

// Serial speed
#define SERIAL_BAUDRATE 9600
#define LIGHTSENSOR_PIN 3

#define FETCH_INTERVAL 100
#define POST_INTERVAL 5000

const int numLedSamples = POST_INTERVAL / FETCH_INTERVAL;
int ledSamples[numLedSamples];
int ledSampleCounter = 0;

int light = 0;

// Software serial for XBee module
SoftwareSerial xbeeSerial(2, 3); // RX, TX

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  xbeeSerial.begin(SERIAL_BAUDRATE);
}

void loop() {
  readLightsensor();

  if (ledSampleCounter >= numLedSamples)
    sendLightData();

  delay(FETCH_INTERVAL);
}

void readLightsensor() {
  ledSamples[ledSampleCounter] = analogRead(LIGHTSENSOR_PIN);
  ledSampleCounter++;
  ledSampleCounter %= numLedSamples;
}

void sendLightData() {
  float avg = 0.0f;

  for (int i = 0; i < numLedSamples; i++)
    avg += ledSamples[i];

  avg /= numLedSamples;

  xbeeSerial.print("l=");xbeeSerial.println(avg);
}
