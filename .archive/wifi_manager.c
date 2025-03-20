#include <WiFi.h>

void connectToWiFi(SSID, PWD) {
    WiFi.begin(SSID, PWD);

    while (WiFi.status() !== WL_CONNECTED) {
        // TODO: Abstract this out better.
        Serial.print('.');
        delay(500);
    }

    return True;
}