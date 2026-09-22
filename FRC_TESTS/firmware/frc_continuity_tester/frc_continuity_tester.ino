
// FRC 10-pin general-purpose continuity/pinout tester
// Header A (drive) -> cable/adapter/harness under test -> Header B (sense)
// See ../../README.md for wiring/pinout.
//
// Unlike a plain continuity checker, this does NOT assume a straight-through
// 1:1 cable. Each drive pin has a configurable *expected* set of sense pins
// (a bitmask), so it can verify crossed, remapped, split/fan-out, or
// intentionally-unused pins too.
//
// Serial commands (115200 baud):
//   MAP                     - print current expected mapping
//   SET <pin 1-10> <mask>   - set expected sense pins for a drive pin.
//                             <mask> is a comma list of sense pin numbers
//                             (1-10), or 0 for "expected open".
//                             e.g. "SET 3 7"      -> pin 3 expected only on pin 7 (crossed)
//                                  "SET 1 1,2"    -> pin 1 expected to fan out to pins 1 and 2 (split)
//                                  "SET 9 0"      -> pin 9 expected to be unconnected
//   IDENTITY                - reset mapping to straight-through (pin i -> pin i)
//   RUN                     - run one test pass immediately

const int PIN_COUNT = 10;

const int drivePins[PIN_COUNT] = { 4, 5, 13, 14, 16, 17, 18, 19, 21, 22 };
const int sensePins[PIN_COUNT] = { 23, 25, 26, 27, 32, 33, 34, 35, 36, 39 };

// Onboard blue LED on most ESP32 dev boards; unused by the pinout above.
const int LED_PIN = 2;
bool allPinsPass = false;

// expectedMap[i] = bitmask of sense pins (bit j = sense pin j+1) expected
// HIGH when drive pin i+1 is driven. 0 means "expected open".
uint16_t expectedMap[PIN_COUNT];

void setIdentityMap() {
  for (int i = 0; i < PIN_COUNT; i++) {
    expectedMap[i] = (uint16_t)(1 << i);
  }
}

void printMap() {
  Serial.println("Current expected mapping (drive pin -> sense pin(s)):");
  for (int i = 0; i < PIN_COUNT; i++) {
    Serial.printf("  %2d -> ", i + 1);
    if (expectedMap[i] == 0) {
      Serial.println("(expected open)");
      continue;
    }
    bool first = true;
    for (int j = 0; j < PIN_COUNT; j++) {
      if (expectedMap[i] & (1 << j)) {
        if (!first) Serial.print(",");
        Serial.print(j + 1);
        first = false;
      }
    }
    Serial.println();
  }
}

void allDriveHighZ() {
  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(drivePins[i], INPUT);
  }
}

// Parses "SET <pin> <mask>" already split into pinTok/maskTok strings.
void handleSet(const String &pinTok, const String &maskTok) {
  int pin = pinTok.toInt();
  if (pin < 1 || pin > PIN_COUNT) {
    Serial.println("ERR: pin must be 1-10");
    return;
  }

  uint16_t mask = 0;
  if (maskTok != "0") {
    int start = 0;
    while (start < (int)maskTok.length()) {
      int comma = maskTok.indexOf(',', start);
      String tok = (comma == -1) ? maskTok.substring(start) : maskTok.substring(start, comma);
      int sensePin = tok.toInt();
      if (sensePin < 1 || sensePin > PIN_COUNT) {
        Serial.printf("ERR: sense pin '%s' out of range 1-10\n", tok.c_str());
        return;
      }
      mask |= (1 << (sensePin - 1));
      if (comma == -1) break;
      start = comma + 1;
    }
  }

  expectedMap[pin - 1] = mask;
  Serial.printf("OK: pin %d expected mask updated\n", pin);
}

void handleSerialCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp1 = line.indexOf(' ');
  String cmd = (sp1 == -1) ? line : line.substring(0, sp1);
  cmd.toUpperCase();

  if (cmd == "MAP") {
    printMap();
  } else if (cmd == "IDENTITY") {
    setIdentityMap();
    Serial.println("OK: mapping reset to straight-through");
  } else if (cmd == "RUN") {
    runTestPass();
  } else if (cmd == "SET") {
    String rest = line.substring(sp1 + 1);
    rest.trim();
    int sp2 = rest.indexOf(' ');
    if (sp2 == -1) {
      Serial.println("ERR: usage SET <pin 1-10> <mask>");
      return;
    }
    String pinTok = rest.substring(0, sp2);
    String maskTok = rest.substring(sp2 + 1);
    maskTok.trim();
    handleSet(pinTok, maskTok);
  } else {
    Serial.println("ERR: unknown command. Use MAP, SET, IDENTITY, RUN");
  }
}

void runTestPass() {
  int failCount = 0;

  Serial.println("----------------------------------------");

  for (int i = 0; i < PIN_COUNT; i++) {
    allDriveHighZ();
    pinMode(drivePins[i], OUTPUT);
    digitalWrite(drivePins[i], HIGH);
    delayMicroseconds(200); // settle time

    uint16_t actual = 0;
    for (int j = 0; j < PIN_COUNT; j++) {
      if (digitalRead(sensePins[j]) == HIGH) {
        actual |= (1 << j);
      }
    }

    uint16_t expected = expectedMap[i];
    Serial.printf("Pin %2d (GPIO%2d drive): ", i + 1, drivePins[i]);

    if (actual == expected) {
      Serial.println("PASS");
    } else {
      uint16_t missing = expected & ~actual; // expected but not seen -> open
      uint16_t extra = actual & ~expected;   // seen but not expected -> short/miswire
      Serial.print("FAIL -");
      if (missing) {
        Serial.print(" missing:");
        for (int j = 0; j < PIN_COUNT; j++) {
          if (missing & (1 << j)) Serial.printf(" %d", j + 1);
        }
      }
      if (extra) {
        Serial.print(" unexpected:");
        for (int j = 0; j < PIN_COUNT; j++) {
          if (extra & (1 << j)) Serial.printf(" %d", j + 1);
        }
      }
      Serial.println();
      failCount++;
    }
  }

  allDriveHighZ();

  allPinsPass = (failCount == 0);
  if (allPinsPass) {
    Serial.println("Result: ALL 10 PINS MATCH EXPECTED MAP");
  } else {
    Serial.printf("Result: %d PIN(S) FAILED\n", failCount);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(sensePins[i], INPUT); // external 10k pulldown on each sense pin
  }
  allDriveHighZ();
  setIdentityMap();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println("FRC 10-pin general-purpose pinout tester");
  Serial.println("Type MAP to view mapping, SET to edit it, RUN to test now.");
  printMap();
}

void loop() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      handleSerialCommand(line);
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }

  runTestPass();

  digitalWrite(LED_PIN, allPinsPass ? HIGH : LOW);
  delay(2000);
}
