// Row Together: Beat the Tide. Based on Tiffany Tseng's Module 3 button masher.
#include <WiFi.h>
#include <esp_now.h>
#include <esp_arduino_version.h>
#include <TFT_eSPI.h>
const byte PLAYER = 0, TEAM = 73;
const int TARGET = 19, BEATS = 12, BEAT_MS = 1400, SYNC_MS = 180;
enum Phase : byte { READY, COUNTDOWN, ROWING, WON, LOST };
struct Msg {
  uint32_t magic = 0x52425735, session = 0, seq = 0, elapsed = 0;
  uint16_t target = TARGET;
  byte team = TEAM, role = PLAYER, phase = READY, beat = 0, score = 0,
       result = 0, ready = 0, moves = 0, singles = 0, replay = 0;
  int16_t tap = -1;
};
TFT_eSPI tft;
TFT_eSprite f(&tft);
QueueHandle_t inbox;
Msg state;
const byte broadcast[6] = {255, 255, 255, 255, 255, 255};
uint32_t lastRx = 0, lastTx = 0, seq = 0, remoteSeq = 0, start = 0, rxClock = 0,
         lastDraw = 0, edge = 0;
bool seen = false, myReady = false, theirReady = false, raw = false,
     stable = false, armed = false;
int myTap = -1, theirTap = -1;
bool scored = false;
void resetHost() {
  state = Msg();
  state.session = esp_random();
  start = millis();
  myReady = theirReady = scored = false;
  myTap = theirTap = -1;
  remoteSeq = 0;
}
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void receive(const esp_now_recv_info_t *, const uint8_t *data, int len) {
#else
void receive(const uint8_t *, const uint8_t *data, int len) {
#endif
  if (len != sizeof(Msg))
    return;
  Msg p;
  memcpy(&p, data, len);
  if (p.magic == 0x52425735 && p.team == TEAM && p.role == 1 - PLAYER)
    xQueueOverwrite(inbox, &p);
}
void halt(const char *s) {
  tft.fillScreen(TFT_BLACK);
  tft.drawString(s, 4, 45, 2);
  for (;;)
    delay(500);
}
void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(35, INPUT);
  tft.init();
  tft.setRotation(1);
  f.setColorDepth(8);
  if (!f.createSprite(240, 135))
    halt("Display memory error");
  inbox = xQueueCreate(1, sizeof(Msg));
  WiFi.mode(WIFI_STA);
  if (!inbox || esp_now_init() != ESP_OK ||
      esp_now_register_recv_cb(receive) != ESP_OK)
    halt("Radio setup failed");
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast, 6);
  if (esp_now_add_peer(&peer) != ESP_OK)
    halt("Peer setup failed");
  if (!PLAYER)
    resetHost();
}
const uint16_t ink = 0x10E6, sky = 0xA71E, sea = 0x24D5, foam = 0xBFFF,
               sand = 0xF672, pine = 0x246C;
uint32_t swing[2] = {0, 0}, resultAt = 0;
bool swinging[2] = {false, false}, inResult = false, replayRequested = false;
uint8_t localMoves = 0, previousMoves = 0;
void sweep(int side, uint32_t now) {
  swing[side] = now;
  swinging[side] = true;
}
void oar(int side, uint32_t now) {
  int mirror = side ? 1 : -1;
  uint32_t age = now - swing[side];
  float pull = swinging[side] && age < 460 ? sinf(3.14159f * age / 460.f) : 0;
  float px = 120 + mirror * 62, py = 91, dx = mirror * (0.88f - 0.33f * pull),
        dy = -0.48f + 1.31f * pull;
  float len = sqrtf(dx * dx + dy * dy);
  dx /= len;
  dy /= len;
  auto point = [&](float d, float w, int &x, int &y) {
    x = px + d * dx - w * dy;
    y = py + d * dy + w * dx;
  };
  auto quad = [&](int a, int b, int wa, int wb, uint16_t c) {
    int x[4], y[4];
    point(a, -wa, x[0], y[0]);
    point(a, wa, x[1], y[1]);
    point(b, wb, x[2], y[2]);
    point(b, -wb, x[3], y[3]);
    f.fillTriangle(x[0], y[0], x[1], y[1], x[2], y[2], c);
    f.fillTriangle(x[0], y[0], x[2], y[2], x[3], y[3], c);
  };
  quad(-28, 32, 2, 2, 0xBC6C);
  quad(23, 34, 2, 6, ink);
  quad(34, 48, 6, 8, ink);
  quad(34, 46, 4, 6, side == PLAYER ? 0xFE27 : 0xC6B7);
  quad(40, 42, 5, 5, 0xFFFF);
}
void scene(uint32_t now) {
  int progress = min((int)state.score, TARGET), r = 8 + progress * 2;
  f.fillRect(0, 25, 240, 38, sky);
  f.fillCircle(42, 43, 9, sand);
  f.fillRect(0, 63, 240, 54, sea);
  f.fillTriangle(122 - r, 64, 122, 61 - r / 2, 122 + r, 64, pine);
  f.fillEllipse(122, 65, r + 3, 2 + r / 12, sand);
  int tower = 4 + progress / 3;
  f.fillRect(128, 63 - tower, 3 + progress / 6, tower, TFT_WHITE);
  f.fillTriangle(126, 63 - tower, 130 + progress / 12, 59 - tower,
                 134 + progress / 6, 63 - tower, 0xDAE8);
  for (int i = 0; i < 4; i++) {
    int y = 70 + i * 9;
    f.drawFastHLine(16 + i * 12, y, 10 + i * 3, foam);
    f.drawFastHLine(188 - i * 8, y - 2, 8 + i * 2, foam);
  }
  f.fillTriangle(68, 117, 120, 79, 172, 117, ink);
  f.fillTriangle(76, 117, 120, 83, 164, 117, 0xC4EE);
  f.drawLine(120, 86, 120, 116, sand);
  oar(0, now);
  oar(1, now);
}
void center(const String &s, int y, int font, uint16_t c) {
  f.setTextColor(c);
  f.drawCentreString(s, 120, y, font);
}
void confetti(uint32_t ms) {
  const uint16_t palette[] = {0xFE27, 0xF398, 0x4FFB, 0x8FED, TFT_WHITE};
  for (int i = 0; i < 48; i++) {
    int y = 25 + (i * 37 + ms / (12 + i % 9)) % 88;
    int x = (i * 67 + 8 + (int)(7 * sinf(ms * 0.003f + i))) % 240;
    if (x > 42 && x < 198 && y > 32 && y < 106)
      continue;
    f.fillRect(x, y, 2 + (i % 3), 2 + ((i + ms / 180) % 3), palette[i % 5]);
  }
}
void drawGame(bool linked, uint32_t elapsed) {
  uint32_t now = millis();
  f.fillSprite(ink);
  f.setTextColor(TFT_WHITE);
  f.drawString(PLAYER ? "RIGHT OAR >" : "< LEFT OAR", 5, 3, 2);
  if (state.phase == ROWING) {
    int remain = max(0, (BEATS - state.beat) * (BEAT_MS + 100) - (int)elapsed);
    char time[20];
    snprintf(time, sizeof time, "%d.%ds", remain / 1000, (remain % 1000) / 100);
    f.drawRightString(time, 236, 0, 4);
  } else
    f.drawRightString("BEAT THE TIDE", 235, 4, 2);
  scene(now);
  if (state.phase >= WON) {
    f.fillRoundRect(13, 32, 214, 77, 7, ink);
    if (state.phase == WON)
      confetti(now - resultAt);
    center(state.phase == WON ? "YOU WON!" : "TIME'S UP!", 36, 4,
           state.phase == WON ? 0x8FED : sand);
    center(state.phase == WON ? "Island reached together"
                              : "You lost this round",
           65, 2, TFT_WHITE);
    center(String(state.score) + " / " + String(TARGET) + " spaces", 88, 2,
           TFT_WHITE);
    if (now - resultAt >= 1200)
      center("Tap either board to race again", 118, 2, TFT_WHITE);
  } else if (state.phase == ROWING) {
    f.fillRoundRect(3, 29, 56, 18, 3, ink);
    f.setTextColor(TFT_WHITE);
    f.drawString(String(state.score) + "/" + String(TARGET), 8, 30, 2);
    center(state.result == 2 ? "IN SYNC  +2"
           : state.result == 1
               ? (state.singles == 3 ? "+1   Sync next!" : "PAIRED  +1")
           : state.singles == 3 ? (state.result == 3 ? "No point - sync to row"
                                                     : "Tap together for +2!")
           : myTap >= 0         ? "Stroke in"
                                : "ROW!   Tap once",
           118, 2, TFT_WHITE);
    int bar = 240 * (BEAT_MS - min((int)elapsed, BEAT_MS)) / BEAT_MS;
    f.fillRect(0, 114, bar, 3, sand);
  } else if (state.phase == COUNTDOWN) {
    f.fillRoundRect(88, 33, 64, 36, 5, ink);
    center(String(3 - min(2, (int)elapsed / 1000)), 37, 4, TFT_WHITE);
    center("Get ready to row", 118, 2, TFT_WHITE);
  } else
    center(!linked   ? "Waiting for other board"
           : myReady ? "Ready! Waiting for partner"
                     : "Tap either button to ready",
           118, 2, TFT_WHITE);
  f.pushSprite(0, 0);
}
void loop() {
  uint32_t now = millis();
  Msg p;
  if (xQueueReceive(inbox, &p, 0) == pdTRUE) {
    bool wasOffline = !seen || now - lastRx > 1500;
    seen = true;
    lastRx = now;
    if (PLAYER &&
        (p.session != state.session || wasOffline || p.seq > remoteSeq)) {
      if (p.session != state.session) {
        myReady = false;
        myTap = -1;
      }
      if (p.beat != state.beat || p.phase != state.phase)
        myTap = -1;
      if (p.phase == ROWING && p.session == state.session &&
          p.moves != previousMoves)
        sweep(0, now);
      previousMoves = p.moves;
      state = p;
      remoteSeq = p.seq;
      rxClock = now;
    } else if (!PLAYER && p.session == state.session) {
      if (state.phase >= WON && p.phase == state.phase && p.replay)
        replayRequested = true;
      if (state.phase == ROWING && p.phase == ROWING &&
          p.moves != previousMoves)
        sweep(1, now);
      previousMoves = p.moves;
      if (state.phase == READY && p.ready)
        theirReady = true;
      if (state.phase == ROWING && p.phase == ROWING && p.beat == state.beat &&
          p.tap >= 0 && p.tap < BEAT_MS && theirTap < 0)
        theirTap = p.tap;
    }
  }
  bool linked = seen && now - lastRx < 1500;
  bool down = !digitalRead(0) || !digitalRead(35), press = false;
  if (down != raw) {
    raw = down;
    edge = now;
  }
  if (now - edge >= 25) {
    if (!raw)
      armed = true;
    if (stable != raw) {
      stable = raw;
      press = stable && armed;
      if (stable)
        armed = false;
    }
  }
  uint32_t elapsed = PLAYER ? state.elapsed + (now - rxClock) : now - start;
  if (state.phase >= WON && !inResult) {
    inResult = true;
    resultAt = now;
    replayRequested = false;
  }
  if (state.phase < WON) {
    inResult = false;
    replayRequested = false;
  }
  if (press && linked) {
    if (inResult && now - resultAt >= 1200)
      replayRequested = true;
    if (state.phase == READY)
      myReady = true;
    else if (state.phase == ROWING) {
      sweep(PLAYER, now);
      localMoves++;
      if (myTap < 0 && elapsed < BEAT_MS)
        myTap = elapsed;
    }
  }
  if (!PLAYER) {
    if (state.phase >= WON && replayRequested && linked &&
        now - resultAt >= 1200) {
      resetHost();
      state.phase = COUNTDOWN;
      start = now;
    }
    if (!linked && (state.phase == COUNTDOWN || state.phase == ROWING))
      resetHost();
    if (state.phase == READY && linked && myReady && theirReady) {
      state.phase = COUNTDOWN;
      start = now;
    }
    if (state.phase == COUNTDOWN && now - start >= 3000) {
      state.phase = ROWING;
      start = now;
    }
    if (state.phase == ROWING) {
      if (!scored && myTap >= 0 && theirTap >= 0) {
        if (abs(myTap - theirTap) <= SYNC_MS) {
          state.score += 2;
          state.result = 2;
          state.singles = 0;
        } else if (state.singles < 3) {
          state.score++;
          state.singles++;
          state.result = 1;
        } else {
          state.result = 3; // Blocked pair: feedback only, no point.
        }
        scored = true;
        if (state.score >= TARGET)
          state.phase = WON;
      }
      // Brief radio grace: no new taps past the beat, but allow in-flight
      // packets.
      if (state.phase == ROWING && now - start >= BEAT_MS + 100) {
        if (state.beat + 1 >= BEATS)
          state.phase = LOST;
        else {
          state.beat++;
          start = now;
          myTap = theirTap = -1;
          scored = false;
          state.result = 0;
        }
      }
    }
    state.elapsed = now - start;
    elapsed = state.elapsed;
  }
  if (now - lastTx >= 30 || press) {
    Msg out = state;
    out.role = PLAYER;
    out.seq = ++seq;
    out.tap = myTap;
    out.ready = myReady;
    out.moves = localMoves;
    out.replay = replayRequested;
    esp_now_send(broadcast, (uint8_t *)&out, sizeof out);
    lastTx = now;
  }
  if (now - lastDraw >= 50) {
    drawGame(linked, elapsed);
    lastDraw = now;
  }
  delay(1);
}
