#define ARDUINO_SAMD_ZERO
#include "FastLED.h"
#include <ResponsiveAnalogRead.h>

FASTLED_USING_NAMESPACE

// Based on FastLED "100-lines-of-code" demo reel, by Mark Kriegsman, December 2014
// Modified with additional code for patterns and navigation, by Sander van de Bor, March 2018



//                        4
//                    
//                        3
//
//  20                    2                     8
//       19                                7
//            18          1           6
//                  17          5 
//                        0
//                
//                    13     9
//             
//                14            10
// 
//            15                   11
//                         
//        16                           12  


ResponsiveAnalogRead batteryStatus(A5, true);

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    8
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    31
#define NUM_ARMS    5

CRGB leds[NUM_LEDS];

#define FRAMES_PER_SECOND  120

byte BRIGHTNESS = 96;
int  ARM = 0;
int  autoCycle = 1;

void setup() {
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  SerialUSB.begin(115200);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, spin, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  if (autoCycle == 1) {
     EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
  }

  batteryStatus.update();

  if(digitalRead(A2) == LOW) autoCycle = 1;
  if(digitalRead(2) == LOW) 
  {
    nextPattern();
    autoCycle = 0;
    delay(500);
  }
  if(digitalRead(A0) == LOW) {
    previousPattern();
    autoCycle = 0;
    delay(500);
  }

  if(digitalRead(4) == LOW && digitalRead(6) == LOW)
  {
     battery();
  }
  else if(digitalRead(6) == LOW && BRIGHTNESS > 5) {
    BRIGHTNESS--;
    FastLED.setBrightness(BRIGHTNESS);
  }
  else if(digitalRead(4) == LOW && BRIGHTNESS < 255) {
    BRIGHTNESS++;
    FastLED.setBrightness(BRIGHTNESS);
  }
  else
  {
    gPatterns[gCurrentPatternNumber]();
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
  SerialUSB.println(gCurrentPatternNumber);
}

void previousPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = ((gCurrentPatternNumber - 1) + ARRAY_SIZE( gPatterns)) % ARRAY_SIZE( gPatterns);
  SerialUSB.println(gCurrentPatternNumber);
}

void battery()
{
  // Show battery status, percentage of number of lights equals the battery remaining capacity
  FastLED.clear ();
  fill_solid(leds+0, map(batteryStatus.getValue(), 530, 660, 1, NUM_LEDS), CHSV( 255, 255, 192));
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  int LED_PER_ARM = NUM_LEDS / NUM_ARMS;
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 30, 0, LED_PER_ARM );
  for ( int i = 0; i < NUM_ARMS; i++){
    if (pos == 0) leds[0] += CHSV( gHue, 255, 192);
    else leds[pos+i*LED_PER_ARM] += CHSV( gHue, 255, 192);
  }
}

void spin()
{
  // a colored dot sweeping back and forth, with fading trails
  int LED_PER_ARM = NUM_LEDS / NUM_ARMS;
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 30, 0, NUM_ARMS);
  if (pos == 5) pos = 0;
  leds[0] = CHSV( gHue, 255, 192);
  fill_solid(leds+1+pos*LED_PER_ARM, LED_PER_ARM, CHSV( gHue, 255, 192));
  //CHSV( gHue, 255, 192));
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
