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
void set_color_from_co2();
void print_data();
uint32_t get_co2_color();

void setup() {
  pixels.begin();
  pixels.setBrightness(DIM);

  while (!scd30.begin()) {
    blink(PURPLE);
    pixels.clear();
    pixels.show();
    delay(blink_duration);
  }

  scd30.selfCalibrationEnabled(false);

  pixels.clear();
  pixels.show();

  pinMode(ready_pin, INPUT);
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(ready_pin), NULL, HIGH);

  display = Adafruit_SSD1306(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &Wire);
  while (!display.begin()) {
    blink(ORANGE);
    blink(BLUE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
}

void loop() {
  if (!scd30.dataReady()){
    return;
  }

  if (!scd30.read()){
    displayMessage("Error reading sensor data");
    return;
  }

  print_data();
  set_color_from_co2();

  LowPower.idle();
}

void set_color_from_co2() {
  static uint32_t previous_color = 0;
  uint32_t current_color = get_co2_color();

  // Brighten on change
  if (previous_color != current_color)
    pixels.setBrightness(BRIGHT);
  else
    pixels.setBrightness(DIM);

  previous_color = current_color;

  pixels.setPixelColor(0, current_color);
  pixels.show();
}

void print_data() {
  display.clearDisplay();

  // SCD-30
  // Measurement range: 400 ppm – 10.000 ppm
  // Accuracy: ±(30 ppm + 3%)
  display.setCursor(leftmost_x, 0);
  display.print(scd30.CO2, 0);
  display.print(" ppm CO2");

  // SHT31
  // Accuracy 0.3 C
  display.setCursor(leftmost_x, 10);
  display.print(scd30.temperature, 1);
  display.print(" C");

  // SHT31
  // Accuracy ±2% - 4% at 90%+
  display.setCursor(leftmost_x, 20);
  display.print(scd30.relative_humidity, 0);
  display.print("% humidity");

  display.display();
}

/*
 * 250-400ppm      Normal background concentration in outdoor ambient air
 * 400-1,000ppm    Concentrations typical of occupied indoor spaces with good air exchange
 * 1,000-2,000ppm  Complaints of drowsiness and poor air.
 * 2,000-5,000 ppm Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of attention, increased heart rate and slight nausea may also be present.
 * 5,000           Workplace exposure limit (as 8-hour TWA) in most jurisdictions.
 * >40,000 ppm     Exposure may lead to serious oxygen deprivation resulting in permanent brain damage, coma, even death.
 */
uint32_t get_co2_color() {
  if (scd30.CO2 < 250) return WHITE;
  if (scd30.CO2 < 400) return BLUE;
  if (scd30.CO2 < 1000) return GREEN;
  if (scd30.CO2 < 2000) return ORANGE;
  if (scd30.CO2 < 5000) return RED;

  return MAGENTA;
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
