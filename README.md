# Simpleton — by Freaxment

A tiny channel utility for macOS in the spirit of Fruity Balance, Ableton Utility
and Bitwig Tool. VST3 + AU, universal binary (Apple Silicon + Intel).

Two skins drive the same four parameters: **Minimalist** (warm paper, two-tone arcs)
and **Flex** (dark panel, lime accent). Right-click the plugin background and pick
the skin from the menu; the choice is saved with the project.

```
┌──────────────────────────────┐
│    simpleton  by Freaxment   │
│   VOLUME            WIDTH    │
│    (o)               (o)     │
│   0.0 dB           100 %     │
│  [ MUTE ]          [ MONO ]  │
└──────────────────────────────┘
```

## Controls

| Control | knob left      | knob middle (neutral) | knob right       |
|---------|----------------|-----------------------|------------------|
| Volume  | 0 % = silence  | 50 % = 0 dB           | 100 % = +10 dB   |
| Width   | 0 % = mono     | 100 % = untouched     | 300 % = side ×3  |
| Mute    | — output is silenced (20 ms fade, no click)              |
| Mono    | — mid only, L = R (overrides Width)                      |

* Volume below the middle is a fader-like taper (gain = (2x)², so 25 % = −12 dB);
  above the middle it is linear in dB.
* Width is the side gain in percent: L = M + g·S, R = M − g·S with g = width / 100.
  The knob travel is non-linear (g = 2x² + x for knob position x) so 100 % sits in the middle.
* Every change is smoothed over 20 ms, so automation and button presses never click.
* Double-click a knob to return to neutral. Double-click a value readout to type a
  value (`-6`, `+3 dB`, `80 %`, `-inf`, `mono` all work).
* The window is resizable; the Minimalist layout scales, Flex keeps its fixed geometry.

## Build

Requires Xcode (command line tools are enough) and CMake ≥ 3.22 + Ninja.
The repo pulls JUCE 8.0.15 into `libs/JUCE` on first build.

```bash
./build.sh
```

Output is copied to `~/Library/Audio/Plug-Ins/VST3/Simpleton.vst3` and
`~/Library/Audio/Plug-Ins/Components/Simpleton.component`. Rescan plugins in your DAW.

If CMake/Ninja are missing and you don't use Homebrew:

```bash
uv tool install cmake && uv tool install ninja
```

## Windows build and the release archive

Windows needs MSVC, so the Windows VST3 is built by GitHub Actions
(`.github/workflows/build.yml`). Every push to `main` builds macOS (VST3 + AU,
self-test + auval) and Windows (VST3, validated with pluginval) and uploads
`Simpleton-by-Freaxment-v<version>-macOS-Windows.zip` as a workflow artifact.
Pushing a tag such as `v1.1.1` also attaches the archive to a GitHub Release.
`packaging/INSTALL.txt` is the note shipped inside the archive.

Building on a Windows machine by hand works too:

```bat
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE libs\JUCE
cmake -B build -A x64
cmake --build build --config Release --target Simpleton_VST3
```

## Self-test

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMPLETON_BUILD_TESTS=ON
cmake --build build
./build/SimpletonTest_artefacts/Release/SimpletonTest snapshots
```

Checks the knob laws numerically, smoothing, parameter text, state round-trip,
mono bus layout, loads the installed VST3 and AU bundles via JUCE hosting
(name, manufacturer, audio) and writes PNG snapshots of the editor.
Apple's own validator also passes: `auval -v aufx Smpl Frxm`.

## Identifiers

| | |
|---|---|
| Product | Simpleton |
| Manufacturer | Freaxment |
| Bundle ID | com.freaxment.simpleton |
| Manufacturer code / plugin code | `Frxm` / `Smpl` |

## Layout

```
CMakeLists.txt        project (plugin + optional test host)
build.sh              one-shot build & install
Source/PluginProcessor.*   parameters, knob laws, DSP
Source/PluginEditor.*      host window, swaps skins
Source/Skins.h             skin id, persistence, skin base class with the right-click skin menu
Source/MinimalistSkin.*    Minimalist skin (layout + drawing via LookAndFeel.h)
Source/FlexSkin.*          Flex skin (self-painted knobs and buttons)
Source/LookAndFeel.h       Minimalist palette, knob / button / text-editor drawing
Resources/freaxment_logo.svg  logo, embedded as binary data
tests/SimpletonTest.cpp    self-test host
libs/JUCE                  JUCE 8.0.15 (cloned, not committed)
```

## License

JUCE 8 is used under its dual license. Distributing Simpleton binaries publicly
either requires a JUCE license that matches your revenue tier or releasing this
source under GPLv3, as JUCE's terms require.
