/*
This sketch can be used with buttons or door/windows contact sensors (or anything binary)

By Oliver Hilsky
Documentation: http://forum.mysensors.org...

27.10.2016 1.0 Base sketch
12.11.2016 2.0 Upgrade to mysensors 2.0
10.ß2.2017 3.0 disabled pullup, added external resistor
*/

// library settings
#define MY_RADIO_NRF24
#define MY_DEBUG    // Enables debug messages in the serial log
#define MY_BAUD_RATE  9600 // Sets the serial baud rate for console and serial gateway
//#define MY_SIGNING_SOFT // Enables software signing
//#define MY_SIGNING_REQUEST_SIGNATURES // Always request signing from gateway
//#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7 // floating pin for randomness
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {...}}} // gateway addres if you want to use whitelisting (node only works with messages from this one gateway)
//#define MY_NODE_ID 26

#include <SPI.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <Vcc.h>

// variables and helpers
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define CHILD_ID_BUTTON     0
#define CHILD_ID_BATTERY    1
#define BUTTON_PIN          2         // Digital interrupt pin 0
#define BATTERY_FULL        3.0      // 2xAA usually give 3.143V when full
#define BATTERY_ZERO        1.8      // 2.34V limit for 328p at 8MHz. 1.9V, limit for nrf24l01 without step-up. 2.8V limit for Atmega328 with default BOD settings.
#define ONE_DAY_SLEEP_TIME  86400000  // report battery status each day at least once

MyMessage msg(CHILD_ID_BUTTON, V_TRIPPED);
MyMessage voltage_msg(CHILD_ID_BATTERY, V_VOLTAGE);
Vcc vcc(1.0);
float oldvoltage = 0;

void before()
{
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up or (even better) use external pullup/down resistor
  //digitalWrite(BUTTON_PIN,HIGH);
}

void presentation() {
  sendSketchInfo("Button sensor", "2.0 19122016");

  present(CHILD_ID_BUTTON, S_DOOR);
  delay(250);
  present(CHILD_ID_BATTERY, S_CUSTOM);
}

void loop()
{
  wait(30); // "debouncing"
  
  // check if the door/button is open or closed
  if (digitalRead(BUTTON_PIN) == LOW) {
    send(msg.set("1")); // door open / button pressed
  } else {
    //send(msg.set("0")); // door closed / button not pressed
  }

  // get voltage
  float voltage = vcc.Read_Volts();
  float percentage = vcc.Read_Perc(BATTERY_ZERO, BATTERY_FULL, true);
  int batteryPcnt = static_cast<int>(percentage);

  if (oldvoltage != voltage) { // Only send battery information if voltage has changed, to conserve battery.
    send(voltage_msg.set(voltage, 3)); // Set how many decimals are used (3 in our case)
    sendBatteryLevel(batteryPcnt);
    oldvoltage = voltage;
  }

  // sleep so it doesnt get pressed right away again
  sleep(250);

  // sleep to conserve energy
  sleep(BUTTON_PIN-2, FALLING, ONE_DAY_SLEEP_TIME);
}
