# Your Turn

I’ve danced ballet for 18 years. For this project, I wanted someone who’s never taken a ballet class to be able to make a dance of their own. I gave them two decisions: where should the dancer go, and what should she do when she gets there?

Move the joystick to place the spotlight, turn the knob to choose a plié, turn, or jump, then press the yellow button to try that step. After four choices, the ballerina performs your dance.

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

## What the controller taught me

### Why the LED lit up but the circle stayed still

My first joystick test was a Processing sketch that moved a circle around the screen. When I added the knob and yellow button, pressing the button still lit the LED, but moving the joystick no longer moved the circle. The LED was controlled by the ESP32 itself. It could light up even if the laptop ignored every reading.

The difference shows up in two lines of the [Arduino sketch](controller/controller.ino). One drives a physical pin. The other sends a message over USB:

```cpp
digitalWrite(ledPin, button == LOW ? HIGH : LOW);
Serial.printf("%d,%d,%d,%d,%d\n", x, y, joystickSwitch, knob, button);
```

My old circle sketch expected three numbers per line. Once I added the knob and separate button, it needed to accept five. In the [final Processing sketch](your_turn/your_turn.pde), this runs when the newline arrives:

```java
String[] values = split(trim(incoming), ',');
incoming = "";
if (values.length != 5) continue;
```

That changed how I tested the controller. Pressing the button could tell me the circuit worked. To find out why the circle was stuck, I had to compare what Arduino actually sent with what Processing would accept.

### Turning a noisy knob into three choices

During testing, I could leave the knob alone and still see the numbers change. There were two different problems to sort out. First, I’d wired the knob’s output to pin 13, so the Arduino sketch had to read **13**, not 27. Once turning it actually changed the reading, the question was how to keep a small fluctuation from changing the dancer’s step.

```java
if (selected < 2 && knob > (selected + 1) * 1365 + 90) selected++;
if (selected > 0 && knob < selected * 1365 - 90) selected--;
```

The first boundary is 1365. Starting on plié, the knob has to pass 1455 to select turn. Coming back from turn, it has to fall below 1275 to select plié again. So if the reading wobbles around 1365, the dancer keeps whichever move I last chose. What mattered to me was being able to let go of the knob, reach for the yellow button, and trust that I was still adding the same move.

### How a jump makes it into the dance

Say I put a jump on the left side of the stage, then turn the knob to try a plié on the right. I still want that first jump back when the dance plays. The yellow button is what separates trying out an idea from keeping it. When I press it, [the Processing sketch](your_turn/your_turn.pde) runs this function:

```java
void addStep() {
  if (phase != 0 || count >= 4) return;

  stepX[count] = markerX;
  stepY[count] = markerY;
  stepMove[count] = selected;

  count++;
  phase = 1;
  beginAnimation();
}
```

Those three assignments save the spot and the movement in the same numbered slot. The dancer previews the step right away; the first line keeps another press during that preview from taking up a second slot. After the fourth preview, playback goes back to slot zero and reads the four choices in order.

### Why the spotlight stayed behind

When I watched the four steps play back, the dancer moved away from the spot where I’d left the joystick. I wanted the light to travel with her, including the moment *between* two steps. This is the part of the animation loop that gives her a position on every frame:

```java
int i = phase == 1 ? count - 1 : playing;
float elapsed = millis() - phaseStart;
float travel = constrain(elapsed / 650, 0, 1);
float eased = travel * travel * (3 - 2 * travel);
dancerX = lerp(fromX, stepX[i], eased);
dancerY = lerp(fromY, stepY[i], eased);
```

The preview reads the newest slot (`count - 1`); playback reads the slot named by `playing`. That one choice lets the same animation run both times. For each step, `travel` goes from 0 to 1 over 650 milliseconds. The middle line softens the start and stop; the two `lerp` calls place her partway between where she started and where she’s going. I already had the coordinates the light needed. In `drawStage()`, I used the joystick’s marker while choosing and `dancerX` and `dancerY` during playback. Using the saved destination instead would have made the light jump ahead and wait for her.

Controller adapted from the course’s [Lab 2](https://comsbc3930-fall-2026.notion.site/Lab-2-Breadboarding-Sensing-Serial-Communication-da2350cc6f0583789b5d816f36e83bbe).
