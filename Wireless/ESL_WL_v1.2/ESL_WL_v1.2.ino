#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TM1637Display.h>

// Replace these with your network credentials
const char* ssid = "Vineeth";
const char* password = "iron7613";

// Google Apps Script web app URL
const char* googleSheetsUrl = "https://script.google.com/macros/s/AKfycbycK5WSGJnVsW5JF8KQoqgJuagm4arhQgX5nijMGBIBSHnaRprARSzFfiMPAGvByOVc/exec";

// Display connected to GPIO 16 (CLK) and GPIO 14 (DIO)
TM1637Display display(16, 14);

// Helper function to display numbers with decimal points
void displayNumberWithDecimal(TM1637Display &display, String numberStr) {
    uint8_t segments[4] = {0};  // Array to store segment data
    int digitIndex = 0;  // Index for tracking digits

    // Parse through the string to extract digits and decimal
    for (int i = 0; i < numberStr.length() && digitIndex < 4; i++) {
        char c = numberStr.charAt(i);

        if (c == '.') {
            // Add decimal point to the previous digit, except for the last digit
            if (digitIndex > 0 && digitIndex < 4) {
                segments[digitIndex - 1] |= 0b10000000;  // Set MSB to 1 for decimal point
            }
        } else if (isdigit(c)) {
            // Encode digit and store it in the segment array
            segments[digitIndex++] = display.encodeDigit(c - '0');
        }
    }

    // Display the digits with decimals
    display.setSegments(segments, 4, 0);
}

void setup() {
    Serial.begin(115200);
  
    // Initialize the TM1637 display
    display.setBrightness(0x0f);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        displayDashes();
    }
    Serial.println("\nConnected to WiFi");
    displayCONN();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;  // Use WiFiClientSecure for HTTPS
        client.setInsecure();  // Disable SSL certificate validation

        HTTPClient http;
        String currentUrl = googleSheetsUrl;
        bool redirecting = true;
        bool requestFailed = false;

        while (redirecting) {
            http.begin(client, currentUrl);  // Use client with the URL
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Follow redirects
            http.setTimeout(15000);  // Increase timeout to 15 seconds
            
            int httpCode = http.GET();

            if (httpCode == -1 || httpCode == -11) {
                Serial.print("HTTP GET failed, error code: ");
                Serial.println(httpCode);
                requestFailed = true;
                break;
            }

            if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
                // Handle redirect
                String redirectUrl = http.getLocation();  // Get the redirect URL
                Serial.println("Redirected to: " + redirectUrl);

                if (redirectUrl.length() == 0) {
                    Serial.println("Error: No redirect location provided.");
                    requestFailed = true;
                    break;
                }

                currentUrl = redirectUrl;  // Set the current URL to the new redirected URL
                http.end();  // Close the connection before starting a new one
                delay(100);
            } else {
                // No more redirects; print the response
                Serial.print("HTTP Response Code: ");
                Serial.println(httpCode);

                if (httpCode > 0) {
                    String payload = http.getString();
                    Serial.println("Response: " + payload);

                    // Assume payload is a comma-separated string of numbers (like "1234,567.8,...")
                    String tokens[8];
                    int index = 0;
                    char* token = strtok(&payload[0], ",");
                    while (token != NULL && index < 8) {
                        tokens[index++] = String(token);
                        token = strtok(NULL, ",");
                    }

                    // Display only the first number on the TM1637 display with decimal support
                    if (index >= 1) {
                        displayNumberWithDecimal(display, tokens[4]);
                    }
                } else {
                    Serial.println("Failed to retrieve data");
                    requestFailed = true;
                }

                // Stop the redirection loop
                redirecting = false;
            }
        }

        if (requestFailed) {
            displayError();
        }

        http.end();  // Close the connection
    } else {
        Serial.println("WiFi not connected");
        displayError();
    }

    delay(5000);  // Update every 5 seconds
}

// Function to display "Err" on the TM1637 display
void displayError() {
    uint8_t Err[] = {
        0b01111001,  // 'E'
        0b01010000,  // 'r'
        0b01010000,  // 'r'
        0b00000000   // Blank or you can leave it off
    };
    display.setSegments(Err);
}

// Function to display "CONN" on the TM1637 display
void displayCONN() {
    uint8_t CONN[] = {
        0b00111001,  // 'C' -> Segments: a, d, e, f
        0b01011100,  // 'o' -> Segments: e, d, c, g
        0b01010100,  // 'n'
        0b01010100   // 'n'
    };
    display.setSegments(CONN);
}

// Function to display "----" on the TM1637 display
void displayDashes() {
    uint8_t dashes[] = {
        0b01000000,  // '-'
        0b01000000,  // '-'
        0b01000000,  // '-'
        0b01000000   // '-'
    };
    display.setSegments(dashes);
}
