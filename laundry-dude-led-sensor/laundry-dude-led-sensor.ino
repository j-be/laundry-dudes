#include <Wire.h>
#include <SoftwareSerial.h>

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Serial speed
#define SERIAL_BAUDRATE 9600
#define LIGHTSENSOR_PIN 3

#define FETCH_INTERVAL 100
#define POST_INTERVAL 5000

#define TAG_LENGTH 12

const int numLedSamples = POST_INTERVAL / FETCH_INTERVAL;
int ledSamples[numLedSamples];
int ledSampleCounter = 0;

int light = 0;

MPU6050 mpu;
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64];

char rfidTag[TAG_LENGTH + 1];
char lockTag[TAG_LENGTH + 1];
bool locked = false;
unsigned long lastRead = 0;

char buffer[TAG_LENGTH + 1];
int count = 0;

// Software serial for XBee module
SoftwareSerial xbeeSerial(2, 3); // RX, TX
// Hardware serial for RFID reader
#define rfidSerial Serial

void setup() {
  // Setup serials
  xbeeSerial.begin(SERIAL_BAUDRATE);
  rfidSerial.begin(SERIAL_BAUDRATE);

  // Setup lock
  pinMode(A0, OUTPUT);
  setLockState(false);

  // Setup Accelerometer
  initMpu();
}

bool readRfid() {
  bool tagChanged = false;
  bool ret = false;
  char tmp;

  while (rfidSerial.available()) {
    while (rfidSerial.available() && count == -1) {
      tmp = rfidSerial.read();
      Serial.println((int)tmp);
      if (tmp == 2) {
        count++;
        buffer[TAG_LENGTH] = 0;
        Serial.println("Got start");
      }
    }

    while (count > -1 && count < TAG_LENGTH && rfidSerial.available()) {
      tmp = rfidSerial.read();
      if (tmp == 2) {
        count = 0;
        return false;
      }
      buffer[count] = tmp;
      count++;
    }

    if (count == TAG_LENGTH) {
      count = 0;

      if (rfidSerial.read() != 3)
        return false;

      if (!checkRfidChecksum(buffer))
        return false;

      memcpy(rfidTag, buffer, TAG_LENGTH);
      rfidTag[TAG_LENGTH] = 0;
      return true;
    }
  }
  return false;
}

bool compareReadTag(const char* otherTag) {
  bool ret = true;

  for (int i = 0; i < TAG_LENGTH; i++) {
    ret &= otherTag[i] == rfidTag[i];
  }

  return ret;
}

bool checkRfidChecksum(const char* buffer) {
  uint16_t checkSum = 0;
  char checkSum_L = 0;
  char checkSum_H = 0;
  String hexstr = "";
  uint16_t tmp = 0;

  // Calculate
  for (int i = 0; i < (TAG_LENGTH - 2) / 2; i++) {
    hexstr = "0x";
    hexstr += buffer[2*i];
    hexstr += buffer[2*i + 1];

    tmp = strtoul(hexstr.c_str(), 0, 16);
    checkSum = checkSum ^ tmp;
  }

  // Check
  hexstr = "0x";
  hexstr += buffer[TAG_LENGTH - 2];
  hexstr += buffer[TAG_LENGTH - 1];
  tmp = strtoul(hexstr.c_str(), 0, 16);//hexstr.toInt();
  return checkSum == tmp;
}

void setLockState(bool newState) {
  digitalWrite(A0, newState);
  locked = newState;

  // Clear lock Tag
  if (!newState)
    for (int i = 0; i < TAG_LENGTH; i++)
      lockTag[i] = '#';

}

void processRfid() {
  if (!readRfid())
    return;

  // FIXME: Runs over after 50 days!
  if (millis() - lastRead < 1000) {
    lastRead = millis();
    return;
  }
  lastRead = millis();

  if (locked) {
    bool tagValid = compareReadTag(lockTag);
    if (tagValid) {
      Serial.print("Tag valid: "); Serial.println(rfidTag);
    } else {
      Serial.print("Tag invalid: "); Serial.println(rfidTag);
    }
    setLockState(!tagValid);
  } else {
    setLockState(true);
    memcpy(lockTag, rfidTag, TAG_LENGTH + 1);
    Serial.print("Locked with tag: "); Serial.println(lockTag);
  }
}

void loop() {
  processRfid();
  readLightsensor();
  readDataFromMpu();

  if (xbeeSerial.available()) {
    switch (xbeeSerial.read()) {
    case 'l':
      sendLightData();
      break;
    case 'a':
      xbeeSerial.print("a=");xbeeSerial.println(aaWorld.x + aaWorld.y + aaWorld.z);
      break;
    case 'b':
      while (!xbeeSerial.available());
      switch (xbeeSerial.read()) {
        case '1':
          setLockState(true);
          break;
        case '0':
          setLockState(false);
          break;
      }
      break;
    case 'r':
      // FIXME: This is highly dangerous
      xbeeSerial.print("r=");xbeeSerial.println(lockTag);
      break;
    }
  }
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

void initMpu() {
  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();

  // verify connection
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // load and configure the DMP
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for request"));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
    mpu.resetFIFO();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

}

void checkFifoOnMpu() {
  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    Serial.println(F("FIFO overflow!"));
    // reset so we can continue cleanly
    mpu.resetFIFO();
    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & 0x02 && fifoCount < packetSize)
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
}

void readDataFromMpu() {
  mpuIntStatus = mpu.getIntStatus();

  if (mpuIntStatus & 0x02) {
    checkFifoOnMpu();

    // Read all packets from FIFO and keep last
    do {
      mpu.getFIFOBytes(fifoBuffer, packetSize);
      fifoCount -= packetSize;
    } while (fifoCount > packetSize);

    // Get Acceleration in world-frame
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
    mpu.resetFIFO();
  }
}
