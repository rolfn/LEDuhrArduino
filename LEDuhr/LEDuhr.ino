//
//  www.blinkenlight.net
//
//  Copyright 2015 Udo Klein
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see http://www.gnu.org/licenses/

#include <dcf77.h>
#include "RN-utils.h"
#include "button.h"
#include <Adafruit_i2c_7seg_LED.h>

const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;       // A5 == d19
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 0;
const uint8_t dcf77_pull_up = 0;

//const uint8_t dcf77_monitor_led = 18;  // A4 == d18
const uint8_t dcf77_monitor_led = 13;

Adafruit_LED DISP1;
Adafruit_LED DISP2;

// Setup a new OneButton on pin A1. (aktive low)
OneButton button(A1, true);

uint8_t sample_input_pin() {
  /*
  const uint8_t sampled_data =
  dcf77_inverted_samples ^ (dcf77_analog_samples ?
    (analogRead(dcf77_analog_sample_pin) > 200) :
  digitalRead(dcf77_sample_pin)); */
  button.tick();
  const uint8_t sampled_data = dcf77_inverted_samples ^
    digitalRead(dcf77_sample_pin);
  digitalWrite(dcf77_monitor_led, sampled_data);
  return sampled_data;
}

void setup() {
    using namespace Clock;

    Serial.begin(9600);
    while (!Serial); // Leonardo: wait for serial monitor

    pinMode(dcf77_monitor_led, OUTPUT);

    pinMode(dcf77_sample_pin, INPUT);
    digitalWrite(dcf77_sample_pin, dcf77_pull_up);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);

    restart_timer_0();

    button.attachClick(click);
    button.attachDoubleClick(doubleclick);
    //button.attachDuringLongPress(longPress);
    button.attachLongPressStart(longPressStart);
    button.attachLongPressStop(longPressStop);
    button.setPressTicks(1000);

    DISP1.begin(0x00);
    DISP1.sleep();
    DISP2.begin(0x01);
    DISP2.sleep();

    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = Clock::useless;
        state == Clock::useless || state == Clock::dirty;
        state = DCF77_Clock::get_clock_state()) {

        // button.tick(); // ?

        // wait for next sec
        Clock::time_t now;
        DCF77_Clock::get_current_time(now);

        // render one dot per second while initializing
        static uint8_t count = 0;
        Serial.print('*');
        ++count;
        if (count == 60) {
          count = 0;
          Serial.println();
        }
    }
    DISP1.normal();
    DISP1.setPoint(COLON);
    DISP1.setPoint(POINT_UPPER_LEFT);
    DISP2.normal();
}

void paddedPrint(BCD::bcd_t n) {
    Serial.print(n.digit.hi);
    Serial.print(n.digit.lo);
}

void loop() {
    Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {
        switch (DCF77_Clock::get_clock_state()) {
            case Clock::useless:  Serial.print(F("useless"));  break;
            case Clock::dirty:    Serial.print(F("dirty"));    break;
            case Clock::free:     Serial.print(F("free"));     break;
            case Clock::unlocked: Serial.print(F("unlocked")); break;
            case Clock::locked:   Serial.print(F("locked"));   break;
            case Clock::synced:   Serial.print(F("synced"));   break;
            default:              Serial.print(F("undefined"));
        }
        Serial.print(' ');

        Serial.print(F("20"));
        paddedPrint(now.year);
        Serial.print('-');
        paddedPrint(now.month);
        Serial.print('-');
        paddedPrint(now.day);
        Serial.print(' ');

        paddedPrint(now.hour);
        Serial.print(':');
        paddedPrint(now.minute);
        Serial.print(':');
        paddedPrint(now.second);

        Serial.print("+0");
        Serial.print(now.uses_summertime? '2': '1');
        Serial.print(F("  "));
        Serial.print(millis());
        Serial.print(F("  "));
        Serial.print(DCF77_Clock::get_overall_quality_factor());
        Serial.print(F("  "));
        Serial.print(DCF77_Clock::get_prediction_match());// ^-- Evtl. Doppelklick
        Serial.println();

        DISP1.setDigit(DIGIT_1, now.hour.digit.hi);
        DISP1.setDigit(DIGIT_2, now.hour.digit.lo);
        DISP1.setDigit(DIGIT_3, now.minute.digit.hi);
        DISP1.setDigit(DIGIT_4, now.minute.digit.lo);
        if (alarmActive) {
          DISP1.setPoint(POINT_UPPER_LEFT);
        } else {
          DISP1.clearPoint(POINT_UPPER_LEFT);
        }
        switch (viewMode) {
          case SHOW_DATE:
            DISP2.setDigit(DIGIT_1, now.day.digit.hi ? now.day.digit.hi : BLANK);
            DISP2.setDigit(DIGIT_2, now.day.digit.lo, true);
            DISP2.setDigit(DIGIT_3, now.month.digit.hi ? now.month.digit.hi : BLANK);
            DISP2.setDigit(DIGIT_4, now.month.digit.lo);
            break;
          case SHOW_SEC:
            DISP2.setDigit(DIGIT_1, BLANK);
            DISP2.setDigit(DIGIT_2, BLANK);
            DISP2.setDigit(DIGIT_3, now.second.digit.hi);
            DISP2.setDigit(DIGIT_4, now.second.digit.lo);
            break;
          case SHOW_QTY:
            DISP2.setDigit(DIGIT_1, BLANK);
            DISP2.setDigit(DIGIT_2, BLANK);
            DISP2.setDigit(DIGIT_3, DCF77_Clock::get_prediction_match() >> 4);
            DISP2.setDigit(DIGIT_4, DCF77_Clock::get_prediction_match() & 0x0F);
        }

    }
}

/*

https://github.com/t3db0t/Button
https://github.com/mathertel/OneButton !!!

https://github.com/adafruit/Adafruit-LED-Backpack-Library
https://learn.sparkfun.com/tutorials/using-the-serial-7-segment-display/example-3-i2c
https://github.com/lpaseen/ht16k33/blob/master/ht16k33.cpp

*/

