/**
 * Created by oliver Hilsky
 * Based on the sketch of m26872 here https://forum.mysensors.org/topic/2715/slim-node-as-a-mini-2aa-battery-pir-motion-sensor
 * Documentation: http://forum.mysensors.org...
 *
 * Version 1 06.09.2016 basesketch
 * Version 2 12.11.2016 update to mysensors 2
 */
 
// library settings
#define MY_RADIO_NRF24
//#define MY_DEBUG    // Enables debug messages in the serial log
#define MY_BAUD_RATE  9600 // Sets the serial baud rate for console and serial gateway
#define MY_SIGNING_SOFT // Enables software signing
#define MY_SIGNING_REQUEST_SIGNATURES // Always request signing from gateway
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7 // floating pin for randomness

#include <SPI.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <Vcc.h>

// program settings
#define CHILD_ID 1
#define CHILD_ID_BATTERY 2
#define MOTION_INPUT_PIN 3
#define BATTERY_REPORT_DAY 1   // Desired heartbeat(battery report) interval when inactive. 
#define BATTERY_REPORT_BY_IRT_CYCLE 30 // Make a battery report after this many trips. Maximum report interval will also be equal to this number of days.
#define ONE_DAY_SLEEP_TIME 86400000
#define VCC_MIN 1.9
#define VCC_MAX 3.3

int dayCounter = BATTERY_REPORT_DAY;
int irtCounter = 0;

bool interruptReturn = false; // "false" will make the first loop disregard high output from HV-505 (from start-up) and make a battery report instead.
 
Vcc vcc;
MyMessage msg(CHILD_ID, V_TRIPPED);
MyMessage voltage_msg(CHILD_ID_BATTERY, V_VOLTAGE);


void before() {
  pinMode(MOTION_INPUT_PIN, INPUT);
  digitalWrite(MOTION_INPUT_PIN, LOW);    // Disable internal pull-ups
}

void presentation()  
{  
  sendSketchInfo("Motion sensor", "12112016 v2.0");
  present(CHILD_ID, S_MOTION);
  present(CHILD_ID_BATTERY, S_MULTIMETER);

  sleep(20000); // Wait until HC-505 warmed-up and output returned low.
}

void loop() 
{
  if (interruptReturn) {    // Woke up by rising pin
    send(msg.set("1"));  // Just send trip (set) commands to controller.
    wait(50);
    send(msg.set("0"));  // Send the off command right after
    irtCounter++;
    if (irtCounter >= BATTERY_REPORT_BY_IRT_CYCLE) {
        irtCounter=0;
        sendBatteryReport();
    }
  }
  else { // Woke up by timer  (or it's the first run)
    dayCounter++; 
    if (dayCounter >= BATTERY_REPORT_DAY) {
          dayCounter = 0;
          sendBatteryReport();
    }
  }
  
  sleep(3000);  // Make sure everything is stable before start to sleep with interrupts. (don't use "wait()" here). Tests shows false trip ~2s after battery report otherwise.

  // Sleep until interrupt comes in on motion sensor or sleep time passed.
  interruptReturn = sleep(MOTION_INPUT_PIN-2, RISING, ONE_DAY_SLEEP_TIME);
} 

void sendBatteryReport() {
  float p = vcc.Read_Perc(VCC_MIN, VCC_MAX, true);
  float v = vcc.Read_Volts();
  int batteryPcnt = static_cast<int>(p);
  sendBatteryLevel(batteryPcnt);
  send(voltage_msg.set(v,3));
}
