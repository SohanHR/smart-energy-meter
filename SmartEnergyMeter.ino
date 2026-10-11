#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);


// =================================================
// PIN DEFINITIONS
// =================================================

const int voltagePin = A0;
const int currentPin = A1;
const int pfPin = A2;

const int greenLED = 6;
const int redLED = 7;
const int relayPin = 8;
const int buzzerPin = 9;
const int resetButton = 2;


// =================================================
// PROTECTION LIMITS
// =================================================

const float maxCurrent = 10.0;
const float minVoltage = 200.0;
const float maxVoltage = 250.0;


// =================================================
// POWER FACTOR WARNING
// =================================================

const float minPowerFactor = 0.70;

// Warning clears only after PF rises above 0.75.
const float pfWarningReset = 0.75;


// =================================================
// LOAD CLASSIFICATION LIMITS
// =================================================

const float lowLoadLimit = 500.0;
const float normalLoadLimit = 1500.0;
const float highLoadLimit = 2500.0;


// =================================================
// ELECTRICITY TARIFF
// =================================================

const float tariff = 8.0;


// =================================================
// MEASURED / SENSOR VALUES
// =================================================

float voltage = 0.0;

float sensorCurrent = 0.0;

float current = 0.0;

float powerFactor = 1.0;

float sensorPower = 0.0;

float power = 0.0;


// =================================================
// ENERGY AND COST
// =================================================

float energy_kWh = 0.0;

float cost = 0.0;


// =================================================
// RECORDED MAXIMUM VALUES
// =================================================

float maxPower = 0.0;

float maxCurrentRecorded = 0.0;


// =================================================
// LOAD OPERATING TIME
// =================================================

float loadOperatingTimeHours = 0.0;

float averagePower = 0.0;


// =================================================
// MAXIMUM DEMAND
// =================================================

float maximumDemand = 0.0;

float demandEnergy_kWh = 0.0;


// =================================================
// PROTECTION FAULT
// =================================================

bool faultState = false;

int faultType = 0;

// 0 = No fault
// 1 = Overcurrent
// 2 = Overvoltage
// 3 = Undervoltage


// =================================================
// LOW POWER FACTOR WARNING
// =================================================

bool lowPFFault = false;


// =================================================
// LOAD STATUS
// =================================================

int loadStatus = 0;

// 0 = LOW LOAD
// 1 = NORMAL LOAD
// 2 = HIGH LOAD
// 3 = HEAVY LOAD


// =================================================
// FAULT EVENT COUNTERS
// =================================================

unsigned long overcurrentEvents = 0;

unsigned long overvoltageEvents = 0;

unsigned long undervoltageEvents = 0;


// =================================================
// LCD NORMAL SCREENS
// =================================================

// 0 = Voltage + Current + PF + Power
// 1 = Power + PF + Voltage
// 2 = Energy
// 3 = Cost
// 4 = Peak Power
// 5 = Maximum Current
// 6 = Average Power
// 7 = Load Status
// 8 = Maximum Demand
// 9 = Fault Event Log

int screen = 0;


// =================================================
// LCD DISPLAY MODES
// =================================================

// 0 = Normal
// 1 = Protection Fault
// 2 = Low PF Warning
// 3 = Input Adjustment Mode

int displayMode = -1;

int displayedFaultType = 0;


// =================================================
// INPUT ADJUSTMENT MODE
// =================================================

bool inputAdjustMode = false;

bool buttonActionHandled = false;

unsigned long buttonPressStart = 0;

const unsigned long longPressTime = 2000;

unsigned long previousAdjustDisplayMillis = 0;

const unsigned long adjustDisplayInterval = 200;


// =================================================
// LOW PF LED BLINK
// =================================================

unsigned long previousPFLedMillis = 0;

bool pfRedLEDState = false;

const unsigned long pfLEDInterval = 500;


// =================================================
// FAULT DISPLAY UPDATE
// =================================================

// The fault title is shown continuously,
// while the actual voltage/current value
// is updated every 200 ms.

unsigned long previousFaultDisplayMillis = 0;

const unsigned long faultDisplayInterval = 200;


// =================================================
// GENERAL TIMING
// =================================================

unsigned long previousEnergyMillis = 0;

unsigned long previousDisplayMillis = 0;

unsigned long previousSerialMillis = 0;

unsigned long previousEEPROMMillis = 0;

unsigned long demandStartMillis = 0;

unsigned long previousPFDisplayMillis = 0;


// =================================================
// DEMAND INTERVAL
// =================================================

// 1 minute for Wokwi demonstration.

const unsigned long demandInterval = 60000;


// =================================================
// LCD TIMING
// =================================================

const unsigned long normalScreenInterval = 3000;

const unsigned long pfDisplayInterval = 500;


// =================================================
// EEPROM ADDRESSES
// =================================================

const int energyAddress = 0;

const int maxPowerAddress = 10;

const int maxCurrentAddress = 20;

const int operatingTimeAddress = 30;

const int maximumDemandAddress = 40;

const int overcurrentEventsAddress = 50;

const int overvoltageEventsAddress = 60;

const int undervoltageEventsAddress = 70;


// =================================================
// EEPROM VERSION / SIGNATURE
// =================================================

const int eepromSignatureAddress = 80;

const uint16_t eepromSignature = 0xA217;


// =================================================
// EEPROM VALIDATION
// =================================================

const unsigned long maximumValidEventCount =
  1000000UL;


// =================================================
// FUNCTION DECLARATIONS
// =================================================

void loadEEPROMData();

void updateLCD(
  unsigned long currentMillis
);

void displayNormalScreen();

void updateProtectionOutputs();

void saveEEPROMData();

void printSerialData();

void detectProtectionFault();

void handleResetAndAdjustmentButton(
  unsigned long currentMillis
);

void updateFaultDisplay(
  unsigned long currentMillis
);


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(9600);

  lcd.init();

  lcd.backlight();


  // ---------------- PIN MODES ----------------

  pinMode(
    greenLED,
    OUTPUT
  );

  pinMode(
    redLED,
    OUTPUT
  );

  pinMode(
    relayPin,
    OUTPUT
  );

  pinMode(
    buzzerPin,
    OUTPUT
  );

  pinMode(
    resetButton,
    INPUT_PULLUP
  );


  // ---------------- LOAD EEPROM ----------------

  loadEEPROMData();


  // ---------------- INITIALIZE TIMERS ----------------

  unsigned long currentMillis =
    millis();

  previousEnergyMillis =
    currentMillis;

  previousDisplayMillis =
    currentMillis;

  previousSerialMillis =
    currentMillis;

  previousEEPROMMillis =
    currentMillis;

  demandStartMillis =
    currentMillis;

  previousPFDisplayMillis =
    currentMillis;

  previousAdjustDisplayMillis =
    currentMillis;

  previousPFLedMillis =
    currentMillis;

  previousFaultDisplayMillis =
    currentMillis;


  // ---------------- INITIAL AVERAGE POWER ----------------

  if (
    loadOperatingTimeHours > 0.0
  ) {

    averagePower =
      (
        energy_kWh /
        loadOperatingTimeHours
      ) *
      1000.0;

  }

  else {

    averagePower =
      0.0;

  }


  // ---------------- STARTUP LCD ----------------

  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Smart Energy"
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "Meter"
  );

  delay(2000);

  lcd.clear();


  // ---------------- STARTUP SERIAL ----------------

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "     SMART ENERGY METER"
  );

  Serial.println(
    "================================"
  );


  Serial.print(
    "Stored Energy: "
  );

  Serial.print(
    energy_kWh,
    4
  );

  Serial.println(
    " kWh"
  );


  Serial.print(
    "Peak Power: "
  );

  Serial.print(
    maxPower,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Maximum Current: "
  );

  Serial.print(
    maxCurrentRecorded,
    2
  );

  Serial.println(
    " A"
  );


  Serial.print(
    "Load Operating Time: "
  );

  Serial.print(
    loadOperatingTimeHours,
    4
  );

  Serial.println(
    " h"
  );


  Serial.print(
    "Average Power: "
  );

  Serial.print(
    averagePower,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Maximum Demand: "
  );

  Serial.print(
    maximumDemand,
    0
  );

  Serial.println(
    " W"
  );


  Serial.println();

  Serial.println(
    "----- FAULT EVENT LOG -----"
  );


  Serial.print(
    "Overcurrent Events: "
  );

  Serial.println(
    overcurrentEvents
  );


  Serial.print(
    "Overvoltage Events: "
  );

  Serial.println(
    overvoltageEvents
  );


  Serial.print(
    "Undervoltage Events: "
  );

  Serial.println(
    undervoltageEvents
  );


  Serial.println(
    "================================"
  );

}


// =================================================
// MAIN LOOP
// =================================================

void loop() {

  unsigned long currentMillis =
    millis();


  // =================================================
  // 1. READ VOLTAGE
  // =================================================

  int rawVoltage =
    analogRead(
      voltagePin
    );

  voltage =
    map(
      rawVoltage,
      0,
      1023,
      180,
      260
    );


  // =================================================
  // 2. READ CURRENT
  // =================================================

  int rawCurrent =
    analogRead(
      currentPin
    );

  sensorCurrent =
    map(
      rawCurrent,
      0,
      1023,
      0,
      1500
    ) / 100.0;


  // =================================================
  // 3. READ POWER FACTOR
  // =================================================

  int rawPF =
    analogRead(
      pfPin
    );

  powerFactor =
    map(
      rawPF,
      0,
      1023,
      50,
      100
    ) / 100.0;


  // =================================================
  // 4. CALCULATE SENSOR POWER
  // =================================================

  sensorPower =
    voltage *
    sensorCurrent *
    powerFactor;


  // =================================================
  // 5. RECORD PEAK POWER
  // =================================================

  if (
    sensorPower >
    maxPower
  ) {

    maxPower =
      sensorPower;


    Serial.print(
      "New Peak Power: "
    );

    Serial.print(
      maxPower,
      0
    );

    Serial.println(
      " W"
    );

  }


  // =================================================
  // 6. RECORD MAXIMUM CURRENT
  // =================================================

  if (
    sensorCurrent >
    maxCurrentRecorded
  ) {

    maxCurrentRecorded =
      sensorCurrent;


    Serial.print(
      "New Maximum Current: "
    );

    Serial.print(
      maxCurrentRecorded,
      2
    );

    Serial.println(
      " A"
    );

  }


  // =================================================
  // 7. LOW POWER FACTOR
  // =================================================

  if (
    !lowPFFault &&
    powerFactor <
    minPowerFactor
  ) {

    lowPFFault =
      true;

  }

  else if (
    lowPFFault &&
    powerFactor >
    pfWarningReset
  ) {

    lowPFFault =
      false;

  }


  // =================================================
  // 8. PROTECTION FAULT DETECTION
  // =================================================

  detectProtectionFault();


  // =================================================
  // 9. BUTTON HANDLING
  // =================================================

  handleResetAndAdjustmentButton(
    currentMillis
  );


  // =================================================
  // 10. EFFECTIVE CURRENT
  // =================================================

  if (
    faultState
  ) {

    current =
      0.0;

  }

  else {

    current =
      sensorCurrent;

  }


  // =================================================
  // 11. EFFECTIVE POWER
  // =================================================

  if (
    faultState
  ) {

    power =
      0.0;

  }

  else {

    power =
      sensorPower;

  }


  // =================================================
  // 12. ENERGY
  // =================================================

  unsigned long elapsedTime =
    currentMillis -
    previousEnergyMillis;


  float elapsedHours =
    elapsedTime /
    3600000.0;


  float power_kW =
    power /
    1000.0;


  float energyIncrement =
    power_kW *
    elapsedHours;


  energy_kWh +=
    energyIncrement;


  previousEnergyMillis =
    currentMillis;


  // =================================================
  // 13. LOAD OPERATING TIME
  // =================================================

  if (
    !faultState &&
    sensorCurrent > 0.0
  ) {

    loadOperatingTimeHours +=
      elapsedHours;

  }


  // =================================================
  // 14. AVERAGE POWER
  // =================================================

  if (
    loadOperatingTimeHours >
    0.0
  ) {

    averagePower =
      (
        energy_kWh /
        loadOperatingTimeHours
      ) *
      1000.0;

  }

  else {

    averagePower =
      0.0;

  }


  // =================================================
  // 15. COST
  // =================================================

  cost =
    energy_kWh *
    tariff;


  // =================================================
  // 16. LOAD CLASSIFICATION
  // =================================================

  if (
    power <
    lowLoadLimit
  ) {

    loadStatus =
      0;

  }

  else if (
    power <
    normalLoadLimit
  ) {

    loadStatus =
      1;

  }

  else if (
    power <
    highLoadLimit
  ) {

    loadStatus =
      2;

  }

  else {

    loadStatus =
      3;

  }


  // =================================================
  // 17. MAXIMUM DEMAND
  // =================================================

  demandEnergy_kWh +=
    energyIncrement;


  if (
    currentMillis -
    demandStartMillis >=
    demandInterval
  ) {

    float demandPower =
      (
        demandEnergy_kWh /
        (
          demandInterval /
          3600000.0
        )
      ) *
      1000.0;


    if (
      demandPower >
      maximumDemand
    ) {

      maximumDemand =
        demandPower;


      Serial.println();

      Serial.print(
        "New Maximum Demand: "
      );

      Serial.print(
        maximumDemand,
        0
      );

      Serial.println(
        " W"
      );

    }


    demandEnergy_kWh =
      0.0;

    demandStartMillis =
      currentMillis;

  }


  // =================================================
  // 18. OUTPUT CONTROL
  // =================================================

  updateProtectionOutputs();


  // =================================================
  // 19. EEPROM
  // =================================================

  if (
    currentMillis -
    previousEEPROMMillis >=
    3000
  ) {

    saveEEPROMData();

    previousEEPROMMillis =
      currentMillis;

  }


  // =================================================
  // 20. LCD
  // =================================================

  updateLCD(
    currentMillis
  );


  // =================================================
  // 21. SERIAL
  // =================================================

  if (
    currentMillis -
    previousSerialMillis >=
    3000
  ) {

    previousSerialMillis =
      currentMillis;

    printSerialData();

  }

}


// =================================================
// RESET / ADJUSTMENT BUTTON
// =================================================

void handleResetAndAdjustmentButton(
  unsigned long currentMillis
) {

  bool buttonPressed =
    digitalRead(
      resetButton
    ) == LOW;


  // =================================================
  // BUTTON PRESSED
  // =================================================

  if (
    buttonPressed
  ) {

    if (
      buttonPressStart == 0
    ) {

      buttonPressStart =
        currentMillis;

      buttonActionHandled =
        false;

    }


    // =================================================
    // LONG PRESS
    // =================================================

    if (
      !buttonActionHandled &&
      !faultState &&
      (
        currentMillis -
        buttonPressStart
      ) >=
      longPressTime
    ) {

      inputAdjustMode =
        !inputAdjustMode;


      buttonActionHandled =
        true;


      displayMode =
        -1;

      displayedFaultType =
        0;


      previousAdjustDisplayMillis =
        currentMillis;


      if (
        inputAdjustMode
      ) {

        Serial.println();

        Serial.println(
          "INPUT ADJUSTMENT MODE: ON"
        );

      }

      else {

        Serial.println();

        Serial.println(
          "INPUT ADJUSTMENT MODE: OFF"
        );

        previousDisplayMillis =
          currentMillis;

      }

    }

  }


  // =================================================
  // BUTTON RELEASED
  // =================================================

  else {

    // Short press during protection fault
    // resets only when values are safe.

    if (
      buttonPressStart != 0 &&
      !buttonActionHandled &&
      faultState
    ) {

      if (
        sensorCurrent <=
        maxCurrent &&
        voltage >=
        minVoltage &&
        voltage <=
        maxVoltage
      ) {

        faultState =
          false;

        faultType =
          0;


        Serial.println();

        Serial.println(
          "Protection Fault Reset"
        );


        displayMode =
          -1;

        displayedFaultType =
          0;

        screen =
          0;


        previousEnergyMillis =
          currentMillis;

        previousDisplayMillis =
          currentMillis;

        previousPFDisplayMillis =
          currentMillis;

        previousFaultDisplayMillis =
          currentMillis;

      }

    }


    buttonPressStart =
      0;

    buttonActionHandled =
      false;

  }

}


// =================================================
// LOAD EEPROM DATA
// =================================================

void loadEEPROMData() {

  uint16_t storedSignature =
    0;


  EEPROM.get(
    eepromSignatureAddress,
    storedSignature
  );


  // ---------------- ENERGY ----------------

  EEPROM.get(
    energyAddress,
    energy_kWh
  );


  if (
    isnan(
      energy_kWh
    ) ||
    energy_kWh < 0.0
  ) {

    energy_kWh =
      0.0;

  }


  // ---------------- PEAK POWER ----------------

  EEPROM.get(
    maxPowerAddress,
    maxPower
  );


  if (
    isnan(
      maxPower
    ) ||
    maxPower < 0.0
  ) {

    maxPower =
      0.0;

  }


  // ---------------- MAXIMUM CURRENT ----------------

  EEPROM.get(
    maxCurrentAddress,
    maxCurrentRecorded
  );


  if (
    isnan(
      maxCurrentRecorded
    ) ||
    maxCurrentRecorded < 0.0
  ) {

    maxCurrentRecorded =
      0.0;

  }


  // ---------------- LOAD OPERATING TIME ----------------

  if (
    storedSignature ==
    eepromSignature
  ) {

    EEPROM.get(
      operatingTimeAddress,
      loadOperatingTimeHours
    );


    if (
      isnan(
        loadOperatingTimeHours
      ) ||
      loadOperatingTimeHours < 0.0
    ) {

      loadOperatingTimeHours =
        0.0;

    }

  }

  else {

    // Clear old operating-time meaning.

    loadOperatingTimeHours =
      0.0;


    EEPROM.put(
      operatingTimeAddress,
      loadOperatingTimeHours
    );


    EEPROM.put(
      eepromSignatureAddress,
      eepromSignature
    );

  }


  // ---------------- MAXIMUM DEMAND ----------------

  EEPROM.get(
    maximumDemandAddress,
    maximumDemand
  );


  if (
    isnan(
      maximumDemand
    ) ||
    maximumDemand < 0.0
  ) {

    maximumDemand =
      0.0;

  }


  // ---------------- OVERCURRENT ----------------

  EEPROM.get(
    overcurrentEventsAddress,
    overcurrentEvents
  );


  if (
    overcurrentEvents >
    maximumValidEventCount
  ) {

    overcurrentEvents =
      0;

  }


  // ---------------- OVERVOLTAGE ----------------

  EEPROM.get(
    overvoltageEventsAddress,
    overvoltageEvents
  );


  if (
    overvoltageEvents >
    maximumValidEventCount
  ) {

    overvoltageEvents =
      0;

  }


  // ---------------- UNDERVOLTAGE ----------------

  EEPROM.get(
    undervoltageEventsAddress,
    undervoltageEvents
  );


  if (
    undervoltageEvents >
    maximumValidEventCount
  ) {

    undervoltageEvents =
      0;

  }

}


// =================================================
// PROTECTION FAULT DETECTION
// =================================================

void detectProtectionFault() {

  // Identify the currently active protection condition.

  int activeFaultType =
    0;


  // =================================================
  // DETERMINE ACTIVE FAULT
  // =================================================

  if (
    sensorCurrent >
    maxCurrent
  ) {

    // Overcurrent gets highest priority.

    activeFaultType =
      1;

  }

  else if (
    voltage >
    maxVoltage
  ) {

    activeFaultType =
      2;

  }

  else if (
    voltage <
    minVoltage
  ) {

    activeFaultType =
      3;

  }


  // =================================================
  // NO ACTIVE CONDITION
  // =================================================

  if (
    activeFaultType == 0
  ) {

    // Fault remains latched.
    // Reset must be performed manually.

    return;

  }


  // =================================================
  // NEW FAULT
  // =================================================

  if (
    !faultState
  ) {

    faultState =
      true;

    faultType =
      activeFaultType;

  }


  // =================================================
  // FAULT TYPE CHANGED
  // =================================================

  else if (
    activeFaultType !=
    faultType
  ) {

    // Keep relay OFF, but update the
    // displayed/current fault type.

    faultType =
      activeFaultType;

  }


  // =================================================
  // SAME FAULT
  // =================================================

  else {

    return;

  }


  // =================================================
  // RECORD EVENT
  // =================================================

  if (
    faultType == 1
  ) {

    overcurrentEvents++;


    EEPROM.put(
      overcurrentEventsAddress,
      overcurrentEvents
    );


    Serial.println();

    Serial.println(
      "!!! OVERCURRENT EVENT !!!"
    );


    Serial.print(
      "Fault Current: "
    );

    Serial.print(
      sensorCurrent,
      2
    );

    Serial.println(
      " A"
    );


    Serial.print(
      "Overcurrent Events: "
    );

    Serial.println(
      overcurrentEvents
    );

  }


  else if (
    faultType == 2
  ) {

    overvoltageEvents++;


    EEPROM.put(
      overvoltageEventsAddress,
      overvoltageEvents
    );


    Serial.println();

    Serial.println(
      "!!! OVERVOLTAGE EVENT !!!"
    );


    Serial.print(
      "Fault Voltage: "
    );

    Serial.print(
      voltage,
      0
    );

    Serial.println(
      " V"
    );


    Serial.print(
      "Overvoltage Events: "
    );

    Serial.println(
      overvoltageEvents
    );

  }


  else if (
    faultType == 3
  ) {

    undervoltageEvents++;


    EEPROM.put(
      undervoltageEventsAddress,
      undervoltageEvents
    );


    Serial.println();

    Serial.println(
      "!!! UNDERVOLTAGE EVENT !!!"
    );


    Serial.print(
      "Fault Voltage: "
    );

    Serial.print(
      voltage,
      0
    );

    Serial.println(
      " V"
    );


    Serial.print(
      "Undervoltage Events: "
    );

    Serial.println(
      undervoltageEvents
    );

  }

}


// =================================================
// RELAY / LED / BUZZER CONTROL
// =================================================

void updateProtectionOutputs() {

  // =================================================
  // PROTECTION FAULT
  // =================================================

  if (
    faultState
  ) {

    digitalWrite(
      relayPin,
      LOW
    );

    digitalWrite(
      greenLED,
      LOW
    );

    digitalWrite(
      redLED,
      HIGH
    );

    digitalWrite(
      buzzerPin,
      HIGH
    );


    pfRedLEDState =
      false;

    previousPFLedMillis =
      millis();


    return;

  }


  // =================================================
  // LOW POWER FACTOR
  // =================================================

  if (
    lowPFFault
  ) {

    // Relay remains ON.

    digitalWrite(
      relayPin,
      HIGH
    );


    // Green LED remains ON.

    digitalWrite(
      greenLED,
      HIGH
    );


    // No buzzer.

    digitalWrite(
      buzzerPin,
      LOW
    );


    // Red LED blinks.

    if (
      millis() -
      previousPFLedMillis >=
      pfLEDInterval
    ) {

      previousPFLedMillis =
        millis();


      pfRedLEDState =
        !pfRedLEDState;


      digitalWrite(
        redLED,
        pfRedLEDState
      );

    }


    return;

  }


  // =================================================
  // NORMAL OPERATION
  // =================================================

  digitalWrite(
    relayPin,
    HIGH
  );

  digitalWrite(
    greenLED,
    HIGH
  );

  digitalWrite(
    redLED,
    LOW
  );

  digitalWrite(
    buzzerPin,
    LOW
  );


  pfRedLEDState =
    false;

  previousPFLedMillis =
    millis();

}


// =================================================
// SAVE EEPROM DATA
// =================================================

void saveEEPROMData() {

  EEPROM.put(
    energyAddress,
    energy_kWh
  );

  EEPROM.put(
    maxPowerAddress,
    maxPower
  );

  EEPROM.put(
    maxCurrentAddress,
    maxCurrentRecorded
  );

  EEPROM.put(
    operatingTimeAddress,
    loadOperatingTimeHours
  );

  EEPROM.put(
    maximumDemandAddress,
    maximumDemand
  );

  EEPROM.put(
    overcurrentEventsAddress,
    overcurrentEvents
  );

  EEPROM.put(
    overvoltageEventsAddress,
    overvoltageEvents
  );

  EEPROM.put(
    undervoltageEventsAddress,
    undervoltageEvents
  );

  EEPROM.put(
    eepromSignatureAddress,
    eepromSignature
  );

}


// =================================================
// UPDATE FAULT DISPLAY
// =================================================

void updateFaultDisplay(
  unsigned long currentMillis
) {

  bool faultTypeChanged =
    (
      displayMode != 1 ||
      displayedFaultType !=
      faultType
    );


  // =================================================
  // UPDATE FAULT TITLE
  // =================================================

  if (
    faultTypeChanged
  ) {

    lcd.clear();


    lcd.setCursor(
      0,
      0
    );


    if (
      faultType == 1
    ) {

      lcd.print(
        "OVERCURRENT!"
      );

    }

    else if (
      faultType == 2
    ) {

      lcd.print(
        "OVERVOLTAGE!"
      );

    }

    else if (
      faultType == 3
    ) {

      lcd.print(
        "UNDERVOLTAGE!"
      );

    }


    displayMode =
      1;

    displayedFaultType =
      faultType;


    // Update the value immediately.

    previousFaultDisplayMillis =
      currentMillis -
      faultDisplayInterval;

  }


  // =================================================
  // UPDATE LIVE VALUE
  // =================================================

  if (
    faultTypeChanged ||
    (
      currentMillis -
      previousFaultDisplayMillis >=
      faultDisplayInterval
    )
  ) {

    previousFaultDisplayMillis =
      currentMillis;


    lcd.setCursor(
      0,
      1
    );


    // =================================================
    // OVERCURRENT
    // =================================================

    if (
      faultType == 1
    ) {

      int currentTenths =
        (int)(
          sensorCurrent *
          10.0 +
          0.5
        );


      int currentWhole =
        currentTenths /
        10;

      int currentFraction =
        currentTenths %
        10;


      if (
        sensorCurrent >
        maxCurrent
      ) {

        // Example:
        // I:10.8A >10A

        lcd.print(
          "I:"
        );

        lcd.print(
          currentWhole
        );

        lcd.print(
          "."
        );

        lcd.print(
          currentFraction
        );

        lcd.print(
          "A >10A"
        );

      }

      else {

        // Example:
        // I:9.8A IN LIMIT
        // I:10.0A IN LIMIT

        lcd.print(
          "I:"
        );

        lcd.print(
          currentWhole
        );

        lcd.print(
          "."
        );

        lcd.print(
          currentFraction
        );

        lcd.print(
          "A IN LIMIT"
        );

      }

    }


    // =================================================
    // OVERVOLTAGE / UNDERVOLTAGE
    // =================================================

    else if (
      faultType == 2 ||
      faultType == 3
    ) {

      int voltageValue =
        (int)(
          voltage +
          0.5
        );


      // ---------------- OVERVOLTAGE ----------------

      if (
        faultType == 2
      ) {

        if (
          voltage >
          maxVoltage
        ) {

          // Example:
          // V:254V >250V

          lcd.print(
            "V:"
          );

          lcd.print(
            voltageValue
          );

          lcd.print(
            "V >250V"
          );

        }

        else {

          // Example:
          // V:249V IN LIMIT

          lcd.print(
            "V:"
          );

          lcd.print(
            voltageValue
          );

          lcd.print(
            "V IN LIMIT"
          );

        }

      }


      // ---------------- UNDERVOLTAGE ----------------

      else {

        if (
          voltage <
          minVoltage
        ) {

          // Example:
          // V:195V <200V

          lcd.print(
            "V:"
          );

          lcd.print(
            voltageValue
          );

          lcd.print(
            "V <200V"
          );

        }

        else {

          // Example:
          // V:205V IN LIMIT

          lcd.print(
            "V:"
          );

          lcd.print(
            voltageValue
          );

          lcd.print(
            "V IN LIMIT"
          );

        }

      }

    }

  }

}


// =================================================
// LCD DISPLAY CONTROL
// =================================================

void updateLCD(
  unsigned long currentMillis
) {

  // =================================================
  // 1. PROTECTION FAULT
  // =================================================

  if (
    faultState
  ) {

    updateFaultDisplay(
      currentMillis
    );

    return;

  }


  // =================================================
  // 2. INPUT ADJUSTMENT MODE
  // =================================================

  if (
    inputAdjustMode
  ) {

    if (
      displayMode != 3
    ) {

      lcd.clear();

      displayMode =
        3;

    }


    if (
      currentMillis -
      previousAdjustDisplayMillis >=
      adjustDisplayInterval
    ) {

      previousAdjustDisplayMillis =
        currentMillis;


      // ---------------- LINE 1 ----------------

      lcd.setCursor(
        0,
        0
      );

      lcd.print(
        "V:"
      );

      lcd.print(
        voltage,
        0
      );

      lcd.print(
        "V I:"
      );

      lcd.print(
        sensorCurrent,
        2
      );

      lcd.print(
        "A "
      );


      // ---------------- LINE 2 ----------------

      lcd.setCursor(
        0,
        1
      );

      lcd.print(
        "PF:"
      );

      lcd.print(
        powerFactor,
        2
      );


      if (
        lowPFFault
      ) {

        lcd.print(
          " LOW PF"
        );

      }

      else {

        lcd.print(
          "        "
        );

      }

    }

    return;

  }


  // =================================================
  // 3. LOW PF WARNING
  // =================================================

  if (
    lowPFFault
  ) {

    if (
      displayMode != 2
    ) {

      lcd.clear();


      lcd.setCursor(
        0,
        0
      );

      lcd.print(
        "LOW PF WARNING"
      );


      lcd.setCursor(
        0,
        1
      );

      lcd.print(
        "PF: "
      );

      lcd.print(
        powerFactor,
        2
      );


      displayMode =
        2;


      previousPFDisplayMillis =
        currentMillis;

    }


    if (
      currentMillis -
      previousPFDisplayMillis >=
      pfDisplayInterval
    ) {

      previousPFDisplayMillis =
        currentMillis;


      lcd.setCursor(
        0,
        1
      );

      lcd.print(
        "PF: "
      );

      lcd.print(
        powerFactor,
        2
      );

      lcd.print(
        "    "
      );

    }


    return;

  }


  // =================================================
  // 4. NORMAL OPERATION
  // =================================================

  if (
    displayMode != 0
  ) {

    lcd.clear();


    displayMode =
      0;

    screen =
      0;


    displayNormalScreen();


    previousDisplayMillis =
      currentMillis;


    return;

  }


  // =================================================
  // 5. NORMAL SCREEN ROTATION
  // =================================================

  if (
    currentMillis -
    previousDisplayMillis >=
    normalScreenInterval
  ) {

    previousDisplayMillis =
      currentMillis;


    screen++;


    if (
      screen >
      9
    ) {

      screen =
        0;

    }


    lcd.clear();

    displayNormalScreen();

  }

}


// =================================================
// DISPLAY NORMAL SCREEN
// =================================================

void displayNormalScreen() {

  // =================================================
  // SCREEN 0
  // =================================================

  if (
    screen == 0
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "V:"
    );

    lcd.print(
      voltage,
      0
    );

    lcd.print(
      " I:"
    );

    lcd.print(
      current,
      2
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "PF:"
    );

    lcd.print(
      powerFactor,
      2
    );

    lcd.print(
      " P:"
    );

    lcd.print(
      power,
      0
    );

    lcd.print(
      "W"
    );

  }


  // =================================================
  // SCREEN 1
  // =================================================

  else if (
    screen == 1
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Power:"
    );

    lcd.print(
      power,
      0
    );

    lcd.print(
      "W"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "PF:"
    );

    lcd.print(
      powerFactor,
      2
    );

    lcd.print(
      " V:"
    );

    lcd.print(
      voltage,
      0
    );

  }


  // =================================================
  // SCREEN 2
  // =================================================

  else if (
    screen == 2
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Energy:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      energy_kWh,
      4
    );

    lcd.print(
      " kWh"
    );

  }


  // =================================================
  // SCREEN 3
  // =================================================

  else if (
    screen == 3
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Cost:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "Rs "
    );

    lcd.print(
      cost,
      2
    );

  }


  // =================================================
  // SCREEN 4
  // =================================================

  else if (
    screen == 4
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Peak Power:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      maxPower,
      0
    );

    lcd.print(
      " W"
    );

  }


  // =================================================
  // SCREEN 5
  // =================================================

  else if (
    screen == 5
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Max Current:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      maxCurrentRecorded,
      2
    );

    lcd.print(
      " A"
    );

  }


  // =================================================
  // SCREEN 6
  // =================================================

  else if (
    screen == 6
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Avg Power:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      averagePower,
      0
    );

    lcd.print(
      " W"
    );

  }


  // =================================================
  // SCREEN 7
  // =================================================

  else if (
    screen == 7
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Load Status:"
    );


    lcd.setCursor(
      0,
      1
    );


    if (
      loadStatus == 0
    ) {

      lcd.print(
        "LOW LOAD"
      );

    }

    else if (
      loadStatus == 1
    ) {

      lcd.print(
        "NORMAL LOAD"
      );

    }

    else if (
      loadStatus == 2
    ) {

      lcd.print(
        "HIGH LOAD"
      );

    }

    else {

      lcd.print(
        "HEAVY LOAD"
      );

    }

  }


  // =================================================
  // SCREEN 8
  // =================================================

  else if (
    screen == 8
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Max Demand:"
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      maximumDemand,
      0
    );

    lcd.print(
      " W"
    );

  }


  // =================================================
  // SCREEN 9
  // =================================================

  else if (
    screen == 9
  ) {

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "OC:"
    );

    lcd.print(
      overcurrentEvents
    );

    lcd.print(
      " OV:"
    );

    lcd.print(
      overvoltageEvents
    );


    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "UV:"
    );

    lcd.print(
      undervoltageEvents
    );

  }

}


// =================================================
// SERIAL MONITOR
// =================================================

void printSerialData() {

  Serial.println();

  Serial.println(
    "--------- METER DATA ---------"
  );


  Serial.print(
    "Voltage: "
  );

  Serial.print(
    voltage,
    0
  );

  Serial.println(
    " V"
  );


  Serial.print(
    "Measured Current: "
  );

  Serial.print(
    sensorCurrent,
    2
  );

  Serial.println(
    " A"
  );


  Serial.print(
    "Load Current: "
  );

  Serial.print(
    current,
    2
  );

  Serial.println(
    " A"
  );


  Serial.print(
    "Power Factor: "
  );

  Serial.println(
    powerFactor,
    2
  );


  Serial.print(
    "Measured Power: "
  );

  Serial.print(
    sensorPower,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Load Power: "
  );

  Serial.print(
    power,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Energy: "
  );

  Serial.print(
    energy_kWh,
    4
  );

  Serial.println(
    " kWh"
  );


  Serial.print(
    "Cost: Rs "
  );

  Serial.println(
    cost,
    2
  );


  Serial.print(
    "Peak Power: "
  );

  Serial.print(
    maxPower,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Maximum Current: "
  );

  Serial.print(
    maxCurrentRecorded,
    2
  );

  Serial.println(
    " A"
  );


  Serial.print(
    "Average Power: "
  );

  Serial.print(
    averagePower,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Load Operating Time: "
  );

  Serial.print(
    loadOperatingTimeHours,
    4
  );

  Serial.println(
    " h"
  );


  Serial.print(
    "Maximum Demand: "
  );

  Serial.print(
    maximumDemand,
    0
  );

  Serial.println(
    " W"
  );


  Serial.print(
    "Load Status: "
  );


  if (
    loadStatus == 0
  ) {

    Serial.println(
      "LOW LOAD"
    );

  }

  else if (
    loadStatus == 1
  ) {

    Serial.println(
      "NORMAL LOAD"
    );

  }

  else if (
    loadStatus == 2
  ) {

    Serial.println(
      "HIGH LOAD"
    );

  }

  else {

    Serial.println(
      "HEAVY LOAD"
    );

  }


  Serial.print(
    "Protection: "
  );


  if (
    !faultState
  ) {

    Serial.println(
      "NORMAL"
    );

  }

  else if (
    faultType == 1
  ) {

    Serial.println(
      "OVERCURRENT"
    );

  }

  else if (
    faultType == 2
  ) {

    Serial.println(
      "OVERVOLTAGE"
    );

  }

  else if (
    faultType == 3
  ) {

    Serial.println(
      "UNDERVOLTAGE"
    );

  }


  Serial.print(
    "Low PF Warning: "
  );


  if (
    lowPFFault
  ) {

    Serial.println(
      "YES"
    );

  }

  else {

    Serial.println(
      "NO"
    );

  }


  Serial.print(
    "Input Adjustment Mode: "
  );


  if (
    inputAdjustMode
  ) {

    Serial.println(
      "ON"
    );

  }

  else {

    Serial.println(
      "OFF"
    );

  }


  Serial.println(
    "----- FAULT EVENT LOG -----"
  );


  Serial.print(
    "Overcurrent Events: "
  );

  Serial.println(
    overcurrentEvents
  );


  Serial.print(
    "Overvoltage Events: "
  );

  Serial.println(
    overvoltageEvents
  );


  Serial.print(
    "Undervoltage Events: "
  );

  Serial.println(
    undervoltageEvents
  );


  Serial.println(
    "------------------------------"
  );

}
