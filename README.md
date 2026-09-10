# FreaxVolume — by Freaxment

A tiny channel utility for macOS in the spirit of Fruity Balance, Ableton Utility
and Bitwig Tool. VST3 + AU, universal binary (Apple Silicon + Intel).

Looks like the rest of the Freaxment plugins: dark panel, acid-lime accent, TikTok Sans.
Two layouts drive the same four parameters. **Faders** (default): horizontal faders with
editable values, lamp toggles in the header next to the Freaxment logo. **Knobs**: two big
knobs with value pills and MUTE / MONO buttons along the bottom. Right-click the plugin
background to pick a layout; the choice is saved with the project.

```
┌──────────────────────────────┐
│    freaxvolume  by Freaxment   │
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
* Double-click a fader to return to neutral. Click the value pill to type a value
  (`-6`, `+3 dB`, `80 %`, `-inf`, `mono` all work).

## Build

Requires Xcode (command line tools are enough) and CMake ≥ 3.22 + Ninja.
The repo pulls JUCE 8.0.15 into `libs/JUCE` on first build.

```bash
./build.sh
```

Output is copied to `~/Library/Audio/Plug-Ins/VST3/FreaxVolume.vst3` and
`~/Library/Audio/Plug-Ins/Components/FreaxVolume.component`. Rescan plugins in your DAW.

If CMake/Ninja are missing and you don't use Homebrew:

```bash
uv tool install cmake && uv tool install ninja
```

## Windows build and the release archive

Windows needs MSVC, so the Windows VST3 is built by GitHub Actions
(`.github/workflows/build.yml`). Every push to `main` builds macOS (VST3 + AU,
self-test + auval) and Windows (VST3, validated with pluginval) and uploads
`FreaxVolume-by-Freaxment-v<version>-macOS-Windows.zip` as a workflow artifact.
Pushing a tag such as `v1.1.1` also attaches the archive to a GitHub Release.
`packaging/INSTALL.txt` is the note shipped inside the archive.

Building on a Windows machine by hand works too:

```bat
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE libs\JUCE
cmake -B build -A x64
cmake --build build --config Release --target FreaxVolume_VST3
```

## Self-test

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFREAXVOLUME_BUILD_TESTS=ON
cmake --build build
./build/FreaxVolumeTest_artefacts/Release/FreaxVolumeTest snapshots
```

Checks the knob laws numerically, smoothing, parameter text, state round-trip,
mono bus layout, loads the installed VST3 and AU bundles via JUCE hosting
(name, manufacturer, audio) and writes PNG snapshots of the editor.
Apple's own validator also passes: `auval -v aufx Smpl Frxm`.

## Identifiers

| | |
|---|---|
| Product | FreaxVolume |
| Manufacturer | Freaxment |
| Bundle ID | com.freaxment.freaxvolume |
| Manufacturer code / plugin code | `Frxm` / `Smpl` |

## Layout

```
CMakeLists.txt        project (plugin + optional test host)
build.sh              one-shot build & install
Source/PluginProcessor.*   parameters, knob laws, DSP
Source/PluginEditor.*      host window, swaps skins
Source/Skins.h             skin id, persistence, skin base class with the right-click skin menu
Source/FamilySkin.*        Faders layout: Fader / Lamp widgets
Source/KnobSkin.*          Knobs layout: rotary knobs + pill buttons
Source/LookAndFeel.h       Theme palette, embedded TikTok Sans, knob / slider / button / label drawing
Assets/                    Freaxment logo and TikTok Sans fonts (OFL), embedded as binary data
tests/FreaxVolumeTest.cpp    self-test host
libs/JUCE                  JUCE 8.0.15 (cloned, not committed)
```

## License

JUCE 8 is used under its dual license. Distributing FreaxVolume binaries publicly
either requires a JUCE license that matches your revenue tier or releasing this
source under GPLv3, as JUCE's terms require.
