# ESP Proof of Concepts

## Purpose

This repository contains small, standalone proof-of-concept projects for the ESP32. The goal is to explore the board's hardware and features step by step. We start by controlling an external LED, then add a simple HTTP server to control it.

## Development Board

The board is a **NodeMCU ESP32S** with an **ESP-WROOM-32** module and the original ESP32 chip. It provides 2.4 GHz Wi-Fi and GPIO pins with 3.3 V logic levels.

The external LED will be connected to **GPIO23** in series with a resistor (for example, 220 Ω), then to GND.

### Pinout

The diagram is shown with the USB connector at the bottom. The labels in square brackets are the board pin labels; GPIO numbers are shown for GPIO pins.

```text
            Left-side functions                NodeMCU ESP32S                 Right-side functions
                                 ┌─────────────────────────────────────────┐
                    3.3 V power  │ [ 3V3 ]                         [ GND ] │  Ground
                 Reset / enable  │ [ EN  ]                         [ 23  ] │  SPI MOSI
 ADC1_CH0 (VP), RTC, input only  │ [ 36  ]                         [ 22  ] │  SPI SCK; common I2C SCL
 ADC1_CH3 (VN), RTC, input only  │ [ 39  ]                         [  1  ] │  UART0 TX
           ADC1_CH6, input only  │ [ 34  ]                         [  3  ] │  UART0 RX
           ADC1_CH7, input only  │ [ 35  ]                         [ 21  ] │  Common I2C SDA
         ADC1_CH4, Touch 9, RTC  │ [ 32  ]                         [ GND ] │  Ground
         ADC1_CH5, Touch 8, RTC  │ [ 33  ]                         [ 19  ] │  SPI MISO
                 ADC2_CH8, DAC1  │ [ 25  ]       ESP-WROOM-32      [ 18  ] │  SPI SCK
                 ADC2_CH9, DAC2  │ [ 26  ]                         [  5  ] │  SPI CS; strapping pin
         ADC2_CH7, Touch 7, RTC  │ [ 27  ]                         [ 17  ] │  UART2 TX
        ADC2_CH6, Touch 6, JTAG  │ [ 14  ]                         [ 16  ] │  UART2 RX
 ADC2_CH5, Touch 5, JTAG, strap  │ [ 12  ]                         [  4  ] │  ADC2_CH0, Touch 0, RTC
                         Ground  │ [ GND ]                         [  0  ] │  ADC2_CH1, Touch 1, BOOT/strap
        ADC2_CH4, Touch 4, JTAG  │ [ 13  ]                         [  2  ] │  ADC2_CH2, Touch 2, onboard LED, strap
                 SPI flash: SD2  │ [  9  ]                         [ 15  ] │  ADC2_CH3, Touch 3, JTAG, strap
                 SPI flash: SD3  │ [ 10  ]                         [  8  ] │  SPI flash: SD1
                 SPI flash: CMD  │ [ 11  ]                         [  7  ] │  SPI flash: SD0
                      5 V power  │ [ 5V  ]          USB            [  6  ] │  SPI flash: CLK
                                 └─────────────────────────────────────────┘
```
_Note_:
  - GPIO34–GPIO39 are input-only,
  - GPIO6–GPIO11 are used by the module's SPI flash and should not be used for other purposes,
  - Strapping pins can affect boot behavior; use them with care.


## Development Environment

- Windows
- Visual Studio Code with the Espressif ESP-IDF extension
- ESP-IDF 6.1
- CMake and Ninja, used by ESP-IDF

The project is built, flashed, and monitored using the ESP-IDF tools in VS Code.

## Create an ESP-IDF Project

1. Close any open folders.
2. In **ESP-IDF Explorer**, select **New Project Wizard** under **Advanced**.
3. Select the `sample_project` template from the list and complete the wizard form.
4. Open the project folder.
5. Connect the ESP32 board via USB.
6. In **ESP-IDF Explorer**, select **Select Port to Use** and choose the board's COM port.
7. Select **Build Project**. After a successful build, select **Flash Device** to upload the project, and then select **Monitor Device** to view the serial output. Alternatively, use **Build, Flash and Monitor** to run all steps together.
