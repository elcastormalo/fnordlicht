# fnordlicht
Let there be lights! (And confusion, lots of confusion.)

Was man braucht:

NodeMCU Board im Boarmanager einpflegen:
http://arduino.esp8266.com/stable/package_esp8266com_index.json
NodeMCU 1.0 als Board auswählen.
Entweder ist ESP8266 dann als Library im Boardmanager eingebunden oder kann dort installiert werden.

WifiManager im Library Verwalter einbinden.
 
FastLED im Library Verwalter einbinden.

Websockets von Markus Sattler im Library Verwalter einbinden.


Sketch/Data Verzeichnis Dateien ins SPIFFS Dateisystem des ESP hochladen:
https://github.com/esp8266/arduino-esp8266fs-plugin/releases runterladen.
Place here: Program files/Arduino/tools/ESP8266FS/tool/esp8266fs.jar
In der Arduino Oberfläche unter Tools/Werkzeuge -> "ESP8266 Sketch Data Upload" werden dann die Dateien aus dem Data Verzeichnis auf den ESP hochgeladen. Irgendwann geht das auch over the air, wenn der FIlemanager vernünftig eingebunden ist.

http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html ist auch interessant

Wenn der ESP beim Start keinen bekannten Access Point findet, macht er selber einen unter fNordlicht-config auf, wo man dann den AP und die Credentials hinterlegen kann.

Wenn der DHCP Server es unterstützt, ist er danach unter http://fnordlicht zu erreichen
http://fnordlicht/update zum updaten des Firmware Images (Firmware Image mittels "Menü -> Sketch -> Kompilierte Binärdatei exportiern" erzeugen)


