/***********************************************************************|
| tinyAVR 0/1/2-series analog comparator library                        |
|                                                                       |
| Interrupt.ino                                                         |
|                                                                       |
| A library for interfacing with the tinyAVR analog comparator(s).      |
| Developed in 2019 by MCUdude  https://github.com/MCUdude/             |
| Ported to tinyAVR 2021 by Spence Konde for megaTinyCore               |
| https://github.com/SpenceKonde/megaTinyCore                           |
|                                                                       |
| In this example we use an internal reference voltage instead of an    |
| external one on the negative pin. This eliminates the need for an     |
| external voltage divider to generate a reference. Note that the       |
| internal reference requires a stable voltage to function properly.    |
| Instead of using a physical output pin we're instead triggering an    |
| interrupt that will run a user defined function.                      |
|                                                                       |
| This is the formula for the generated voltage:                        |
| Vdacref = (DACREF / 256) * Vref                                       |
|                                                                       |
| On the 0-series there is no DACREF option - but we're using the full  |
| reference voltage here anyway, so we instead specify in_n::vref       |
|                                                                       |
|***********************************************************************/

#include <Comparator.h>
#include "HT16K33.h"

#define LED PIN_PA7
#define ANALOG_IN PIN_PB1
#define PROBE PIN_PA6
#define BIT_A(bits) ((bits&0b00010000) ? 1 : 0)
#define BIT_B(bits) ((bits&0b00001000) ? 1 : 0)

volatile bool rising_edge = false;
HT16K33 seg(0x70);

// represent the state of signal in the first 600ms of each second by a 6-bit field where each bit represents a 100ms period, so
// 0b00111110 - 500ms on = the 'zero' second
// 0b00111000 - 300ms on, meaning bits A = on, B = on
// 0b00101000 - two pulses of 100ms with a gap, meaning A= off, B = on
// 0b00110000 - 200ms on, meaning A = on, B = off
// 0b00100000 - 100ms on, meaning A = off, B = off
uint8_t seconds[60] = {0};


// This function runs when a rising edge exceeds 0.55v causing an interrupt
void riseFunction() {
  // Comparator.stop();
  // Serial.println("Output of analog comparator went high!");
  rising_edge = true;
}

// Invoked at the beginning of a 100ms timeslot, returns the effective status of that time period
// using averaging of samples in the central period of the 100ms. It *must* have duration of 100ms
uint8_t sample100Millis() {
  uint8_t high_count = 0;
  
  // Clocks works on AA batteries, 1.2-1.5V, and it's output signal is the same.
  // We choose 2.5V as the max range of the signal, but treat anything above 0.55V as 'on'.
  analogReference(INTERNAL2V5);

  // wait until the meaningful central section of the 100ms pulse
  delay(40);

  // flash the probe for debugging purposes
  digitalWrite(PROBE, HIGH);

  // sample at 40, 45, 50, 55, 60, 65, 70, 75ms
  for (uint8_t loop=0; loop<8; loop++) {

    // default resolution is 10-bits so max value 1024, 0.55V corresponds to 225

    int voltage = analogRead(ANALOG_IN);

    
    if (voltage > 225) {
      high_count++;
      digitalWrite(LED, HIGH);
    } else {
      digitalWrite(LED, LOW);
    }
    delay(5);
  }
  
  digitalWrite(PROBE, LOW);
  
  // kill time until the 100ms is finished
  delay(20);
   
  digitalWrite(LED, LOW);
  return ((high_count > 4) ? 1 : 0);
  
}

uint8_t sampleSecond() {
  // sample the first six periods of 100ms and return as a bit-field
  uint8_t value = 0;

  for (int loop=0; loop < 6; loop++) {
    value = (value<<1) + sample100Millis();
  }
  return (value);
}

void displayBits(uint8_t bits) {
  uint32_t value;
  
  switch (bits) {
    case 0b00111110:
      value = 0x0000;
      break;

    case 0b00100000:
      value = 0xA0B0;
      break;

    case 0b00101000:
      value = 0xA0B1;
      break;

    case 0b00110000:
      value = 0xA1B0;
      break;

    case 0b00111000:
      value = 0xA1B1;
      break;

    default:
      value = 0xEEEE;
      break;
  }
  
  // seg.displayHex(value);
    
}

void displayTime() {
  uint16_t year;
  uint8_t month, day_om, day_ow, hour, minute;
  uint8_t accum, i;
  uint16_t parity = 0;
  
  year =  2000 + BIT_A(seconds[17]) * 80 +
          BIT_A(seconds[18]) * 40 +
          BIT_A(seconds[19]) * 20 +
          BIT_A(seconds[20]) * 10 +
          BIT_A(seconds[21]) * 8 +
          BIT_A(seconds[22]) * 4 +
          BIT_A(seconds[23]) * 2 +
          BIT_A(seconds[24]) * 1;
  month = BIT_A(seconds[25]) * 10 +
          BIT_A(seconds[26]) * 8 +
          BIT_A(seconds[27]) * 4 +
          BIT_A(seconds[28]) * 2 +
          BIT_A(seconds[29]) * 1;
  day_om =BIT_A(seconds[30]) * 20 +
          BIT_A(seconds[31]) * 10 +
          BIT_A(seconds[32]) * 8 +
          BIT_A(seconds[33]) * 4 +
          BIT_A(seconds[34]) * 2 +
          BIT_A(seconds[35]) * 1;
  day_ow =BIT_A(seconds[36]) * 4 +
          BIT_A(seconds[37]) * 2 +
          BIT_A(seconds[38]) * 1;        
  hour =  BIT_A(seconds[39]) * 20 +
          BIT_A(seconds[40]) * 10 +
          BIT_A(seconds[41]) * 8 +
          BIT_A(seconds[42]) * 4 +
          BIT_A(seconds[43]) * 2 +
          BIT_A(seconds[44]) * 1;  
  minute =BIT_A(seconds[45]) * 40 +
          BIT_A(seconds[46]) * 20 +
          BIT_A(seconds[47]) * 10 +
          BIT_A(seconds[48]) * 8 +
          BIT_A(seconds[49]) * 4 +
          BIT_A(seconds[50]) * 2 +
          BIT_A(seconds[51]) * 1;  

  for (accum = 0, i = 17; i <= 24; i++) {
    accum += BIT_A(seconds[i]);
  }
  if ((accum % 1) == BIT_B(seconds[54]))
    parity |= 0xe000;

  for (accum = 0, i = 25; i <= 35; i++) {
    accum += BIT_A(seconds[i]);
  }
  if ((accum % 1) == BIT_B(seconds[55]))
    parity |= 0x0e00;

  for (accum = 0, i = 36; i <= 38; i++) {
    accum += BIT_A(seconds[i]);
  }
  if ((accum % 1) == BIT_B(seconds[56]))
    parity |= 0x00e0;

  for (accum = 0, i = 39; i <= 51; i++) {
    accum += BIT_A(seconds[i]);
  }
  if ((accum % 1) == BIT_B(seconds[57]))
    parity |= 0x000e;

  seg.displayInt(year);
  delay(1000);
  seg.displayDate(day_om, month);
  delay(1000);
  seg.displayTime(hour, minute);
  delay(1000);
  seg.displayHex(parity);

}

void setup() {
  // Configure serial port
  delay(2000);
  Serial.begin(57600);

  // Configure an putput pin used for debugging
  pinMode(PROBE, OUTPUT);
  digitalWrite(PROBE, LOW);

  // Configure the display
  Wire.swap(1);
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.displayClear();
  

  // Configure relevant comparator parameters
  Comparator.input_p = in_p::in2;       // Use positive input 2 (PB1)
  Comparator.input_n = in_n::vref;    // Connect the negative pin to tinternal voltage reference
  Comparator.reference = ref::vref_0v55; // Set the reference voltage to 0.55V
  Comparator.hysteresis = hyst::large;  // Use a 50mV hysteresis
  Comparator.output = out::disable;     // Use interrupt trigger instead of output pin

  Serial.println("about to enable comparator");

  // Initialize comparator
  Comparator.init();

  // Set interrupt (supports RISING, FALLING and CHANGE)
  Comparator.attachInterrupt(riseFunction, RISING);

  // Start comparator
  Comparator.start();
}

void loop() {

  static uint8_t sec_of_min = 0;
  uint8_t bits = 0;  
  
  if (rising_edge) {

    Serial.println("Loop: rising edge detected.");
    rising_edge = false;
    Comparator.stop();  // don't look for new rising edges while we're in a second
    bits = sampleSecond();
    if (bits == 0b00111110) {
      // reset to the first slot if we pick up a 500ms pulse
      sec_of_min = 0;
    }
    // stash the sample in the next slot of the 'seconds' store
    seconds[sec_of_min++] = bits;
    Serial.print("Second: ");
    Serial.print(sec_of_min, DEC);
    Serial.print(" Bits: ");
    Serial.println(bits, BIN);
    seg.displayInt(sec_of_min);
    
    // have we completed gathering a minute of samples?
    if (sec_of_min == 60) {
      displayTime();
      sec_of_min = 0;
    }

    // re-enable the comparator for the next second, or next broadcast of time signal
    Comparator.start();
  }

  // Could sleep here, pending interrupt from Comparator

}
