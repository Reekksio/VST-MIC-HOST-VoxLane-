# VoxLane — Beta 0.7

VoxLane is a portable real-time microphone processor for Windows, built with C++ and JUCE. Use external VST3 plugins, built-in VoxLane processors or Fun Voices, then send your sound to an audio output, Discord, OBS or games through a virtual audio cable.

## Download

**[Download VoxLane Beta 0.7](https://github.com/Reekksio/VST-MIC-HOST-VoxLane-/releases/download/v0.7.0/VoxLane-Beta-0.7.zip)** · [Release notes](https://github.com/Reekksio/VST-MIC-HOST-VoxLane-/releases/latest)

Extract the ZIP and run **VoxLane Beta 0.7.exe**. Keep `VSTScanner.exe` and `ThirdParty` beside it. No installation is required.

**All audio features are included. No account, payment, activation or internet connection is required to use VoxLane.**

## Screenshots

![VoxLane Professional](assets/voxlane.png)

![VoxLane Fun Voices](assets/fun-voices.png)

## Features

- External VST3 hosting, scanning, plugin editors, reordering and bypass.
- 28 built-in VoxLane processors with dedicated editors and full parameter access.
- 100 Fun Voices in 10 categories, adjustable effect controls, randomization and custom voice presets.
- A/B comparison, Undo/Redo, preset saving and loading.
- Favourite plugins and voices, search and a VoxLane-only filter.
- Compressor gain-reduction metering, input/output meters and a safety limiter.
- Device selection, sample rate, buffer size, gain controls and audio latency display.
- Automatic audio reconnection, close to Windows tray and optional startup with Windows.

Fun Voices starts at **CLEAN / No Effect**. Pitch/formant and layered effects add processing latency; total latency also depends on the audio device and plugin chain.

## Discord, OBS and games

Use a virtual audio cable such as [VB-CABLE](https://vb-audio.com/Cable/):

1. Select your microphone as VoxLane's input.
2. Select **CABLE Input** as VoxLane's output.
3. Configure your processing chain or select a Fun Voice.
4. Select **CABLE Output** as the microphone in Discord, OBS or your game.

`Microphone → VoxLane → VB-CABLE → Discord / OBS / Game`

If the output test tone works but your microphone is silent, check the selected input, active channel and **Mute** control.

## Project status

VoxLane Beta 0.7 is under development. Please report reproducible issues through [GitHub Issues](https://github.com/Reekksio/VST-MIC-HOST-VoxLane-/issues).

The [VoxLane 0.5 source edition](https://github.com/Reekksio/VST-MIC-HOST-VoxLane-/tree/legacy-0.5) remains publicly available. Beta 0.7 application source is private.

<details>
<summary>Account & Payments Showcase</summary>

An [archived integration build](https://github.com/Reekksio/VST-MIC-HOST-VoxLane-/releases/tag/v0.7) demonstrates Supabase authentication, persistent sessions, Stripe Checkout and account-based access management. This is a separate showcase edition; the standard download above includes all audio features without accounts or purchases.

</details>
