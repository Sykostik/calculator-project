# ESP32-S3 Handheld Graphing & AI Calculator

A pocket-sized, battery-powered graphing and scientific calculator built on an **ESP32-S3-CAM** microcontroller. Written in C using **ESP-IDF**, it runs an offline math and symbolic differentiation engine, plots multiple functions at once, and connects to the **Gemini API** over Wi-Fi for step-by-step explanations, function analysis, and camera-based problem solving—all inside a custom 3D-printed case.

---

## Core Features

* **Offline Math & Symbolic Solver:** Handles arithmetic, powers, square/cube roots, logarithms (`log`, `ln`, and custom bases like `log2(8)`), factorials, modulus (`|x|`), and trig/inverse trig in **DEG** and **RAD** modes. Clean trig and radical results display in exact symbolic form (e.g., `PI/4`, `sqrt(3)/2`), and `d/dx(...)` computes symbolic derivatives for polynomials, trig, logs, exponentials, and modulus.
* **Multi-Function Graphing:** Plot up to **5 equations simultaneously** by separating them with a space (e.g., `x^2 2*x |x|`). Each curve renders in its own color (Green, Cyan, Orange, Magenta, Yellow) with dynamic axis ticks, panning, zooming, and numerical `d/dx(...)` plotting.
* **6-Layer Keypad & T9 Text Input:** Maps a full scientific calculator and A–Z alphabet onto a 16-button 4x4 matrix across 6 layers, including a **Letters** layer with 0.8s multi-tap typing. Input automatically scales down and wraps across up to 4 lines (128 characters max).
* **Gemini AI & Multi-Turn Chat:** Supports short answers with brief explanations (`Solv`), full step-by-step breakdowns (`Step`), and 7-point basic function analysis (`Anlz`: domain, range, parity, intercepts, sign intervals, monotonicity/extrema, and asymptotes). Retains the last 4 back-and-forth turns in PSRAM for follow-up questions (`NewC` resets the chat).
* **Camera Vision Solver:** Live OV3660 viewfinder on the TFT screen. Snap a photo of a handwritten or printed problem and the ESP32 encodes it to JPEG/Base64 in PSRAM and sends it to Gemini.
* **Clipboard & Character-Level AI Selection:** Copy text behind the cursor in calculator mode, or move a block cursor (`2/8/4/6`) inside AI response pages to highlight and copy exact formulas back into the calculator.

---

## Hardware & Power Design

| Component | Part / Spec | Role |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-S3-CAM (Octal PSRAM) | Main processor, Wi-Fi, on-board OV3660 camera |
| **Display** | 320x240 ILI9341 TFT LCD | 40 MHz SPI screen for UI, graphs, viewfinder, and AI pages |
| **Keypad** | 4x4 Matrix Keypad | 16-button input scanned across 6 software layers |
| **Battery** | 3.7V Single-Cell LiPo Pouch | Portable power source |
| **Charger** | TP4056 USB-C Module | LiPo charging via external USB-C cutout |
| **Boost Converter** | MT3608 DC-DC Step-Up | Boosts 3.7V LiPo to a regulated **5.0V–5.1V** |
| **Power Switch** | Toggle Switch | Isolates battery output from boost input |
| **Enclosure** | Custom 3D-Printed Shell | Two-piece PLA/PETG case modeled in Onshape (2.5 mm walls) |

### Power Path
```text
[3.7V LiPo Battery] --> [TP4056 USB-C Charger (B+/B-)]
                               | (OUT+ / OUT-)
                               v
                        [Toggle Switch] (inline on OUT+)
                               |
                               v
                      [MT3608 Boost (IN+/IN-)] --> Calibrated to 5.0V-5.1V
                               | (VOUT+ / VOUT-)
                               v
                 [ESP32-S3 5V & Common GND Rail]
