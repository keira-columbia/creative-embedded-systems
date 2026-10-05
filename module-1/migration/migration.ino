// The things we leave behind — carrying home from Lebanon to New York.
// Each piece leaves the house, becomes a keepsake, and joins the new building.
// Original TTGO T-Display: TFT_eSPI Setup25, 240 x 135 landscape.
#include <TFT_eSPI.h>
#include <SPI.h>
#include <esp_system.h>
#include <bootloader_random.h>
#include <math.h>
#include "photos.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);

constexpr int WIDTH = 240, HEIGHT = 135;
constexpr int MAX_TRAVELERS = 24, TRAIL = 7;
constexpr uint32_t FRAME_MS = 33; // About 30 frames per second.
constexpr float MIGRATION_SPEED = 1.15f; // About 40 seconds from house to building.
constexpr float PI_F = 3.14159265359f;
constexpr float TAU = 2.0f * PI_F;
const uint16_t BACKGROUND = 0x0041; // Almost-black midnight blue.

enum Phase : uint8_t { AT_HOME, TRAVELING, SETTLED, RETURNING };
enum Scene : uint8_t { HOLD_HOME, MIGRATING, HOLD_CITY, RENEWING };
struct Particle {
  // A piece has a position in each building and its own travel timing.
  Phase phase; // Still home, on the way, arrived, or fading for the next cycle.
  int destinationTile;
  float age, wait, progress, duration; // Seconds, except progress (0 to 1).
  float homeX, homeY, destX, destY, x, y;
  float bend, wave, frequency, offset, radius, warmth;
  float eventAt, eventLeft, eventLength, eventSpeed;
  bool eventDone;
  float tailX[TRAIL], tailY[TRAIL], tailClock;
  int tailCount;
};
Particle particles[PARTICLES];
int destinationOwner[PARTICLES];
Scene scene = HOLD_HOME;
float sceneAge = 0, sceneHold = 5.0f;
uint32_t lastFrame = 0;
float breath = 0;
bool canvasReady = false;
uint16_t skyPixels[WIDTH * HEIGHT];
float skyDusk = -1.0f;
float sunrise = 0.0f;
float sunJourney = 0.0f;

float clamp01(float x) { return fmaxf(0.0f, fminf(1.0f, x)); }
float mix(float a, float b, float t) { return a + (b - a) * t; }
// Ease in and out, so pieces do not start and stop abruptly.
float smooth(float t) {
  t = clamp01(t);
  return t * t * (3.0f - 2.0f * t);
}
// Arduino random() returns integers; this gives us a value between two floats.
float chance(float low, float high) {
  return mix(low, high, random(1000000L) / 1000000.0f);
}

// Reassign destinations and generate fresh journeys after every reconstruction.
// The same piece keeps its keepsake symbol throughout the journey.
void prepareJourney(Particle &p, int index, bool first) {
  p.homeX = HOME[index].x;
  p.homeY = HOME[index].y;
  p.phase = AT_HOME;
  p.age = 3.0f;
  p.wait = chance(0.0f, 17.0f);
  p.progress = 0;
  p.duration = chance(5.0f, 8.5f);
  p.bend = chance(-26.0f, 22.0f);
  p.wave = chance(3.0f, 9.0f);
  p.frequency = chance(0.7f, 1.6f);
  p.offset = chance(0.0f, TAU);
  if (first) {
    p.radius = chance(0.95f, 1.25f);
    p.warmth = chance(0.0f, 1.0f);
  }
  p.eventAt = chance(0.25f, 0.70f);
  p.eventLength = chance(0.5f, 1.4f);
  p.eventSpeed = random(4) == 0 ? -0.4f : 0.08f;
  p.eventLeft = 0;
  p.eventDone = random(3) == 0;
  p.tailCount = 0;
  p.tailClock = 0;
  p.x = p.homeX;
  p.y = p.homeY;
}

void beginChapter(bool first) {
  int order[PARTICLES];
  for (int i = 0; i < PARTICLES; i++) order[i] = i;
  // Shuffle once so every new-home tile gets exactly one traveler.
  for (int i = PARTICLES - 1; i > 0; i--) {
    int j = random(i + 1);
    int temp = order[i];
    order[i] = order[j];
    order[j] = temp;
  }
  for (int i = 0; i < PARTICLES; i++) {
    prepareJourney(particles[i], i, first);
    particles[i].destX = CITY[order[i]].x;
    particles[i].destY = CITY[order[i]].y;
    particles[i].destinationTile = order[i];
    destinationOwner[order[i]] = i;
  }
  // A few independently chosen pieces of the old home resist leaving.
  for (int j = 0; j < 4; j++) particles[order[j]].wait = chance(25.0f, 33.0f);
  scene = HOLD_HOME;
  sceneAge = 0;
  sceneHold = first ? 5.0f : chance(4.0f, 7.0f);
  sunrise = 0.0f;
  sunJourney = 0.0f;
}

void updateParticles(float dt) {
  // Accelerate the journeys, but preserve the pauses to view both homes.
  if (scene == MIGRATING) dt *= MIGRATION_SPEED;
  sceneAge += dt;
  if (scene == HOLD_HOME) sunrise = smooth(sceneAge / sceneHold);
  breath = fmodf(breath + dt * 0.65f, TAU);
  if (scene == HOLD_HOME && sceneAge >= sceneHold) {
    scene = MIGRATING;
    sceneAge = 0;
  }
  if (scene == HOLD_CITY && sceneAge >= sceneHold) {
    scene = RENEWING;
    sceneAge = 0;
    for (Particle &p : particles) {
      p.phase = RETURNING;
      p.age = 0;
      p.wait = chance(0.0f, 9.0f);
    }
  }

  int travelers = 0;
  for (const Particle &p : particles) if (p.phase == TRAVELING) travelers++;
  for (int i = 0; i < PARTICLES; i++) {
    Particle &p = particles[i];
    p.age += dt;
    if (p.phase == AT_HOME) {
      p.x = p.homeX; p.y = p.homeY; // Exact anchors make the house readable.
      if (scene == MIGRATING && sceneAge >= p.wait && travelers < MAX_TRAVELERS) {
        p.phase = TRAVELING;
        p.age = 0;
        travelers++;
      }
    } else if (p.phase == TRAVELING) {
      if (!p.eventDone && p.progress >= p.eventAt) {
        p.eventLeft = p.eventLength;
        p.eventDone = true;
      }
      // Some travelers pause or briefly turn back before continuing.
      float pace = p.eventLeft > 0 ? p.eventSpeed : 1.0f;
      p.eventLeft = fmaxf(0.0f, p.eventLeft - dt);
      p.progress = clamp01(p.progress + dt * pace / p.duration);
      // The sideways bend fades at both ends, so pieces land on their tiles.
      float t = p.progress, ease = smooth(t), envelope = sinf(PI_F * t);
      p.x = mix(p.homeX, p.destX, ease);
      p.y = mix(p.homeY, p.destY, ease) + envelope *
        (p.bend + p.wave * sinf(TAU * p.frequency * t + p.offset));
      p.y = fmaxf(9.0f, fminf(HEIGHT - 10.0f, p.y));
      p.tailClock += dt;
      if (p.tailClock >= 0.07f) {
        p.tailClock = fmodf(p.tailClock, 0.07f);
        for (int j = TRAIL - 1; j > 0; j--) {
          p.tailX[j] = p.tailX[j - 1]; p.tailY[j] = p.tailY[j - 1];
        }
        p.tailX[0] = p.x; p.tailY[0] = p.y;
        if (p.tailCount < TRAIL) p.tailCount++;
      }
      if (p.progress >= 1.0f) {
        p.phase = SETTLED; p.age = 0; p.tailCount = 0;
        p.x = p.destX; p.y = p.destY;
        travelers--;
      }
    } else if (p.phase == RETURNING && p.age >= p.wait + 2.5f) {
      // Fade the city a piece at a time, then bring the old home back.
      p.phase = AT_HOME; p.age = 0; p.progress = 0;
      p.x = p.homeX; p.y = p.homeY;
    }
  }
  bool allArrived = true, allHome = true;
  for (const Particle &p : particles) {
    if (p.phase != SETTLED) allArrived = false;
    if (p.phase != AT_HOME || p.age < 3.0f) allHome = false;
  }
  if (scene == MIGRATING && allArrived) {
    scene = HOLD_CITY; sceneAge = 0; sceneHold = chance(8.0f, 11.0f);
  }
  if (scene == RENEWING && allHome) beginChapter(false);
}

float homePresence(const Particle &p) {
  return p.phase == AT_HOME ? smooth(p.age / 2.8f) : 0.0f;
}
float cityPresence(const Particle &p) {
  if (p.phase == SETTLED) return smooth(p.age / 0.8f);
  if (p.phase == RETURNING) return 1.0f - smooth((p.age - p.wait) / 2.5f);
  return 0.0f;
}

uint16_t blend(uint16_t base, int red, int green, int blue, float alpha) {
  alpha = clamp01(alpha);
  int r = ((base >> 11) & 31) * 255 / 31;
  int g = ((base >> 5) & 63) * 255 / 63;
  int b = (base & 31) * 255 / 31;
  return tft.color565(mix(r, red, alpha), mix(g, green, alpha), mix(b, blue, alpha));
}

// Subpixel light cores and a very restrained halo; no flashing pixels.
void light(float x, float y, float radius, int r, int g, int b, float alpha) {
  float reach = radius + 1.8f;
  int lowX = (int)floorf(x - reach), highX = (int)ceilf(x + reach);
  int lowY = (int)floorf(y - reach), highY = (int)ceilf(y + reach);
  for (int py = lowY; py <= highY; py++) {
    if (py < 0 || py >= HEIGHT) continue;
    for (int px = lowX; px <= highX; px++) {
      if (px < 0 || px >= WIDTH) continue;
      float dx = px + 0.5f - x, dy = py + 0.5f - y;
      float distance = sqrtf(dx * dx + dy * dy);
      float core = clamp01(radius + 0.3f - distance);
      float halo = clamp01(1.0f - distance / reach);
      float coverage = alpha * fminf(1.0f, core + 0.15f * halo * halo);
      if (coverage > 0.012f)
        canvas.drawPixel(px, py, blend(canvas.readPixel(px, py), r, g, b, coverage));
    }
  }
}

// Each fragment becomes a keepsake in transit, then joins the new photograph.
// Compact pixel sprites stay legible on the physical 240 x 135 display.
const char *KEEPSAKES[6][11] = {
  {"....g....","...ggg...","....g....","..ggggg..","...ggg...",".ggggggg.","..ggggg..","ggggggggg","....b....","....b....","...bbb..."}, // cedar
  {"..w.w....","...w.w...",".........",".wwwwww..",".wbbbbwww",".wbbbbw.w",".wbbbbwww","..wwww...",".........","wwwwwwwww","........."}, // coffee
  {"...www...","..wwwww..","..wsssw..","...sss...","..vvvvv..",".svvvvvs.",".svvvvvs.","..vvvvv..","..vvvvv..","...b.b...","..bb.bb.."}, // grandmother
  {"...www...","..wwwww..","..wsssw..","...sss...","..bbbbb..",".sbbwbbs.",".sbbwbbsb","..bbbbb.b","...b.b..b","...b.b..b","..bb.bb.b"}, // grandfather with cane
  {"...bbb...","...sss...","...sss...","....s....","..uuuuu..",".suuuuus.",".suuuuus.","...bbb...","...b.b...","...b.b...","..bb.bb.."}, // person
  {"...www...","..w...w..","..w...w..",".bbbbbbb.","bwbbbbwbb","bwbbbbwbb","bwbbbbwbb","bwbbbbwbb",".bbbbbbb.","..b...b..","........."} // suitcase
};
float keepsakePresence(const Particle &p) {
  return p.phase == TRAVELING ? smooth(p.progress / 0.16f) * smooth((1.0f - p.progress) / 0.16f) : 0;
}
void renderKeepsake(const Particle &p, int index) {
  float opacity = keepsakePresence(p);
  if (opacity < 0.01f) return;
  int left = (int)roundf(p.x) - 4, top = (int)roundf(p.y) - 5;
  for (int y = 0; y < 11; y++) for (int x = 0; x < 9; x++) {
    char c = KEEPSAKES[index % 6][y][x];
    int px = left + x, py = top + y;
    if (c == '.' || px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT) continue;
    int r = 245, g = 229, b = 190;
    if (c == 'g') { r = 45; g = 165; b = 95; }
    if (c == 'b') { r = 88; g = 48; b = 32; }
    if (c == 's') { r = 238; g = 174; b = 124; }
    if (c == 'v') { r = 190; g = 86; b = 163; }
    if (c == 'u') { r = 76; g = 188; b = 217; }
    canvas.drawPixel(px, py, blend(canvas.readPixel(px, py), r, g, b, opacity));
  }
}

// This is the image-drawing part: match pixels from the two building tiles.
void renderFragment(const Particle &p, int index) {
  const PhotoTile &source = HOME_TILES[index];
  const PhotoTile &target = CITY_TILES[p.destinationTile];
  // Blend the two image textures as a piece approaches its destination.
  float t = p.phase == AT_HOME ? 0.0f : smooth(p.progress);
  float scale = p.phase == TRAVELING ? 1.0f - 0.60f * sinf(PI_F * p.progress) : 1.0f;
  float w = mix(source.w, target.w, t) * scale;
  float h = mix(source.h, target.h, t) * scale;
  float left = p.x - w * 0.5f, top = p.y - h * 0.5f;
  float visibility = 1.0f - keepsakePresence(p);
  if (p.phase == AT_HOME) visibility = homePresence(p);
  if (p.phase == RETURNING) visibility = cityPresence(p);
  for (int y = (int)floorf(top); y < (int)ceilf(top + h); y++) {
    if (y < 0 || y >= HEIGHT) continue;
    float v = clamp01((y + 0.5f - top) / h);
    int sy = source.y + (int)fminf(source.h - 1, v * source.h);
    int ty = target.y + (int)fminf(target.h - 1, v * target.h);
    float edgeY = fminf(1.0f, fminf(y + 1.0f - top, top + h - y));
    for (int x = (int)floorf(left); x < (int)ceilf(left + w); x++) {
      if (x < 0 || x >= WIDTH) continue;
      float u = clamp01((x + 0.5f - left) / w);
      int sx = source.x + (int)fminf(source.w - 1, u * source.w);
      int tx = target.x + (int)fminf(target.w - 1, u * target.w);
      int si = sy * PHOTO_W + sx, ti = ty * PHOTO_W + tx;
      float sa = HOME_ALPHA[si] / 255.0f * (1.0f - t);
      float ta = CITY_ALPHA[ti] / 255.0f * t;
      float opacity = sa + ta;
      if (opacity < 0.01f) continue;
      uint16_t sc = HOME_RGB[si], tc = CITY_RGB[ti];
      int r = ((((sc >> 11) & 31) * sa + ((tc >> 11) & 31) * ta) / opacity) * 255.0f / 31.0f;
      int g = ((((sc >> 5) & 63) * sa + ((tc >> 5) & 63) * ta) / opacity) * 255.0f / 63.0f;
      int b = (((sc & 31) * sa + (tc & 31) * ta) / opacity) * 255.0f / 31.0f;
      float edgeX = fminf(1.0f, fminf(x + 1.0f - left, left + w - x));
      canvas.drawPixel(x, y, blend(canvas.readPixel(x, y), r, g, b,
                       opacity * visibility * edgeX * edgeY));
    }
  }
}

// A continuous dawn-to-dusk sky, cached so the full gradient is not rebuilt
// every frame. RGB565 ordered dithering keeps the small display's sky smooth.
void buildSky(float dusk) {
  const uint8_t bayer[4][4] = {{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
  for (int y = 0; y < HEIGHT; y++) {
    float horizon = smooth(y / 123.0f);
    float ground = smooth((y - 122.0f) / 12.0f);
    float warmR = mix(83, 224, horizon);
    float warmG = mix(39, 125, horizon);
    float warmB = mix(50, 59, horizon);
    float coolR = mix(mix(12, 6, dusk), mix(72, 36, dusk), horizon);
    float coolG = mix(mix(30, 16, dusk), mix(118, 65, dusk), horizon);
    float coolB = mix(mix(67, 44, dusk), mix(157, 112, dusk), horizon);
    for (int x = 0; x < WIDTH; x++) {
      float side = smooth((x - 69.0f) / 112.0f);
      float r = mix(warmR, coolR, side), g = mix(warmG, coolG, side);
      float b = mix(warmB, coolB, side);
      r = mix(r, mix(45, 8, side), ground);
      g = mix(g, mix(28, 17, side), ground);
      b = mix(b, mix(31, 36, side), ground);
      float dither = (bayer[y & 3][x & 3] - 7.5f) * 0.38f;
      skyPixels[y * WIDTH + x] = tft.color565(
        (int)fmaxf(0, fminf(255, r + dither)),
        (int)fmaxf(0, fminf(255, g + dither * 0.5f)),
        (int)fmaxf(0, fminf(255, b + dither)));
    }
  }
  skyDusk = dusk;
}

// A soft atmospheric disc, behind the buildings, without cartoon rays.
void renderSun(float cx, float cy, float radius, float setting, float visibility) {
  float reach = radius * 3.8f;
  for (int y = (int)floorf(cy - reach); y <= (int)ceilf(cy + reach); y++) {
    if (y < 0 || y >= 123) continue; // Disappears below the horizon.
    for (int x = (int)floorf(cx - reach); x <= (int)ceilf(cx + reach); x++) {
      if (x < 0 || x >= WIDTH) continue;
      float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
      float d = sqrtf(dx * dx + dy * dy);
      float halo = clamp01(1.0f - d / reach);
      float disc = clamp01(radius + 0.5f - d);
      float a = visibility * fminf(1.0f, 0.24f * halo * halo + disc * 0.94f);
      if (a > 0.006f)
        canvas.drawPixel(x, y, blend(canvas.readPixel(x, y), 255,
                         mix(222, 167, setting), mix(151, 112, setting), a));
    }
  }
}

void renderSky() {
  float journey = 0;
  for (const Particle &p : particles) {
    if (p.phase == TRAVELING) journey += p.progress;
    if (p.phase == SETTLED) journey += 1.0f;
    if (p.phase == RETURNING) journey += cityPresence(p);
  }
  journey = clamp01(journey / PARTICLES);
  if (skyDusk < 0 || fabsf(journey - skyDusk) > 0.018f) buildSky(journey);
  canvas.pushImage(0, 0, WIDTH, HEIGHT, skyPixels);
  // One continuous sun; brief backward steps by travelers cannot reverse it.
  if (scene == MIGRATING || scene == HOLD_CITY)
    sunJourney = fmaxf(sunJourney, journey);
  float x = 30, y = mix(131, 25, sunrise);
  if (scene != HOLD_HOME) {
    float t = smooth(sunJourney);
    x = mix(30, 227, t);
    y = 25 + 106 * t * t - 35 * sinf(PI_F * t);
  }
  // During renewal the sun remains below the NYC horizon until the next dawn.
  renderSun(x, y, 7, smooth(sunJourney), 1.0f);
}

void renderArtwork() {
  renderSky();
  // Stationary fragments first, then travelers, so departure is visible.
  for (int layer = 0; layer < 2; layer++) {
    for (int i = 0; i < PARTICLES; i++) {
      const Particle &p = particles[i];
      bool moving = p.phase == TRAVELING;
      if (moving != (layer == 1)) continue;
      if (moving) {
        for (int j = p.tailCount - 1; j >= 1; j--)
          light(p.tailX[j], p.tailY[j], 0.7f, 222, 206, 179,
                0.15f * (1.0f - (float)j / TRAIL));
      }
      renderFragment(p, i);
      if (moving) renderKeepsake(p, i);
    }
  }
  canvas.pushSprite(0, 0);
}

void setup() {
  Serial.begin(115200);
  // Sample physical entropy once, before display/peripherals. Radios stay off.
  bootloader_random_enable();
  uint32_t seed = esp_random();
  bootloader_random_disable();
  randomSeed(seed == 0 ? 1 : seed);

  // Same initialization as the working lab sketch. Pins stay in Setup25.
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND);
  // Draw a complete frame off-screen first to avoid flicker.
  canvas.setColorDepth(16);
  canvasReady = canvas.createSprite(WIDTH, HEIGHT) != nullptr;
  if (!canvasReady) {
    Serial.println("Frame allocation failed; restart the board.");
    tft.setTextColor(TFT_WHITE, BACKGROUND);
    tft.drawString("Please restart", 28, 56, 2);
    return;
  }
  canvas.setSwapBytes(true); // RGB565 sky is converted to sprite byte order.
  beginChapter(true);
  renderArtwork();
  lastFrame = millis();
  Serial.printf("The things we leave behind ready: seed=%lu, free heap=%u\n", (unsigned long)seed, ESP.getFreeHeap());
}

void loop() {
  if (!canvasReady) { delay(1000); return; }
  uint32_t now = millis();
  uint32_t elapsed = now - lastFrame; // Safe across millis() rollover.
  if (elapsed < FRAME_MS) { delay(1); return; }
  lastFrame = now;
  float dt = fminf(elapsed * 0.001f, 0.1f);
  updateParticles(dt);
  renderArtwork();
}
