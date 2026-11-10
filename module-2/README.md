# Your Turn

After 18 years of ballet, I wanted to give someone who’s never danced a chance to choreograph. Move the joystick to place the spotlight, turn the knob to choose a plié, turn, or jump, then press the yellow button to add that step. After four choices, the ballerina performs your dance.

![A four-step dance playing on the Processing stage](media/dance-preview.gif)

[Watch the controller demo and read the project story](https://keira-columbia.github.io/creative-embedded-systems/module-2/#watch-demo)

## Build the controller

**Parts:** Original TTGO T-Display ESP32 with soldered headers, breadboard, jumper wires, analog joystick, three-terminal potentiometer, momentary pushbutton, LED, 220 Ω resistor, and a USB data cable.

<p>
  <a href="media/final-controller.jpg"><img src="media/final-controller.jpg" height="230" alt="Assembled breadboard controller"></a>
  <a href="media/your-turn-circuit.svg"><img src="media/your-turn-circuit.svg" height="230" alt="Fritzing breadboard diagram showing the controller wiring"></a>
</p>

Unplug USB before wiring. The photo shows my build; click the diagram to see the connections or [open it in Fritzing](circuit/your-turn.fzz).

| Part | Connect to the ESP32 |
| --- | --- |
| Joystick | VRx → 39, VRy → 32, SW → 33, GND → G. Connect the supply pin labeled +5V to **3V (3.3 V)**. |
| Potentiometer | Terminal 1 → G, terminal 2 (wiper) → 13, terminal 3 → 3V. Put each leg on a separate breadboard strip. |
| Yellow button | Place it across the breadboard gap. Connect two contacts that close when pressed to 2 and G. |
| LED | 15 → 220 Ω resistor → long leg; short leg → G. |

## Run the dance

1. Download the repository. In Arduino IDE, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` under **Preferences → Additional Boards Manager URLs**. In Boards Manager, install **esp32 by Espressif Systems 2.0.14**.
2. Connect the board by USB. Open [controller.ino](controller/controller.ino), select **ESP32 Dev Module** and the board’s port, then click **Upload**. Close Serial Monitor so Processing can use the port.
3. In Processing 4 (Java mode), open [your_turn.pde](your_turn/your_turn.pde) and click **Run**. The stage appears on your laptop. Once it says **CONTROLLER CONNECTED**, add a step and wait for its preview before adding the next. Press **Esc**, then **Run** for a new dance.

**If the stage says “Waiting for controller”:** The sketch looks for my Mac’s USB port. Add `printArray(Serial.list());` inside `setup()` and run once. In `readController()`, replace `name.startsWith("/dev/cu.wchusbserial")` with `name.equals("COM3")`, substituting the port name printed for your board. Run again.

**No controller?** Press **D** before the first step. Move the mouse to place the spotlight, press **1**, **2**, or **3** to choose a movement, and **Space** to add it.

Controller adapted from the course’s [Lab 2](https://comsbc3930-fall-2026.notion.site/Lab-2-Breadboarding-Sensing-Serial-Communication-da2350cc6f0583789b5d816f36e83bbe). [AI support note](https://keira-columbia.github.io/creative-embedded-systems/module-2/#ai-support).
