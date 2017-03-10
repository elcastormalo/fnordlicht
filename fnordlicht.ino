#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )

#include "fnord.h"

void setup() {
   delay(3000); // 3 second delay for recovery
   WiFiManager wifiManager;
   // tell FastLED about the LED strip configuration
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_ONE,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_TWO,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  Serial.begin(115200);
  gCurrentPatternNumber = 6;
  DEBUGGING("Current PAttern #7");  
  DEBUGGING("Trying Wifi connection.....");
  wifiManager.autoConnect("fNordlicht-config");
  DEBUGGING("WiFi Manager OK");
  DEBUGGING("Trying Websocket connection.....");
  WebSocketConnect();
  DEBUGGING("Mounting SPIFFS Filesystem");
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DEBUGGING(fileName.c_str());
    }
  }
  MDNSConnect();
  HTTPUpdateConnect();
  HTTPServerInit();
  // rote Lampe wenn boot durch
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
   webSocket.loop();  //Websocket Befehle/Messages
   // Firmware Update Webserver
   if (millis() - lastTimeHost > 10) {
      httpServer.handleClient();
      lastTimeHost = millis();
      // DEBUGGING("webloop");
    }
    gLedCounter = gLedCounter + 1;
    if (gLedCounter > NUM_LEDS) {
       gLedCounter = 0;
    }
    
    //Aktuelles Pattern aufrufen
    gPatterns[gCurrentPatternNumber](); 
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    // do some periodic updates
    //EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    //EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}


