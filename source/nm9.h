/* nm9.h -- what the 9mm port's files share. MIT.
 *
 * 9mm HD (Gameloft, com.gameloft.android.ANMP.Gloft9MHM): libCopStory.so is the
 * game, libStormGLOFT.so the engine runtime it imports _Unwind_*, __aeabi_*div*
 * and friends from (load StormGLOFT first). The natives are static JNI
 * functions (env, class, ...) on GLGame, GameRenderer, GameGLSurfaceView. */
#ifndef NM9_H
#define NM9_H

#include <stddef.h>
#include <stdint.h>

#include "jni.h"

/* The Java classes the engine calls into / looks static methods up on. */
#define NM9_BASE      "com/gameloft/android/ANMP/Gloft9MHM"
#define NM9_CLS_GAME  NM9_BASE "/GLGame"          /* the activity */
#define NM9_CLS_REND  NM9_BASE "/GameRenderer"    /* the GLSurfaceView renderer */
#define NM9_CLS_VIEW  NM9_BASE "/GameGLSurfaceView"

/* ---------------------------------------------------------------- nm9_gl.c */
/* The EGL display, config, window surface and a GLES 2 context: what Java's
 * GLSurfaceView made. Idempotent; 1 when the context is current here. Call it
 * before GameRenderer_nativeInit (appInit creates GL objects). */
int nm9_egl_ensure(void);
void nm9_egl_view_settings(int pixel, int depth, int stencil, int csaa);
int nm9_egl_set_current(int id);
/* eglSwapBuffers: the engine asks for it through GameRenderer.swapEGLBuffers(). */
void nm9_egl_swap(void);
void nm9_crosshair_draw(void);
/* Menu vs gameplay as the wrapper sees it (crosshair only; 9mm: always menu). */
void nm9_zone_note(const char *path);
int nm9_zone_is_menu(void);

/* ---------------------------------------------------------------- nm9_audio.c */
/* android.media.AudioTrack over audout; the handlers are in nm9_java.c's table. */
JNI_H_DECL(nm9_at_init);
JNI_H_DECL(nm9_at_min_buffer);
JNI_H_DECL(nm9_at_native_rate);
JNI_H_DECL(nm9_at_play);
JNI_H_DECL(nm9_at_pause);
JNI_H_DECL(nm9_at_stop);
JNI_H_DECL(nm9_at_release);
JNI_H_DECL(nm9_at_write);
JNI_H_DECL(nm9_at_state);
void nm9_audio_pause(int paused);

/* ---------------------------------------------------------------- nm9_input.c */
void nm9_input_init(void);
void nm9_input_poll(void); /* once per frame, before render */

/* ---------------------------------------------------------------- the natives */
/* libCopStory.so's exports, resolved by nm9_main.c (see docs: argument
 * meaning read from the ARM code, not from names).
 *   GameRenderer_nativeInit(env, cls, a, b, c): first call -> appInit(a, b, c);
 *       later calls only set mbOGLLostContext. a,b are most likely the screen
 *       size; what c is is not settled yet.
 *   GameRenderer_nativeResize / nativeDestroy only log.
 *   GameRenderer_nativeRender ignores its arguments.
 *   TouchPressed/Moved/Released(env, cls, i1, i2, i3): three ints each.
 *   Orientation / Accelerometer(env, cls, x, y, z): three 32-bit floats, passed
 *       as raw words (softfp: two in r2/r3, the third on the stack). */
typedef struct Nm9Native {
  void (*game_init)(void *env, void *cls);
  void (*rend_init)(void *env, void *cls, int a, int b, int c);
  void (*rend_resize)(void *env, void *cls, int w, int h);
  void (*rend_render)(void *env, void *cls);
  void (*rend_destroy)(void *env, void *cls);
  void (*touch_pressed)(void *env, void *cls, int i1, int i2, int i3);
  void (*touch_moved)(void *env, void *cls, int i1, int i2, int i3);
  void (*touch_released)(void *env, void *cls, int i1, int i2, int i3);
  void (*touch_canceled)(void *env, void *cls, int i1, int i2, int i3);
  void (*key_down)(void *env, void *cls, int keycode);
  void (*key_up)(void *env, void *cls, int keycode);
  void (*accelerometer)(void *env, void *cls, uint32_t x, uint32_t y, uint32_t z);
  void (*orientation)(void *env, void *cls, uint32_t x, uint32_t y, uint32_t z);
  void (*view_pause)(void *env, void *cls);
  void (*view_resume)(void *env, void *cls);
  void (*view_set_focus)(void *env, void *cls, int focus);
  /* Arguments not read yet (GLUtils_Device_nativeInit 0x280c18,
   * GLResLoader_nativeInit 0x2832c0, GLMediaPlayer_nativeInit 0x281ba8):
   * called as (env, cls) only after their disassembly is checked. */
  void *device_init, *resloader_init, *mediaplayer_init;
} Nm9Native;
extern Nm9Native g_nat;
extern void *g_game_cls; /* the GLGame class object, as the natives take it */
extern void *g_rend_cls; /* the GameRenderer class object */

/* After jni_init(): MonitorEnter/Exit lock for real (the runtime's are
 * no-ops; loading threads would read each other's shared buffers). */
void nm9_java_patch_monitors(void);

/* ---------------------------------------------------------------- nm9_gpu.c */
void nm9_gpu_boost_apply(void);  /* once, when the game is up */
void nm9_gpu_boost_tick(void);   /* dock/undock and option changes */
void nm9_gpu_boost_remove(void); /* back to the saved rate */

#endif /* NM9_H */