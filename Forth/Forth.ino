// Forth for controlling an ESP32 with a REPL
//
// Wish List:
// - TODO: Automatically stream Forth source code from macOS to ESP32 on-save.
// - TODO: Wireless REPL (raw TCP)
//   - #include <WiFi.h>
//   - tether to iPhone

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <vector>

// WiFi credentials ("Personal Hotspot")
const char *ssid = "iPhone";
const char *password = "security";

WiFiServer telnetServer(23);
WiFiClient telnetClient(23);

static std::vector<int> stack;

void interpret(const String &token) {
    // Try integer
    Serial.print("Unknown word: ");
    Serial.println(token);
}

std::vector<String> tokenize(const String &line) {
    std::vector<String> result;
    int i = 0;

    // Skip leading whitespace
    while (i < line.length() && line[i] == ' ') {
        i += 1;
    }

    String curr = "";
    while (i < line.length()) {
        if (line[i] == ' ' || line[i] == '\n') {
            result.push_back(curr);
            curr = "";
            i += 1;
        } else {
            curr += line[i];
            i += 1;
        }
    }
    if (curr.length() > 0) {
        result.push_back(curr);
    }
    return result;
}

int digit(char c) {
    if (c == '0') return 0;
    if (c == '1') return 1;
    if (c == '2') return 2;
    if (c == '3') return 3;
    if (c == '4') return 4;
    if (c == '5') return 5;
    if (c == '6') return 6;
    if (c == '7') return 7;
    if (c == '8') return 8;
    if (c == '9') return 9;
    return -1;
}

int parseInt(String &x) {
    // Special constants
    if (x == "OUTPUT") return OUTPUT;
    if (x == "INPUT") return INPUT;
    if (x == "PULLUP") return PULLUP;
    if (x == "HIGH") return HIGH;
    if (x == "LOW") return LOW;

    int result = 0;
    int i = x.length() - 1;
    int power = 0;
    while (i > -1) {
        int d = digit(x[i]);
        if (d == -1) {
            return -1;
        }
        result += digit(x[i]) * pow(10, power);
        i -= 1;
        power += 1;
    }
    return result;
}

// Convenience to mutatively pop and return from the stack.
int pop() {
    int x = stack.back();
    stack.pop_back();
    return x;
}

//////////////////////////////////////////////////////////////
// Builtins
//////////////////////////////////////////////////////////////

// Pops the top two elements off of the stack, adds them together
// and pushes the result back onto the stack.
void plus() {
    int rhs = pop();
    int lhs = pop();
    stack.push_back(lhs + rhs);
}

void mult() {
    int rhs = pop();
    int lhs = pop();
    stack.push_back(lhs * rhs);
}

// Duplicates the top of the stack.
void dup() {
    int x = stack.back();
    stack.push_back(x);
}

// Pops from the top of the stack and prints the result.
void dot() {
    if (stack.size() == 0) {
        Serial.println("EMPTY");
        return;
    }
    int x = pop();
    Serial.print(x, DEC);
    Serial.println();
}

// Same as "." but doesn't print the value after popping it off of the
// stack.
void handleDrop() {
    if (stack.size() == 0) {
        Serial.println("ERR: Just tried to DROP from an empty stack");
        return;
    }
    pop();
}

// Here are the following modes:
//   - INPUT  0
//   - OUTPUT 1
//   - PULLUP 2
//
// pinMode(32, OUTPUT) => "32 OUTPUT pinMode"
void handlePinMode() {
    int mode = pop();
    int pin = pop();
    Serial.print("Setting ");
    Serial.print(pin, DEC);
    Serial.print(" to ");
    Serial.println(mode == OUTPUT   ? "OUTPUT"
                   : mode == INPUT  ? "INPUT"
                   : mode == PULLUP ? "PULLUP"
                                    : "<Unsupported>");
    pinMode(pin, mode);
}

// Reads from a pin and pushes the result onto the top of the stack.
// Example: "32 digitalRead"
void handleDigitalRead() {
    int pin = pop();
    int result = digitalRead(pin);
    stack.push_back(result);
}

// Example: "32 HIGH digitalWrite"
void handleDigitalWrite() {
    int val = pop();
    int pin = pop();
    digitalWrite(pin, val);
}

// Makes a few attempts to connect to a known Wi-Fi network.
void handleConnectWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Already connected. Aborting...");
        return;
    }

    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    delay(500);

    // The ESP32 repeatedly tries reconnecting on its own, so calling
    // WiFi.begin again causes problems.
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 100) {
        if (attempt % 10 == 0) {
            Serial.println("Polling...");
        }
        delay(100);
        attempt += 1;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to Wi-Fi.");
    } else {
        Serial.println("Successfully connected to Wi-Fi!");
        Serial.print("IP addr: ");
        Serial.println(WiFi.localIP());
    }
}

// Gracefully disconnect from Wi-Fi.
void handleDisconnectWifi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Already disconnected. Aborting...");
        return;
    }

    Serial.println("Disconnecting from Wi-Fi...");
    WiFi.disconnect();

    // Retry with backoff
    int attempt = 0;
    while (WiFi.status() == WL_CONNECTED && attempt < 3) {
        int ms = pow(2, attempt) * 500;
        Serial.print("Failed. Retrying in ");
        Serial.print((float)ms / 1000, 2);
        Serial.println("s...");
        delay(ms);
        attempt += 1;
        WiFi.disconnect();
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Failed to disconnect from Wi-Fi");
    } else {
        Serial.println("Successfully disconnected from Wi-Fi!");
    }
}

// This starts a telnet server. It isn't that useful until you have a
// connected client.
void handleTelnet() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERR: You need to connect to Wi-Fi first. Aborting...");
        return;
    }

    telnetServer.begin();
    Serial.println("Listening on port 23...");
}

// Prints a newline.
void handleCR() { Serial.println(); }

// Prints ESP32 system information for diagnostics purposes. Eventually
// publishing this in an OTEL format would be quite useful.
void handleInfo() {
    Serial.println("System Diagnostics");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Min free heap: ");
    Serial.println(ESP.getMinFreeHeap());
    Serial.print("Max alloc heap: ");
    Serial.println(ESP.getMaxAllocHeap());

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Wi-Fi: Connected to \"");
        Serial.print(ssid);
        Serial.println("\"");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Wi-Fi: N/A");
        Serial.println("IP Address: N/A");
    }
}

// Example: 1000 sleep
void handleSleep() {
    int ms = pop();
    Serial.print("Sleeping for ");
    Serial.print(ms, DEC);
    Serial.println("ms...");
    delay(ms);
}

// Buffers Serial input character-by-character until it encounters a newline at
// which point it invokves the callback function with the buffered input.
void onMessage(void (*callback)(const String &)) {
    static String buffer = "";

    while (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            callback(buffer);
            buffer = "";
        } else {
            buffer += c;
        }
    }
}

// Prints the entire stack.
void debug() {
    Serial.print("[ ");
    for (auto &x : stack) {
        Serial.print(x, DEC);
        Serial.print(" ");
    }
    Serial.println("]");
}

// Receives a string representing all of the characters in an input buffer up
// until a newline character was received.
//   - Tokenize the characters
//   - Interpret the tokens
void handleMessage(const String &msg) {
    // Skip full-width comments
    if (msg[0] == '\\') {
        return;
    }

    bool comment = false;
    auto tokens = tokenize(msg);
    for (auto &token : tokens) {
        // Comments
        if (comment && token != ")") {
            continue;
        } else if (token == "(") {
            // Skip tokens until we encounter ")"
            comment = true;
        } else if (comment && token == ")") {
            comment = false;
        }
        // Builtins
        else if (token == "+") {
            plus();
        } else if (token == "*") {
            mult();
        } else if (token == "DUP") {
            dup();
        } else if (token == "CR") {
            handleCR();
        } else if (token == "info") {
            handleInfo();
        } else if (token == "sleep") {
            handleSleep();
        } else if (token == ".") {
            dot();
        } else if (token == "DROP") {
            handleDrop();
        } else if (token == ".s") {
            debug();
        } else if (token == "pinMode") {
            handlePinMode();
        } else if (token == "digitalRead") {
            handleDigitalRead();
        } else if (token == "digitalWrite") {
            handleDigitalWrite();
        } else if (token == "connectWifi") {
            handleConnectWifi();
        } else if (token == "disconnectWifi") {
            handleDisconnectWifi();
        } else if (token == "startTelnet") {
            handleTelnet();
        }
        // Try to parse the token as an integer
        else {
            int parsed = parseInt(token);
            if (parsed == -1) {
                Serial.print("ERR: Unsupported token \"");
                Serial.print(token);
                Serial.println("\"");
            } else {
                stack.push_back(parsed);
            }
        }
    }
}

// Main
void setup() {
    Serial.begin(9600);

    // Wi-Fi status LED:
    //   - ON: stable
    //   - OFF: blinking
    pinMode(32, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to Wi-Fi...");
        digitalWrite(32, HIGH);
        delay(500);
        digitalWrite(32, LOW);
        delay(500);
    }
    digitalWrite(32, HIGH);
    Serial.println("Connected to Wi-Fi.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
        Serial.println("OTA update received: " + type);
    });

    ArduinoOTA.begin();
    Serial.println("Ready to receive OTA updates...");
    Serial.println("ESP32 REPL");
}

// Forth doesn't read the entire program into memory at once and then
// parse it. It handles code line-by-line, token-by-token.
//
// Ideas:
//   - Persist WiFi credentials
//   - Prompt/warn on disconnect if connected over telnet
//
// We need logic to control where we're reading our inputs from because
// we can read from two sources:
//   - Serial input with Serial.read
//   - Connected telnet client with telnetClient.read
//
// If we're going to read from Telnet client we need:
//   - To be connected to Wi-Fi
//   - To have a connected client
// If we don't have these things, we should prefer serial.
void loop() {
    ArduinoOTA.handle();

    // Wi-Fi status LED:
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(32, LOW);
    } else {
        digitalWrite(32, HIGH);
    }

    onMessage(handleMessage);
}