/*
Based on https://github.com/mfalkvidd/arduino-plantmoisture

This sketch uses the soilmoisture "forks" only that are found on ebay. They are connected between 2 analog pins where one gets pulled low and the other one measures.
The pins are switched every time to avoid corrosion. Between readings the sensos sleeps to preserve batterylife.

By Oliver Hilsky
Documentation: http://forum.mysensors.org...

21.06.2016 V1.0 Base sketch
12.11.2016 V2.0 udpated to mysensors v2
*/

// library settings
#define MY_RADIO_NRF24
//#define MY_DEBUG    // Enables debug messages in the serial log
#define MY_BAUD_RATE  9600 // Sets the serial baud rate for console and serial gateway
#define MY_SIGNING_SOFT // Enables software signing
#define MY_SIGNING_REQUEST_SIGNATURES // Always request signing from gateway
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7 // floating pin for randomness
#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01}}} // gateway addres if you want to use whitelisting (node only works with messages from this one gateway)

#include <SPI.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <Vcc.h>

// programm settings
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define CHILD_ID_MOISTURE 0
#define CHILD_ID_BATTERY 1
#define SLEEP_TIME 1800000 // Sleep time between reads (in milliseconds) - 30 minutes
#define THRESHOLD 1.2 // Only make a new reading with reverse polarity if the change is larger than 10%.
#define STABILIZATION_TIME 500 // Let the sensor stabilize before reading
#define BATTERY_FULL 3.3 // 2xAA usually give 3.143V when full
#define BATTERY_ZERO 1.8 // 2.34V limit for 328p at 8MHz. 1.9V, limit for nrf24l01 without step-up. 2.8V limit for Atmega328 with default BOD settings.
const int SENSOR_ANALOG_PINS[] = {A1, A2}; // Sensor is connected to these two pins. Avoid A3 if using ATSHA204. A6 and A7 cannot be used because they don't have pullups.

MyMessage msg(CHILD_ID_MOISTURE, V_HUM);
MyMessage voltage_msg(CHILD_ID_BATTERY, V_VOLTAGE);
Vcc vcc(1.0);
float oldvoltage = 0;
byte direction = 0;
int oldMoistureLevel = -1;

void before() {
  // init sensor pins
  pinMode(SENSOR_ANALOG_PINS[0], OUTPUT);
  pinMode(SENSOR_ANALOG_PINS[1], OUTPUT);
  digitalWrite(SENSOR_ANALOG_PINS[0], LOW);
  digitalWrite(SENSOR_ANALOG_PINS[1], LOW);
}

void presentation()
{
  sendSketchInfo("Plant moisture", "2.0 12112016");
  present(CHILD_ID_MOISTURE, S_HUM);
  present(CHILD_ID_BATTERY, S_CUSTOM);
}

void loop()
{
  int moistureLevel = readMoisture();

  // Send rolling average of 2 samples to get rid of the "ripple" produced by different resistance in the internal pull-up resistors
  // See http://forum.mysensors.org/topic/2147/office-plant-monitoring/55 for more information
  if (oldMoistureLevel == -1) { // First reading, save current value as old
    oldMoistureLevel = moistureLevel;
  }
  if (moistureLevel > (oldMoistureLevel * THRESHOLD) || moistureLevel < (oldMoistureLevel / THRESHOLD)) {
    // The change was large, so it was probably not caused by the difference in internal pull-ups.
    // Measure again, this time with reversed polarity.
    moistureLevel = readMoisture();
  }

  // send value and reset level
  send(msg.set((moistureLevel + oldMoistureLevel) / 2.0 / 10.23, 1));
  oldMoistureLevel = moistureLevel;

  // get voltage
  float voltage = vcc.Read_Volts();
  float percentage = vcc.Read_Perc(BATTERY_ZERO, BATTERY_FULL, true);
  int batteryPcnt = static_cast<int>(percentage);

  if (oldvoltage != voltage) { // Only send battery information if voltage has changed, to conserve battery.
    send(voltage_msg.set(voltage, 3)); // Set how many decimals are used (3 in our case)
    sendBatteryLevel(batteryPcnt);
    oldvoltage = voltage;
  }

  // sleep to conserve energy
  sleep(SLEEP_TIME);
}

/*
  Reads the current moisture level from the sensor.
  Alternates the polarity to reduce corrosion
*/
int readMoisture() {
  pinMode(SENSOR_ANALOG_PINS[direction], INPUT_PULLUP); // Power on the sensor by activating the internal pullup
  analogRead(SENSOR_ANALOG_PINS[direction]);// Read once to let the ADC capacitor start charging
  sleep(STABILIZATION_TIME);

  int sensorRead = analogRead(SENSOR_ANALOG_PINS[direction]);
  int moistureLevel = (1023 - sensorRead); // take the actual reading

  Serial.print("Sensor read: ");
  Serial.println(sensorRead);
  Serial.print("Moisture level: ");  
  Serial.println(moistureLevel);  

  // Turn off the sensor to conserve battery and minimize corrosion
  pinMode(SENSOR_ANALOG_PINS[direction], OUTPUT);
  digitalWrite(SENSOR_ANALOG_PINS[direction], LOW);

  direction = (direction + 1) % 2; // Make direction alternate between 0 and 1 to reverse polarity which reduces corrosion
  return moistureLevel;
}