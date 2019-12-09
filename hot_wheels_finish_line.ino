#include <Adafruit_NeoPixel.h>

// GPIO pin that the neopixel strip is connected to.
#define NEOPIXEL_PIN 10

// Number of racing lanes.
#define NUM_LANES 6

// How long to wait before resetting after no new IR signals.
#define WATCHDOG_TIMEOUT_MS 12000

// Record order of finishers.
// position[0] is the lane number of the winner.
// position[1] is the lane number of second place.
// etc...
int positions[NUM_LANES];
int next_position_to_record;

// Map from lane number to physical GPIO pin.
int lane_pins[NUM_LANES];

// When was the last time the watchdog was petted?
unsigned long last_watchdog_reset_time_ms;

// Neopixels used to indicate finish order.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LANES, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);

void setup() {
  // Arduino pins used: 1, 2, 3, 5, 7, 9
  // Figured out corresponding lane by trial and error.
  lane_pins[0] = 5;
  lane_pins[1] = 3;
  lane_pins[2] = 7;
  lane_pins[3] = 2;
  lane_pins[4] = 1;
  lane_pins[5] = 9;

  // All lane pins are inputs (to read the IR signal),
  // and write HIGH to them to turn on the pullup resistor.
  for (int lane = 0; lane < NUM_LANES; lane++) {
    pinMode(lane_pins[lane], INPUT);
    digitalWrite(lane_pins[lane], HIGH);
  }

  Serial.begin(9600);
  strip.begin();

  reset_pixels();
  reset_lane_state();
  reset_watchdog();
}

void loop(){
  check_for_cars();
  display_positions();
  watchdog();
}

void reset_lane_state() {
  // First place will have position "1".
  next_position_to_record = 1;
  for (int lane = 0; lane < NUM_LANES; lane++) {
    positions[lane] = 0; // "0" indicates no finisher yet.
  }
}

void reset_pixels() {
  for (int lane = 0; lane < NUM_LANES; lane++) {
    strip.setPixelColor(lane, strip.Color(0,0,0));
  }
   strip.show();
}

void reset_watchdog() {
  last_watchdog_reset_time_ms = millis();
}

void watchdog() {
  unsigned long now_ms = millis();
  unsigned long diff_ms = now_ms - last_watchdog_reset_time_ms;

  if (diff_ms > WATCHDOG_TIMEOUT_MS) {
    reset_lane_state();
    reset_pixels();
    reset_watchdog();
    Serial.println("Watchdog reset!");
  }
}

void display_positions() {
  for (int lane = 0; lane < NUM_LANES; lane++) {
    int pos = positions[lane];
    if (pos != 0) {
      switch (pos) {
        case 1:
          // First place is green.
          strip.setPixelColor(lane, strip.Color(0,32,0));
          break;
        case 2:
          // Second place is yellow.
          strip.setPixelColor(lane, strip.Color(32,16,0));
          break;
        default:
          // All other places are red.
          strip.setPixelColor(lane, strip.Color(32,0,0));
      }
    }
  }
  strip.show();
}

// Critical function that checks each lane's IR sensor
// and figures out if a car has broken the beam. If so,
// records finish position for that lane.
void check_for_cars() {
  for (int lane = 0; lane < NUM_LANES; lane++) {
    int value = digitalRead(lane_pins[lane]);
    bool not_yet_recorded = (positions[lane] == 0);
    bool car_detected = (value == LOW);
    if (not_yet_recorded && car_detected) {
      Serial.print("Lane ");
      Serial.print(lane);
      Serial.print(" in position ");
      Serial.print(next_position_to_record);
      Serial.println();
      positions[lane] = next_position_to_record++;
      reset_watchdog();
    }
  }
}
