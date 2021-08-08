#include <Adafruit_NeoPixel.h>
#include <Adafruit_SCD30.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ArduinoLowPower.h"

// SSD1306 header #defines WHITE.
#undef WHITE

// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
Adafruit_SCD30  scd30;
Adafruit_SSD1306 display;

// TODO: capacitive thing to set pixel brightness?

const uint32_t WHITE = pixels.Color(255, 255, 255);
const uint32_t RED = pixels.Color(255, 0, 0);
const uint32_t GREEN = pixels.Color(0, 255, 0);
const uint32_t BLUE = pixels.Color(0, 0, 255);
const uint32_t ORANGE = pixels.Color(255, 128, 0);
const uint32_t MAGENTA = pixels.Color(255, 0, 255);
const uint32_t PURPLE = pixels.Color(128, 0, 128);

const uint8_t DIM = 5;
const uint8_t BRIGHT = 50;

const uint32_t blink_duration = 222;

const uint8_t button_a = A3;
const uint8_t button_b = A2;
const uint8_t button_c = A1;
const uint8_t ready_pin = A0;

// 0 and 1 are cut off on my display.
const uint16_t leftmost_x = 2;

void displayMessage(const char* message);
void blink(uint32_t color);
void set_color_from_co2(float CO2);
void print_data(float CO2, float temperature, float relative_humidity);
uint32_t get_co2_color(float CO2);
const char* get_co2_description(float CO2);

void setup() {
  pixels.begin();
  pixels.setBrightness(DIM);
  pixels.setPixelColor(0, PURPLE);

  pinMode(ready_pin, INPUT);

  while (!scd30.begin()) {
    blink(PURPLE);
    pixels.clear();
    pixels.show();
    delay(blink_duration);
  }

  scd30.selfCalibrationEnabled(false);

  pixels.clear();
  pixels.show();

  display = Adafruit_SSD1306(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &Wire);
  while (!display.begin()) {
    blink(ORANGE);
    blink(BLUE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();

  LowPower.attachInterruptWakeup(digitalPinToInterrupt(ready_pin), NULL, RISING);
}

void loop() {
  if (!scd30.dataReady()){
    return;
  }

  if (!scd30.read()){
    displayMessage("Error reading sensor data");
    return;
  }

  print_data(scd30.CO2, scd30.temperature, scd30.relative_humidity);
  set_color_from_co2(scd30.CO2);

  LowPower.idle();
}

void set_color_from_co2(float CO2) {
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

void print_data(float CO2, float temperature, float relative_humidity) {
  display.clearDisplay();

  // SCD-30
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
  display.print(temperature * (9.0/5) + 32, 1);
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

/*
 * 350-400ppm      Normal background concentration in outdoor ambient air
 * 400-900ppm      Well-ventilated occupied indoor space
 * 900-2,000ppm    Complaints of drowsiness and poor air.
 * 2,000-5,000 ppm Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of attention, increased heart rate and slight nausea may also be present.
 * 5,000           Workplace exposure limit (as 8-hour TWA) in most jurisdictions.
 * >40,000 ppm     Immediately harmful due to oxygen deprivation.
 * 
 * Working from
 * https://www.dhs.wisconsin.gov/chemical/carbondioxide.htm
 * https://www.epa.gov/sites/default/files/2014-08/documents/base_3c2o2.pdf
 */
uint32_t get_co2_color(float CO2) {
  if (CO2 < 350) return  WHITE;
  if (CO2 < 400) return  BLUE;
  if (CO2 < 900) return  GREEN;
  if (CO2 < 2000) return ORANGE;
  if (CO2 < 5000) return RED;

  return MAGENTA;
}

const char* get_co2_description(float CO2) {
  if (CO2 < 350) return  "Very good outdoor";
  if (CO2 < 400) return  "Typical outdoor";
  if (CO2 < 900) return  "Well-ventilated indoor";
  if (CO2 < 2000) return "Poor air";
  if (CO2 < 5000) return "Stale, stuffy air";

  return                 "> exposure limit";
}

void blink(uint32_t color) {
    pixels.setPixelColor(0, color);
    pixels.show();
    delay(blink_duration);
}

void displayMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(leftmost_x, 0);
  display.println(message);
  display.display();
}
