# Row Together · Beat the Tide

Two people hold opposite oars of the same boat. We remixed a competitive button masher so that getting to the island depends on noticing when your partner presses. Each player holds one ESP32. Both see the same distance and race clock.

![Two people tapping the onboard buttons during a race](media/rowing.gif)

## Build it

1. Get two original LilyGO/TTGO T-Display ESP32 boards with the 240 × 135 screen and two USB data cables. The buttons and screens are built in.
2. In Arduino IDE, install **esp32 by Espressif Systems** in Boards Manager and **TFT_eSPI by Bodmer** in Library Manager. The project uses ESP32 core **2.0.14** and TFT_eSPI **2.5.43**. If the board package is missing, add the [Espressif package index](https://espressif.github.io/arduino-esp32/package_esp32_index.json) in Preferences → Additional Boards Manager URLs.
3. Open the installed library’s `TFT_eSPI/User_Setup_Select.h`. Enable `#include <User_Setups/Setup25_TTGO_T_Display.h>` as the only active display setup. Select **ESP32 Dev Module** in the IDE.
4. Plug in the first board, choose its USB port, and upload [LeftOar.ino](LeftOar/LeftOar.ino). Repeat with the second board and [RightOar.ino](RightOar/RightOar.ino). Power both boards. The screens should say **LEFT OAR** and **RIGHT OAR**. They find each other over ESP-NOW. If several crews play nearby, give each pair its own matching `TEAM` value in both sketches.

## Play one round

Tap either front button on **each board** to ready up. After the shared countdown, tap once each time the beat bar refills. A solo tap earns zero. Two taps in the same beat earn **+1**, or **+2** if they land within **180 ms**. After three +1s, the next point requires a synchronized pair. Reach **19 spaces in 12 beats (18 seconds)**. At the replay prompt, one press on either board starts another countdown.

[Watch the two-player demo and read Kira’s design notes](https://keira-columbia.github.io/creative-embedded-systems/module-3/) · [Professor Tseng’s starter sketch](https://github.com/ttseng/bccoms3930-module-3)
