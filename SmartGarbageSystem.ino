// ================================================================
//       SMART GARBAGE MONITORING AND SORTING SYSTEM
//                     Arduino UNO
// ================================================================
//  Components:
//    - Moisture Sensor      → A0
//    - IR Sensor            → D6         ← NEWLY ADDED
//    - Ultrasonic (Dry Bin) → TRIG: D2 | ECHO: D3
//    - Ultrasonic (Wet Bin) → TRIG: D4 | ECHO: D5
//    - Servo Motor          → D9
//    - GSM Module (SIM800L) → RX: D10 | TX: D11  ← NOW ACTIVE
// ================================================================

#include <Servo.h>
#include <SoftwareSerial.h>   // For GSM module serial communication


// ================================================================
//  SECTION 1: PIN DEFINITIONS
//  Assigning each component to its respective Arduino pin
// ================================================================

#define MOISTURE_PIN   A0    // Moisture sensor → analog read pin
#define IR_SENSOR_PIN   6    // IR obstacle sensor → digital read pin
                             // OUTPUT: LOW = object detected, HIGH = clear
#define TRIG_DRY        2    // Dry bin ultrasonic → Trigger pin
#define ECHO_DRY        3    // Dry bin ultrasonic → Echo pin
#define TRIG_WET        4    // Wet bin ultrasonic → Trigger pin
#define ECHO_WET        5    // Wet bin ultrasonic → Echo pin
#define SERVO_PIN       9    // Servo motor → Signal pin


// ================================================================
//  SECTION 2: ADJUSTABLE SYSTEM PARAMETERS
//  Tweak these values to match your hardware setup
// ================================================================

const int   MOISTURE_THRESHOLD = 850;   // Raw ADC value (0–1023)
                                        // ABOVE threshold = DRY waste
                                        // BELOW threshold = WET waste

const float BIN_HEIGHT_CM      = 13.5;  // Physical height of each bin (cm)
                                        // Ultrasonic reads from lid downward

const int   FULL_ALERT_PERCENT = 80;    // Send GSM alert when bin ≥ this %


// ================================================================
//  SECTION 3: SERVO ANGLE POSITIONS
//  Standard servo range: 0° to 180°
//  We treat 90° as neutral center
// ================================================================

const int SERVO_NEUTRAL  = 90;   // Resting / center position
const int SERVO_WET_BIN  = 180;  // +90° clockwise       → directs to WET bin
const int SERVO_DRY_BIN  = 0;    // −90° counter-clockwise → directs to DRY bin


// ================================================================
//  SECTION 4: GSM CONFIGURATION  — SIM800L now active
// ================================================================

SoftwareSerial gsmSerial(10, 11);          // GSM RX→D10, TX→D11
const String PHONE_NUMBER = "+918121847746"; 


// ================================================================
//  SECTION 5: GLOBAL OBJECTS & VARIABLES
// ================================================================

Servo sortingServo;   // Servo motor object
bool  alertSentDry = false;  // Tracks if GSM alert was already sent for dry bin
bool  alertSentWet = false;  // Tracks if GSM alert was already sent for wet bin
                             // Prevents repeated SMS for same full-bin event


// ================================================================
//  SETUP  —  Runs ONCE at power-on or reset
// ================================================================

void setup() {

  // Start serial monitor communication at 9600 baud
  Serial.begin(9600);

  // Initialize servo and move it to neutral (center) position
  sortingServo.attach(SERVO_PIN);
  sortingServo.write(SERVO_NEUTRAL);

  // Configure Ultrasonic sensor pins
  pinMode(TRIG_DRY, OUTPUT);
  pinMode(ECHO_DRY, INPUT);
  pinMode(TRIG_WET, OUTPUT);
  pinMode(ECHO_WET, INPUT);

  // Configure IR sensor pin — reads LOW when object/trash is detected
  pinMode(IR_SENSOR_PIN, INPUT);

  // Moisture sensor pin (A0) is INPUT by default — no pinMode needed

  // Initialize GSM module serial communication
  gsmSerial.begin(9600);

  // Startup message on Serial Monitor
  delay(500);
  Serial.println(F("================================================"));
  Serial.println(F("   Smart Garbage Monitoring & Sorting System    "));
  Serial.println(F("================================================"));
  Serial.println(F(" Moisture Threshold : 850 (adjustable in code)"));
  Serial.println(F(" Bin Height         : 13.5 cm"));
  Serial.println(F(" Trigger Mode       : IR Sensor (Automatic)"));
  Serial.println(F("================================================"));
  Serial.println(F(" System Ready. Waiting for trash..."));
  Serial.println();

  delay(2000); // Give time to read the startup message
}


// ================================================================
//  MAIN LOOP  —  Runs REPEATEDLY after setup()
//  The system now waits for the IR sensor to detect trash.
//  When triggered, it runs the full sort + measure cycle instantly.
// ================================================================

void loop() {

  // Continuously poll the IR sensor
  // IR sensor outputs LOW when an object is detected in front of it
  if (digitalRead(IR_SENSOR_PIN) == LOW) {

    delay(80); // Brief debounce — ignore electrical noise / false triggers

    // Re-confirm detection after debounce to avoid false positives
    if (digitalRead(IR_SENSOR_PIN) == LOW) {

      printSectionHeader("NEW CYCLE STARTING");

      // ── STEP 1: IR confirmed trash ─────────────────────────────────
      Serial.println(F("  [IR SENSOR] >>> Trash Detected! Processing waste..."));
      Serial.println();
      delay(300); // Short pause for readability before processing

      // ── STEP 2: Read moisture sensor ────────────────────────────────
      int moistureValue = readMoisture();

      // ── STEP 3: Decide waste type based on threshold ─────────────────
      bool isWet = (moistureValue < MOISTURE_THRESHOLD);
      // If value < threshold  → moisture present → WET waste
      // If value >= threshold → dry surface      → DRY waste

      // ── STEP 4: Print moisture result ─────────────────────────────────
      Serial.print(F("  Moisture Sensor Value : "));
      Serial.println(moistureValue);
      Serial.print(F("  Waste Type Detected   : "));
      if (isWet) {
        Serial.println(F("*** WET WASTE ***"));
      } else {
        Serial.println(F("*** DRY WASTE ***"));
      }
      Serial.println();

      // ── STEP 5: Rotate servo to correct bin ────────────────────────
      controlServo(isWet);
      delay(1500); // Allow servo to physically complete rotation

      // ── STEP 6: Measure fill levels of both bins ───────────────────
      Serial.println(F("  Measuring bin fill levels..."));
      delay(300);

      float dryFillPercent = getBinFillLevel(TRIG_DRY, ECHO_DRY);
      float wetFillPercent = getBinFillLevel(TRIG_WET, ECHO_WET);

      // ── STEP 7: Display bin fill levels ───────────────────────────
      Serial.println();
      Serial.println(F("  ┌─────────────────────────────────┐"));
      Serial.println(F("  │         BIN STATUS REPORT        │"));
      Serial.println(F("  ├─────────────────────────────────┤"));
      Serial.print(F("  │ Dry Bin Fill Level  : "));
      printPaddedPercent(dryFillPercent);
      Serial.print(F("  │ Wet Bin Fill Level  : "));
      printPaddedPercent(wetFillPercent);
      Serial.println(F("  └─────────────────────────────────┘"));
      Serial.println();

      // ── STEP 8: Check for full bins and trigger GSM alert ──────────
      // alertSentDry / alertSentWet flags prevent repeated SMS spam
      // They reset only when bin level drops below threshold (bin emptied)

      if (dryFillPercent >= FULL_ALERT_PERCENT) {
        Serial.println(F("  ⚠ WARNING: DRY BIN IS FULL!"));
        if (!alertSentDry) {
          sendGSMAlert("DRY");
          alertSentDry = true; // Mark alert as sent — don't resend until emptied
        }
      } else {
        alertSentDry = false;  // Bin was emptied — reset flag
      }

      if (wetFillPercent >= FULL_ALERT_PERCENT) {
        Serial.println(F("  ⚠ WARNING: WET BIN IS FULL!"));
        if (!alertSentWet) {
          sendGSMAlert("WET");
          alertSentWet = true;
        }
      } else {
        alertSentWet = false;
      }

      // ── STEP 9: Return servo to neutral position after sorting ──────
      Serial.println();
      Serial.println(F("  Servo returning to NEUTRAL position (90°)..."));
      sortingServo.write(SERVO_NEUTRAL);

      // ── STEP 10: End of cycle ───────────────────────────────────────
      Serial.println();
      Serial.println(F("  Cycle complete. Waiting for next trash..."));
      printSectionFooter();
      Serial.println();

      // Wait until the trash has actually passed / IR clears
      // Prevents re-triggering the same piece of waste
      while (digitalRead(IR_SENSOR_PIN) == LOW) {
        delay(50); // Keep checking every 50ms until IR is clear again
      }
      delay(500); // Extra settling time before watching for next item
    }
  }
}


// ================================================================
//  FUNCTION: readMoisture()
//
//  PURPOSE  : Reads the raw analog value from the moisture sensor.
//  RETURNS  : Integer value between 0 (fully wet) and 1023 (fully dry)
//
//  HOW IT WORKS:
//    The moisture sensor has two probes. When placed in wet waste,
//    water conducts electricity between the probes, lowering resistance
//    and thus the analog voltage → lower ADC value.
//    In dry waste, resistance is high → higher ADC value.
// ================================================================

int readMoisture() {
  int raw = analogRead(MOISTURE_PIN); // Read 10-bit ADC value (0–1023)
  return raw;
}


// ================================================================
//  FUNCTION: controlServo(bool isWet)
//
//  PURPOSE  : Rotates the servo to direct waste into the correct bin.
//  INPUT    : isWet → true = WET waste, false = DRY waste
//
//  SERVO POSITIONS:
//    90°  → Neutral  (default resting position)
//    180° → Wet bin  (rotates 90° clockwise from neutral)
//    0°   → Dry bin  (rotates 90° counter-clockwise from neutral)
// ================================================================

void controlServo(bool isWet) {
  if (isWet) {
    Serial.println(F("  Servo Action : Rotating → WET BIN  (180° clockwise)"));
    sortingServo.write(SERVO_WET_BIN); // Move to 180°
  } else {
    Serial.println(F("  Servo Action : Rotating → DRY BIN  (0° counter-clockwise)"));
    sortingServo.write(SERVO_DRY_BIN); // Move to 0°
  }
}


// ================================================================
//  FUNCTION: getBinFillLevel(int trigPin, int echoPin)
//
//  PURPOSE  : Measures how full a bin is using an HC-SR04 ultrasonic sensor.
//  INPUT    : trigPin → the TRIG pin of the sensor
//             echoPin → the ECHO pin of the sensor
//  RETURNS  : Fill percentage (0.0% = empty, 100.0% = completely full)
//
//  HOW IT WORKS:
//    1. Send a 10µs HIGH pulse on TRIG → sensor emits ultrasonic burst
//    2. Measure how long ECHO stays HIGH → this is the round-trip travel time
//    3. Distance (cm) = (duration × speed of sound) / 2
//       Speed of sound ≈ 0.0343 cm/µs
//    4. Fill % = ((BIN_HEIGHT - distance) / BIN_HEIGHT) × 100
//       → If sensor reads 13.5 cm (full bin height) → 0% full (empty)
//       → If sensor reads ~0 cm (waste right at top)  → 100% full
// ================================================================

float getBinFillLevel(int trigPin, int echoPin) {

  // --- Trigger ultrasonic pulse ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);           // Ensure clean LOW signal first
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);          // 10 µs HIGH pulse triggers the sensor
  digitalWrite(trigPin, LOW);

  // --- Measure echo duration ---
  // pulseIn() returns 0 if no echo received within timeout (30ms)
  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout = 30ms

  // --- Convert duration to distance ---
  float distance = (duration * 0.0343) / 2.0;

  // --- Handle edge cases ---
  if (duration == 0) {
    // No echo → sensor may be out of range → assume bin is empty
    distance = BIN_HEIGHT_CM;
  }

  // Clamp distance to valid physical range
  distance = constrain(distance, 0.0, BIN_HEIGHT_CM);

  // --- Calculate fill percentage ---
  float fillPercent = ((BIN_HEIGHT_CM - distance) / BIN_HEIGHT_CM) * 100.0;
  fillPercent = constrain(fillPercent, 0.0, 100.0);

  return fillPercent;
}


// ================================================================
//  FUNCTION: sendGSMAlert(String binType)
//
//  PURPOSE  : Sends a real SMS alert via SIM800L when a bin is full.
//  INPUT    : binType → "DRY" or "WET"
//
//  GSM AT COMMANDS USED:
//    AT          → Test communication with GSM module (expects "OK")
//    AT+CMGF=1   → Switch to TEXT mode (human-readable SMS format)
//    AT+CMGS     → Start composing SMS to a phone number
//    Ctrl+Z (26) → ASCII code 26 — signals end of message and sends it
// ================================================================

void sendGSMAlert(String binType) {

  String alertMessage = "ALERT: " + binType +
                        " bin is FULL! Please empty it immediately. - SmartGarbage System";

  Serial.println(F("  ─────────────────────────────────────"));
  Serial.println(F("  [GSM] Preparing alert SMS..."));
  Serial.print(F("  [GSM] To      : "));
  Serial.println("xxxxxxxxxx");
  Serial.print(F("  [GSM] Message : "));
  Serial.println(alertMessage);

  // ── Step 1: Handshake — confirm GSM module is responding ────────────
  gsmSerial.println("AT");
  delay(1000);

  // ── Step 2: Set SMS to Text Mode ────────────────────────────────────
  gsmSerial.println("AT+CMGF=1");
  delay(1000);

  // ── Step 3: Address the recipient ───────────────────────────────────
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print("xxxxxxxxxx");
  gsmSerial.println("\"");
  delay(1000);

  // ── Step 4: Type the message body ───────────────────────────────────
  gsmSerial.print(alertMessage);
  delay(500);

  // ── Step 5: Send — Ctrl+Z (ASCII 26) tells GSM module to transmit ───
  gsmSerial.write(26);
  delay(5000); // Wait for GSM to finish transmitting

  Serial.println(F("  [GSM] Status  : ✓ Message Sent Successfully!"));
  Serial.println(F("  ─────────────────────────────────────"));
}


// ================================================================
//  HELPER: printSectionHeader / printSectionFooter / printPaddedPercent
//  These are purely for clean Serial Monitor formatting
// ================================================================

void printSectionHeader(const char* title) {
  Serial.println(F("================================================"));
  Serial.print(F("  "));
  Serial.println(title);
  Serial.println(F("================================================"));
}

void printSectionFooter() {
  Serial.println(F("================================================"));
}

// Prints fill percentage with a visual bar
void printPaddedPercent(float percent) {
  Serial.print(F("  │   "));
  Serial.print(percent, 1);
  Serial.print(F("%   ["));

  // Visual ASCII bar (10 segments)
  int filled = (int)(percent / 10);
  for (int i = 0; i < 10; i++) {
    if (i < filled) Serial.print(F("█"));
    else            Serial.print(F("░"));
  }
  Serial.println(F("]"));
}

// ================================================================
//  END OF PROGRAM
// ================================================================
