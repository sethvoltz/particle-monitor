/*
* ==============================================================================
* The Monitor Monitor - Show which screen is currently active while fullscreen
*
* Author: Seth Voltz
* Created: 14-July-2016
* License: MIT
* ==============================================================================
*/

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "neopixel/neopixel.h"
#include "application.h"


// =--------------------------------------------------------------= Defines =--=
#define PIXEL_COUNT 10
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B
// pixel type [ WS2812, WS2812B, WS2812B2, WS2811, TM1803, TM1829, SK6812RGBW ]
// note: If not specified, WS2812B is selected for you.
// note: RGB order is automatically applied to WS2811,
//       WS2812/WS2812B/WS2812B2/TM1803 is GRB order.

#define SCREEN_COUNT 20          // Number of screens that can be stored
#define COMMAND_BUFFER_SIZE 128  // How long can an incoming command string be
#define INDICATOR_COLOR 55       // Color as angle [0 <= n < 360]
#define INDICATOR_BRIGHTNESS 128 // Global indicator brightness [0 <= n < 256]

// LED Fading
#define FADE_DURATION_MSEC 250
#define FADE_UPDATE_INTERVAL_MSEC 33 // ~30fps


// =--------------------------------------------------------------= Globals =--=
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
float indicatorBrightness[PIXEL_COUNT];
int currentIndicator;
int serialCounter = 0;
std::stringstream serialBuffer;
std::map<std::string, int> screenMap;


// =-------------------------------------------------= EEPROM Configuration =--=
struct screenConfig {
  uint32_t id;
  unsigned short int indicator;
};

struct screenEEPROM {
  size_t count;
  screenConfig screens[SCREEN_COUNT];
  byte brightness;
};

union {
    screenEEPROM eevar;
    char eeArray[sizeof(screenEEPROM)];
} EEPROMData;


// =--------------------------------------------------= Function Prototypes =--=
uint32_t Wheel(byte WheelPos, float brightness);
byte scale(byte value, float percent);
void setIndicatorByName(std::string name);
void setIndicator(int indicator);
void parseCommand(std::string command);
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
std::string uintToString(uint32_t number);
uint32_t stringToUint(std::string value);
void loadScreens();
void updateScreens();
void readEEPROM(void);
void writeEEPROM(void);
int call_addScreen(String input);
int call_removeScreen(String input);
bool listScreens();
bool addScreen(std::vector<std::string> parsed);
bool removeScreen(std::vector<std::string> parsed);
void updateLEDs(unsigned long time_diff);


// =-------------------------------------------------------= Core Functions =--=
void setup() {
  // Start Serial
  Serial.begin(9600);

  // Setup Particle cloud functions
  Particle.function("addScreen", call_addScreen);
  Particle.function("removeScreen", call_removeScreen);

  // Start NeoPixel Set
  strip.begin();
  setIndicator(-1); // Initialize all pixels to 'off'

  // Load screens
  loadScreens();
}

void loop() {
  static unsigned long fadeUpdateTimer = millis();

  unsigned long fadeUpdateTimeDiff = millis() - fadeUpdateTimer;
  if (fadeUpdateTimeDiff > FADE_UPDATE_INTERVAL_MSEC) {
    updateLEDs(fadeUpdateTimeDiff);
    fadeUpdateTimer = millis();
  }
}

void updateLEDs(unsigned long time_diff) {
  // current indicator fades to full, all others fade out
  float percent = (float)time_diff / (float)FADE_DURATION_MSEC;
  bool needToWrite = false;

  for (int i = 0; i < PIXEL_COUNT; ++i) {
    if (i == currentIndicator && indicatorBrightness[i] < 1) {
      // fade up
      indicatorBrightness[i] += percent;
      if (indicatorBrightness[i] > 1) indicatorBrightness[i] = 1;
      needToWrite = true;
    } else if (i != currentIndicator && indicatorBrightness[i] > 0) {
      // fade down
      indicatorBrightness[i] -= percent;
      if (indicatorBrightness[i] < 0) indicatorBrightness[i] = 0;
      needToWrite = true;
    }
    strip.setPixelColor(i, Wheel(INDICATOR_COLOR, indicatorBrightness[i]));
  }

  if (needToWrite) {
    strip.setBrightness(INDICATOR_BRIGHTNESS);
    strip.show();
  }
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


// =---------------------------------------------------= Command Processing =--=
void parseCommand(std::string input) {
  std::vector<std::string> parsed = split(input, ' ');
  // Uncomment to print parsed command
  // for (unsigned i = 0; i < parsed.size(); i++) {
  //   Serial.println(parsed.at(i).c_str());
  // }

  std::string command = parsed.at(0);
  parsed.erase(parsed.begin());

  if (command.compare("set") == 0) {
    setIndicatorByName(parsed.at(0));
  } else if (command.compare("list") == 0) {
    listScreens();
  } else if (command.compare("add") == 0) {
    addScreen(parsed);
  } else if (command.compare("remove") == 0) {
    removeScreen(parsed);
  } else {
    Serial.println("ERROR: Unknown command");
  }
}

bool listScreens() {
  Serial.println("OK");

  for (
    auto screen = screenMap.begin();
    screen != screenMap.end();
    screen++
  ) {
    Serial.printlnf("SCREEN: %s (%i)", screen->first.c_str(), screen->second);
  }

  return true;
}

bool addScreen(std::vector<std::string> parsed) {
  if (parsed.size() < 2) {
    Serial.println("ERROR: Insufficient parameters");
    return false;
  }

  screenMap[parsed.at(0)] = (int)stringToUint(parsed.at(1)); // update map
  updateScreens();

  Serial.println("OK");
  return true;
}

bool removeScreen(std::vector<std::string> parsed) {
  if (parsed.size() < 1) {
    Serial.println("ERROR: Insufficient parameters");
    return false;
  }

  screenMap.erase(parsed.at(0)); // update map
  updateScreens();

  Serial.println("OK");
  return true;
}

void setIndicatorByName(std::string name) {
  auto search = screenMap.find(name);

  if (search == screenMap.end()) {
    setIndicator(-1);
    Serial.println("ERROR: Unknown screen");
  } else {
    setIndicator(search->second);
    Serial.println("OK");
  }
}

void setIndicator(int indicator) {
  currentIndicator = indicator;
}


// =-----------------------------------------------------= Helper Functions =--=
std::string uintToString(uint32_t number) {
  std::stringstream ss;
  ss << number;
  std::string str;
  ss >> str;
  return str;
}

uint32_t stringToUint(std::string value) {
  std::stringstream buffer(value);
  long var;
  buffer >> var;
  return (uint32_t)var;
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
// Brightness percent between 0 and 1
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float brightness) {
  if (brightness == 0) {
    return strip.Color(0, 0, 0);
  }

  if (WheelPos < 85) {
    return strip.Color(
      scale(WheelPos * 3, brightness),
      scale(255 - WheelPos * 3, brightness),
      scale(0, brightness)
    );
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(
      scale(255 - WheelPos * 3, brightness),
      scale(0, brightness),
      scale(WheelPos * 3, brightness)
    );
  } else {
    WheelPos -= 170;
    return strip.Color(
      scale(0, brightness),
      scale(WheelPos * 3, brightness),
      scale(255 - WheelPos * 3, brightness)
    );
  }
}

byte scale(byte value, float percent) {
  return map(value, 0, 255, 0, (int)(percent * 255));
}

// =--------------------------------------------= Config / EEPROM Functions =--=
void loadScreens() {
  readEEPROM();

  if (EEPROMData.eevar.count > SCREEN_COUNT)
    EEPROMData.eevar.count = SCREEN_COUNT;

  for (unsigned int i = 0; i < EEPROMData.eevar.count; i++) {
    screenConfig screen = EEPROMData.eevar.screens[i];

    // Only load valid data
    if (screen.id > 0 && screen.indicator < PIXEL_COUNT) {
      Serial.printlnf("screen: %i (%i)", screen.id, screen.indicator);
      screenMap.insert(std::make_pair(uintToString(screen.id), screen.indicator));
    }
  }

  if (screenMap.size() != EEPROMData.eevar.count) {
    Serial.println("ERROR: screen count and data do not match, rewriting");
    updateScreens();
  }
}

void updateScreens() {
  // Iterate screenMap into eeprom struct
  int index = 0;
  for (
    auto screen = screenMap.begin();
    screen != screenMap.end();
    screen++, index++
  ) {
    EEPROMData.eevar.screens[index].id = stringToUint(screen->first);
    EEPROMData.eevar.screens[index].indicator = screen->second;
  }
  EEPROMData.eevar.count = screenMap.size() > SCREEN_COUNT ? SCREEN_COUNT : screenMap.size();

  writeEEPROM();
}

void readEEPROM(void) {
  for (int i = 0; i < sizeof(screenEEPROM); i++) {
    EEPROMData.eeArray[i] = EEPROM.read(i);
  }
}

void writeEEPROM(void) {
  for (int i = 0; i < sizeof(screenEEPROM); i++) {
    EEPROM.write(i, EEPROMData.eeArray[i]);
  }
}


// =---------------------------------------------= Particle Cloud Functions =--=
int call_addScreen(String input) {
  // input => screenId, indicator
  std::string command = input.c_str();
  std::vector<std::string> parsed = split(command, ' ');
  return addScreen(parsed) ? 0 : -1;
}

int call_removeScreen(String input) {
  // input => screenId
  std::string command = input.c_str();
  std::vector<std::string> parsed = split(command, ' ');
  return removeScreen(parsed) ? 0 : -1;
}
