# TuffBoy

A handheld multi-tool inspired by the Flipper Zero, but built around the ESP32-C5 Devkit C with **Dual Band** (2.4GHz + 5Ghz), and a big idea: an expansion port that lets you bolt on a whole second microcontroller (Black Pill, or basically any 3.3V dev board).

This is the part that makes TuffBoy ~200× more flexible than a Flipper — instead of being stuck with only 2.4 GHz WiFi we have a breakthrough with the support of 5GHz, opening the door to WiFi security testing, along with support for plugging in whatever co-processor the task needs.

![Status](https://img.shields.io/badge/status-PCB%20completed-brightgreen) ![Made with](https://img.shields.io/badge/made%20with-KiCad%2010-blue) ![Hack Club](https://img.shields.io/badge/made%20with%20help%20from-Hack%20Club-EC5C28)

**TuffBoy so far:**

**3d model of the device**

![3D Model](images/full.png)

**3d render, of back**

![Back Render](images/back_back.png)

**pcb layout**

![PCB Layout](images/pcb.png)

**the 4-pin expansion connector**

![JST Power Connector](images/jst_power.png)

**Schematic**

![Schematic](images/sys.png)

## what is this

So the Flipper is cool but it's not that good in base form and expensive, and things like M5 Stacks can't really do proper wifi work (absence of 5GHz). TuffBoy is my take on what that device *should* be:

* **ESP32-C5** as the main brain — wifi 5GHz/2.4GHz + BLE, USB-C
* **nRF24L01+** for 2.4GHz radio stuff
* **CC1101** for sub-GHz (300–928 MHz)

![Back](images/Back.png)

* **TFT** — a 2.8" inch which is fully touch, no button, which makes it look like a small phone.

![Top](images/Top.png)

It's being made with the help of [Hack Club](https://hackclub.com/).

## the uart extension (the big idea)

Every flipper-clone has the same problem: you get one MCU and that's it. The TuffBoy has an **expansion port for a second microcontroller**:

* plug in a **STM32 Black Pill** (or any dev board that runs on 3.3V logic)
* the ESP32-C5 stays the brain (UI, wifi, displays) and the STM32 becomes a co-processor for whatever you want — real-time tasks, extra peripherals, other radio stacks, whatever you can think of
* the connection is a simple **4-pin jst connector: TX, RX, 3.3V and GND** (J1 on the board) — no special hardware, just serial
* since it's just TX/RX + power, basically any 3.3V board fits — black pill, another xiao, whatever you have lying around

![JST Connector](images/jst_power.png)

With this we can add basically anything we like, without redesigning the main board every time. New feature = new daughter board / new firmware, not a new device.

## wifi hack firmware (The core idea)

The main focus of the software is my **"wifi hack" firmware** — the whole reason I started this. Wifi security testing is weirdly hard to do on existing pocket tools; the Flipper can't do it properly at all and M5 stacks are clunky for it as both lack a 5GHz radio. The goal is a firmware that makes wifi security testing *easy*, straight from the device UI:

* scan / monitor mode
* deauth + handshake capture workflows
* works on **2.4GHz AND 5GHz** wifi — the esp32-c5 has 5GHz wifi built in, and **basically no pocket tool does 5GHz**. flipper can't, m5 can't. this is the rare one.

**obviously: use it only on networks you own or have permission to test. this is a learning tool, not a crime stick.**

## catching up to the flipper (then passing it)

The flipper has a bunch of stuff mine doesn't (yet). here's the honest status on each:

| feature                           | flipper | tuffboy                                            |
| --------------------------------- | ------- | -------------------------------------------------- |
| sub-ghz radio                     | ✅       | ✅ already on board (cc1101)                        |
| 2.4GHz radio                      | ✅       | ✅ already on board (nrf24l01+)                     |
| wifi                              | ❌       | ✅ 2.4 + **5GHz** — the thing flipper can't do      |
| stm32 / co-processor expansion    | ❌       | ✅ 4-pin connector already on board                 |
| IR blaster                        | ✅       | 🔜 needs an IR led + receiver on the next rev      |
| RFID 125kHz                       | ✅       | 🔜 needs extra hardware                            |
| NFC 13.56MHz                      | ✅       | 🔜 needs extra hardware                            |
| iButton                           | ✅       | 🔜 1-wire on a gpio, firmware work                 |
| USB HID / badUSB                  | ✅       | 🔜 the c5 has native usb, so this is firmware-only |
| U2F security key                  | ✅       | 🔜 same, firmware work                             |
| open expansion for any 3.3v board | ❌       | ✅                                                  |

So the plan: everything the flipper does gets added over a few board revs + firmware, and the flipper literally can't do wifi (or take a second mcu) — that's where tuffboy stays ahead.

## software status

The project currently has two firmware paths. You can either use the included TuffBoy starter/custom firmware or clone Bruce and continue from that firmware project.

### Use the TuffBoy firmware

The native starter firmware is located at [`firmware/TuffBoy/TuffBoy.ino`](firmware/TuffBoy/TuffBoy.ino). It is written for the ESP32-C5 Arduino framework and currently provides:

* boot diagnostics and chip information
* configurable LED status indicator
* expansion UART test messages
* I2C bus scanning
* Wi-Fi station mode and nearby-network scanning
* a serial command interface (`help`, `status`, `wifi`, `i2c`, `led`, and more)

The LED, I2C, and expansion UART pins are defined as constants at the top of the sketch because the final pin mapping may change between board revisions. The sketch uses the ESP32 Arduino framework APIs and the built-in `Wire` and `WiFi` libraries.

The project also contains custom firmware developed specifically for TuffBoy, which will be used as the project continues beyond the initial hardware testing stage.

### Clone Bruce firmware

The [`firmware/Bruce Firmware Cloner.py`](firmware/Bruce%20Firmware%20Cloner.py) script downloads the [Bruce firmware](https://github.com/pr3y/Bruce) into a `Bruce/` directory. It requires Python 3.7 or newer. Git is used by default, or the `--zip` option can download an archive without Git.

```bash
# Clone Bruce into the current directory
python3 "firmware/Bruce Firmware Cloner.py"

# Shallow clone into a chosen directory
python3 "firmware/Bruce Firmware Cloner.py" -d ~/projects --depth 1

# Download Bruce as a ZIP archive instead of using Git
python3 "firmware/Bruce Firmware Cloner.py" -d ~/projects --zip
```

Useful options are `--branch` for a specific branch and `--deps` to install the Python build dependencies used by Bruce. After cloning, follow Bruce's own board and build instructions in its repository.

## CAD and enclosure

The PCB uses 3D CAD models of the individual components mainly for **visualisation of the PCB and complete assembly**.

The individual component CAD models used in the PCB 3D model were downloaded from **GrabCAD**. These models are used to represent the physical components and their approximate placement in the complete assembly.

The **TuffBoy cover/enclosure was made according to the requirements of the project and the completed PCB**. The cover was made externally according to the required dimensions, component placement and mechanical requirements, and the STEP file was provided for use with the project.

## current progress

* [x] part selection + custom symbols/footprints (esp32_c5 lib: XIAO SMD footprint, cc1101, oled)
* [x] schematic — all blocks placed (mcu, 2 radios, 2 displays, 4 buttons, battery + headers)
* [x] board outline + placement
* [x] first hand-routed nets (radio area, battery area)
* [x] finish routing + ground pour
* [x] decoupling caps
* [x] DRC clean + renders
* [x] stm32 expansion connector on the board (J1 = TX, RX, 3V3, GND)
* [x] PCB design completed
* [x] custom TuffBoy firmware started
* [x] enclosure / cover STEP file

## files

```text
tuffboy/          the kicad 10 project (sch + pcb)
tuffboy.pretty/   third-party libs (xiaosymbols, ssd1306 footprints etc)
CAD/              step models (esp32-c5, nrf24l01, cc1101, oleds, 603040 battery)
images/           pcb renders + board pics
firmware/         TuffBoy custom firmware and Bruce cloning script
BOM.csv           bill of materials
Front Housing.step
Rear Housing.step
assembly(full)-reduced.stl
tuffboy(without case)-reduced.stl
```

Open `tuffboy/tuffboy.kicad_pro` in kicad 10 and you're good.

## credits

* completely made by ishant dahiya
* inspired by the [Flipper Zero](https://flipperzero.one/) (obviously)
* [Hack Club](https://hackclub.com/) for the support
* individual component CAD models sourced from **GrabCAD** for PCB visualisation
* TuffBoy cover/enclosure made according to the project requirements and provided as a STEP file

---

## Images

| Image                                     | Description                   |
| ----------------------------------------- | ----------------------------- |
| ![full.png](images/full.png)              | 3D model of the device        |
| ![2\_usbC.png](images/2_usbC.png)         | USB-C connector detail        |
| ![pcb.png](images/pcb.png)                | PCB layout                    |
| ![sys.png](images/sys.png)                | Schematic                     |
| ![Top.png](images/Top.png)                | Top view of the device        |
| ![SD.png](images/SD.png)                  | SD card slot                  |
| ![back\_cover.png](images/back_cover.png) | Back cover                    |
| ![back\_back.png](images/back_back.png)   | 3D render of back             |
| ![Top\_cover.png](images/Top_cover.png)   | Top cover                     |
| ![jst\_power.png](images/jst_power.png)   | 4-pin JST expansion connector |
| ![Back.png](images/Back.png)              | Back view of the device       |

---

*hardware: PCB completed · software: custom firmware in development · last updated Sep 2026*
