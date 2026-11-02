/* nm9_main.c -- 9mm HD: from the APK to the frame loop.
 *
 * port_load: the setup unpacks libStormGLOFT.so and libCopStory.so from the
 * APK (first start, and whenever the APK changes); both are loaded, relocated,
 * bound and sealed as code, StormGLOFT first (it exports the unwinder and the
 * __aeabi helpers libCopStory imports). port_run: constructors, JNI_OnLoad,
 * what the Java activity did (GLGame.nativeInit, a GLES 2 context,
 * GameRenderer.nativeInit/nativeResize, GameGLSurfaceView resume/focus) and
 * every frame GameRenderer.nativeRender. The engine swaps by itself through
 * GameRenderer.swapEGLBuffers() (nm9_java.c: h_swap), so the loop does not.
 * The DRM / installer / billing natives are never called. MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "dcr_path.h"
#include "dcr_setup.h"
#include "bionic_io.h"
#include "dcr_boost.h"
#include "error.h"
#include "gl_layer.h"
#include "rt_applet.h"
#include "rt_boot.h"
#include "rt_cfg.h"
#include "rt_settings.h"
#include "rt_window.h"
#include "so_util.h"
#include "nm9.h"
#include "util.h"
#include "watchdog.h"

static so_module g_storm; /* libStormGLOFT.so: loaded first */
static so_module g_mod;   /* libCopStory.so: the game */
static void *g_view_cls;
static volatile int g_running;

/* The first start: both libraries (bar 50 to 800 by bytes), the class list
 * (800-950). */
static const char *const k_libs[] = {"libStormGLOFT.so", "libCopStory.so"};
const RtSetupPlan port_setup_plan = {
    .libs = k_libs,
    .nlibs = 2,
    .libs_what = "Unpacking the game's libraries",
    .libs_p0 = 50,
    .libs_p1 = 800,
    .classes_p0 = 800,
    .classes_p1 = 950,
};

uint64_t dcr_boot_frames(void) { return dcr_gl_frames(); }

static void load_one(so_module *m, const char *name) {
  char lib[DCR_PATH_MAX];
  rt_root_path(lib, sizeof lib, name);
  int rc = so_load(m, lib, NULL, PORT_SO_REGION_BYTES);
  if (rc)
    fatal_error("%s could not be loaded (%d).\n\n%s", name, rc, port_apk_help());
  if (so_relocate(m))
    fatal_error("%s: relocation failed.", name);
  /* Imports resolve against the runtime's shims and the modules already
   * loaded (libStormGLOFT for libCopStory). */
  int left = so_resolve(m, NULL, 0, 0);
  if (left)
    debugPrintf("[boot] WARNING: %d import(s) of %s unresolved\n", left, name);
  so_fix_kuser_helpers(m);
  so_finalize(m);
  so_flush_caches(m);
  debugPrintf("[boot] %s loaded\n", name);
}

int port_load(const char *apk) {
  /* No OBB: 9mm's data (.gla) lives in gameloft/games/Gloft9MHM, which the
   * game creates and reads itself. */
  dcr_setup_from_apk(apk);
  load_one(&g_storm, "libStormGLOFT.so");
  load_one(&g_mod, "libCopStory.so");
  return 0;
}

#define JNI_SYM(cls, name) "Java_com_gameloft_android_ANMP_Gloft9MHM_" cls "_" name

#define NAT(field, cls, name, required)                                             \
  do {                                                                              \
    g_nat.field = (void *)so_try_find_addr_rx(&g_mod, JNI_SYM(cls, name));          \
    if ((required) && !g_nat.field)                                                 \
      fatal_error("libCopStory.so has no " cls "." name);                           \
    debugPrintf("[boot] native " cls "." name ": %s\n", g_nat.field ? "found" : "MISSING"); \
  } while (0)

static void find_natives(void) {
  NAT(game_init, "GLGame", "nativeInit", 1);
  NAT(rend_init, "GameRenderer", "nativeInit", 1);
  NAT(rend_resize, "GameRenderer", "nativeResize", 1);
  NAT(rend_render, "GameRenderer", "nativeRender", 1);
  NAT(rend_destroy, "GameRenderer", "nativeDestroy", 0);
  NAT(touch_pressed, "GLGame", "nativeTouchPressed", 0);
  NAT(touch_moved, "GLGame", "nativeTouchMoved", 0);
  NAT(touch_released, "GLGame", "nativeTouchReleased", 0);
  NAT(touch_canceled, "GLGame", "nativeTouchCanceled", 0);
  NAT(key_down, "GLGame", "nativeSetOnKeyDown", 0);
  NAT(key_up, "GLGame", "nativeSetOnKeyUp", 0);
  NAT(accelerometer, "GLGame", "nativeAccelerometer", 0);
  NAT(orientation, "GLGame", "nativeOrientation", 0);
  NAT(view_pause, "GameGLSurfaceView", "nativePause", 0);
  NAT(view_resume, "GameGLSurfaceView", "nativeResume", 0);
  NAT(view_set_focus, "GameGLSurfaceView", "nativeSetFocus", 0);
  /* kept for later: arguments not read yet, never called */
  g_nat.device_init = (void *)so_try_find_addr_rx(&g_mod, JNI_SYM("GLUtils_Device", "nativeInit"));
  g_nat.resloader_init = (void *)so_try_find_addr_rx(&g_mod, JNI_SYM("GLResLoader", "nativeInit"));
  g_nat.mediaplayer_init = (void *)so_try_find_addr_rx(&g_mod, JNI_SYM("GLMediaPlayer", "nativeInit"));
  if (!g_nat.touch_pressed)
    debugPrintf("[boot] WARNING: GLGame.nativeTouchPressed not found: touch will do nothing\n");
}

void port_focus_lost(void) {
  if (!g_running)
    return;
  nm9_audio_pause(1);
  if (g_nat.view_set_focus)
    g_nat.view_set_focus(g_jni_env, g_view_cls, 0);
  if (g_nat.view_pause)
    g_nat.view_pause(g_jni_env, g_view_cls);
}

void port_focus_gained(void) {
  if (!g_running)
    return;
  if (g_nat.view_resume)
    g_nat.view_resume(g_jni_env, g_view_cls);
  if (g_nat.view_set_focus)
    g_nat.view_set_focus(g_jni_env, g_view_cls, 1);
  nm9_audio_pause(0);
}

/* HOME and sleep freeze the whole process; the runtime's clocks find each
 * freeze. What Android does around it: the focus lost, then back. */
void port_process_frozen(unsigned count) {
  debugPrintf("[game] the process was held (HOME menu or sleep; freeze %u)\n", count);
  port_focus_lost();
  port_focus_gained();
}

void nm9_present_fallback(uint32_t before);
void port_run(void) {
  /* System.loadLibrary("StormGLOFT"), then ("CopStory"): constructors in order. */
  so_execute_init_array(&g_storm);
  so_execute_init_array(&g_mod);
  jni_init();
  nm9_java_patch_monitors(); /* synchronized blocks lock for real from here on */
  void *env = g_jni_env;
  g_game_cls = jni_class(NM9_CLS_GAME)->obj;
  g_rend_cls = jni_class(NM9_CLS_REND)->obj;
  g_view_cls = jni_class(NM9_CLS_VIEW)->obj;
  find_natives();

  /* libStormGLOFT has a JNI_OnLoad of its own: not called until it is known
   * to be needed (libCopStory does not list it as NEEDED). */
  jint (*on_load)(void *, void *) = (jint(*)(void *, void *))so_try_find_addr_rx(&g_mod, "JNI_OnLoad");
  if (on_load)
    debugPrintf("[boot] JNI_OnLoad -> 0x%x\n", (unsigned)on_load(g_jni_vm, NULL));

  nm9_input_init();

  /* GLGame.onCreate: GLGame_nativeInit caches the Java method IDs (it reads
   * the class's static methods: nm9_java.c answers them). */
  g_nat.game_init(env, g_game_cls);

  /* GLSurfaceView.Renderer.onSurfaceCreated / onSurfaceChanged: the context
   * first (appInit creates GL objects), then init and resize. */
  if (!nm9_egl_ensure())
    fatal_error("No GLES 2 context could be made.\nSee debug.log.");
  int w = 1280, h = 720;
  dcr_window_size(&w, &h);
  g_nat.rend_init(env, g_rend_cls, w, h, 0); /* a, b: the screen; c: not settled */
  g_nat.rend_resize(env, g_rend_cls, w, h);

  g_running = 1;
  port_focus_gained();
  dcr_watchdog_start();
  nm9_gpu_boost_apply();
  debugPrintf("[boot] the game is running (%dx%d)\n", w, h);
  int show_fps = rt_config_value("display.show_fps", 1);
  u64 fps_t0 = armGetSystemTick();
  unsigned fps_n = 0;
  u64 flush_t0 = fps_t0;
  int log_quiet = 0;
  int first_frame = 1;

  while (!rt_exit_requested() && appletMainLoop()) {
    rt_applet_poll();
    if (!rt_focused()) {
      dcr_boost_idle(); /* HOME/sleep: no frame runs, none of it is a load */
      svcSleepThread(50000000ll);
      continue;
    }
    nm9_input_poll();
    uint32_t fr0 = dcr_gl_frames();
    g_nat.rend_render(env, g_rend_cls); /* the swap happens inside, through Java */
    nm9_present_fallback(fr0);
    dcr_boost_poll();
    if (first_frame) {
      first_frame = 0;
      dcr_boost_launch_end();
    }
    if (!log_quiet && dcr_gl_frames() > 180) {
      log_quiet = 1;
      dcr_io_report_readahead();
      dcr_io_report_patterns();
      log_set_quiet(1);
    }
    if (log_quiet && armTicksToNs(armGetSystemTick() - flush_t0) >= 10000000000ull) {
      flush_t0 = armGetSystemTick();
      nm9_gpu_boost_tick();
      dcr_boost_report();
      log_flush_ring();
    }
    if (show_fps) {
      fps_n++;
      u64 ns = armTicksToNs(armGetSystemTick() - fps_t0);
      if (ns >= 2000000000ull) {
        debugPrintf("[fps] %.1f (%llu ms/frame)\n", (double)fps_n * 1e9 / (double)ns,
                    (unsigned long long)(ns / 1000000ull / (fps_n ? fps_n : 1)));
        fps_t0 = armGetSystemTick();
        fps_n = 0;
      }
    }
  }
  rt_applet_stop();
  nm9_gpu_boost_remove();
  if (log_quiet)
    log_set_quiet(0);
  log_flush_ring();
  g_running = 0; /* no nativeDestroy: the process ends */
}
