
#include "fnord.h"


void setup() {
   //Serial.begin(9600);
   //Serial.println("fnordlicht"); Serial.println("");
    
   delay(3000); // 3 second delay for recovery (Stand irgendwo, dass das gut ist)
   // Wifimanager iniitaliaisern. Bucht sich in ein bekanntes Netz ein. Wenn er kein bekanntes Netz findet macht er einen Hostspot auf, um den client zu konfigurieren.
   WiFiManager wifiManager;
   WiFi.hostname(host);
   //MDNS.begin(host);
   //MDNS.addService("http", "tcp", 80);
   
   // tell FastLED about the LED strip configuration
   // zwei Strips
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_ONE,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
   FastLED.addLeds<LED_TYPE,DATA_PIN_STRIP_TWO,COLOR_ORDER>(leds, MAX_LEDS).setCorrection(TypicalLEDStrip);
   // set master brightness control
   //Serial.println("Initialized strip");
   FastLED.setBrightness(BRIGHTNESS);
   // Lege ein schickes muster für den Start fest
   gCurrentPatternNumber =11;
   // Hotspot Name um den CLient zu konfigurieren, wenn er kein bekanntes Netz findet
   wifiManager.autoConnect("fNordLicht-config");
   // Initialisiere das Filesstem
   // HTTP Server für Firmware updates http://<ip>/update
   HTTPUpdateConnect();
   //Serial.println("Initialized http /update server");
   // HTTP Server für die Steuerung
   HTTPServerInit();
   //Serial.println("Initialized http server");
   // rote Lampe wenn boot komplett.
   //pinMode(LED_BUILTIN, OUTPUT);
   //Serial.println("Red led finishes init phase");
}

// Mainloop
void loop() {
    //Webserver abarbeiten
    if (millis() - lastTimeHost > 100)
    {
      httpServer.handleClient();
      lastTimeHost = millis();
    }
    //Aktuelles Patternmuster ausführen
    gPatterns[gCurrentPatternNumber](); 
    // send the 'leds' array out to the actual LED strip
    // Use only if no FasLED.Delay is used: FastLED.show();  
    // insert a delay to keep the framerate modest
    //FastLED.delay(MyDelay); 
    FastLED.show();
    // do some periodic updates
    //EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    //EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically	
    //updatehtml();
}


