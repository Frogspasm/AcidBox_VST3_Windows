# AcidBox plugin

AcidBox as a VST3 instrument (plus Audio Unit on Mac and a standalone app). It contains the same engine as the web version: two oscillators with morph and ring mod, a 303-style filter with accent and slide, four distortion types, echo, reverb, five synthesized drums with sidechain ducking and a kick + snare compressor, four bass and four drum patterns, and a 16-bar song chain.

## Getting the plugin without installing any tools

GitHub builds it for Windows, Mac and Linux for free.

1. Create a free account at github.com, then click **New repository**. Give it any name and leave it empty.
2. On the new repository's page, click **uploading an existing file** and drag in `CMakeLists.txt`, `README.md`, `.gitignore` and the `Source` folder. Click **Commit changes**.
3. The workflow file lives in a hidden folder, which some file browsers skip when you drag. To add it reliably, click **Add file → Create new file**, type `.github/workflows/build.yml` as the name, paste in the contents of that file, and commit.
4. Open the **Actions** tab. A build starts by itself and takes about 10 minutes. When all three show a green tick, open the run and download **AcidBox-Windows**, **AcidBox-macOS** or **AcidBox-Linux** at the bottom of the page.

Each download is a zip inside a zip. Unzip both.

## Installing

| System | Copy this | To here |
|---|---|---|
| Windows | `AcidBox.vst3` | `C:\Program Files\Common Files\VST3\` |
| Mac (VST3) | `AcidBox.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |
| Mac (Logic, GarageBand) | `AcidBox.component` | `~/Library/Audio/Plug-Ins/Components/` |
| Linux | `AcidBox.vst3` | `~/.vst3/` |

**Mac only:** the plugin isn't notarized by Apple, so macOS blocks it at first. After copying, open Terminal and run:

```
xattr -cr ~/Library/Audio/Plug-Ins/VST3/AcidBox.vst3 ~/Library/Audio/Plug-Ins/Components/AcidBox.component
```

Then rescan plugins in your DAW. AcidBox appears as an instrument.

## Using it in a DAW

- **Sync to host** (on by default): press play in your DAW and AcidBox plays in time with it. It follows the DAW's tempo and position, so loops and jumps land on the right step and the right song bar.
- With your DAW stopped, the **Play** button runs AcidBox on its own clock.
- **MIDI notes** on the AcidBox track play the bass live over the drums. Overlapping notes slide, and velocity 100 or higher adds accent.
- **Hardware controllers:** right-click any knob and choose MIDI Learn, then move a knob or fader on your controller. The knob shows a small CC badge once it's mapped. The **MIDI** button lists every mapping (yours plus the built-in ones, like CC 74 for cutoff) and lets you forget them. Mappings are saved with your project.
- The **Effects** tab has the kick + snare compressor. It's off by default, and when it's off the drums sound exactly as before.
- Every knob can be automated, and your patterns and song are saved inside your DAW project.

## Building it yourself (optional)

Install CMake 3.22+ and a C++ compiler (Visual Studio 2022 with "Desktop development with C++" on Windows, Xcode on Mac, or `build-essential` plus the libraries listed in the workflow on Linux). Then, in this folder:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DACIDBOX_COPY_AFTER_BUILD=ON
cmake --build build --config Release
```

The first run downloads JUCE (the plugin framework) automatically. With `ACIDBOX_COPY_AFTER_BUILD=ON` the plugin is copied into your system plugin folder; on Windows that needs a terminal opened as administrator.

## Licence note

AcidBox is built on JUCE 8, which is free for personal projects and for anyone earning under the JUCE Starter licence limit. If you want to sell the plugin, check the current terms at juce.com.
