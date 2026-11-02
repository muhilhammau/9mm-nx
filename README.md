<div align="center">

<img src="launcher/icon.jpg" alt="9mm" width="180">

# 9mm_nx

**9mm on Nintendo Switch**

An unofficial Nintendo Switch wrapper/port of Gameloft's Android game.

[![Switch](https://img.shields.io/badge/Nintendo_Switch-Homebrew-E60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)](#installation)
[![Version](https://img.shields.io/badge/version-v1.0.0-4C8BF5?style=for-the-badge)](#)
[![Android](https://img.shields.io/badge/Android-1.1.0-3DDC84?style=for-the-badge&logo=android&logoColor=white)](#about)
[![AArch32](https://img.shields.io/badge/AArch32-32--bit-0091BD?style=for-the-badge&logo=arm&logoColor=white)](#building)

</div>

---

## About

9mm_nx runs the original 32-bit Android version of **9mm** on Nintendo Switch, using the open-source [android32](https://github.com/aks796/android32) runtime.

The supported game build is:

```	ext
Package:      com.gameloft.android.ANMP.Gloft9MHM
Version:      1.0.1
Version code: 110
ABI:          armeabi-v7a / AArch32
```

The original ARM game libraries (libCopStory.so, libStormGLOFT.so) run directly inside a 32-bit Horizon OS process. Players must provide their own APK and game data files (gameloft/games/Gloft9MHM/); these are not included in the repository or release download.

**No game files are included.** You need your own copy of the game's APK. This project is not affiliated with or endorsed by Gameloft or Nintendo.

> Work in progress.

## Status

| Feature | State |
| --- | --- |
| Boot, loading, 3D gameplay | working ✓ |
| Touchscreen | working ✓ |
| Audio (AudioTrack on audout) | working ✓ |
| Gamepad: movement (left stick), camera (right stick) | working ✓ |
| Gamepad: R/ZR fire, L bullet time, B sprint, Y reload, + pause | working ✓ |
| D-pad weapon switching | working ✓ |
| Intro and cutscene videos | skipped (the game is told they finished) |
| Online features / Gameloft Live | not implemented |

## Features

- Original Android **1.1.0** game running natively in AArch32 on Switch.
- **1280 x 720** widescreen presentation with touchscreen and gamepad integration.
- Floating virtual joystick (left stick) and camera drag (right stick).
- Direct game SD data mounting (gameloft/games/Gloft9MHM/).
- Native checkpoint saves and settings persistence.
- Audio playback via android32 AudioTrack / OpenSL ES driver.
- PVRTC texture decompression on CPU (4bpp supported, 2bpp displays grey).
- 64-bit NRO launcher carrying the 32-bit payload with sphaira HOME-menu forwarder support.

## Installation

### Requirements

- A Nintendo Switch running Atmosphère custom firmware.
- [sphaira](https://github.com/ITotalJustice/sphaira) for installing the HOME-menu forwarder.
- Your own supported **1.1.0 ARMv7 APK** (com.gameloft.android.ANMP.Gloft9MHM).
- Extracted Gameloft SD data folder (Gloft9MHM).

Copy the switch/ folder to the root of your SD card. Ensure the game files are placed according to the layout below:

```	ext
sd:/switch/9mm_nx/
├── 9mm_nx.nro
├── 9mm.apk
└── gameloft/
    └── games/
        └── Gloft9MHM/
            ├── *.gla
            ├── sounds_hi.glza
            ├── *.dat
            └── ...
```

### First launch

1. Open **sphaira** and locate **9mm** in Homebrew.
2. Select **Install Forwarder**.
3. Launch the game icon from the Switch **HOME menu**.
4. On initial launch, the runtime extracts necessary native libraries from the APK and initializes local configurations.

## Controls

### Gameplay

| Nintendo Switch | Input |
| --- | --- |
| **Left Stick** | Character movement (floating virtual joystick) |
| **Right Stick** | Camera / look (continuous drag) |
| **R / ZR** | Fire |
| **L** | Bullet time |
| **B** (held) | Sprint |
| **Y** | Reload |
| **+** | Pause / Game menu |
| **D-Pad Left / Right** | Switch weapon (emulates horizontal swipe) |
| **Touchscreen** | Native touch controls |

### Cursor / Navigation

- Move the **Right Stick** to control the camera view.
- Press **R / ZR** to fire at the current aim position.
- Button positions can be modified by creating config.ini inside /switch/9mm_nx/ (refer to source for control mapping options).

## File Structure & Saves

The application directory is located under sd:/switch/9mm_nx/:

```	ext
data/              Android internal storage and native saves
gameloft/          Gameloft shared data (gameloft/games/Gloft9MHM/)
sdcard/            Android external storage root
config.ini         Custom control mapping overrides
debug.log          Current execution log
crash.log          Crash diagnostics output
```

## Building

### Requirements

- Docker with Linux container support.
- [android32](https://github.com/aks796/android32) in 
untime/ (Git submodule).
- [libnx32](https://github.com/aks796/libnx32) prefix (version 4.12.0 or newer).
- [mesa32](https://github.com/aks796/mesa32) or mesa-switch32 in portlibs32/.
- devkitPro devkita64 for launcher compilation.
- A launcher icon at launcher/icon.jpg (256x256). Not included; use your own.

### Build steps

```powershell
git submodule update --init --recursive
.\build.ps1
cd launcher; ./build.sh
```

The compiled binary will be located at launcher/9mm_nx.nro.

## Known Issues

- PVRTC is decoded on the CPU (about 0.9 ms per 512 KB level); loading screens cost time. PVRTC 2bpp is not decoded (grey).
- SWP/SWPB ARMv5 instructions are emulated through an exception hook: correct but slow when the engine takes its locks frequently. A static patch of the sites is the next step if it shows.
- The engine logs unbound parameter for the sampler uniforms of its world shaders; the rendering looks right, but the cause is not explained.
- getKeyboardText returns NULL; there is no on-screen keyboard (name entry).
- Online services, billing, Gameloft Live and DRM natives answer "nothing there".
- Some leftovers of the original port this was started from remain in 
m9_obb.c (OBB lookup, unused by this game).

## Credits

- [aks796/android32](https://github.com/aks796/android32) — core AArch32 Android runtime.
- [libnx32](https://github.com/aks796/libnx32) and Mesa Switch contributors — SDK and graphics stack.
- [tasm2_nx](https://github.com/Muhilhammau/tasm2-nx) — documentation and structure reference.
- [tintin_nx](https://github.com/muhilhammau/tintin-nx) — structure and README reference.
- 9mm Vita port — reference for native call signatures and HUD positions of the controls.
- Gameloft — original game developers and rights holders.

## License

The port code is licensed under MIT (see LICENSE). Game assets and binaries remain the property of their respective owners.
