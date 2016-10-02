/**
 * EgSlimReed2
 * Sketch for Slim Node and HC-SR505 based motion sensor. 
 * Inspired by:
 * - MySensors motion sensor example: http://www.mysensors.org/build/motion
 * - AWI's CR123A based Slim Node motion sensor: http://forum.mysensors.org/topic/2478/slim-cr123a-2aa-battery-node
 *
 * Created by oliver Hilsky
 * Based on the sketch of m26872 here https://forum.mysensors.org/topic/2715/slim-node-as-a-mini-2aa-battery-pir-motion-sensor
 * Documentation: http://forum.mysensors.org...
 *
 *
 */
 
#include <MySensor.h>
#include <SPI.h>
#include <Vcc.h>

#define DEBUG 0

#define CHILD_ID 1
#define CHILD_ID_BATTERY 2
#define MOTION_INPUT_PIN 3
#define BATTERY_REPORT_DAY 1   // Desired heartbeat(battery report) interval when inactive. 
#define BATTERY_REPORT_BY_IRT_CYCLE 30 // Make a battery report after this many trips. Maximum report interval will also be equal to this number of days.
#define ONE_DAY_SLEEP_TIME 86400000
#define VCC_MIN 1.9
#define VCC_MAX 3.3

#ifdef DEBUG
#define DEBUG_SERIAL(x) Serial.begin(x)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_SERIAL(x)
#define DEBUG_PRINT(x) 
#define DEBUG_PRINTLN(x) 
#endif

int dayCounter = BATTERY_REPORT_DAY;
int irtCounter = 0;


bool interruptReturn = false; // "false" will make the first loop disregard high output from HV-505 (from start-up) and make a battery report instead.
 
Vcc vcc;
MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);
MyMessage voltage_msg(CHILD_ID_BATTERY, V_VOLTAGE);

void setup()  
{  
  DEBUG_SERIAL(9600);
  DEBUG_PRINTLN(("Serial started"));

  delay(100); // to settle power for radio
  gw.begin();

  pinMode(MOTION_INPUT_PIN, INPUT);
  digitalWrite(MOTION_INPUT_PIN, LOW);    // Disable internal pull-ups

  gw.sendSketchInfo("Motion sensor", "06092016 v1.0");
  gw.present(CHILD_ID, S_MOTION);
  gw.present(CHILD_ID_BATTERY, S_MULTIMETER);

  DEBUG_PRINTLN("Warming and blocking PIR trip for 20s.");
  gw.sleep(20000); // Wait until HC-505 warmed-up and output returned low.
}

void loop() 
{
  if (interruptReturn) {    // Woke up by rising pin
    gw.send(msg.set("1"));  // Just send trip (set) commands to controller.
    gw.wait(50);
    gw.send(msg.set("0"));  // Send the off command right after
    irtCounter++;
    if (irtCounter>=BATTERY_REPORT_BY_IRT_CYCLE) {
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
  
  gw.sleep(3000);  // Make sure everything is stable before start to sleep with interrupts. (don't use "gw.wait()" here). Tests shows false trip ~2s after battery report otherwise.

  // Sleep until interrupt comes in on motion sensor or sleep time passed.
  interruptReturn = gw.sleep(MOTION_INPUT_PIN-2, RISING, ONE_DAY_SLEEP_TIME);
  // DEBUG_PRINT("interruptReturn: ");DEBUG_PRINTLN(interruptReturn);

} 

void sendBatteryReport() {
          float p = vcc.Read_Perc(VCC_MIN, VCC_MAX, true);
          float v = vcc.Read_Volts();
          int batteryPcnt = static_cast<int>(p);
          gw.sendBatteryLevel(batteryPcnt);
          gw.send(voltage_msg.set(v,3));
}
