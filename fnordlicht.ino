#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )


#include "fnord.h"


void setup() {
   delay(3000); // 3 second delay for recovery (Stand irgendwo, dass das gut ist)
   // Wifimanager iniitaliaisern. Bucht sich in ein bekanntes Netz ein. Wenn er kein bekanntes Netz findet macht er einen Hostspot auf, um den client zu konfigurieren.
   WiFiManager wifiManager;
   // tell FastLED about the LED strip configuration
   // zwei Strips
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_ONE,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_TWO,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
   // set master brightness control
   FastLED.setBrightness(BRIGHTNESS);
   // Serielle Schnittstelle fürs Debuggen
   //Serial.begin(115200);
   // Lege ein schickes muster für den Start fest
   gCurrentPatternNumber =11;
   // Hotspot Name um den CLient zu konfigurieren, wenn er kein bekanntes Netz findet
   wifiManager.autoConnect("fNordlicht-config");
   // Websockets dienen der Kommunikation mit der Webseite, damit wir auch schön den State zurückmelden können
   WebSocketConnect();
   // Initialisiere das Filesstem
   SPIFFS.begin(); 
   {
      Dir dir = SPIFFS.openDir("/");
      while (dir.next()) {    
         String fileName = dir.fileName();
         size_t fileSize = dir.fileSize();
      }
   }
   // HTTP Server für Firmware updates http://<ip>/update
   HTTPUpdateConnect();
   // HTTP Server für die Steuerung
   HTTPServerInit();
   // rote Lampe wenn boot komplett.
   pinMode(LED_BUILTIN, OUTPUT);
}

// Mainloop
void loop() {
   // Websocket Befehle/Messages
   webSocket.loop();
   // Muss der Webserver auf etwas antworten?
   if (millis() - lastTimeHost > 10)
   {
      httpServer.handleClient();
      lastTimeHost = millis();
    }
    // Led counter. Bei anchem Mustern wahrscheinlich nützlich
    gLedCounter = gLedCounter + 1;
    if (gLedCounter > NUM_LEDS)
    {
       gLedCounter = 0;
    }
    //Aktuelles Patternmuster ausführen
    gPatterns[gCurrentPatternNumber](); 
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    // do some periodic updates
    //EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    //EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
}


