#define SSID_ME "fnordeingang"
#define PW_ME "R112Zr11ch3353burger"
#define HOST_ME "fnordlicht"

#define CONST_NUM_TRAINS 10

// Leds Pins D3 und D4 auf NodeMCU
#define DATA_PIN_STRIP_ONE    1
#define DATA_PIN_STRIP_TWO    2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define MAX_LEDS    600
int NUM_LEDS=    600;
#define BRIGHTNESS          80   // initial brightness
#define FRAMES_PER_SECOND   60

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

bool gReverseDirection = false;

#include <ESP8266WiFi.h>          // Board Manager:http://arduino.esp8266.com/stable/package_esp8266com_index.json
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal. Kommt mit ESP8266Wifi Board
#include <WiFiManager.h>          // Im Librarymanager installierbar https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPUpdateServer.h> // Kommt mit ESP8266Wifi Board
//#include <ESP8266mDNS.h>
#define FASTLED_INTERNAL
#include <FastLED.h>              // Im Librarymanager installierbar
#include <Arduino.h>
#include <Hash.h>
#include "trains.h"
#include "track.h"

FASTLED_USING_NAMESPACE

//Globals
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = HOST_ME;
const char* ssid     = SSID_ME;
const char* password = PW_ME;
unsigned long lastTimeHost = 0;
unsigned long lastTimeRefresh = 0;


// LED Array
CRGBArray<MAX_LEDS> leds;

int gLedCounter = 0; // global Led Postition Counter for larger iterative Patterns

uint8_t gRed = 254;
uint8_t gGreen = 254;
uint8_t gBlue = 100;

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gSat = 0;
uint8_t gVal = 0;

uint8_t gBright = BRIGHTNESS;
int gMybright;
int MyDelay = 1;

// LED Musterfunktionen

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120


void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[MAX_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}


// In SimplePatternList gPatterns eintragen und über die Position als gCurrentPatternNumber ansprechen
void steadyRGB() 
{
   leds(0,NUM_LEDS - 1) = CRGB(gRed,gGreen,gBlue);
}

void steadyHSV()
{
   leds[gLedCounter]= CHSV( gHue, gSat, gVal);
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter)
{
  if( random8() < chanceOfGlitter)
  {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( random8(), 200, 255);
}

void sinelon() 
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
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

void juggle() 
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void fading_colors()
{
  // 
  leds(0, NUM_LEDS - 1) = CHSV(gHue, 200, gBright);
  gHue++;
  gHue %= 255;
}

void matrix()
{
  // different shaded of green are pushed doen the strip
  for(int i = NUM_LEDS-1; i >= 1; i--) 
  { 
    leds[i] = leds[i-1];
  }  
  leds[0] = CHSV(96, random8(100)+155, random8(200)+55);
}

int8_t pulse_dir = 8;
void pulse()
{
  // brightness goes slowly from dark to bright, color changes slightly everytime it's dark
  leds(0, NUM_LEDS - 1) = CHSV(gHue, 200, gBright);
  int b = gBright + pulse_dir;
  if (b >= 254)
  {
    gBright = 254;
    pulse_dir = -8;
  }
  else if (b <= 32) // minimum brightness, to reduce time without any light
  {
    gBright = 32;
    pulse_dir = 8;
    gHue += 16;
    gHue %= 255;
  }
  else
  {
    gBright += pulse_dir;
  }
}

Track *track = new Track(NUM_LEDS);

void trains()
{
  // Trains of random length, color and speed drive around, if trains are in the same position, their rgb values are added
  leds( leds, NUM_LEDS) = CRGB::Black;
  track->step();
  track->draw(leds);
}

void trains2()  // like trains, but the wagons have a tail
{
  fadeToBlackBy( leds, NUM_LEDS, 128);
  track->step();
  track->draw(leds);
}

static uint16_t dist;         // A random number for our noise generator.
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.
 
CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(OceanColors_p);

void fillnoise8() 
{
  for(int i = 0; i < NUM_LEDS; i++) 
  {                                                                        // Just ONE loop to fill up the LED array as all of the pixels change.
    uint8_t index = inoise8(i*scale, dist+i*scale) % 255;                  // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  dist += beatsin8(10,1, 4);                                               // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
                                                                           // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
}

void trippy()
{
  EVERY_N_MILLISECONDS(10) 
  {
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // Blend towards the target palette
    fillnoise8();                                                           // Update the LED array with noise at the new location
  }
  EVERY_N_SECONDS(5) 
  {             // Change the target palette to a random one every 5 seconds.
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 192, random8(128,255)), CHSV(random8(), 255, random8(128,255)));
  }
}


int offset = 0;
void setupSinewave()
{
  offset++;
  offset %= 6;  
  for(int i=0; i<NUM_LEDS; i++)
  {
    leds[i] = CHSV(gHue, 200, sin((i+offset)*45) * 127 + 128);
  }
}

void setupSinewaveColor()
{
  offset++;
  offset %= 6;  
  gHue = 0;
  for(int i=0; i<NUM_LEDS; i++)
  {
    if ((i + offset) % 6 == 0)
    {
      gHue += 16;
      gHue %= 255;
    }
    leds[i] = CHSV(gHue, 200, sin((i+offset)*45) * 127 + 128);
  }
}

void sinewave()
{
  CRGB c = leds[NUM_LEDS-1];
  for(int i = NUM_LEDS-1; i >= 1; i--) 
  { 
    leds[i] = leds[i-1];
  }  
  leds[0] = c;
}

uint8_t sineHue=0;

void sinewave_color()
{
  CRGB c = leds[NUM_LEDS-1];
  for(int i = NUM_LEDS-1; i >= 1; i--) 
  { 
    leds[i] = leds[i-1];
  }  
  leds[0] = c;
}

void measure()
{
  // used to show where which leds are at what position in the room. will be used for CORNERS-effect
  leds(0, NUM_LEDS) = CRGB::Black;
  leds[0] = CRGB::White;
  leds[50] = CRGB::Red;
  leds[100] = CRGB::Green;
  leds[150] = CRGB::Blue;
  leds[200] = CRGB::Cyan;
  leds[250] = CRGB::Yellow;
  leds[299] = CRGB::White;
}

void corners()
{
  // corners will light up and shoot different effects between them
  leds(0, NUM_LEDS) = CRGB::Black;
  leds(0, 20) = CHSV(96, 255, random8(55)+200);
  leds(180, 200) = CHSV(96, 255, random8(55)+200);
  leds(280, NUM_LEDS) = CHSV(96, 255, random8(55)+200);
}

void color_chase(uint32_t color, uint8_t wait)
{
  //clear() turns all LEDs off
  //FastLED.clear();
  //The brightness ranges from 0-255
  //Sets brightness for all LEDS at once
  FastLED.setBrightness(gBright);
  // Move a single led
  for(int led_number = 0; led_number < NUM_LEDS; led_number++)
  {
    // Turn our current led ON, then show the leds
    leds[led_number] = color;

    // Show the leds (only one of which is has a color set, from above
    // Show turns actually turns on the LEDs
    FastLED.show();

    // Wait a little bit
    delay(wait);

    // Turn our current led back to black for the next loop around
    leds[led_number] = CRGB::Black;
  }
  return;
}

void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUM_LEDS; i=i+3) {
        leds[i+q] = CHSV(red, green, blue);    //turn every third pixel on
      }
      FastLED.show();
      delay(SpeedDelay);
      for (int i=0; i < NUM_LEDS; i=i+3) {
        leds[i+q] = CHSV(0,0,0);        //turn every third pixel off
      }
    }
  }
}
void theatre()
{
    theaterChase(0xff,0,0,50);  
}

void cops()
{
  color_chase(CRGB::Blue, 1);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, steadyRGB, theatre, 
                                fading_colors, matrix, pulse, trippy, trains, measure, corners, trains2, sinewave, 
                                sinewave_color, steadyHSV, Fire2012, cops };

// Nicht LED Muster Funktionen
void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

//-----------------------------------------------------------
char htmlpage[2048];
void updatehtml(){
  int temp = snprintf( htmlpage, 2048, "<html>\
    <head>\
      <title>fnordlicht</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      </style>\
    </head>\
    <body>\
      <h1>fnordlicht</h1>\
      <p><a href=\"/\">Refresh</a></p>\
      <p><a href=\"/brightminus\"><b>-</b></a>&nbsp;Brightness:&nbsp; %i &nbsp;<a href=\"/brightplus\"><b>+</b></a></p>\
      <p>MyBright: %i </p>\
      <table border=1>\
        <tr><td><a href=\"/rainbow\">Rainbow</a></td><td><a href=\"/rainbowwithglitter\">Rainbow with Glitter</a></td><td><a href=\"/confetti\">Confetti</a></td></tr>\
        <tr><td><a href=\"/sinelon\">Sinelon</a></td><td><a href=\"/juggle\">Juggle</a></td><td><a href=\"/bpm\">BPM</a></td></tr>\
        <tr><td><a href=\"/steadyrgb\">Steady RGB</a></td><td><a href=\"/theatre\">Theatre</a></td><td><a href=\"/fading_colors\">Fading Colors</a></td></tr>\
        <tr><td><a href=\"/matrix\">Matrix</a></td><td><a href=\"/pulse\">Pulse</a></td><td><a href=\"/trippy\">Trippy</a></td></tr>\
		<tr><td><a href=\"/trains\">Trains</a></td><td><a href=\"/measure\">Measure</a></td><td><a href=\"/corners\">Corners</a></td></tr>\
		<tr><td><a href=\"/trains2\">Trains 2</a></td><td><a href=\"/sinewave\">Sinewave</a></td><td><a href=\"/sinewave_color\">Sinewave Color</a></td></tr>\
		<tr><td><a href=\"/off\">off</a></td><td><a href=\"/fire2012\">Fire 2012</a></td><td><a href=\"/cops\">Cops</a></td></tr>\
      </table>\
    <body>\
  </html>",gBright, gMybright);
}



// Wifi Connection
// Wifi Connection initialisieren
void WifiConnect() {
   WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(800);
  }
}


// HTTP updater initialisieren
void HTTPUpdateConnect() 
{
   httpUpdater.setup(&httpServer, "/update", "root", "fNordAdmin");
   httpServer.begin();
}


// Webseiten initialisieren/routen
void HTTPServerInit() {
  updatehtml();
  httpServer.on("/index.html",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                        gCurrentPatternNumber = 11; }  );
  httpServer.on("/",  []() {httpServer.send ( 200, "text/html", htmlpage ); }  );
  httpServer.onNotFound ( []() {httpServer.send ( 200, "text/html", htmlpage ); } );
  httpServer.on("/rainbow",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 0; updatehtml(); }  );
  httpServer.on("/rainbowwithglitter",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 1; updatehtml(); }  );
  httpServer.on("/confetti",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 2; updatehtml();}  );
  httpServer.on("/sinelon",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 3; updatehtml();}  );
  httpServer.on("/juggle",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0; 
                                                          gCurrentPatternNumber = 4; updatehtml();}  );
  httpServer.on("/bpm",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0; 
                                                          gCurrentPatternNumber = 5; updatehtml();}  );
  httpServer.on("/steadyrgb",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;  
                                                          gCurrentPatternNumber = 6; updatehtml();}  );
  httpServer.on("/theatre",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 7; updatehtml();}  );
  httpServer.on("/fading_colors",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0; 
                                                          gCurrentPatternNumber = 8; updatehtml();}  );
  httpServer.on("/matrix",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 9; updatehtml();}  );
  httpServer.on("/pulse",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 10; updatehtml();}  );
  httpServer.on("/trippy",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 11; updatehtml();}  );
  httpServer.on("/trains",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;  
                                                          gCurrentPatternNumber = 12; updatehtml();}  );
  httpServer.on("/measure",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 13; updatehtml();}  );
  httpServer.on("/corners",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          MyDelay=0; 
                                                          gCurrentPatternNumber = 14; updatehtml();}  );
  httpServer.on("/trains2",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 15; updatehtml();}  );
  httpServer.on("/sinewave",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=2000;
                                                          setupSinewave();
                                                          gCurrentPatternNumber = 16; updatehtml();}  );
  httpServer.on("/sinewave_color",  []() {httpServer.send ( 200, "text/html", htmlpage );
                                                          MyDelay=2000;
                                                          setupSinewaveColor();
                                                          gCurrentPatternNumber = 17; updatehtml();}  );
  httpServer.on("/steadyhsv",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 18; updatehtml();}  );
  httpServer.on("/fire2012",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          gCurrentPatternNumber = 19; updatehtml();}  );
  httpServer.on("/cops",  []() {httpServer.send ( 200, "text/html", htmlpage ); 
                                                          MyDelay=0;
                                                          FastLED.clear();
                                                          gCurrentPatternNumber = 20; updatehtml();}  );
  httpServer.on("/brightminus",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          gBright = gBright - 10; FastLED.setBrightness(gBright); 
                                                          if (gBright > 250) { gBright=250; }
                                                          if (gBright < 0 ) { gBright=0; }
                                                          updatehtml();}  );
  httpServer.on("/brightplus",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          gBright = gBright + 10; FastLED.setBrightness(gBright); updatehtml();}  );
  httpServer.on("/off",  []() {httpServer.send ( 200, "text/html", htmlpage );  
                                                          gBright = 0; FastLED.setBrightness(gBright); updatehtml();}  );
}


