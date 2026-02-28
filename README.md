# Debounce16 Library

A robust 16-bit pattern-based button debouncing library for ESP32 microcontrollers using the Arduino or ESP-IDF frameworks.

## Features

- **16-bit pattern recognition** for superior noise immunity
- **Non-blocking operation** - works with polling or hardware timer interrupts
- **Advanced press pattern detection**:
  - Single press detection
  - Double press detection with configurable time window
  - Long press detection with configurable threshold
- **Optional callback system** for event-driven programming
- **Multiple button support** with efficient resource management
- **Configurable timing parameters** for different use cases

## Installation

### Arduino IDE

1. Download the library as a ZIP file
2. In Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### PlatformIO

1. Copy the `debounce` folder to your project's `lib` directory
2. The library will be automatically detected and included

## Hardware Setup

### Active HIGH Configuration (button connects to VCC)

```
VCC ----o  o---- GPIO_PIN
           ↑
        Button

GPIO_PIN ----[10kΩ]---- GND
         Pull-down
```

### Active LOW Configuration (button connects to GND)

```
GPIO_PIN ----o  o---- GND
           ↑
        Button

VCC ----[10kΩ]---- GPIO_PIN
         Pull-up
```

Or use internal pull-up:
```cpp
// Library automatically configures INPUT_PULLUP for active LOW
Debounce16 button(PIN_BUTTON, LOW);
```

## Basic Usage

### Simple Debouncing (Polling)

```cpp
#include <Debounce16.h>

const uint8_t PIN_BUTTON = 17;
const uint8_t PIN_LED = 15;

Debounce16 button(PIN_BUTTON, HIGH);  // Active HIGH

void setup() {
    pinMode(PIN_LED, OUTPUT);
}

void loop() {
    static unsigned long lastUpdate = 0;

    // Update button state every 1ms
    if (millis() - lastUpdate >= 1) {
        lastUpdate = millis();
        button.update();
    }

    // Check for button press
    if (button.isPressed()) {
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }
}
```

### Using Hardware Timer Interrupt

```cpp
#include <Debounce16.h>

const uint8_t PIN_BUTTON = 17;
Debounce16 button(PIN_BUTTON, HIGH);

hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
    button.update();  // Update at precise 1ms intervals
}

void setup() {
    // Configure timer for 1ms interrupts
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000, true);
    timerAlarmEnable(timer);
}

void loop() {
    if (button.isPressed()) {
        // Handle button press
    }
}
```

### Using Hardware Timer Interrupt with latest Arduino

```cpp
#include <Debounce16.h>

const uint8_t PIN_BUTTON = 17;
Debounce16 button(PIN_BUTTON, HIGH);

hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
    button.update();  // Update at precise 1ms intervals
}

void setup() {
    // Configure timer for 1 MHz and callback every 1000 ticks
    timer = timerBegin(1000000); // 1 MHz
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(buttonTimer, 1000, true, 0);  // 1ms, autoreload, forever
}

void loop() {
    if (button.isPressed()) {
        // Handle button press
    }
}
```

### Using Hardware Timer Interrupt with ESP-IDF

```cpp
#include <Debounce16.h>

#include "esp_timer.h"

const uint8_t PIN_BUTTON = 17;
Debounce16 button(PIN_BUTTON, DB_HIGH);

hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer(void *) {
    button.update();  // Update at precise 1ms intervals
}

void setup() {
  const esp_timer_create_args_t timer_args = {
    .callback = &onButtonTimer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "Debounce16",
    .skip_unhandled_events = true
  };
  esp_timer_handle_t timer_handler;

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handler));
  ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, 1000));
}

void loop() {
    if (button.isPressed()) {
        // Handle button press
    }
}
```

## Advanced Features

### Double Press Detection

```cpp
#include <Debounce16.h>

Debounce16 button(17, HIGH);

void setup() {
    button.enableDoublePressDetection(true);
    button.setDoublePressWindow(300);  // 300ms window
}

void loop() {
    button.update();

    if (button.isDoublePressed()) {
        // Handle double press
    }

    // Or check click count
    uint8_t clicks = button.getClickCount();
    if (clicks == 1) {
        // Single press
    } else if (clicks == 2) {
        // Double press
    }
}
```

### Long Press Detection

```cpp
#include <Debounce16.h>

Debounce16 button(17, HIGH);

void setup() {
    button.enableLongPressDetection(true);
    button.setLongPressThreshold(1000);  // 1 second
}

void loop() {
    button.update();

    if (button.isLongPressed()) {
        // Button is being held down
    }
}
```

### Using Callbacks

```cpp
#include <Debounce16.h>

Debounce16 button(17, HIGH);

void onButtonPress() {
    Serial.println("Button pressed!");
}

void onButtonRelease() {
    Serial.println("Button released!");
}

void setup() {
    Serial.begin(115200);
    button.onPress(onButtonPress);
    button.onRelease(onButtonRelease);
}

void loop() {
    button.update();
}
```

## API Reference

### Constructor

```cpp
Debounce16(uint8_t|gpio_num_t pin, bool activeLevel = DB_HIGH)
```

- `pin`: GPIO pin number where button is connected
- `activeLevel`: Logic level when button is pressed (DB_HIGH or DB_LOW)

### Core Methods

```cpp
void update()                    // Update button state (call every 1ms)
bool isPressed()                 // Returns true on press event
bool isReleased()                // Returns true on release event
bool isDown()                    // Returns true if button is held down
bool isUp()                      // Returns true if button is released
```

### Configuration Methods

```cpp
void enableDoublePressDetection(bool enable = true)
void setDoublePressWindow(uint16_t windowMs)
void enableLongPressDetection(bool enable = true)
void setLongPressThreshold(uint16_t thresholdMs)
```

### Advanced Query Methods

```cpp
bool isDoublePressed()           // Returns true on double-press event
bool isLongPressed()             // Returns true when long-press active
uint8_t getClickCount()          // Returns current click count
```

### Callback Registration

```cpp
void onPress(void (*callback)())
void onRelease(void (*callback)())
void onDoublePress(void (*callback)())
void onLongPressStart(void (*callback)())
void onLongPressEnd(void (*callback)())
```

## How It Works

The library uses a 16-bit shift register to store the last 16 button state readings. When `update()` is called (every 1ms), the register shifts left and adds the current button state.

### Press Detection Pattern

A press is detected when the latest 6 bits are all HIGH (button pressed) after being LOW (with some possible bounce in between, the "don't cares" being indicated as x):
```
0b00000xxxxx111111
```

This means the button must be consistently pressed for at least 6ms before being recognized.

### Release Detection Pattern

A release is detected when the latest 6 bits are all LOW (button not pressed) and the rest are HIGH (with some possible bounce in between, the "don't cares" being indicated as x):
```
0b11111xxxxx000000
```

This provides excellent noise immunity while maintaining fast response times.

## Theory and Background

This library is based on:
- **Jack Ganssle's "A Guide to Debouncing"**
  - https://www.ganssle.com/debouncing.pdf
  - https://www.ganssle.com/debouncing-pt2.htm
- **Elliot Williams' "Ultimate Debouncer"**
  - https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/
  - https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/

## Performance

- **Memory**: ~44 bytes per button (with all features enabled)
- **CPU**: ~0.04% per button @ 240MHz (polling at 1ms intervals)
- **Response Time**: ~6ms from physical press to detection

## License

This library is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

Copyright (c) 2025 Brooks

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Credits

- Based on research by Jack Ganssle
- Inspired by Elliot Williams' "Ultimate Debouncer"
- Implemented for ESP32 Arduino framework
