# Smart Energy Meter Using Arduino Uno

An Arduino Uno-based smart energy meter simulation developed in Wokwi to demonstrate electrical parameter monitoring, energy consumption calculation, load protection, and fault logging.

## Project Overview

This project simulates a smart energy meter using adjustable inputs to represent voltage, current, and power factor. The Arduino processes these inputs and displays the calculated electrical parameters and system status on a 16×2 I2C LCD.

The project focuses on energy monitoring, protection logic, and fault handling in an educational simulation environment.

## Features

- Simulated voltage, current, and power factor monitoring
- Active power calculation
- Energy consumption calculation in kWh
- Estimated electricity cost calculation
- Overcurrent, overvoltage, and undervoltage detection
- Relay control during fault conditions
- Buzzer and LED status indications
- Fault latching and manual reset
- Low power factor warning
- Peak power and maximum current tracking
- Maximum demand demonstration
- EEPROM-based data persistence
- LCD-based monitoring and status display

## Hardware and Simulation Components

- Arduino Uno
- 16×2 I2C LCD
- Potentiometers for simulated electrical inputs
- Relay module
- Buzzer
- Green and red LEDs
- Pushbutton
- Wokwi online simulator

## Electrical Calculations

**Active power:**

`P = V × I × PF`

Where:
- `P` = Active power in watts
- `V` = Voltage in volts
- `I` = Current in amperes
- `PF` = Power factor

**Energy consumption:**

`Energy (kWh) = Power (W) × Time (hours) / 1000`

**Estimated electricity cost:**

`Cost = Energy consumed (kWh) × Tariff per kWh`

The simulation uses a configurable tariff of ₹8 per kWh.

## Protection Logic

The program monitors the simulated inputs and activates fault handling when configured limits are exceeded.

| Condition | Simulated response |
|---|---|
| Overcurrent | Fault trip and warning |
| Overvoltage | Fault trip and warning |
| Undervoltage | Fault trip and warning |
| Low power factor | Warning indication |

A latched fault requires a manual reset after the inputs return to safe conditions.

## Simulation

Run the project in Wokwi:

https://wokwi.com/projects/470533202965811201

## Important Limitations

This is an educational simulation, not a certified electricity meter. Voltage, current, and power factor are simulated using potentiometer inputs rather than measured from a real electrical supply.

The simulation does not establish real-world measurement accuracy, true-RMS performance, or hardware protection reliability. No direct AC mains testing has been performed.

## Future Improvements

- Integrate suitable voltage and current sensors
- Validate measurements against calibrated instruments
- Develop a real-time monitoring dashboard
- Add data export and longer-term energy analysis
- Test protection and relay operation on a properly designed, isolated low-voltage test setup

## Author

Sohan H. R.
