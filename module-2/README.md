# Your Turn

A small ballet controller that lets a visitor create a four-step dance. Move the joystick to choose a spot on the stage, turn the knob to choose a plié, turn, or jump, then press the yellow button to add the step. Each choice is previewed before the complete dance plays.

The dancer and stage are drawn with shapes in Processing. There are no sprite files or image downloads. After playback, the finished dance stays on screen. Close the sketch window and press Run in Processing to begin again.

## Project code

- [controller/controller.ino](controller/controller.ino): reads the physical controls and sends their values over USB.
- [your_turn/your_turn.pde](your_turn/your_turn.pde): draws the stage and dancer, handles the controls, and stores and plays the four steps.

The controller code builds on the course's Lab 2 button and joystick examples. The earlier practice sketches are not included here.

## Run it

This version was built with Arduino IDE 2.3.10, ESP32 board package 2.0.14, and Processing 4.5.7. Use the original ESP32 TTGO T-Display, not the T-Display S3.

1. With USB unplugged, wire the controls using the table below. Use 3.3V for the joystick and potentiometer, and connect their grounds to the ESP32 ground.
2. Open `controller/controller.ino` in Arduino IDE. Select **ESP32 Dev Module** and your board's USB port, then upload.
3. Close the Arduino Serial Monitor and any other serial test programs.
4. Open `your_turn/your_turn.pde` in Processing and press **Run**. The sketch uses Processing's bundled Serial library and automatically looks for a Mac port beginning with `/dev/cu.wchusbserial`. Change that check in `readController()` if your computer uses another port name.
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

## Documentation

The [original proposal](proposal.html) records the initial plan. The final blog post, demonstration GIF/video, and Fritzing circuit diagram will be added for the documentation deliverable.
