#pragma once

#include <JuceHeader.h>

class MainComponent;

class VstPanel final : public juce::Component
{
public:
    explicit VstPanel(MainComponent& ownerToUse);

    void resized() override;

    juce::TextButton scanButton { "Rescan VST3" };
    juce::TextButton addButton { "Add Plugin" };
    juce::TextButton clearPluginCacheButton { "Clear Cache" };
    juce::TextButton cancelScanButton { "Cancel" };

    juce::ToggleButton globalBypassButton { "Bypass All" };
    juce::ToggleButton muteButton { "Mute" };

    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton { "Save Preset" };
    juce::TextButton deletePresetButton { "Delete" };
    juce::TextButton defaultPresetButton { "Set as Default" };

    juce::ToggleButton autoReconnectButton {
        "Auto Reconnect"
    };

    juce::ToggleButton autoStartButton {
        "Start with Windows"
    };

    juce::TextButton removeButton { "Remove" };
    juce::TextButton bypassButton { "Bypass" };
    juce::TextButton upButton { "Up" };
    juce::TextButton downButton { "Down" };
    juce::TextButton editorButton { "Open GUI" };

    juce::Label chainTitle;

    juce::ListBox pluginChainList;

    juce::TextEditor pluginSearch;
    juce::ListBox pluginBrowserList;

    void setScanningState(bool scanning);

private:
    void wireCallbacks();

    MainComponent& owner;
};
