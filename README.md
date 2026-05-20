# BW16 WiFi Deauther - Watch Dogs 2 DedSec Edition

🔥 A 2.4GHz & 5GHz WiFi Deauther for **BW16 (RTL8720DN)** with Watch Dogs 2 DedSec style UI, button navigation, and glitch splash screen.

---

## ✨ Features

- ✅ **WiFi Scan** - Detect 2.4GHz & 5GHz networks with channel info
- ✅ **Deauth Attack** - ALL mode (every AP) & SELECT mode (single target)
- ✅ **BLE Spam** - Module placeholder (coming soon)
- ✅ **DedSec UI** - Watch Dogs 2 inspired cyberpunk interface
- ✅ **Splash Glitch** - Pixel shift animation on boot
- ✅ **Button Navigation** - UP/DOWN/OK/BACK physical buttons
- ✅ **System Reboot** - Reboot via menu with watchdog timer
- ✅ **5GHz Support** - Native 5GHz deauth (RTL8720DN advantage!)

---

## 🔧 Hardware Requirements

| Component | Specification | Est. Price |
|-----------|---------------|------------|
| **BW16 DevKit** | RTL8720DN, Dual-band WiFi 2.4GHz + 5GHz | ~$5 |
| **TFT Display** | 1.44" ST7735, 128x160, SPI Interface | ~$3 |
| **Push Buttons** | 4x Tactile Switch 6x6mm | ~$1 |
| **Jumper Wires** | Female-to-Female Dupont | ~$1 |
| **Breadboard** | Optional, for cleaner wiring | ~$2 |

> **Total Build Cost: ~$10-12**

---

## 🔌 Wiring

### TFT ST7735 (1.44" 128x160)
| TFT Pin | BW16 Pin | GPIO |
|---------|----------|------|
| VCC | 3.3V | - |
| GND | GND | - |
| CS | PA15 | GPIO 9 |
| DC/RS | PA26 | GPIO 8 |
| RST | PA25 | GPIO 7 |
| MOSI (SDA) | PA12 | GPIO 12 |
| SCK (SCL) | PA14 | GPIO 10 |
| LED (BLK) | 3.3V | - |

### Navigation Buttons
| Button | BW16 Pin | GPIO | Mode |
|--------|----------|------|------|
| UP | PB1 | GPIO 4 | INPUT_PULLUP |
| DOWN | PB2 | GPIO 5 | INPUT_PULLUP |
| OK | PB3 | GPIO 6 | INPUT_PULLUP |
| BACK | PA30 | GPIO 3 | INPUT_PULLUP |

> Buttons connect directly between GPIO and GND. No external resistor needed (uses internal pull-up).

---

## 📚 Required Libraries

Install via Arduino Library Manager:

| Library | Author | Purpose |
|---------|--------|---------|
| **Adafruit GFX Library** | Adafruit | Graphics core |
| **Adafruit ST7735** | Adafruit | TFT Display driver |
| **WiFi** | Realtek (built-in) | WiFi functions |
| **SPI** | Arduino (built-in) | SPI communication |

---

## 🚀 How to Use

1. **Upload** `jamm2.ino` to BW16 via Arduino IDE
2. **Splash screen** appears with glitch effect (3 seconds)
3. **Main Menu** - 4 options with DedSec style UI
4. **Navigate** using UP/DOWN buttons
5. **Select** with OK button
6. **Go back** with BACK button

### Menu Options:
| Menu | Description |
|------|-------------|
| **WiFi_SCAN** | Scan 2.4GHz & 5GHz networks |
| **DEAUTH** | Launch deauth attack (ALL or SELECT mode) |
| **BLE_SPAM** | BLE spam module (coming soon) |
| **REBOOT** | System restart |

---
## 🎮 Controls

| Button | Function |
|--------|----------|
| **UP** | Navigate up / Change mode |
| **DOWN** | Navigate down / Change mode |
| **OK** | Select / Start attack / Stop attack |
| **BACK** | Go back / Return to menu |

---

## 🔮 Upcoming Features

- 🟢 **BLE Spam** - Bluetooth Low Energy advertising spam
- 🟢 **WiFi Phishing** - Evil twin / captive portal
- 🟢 **TFT ILI9225** - Support for 2.2" display
- 🟢 **Touch Screen** 
- 🟢 **Web UI** - Browser-based control (like original Deauther)

---

## 📝 Changelog

### v1.0 (May 2025)
- Initial release
- WiFi Scan 2.4GHz + 5GHz
- Deauth ALL & SELECT mode
- DedSec Watch Dogs 2 UI
- Splash screen with glitch effect
- Button navigation
- System reboot

---

## ⚠️ Disclaimer

**THIS PROJECT IS FOR EDUCATIONAL PURPOSES ONLY!**

- Only use on networks you **own** or have **permission to test**
- Deauth attacks can **disrupt WiFi networks**
- **Unauthorized use is illegal** in many countries
- The author is **not responsible** for any misuse or damage

---

## 📄 License

This project is **FREE and OPEN SOURCE**

- ✅ Feel free to **use** the code
- ✅ Feel free to **modify** the code
- ✅ Feel free to **share** the code
- ✅ Feel free to **develop your own version**
- ❌ No warranty provided


---

## 🌐 Community

its free, dont buy me a coffee, not open to donation. 

**Happy Hacking! (N0t = Ethically   :)  ;) wink** 🔥