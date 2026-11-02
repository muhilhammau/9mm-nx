/* nm9_input.c -- the Switch's inputs for 9mm HD.
 *
 * The game is played on a touch screen and its natives are three-int touches
 * (x, y, id) on GLGame: touch_pressed / touch_moved / touch_released. The
 * Joy-Cons drive it through the touchmap module (touchmap.h), the same way the
 * 9mm Vita port does:
 *   left stick   floating joystick (press, drag centre + stick * radius)
 *   right stick  camera drag (one finger that keeps moving)
 *   R, ZR        fire        L   bullet time     B   sprint (held)
 *   Y            reload      Plus   pause button
 *   D-pad L/R    weapon switch: the game wants a real horizontal swipe
 *   touch screen passed through
 * Positions are 1280x720 values (the Vita's 960x544 positions scaled), moved
 * to window pixels by touchmap; the engine is initialised with the same window
 * size (nm9_main.c), so both are one coordinate space. The mapping is always
 * the full one: the game has no zone detection for menus. MIT.
 */
#include <math.h>
#include <string.h>
#include <switch.h>

#include "rt_cfg.h"
#include "rt_applet.h"
#include "rt_pad.h"
#include "rt_window.h"
#include "nm9.h"
#include "touchmap.h"
#include "util.h"

static u64 g_prev;
static TmPad g_tm;
static TmBind g_binds[8];
static u32 g_seen[16];
static int g_nseen;
static int g_keylog;

/* weapon switch swipe, replayed over several frames (as the Vita port does) */
#define VSWIPE_KEY 0x80000032u
static int g_sw_active, g_sw_frame, g_sw_dir;

static void input_sampler(void *arg);

/* touchmap's actions: 0 down, 1 up, 2 move (window pixels, id). */
static void tm_touch(int action, int x, int y, int id) {
  if (!g_game_cls)
    return;
  if (action == 0 && g_nat.touch_pressed)
    g_nat.touch_pressed(g_jni_env, g_game_cls, x, y, id);
  else if (action == 1 && g_nat.touch_released)
    g_nat.touch_released(g_jni_env, g_game_cls, x, y, id);
  else if (action == 2 && g_nat.touch_moved)
    g_nat.touch_moved(g_jni_env, g_game_cls, x, y, id);
}

static void tm_key(int keycode, int down) {
  if (g_keylog < 100) {
    g_keylog++;
    debugPrintf("[input] key %d %s\n", keycode, down ? "down" : "up");
  }
  if (!g_game_cls)
    return;
  if (down && g_nat.key_down)
    g_nat.key_down(g_jni_env, g_game_cls, keycode);
  if (!down && g_nat.key_up)
    g_nat.key_up(g_jni_env, g_game_cls, keycode);
}

static void build_binds(void) {
  int touch = rt_config_value("controls.gamepad_touch", 1);
  /* HUD positions: the Vita port's 960x544 values scaled to 1280x720. */
  int fx = rt_config_value("controls.touch_btn_fire_x", 1053);
  int fy = rt_config_value("controls.touch_btn_fire_y", 556);
  int sx = rt_config_value("controls.touch_btn_slow_x", 1173);
  int sy = rt_config_value("controls.touch_btn_slow_y", 417);
  int rx = rt_config_value("controls.touch_btn_reload_x", 1160);
  int ry = rt_config_value("controls.touch_btn_reload_y", 64);
  int px = rt_config_value("controls.touch_btn_sprint_x", 576);
  int py = rt_config_value("controls.touch_btn_sprint_y", 672);
  int qx = rt_config_value("controls.touch_btn_pause_x", 40);
  int qy = rt_config_value("controls.touch_btn_pause_y", 36);

  int n = 0;
  /* Taps only: the game takes its input as touches, no keys are sent. */
  g_binds[n++] = (TmBind){HidNpadButton_R, fx, fy, -1};
  g_binds[n++] = (TmBind){HidNpadButton_ZR, fx, fy, -1};
  g_binds[n++] = (TmBind){HidNpadButton_L, sx, sy, -1};
  g_binds[n++] = (TmBind){HidNpadButton_B, px, py, -1};
  g_binds[n++] = (TmBind){HidNpadButton_Y, rx, ry, -1};
  g_binds[n++] = (TmBind){HidNpadButton_Plus, qx, qy, -1};

  g_tm.enable = touch;
  g_tm.touch = touch;
  g_tm.cam_touch = touch;
  g_tm.joy_x = rt_config_value("controls.touch_joy_x", 161);  /* Vita 121,425 */
  g_tm.joy_y = rt_config_value("controls.touch_joy_y", 562);
  g_tm.joy_r = rt_config_value("controls.touch_joy_r", 103);  /* Vita 77 */
  g_tm.cam_speed = rt_config_value("controls.touch_cam_speed", 19); /* Vita 14 */
  /* The Vita lets the camera finger run off screen (only the delta counts;
   * re-anchoring stutters): a ratchet radius that is never reached. */
  g_tm.cam_r = rt_config_value("controls.touch_cam_r", 1000);
  g_tm.binds = g_binds;
  g_tm.nbinds = n;
  tm_pad_config(&g_tm);
  debugPrintf("[input] touch_pressed=%p vtouch=%d binds=%d joy=(%d,%d,r%d)\n",
              (void *)g_nat.touch_pressed, touch, n, g_tm.joy_x, g_tm.joy_y, g_tm.joy_r);
}

void nm9_input_init(void) {
  hidInitializeTouchScreen();
  rt_pad_setup(1, 1);
  /* NOTE: no pad state here; the sampler thread owns the only PadState, so
   * slow frames never lose edges (see input_sampler). */
  tm_init(tm_touch, tm_key);
  build_binds();
  static Thread s_sampler;
  if (R_FAILED(threadCreate(&s_sampler, input_sampler, NULL, NULL, 0x4000, 0x30, -2)) ||
      R_FAILED(threadStart(&s_sampler)))
    debugPrintf("[input] no sampler thread: quick taps may be lost in slow frames\n");
}

void nm9_input_refresh(void) { build_binds(); }

/* A 120 Hz sampler: quick taps between two slow frames would otherwise never
 * exist (edges computed from one poll to the next miss press+release). The
 * snapshots carry pad + touchscreen state; poll() replays them in order, so
 * every transition reaches the engine. No JNI here, ever. */
#define SNAP_RING 128
typedef struct {
  u64 buttons;
  float sticks[4];
  u8 nt;
  u32 tid[10];
  s32 tx[10], ty[10];
} Snap;
static Snap g_ring[SNAP_RING];
static volatile uint32_t g_head, g_tail;
static PadState g_thread_pad;

static void input_sampler(void *arg) {
  (void)arg;
  padInitializeDefault(&g_thread_pad);
  u64 quiet_since = armGetSystemTick();
  for (;;) {
    if (rt_focused()) {
      padUpdate(&g_thread_pad);
      float sticks[4] = {0, 0, 0, 0};
      u64 b = rt_pad_read(&g_thread_pad, sticks);
      HidTouchScreenState st;
      memset(&st, 0, sizeof st);
      hidGetTouchScreenStates(&st, 1);
      int active = b || st.count;
      for (int i = 0; !active && i < 4; i++)
        if (sticks[i] > 0.05f || sticks[i] < -0.05f)
          active = 1;
      u64 now = armGetSystemTick();
      /* Idle 5 s: sample at ~33 Hz instead of 120 Hz (battery/heat), still
       * quick to wake on the first input. */
      if (active)
        quiet_since = now;
      else if (armTicksToNs(now - quiet_since) > 5000000000ull) {
        svcSleepThread(30000000ll);
        continue;
      }
      uint32_t h = __atomic_load_n(&g_head, __ATOMIC_RELAXED);
      uint32_t nn = (h + 1) & (SNAP_RING - 1);
      if (nn != __atomic_load_n(&g_tail, __ATOMIC_ACQUIRE)) {
        Snap *s = &g_ring[h];
        s->buttons = b;
        memcpy(s->sticks, sticks, sizeof sticks);
        s->nt = st.count > 10 ? 10 : (u8)st.count;
        for (int i = 0; i < s->nt; i++) {
          s->tid[i] = st.touches[i].finger_id;
          s->tx[i] = st.touches[i].x;
          s->ty[i] = st.touches[i].y;
        }
        __atomic_store_n(&g_head, nn, __ATOMIC_RELEASE);
      }
    }
    svcSleepThread(8000000ll);
  }
}

static void touches_from(const Snap *s, int w, int h) {
  u32 cur[16];
  int ncur = 0;
  for (int i = 0; i < s->nt && ncur < 16; i++) {
    int x = (int)((u64)(uint32_t)s->tx[i] * (u64)w / 1280);
    int y = (int)((u64)(uint32_t)s->ty[i] * (u64)h / 720);
    tm_move(s->tid[i], x, y);
    cur[ncur++] = s->tid[i];
  }
  for (int k = 0; k < g_nseen; k++) {
    int gone = 1;
    for (int i = 0; i < ncur; i++)
      if (cur[i] == g_seen[k])
        gone = 0;
    if (gone)
      tm_up(g_seen[k]);
  }
  memcpy(g_seen, cur, sizeof(u32) * (size_t)ncur);
  g_nseen = ncur;
}

/* One step of the weapon-switch swipe (the Vita's: y 48, 150 px long, 6 steps,
 * scaled to 1280x720: y 64, 200 px, starting at x 1067). */
static void weapon_swipe_tick(int w, int h) {
  if (!g_sw_active)
    return;
  const int Y = 64, LEN = 200, FRAMES = 6, X0 = 1067;
  int startx = (g_sw_dir > 0) ? X0 : X0 + LEN; /* opposite side of the way we go */
  int x = startx + g_sw_dir * (LEN * g_sw_frame / FRAMES);
  int px = x * w / 1280, py = Y * h / 720;
  if (g_sw_frame == 0)
    tm_down(VSWIPE_KEY, px, py);
  else if (g_sw_frame < FRAMES)
    tm_move(VSWIPE_KEY, px, py);
  else {
    tm_up(VSWIPE_KEY);
    g_sw_active = 0;
  }
  g_sw_frame++;
}

void nm9_input_poll(void) {
  int w = 1280, h = 720;
  dcr_window_size(&w, &h);
  const u64 dpad = HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right;
  /* Replay snapshots in order (no edge lost); collapse runs of identical
   * ones (idle menus) into their last: same edges, one processing. */
  for (;;) {
    uint32_t t = __atomic_load_n(&g_tail, __ATOMIC_RELAXED);
    if (t == __atomic_load_n(&g_head, __ATOMIC_ACQUIRE))
      break;
    uint32_t last = t;
    for (;;) {
      uint32_t nx = (last + 1) & (SNAP_RING - 1);
      if (nx == __atomic_load_n(&g_head, __ATOMIC_ACQUIRE))
        break;
      const Snap *a = &g_ring[last], *b = &g_ring[nx];
      if (a->buttons != b->buttons || a->nt != b->nt ||
          memcmp(a->sticks, b->sticks, sizeof a->sticks) != 0)
        break;
      int same = 1;
      for (int i = 0; i < a->nt; i++)
        if (a->tid[i] != b->tid[i] || a->tx[i] != b->tx[i] || a->ty[i] != b->ty[i]) {
          same = 0;
          break;
        }
      if (!same)
        break;
      last = nx;
    }
    const Snap *s = &g_ring[last];
    u64 now = s->buttons;
    u64 down = now & ~g_prev, up = g_prev & ~now;
    g_prev = now;
    touches_from(s, w, h);

    /* D-pad left/right: weapon switch (right cycles by swiping left). */
    if ((down & (HidNpadButton_Left | HidNpadButton_Right)) && !g_sw_active) {
      g_sw_active = 1;
      g_sw_frame = 0;
      g_sw_dir = (down & HidNpadButton_Right) ? -1 : +1;
    }
    /* The D-pad is not a second joystick here: hide it from touchmap. */
    tm_pad_poll(now & ~dpad, down & ~dpad, up & ~dpad, s->sticks, w, h);
    __atomic_store_n(&g_tail, (last + 1) & (SNAP_RING - 1), __ATOMIC_RELEASE);
  }
  weapon_swipe_tick(w, h); /* once per frame, not per snapshot */
}