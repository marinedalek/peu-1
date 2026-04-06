# PEU/1 AU Build Notes

## Requirements

You need local copies of:

- [VST3 SDK](https://steinbergmedia.github.io/vst3_dev_portal/pages/What+is+the+VST+3+SDK/Wrappers/AUv2+Wrapper.html)
- [AudioUnitSDK](https://github.com/apple/AudioUnitSDK)
- [CoreAudioUtilityClasses](https://developer.apple.com/library/archive/samplecode/CoreAudioUtilityClasses/CoreAudioUtilityClasses.zip)

Required environment variables:

- `VST3SDK_ROOT`
- `SMTG_AUDIOUNIT_SDK_PATH`
- `SMTG_COREAUDIO_SDK_PATH`

## Workflow

First, edit the source/CMake/plist files (the ones in the repo already have the necessary changes). Then, from the repo root:

```bash
export VST3SDK_ROOT=/path/to/vst3sdk
export SMTG_AUDIOUNIT_SDK_PATH=/path/to/AudioUnitSDK
export SMTG_COREAUDIO_SDK_PATH=/path/to/CoreAudioUtilityClasses
cmake -S . -B build -G Xcode
```

Open `build/PEU_1.xcodeproj` and create a scheme for the `PEU_1_AU` target (it only creates `PEU_1` by default). This builds both the AU and the VST3 target it depends on. You'll get VSTGUI deprecation warnings unless you've updated `CMakeLists.txt` to suppress deprecated `libc++` warnings triggered by VSTGUI. Leave that in place unless the Steinberg SDK is updated.

## Files to edit

The files in the repo are already correct, but for future reference, the important changes are:

### `CMakeLists.txt`

- AUv2 target setup
- VSTGUI deprecation suppression
- project version
- `OUTPUT_NAME "PEU_1"`

### `resource/au-info.plist`

- display name: `PEU/1`
- manufacturer code: `MDLK`
- subtype: `peu1`
- type: `aufx`

Set the version metadata:

```xml
<key>AudioUnit Version</key>
<string>00010000</string>
```

and:

```xml
<key>version</key>
<integer>65536</integer>
```

If Logic ignores metadata changes, it likely means you have to update the AU version metadata.

### `source/version.h`

Keep the real plugin filename filesystem-safe: `PEU-1.vst3`. Display strings can still say `PEU/1`.

### `source/Peu1processor.cpp`

Do not treat `data.inputs[1]` as the right channel, use `data.inputs[0]` plus `numChannels`.
