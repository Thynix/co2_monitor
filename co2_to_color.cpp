#include <Adafruit_NeoPixel.h>
#include <Adafruit_SCD30.h>

// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
Adafruit_SCD30  scd30;

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

void setup() {
  pixels.begin();
  pixels.setBrightness(DIM);

  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);

  while (!scd30.begin()) {
    Serial.println("No SCD30 chip detected.");
    blink(PURPLE);
    pixels.clear();
    pixels.show();
    delay(blink_duration);
  }

  scd30.selfCalibrationEnabled(false);

  pixels.clear();
  pixels.show();

  Serial.print("Measurement Interval: "); 
  Serial.print(scd30.getMeasurementInterval()); 
  Serial.println(" seconds");
}

void loop() {
  if (!scd30.dataReady()){
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    return;
  }

  if (!scd30.read()){
    Serial.println("Error reading sensor data");
    return; 
  }

  digitalWrite(LED_BUILTIN, HIGH);
  
  print_data();
  set_color_from_co2();
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
  Serial.print("Temperature: ");
  Serial.print(scd30.temperature);
  Serial.println(" degrees C");
  
  Serial.print("Relative Humidity: ");
  Serial.print(scd30.relative_humidity);
  Serial.println(" %");
  
  Serial.print("CO2: ");
  Serial.print(scd30.CO2, 3);
  Serial.println(" ppm");
  Serial.println("");
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
