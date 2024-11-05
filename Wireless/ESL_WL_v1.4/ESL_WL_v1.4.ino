#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TM1637Display.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// WiFi and Google Sheets setup
const char* ssid = "USER02";
const char* password = "87654321";
const char* googleSheetsUrl = "https://script.google.com/macros/s/AKfycbycK5WSGJnVsW5JF8KQoqgJuagm4arhQgX5nijMGBIBSHnaRprARSzFfiMPAGvByOVc/exec";

// Display setup on GPIO 16 (CLK) and GPIO 14 (DIO)
TM1637Display display(16, 14);

// NTP time client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // IST offset

// Target wakeup time (10:30 AM IST)
const int targetHour = 10;
const int targetMinute = 30;

void setup() {
    Serial.begin(115200);
    display.setBrightness(0x0f);
    displayDashes();
}

void loop() {
    connectWiFi();              // Connect to Wi-Fi
    fetchDataAndDisplay();      // Fetch data from Google Sheets and display it
    delay(1000);
    connectNTPAndSleep();       // Sync time with NTP, calculate delay, and sleep
}

// Helper function to connect to Wi-Fi
void connectWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        displayDashes();
    }
    Serial.println("\nConnected to WiFi");
    displayCONN();
}

// Function to connect to NTP server, calculate delay, and delay until 10:30 AM next day
void connectNTPAndSleep() {
    timeClient.begin();
    delay(500);
    if (timeClient.update()) {
        int currentHour = timeClient.getHours();
        int currentMinute = timeClient.getMinutes();
        unsigned long currentEpoch = timeClient.getEpochTime();
        unsigned long targetEpoch;

        if (currentHour < targetHour || (currentHour == targetHour && currentMinute < targetMinute)) {
            // Calculate remaining time until today's 10:30 AM
            targetEpoch = currentEpoch + ((targetHour - currentHour) * 3600 + (targetMinute - currentMinute) * 60);
        } else {
            // Calculate time until tomorrow's 10:30 AM
            targetEpoch = currentEpoch + ((24 - currentHour + targetHour) * 3600 + (targetMinute - currentMinute) * 60);
        }

        unsigned long sleepDuration = (targetEpoch - currentEpoch) * 1000UL;  // Convert to milliseconds
        Serial.print("Sleeping for ");
        Serial.print(sleepDuration / 1000);
        Serial.println(" seconds (until 10:30 AM)");

        WiFi.disconnect();  // Disconnect Wi-Fi to save power
        delay(sleepDuration);  // Delay until the next 10:30 AM
    } else {
        Serial.println("Failed to update time.");
        displayError();
    }
}

// Existing fetchDataAndDisplay() function
void fetchDataAndDisplay() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;  // HTTPS
        client.setInsecure();

        HTTPClient http;
        String currentUrl = googleSheetsUrl;
        bool redirecting = true;
        bool requestFailed = false;

        while (redirecting) {
            http.begin(client, currentUrl);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.setTimeout(15000);

            int httpCode = http.GET();
            if (httpCode == -1 || httpCode == -11) {
                Serial.print("HTTP GET failed, error code: ");
                Serial.println(httpCode);
                requestFailed = true;
                break;
            }

            if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
                String redirectUrl = http.getLocation();
                Serial.println("Redirected to: " + redirectUrl);

                if (redirectUrl.length() == 0) {
                    Serial.println("Error: No redirect location.");
                    requestFailed = true;
                    break;
                }

                currentUrl = redirectUrl;
                http.end();
                delay(100);
            } else {
                Serial.print("HTTP Response Code: ");
                Serial.println(httpCode);

                if (httpCode > 0) {
                    String payload = http.getString();
                    Serial.println("Response: " + payload);

                    String tokens[8];
                    int index = 0;
                    char* token = strtok(&payload[0], ",");
                    while (token != NULL && index < 8) {
                        tokens[index++] = String(token);
                        token = strtok(NULL, ",");
                    }

                    if (index >= 1) {
                        displayNumberWithDecimal(display, tokens[3]);
                    }
                } else {
                    Serial.println("Failed to retrieve data");
                    requestFailed = true;
                }
                redirecting = false;
            }
        }
        if (requestFailed) displayError();
        http.end();
    } else {
        Serial.println("WiFi not connected");
        displayError();
    }
}

// Helper functions for display
void displayNumberWithDecimal(TM1637Display &display, String numberStr) {
    uint8_t segments[4] = {0};
    int digitIndex = 0;

    for (int i = 0; i < numberStr.length() && digitIndex < 4; i++) {
        char c = numberStr.charAt(i);
        if (c == '.') {
            if (digitIndex > 0 && digitIndex < 4) {
                segments[digitIndex - 1] |= 0b10000000;
            }
        } else if (isdigit(c)) {
            segments[digitIndex++] = display.encodeDigit(c - '0');
        }
    }
    display.setSegments(segments, 4, 0);
}

void displayError() {
    uint8_t Err[] = { 0b01111001, 0b01010000, 0b01010000, 0b00000000 };
    display.setSegments(Err);
}

void displayCONN() {
    uint8_t CONN[] = { 0b00111001, 0b01011100, 0b01010100, 0b01010100 };
    display.setSegments(CONN);
}

void displayDashes() {
    uint8_t dashes[] = { 0b01000000, 0b01000000, 0b01000000, 0b01000000 };
    display.setSegments(dashes);
}
