# Your Turn

A small ballet controller that lets a visitor create a four-step dance. Move the joystick to choose a spot on the stage, turn the knob to choose a plié, turn, or jump, then press the yellow button to add the step. Each choice is previewed before the complete dance plays.

The dancer and stage are drawn with shapes in Processing. There are no sprite files or image downloads. After playback, the finished dance stays on screen. Close the sketch window and press Run in Processing to begin again.

## Materials

- Original ESP32 TTGO T-Display with soldered headers
- Breadboard and jumper wires
- Two-axis analog joystick module with click switch
- Three-terminal potentiometer
- Momentary pushbutton
- LED and 220-ohm series resistor
- USB data cable and a laptop with Arduino IDE and Processing

## Visual documentation — still to add

The demonstration GIF and a Fritzing breadboard diagram of the finished circuit are pending. The wiring table below records the final code connections; it does not replace the required Fritzing diagram.

## Project code

- [controller/controller.ino](controller/controller.ino): reads the physical controls and sends their values over USB.
- [your_turn/your_turn.pde](your_turn/your_turn.pde): draws the stage and dancer, handles the controls, and stores and plays the four steps.

The controller code builds on the course's Lab 2 button and joystick examples. The earlier practice sketches are not included here.

## Run it

This version was built with Arduino IDE 2.3.10, ESP32 board package 2.0.14, and Processing 4.5.7. Use the original ESP32 TTGO T-Display, not the T-Display S3.

1. With USB unplugged, wire the controls using the table below. Use 3.3V for the joystick and potentiometer, and connect their grounds to the ESP32 ground.
2. In Arduino IDE preferences, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` to Additional Boards Manager URLs. In Boards Manager, install **esp32 by Espressif Systems**, version **2.0.14**. Open `controller/controller.ino` in Arduino IDE. Select **ESP32 Dev Module** and your board's USB port, then upload.
3. Close the Arduino Serial Monitor and any other serial test programs.
4. Open `your_turn/your_turn.pde` in Processing and press **Run**. The sketch uses Processing's bundled Serial library and automatically looks for a Mac port beginning with `/dev/cu.wchusbserial`. For another computer, temporarily add `printArray(Serial.list());` to `setup()` and run once to find the board port in the console. Replace the `name.startsWith("/dev/cu.wchusbserial")` condition in `readController()` with `name.equals("YOUR_PORT")`, using that exact port (for example, `COM3` on Windows or `/dev/ttyUSB0` on Linux). Stop and run again. No extra graphics assets or libraries are needed.
5. Wait for **CONTROLLER CONNECTED**, then make four choices. Wait for each preview to finish before adding the next step.

| Connection | ESP32 pin |
| --- | --- |
| Joystick VRx | 39 |
| Joystick VRy | 32 |
| Joystick SW | 33 |
| Joystick power (marked +5V on the module) | 3V |
| Joystick GND | G |
| Potentiometer output (terminal 2) | 13 |
| Potentiometer terminals 1 and 3 | G and 3V |
| Separate button | 2 and G |
| LED | 15 through a 220-ohm resistor; cathode to G |

The separate button uses the ESP32's internal pull-up resistor. The LED lights while the button is held. The joystick click is transmitted but is not used to add dance steps. USB provides power; the ESP32 display is not used for this project.

Serial runs at 9600 baud. Each line contains `x,y,joystickClick,knob,button`; button values are 0 when pressed and 1 when released. Keep Wi-Fi off: the potentiometer uses an ADC2 pin.

For a laptop-only preview, press **D** before adding any steps. Move the mouse over the stage, select a movement with **1**, **2**, or **3**, and press **Space** to add it. Restart the sketch to clear the dance.

## Build notes

Unplug USB before changing wires. Each potentiometer terminal must have its own electrical connection: terminal 2 is the moving contact (output); terminals 1 and 3 go to ground and 3.3V. Do not put all three in one connected breadboard row. Connect the button across the breadboard gap, using contacts that become connected when pressed. If the power rails are split, bridge the sections you use. All controls share ESP32 ground.

The LED's long leg (anode) connects through the resistor to pin 15; its short leg (cathode) goes to ground. Do not power the joystick from 5V even if its power pin is labeled +5V: its analog outputs must stay within the ESP32's 3.3V range.

## Using and restarting the piece

The sketch opens full-screen and keeps the artwork in proportion. Press **Esc** to close it. Press Run in the editor to start an empty four-step dance. Choices are kept only in memory, not saved to disk. Reopen the `.pde` file after changing it externally, so Processing loads the latest version.

## Troubleshooting

- **WAITING FOR CONTROLLER:** check the USB data cable and selected port. Close Serial Monitor and other sketches that might be using the port. A power light alone does not confirm a data connection.
- **Knob does not change movements:** check that its output is connected to **13**, matching `potentiometerPin`, and the other terminals have 3.3V and ground. The earlier pin-27 test is not the final wiring.
- **Joystick drifts or stays at an edge:** check power, common ground, X on 39, and Y on 32. The code assumes a center around 2048 and ignores a small central range.
- **Button does not add a step:** wait for the preview to finish. Only four steps can be added; close and rerun after playback. Use the separate button, not the joystick click.
- **Blank ESP32 screen:** expected; this program draws on the laptop.

## Documentation and credits

[Read the project blog](https://keira-columbia.github.io/creative-embedded-systems/module-2/) for the design process and technical explanation. The [proposal](proposal.html) records the original plan.

The controller builds on the course Lab 2 examples. The dancer is drawn in code rather than imported from a sprite pack.
