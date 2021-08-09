#include "ArduinoLowPower.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SCD30.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

// SSD1306 header #defines WHITE.
#undef WHITE

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
Adafruit_SCD30 scd30;
Adafruit_SSD1306 display;

// TODO: capacitive thing to set pixel brightness?

const uint32_t WHITE = pixels.Color(255, 255, 255);
const uint32_t RED = pixels.Color(255, 0, 0);
const uint32_t GREEN = pixels.Color(0, 255, 0);
const uint32_t BLUE = pixels.Color(0, 0, 255);
const uint32_t YELLOW = pixels.Color(255, 255, 0);
const uint32_t ORANGE = pixels.Color(255, 128, 0);
const uint32_t MAGENTA = pixels.Color(255, 0, 255);
const uint32_t PURPLE = pixels.Color(128, 0, 128);
const uint32_t HOT_PINK = pixels.Color(255, 0, 128);

const uint8_t DIM = 5;
const uint8_t BRIGHT = 50;

const uint32_t blink_duration = 500; // milliseconds

const uint8_t button_a = A3;
const uint8_t button_b = A2;
const uint8_t button_c = A1;
const uint8_t ready_pin = A0;

// 0 and 1 are cut off on my display.
const uint16_t leftmost_x = 2;

void displayMessage(const char *message);
void blink(uint32_t color);
void update_color_code(float CO2, uint32_t read_count);
void update_display(float CO2, float temperature, float relative_humidity,
                    uint32_t read_count);
uint32_t get_co2_color(float CO2);
const char *get_co2_description(float CO2);
void display_button_interrupt();
void neopixel_button_interrupt();

volatile unsigned long last_display_button_falling;
volatile bool pending_display_toggle = false;
volatile unsigned long last_neopixel_button_falling;
volatile bool pending_neopixel_toggle = false;
const unsigned long debounce_interval = 100;

void setup() {
  pixels.begin();
  pixels.setBrightness(DIM);
  pixels.setPixelColor(0, PURPLE);
  pixels.show();

  pinMode(ready_pin, INPUT);
  pinMode(button_a, INPUT_PULLUP);
  pinMode(button_b, INPUT_PULLUP);
  pinMode(button_c, INPUT_PULLUP);

  // Wait so that if it's just received power, the SCD30 can start
  // immediately instead of having to retry. Are voltages stabilizing?
  delayMicroseconds(10000);

  while (!scd30.begin()) {
    blink(BLUE);
    blink(PURPLE);
  }

  scd30.selfCalibrationEnabled(false);

  // When first powered the SSD1306 is liable to return success but
  // keep the display blank. Wait for... booting or something.
  delayMicroseconds(100000);

  display = Adafruit_SSD1306(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &Wire);
  while (!display.begin()) {
    blink(ORANGE);
    blink(BLUE);
  }

  LowPower.attachInterruptWakeup(digitalPinToInterrupt(ready_pin), NULL,
                                 RISING);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(button_a),
                                 display_button_interrupt, FALLING);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(button_b),
                                 neopixel_button_interrupt, FALLING);

  pixels.clear();
  pixels.show();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  displayMessage("Waiting for sensor");
}

void loop() {
  // Count reads to allow updates to avoid doing needless work if called
  // multiple times for the same measurement.
  static uint32_t read_count = 0;
  if (scd30.dataReady()) {
    if (scd30.read()) {
      read_count++;
    } else {
      displayMessage("Error reading sensor data");
    }
  }

  // Do not update display and color when the sensor has never been read.
  // "nan" is not useful information.
  if (read_count == 0) {
    LowPower.idle();
    return;
  }

  update_display(scd30.CO2, scd30.temperature, scd30.relative_humidity,
                 read_count);
  update_color_code(scd30.CO2, read_count);

  LowPower.idle();
}

void update_color_code(float CO2, uint32_t read_count) {
  static bool neopixel_enabled = true;
  if (pending_neopixel_toggle &&
      (millis() - last_neopixel_button_falling) > debounce_interval) {
    pending_neopixel_toggle = false;

    neopixel_enabled = !neopixel_enabled;
    if (!neopixel_enabled) {
      pixels.setBrightness(0);
      pixels.show();
      return;
    }
  }

  static uint32_t previous_read_count = 0;
  if (previous_read_count == read_count) {
    previous_read_count = read_count;
    return;
  }

  previous_read_count = read_count;

  static uint32_t previous_color = 0;
  uint32_t current_color = get_co2_color(CO2);

  // Brighten on change
  if (previous_color != current_color)
    pixels.setBrightness(BRIGHT);
  else
    pixels.setBrightness(DIM);

  previous_color = current_color;

  pixels.setPixelColor(0, current_color);
  pixels.show();
}

void update_display(float CO2, float temperature, float relative_humidity,
                    uint32_t read_count) {
  static bool display_enabled = true;
  if (pending_display_toggle &&
      (millis() - last_display_button_falling) > debounce_interval) {
    pending_display_toggle = false;

    display_enabled = !display_enabled;
    if (!display_enabled) {
      display.clearDisplay();
      display.display();
      return;
    }
  }

  static uint32_t previous_read_count = 0;
  if (previous_read_count == read_count) {
    previous_read_count = read_count;
    return;
  }

  previous_read_count = read_count;

  display.clearDisplay();

  // SCD30
  // Measurement range: 400 ppm – 10.000 ppm
  // Accuracy: ±(30 ppm + 3%)
  display.setCursor(leftmost_x, 0);
  display.print(CO2, 0);
  display.print(" ppm CO2");

  // SHT31
  // Accuracy 0.3 C
  display.setCursor(leftmost_x, 8);
  display.print(temperature, 1);
  display.print(" C | ");
  display.print(temperature * (9.0 / 5) + 32, 1);
  display.print(" F");

  // SHT31
  // Accuracy ±2% - 4% at 90%+
  display.setCursor(leftmost_x, 16);
  display.print(relative_humidity, 0);
  display.print("% relative humidity");

  display.setCursor(leftmost_x, 24);
  display.print(get_co2_description(CO2));

  display.display();
}

// clang-format off
/*
 * 350-400 ppm     Normal background concentration in outdoor ambient air
 * 870 ppm         American Society of Heating, Refrigeration, and
 *                 Air-conditioning Engineers indoor steady-state
 *                 recommendation.
 * 400-1000 ppm    Well-ventilated occupied indoor space.
 * 900-2,000 ppm   Complaints of drowsiness and poor air.
 * 2,000-5,000 ppm Headaches, sleepiness and stagnant, stale, stuffy air.
 *                 Poor concentration, loss of attention, increased heart
 *                 rate and slight nausea may also be present.
 * 5,000           Workplace exposure limit (as 8-hour TWA) in most
 *                 jurisdictions.
 * >40,000 ppm     Immediately harmful due to oxygen deprivation.
 *
 * SCD30 datasheet says it offers high accuracy "400 ppm – 10’000 ppm."
 *
 * Working from
 * https://www.dhs.wisconsin.gov/chemical/carbondioxide.htm
 * https://www.epa.gov/sites/default/files/2014-08/documents/base_3c2o2.pdf
 */
// clang-format on
uint32_t get_co2_color(float CO2) {
  // clang-format off
  if (CO2 < 350) return  WHITE;
  if (CO2 < 400) return  BLUE;
  if (CO2 < 870) return  GREEN;
  if (CO2 < 1000) return YELLOW;
  if (CO2 < 2000) return ORANGE;
  if (CO2 < 5000) return RED;
  if (CO2 < 9999) return HOT_PINK;
  return                 MAGENTA;
  // clang-format on
}

const char *get_co2_description(float CO2) {
  // clang-format off
  // Maximum line length "123456789ABCDEFGHIJKL";
  if (CO2 < 350) return  "Very good outdoor air";
  if (CO2 < 400) return  "Typical outdoor air";
  if (CO2 < 870) return  "Good indoor air";
  if (CO2 < 1000) return "Borderline indoor air";
  if (CO2 < 2000) return "Poor indoor air";
  if (CO2 < 5000) return "Bad - stale & stuffy";
  if (CO2 < 9999) return "Above exposure limit";
  return                 "Too high for accuracy";
  // clang-format on
}

void blink(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
  delay(blink_duration);
}

void displayMessage(const char *message) {
  display.clearDisplay();
  display.setCursor(leftmost_x, 0);
  display.println(message);
  display.display();
}

void display_button_interrupt() {
  unsigned long now = millis();
  if (now - last_display_button_falling > debounce_interval) {
    last_display_button_falling = now;
    pending_display_toggle = true;
  }
}

void neopixel_button_interrupt() {
  unsigned long now = millis();
  if (now - last_neopixel_button_falling > debounce_interval) {
    last_neopixel_button_falling = now;
    pending_neopixel_toggle = true;
  }
}
