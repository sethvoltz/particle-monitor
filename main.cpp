/*
* ==============================================================================
* The Monitor Monitor - Show which monitor is currently active while fullscreen
*
* Author: Seth Voltz
* Created: 14-July-2016
* License: MIT
* ==============================================================================
*/

#include <string>
#include <sstream>
#include <vector>

#include "neopixel/neopixel.h"
#include "application.h"

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_COUNT 2
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

#define COMMAND_BUFFER_SIZE 128
#define INDICATOR_COLOR 55
#define INDICATOR_BRIGHTNESS 64

// pixel type [ WS2812, WS2812B, WS2812B2, WS2811, TM1803, TM1829, SK6812RGBW ]
// note: If not specified, WS2812B is selected for you.
// note: RGB order is automatically applied to WS2811,
//       WS2812/WS2812B/WS2812B2/TM1803 is GRB order.

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
int serialCounter = 0;
std::stringstream serialBuffer;

// Prototypes
uint32_t Wheel(byte WheelPos);
void setIndicatorByName(std::string name);
void setIndicator(int indicator);
void parseCommand(std::string command);
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

void setup() {
  // Start Serial
  Serial.begin(9600);

  // Start NeoPixel Set
  strip.begin();
  // strip.show(); // Initialize all pixels to 'off'
  setIndicator(-1); // Initialize all pixels to 'off'
}

void loop() {
  // setIndicator(1);
}

void serialEvent() {
  char c = Serial.read();
  // serialBuffer[serialCounter++] = c;
  if (c != '\n') serialBuffer << c;

  if (c == '\n' || serialCounter + 1 == COMMAND_BUFFER_SIZE) {
    // new line or full buffer, accept command
    // serialBuffer[serialCounter] = 0; // truncate the string
    parseCommand(serialBuffer.str());

    // reset buffer and counter
    serialCounter = 0;
    serialBuffer.str("");
    serialBuffer.clear(); // Clear state flags.
  }
}

void parseCommand(std::string command) {
  std::vector<std::string> parsed = split(command, ' ');
  // for (unsigned i = 0; i < parsed.size(); i++) {
  //   Serial.println(parsed.at(i).c_str());
  // }
  if (parsed.at(0).compare("set") == 0) {
    setIndicatorByName(parsed.at(1));
  }
}

void setIndicatorByName(std::string name) {
  bool found = true;
  if (name.compare("69732482") == 0) {
    setIndicator(0);
  } else if (name.compare("478210625") == 0) {
    setIndicator(1);
  } else {
    setIndicator(-1);
    found = false;
  }
  Serial.println(found ? "OK" : "NOT FOUND");
}

void setIndicator(int indicator) {
  for (int i = 0; i < PIXEL_COUNT; ++i) {
    if (i == indicator) {
      strip.setPixelColor(i, Wheel(INDICATOR_COLOR));
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }

  strip.setBrightness(INDICATOR_BRIGHTNESS);
  strip.show();
}

// Kudos: http://stackoverflow.com/a/236803/772207
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
