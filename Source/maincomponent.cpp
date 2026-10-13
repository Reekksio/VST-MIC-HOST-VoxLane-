#include "MainComponent.h"

#include "VstPanel.h"

#include <cstdint>

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace
{

juce::String defaultMarkerText()
{
    return juce::String::fromUTF8(
        " \xe2\x98\x85");
}

juce::String defaultButtonCheckText()
{
    return juce::String::fromUTF8(
        "\xe2\x9c\x93");
}

}

static bool setWindowsAutoStart(bool enabled)
{
#if JUCE_WINDOWS

    HKEY key {};

    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_SET_VALUE,
            &key) != ERROR_SUCCESS)
    {
        return false;
    }

    const wchar_t* valueName =
        L"VSTMicHost";

    bool success = false;

    if (enabled)
    {
        const auto exe =
            juce::File::
                getSpecialLocation(
                    juce::File::
                        currentExecutableFile)
                    .getFullPathName();

        const juce::String command =
            "\"" + exe + "\" --tray";

        const auto* data =
            command.toWideCharPointer();

        const DWORD bytes =
            static_cast<DWORD>(
                (command.length() + 1)
                * sizeof(wchar_t));

        success =
            RegSetValueExW(
                key,
                valueName,
                0,
                REG_SZ,
                reinterpret_cast<
                    const BYTE*>(
                    data),
                bytes)
            == ERROR_SUCCESS;
    }
    else
    {
        success =
            RegDeleteValueW(
                key,
                valueName)
            == ERROR_SUCCESS;
    }

    RegCloseKey(key);

    return success;

#else

    return false;

#endif
}

static bool getWindowsAutoStart()
{
#if JUCE_WINDOWS

    HKEY key {};

    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_QUERY_VALUE,
            &key) != ERROR_SUCCESS)
    {
        return false;
    }

    wchar_t buffer[1024] {};

    DWORD size =
        sizeof(buffer);

    const wchar_t* valueName =
        L"VSTMicHost";

    const LONG result =
        RegQueryValueExW(
            key,
            valueName,
            nullptr,
            nullptr,
            reinterpret_cast<LPBYTE>(
                buffer),
            &size);

    RegCloseKey(key);

    return result == ERROR_SUCCESS;

#else

    return false;

#endif
}

MainComponent::PluginWindow::PluginWindow(NodePtr n)
    : DocumentWindow(
        n->getProcessor()->getName(),
        juce::Colours::black,
        DocumentWindow::closeButton),
      node(std::move(n))
{
    setUsingNativeTitleBar(true);

    auto* processor = node->getProcessor();

    juce::AudioProcessorEditor* editor = nullptr;

    if (processor->hasEditor())
        editor = processor->createEditor();

    if (editor == nullptr)
        editor = new juce::GenericAudioProcessorEditor(*processor);

    setContentOwned(editor, true);

    setResizable(true, false);

    centreWithSize(
        juce::jmax(500, editor->getWidth()),
        juce::jmax(300, editor->getHeight()));

    setVisible(true);
}

void MainComponent::PluginWindow::closeButtonPressed()
{
    setVisible(false);
}

MainComponent::PluginChainModel::PluginChainModel(MainComponent& o)
    : owner(o)
{
}

int MainComponent::PluginChainModel::getNumRows()
{
    return owner.getPluginNodes().size();
}

void MainComponent::PluginChainModel::paintListBoxItem(
    int row,
    juce::Graphics& g,
    int width,
    int height,
    bool selected)
{
    auto plugins = owner.getPluginNodes();

    if (!juce::isPositiveAndBelow(row, plugins.size()))
        return;

    if (selected)
        g.fillAll(juce::Colour(0xff343434));

    auto node = plugins[row];
    auto* processor = node->getProcessor();

    g.setColour(juce::Colours::white);

    g.setFont(16.0f);

    auto name = processor->getName();

    if (node->isBypassed())
        name += "  [BYPASS]";

    g.drawText(
        juce::String(row + 1) + ". " + name,
        12,
        0,
        width - 24,
        height,
        juce::Justification::centredLeft);
}

void MainComponent::PluginChainModel::listBoxItemDoubleClicked(
    int,
    const juce::MouseEvent&)
{
    owner.openSelectedPluginEditor();
}

int MainComponent::PluginBrowserModel::getNumRows()
{
    return owner.filteredPlugins.size();
}

void MainComponent::PluginBrowserModel::paintListBoxItem(
    int row,
    juce::Graphics& g,
    int width,
    int height,
    bool selected)
{
    if (!juce::isPositiveAndBelow(
            row,
            owner.filteredPlugins.size()))
        return;

    const auto& plugin =
        owner.filteredPlugins.getReference(row);

    if (selected)
    {
        g.setColour(
            juce::Colour(0xff343434));

        g.fillRoundedRectangle(
            2.0f,
            2.0f,
            static_cast<float>(width - 4),
            static_cast<float>(height - 4),
            5.0f);
    }

    g.setColour(
        juce::Colours::white);

    g.setFont(15.0f);

    g.drawText(
        plugin.name,
        10,
        3,
        width - 52,
        20,
        juce::Justification::centredLeft);

    g.setColour(
        juce::Colours::grey);

    g.setFont(12.0f);

    juce::String info;

    if (plugin.manufacturerName.isNotEmpty())
        info << plugin.manufacturerName;

    if (plugin.category.isNotEmpty())
    {
        if (info.isNotEmpty())
            info << "  •  ";

        info << plugin.category;
    }

    g.drawText(
        info,
        10,
        23,
        width - 52,
        height - 25,
        juce::Justification::centredLeft);
}

void MainComponent::PluginBrowserModel::listBoxItemDoubleClicked(
    int row,
    const juce::MouseEvent&)
{
    owner.addPluginFromBrowser(row);
}

MainComponent::MainComponent()
    : chainModel(*this)
{
    setWantsKeyboardFocus(true);

    appendPresetLog(
        "--- VSTMicHost startup ---");
    appendPresetLog(
        juce::String::formatted(
            "JUCE_VERSION=%d.%d.%d",
            JUCE_MAJOR_VERSION,
            JUCE_MINOR_VERSION,
            JUCE_BUILDNUMBER));

#ifndef VSTMIC_JUCE_COMMIT
#error "VSTMIC_JUCE_COMMIT expected from CMake"
#endif

    appendPresetLog(
        juce::String(
            "JUCE_COMMIT=")
        + VSTMIC_JUCE_COMMIT);

#if VSTMIC_JUCE_PATCH_PRESENT
    appendPresetLog(
        "WAVES_HOST_CONTEXT_FIX=true");
#else
    appendPresetLog(
        "WAVES_HOST_CONTEXT_FIX=false");
#endif

    juce::PropertiesFile::Options options;

    options.applicationName = "VSTMicHost";

    options.folderName = "VSTMicHost";

    appProperties.setStorageParameters(options);

    pluginFormatManager.addDefaultFormats();

    initialiseAudio();

    deviceManager.addChangeListener(
        &deviceChangeListener);

    deviceSelector =
        std::make_unique<juce::AudioDeviceSelectorComponent>(
            deviceManager,

            1,
            1,

            2,
            2,

            false,
            false,
            false,
            false);

    addAndMakeVisible(*deviceSelector);

    inputGainLabel.setText(
        "Input Gain",
        juce::dontSendNotification);

    addAndMakeVisible(inputGainLabel);

    inputGainSlider.setRange(
        -30.0,
        18.0,
        0.1);

    inputGainSlider.setValue(
        0.0,
        juce::dontSendNotification);

    inputGainSlider.setSliderStyle(
        juce::Slider::
            LinearHorizontal);

    inputGainSlider.setTextBoxStyle(
        juce::Slider::
            TextBoxRight,
        false,
        70,
        24);

    inputGainSlider.setTextValueSuffix(
        " dB");

    inputGainSlider.onValueChange =
        [this]
        {
            if (inputUtility != nullptr)
            {
                inputUtility->setGainDb(
                    static_cast<float>(
                        inputGainSlider
                            .getValue()));
            }

            markPresetDirty();
        };

    addAndMakeVisible(
        inputGainSlider);

    outputGainLabel.setText(
        "Output Gain",
        juce::dontSendNotification);

    addAndMakeVisible(
        outputGainLabel);

    outputGainSlider.setRange(
        -30.0,
        18.0,
        0.1);

    outputGainSlider.setValue(
        0.0,
        juce::dontSendNotification);

    outputGainSlider.setSliderStyle(
        juce::Slider::
            LinearHorizontal);

    outputGainSlider.setTextBoxStyle(
        juce::Slider::
            TextBoxRight,
        false,
        70,
        24);

    outputGainSlider.setTextValueSuffix(
        " dB");

    outputGainSlider.onValueChange =
        [this]
        {
            if (outputUtility != nullptr)
            {
                outputUtility->setGainDb(
                    static_cast<float>(
                        outputGainSlider
                            .getValue()));
            }

            markPresetDirty();
        };

    addAndMakeVisible(
        outputGainSlider);

    inputMeterLabel.setText(
        "Input",
        juce::dontSendNotification);

    outputMeterLabel.setText(
        "Output",
        juce::dontSendNotification);

    addAndMakeVisible(
        inputMeterLabel);

    addAndMakeVisible(
        outputMeterLabel);

    addAndMakeVisible(
        inputMeter);

    addAndMakeVisible(
        outputMeter);

    limiterButton.setToggleState(
        true,
        juce::dontSendNotification);

    limiterButton.onClick =
        [this]
        {
            if (outputUtility != nullptr)
            {
                outputUtility
                    ->setLimiterEnabled(
                        limiterButton
                            .getToggleState());
            }

            markPresetDirty();

            saveState();
        };

    addAndMakeVisible(
        limiterButton);

    resetAudioButton.onClick =
        [this]
        {
            statusLabel.setText(
                "Restarting audio...",
                juce::dontSendNotification);

            deviceManager
                .removeAudioCallback(
                    &processorPlayer);

            processorPlayer
                .setProcessor(nullptr);

            processorPlayer
                .setProcessor(&graph);

            deviceManager
                .addAudioCallback(
                    &processorPlayer);

            rebuildGraph();

            statusLabel.setText(
                "Audio restarted",
                juce::dontSendNotification);
        };

    addAndMakeVisible(
        resetAudioButton);

    vstPanel =
        std::make_unique<VstPanel>(*this);

    addAndMakeVisible(*vstPanel);

    statusLabel.setText(
        "Ready",
        juce::dontSendNotification);

    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::lightgrey);

    addAndMakeVisible(statusLabel);

    loadState();

    refreshPluginBrowser();
    refreshPresets();

    if (defaultVstPresetId.isNotEmpty())
    {
        if (vstPresetNames.contains(defaultVstPresetId))
        {
            loadPreset(defaultVstPresetId);
        }
        else
        {
            defaultVstPresetId.clear();
            saveState();
        }
    }

    updateDefaultPresetButton();

    writeStartupLog();

    startTimerHz(5);

    isInitialised = true;

    setSize(1100, 720);
}

MainComponent::~MainComponent()
{
    stopTimer();

    if (pluginScanThread != nullptr)
    {
        scanCancelled.store(true);

        pluginScanThread->stopThread(30000);
        pluginScanThread.reset();
    }

    saveChain();
    saveState();

    pluginWindows.clear();

    deviceManager.removeChangeListener(
        &deviceChangeListener);

    deviceManager.removeAudioCallback(&processorPlayer);

    processorPlayer.setProcessor(nullptr);
}

void MainComponent::initialiseAudio()
{
    auto error =
        deviceManager.initialise(
            1,
            2,
            nullptr,
            true);

    if (error.isNotEmpty())
        juce::Logger::writeToLog(error);

    processorPlayer.setProcessor(&graph);

    deviceManager.addAudioCallback(&processorPlayer);

    createGraphIO();
}

void MainComponent::createGraphIO()
{
    graph.clear();

    pluginChain.clear();

    inputUtility = nullptr;
    outputUtility = nullptr;

    audioInputNode =
        graph.addNode(
            std::make_unique<
                juce::AudioProcessorGraph::
                    AudioGraphIOProcessor>(
                juce::AudioProcessorGraph::
                    AudioGraphIOProcessor::
                        audioInputNode));

    auto inputProcessor =
        std::make_unique<
            AudioUtilityProcessor>(
                AudioUtilityProcessor::
                    Mode::Input);

    inputUtility =
        inputProcessor.get();

    inputUtilityNode =
        graph.addNode(
            std::move(
                inputProcessor));

    auto outputProcessor =
        std::make_unique<
            AudioUtilityProcessor>(
                AudioUtilityProcessor::
                    Mode::Output);

    outputUtility =
        outputProcessor.get();

    outputUtilityNode =
        graph.addNode(
            std::move(
                outputProcessor));

    audioOutputNode =
        graph.addNode(
            std::make_unique<
                juce::AudioProcessorGraph::
                    AudioGraphIOProcessor>(
                juce::AudioProcessorGraph::
                    AudioGraphIOProcessor::
                        audioOutputNode));

    rebuildGraph();
}

void MainComponent::rebuildGraph()
{
    for (const auto& connection : graph.getConnections())
        graph.removeConnection(connection);
    if (!audioInputNode || !audioOutputNode || !inputUtilityNode || !outputUtilityNode)
        return;
    graph.addConnection({{audioInputNode->nodeID, 0}, {inputUtilityNode->nodeID, 0}});
    graph.addConnection({{audioInputNode->nodeID, 0}, {inputUtilityNode->nodeID, 1}});
    NodePtr previous = inputUtilityNode;
    if (!globalBypass.load())
    {
        for (const auto& plugin : pluginChain)
        {
            if (!plugin || !plugin->getProcessor()) continue;
            auto* processor = plugin->getProcessor();
            if (processor->getTotalNumInputChannels() < 1 || processor->getTotalNumOutputChannels() < 1) continue;
            graph.addConnection({{previous->nodeID, 0}, {plugin->nodeID, 0}});
            if (processor->getTotalNumInputChannels() >= 2)
                graph.addConnection({{previous->nodeID, previous->getProcessor()->getTotalNumOutputChannels() >= 2 ? 1 : 0}, {plugin->nodeID, 1}});
            previous = plugin;
        }
    }
    graph.addConnection({{previous->nodeID, 0}, {outputUtilityNode->nodeID, 0}});
    graph.addConnection({{previous->nodeID, previous->getProcessor()->getTotalNumOutputChannels() >= 2 ? 1 : 0}, {outputUtilityNode->nodeID, 1}});
    graph.addConnection({{outputUtilityNode->nodeID, 0}, {audioOutputNode->nodeID, 0}});
    graph.addConnection({{outputUtilityNode->nodeID, 1}, {audioOutputNode->nodeID, 1}});
}

void MainComponent::handleAudioDeviceChanged()
{
    auto* device =
        deviceManager
            .getCurrentAudioDevice();

    if (device == nullptr)
        return;

    const double sampleRate =
        device->getCurrentSampleRate();

    const int bufferSize =
        device
            ->getCurrentBufferSizeSamples();

    for (auto& node : pluginChain)
    {
        if (node == nullptr)
            continue;

        auto* processor =
            node->getProcessor();

        if (processor == nullptr)
            continue;

        processor->setRateAndBufferSizeDetails(
            sampleRate,
            bufferSize);
    }

    rebuildGraph();

    saveState();

    if (!reconnectInProgress.load())
        rememberCurrentDevices();

    statusLabel.setText(
        "Audio device changed",
        juce::dontSendNotification);
}

void MainComponent::rememberCurrentDevices()
{
    auto setup =
        deviceManager.getAudioDeviceSetup();

    if (setup.inputDeviceName.isNotEmpty())
    {
        preferredInputDevice =
            setup.inputDeviceName;
    }

    if (setup.outputDeviceName.isNotEmpty())
    {
        preferredOutputDevice =
            setup.outputDeviceName;
    }

    auto* settings =
        appProperties.getUserSettings();

    if (settings == nullptr)
        return;

    settings->setValue(
        "preferredInput",
        preferredInputDevice);

    settings->setValue(
        "preferredOutput",
        preferredOutputDevice);

    settings->saveIfNeeded();
}

void MainComponent::tryReconnectAudioDevices()
{
    if (!autoReconnectEnabled)
        return;

    if (preferredInputDevice.isEmpty() &&
        preferredOutputDevice.isEmpty())
        return;

    auto setup =
        deviceManager.getAudioDeviceSetup();

    bool needsUpdate = false;

    if (preferredInputDevice.isNotEmpty() &&
        setup.inputDeviceName !=
            preferredInputDevice)
    {
        setup.inputDeviceName =
            preferredInputDevice;

        needsUpdate = true;
    }

    if (preferredOutputDevice.isNotEmpty() &&
        setup.outputDeviceName !=
            preferredOutputDevice)
    {
        setup.outputDeviceName =
            preferredOutputDevice;

        needsUpdate = true;
    }

    if (!needsUpdate)
        return;

    reconnectInProgress.store(true);

    const auto result =
        deviceManager.setAudioDeviceSetup(
            setup,
            true);

    reconnectInProgress.store(false);

    if (result.isEmpty())
    {
        statusLabel.setText(
            "Audio devices reconnected",
            juce::dontSendNotification);
    }
}

juce::String MainComponent::processorStateToBase64(
    juce::AudioProcessor& processor) const
{
    juce::MemoryBlock data;

    processor.getStateInformation(data);

    return data.toBase64Encoding();
}

void MainComponent::restoreProcessorState(
    juce::AudioProcessor& processor,
    const juce::String& base64)
{
    if (base64.isEmpty())
        return;

    juce::MemoryBlock data;

    if (!data.fromBase64Encoding(base64))
        return;

    processor.setStateInformation(
        data.getData(),
        static_cast<int>(
            data.getSize()));
}

void MainComponent::saveChainToXml(
    juce::XmlElement& root) const
{
    root.setAttribute(
        "inputGain",
        inputGainSlider.getValue());

    root.setAttribute(
        "outputGain",
        outputGainSlider.getValue());

    root.setAttribute(
        "limiter",
        limiterButton.getToggleState());

    for (auto& node : pluginChain)
    {
        if (node == nullptr)
            continue;

        auto* processor =
            node->getProcessor();

        if (processor == nullptr)
            continue;

        auto* instance =
            dynamic_cast<
                juce::AudioPluginInstance*>(
                    processor);

        if (instance == nullptr)
            continue;

        juce::PluginDescription desc;

        instance->fillInPluginDescription(
            desc);

        auto* pluginXml =
            root.createNewChildElement(
                "PLUGIN");

        pluginXml->setAttribute(
            "name",
            desc.name);

        pluginXml->setAttribute(
            "manufacturer",
            desc.manufacturerName);

        pluginXml->setAttribute(
            "format",
            desc.pluginFormatName);

        pluginXml->setAttribute(
            "file",
            desc.fileOrIdentifier);

        pluginXml->setAttribute(
            "uid",
            desc.uniqueId);

        pluginXml->setAttribute(
            "identifier",
            desc.createIdentifierString());

        pluginXml->setAttribute(
            "bypass",
            node->isBypassed());

        pluginXml->setAttribute(
            "state",
            processorStateToBase64(
                *processor));
    }
}

bool MainComponent::loadChainFromXml(
    juce::XmlElement& root,
    juce::uint32 sequenceNumber)
{
    presetPluginsCreated = 0;

    for (auto& node : pluginChain)
    {
        if (node != nullptr)
        {
            logDestroyPlugin(node);

            graph.removeNode(
                node->nodeID);
        }
    }

    pluginChain.clear();

    auto* device =
        deviceManager
            .getCurrentAudioDevice();

    double sampleRate = 48000.0;
    int blockSize = 256;

    if (device != nullptr)
    {
        sampleRate =
            device->getCurrentSampleRate();

        blockSize =
            device->getCurrentBufferSizeSamples();
    }

    for (auto* pluginXml =
             root.getFirstChildElement();
         pluginXml != nullptr;
         pluginXml = pluginXml->getNextElement())
    {

        if (!pluginXml->hasTagName(
                "PLUGIN"))
            continue;

        const auto identifier =
            pluginXml->getStringAttribute(
                "identifier");

        const auto& availablePlugins =
            knownPluginList.getTypes();

        const juce::PluginDescription*
            matchingDescription = nullptr;

        for (const auto& desc :
             availablePlugins)
        {
            if (desc.createIdentifierString()
                == identifier)
            {
                matchingDescription =
                    &desc;

                break;
            }
        }

        if (matchingDescription == nullptr)
            continue;

        juce::String error;

        appendPresetLog("WAVES_CREATE");
        appendPresetLog(
            "description.name="
            + matchingDescription->name);
        appendPresetLog(
            "description.descriptiveName="
            + matchingDescription
                  ->descriptiveName);
        appendPresetLog(
            "manufacturer="
            + matchingDescription
                  ->manufacturerName);
        appendPresetLog(
            juce::String::formatted(
                "uid=%d",
                matchingDescription->uniqueId));
        appendPresetLog(
            "fileOrIdentifier="
            + matchingDescription->fileOrIdentifier);
        appendPresetLog(
            "pluginFormatName="
            + matchingDescription
                  ->pluginFormatName);

        appendPresetLog("CREATE_PLUGIN BEGIN");
        appendPresetLog(
            juce::String::formatted(
                "loadSequence=%llu",
                (unsigned long long)
                    sequenceNumber));
        appendPresetLog(
            juce::String(
                "pluginIndex=")
            + juce::String(
                (int)(pluginChain.size() + 1)));
        appendPresetLog(
            "name="
            + matchingDescription->name);
        appendPresetLog(
            "manufacturer="
            + matchingDescription
                  ->manufacturerName);
        appendPresetLog(
            "format="
            + matchingDescription
                  ->pluginFormatName);
        appendPresetLog(
            "fileOrIdentifier="
            + matchingDescription->fileOrIdentifier);
        appendPresetLog(
            juce::String::formatted(
                "uid=%d",
                matchingDescription->uniqueId));
        appendPresetLog(
            "identifier="
            + identifier);

        const auto createStart =
            juce::Time::getMillisecondCounterHiRes();

        auto instance =
            pluginFormatManager
                .createPluginInstance(
                    *matchingDescription,
                    sampleRate,
                    blockSize,
                    error);

        const auto createEnd =
            juce::Time::getMillisecondCounterHiRes();

        if (instance == nullptr)
        {
            appendPresetLog(
                "CREATE_PLUGIN END");
            appendPresetLog(
                "success=false");
            appendPresetLog(
                juce::String::formatted(
                    "elapsedMs=%.3f",
                    createEnd - createStart));
            appendPresetLog(
                "error="
                + error);
            continue;
        }

        appendPresetLog(
            "CREATE_PLUGIN END");
        appendPresetLog(
            "success=true");
        appendPresetLog(
            juce::String::formatted(
                "elapsedMs=%.3f",
                createEnd - createStart));

        appendPresetLog(
            "CREATED_INSTANCE");
        appendPresetLog(
            "instanceName="
            + instance->getName());
        appendPresetLog(
            "instanceManufacturer="
            + instance->getPluginDescription()
                  .manufacturerName);

        ++presetPluginsCreated;

        restoreProcessorState(
            *instance,
            pluginXml->getStringAttribute(
                "state"));

        auto node =
            graph.addNode(
                std::move(instance));

        if (node == nullptr)
            continue;

        node->setBypassed(
            pluginXml->getBoolAttribute(
                "bypass"));

        pluginChain.add(node);
    }

    const auto inputGain =
        root.getDoubleAttribute(
            "inputGain",
            0.0);

    const auto outputGain =
        root.getDoubleAttribute(
            "outputGain",
            0.0);

    const auto limiter =
        root.getBoolAttribute(
            "limiter",
            true);

    inputGainSlider.setValue(
        inputGain,
        juce::dontSendNotification);

    outputGainSlider.setValue(
        outputGain,
        juce::dontSendNotification);

    limiterButton.setToggleState(
        limiter,
        juce::dontSendNotification);

    if (inputUtility != nullptr)
    {
        inputUtility->setGainDb(
            static_cast<float>(
                inputGain));
    }

    if (outputUtility != nullptr)
    {
        outputUtility->setGainDb(
            static_cast<float>(
                outputGain));

        outputUtility->setLimiterEnabled(
            limiter);
    }

    return !pluginChain.isEmpty();
}

void MainComponent::saveChain()
{
    auto* settings =
        appProperties.getUserSettings();

    if (settings == nullptr)
        return;

    auto root =
        std::make_unique<juce::XmlElement>(
            "PLUGIN_CHAIN");

    saveChainToXml(*root);

    settings->setValue(
        "pluginChain",
        root->toString());

    settings->saveIfNeeded();
}

void MainComponent::loadChain()
{
    auto* settings =
        appProperties.getUserSettings();

    if (settings == nullptr)
        return;

    const auto saved =
        settings->getValue(
            "pluginChain");

    if (saved.isEmpty())
        return;

    auto xml =
        juce::parseXML(saved);

    if (xml == nullptr)
        return;

    if (!xml->hasTagName(
            "PLUGIN_CHAIN"))
        return;

    loadChainFromXml(*xml);

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();
}

juce::File MainComponent::getPresetDirectory() const
{
    auto dir =
        juce::File::getSpecialLocation(
            juce::File::
                userApplicationDataDirectory)
            .getChildFile("VSTMicHost")
            .getChildFile("presets");

    dir.createDirectory();

    return dir;
}

void MainComponent::refreshPresets()
{
    vstPresetNames.clear();

    juce::DirectoryIterator iterator(
        getPresetDirectory(),
        false,
        "*.xml",
        juce::File::findFiles);

    while (iterator.next())
    {
        vstPresetNames.add(
            iterator.getFile()
                .getFileNameWithoutExtension());
    }

    std::sort(
        vstPresetNames.begin(),
        vstPresetNames.end(),
        [](const juce::String& a,
           const juce::String& b)
        {
            return a.compareIgnoreCase(b) < 0;
        });

    vstPresetNames.trim();

    vstPanel->presetCombo.clear(
        juce::dontSendNotification);

    for (int i = 0;
         i < vstPresetNames.size();
         ++i)
    {
        const auto& name =
            vstPresetNames[i];

        vstPanel->presetCombo.addItem(
            name
                + (name == defaultVstPresetId
                       ? defaultMarkerText()
                       : juce::String()),
            i + 1);
    }

    if (vstPresetNames.isEmpty())
    {
        activeVstPresetId.clear();
        return;
    }

    selectPresetInCombo(activeVstPresetId);
}

void MainComponent::selectPresetInCombo(
    const juce::String& name)
{
    const int index =
        vstPresetNames.indexOf(
            name,
            true);

    if (index >= 0)
    {
        vstPanel->presetCombo.setSelectedId(
            index + 1,
            juce::dontSendNotification);
    }
    else
    {
        vstPanel->presetCombo.setSelectedId(
            0,
            juce::dontSendNotification);
    }
}

void MainComponent::saveCurrentPreset()
{
    auto* window =
        new juce::AlertWindow(
            "Save preset",
            "Preset name:",
            juce::MessageBoxIconType::NoIcon);

    window->addTextEditor(
        "name",
        presetDirty ? activeVstPresetId : juce::String(),
        "",
        false);

    window->addButton(
        "Save",
        1,
        juce::KeyPress(
            juce::KeyPress::returnKey));

    window->addButton(
        "Cancel",
        0,
        juce::KeyPress(
            juce::KeyPress::escapeKey));

    window->enterModalState(
        true,
        juce::ModalCallbackFunction::create(
            [this, window](int result)
            {
                const auto name =
                    window->getTextEditorContents(
                        "name").trim();

                delete window;

                if (result != 1 ||
                    !name.isNotEmpty())
                    return;

                const auto fileName =
                    juce::File::createLegalFileName(
                        name);

                auto file =
                    getPresetDirectory()
                        .getChildFile(fileName)
                        .withFileExtension(".xml");

                auto root =
                    std::make_unique<
                        juce::XmlElement>(
                        "PLUGIN_CHAIN");

                saveChainToXml(*root);

                if (!root->writeTo(file))
                {
                    statusLabel.setText(
                        "Failed to save preset",
                        juce::dontSendNotification);

                    return;
                }

                activeVstPresetId = fileName;
                presetDirty = false;

                refreshPresets();
                selectPresetInCombo(activeVstPresetId);

                updateDefaultPresetButton();

                saveState();
            }));
}

void MainComponent::loadPreset(
    const juce::String& name)
{
    PresetLoadGuard guard(*this);

    if (!guard.isAdmitted())
    {
        appendPresetLog(
            "LOAD_PRESET BLOCKED: re-entry");
        return;
    }

    const juce::uint32 sequence =
        ++presetLoadSequence;

    const double loadStart =
        juce::Time::getMillisecondCounterHiRes();

    const auto finishLog =
        [this, &loadStart]()
    {
        appendPresetLog(
            "LOAD_PRESET END");
        appendPresetLog(
            juce::String::formatted(
                "elapsedMs=%.3f",
                juce::Time::getMillisecondCounterHiRes()
                    - loadStart));
        appendPresetLog(
            juce::String::formatted(
                "pluginsCreated=%d",
                presetPluginsCreated));
    };

    appendPresetLog(
        "LOAD_PRESET BEGIN");
    appendPresetLog(
        juce::String::formatted(
            "loadSequence=%llu",
            (unsigned long long)
                sequence));
    appendPresetLog(
        juce::String::formatted(
            "threadId=%llu",
            (unsigned long long)
                (std::uintptr_t)
                    juce::Thread::getCurrentThreadId()));
    appendPresetLog(
        "presetId="
        + name);

    const auto file =
        getPresetDirectory()
            .getChildFile(name)
            .withFileExtension(".xml");

    appendPresetLog(
        "presetPath="
        + file.getFullPathName());

    auto xml =
        juce::parseXML(file);

    if (xml == nullptr)
    {
        appendPresetLog(
            "parseResult=failed");

        statusLabel.setText(
            "Could not load preset",
            juce::dontSendNotification);

        finishLog();

        return;
    }

    appendPresetLog(
        "parseResult=ok");

    if (loadChainFromXml(
            *xml,
            sequence))
    {
        lastVstPresetId = activeVstPresetId;

        activeVstPresetId = name;
        presetDirty = false;

        refreshPresets();
        selectPresetInCombo(activeVstPresetId);

        updateDefaultPresetButton();

        saveChain();
        saveState();

        statusLabel.setText(
            "Preset loaded: " + name,
            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText(
            "Preset could not be applied",
            juce::dontSendNotification);
    }

    finishLog();

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();
}

void MainComponent::appendPresetLog(
    const juce::String& line)
{
#if JUCE_WINDOWS
    auto dir =
        juce::File::getSpecialLocation(
            juce::File::
                userApplicationDataDirectory)
            .getChildFile("VSTMicHost");

    if (!dir.exists())
        dir.createDirectory();

    auto file =
        dir.getChildFile(
            "preset_load_log.txt");

    const auto timeLine =
        juce::Time::getCurrentTime()
            .formatted(
                "%Y-%m-%d %H:%M:%S");

    file.appendText(
        timeLine + " | " + line + "\n");
#else
    (void)line;
#endif
}

void MainComponent::logDestroyPlugin(
    const NodePtr& node)
{
    if (node == nullptr)
        return;

    auto* processor =
        node->getProcessor();

    appendPresetLog(
        "DESTROY_PLUGIN");
    appendPresetLog(
        "name="
        + juce::String(
            processor != nullptr
                ? processor->getName()
                : juce::String()));
    appendPresetLog(
        juce::String::formatted(
            "nodeId=%u",
            node->nodeID.uid));
}

void MainComponent::deleteSelectedPreset()
{
    const auto name =
        activeVstPresetId;

    if (!name.isNotEmpty())
        return;

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::
            QuestionIcon,

        "Delete preset",

        "Usun preset \"" +
            name + "\"?",

        "Delete",
        "Cancel",

        nullptr,

        juce::ModalCallbackFunction::
            create(
                [this, name](int result)
                {
                    if (result == 0)
                        return;

                    const auto file =
                        getPresetDirectory()
                            .getChildFile(name)
                            .withFileExtension(".xml");

                    if (file.existsAsFile())
                        file.deleteFile();

                    if (activeVstPresetId == name)
                    {
                        activeVstPresetId.clear();
                        presetDirty = false;
                    }

                    refreshPresets();

                    updateDefaultPresetButton();
                }));
}

void MainComponent::markPresetDirty()
{
    presetDirty = true;

    const int index =
        vstPresetNames.indexOf(
            activeVstPresetId,
            true);

    if (index >= 0)
    {
        const auto& presetName =
            vstPresetNames[index];

        vstPanel->presetCombo.changeItemText(
            index + 1,
            presetName
                + (presetName
                           == defaultVstPresetId
                       ? defaultMarkerText()
                       : juce::String())
                + " *");
    }
    else
    {
        vstPanel->presetCombo.setText(
            "Unsaved *",
            juce::dontSendNotification);
    }
}

void MainComponent::setMuted(bool muted)
{
    if (outputUtility != nullptr)
        outputUtility->setMuted(muted);

    if (vstPanel != nullptr)
    {
        vstPanel->muteButton.setToggleState(
            muted,
            juce::dontSendNotification);
    }

    saveState();
}

void MainComponent::setAutoStart(bool enabled)
{
    setWindowsAutoStart(enabled);

    saveState();
}

void MainComponent::toggleDefaultPreset()
{
    if (!activeVstPresetId.isNotEmpty())
        return;

    if (defaultVstPresetId == activeVstPresetId)
        defaultVstPresetId.clear();
    else
        defaultVstPresetId = activeVstPresetId;

    refreshPresets();

    updateDefaultPresetButton();

    saveState();
}

void MainComponent::updateDefaultPresetButton()
{
    if (vstPanel == nullptr)
        return;

    const bool hasActive =
        activeVstPresetId.isNotEmpty();

    auto& button =
        vstPanel->defaultPresetButton;

    button.setButtonText(
        hasActive
            && defaultVstPresetId
                   == activeVstPresetId
            ? "Default "
                  + defaultButtonCheckText()
            : "Set as Default");

    button.setEnabled(hasActive);

    button.setTooltip(
        "Klik \"Default "
            + defaultButtonCheckText()
            + "\" ponownie, "
        "aby usunac Default (Remove Default).");
}

void MainComponent::clearPluginCache()
{
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::
            QuestionIcon,

        "Clear plugin cache",

        "Cached VST3 list will be removed.\n"
        "Plugins will need a rescan.",

        "Clear",
        "Cancel",

        nullptr,

        juce::ModalCallbackFunction::
            create(
                [this](int result)
                {
                    if (result == 0)
                        return;

                    knownPluginList.clear();

                    filteredPlugins.clear();

                    refreshPluginBrowser();

                    auto* settings =
                        appProperties
                            .getUserSettings();

                    if (settings != nullptr)
                    {
                        settings
                            ->removeValue(
                                "knownPlugins");

                        settings
                            ->saveIfNeeded();
                    }

                    statusLabel.setText(
                        "Plugin cache cleared",
                        juce::dontSendNotification);
                }));
}

juce::File MainComponent::getScannerDirectory() const
{

    auto dir =
        juce::File::getSpecialLocation(
            juce::File::
                userApplicationDataDirectory)
            .getChildFile(
                "VSTMicHost");

    dir.createDirectory();

    return dir;
}

juce::File MainComponent::getScannerExecutable() const
{
    auto app =
        juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);

    return app
        .getParentDirectory()
        .getChildFile(
            "VSTScanner.exe");
}

juce::Array<juce::File>
MainComponent::findVST3Bundles(
    const juce::File& root) const
{
    juce::Array<juce::File> result;

    if (!root.exists())
        return result;

    juce::Array<juce::File> directories;

    directories.add(root);

    while (!directories.isEmpty())
    {
        auto current =
            directories.removeAndReturn(
                directories.size() - 1);

        juce::DirectoryIterator iterator(
            current,
            false,
            "*",
            juce::File::findFilesAndDirectories);

        while (iterator.next())
        {
            auto file =
                iterator.getFile();

            if (file.isDirectory())
            {
                directories.add(file);

                continue;
            }

            if (file.hasFileExtension("vst3"))
                result.add(file);
        }
    }

    return result;
}

bool MainComponent::scanSinglePluginOutOfProcess(
    const juce::File& pluginFile)
{
    const auto scanner =
        getScannerExecutable();

    if (!scanner.existsAsFile())
        return false;

    auto resultsDir =
        getScannerDirectory()
            .getChildFile("results");

    resultsDir.createDirectory();

    const auto safeName =
        juce::String::toHexString(
            pluginFile
                .getFullPathName()
                .hashCode64());

    auto resultFile =
        resultsDir.getChildFile(
            safeName + ".xml");

    resultFile.deleteFile();

    juce::StringArray arguments;

    arguments.add(
        scanner.getFullPathName());

    arguments.add(
        pluginFile.getFullPathName());

    arguments.add(
        resultFile.getFullPathName());

    juce::ChildProcess process;

    if (!process.start(arguments))
        return false;

    const auto startTime =
        juce::Time::
            getMillisecondCounterHiRes();

    constexpr double timeoutMs =
        240000.0;

    while (process.isRunning())
    {
        if (scanCancelled.load() ||
            juce::Thread::
                currentThreadShouldExit())
        {
            process.kill();

            return false;
        }

        const auto elapsed =
            juce::Time::
                getMillisecondCounterHiRes()
            - startTime;

        if (elapsed >= timeoutMs)
        {
            process.kill();

            return false;
        }

        juce::Thread::sleep(50);
    }

    if (process.getExitCode() != 0)
        return false;

    if (!resultFile.existsAsFile())
        return false;

    mergeScannerResult(resultFile);

    return true;
}

void MainComponent::mergeScannerResult(
    const juce::File& resultXml)
{
    auto xml =
        juce::XmlDocument::parse(
            resultXml);

    if (xml == nullptr)
        return;

    juce::KnownPluginList temporaryList;

    temporaryList.recreateFromXml(
        *xml);

    const juce::ScopedLock lock(
        pluginListLock);

    for (const auto& description :
         temporaryList.getTypes())
    {
        const auto identifier =
            description
                .createIdentifierString();

        bool alreadyKnown = false;

        for (const auto& existing :
             knownPluginList.getTypes())
        {
            if (existing
                    .createIdentifierString()
                == identifier)
            {
                alreadyKnown = true;

                break;
            }
        }

        if (!alreadyKnown)
            knownPluginList.addType(
                description);
    }
}

void MainComponent::saveFailedPluginLog()
{
    const juce::ScopedLock lock(
        failedPluginsLock);

    auto file =
        getScannerDirectory()
            .getChildFile(
                "failed_plugins.txt");

    juce::String text;

    for (const auto& path :
         failedPlugins)
    {
        text << path << "\n";
    }

    file.replaceWithText(text);
}

void MainComponent::startPluginScan()
{
    const auto scanner =
        getScannerExecutable();

    if (!scanner.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Scanner missing",
            "Nie znaleziono VSTScanner.exe obok VSTMicHost.exe.");

        return;
    }

    if (pluginScanThread != nullptr)
    {
        if (pluginScanThread->isThreadRunning())
            return;

        pluginScanThread.reset();
    }

    scanCancelled.store(false);

    vstPanel->scanButton.setEnabled(false);
    vstPanel->cancelScanButton.setEnabled(true);

    statusLabel.setText(
        "Scanning...",
        juce::dontSendNotification);

    pluginScanThread =
        std::make_unique<
            PluginScanThread>(*this);

    pluginScanThread->startThread();
}

void MainComponent::runPluginScan()
{
    {
        const juce::ScopedLock lock(
            failedPluginsLock);

        failedPlugins.clear();
    }

    juce::Array<juce::File> plugins;

#if JUCE_WINDOWS

    plugins.addArray(
        findVST3Bundles(
            juce::File(
                "C:\\Program Files\\Common Files\\VST3")));

#endif

    const int total =
        plugins.size();

    int processed = 0;
    int failed = 0;

    for (const auto& plugin : plugins)
    {
        if (scanCancelled.load() ||
            juce::Thread::currentThreadShouldExit())
            break;

        const auto text =
            "Scanning " +
            juce::String(processed + 1) +
            "/" +
            juce::String(total) +
            " - " +
            plugin.getFileName();

        juce::MessageManager::callAsync(
            [this, text]
            {
                statusLabel.setText(
                    text,
                    juce::dontSendNotification);
            });

        if (!scanSinglePluginOutOfProcess(
                plugin))
        {
            ++failed;

            const juce::ScopedLock lock(
                failedPluginsLock);

            failedPlugins.addIfNotAlreadyThere(
                plugin.getFullPathName());
        }

        ++processed;
    }

    saveFailedPluginLog();

    juce::MessageManager::callAsync(
        [this, failed]
        {
            refreshPluginBrowser();

            saveState();

            vstPanel->scanButton.setEnabled(true);
            vstPanel->cancelScanButton.setEnabled(false);

            if (scanCancelled.load())
            {
                statusLabel.setText(
                    "Scan cancelled",
                    juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText(
                    "Scan complete: " +
                        juce::String(
                            knownPluginList
                                .getNumTypes()) +
                        " plugins, " +
                        juce::String(failed) +
                        " failed",
                    juce::dontSendNotification);
            }
        });
}

void MainComponent::addPlugin(
    const juce::PluginDescription& description)
{
    if (description.isInstrument)
    {
        juce::AlertWindow::
            showMessageBoxAsync(
                juce::MessageBoxIconType::
                    WarningIcon,

                "Instrument plugin",

                description.name +
                " jest instrumentem VST3.\n\n"
                "VST Mic Host jest przeznaczony "
                "dla efektów audio.");

        return;
    }

    auto* device =
        deviceManager.getCurrentAudioDevice();

    double sampleRate = 48000.0;
    int blockSize = 256;

    if (device != nullptr)
    {
        sampleRate =
            device->getCurrentSampleRate();

        blockSize =
            device->getCurrentBufferSizeSamples();
    }

    juce::String error;

    auto instance =
        pluginFormatManager.createPluginInstance(
            description,
            sampleRate,
            blockSize,
            error);

    if (instance == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "Nie udało się otworzyć pluginu",
            description.name + "\n\n" + error);

        return;
    }

    instance->setNonRealtime(false);

    auto node =
        graph.addNode(std::move(instance));

    if (node == nullptr)
        return;

    pluginChain.add(node);

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();

    vstPanel->pluginChainList.selectRow(
        pluginChain.size() - 1);

    saveChain();

    markPresetDirty();
}

void MainComponent::refreshPluginBrowser()
{
    filteredPlugins.clear();

    const auto search =
        vstPanel->pluginSearch
            .getText()
            .trim()
            .toLowerCase();

    const juce::ScopedLock lock(
        pluginListLock);

    const auto& plugins =
        knownPluginList.getTypes();

    for (const auto& plugin : plugins)
    {
        juce::String searchable;

        searchable
            << plugin.name
            << " "
            << plugin.manufacturerName
            << " "
            << plugin.category;

        searchable =
            searchable.toLowerCase();

        if (search.isEmpty() ||
            searchable.contains(search))
        {
            filteredPlugins.add(plugin);
        }
    }

    std::sort(
        filteredPlugins.begin(),
        filteredPlugins.end(),
        [](const juce::PluginDescription& a,
           const juce::PluginDescription& b)
        {
            if (a.manufacturerName !=
                b.manufacturerName)
            {
                return a.manufacturerName
                    .compareIgnoreCase(
                        b.manufacturerName) < 0;
            }

            return a.name
                .compareIgnoreCase(
                    b.name) < 0;
        });

    vstPanel->pluginBrowserList.updateContent();
    vstPanel->pluginBrowserList.repaint();
}

void MainComponent::addPluginFromBrowser(int row)
{
    if (!juce::isPositiveAndBelow(
            row,
            filteredPlugins.size()))
        return;

    addPlugin(
        filteredPlugins.getReference(row));
}

void MainComponent::removeSelectedPlugin()
{
    const int row =
        vstPanel->pluginChainList.getSelectedRow();

    if (!juce::isPositiveAndBelow(
            row,
            pluginChain.size()))
        return;

    auto node = pluginChain[row];

    pluginChain.remove(row);

    if (node != nullptr)
    {
        logDestroyPlugin(node);

        graph.removeNode(node->nodeID);
    }

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();

    if (!pluginChain.isEmpty())
    {
        vstPanel->pluginChainList.selectRow(
            juce::jmin(
                row,
                pluginChain.size() - 1));
    }

    saveChain();

    markPresetDirty();
}

void MainComponent::setGlobalBypass(bool enabled)
{
    globalBypass.store(enabled);

    rebuildGraph();

    vstPanel->globalBypassButton.setToggleState(
        enabled,
        juce::dontSendNotification);

    saveState();
}

bool MainComponent::keyPressed(
    const juce::KeyPress& key)
{
    const bool ctrl =
        key.getModifiers().isCtrlDown();

    if (!ctrl)
        return false;

    const auto code =
        key.getKeyCode();

    if (code == 'M')
    {
        const bool newState =
            !vstPanel->muteButton.getToggleState();

        vstPanel->muteButton.setToggleState(
            newState,
            juce::sendNotification);

        return true;
    }

    if (code == 'B')
    {
        const bool newState =
            !vstPanel->globalBypassButton
                .getToggleState();

        vstPanel->globalBypassButton
            .setToggleState(
                newState,
                juce::sendNotification);

        return true;
    }

    if (code == 'R')
    {
        resetAudioButton.triggerClick();

        return true;
    }

    if (code == 'S')
    {
        vstPanel->savePresetButton.triggerClick();

        return true;
    }

    return false;
}

void MainComponent::toggleSelectedBypass()
{
    const int row =
        vstPanel->pluginChainList.getSelectedRow();

    if (!juce::isPositiveAndBelow(
            row,
            pluginChain.size()))
        return;

    auto node =
        pluginChain[row];

    if (node == nullptr)
        return;

    node->setBypassed(
        !node->isBypassed());

    vstPanel->pluginChainList.repaint();

    saveChain();

    markPresetDirty();
}

void MainComponent::moveSelectedPluginUp()
{
    const int row =
        vstPanel->pluginChainList.getSelectedRow();

    if (row <= 0 ||
        row >= pluginChain.size())
        return;

    pluginChain.swap(
        row,
        row - 1);

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();

    vstPanel->pluginChainList.selectRow(
        row - 1);

    saveChain();

    markPresetDirty();
}

void MainComponent::moveSelectedPluginDown()
{
    const int row =
        vstPanel->pluginChainList.getSelectedRow();

    if (row < 0 ||
        row >= pluginChain.size() - 1)
        return;

    pluginChain.swap(
        row,
        row + 1);

    rebuildGraph();

    vstPanel->pluginChainList.updateContent();

    vstPanel->pluginChainList.selectRow(
        row + 1);

    saveChain();

    markPresetDirty();
}

void MainComponent::openSelectedPluginEditor()
{
    const int row =
        vstPanel->pluginChainList.getSelectedRow();

    auto plugins =
        getPluginNodes();

    if (!juce::isPositiveAndBelow(
            row,
            plugins.size()))
        return;

    auto node =
        plugins[row];

    for (auto& window :
         pluginWindows)
    {
        if (window != nullptr &&
            window->node ==
                node)
        {
            window->setVisible(true);
            window->toFront(true);

            return;
        }
    }

    auto window =
        std::make_unique<PluginWindow>(
            node);

    pluginWindows.push_back(
        std::move(window));
}

void MainComponent::saveState()
{
    auto* properties =
        appProperties.getUserSettings();

    if (properties == nullptr)
        return;

    std::unique_ptr<juce::XmlElement>
        pluginXml(
            knownPluginList.createXml());

    if (pluginXml != nullptr)
    {
        properties->setValue(
            "knownPlugins",
            pluginXml->toString());
    }

    std::unique_ptr<juce::XmlElement>
        deviceState(
            deviceManager.createStateXml());

    if (deviceState != nullptr)
    {
        properties->setValue(
            "audioDevice",
            deviceState->toString());
    }

    properties->setValue(
        "inputGain",
        inputGainSlider.getValue());

    properties->setValue(
        "outputGain",
        outputGainSlider.getValue());

    properties->setValue(
        "limiter",
        limiterButton.getToggleState());

    properties->setValue(
        "autoStart",
        getWindowsAutoStart());

    properties->setValue(
        "autoReconnect",
        autoReconnectEnabled);

    properties->setValue(
        "mute",
        vstPanel != nullptr
            ? vstPanel->muteButton
                  .getToggleState()
            : false);

    properties->setValue(
        "globalBypass",
        globalBypass.load());

    properties->setValue(
        "activeVstPreset",
        activeVstPresetId);

    properties->setValue(
        "defaultVstPreset",
        defaultVstPresetId);

    properties->setValue(
        "lastVstPreset",
        lastVstPresetId);

    properties->saveIfNeeded();
}

void MainComponent::loadState()
{
    auto* properties =
        appProperties.getUserSettings();

    if (properties == nullptr)
        return;

    auto pluginData =
        properties->getValue(
            "knownPlugins");

    if (pluginData.isNotEmpty())
    {
        auto xml =
            juce::parseXML(
                pluginData);

        if (xml != nullptr)
            knownPluginList.recreateFromXml(
                *xml);
    }

    auto deviceData =
        properties->getValue(
            "audioDevice");

    if (deviceData.isNotEmpty())
    {
        auto xml =
            juce::parseXML(
                deviceData);

        if (xml != nullptr)
        {
            deviceManager.initialise(
                1,
                2,
                xml.get(),
                true);
        }
    }

    const double savedInputGain =
        properties->getDoubleValue(
            "inputGain",
            0.0);

    const double savedOutputGain =
        properties->getDoubleValue(
            "outputGain",
            0.0);

    const bool savedLimiter =
        properties->getBoolValue(
            "limiter",
            true);

    const bool savedMute =
        properties->getBoolValue(
            "mute",
            false);

    const bool savedGlobalBypass =
        properties->getBoolValue(
            "globalBypass",
            false);

    preferredInputDevice =
        properties->getValue(
            "preferredInput");

    preferredOutputDevice =
        properties->getValue(
            "preferredOutput");

    autoReconnectEnabled =
        properties->getBoolValue(
            "autoReconnect",
            true);

    vstPanel->autoReconnectButton.setToggleState(
        autoReconnectEnabled,
        juce::dontSendNotification);

vstPanel->autoStartButton.setToggleState(
        getWindowsAutoStart(),
        juce::dontSendNotification);

    defaultVstPresetId =
        properties->getValue(
            "defaultVstPreset");

    activeVstPresetId.clear();

    lastVstPresetId =
        properties->getValue(
            "lastVstPreset");

    inputGainSlider.setValue(
        savedInputGain,
        juce::dontSendNotification);

    outputGainSlider.setValue(
        savedOutputGain,
        juce::dontSendNotification);

    limiterButton.setToggleState(
        savedLimiter,
        juce::dontSendNotification);

    vstPanel->muteButton.setToggleState(
        savedMute,
        juce::dontSendNotification);

    if (inputUtility != nullptr)
    {
        inputUtility->setGainDb(
            static_cast<float>(
                savedInputGain));
    }

    if (outputUtility != nullptr)
    {
        outputUtility->setGainDb(
            static_cast<float>(
                savedOutputGain));

        outputUtility->setLimiterEnabled(
            savedLimiter);

        outputUtility->setMuted(
            savedMute);
    }

    setGlobalBypass(savedGlobalBypass);

    vstPanel->pluginChainList.updateContent();
}

void MainComponent::writeStartupLog()
{
    const bool defaultExists =
        defaultVstPresetId.isNotEmpty() &&
        getPresetDirectory()
            .getChildFile(
                defaultVstPresetId)
            .withFileExtension(".xml")
            .existsAsFile();

    const juce::String defaultId =
        defaultVstPresetId.isEmpty()
        ? "-"
        : defaultVstPresetId;

    juce::String logText;

    logText += "STARTUP\n"
               + juce::String("defaultVstPresetId=")
               + defaultId + "\n"
               + juce::String("defaultPresetExists=")
               + (defaultExists ? "true" : "false")
               + "\n"
               + juce::String("restoringChain=false\n")
               + juce::String("pluginsInstantiated=")
               + juce::String(pluginChain.size())
               + "\n";

    juce::Logger::writeToLog(logText);

#if JUCE_WINDOWS

    auto dir =
        juce::File::getSpecialLocation(
            juce::File::
                userApplicationDataDirectory);

    dir = dir.getChildFile(
        "VSTMicHost");

    dir.createDirectory();

    dir.getChildFile(
        "startup_log.txt")
        .replaceWithText(
            logText);

#endif
}

void MainComponent::timerCallback()
{
    if (inputUtility != nullptr)
    {
        const float db =
            inputUtility->getPeakDb();

        inputMeterValue =
            juce::jlimit(
                0.0,
                1.0,
                static_cast<double>(
                    (db + 60.0f)
                    / 60.0f));

        inputMeter.repaint();
    }

    if (outputUtility != nullptr)
    {
        const float db =
            outputUtility->getPeakDb();

        outputMeterValue =
            juce::jlimit(
                0.0,
                1.0,
                static_cast<double>(
                    (db + 60.0f)
                    / 60.0f));

        outputMeter.repaint();
    }

    ++reconnectTimerCounter;

    if (reconnectTimerCounter >= 10)
    {
        reconnectTimerCounter = 0;

        tryReconnectAudioDevices();
    }

    if (pluginScanThread != nullptr &&
        pluginScanThread->isThreadRunning())
        return;

    auto* device =
        deviceManager
            .getCurrentAudioDevice();

    if (device == nullptr)
    {
        statusLabel.setText(
            "NO AUDIO DEVICE",
            juce::dontSendNotification);

        return;
    }

    const double sampleRate =
        device->getCurrentSampleRate();

    const int buffer =
        device->getCurrentBufferSizeSamples();

    const double bufferLatencyMs =
        sampleRate > 0
            ? (
                static_cast<double>(
                    buffer)
                / sampleRate
              ) * 1000.0
            : 0.0;

    juce::String status;

    status
        << device->getName()
        << " | "
        << juce::String(
            sampleRate,
            0)
        << " Hz"
        << " | "
        << buffer
        << " samples"
        << " | ~"
        << juce::String(
            bufferLatencyMs,
            2)
        << " ms buffer";

    statusLabel.setText(
        status,
        juce::dontSendNotification);
}

void MainComponent::paint(
    juce::Graphics& g)
{
    g.fillAll(
        juce::Colour(
            0xff181818));

    g.setColour(
        juce::Colour(
            0xff252525));

    g.fillRoundedRectangle(
        getLocalBounds()
            .toFloat()
            .reduced(8.0f),
        10.0f);
}

void MainComponent::resized()
{
    if (!isInitialised)
        return;

    auto area =
        getLocalBounds()
            .reduced(16);

    auto statusArea =
        area.removeFromBottom(30);

    statusLabel.setBounds(
        statusArea);

    area.removeFromBottom(6);

    auto left =
        area.removeFromLeft(440);

    auto controls =
        left.removeFromBottom(230);

    deviceSelector->setBounds(
        left.reduced(4));

    inputGainLabel.setBounds(
        controls.removeFromTop(22));

    inputGainSlider.setBounds(
        controls.removeFromTop(36));

    controls.removeFromTop(4);

    auto inputMeterRow =
        controls.removeFromTop(25);

    inputMeterLabel.setBounds(
        inputMeterRow
            .removeFromLeft(65));

    inputMeter.setBounds(
        inputMeterRow);

    controls.removeFromTop(8);

    outputGainLabel.setBounds(
        controls.removeFromTop(22));

    outputGainSlider.setBounds(
        controls.removeFromTop(36));

    controls.removeFromTop(4);

    auto outputMeterRow =
        controls.removeFromTop(25);

    outputMeterLabel.setBounds(
        outputMeterRow
            .removeFromLeft(65));

    outputMeter.setBounds(
        outputMeterRow);

    controls.removeFromTop(8);

    auto utilityRow =
        controls.removeFromTop(34);

    limiterButton.setBounds(
        utilityRow
            .removeFromLeft(160));

    resetAudioButton.setBounds(
        utilityRow
            .removeFromLeft(130)
            .reduced(2));

    area.removeFromLeft(12);

    vstPanel->setBounds(area);

}
