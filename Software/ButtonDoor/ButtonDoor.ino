/*
This sketch can be used with buttons or door/windows contact sensors (or anything binary)

By Oliver Hilsk
Documentation: http://forum.mysensors.org...

27.10.2016 V1.0 Base sketch
*/

#include <SPI.h>
#include <MySensor.h>

#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define CHILD_ID_BUTTON     0
#define CHILD_ID_BATTERY    1
#define BUTTON_PIN          2         // Digital interrupt pin 0
#define BATTERY_FULL        3000      // 2xAA usually give 3.143V when full
#define BATTERY_ZERO        1800      // 2.34V limit for 328p at 8MHz. 1.9V, limit for nrf24l01 without step-up. 2.8V limit for Atmega328 with default BOD settings.
#define ONE_DAY_SLEEP_TIME  86400000  // report battery status each day at least once

MySensor gw;
MyMessage msg(CHILD_ID_BUTTON, V_TRIPPED);
MyMessage voltage_msg(CHILD_ID_BATTERY, V_VOLTAGE);
long oldvoltage = 0;

void setup()
{
  gw.begin();

  gw.sendSketchInfo("Door sensor w bat", "1.0 27102016");

  gw.present(CHILD_ID_BUTTON, S_DOOR);
  delay(250);
  gw.present(CHILD_ID_BATTERY, S_CUSTOM);

  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
}

void loop()
{
  // check if the door/button is open or closed
  if (digitalRead(BUTTON_PIN) == HIGH) {
    gw.send(msg.set("1")); // door open / button pressed
  } else {
    gw.send(msg.set("0")); // door closed / button not pressed
  }

  // get voltage
  long voltage = readVcc();

  if (oldvoltage != voltage) { // Only send battery information if voltage has changed, to conserve battery.
    gw.send(voltage_msg.set(voltage / 1000.0, 3)); // redVcc returns millivolts. Set wants volts and how many decimals (3 in our case)
    gw.sendBatteryLevel(round((voltage - BATTERY_ZERO) * 100.0 / (BATTERY_FULL - BATTERY_ZERO)));
    oldvoltage = voltage;
  }

  // sleep to conserve energy
  gw.sleep(BUTTON_PIN-2, CHANGE, ONE_DAY_SLEEP_TIME);
}

long readVcc() {
  // From http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
