// Forth for controlling an ESP32 with a REPL
//
// Wish List:
// - TODO: Automatically stream Forth source code from macOS to ESP32 on-save.
// - TODO: Wireless REPL (raw TCP)
//   - #include <WiFi.h>
//   - tether to iPhone

#include <Arduino.h>

#include <vector>

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

// Prints a newline.
void handleCR() {
    Serial.println();
}

// Example: 1000 sleep
void handleSleep() {
    int ms = pop();
    Serial.print("Sleeping for ");
    Serial.print(ms, DEC);
    Serial.println("ms...");
    delay(ms);
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

// Main
void setup() {
    Serial.begin(9600);
    Serial.println("ESP32 REPL");
}

// Forth doesn't read the entire program into memory at once and then
// parse it. It handles code line-by-line, token-by-token.
void loop() {
    static String line = "";

    bool compile = false;
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            // Skip full-width comments
            if (line[0] == '\\') {
                line = "";
                continue;
            }

            auto tokens = tokenize(line);
            bool comment = false;
            for (auto &token : tokens) {
                if (compile) {
                    if (token == "DO") {
                    }
                }

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
            line = "";
        } else {
            line += c;
        }
    }
}
