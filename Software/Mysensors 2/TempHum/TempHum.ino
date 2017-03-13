/* Sketch with Si7021 and battery monitoring.
by m26872, 20151109 
Changed by Oliver Hilsky

Version 1 17.05.2016 initial sketch
Version 2 13.11.2016 updated to Mysensors V2
*/


// library settings
#define MY_RADIO_NRF24
#define MY_DEBUG    // Enables debug messages in the serial log
//#define MY_DEBUG_VERBOSE_SIGNING //!< Enable signing related debug prints to serial monitor
#define MY_BAUD_RATE  9600 // Sets the serial baud rate for console and serial gateway
//#define MY_SIGNING_SOFT // Enables software signing
//#define MY_SIGNING_REQUEST_SIGNATURES // Always request signing from gateway
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7 // floating pin for randomness
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {...}}} // gateway addres if you want to use whitelisting (node only works with messages from this one gateway)

#include <SPI.h>
#include <Wire.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <Vcc.h>
#include <SparkFunHTU21D.h>
#include <RunningAverage.h>


#define CHILD_ID_TEMP 0
#define CHILD_ID_HUM 1
#define CHILD_ID_BATTERY 2
// #define SLEEP_TIME 15000 // 15s for DEBUG
#define SLEEP_TIME 300000   // 5 min
#define FORCE_TRANSMIT_CYCLE 36  // 5min*12=1 time per hour, 5min*36=1 time per 3hour 
#define BATTERY_REPORT_CYCLE 2016   // Once per 5min   =>   12*24*7 = 2016 (one report/week)
#define BATTERY_ZERO 1.8
#define BATTERY_FULL 3.0
#define HUMI_TRANSMIT_THRESHOLD 3.0  // THRESHOLD tells how much the value should have changed since last time it was transmitted.
#define TEMP_TRANSMIT_THRESHOLD 0.5
#define AVERAGES 2

int batteryReportCounter = BATTERY_REPORT_CYCLE - 1;  // to make it report the first time.
int measureCount = 0;
float lastTemperature = -100;
int lastHumidity = -100;

RunningAverage raHum(AVERAGES);
HTU21D humiditySensor;
Vcc vcc;

MyMessage msgTemp(CHILD_ID_TEMP,V_TEMP); // Initialize temperature message
MyMessage msgHum(CHILD_ID_HUM,V_HUM);
MyMessage msgVolt(CHILD_ID_BATTERY, V_VOLTAGE);

void before() {
  raHum.clear();
  humiditySensor.begin();  
}

void presentation() {  
  sendSketchInfo("TempHum", "2.0 04022017"); 
  present(CHILD_ID_TEMP, S_TEMP);   // Present sensor to controller
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_BATTERY, S_CUSTOM);
}

void loop() { 

  // update temperature and humidity
  measureCount ++;
  batteryReportCounter ++;
  bool forceTransmit = false;
  
  if (measureCount > FORCE_TRANSMIT_CYCLE) {
    forceTransmit = true; 
  }
  sendTempHumidityMeasurements(forceTransmit);

  // Check battery
  if (batteryReportCounter >= BATTERY_REPORT_CYCLE) {

    // get voltage
    float voltage = vcc.Read_Volts();
    float percentage = vcc.Read_Perc(BATTERY_ZERO, BATTERY_FULL, true);
    int batteryPcnt = static_cast<int>(percentage);

    // send voltage
    send(msgVolt.set(voltage, 3)); // Set how many decimals are used (3 in our case)
    sendBatteryLevel(batteryPcnt);

    // reset counter
    batteryReportCounter = 0;
  }
  
  sleep(SLEEP_TIME);
}

/*********************************************
 * * Sends temperature and humidity from Si7021 sensor
 * Parameters
 * - force : Forces transmission of a value (even if it's the same as previous measurement)
 *********************************************/
void sendTempHumidityMeasurements(bool force) {
  bool tx = force;
  
  float temperature = humiditySensor.readTemperature();
  Serial.print("T: ");Serial.println(temperature);

  float diffTemp = abs(lastTemperature - temperature);
  Serial.print(F("TempDiff :"));Serial.println(diffTemp);

  if (diffTemp > TEMP_TRANSMIT_THRESHOLD || tx) {
    send(msgTemp.set(temperature,1));
    lastTemperature = temperature;
    measureCount = 0;
    Serial.println("T sent!");
  }
  
  int humidity = humiditySensor.readHumidity();
  Serial.print("H: ");Serial.println(humidity);

  raHum.addValue(humidity);
  humidity = raHum.getAverage();  // MA sample imply reasonable fast sample frequency
  float diffHum = abs(lastHumidity - humidity);
  Serial.print(F("HumDiff  :"));Serial.println(diffHum); 

  if (diffHum > HUMI_TRANSMIT_THRESHOLD || tx) {
    send(msgHum.set(humidity));
    lastHumidity = humidity;
    measureCount = 0;
    Serial.println("H sent!");
  }

}
