/*
   Kraken64 QT Py Demo: https://github.com/jasoncoon/kraken64-qt-py
   Copyright (C) 2024 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FastLED.h> // https://github.com/FastLED/FastLED

FASTLED_USING_NAMESPACE

#define DATA_PIN MOSI
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 64

#include "GradientPalettes.h"
#include "Map.h"

#define MILLI_AMPS 1600 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND 240

CRGB leds[NUM_LEDS];

uint8_t brightness = 128;

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette(CRGB::Black);
CRGBPalette16 gTargetPalette(gGradientPalettes[0]);

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
uint8_t secondsPerPalette = 10;

void setup()
{
  Serial.begin(115200);

  // Serial.println("serial delay...");
  // delay(3000);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(true);
  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  FastLED.setBrightness(brightness);
}

void loop()
{
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // change to a new cpt-city gradient palette
  EVERY_N_SECONDS(secondsPerPalette)
  {
    gCurrentPaletteNumber = addmod8(gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[gCurrentPaletteNumber];
    // Serial.println("next palette");
  }

  EVERY_N_MILLISECONDS(40)
  {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette(gCurrentPalette, gTargetPalette, 8);
  }

  colorWaves();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void fillWithColorWaves(CRGB *ledarray, uint16_t numleds, CRGBPalette16 &palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16; // gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < numleds; i++)
  {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if (h16_128 & 0x100)
    {
      hue8 = 255 - (h16_128 >> 1);
    }
    else
    {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    // index = triwave8( index);
    index = scale8(index, 240);

    CRGB newcolor = ColorFromPalette(palette, index, bri8);

    uint16_t pixelnumber = i;

    pixelnumber = (numleds - 1) - pixelnumber;

    nblend(ledarray[pixelnumber], newcolor, 128);
  }
}

void colorWaves()
{
  fillWithColorWaves(leds, NUM_LEDS, gCurrentPalette);
}
