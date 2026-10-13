# Verification of the reconstructed 0.5 edition

- Windows x64 Release host and separate VST3 scanner build passed with MSVC 19.29 / JUCE 8.0.8.
- An isolated test of the host's actual graph passed clean startup, mono input duplication to stereo output, input/output utility routing, gain, mute, plugin bypass, bypass all, safety limiting, settings round-trip and preset gain serialization.
- The clean graph has four nodes: device input, input utility, output utility and device output. External VST3 plugins are the only configurable processors.
- The native main component was rendered and inspected: device controls, VST3 browser, VST presets and chain are present; mode tabs, built-in effects and account controls are absent.
- C++ comments were removed from application and scanner sources. Compiler directives and dependency license notices were retained.
- The tested graph used an isolated profile and synthetic audio. Physical microphone operation, third-party plugin compatibility and Windows reboot/autostart were not tested in this verification.
- The legacy snapshot was not retained. This is a source reconstruction, not a byte-identical rebuild of the historic 0.5 executable.
