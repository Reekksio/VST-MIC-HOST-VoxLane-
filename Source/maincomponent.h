#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "AudioUtilityProcessor.h"

class VstPanel;

class MainComponent final :
    public juce::Component,
    private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress&) override;

private:
    using Node = juce::AudioProcessorGraph::Node;
    using NodePtr = Node::Ptr;

    friend class VstPanel;

    class PluginWindow final : public juce::DocumentWindow
    {
    public:
        explicit PluginWindow(NodePtr node);

        void closeButtonPressed() override;

        NodePtr node;
    };

    void initialiseAudio();
    void createGraphIO();
    void rebuildGraph();

    class DeviceChangeListener final
        : public juce::ChangeListener
    {
    public:
        explicit DeviceChangeListener(
            MainComponent& o)
            : owner(o)
        {
        }

        void changeListenerCallback(
            juce::ChangeBroadcaster*) override
        {
            owner.handleAudioDeviceChanged();
        }

    private:
        MainComponent& owner;
    };

    DeviceChangeListener deviceChangeListener { *this };

    void handleAudioDeviceChanged();

    void rememberCurrentDevices();
    void tryReconnectAudioDevices();

    juce::String preferredInputDevice;
    juce::String preferredOutputDevice;

    bool autoReconnectEnabled = true;

    int reconnectTimerCounter = 0;

    std::atomic<bool> reconnectInProgress { false };

    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer processorPlayer;

    juce::AudioProcessorGraph graph;

    NodePtr audioInputNode;
    NodePtr audioOutputNode;

    NodePtr inputUtilityNode;
    NodePtr outputUtilityNode;

    AudioUtilityProcessor* inputUtility = nullptr;
    AudioUtilityProcessor* outputUtility = nullptr;

    class PluginScanThread final
        : public juce::Thread
    {
    public:
        explicit PluginScanThread(
            MainComponent& ownerToUse)
            : juce::Thread(
                "VST Scan Thread"),
              owner(ownerToUse)
        {
        }

        void run() override
        {
            owner.runPluginScan();
        }

    private:
        MainComponent& owner;
    };

    std::unique_ptr<PluginScanThread> pluginScanThread;

    void startPluginScan();
    void runPluginScan();

    void addPlugin(const juce::PluginDescription&);
    void removeSelectedPlugin();

    void moveSelectedPluginUp();
    void moveSelectedPluginDown();

    void toggleSelectedBypass();
    void openSelectedPluginEditor();

    void refreshPluginBrowser();
    void addPluginFromBrowser(int row);

    juce::File getScannerDirectory() const;

    juce::File getScannerExecutable() const;

    juce::Array<juce::File> findVST3Bundles(
        const juce::File& root) const;

    bool scanSinglePluginOutOfProcess(
        const juce::File& pluginFile);

    void mergeScannerResult(
        const juce::File& resultXml);

    void saveFailedPluginLog();

    std::atomic<bool> scanCancelled { false };

    juce::CriticalSection pluginListLock;

    juce::StringArray failedPlugins;
    juce::CriticalSection failedPluginsLock;

    juce::Array<NodePtr> pluginChain;

    const juce::Array<NodePtr>& getPluginNodes() const
    {
        return pluginChain;
    }

    void saveChain();
    void loadChain();

    juce::String processorStateToBase64(juce::AudioProcessor& processor) const;
    void restoreProcessorState(juce::AudioProcessor& processor, const juce::String& base64);

    void saveChainToXml(juce::XmlElement& root) const;
    bool loadChainFromXml(
        juce::XmlElement& root,
        juce::uint32 sequenceNumber = 0);

    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPluginList;

    juce::File getPresetDirectory() const;

    juce::String activeVstPresetId;
    juce::String defaultVstPresetId;
    juce::String lastVstPresetId;

    juce::StringArray vstPresetNames;

    void refreshPresets();
    void saveCurrentPreset();
    void loadPreset(const juce::String& name);
    void deleteSelectedPreset();
    void selectPresetInCombo(const juce::String& name);

    std::atomic_bool isLoadingVstPreset { false };

    juce::uint32 presetLoadSequence = 0;

    int presetPluginsCreated = 0;

    class PresetLoadGuard
    {
    public:
        explicit PresetLoadGuard(
            MainComponent& ownerToUse)
            : owner(ownerToUse)
        {
            admitted =
                !owner.isLoadingVstPreset
                     .exchange(true);
        }

        ~PresetLoadGuard()
        {
            if (admitted)
                owner.isLoadingVstPreset.store(false);
        }

        bool isAdmitted() const
        {
            return admitted;
        }

        PresetLoadGuard(
            const PresetLoadGuard&) = delete;

        PresetLoadGuard& operator=(
            const PresetLoadGuard&) = delete;

    private:
        MainComponent& owner;
        bool admitted = false;
    };

    void appendPresetLog(
        const juce::String& line);

    void logDestroyPlugin(
        const NodePtr& node);

    void setMuted(bool muted);
    void setAutoStart(bool enabled);
    void toggleDefaultPreset();
    void updateDefaultPresetButton();
    void clearPluginCache();

    void saveState();
    void loadState();

    void writeStartupLog();

    juce::ApplicationProperties appProperties;

    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    std::atomic<bool> globalBypass { false };

    void setGlobalBypass(bool enabled);

    std::unique_ptr<VstPanel> vstPanel;

    bool isInitialised = false;

    bool presetDirty = false;

    void markPresetDirty();

    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;

    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    double inputMeterValue = 0.0;
    double outputMeterValue = 0.0;

    juce::ProgressBar inputMeter { inputMeterValue };
    juce::ProgressBar outputMeter { outputMeterValue };

    juce::ToggleButton limiterButton {
        "Safety limiter"
    };

    juce::TextButton resetAudioButton {
        "Reset Audio"
    };

    class PluginChainModel final : public juce::ListBoxModel
    {
    public:
        explicit PluginChainModel(MainComponent& owner);

        int getNumRows() override;

        void paintListBoxItem(
            int rowNumber,
            juce::Graphics& g,
            int width,
            int height,
            bool rowIsSelected) override;

        void listBoxItemDoubleClicked(
            int row,
            const juce::MouseEvent&) override;

    private:
        MainComponent& owner;
    };

    PluginChainModel chainModel;

    class PluginBrowserModel final : public juce::ListBoxModel
    {
    public:
        explicit PluginBrowserModel(MainComponent& ownerToUse)
            : owner(ownerToUse)
        {
        }

        int getNumRows() override;

        void paintListBoxItem(
            int row,
            juce::Graphics& g,
            int width,
            int height,
            bool selected) override;

        void listBoxItemDoubleClicked(
            int row,
            const juce::MouseEvent&) override;

    private:
        MainComponent& owner;
    };

    PluginBrowserModel pluginBrowserModel { *this };

    juce::Array<juce::PluginDescription> filteredPlugins;

    juce::Label statusLabel;

    void timerCallback() override;

    std::vector<std::unique_ptr<PluginWindow>> pluginWindows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
