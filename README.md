# 🔐 Arduino-Based Electronic Safe (Embedded System Project)

## 📌 Overview

This project implements a microcontroller-based electronic safe system inspired by hotel room safes.
The system is built using an Arduino Uno and combines hardware control, embedded programming, and user interaction.

The safe can be locked and unlocked via a 4-digit PIN entered through a keypad.
It integrates persistent storage, power-saving mechanisms, and multiple hardware components to simulate a real-world embedded control system.

---

## 🎯 Features

* 🔢 4-digit PIN authentication via I2C keypad
* 💾 Persistent password storage using EEPROM
* 🔒 Servo motor-based locking mechanism
* 📟 4-digit 7-segment display (TM1637) for user feedback
* 🚦 Status indication using LEDs (green, yellow, red)
* 🔊 Acoustic feedback via buzzer
* ⚡ Power-saving mode with interrupt-based wake-up
* 🚪 Input only allowed when safe door is physically closed
* 🔁 Automatic reset after unlocking (ready for next user)

---

## 🧠 System Behavior

### Locking Process

1. User enters a 4-digit PIN
2. Press `#` to confirm
3. Safe locks via servo motor
4. PIN is stored in EEPROM
5. LED sequence indicates successful locking

### Unlocking Process

1. User enters the correct PIN
2. Safe unlocks via servo motor
3. Green LED indicates success
4. Stored PIN is deleted
5. Safe resets for next usage

### Wrong Input

* Red LED + buzzer signal
* No unlocking

---

## ⚙️ Hardware Components

* Arduino Uno
* 4x4 Keypad (I2C)
* TM1637 4-digit 7-segment display
* Servo motor (locking mechanism)
* LEDs (green, yellow, red)
* Active buzzer
* EEPROM (internal)
* Breadboard & jumper wires
* Custom-built safe housing

---

## 🧩 Technical Highlights

### Embedded Concepts Used

* Non-blocking timing using `millis()` (no delay-based logic)
* Interrupt-based wake-up (low power mode)
* Persistent data storage with EEPROM
* State-based system logic (locked / unlocked / idle / error)
* Hardware abstraction via modular functions

### Interfaces & Communication

* I2C communication (keypad)
* GPIO control (LEDs, buzzer, servo)
* Digital input detection (door state via circuit)

---

## 🔋 Power Management

The system enters a low-power mode after inactivity:

* Display is turned off
* Microcontroller enters sleep mode
* Wake-up triggered via interrupt (keypad or door input)

---

## 🧪 Testing & Results

The system was tested for:

* Correct PIN validation 
* Incorrect PIN handling 
* EEPROM persistence after power loss 
* Lock/unlock mechanism reliability 

Minor optimizations were applied:

* Servo calibration
* LED timing adjustments

---

## 🛠️ Setup & Usage

1. Connect all hardware components according to the circuit diagram
<img width="626" height="743" alt="image" src="https://github.com/user-attachments/assets/a8302c67-97c7-40a4-9a6a-166755140f95" />

2. Upload the code to the Arduino Uno
3. Power the system
4. Close the safe door
5. Enter a PIN and press `#` to lock

---

## 👥 Team

* **Kevin** – Hardware design, implementation, embedded logic
* **Asad** – Documentation and presentation

---

## 📄 License

This project is created for educational purposes.
Feel free to use and modify it.

---

## 💡 Notes

This project was developed as part of a vocational training module on cyber-physical systems.
It demonstrates practical experience in embedded systems, hardware interaction, and real-time logic design.

