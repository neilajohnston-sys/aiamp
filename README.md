# Chime 30

A Windows VST3 (plus standalone .exe) amp-sim voiced after a Class A "Top Boost" British combo:
chimey highs, glassy cleans that break up into jangly crunch as you turn it up. Built with JUCE 8.

## Controls

| Control | What it does |
|---|---|
| Volume | Preamp level and saturation |
| Brilliant (switch) | On: brighter, more gain. Off: warmer "Normal" channel |
| Treble / Bass | Top Boost style two-knob tone controls |
| Cut | High-cut in the power section. Higher = darker (like the real control) |
| Master | Class A power-stage drive and compression |
| Reverb | Spring reverb level |
| Speed / Depth | Tremolo |
| Output | Plugin output level (-24 to +12 dB) |
| Cab Sim (switch) | Speaker cabinet filter. Turn off if you use your own IR loader |

## Signal chain (nonlinear stages run 2x oversampled)

Channel voicing -> triode stage 1 -> Treble/Bass tone circuit (with fixed 750 Hz scoop and 3.2 kHz chime)
-> asymmetric recovery stage -> spring reverb -> tremolo -> class A power stage with sag -> Cut -> cab.

## Getting a Windows build

### Option A - let GitHub build it (no compiler needed)

1. Create a free GitHub account and a new repository, then upload this folder's contents
   (keep the `.github` folder).
2. Open the **Actions** tab, pick **Build Windows VST3**, and click **Run workflow**
   (it also runs on every push).
3. When it finishes (about 10 minutes), download the **Chime30-VST3-Windows** artifact. Unzip it.
4. Copy the `Chime 30.vst3` folder into `C:\Program Files\Common Files\VST3`.

### Option B - build locally

Install **Visual Studio 2022 Community** (workload: *Desktop development with C++*), **CMake** and **Git**. Then in
a terminal in this folder:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel

The plugin is at `build\Chime30_artefacts\Release\VST3\Chime 30.vst3`. Right-click `install_windows.bat` and
choose **Run as administrator** to copy it to the VST3 folder, then rescan plugins in your DAW.
The standalone app is at `build\Chime30_artefacts\Release\Standalone\Chime 30.exe`.

The plugin uses a static runtime, so it doesn't need the Visual C++ redistributable on other machines.

Before sharing it, change `COMPANY_NAME` and `PLUGIN_MANUFACTURER_CODE` in `CMakeLists.txt`.

## Tweaking the sound

Tone-stack frequencies are in `updateTone`, stage gains and bias values in `processBlock`
(`Source/PluginProcessor.cpp`), and spring behaviour in `Source/SpringReverb.h`.

"Vox" and "AC30" are trademarks of their owners; this plugin is an independent, original DSP design that is
only described as voiced in a similar style.
