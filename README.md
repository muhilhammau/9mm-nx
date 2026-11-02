# 9mm-nx

An **unofficial** Nintendo Switch wrapper for *9mm* (Gameloft, Android, armeabi-v7a).
It runs the game's own 32-bit ARM libraries (`libCopStory.so`, `libStormGLOFT.so`)
in a Switch homebrew process on top of an Android-compatibility runtime, and
replaces the Java side the game expects (JNI callbacks, EGL context, input, audio).

**This repository contains no Gameloft code or data.** You need your own legally
obtained copy of the game: the APK and the data the game downloads on its first
launch. 9mm and Gameloft are trademarks of their owners; this project is not
affiliated with or endorsed by them.

## Status

Tested on hardware: the main menu and the first mission render with correct
textures, audio plays. Work in progress; see *Known issues*.

## What you need

| Item | Where |
|---|---|
| 9mm APK (armeabi-v7a, package `com.gameloft.android.ANMP.Gloft9MHM`) | `sdmc:/switch/9mm_nx/` (any file name) |
| The game's downloaded data (`*.gla`, `sounds_hi.glza`, `*.dat`, ...) | `sdmc:/switch/9mm_nx/gameloft/games/Gloft9MHM/` |
| `9mm_nx.nro` (built, see below) | `sdmc:/switch/` |

No OBB is used. A Switch on Atmosphere (or any CFW that runs homebrew) is assumed.

## Building

Prerequisites (not included here):

- Docker. The wrapper is built in `ghcr.io/vita2hos/devcontainer/vita2hos:latest`
  (AArch32 devkitARM); the launcher NRO in `devkitpro/devkita64:latest`.
- The **android32 runtime** checked out as `./runtime` (a submodule or a plain
  clone). It provides the ELF loader, the JNI layer, libc/bionic shims and the
  Mesa/EGL glue. See its own documentation for `portlibs32/` (Mesa for AArch32),
  which must sit in the project folder to get the real renderer.
- **libnx32** (the AArch32 libnx fork), prefix in `DCR_LIBNX32` or next to this folder.
- A launcher icon at `launcher/icon.jpg` (256x256). Not included; use your own.

```powershell
$env:DCR_LIBNX32 = "C:\path\to\libnx32\prefix"
.\build.ps1                       # -> 9mm_nx.nsp + 9mm_nx.build

# the launcher NRO carries the payload; rebuild it after every payload change
$port = (Get-Location).Path; $rt = (Resolve-Path .\runtime).Path
docker run --rm --platform linux/amd64 -v "${port}:/work" -v "${rt}:/runtime:ro" `
  -w /work/launcher devkitpro/devkita64:latest bash -lc 'make -j$(nproc) A32=/runtime'
# -> launcher/9mm_nx.nro
```

`debug.log` and `crash.log` are written next to the NRO's data folder
(`sdmc:/switch/9mm_nx/`).

## Controls

The game is played with touch; the Joy-Cons and the touch screen are mapped to it
(positions as in the 9mm Vita port, scaled from 960x544 to 1280x720). Every
position can be changed in `config.ini` (`controls.touch_*`).

| Switch | Action |
|---|---|
| Left stick | movement (floating virtual joystick) |
| Right stick | camera (a finger that keeps dragging) |
| R / ZR | fire |
| L | bullet time |
| B | sprint (held) |
| Y | reload |
| + | pause |
| D-pad left / right | switch weapon (the game needs a real horizontal swipe) |
| Touch screen | passed through |

## How it works

| File | Role |
|---|---|
| `source/nm9_main.c` | loads `libStormGLOFT.so` then `libCopStory.so`, runs the constructors and `JNI_OnLoad`, then what the activity did: `GLResLoader_nativeInit`, `GLGame_nativeInit`, a GLES 2 context, `GameRenderer_nativeInit/Resize`, and the frame loop (`GameRenderer_nativeRender`) |
| `source/nm9_java.c` | the Java the engine calls back into (`GLGame`, `GameRenderer`, `AudioTrack`, `android.os.Build`, ...). Note: `DetectDeviceGPU`, `getBuildType`, `detectLanguage` and `getWifiIP` return **int**; answering with a string makes the engine pick the wrong texture path |
| `source/nm9_input.c`, `touchmap.*` | Joy-Con / touch mapping, sampled at 120 Hz so short taps are not lost |
| `source/nm9_gl.c`, `nm9_gldiag.c` | EGL/GLES 2 context on Mesa (nouveau); frame diagnostics and a present fallback when the engine does not swap by itself |
| `source/nm9_exc.c` | emulates `SWP`/`SWPB` in the exception hook: the game's lock helpers use this ARMv5 instruction, which the Switch CPU faults on |
| `source/nm9_pvr.c` | the game's world textures are PVRTC 4bpp and the GPU has none: advertises `GL_IMG_texture_compression_pvrtc` (**in front** of the extension list; appended at the end the engine ignores it) and decodes PVRTC to RGBA8888 on the CPU |
| `source/nm9_audio.c` | `android.media.AudioTrack` over `audout` |
| `launcher/` | the NRO: checks the APK, shows setup help, starts the payload |
| `tools/imports.cfg` | modules and import settings for `gen_imports.py` |

## Known issues

- PVRTC is decoded on the CPU (about 0.9 ms per 512 KB level here); loading
  screens cost time. PVRTC 2bpp is not decoded (grey).
- `SWP` is emulated through an exception: correct but slow when the engine takes
  its locks a lot. A static patch of the 31 sites is the next step if it shows.
- The engine logs `unbound parameter ...` for the sampler uniforms of its world
  shaders; the rendering looks right, but the cause is not explained.
- `getKeyboardText` returns `NULL`; there is no on-screen keyboard (name entry).
- Online services, billing, Gameloft Live and DRM natives answer "nothing there".
- Some leftovers of the original port this was started from remain in
  `nm9_obb.c` (OBB lookup, unused by this game).

## Credits and licenses

- The game: Gameloft. Not included.
- The runtime, libnx32, the Mesa AArch32 build: their own projects and licenses;
  check them before redistributing builds.
- The 9mm Vita port was used as a reference for the native call signatures and the
  HUD positions of the controls. Check its license before copying code from it.
- Earlier ports on the same runtime (The Dark Knight Rises, Tintin) were the
  starting point and a reference.
- The code in this repository: MIT (see `LICENSE`).
