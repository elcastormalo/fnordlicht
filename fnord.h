#ifndef DEBUGGING
#define DEBUGGING(...)
#endif
#ifndef DEBUGGING_L
#define DEBUGGING_L(...)
#endif

#define SSID_ME "fnordeingang"
#define PW_ME "R112Zr11ch3353burger"
#define HOST_ME "fnordlicht"

#define DATA_PIN_STRIP_ONE    1
#define DATA_PIN_STRIP_TWO    2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define MAX_LEDS    300
int NUM_LEDS=    300;
#define BRIGHTNESS          165   // initial brightness
#define FRAMES_PER_SECOND  30

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPUpdateServer.h>
#define FASTLED_INTERNAL
#include <FastLED.h>
#include <Arduino.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include "trains.h"
#include "track.h"

//includen wir erst weiter unten, weil es einige Funktionen bracuht. Kack Arduino IDE. 
//// Filesystem
//// Tool Download https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.1.3/ESP8266FS-0.1.3.zip.
//// Docs http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html
//#include "FS.h"//holds the current upload
//#include "filemanager.h"

FASTLED_USING_NAMESPACE

//Globals
WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = HOST_ME;
const char* ssid     = SSID_ME;
const char* password = PW_ME;
unsigned long lastTimeHost = 0;
unsigned long lastTimeRefresh = 0;

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

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


// LED Musterfunktionen
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

void blinktest() {
   int prevled= gLedCounter - 1 ;
   if (prevled < 0 ) {
     prevled = 0;
   }
   // Turn our current led on
   leds[gLedCounter] = CRGB::White;
   leds[prevled] = CRGB::Black;
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



// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, steadyRGB, blinktest, 
                                fading_colors, matrix, pulse, trippy, trains, measure, corners, trains2, sinewave, sinewave_color, steadyHSV };

// Nicht LED Muster Funktionen

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

String myState() {
  String Antwort = "";
  Antwort += gBright;
  Antwort += ";";
  Antwort += gCurrentPatternNumber;
  Antwort += ";";
  Antwort += gRed;
  Antwort += ";";
  Antwort += gGreen;
  Antwort += ";";
  Antwort += gBlue;
  Antwort += ";";
  Antwort += gHue;
  Antwort += ";";
  Antwort += gSat;
  Antwort += ";";
  Antwort += gVal;
  return Antwort;
} 

// WebSocket Events
// "Befehle" der Webseite (websocket) verarbeiten
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String Antwort = "";
  switch (type)
  {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        //webSocket.sendTXT(num, "Connected");
        //webSocket.broadcastTXT("Connected");
        Antwort = myState();
        webSocket.sendTXT(num, Antwort);
      }
      break;
      
    case WStype_TEXT:
      {
        lastTimeRefresh = millis();
        digitalWrite(LED_BUILTIN, LOW);  // built-in LED on um Feedback über empfangene Kommandos zu geben

        String text = String((char *) &payload[0]);
        if (text.startsWith("r")) 
        {
          String rVal = (text.substring(text.indexOf("r") + 1, text.length()));
          gRed = rVal.toInt();
        }
        if (text.startsWith("g")) 
        {
          String gVal = (text.substring(text.indexOf("g") + 1, text.length()));
          gGreen = gVal.toInt();
        }
        if (text.startsWith("b")) 
        {
          String bVal = (text.substring(text.indexOf("b") + 1, text.length()));
          gBlue = bVal.toInt();             
        }
        if (text.startsWith("h")) 
        {
          String hVal = (text.substring(text.indexOf("h") + 1, text.length()));
          gHue = hVal.toInt();
        }
        if (text.startsWith("s")) 
        {
          String sVal = (text.substring(text.indexOf("s") + 1, text.length()));
          gSat = sVal.toInt();
        }
        if (text.startsWith("v")) 
        {
          String vVal = (text.substring(text.indexOf("v") + 1, text.length()));
          gVal = vVal.toInt();             
        }
        if (text.startsWith("m")) 
        {
          String mVal = (text.substring(text.indexOf("m") + 1, text.length()));
          gBright = mVal.toInt();
          FastLED.setBrightness(gBright);
        }        
        if (text == "RESET") {
          leds.fadeToBlackBy(40);
        }
        if (text == "RAINBOW") {
          gCurrentPatternNumber = 0;
        }
        if (text == "RAINBOW_GLITTER") {
          gCurrentPatternNumber = 1;
        }
        if (text == "CONFETTI") {
          gCurrentPatternNumber = 2;
        }
        if (text == "SINELON") {
          gCurrentPatternNumber = 3;
        }
        if (text == "JUGGLE") {
          gCurrentPatternNumber = 4;
        }
        if (text == "BPM") {
          gCurrentPatternNumber = 5;
        }
        if (text == "STEADY") {
          gCurrentPatternNumber = 6;
        }
        if (text == "BLINK") {
          gCurrentPatternNumber = 7;
        }
        if (text == "FADING_RAINBOW") {
          gCurrentPatternNumber = 8;
        }
        if (text == "MATRIX") {
          gCurrentPatternNumber = 9;
        }
        if (text == "PULSE") {
          gCurrentPatternNumber = 10;
        }
        if (text == "TRIPPY") {
          dist = random16(12345);  
          gCurrentPatternNumber = 11;
        }
        if (text == "TRAINS") {
          gCurrentPatternNumber = 12;
        }
        if (text == "MEASURE") {
          gCurrentPatternNumber = 13;
        }
        if (text == "CORNERS") {
          gCurrentPatternNumber = 14;
        }
        if (text == "TRAINS2") {
          gCurrentPatternNumber = 15;
        }
        if (text == "SINEWAVE") {
          setupSinewave();
          gCurrentPatternNumber = 16;
        }
        if (text == "SINEWAVE_COLOR") {
          setupSinewaveColor();
          gCurrentPatternNumber = 17;
        }
        if (text == "STEADY_HSV") {
          gCurrentPatternNumber = 18;
        }
        digitalWrite(LED_BUILTIN, HIGH);   // on-board LED off
        Antwort = myState();
        webSocket.sendTXT(num, Antwort);
      }      
      break;
    
    case WStype_BIN:
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
  }
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

// WebSocket Connection initialisieren
void WebSocketConnect() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// HTTP updater initialisieren
void HTTPUpdateConnect() 
{
   httpUpdater.setup(&httpServer);
   httpServer.begin();
}

// Filesystem
// Tool Download https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.1.3/ESP8266FS-0.1.3.zip.
// Docs http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html
#include "FS.h"//holds the current upload
#include "filemanager.h"

// Webseiten initialisieren/routen
void HTTPServerInit() {
  httpServer.on("/index.html", HTTP_GET, [](){
    if(!handleFileRead("/index.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });
  httpServer.on("/", HTTP_GET, [](){
    if(!handleFileRead("/index.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });  
  httpServer.on("/monitor", HTTP_GET, [](){
    if(!handleFileRead("/monitor.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });
  httpServer.on("/help", HTTP_GET, [](){
    if(!handleFileRead("/help.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });

  httpServer.on("/list", HTTP_GET, handleFileList);
  //load editor
  httpServer.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) httpServer.send(404, "text/plain", "FileNotFound");
  });
  //create file
  httpServer.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  httpServer.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  httpServer.on("/edit", HTTP_POST, [](){ httpServer.send(200, "text/plain", ""); }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  httpServer.onNotFound([](){
    if(!handleFileRead(httpServer.uri()))
      httpServer.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  httpServer.on("/all", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"analog\":"+String(analogRead(A0));
    json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    httpServer.send(200, "text/json", json);
    json = String();
  });
}


